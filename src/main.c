/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Service Client sample
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <sdc_hci_vs.h>
#include <sdc_hci_cmd_status_params.h>

#include <dk_buttons_and_leds.h>

#include <zephyr/settings/settings.h>

#include <zephyr/drivers/uart.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/logging/log.h>

#include "system.h"

#define LOG_MODULE_NAME central_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* UART payload buffer element size. */
#define STACKSIZE 1024
#define PRIORITY_BLE 5
#define PRIORITY_UI 7
#define PRIORITY_RSSI 13
#define UART_BUF_SIZE 20

#define KEY_PASSKEY_ACCEPT DK_BTN1_MSK
#define KEY_PASSKEY_REJECT DK_BTN2_MSK

#define NUS_WRITE_TIMEOUT K_MSEC(150)
#define UART_WAIT_FOR_BUF_DELAY K_MSEC(50)
#define UART_RX_TIMEOUT 50

#define CON_STATUS_LED 7

static const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));
static struct k_work_delayable uart_work;

static const struct device *lcd = DEVICE_DT_GET(DT_NODELABEL(lcd_spi));

static const struct gpio_dt_spec lcdcs = GPIO_DT_SPEC_GET(DT_NODELABEL(lcdcs),
														  gpios);

static K_SEM_DEFINE(nus_write_sem, 0, 1);
static K_SEM_DEFINE(lcd_ini_ok, 0, 1);
static K_SEM_DEFINE(rssi_sem, 0, 1);

struct uart_data_t
{
	void *fifo_reserved;
	uint8_t data[UART_BUF_SIZE];
	uint16_t len;
};

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

static struct bt_conn *default_conn;
static struct bt_nus_client nus_client;

static void ble_data_sent(struct bt_nus_client *nus, uint8_t err,
						  const uint8_t *const data, uint16_t len)
{
	ARG_UNUSED(nus);

	struct uart_data_t *buf;

	/* Retrieve buffer context. */
	buf = CONTAINER_OF(data, struct uart_data_t, data[0]);
	k_free(buf);

	k_sem_give(&nus_write_sem);

	if (err)
	{
		LOG_WRN("ATT error code: 0x%02X", err);
	}
}

static uint8_t ble_data_received(struct bt_nus_client *nus,
								 const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(nus);

	if (len == 12)
	{
		LOG_DBG("%.10s", data);
	}
	else
	{
		LOG_DBG("%s", data);
	}
	system_receiveUpdate(data, len);

	return BT_GATT_ITER_CONTINUE;
}

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);

	static size_t aborted_len;
	struct uart_data_t *buf;
	static uint8_t *aborted_buf;
	static bool disable_req;

	switch (evt->type)
	{
	case UART_TX_DONE:
		LOG_DBG("UART_TX_DONE");
		if ((evt->data.tx.len == 0) ||
			(!evt->data.tx.buf))
		{
			return;
		}

		if (aborted_buf)
		{
			buf = CONTAINER_OF(aborted_buf, struct uart_data_t,
							   data[0]);
			aborted_buf = NULL;
			aborted_len = 0;
		}
		else
		{
			buf = CONTAINER_OF(evt->data.tx.buf,
							   struct uart_data_t,
							   data[0]);
		}

		k_free(buf);

		buf = k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
		if (!buf)
		{
			return;
		}

		if (uart_tx(uart, buf->data, buf->len, SYS_FOREVER_MS))
		{
			LOG_WRN("Failed to send data over UART");
		}

		break;

	case UART_RX_RDY:
		LOG_DBG("UART_RX_RDY");
		buf = CONTAINER_OF(evt->data.rx.buf, struct uart_data_t, data[0]);
		buf->len += evt->data.rx.len;

		if (disable_req)
		{
			return;
		}

		if ((evt->data.rx.buf[buf->len - 1] == '\n') ||
			(evt->data.rx.buf[buf->len - 1] == '\r'))
		{
			disable_req = true;
			uart_rx_disable(uart);
		}

		break;

	case UART_RX_DISABLED:
		LOG_DBG("UART_RX_DISABLED");
		disable_req = false;

		buf = k_malloc(sizeof(*buf));
		if (buf)
		{
			buf->len = 0;
		}
		else
		{
			LOG_WRN("Not able to allocate UART receive buffer");
			k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
			return;
		}

		uart_rx_enable(uart, buf->data, sizeof(buf->data),
					   UART_RX_TIMEOUT);

		break;

	case UART_RX_BUF_REQUEST:
		LOG_DBG("UART_RX_BUF_REQUEST");
		buf = k_malloc(sizeof(*buf));
		if (buf)
		{
			buf->len = 0;
			uart_rx_buf_rsp(uart, buf->data, sizeof(buf->data));
		}
		else
		{
			LOG_WRN("Not able to allocate UART receive buffer");
		}

		break;

	case UART_RX_BUF_RELEASED:
		LOG_DBG("UART_RX_BUF_RELEASED");
		buf = CONTAINER_OF(evt->data.rx_buf.buf, struct uart_data_t,
						   data[0]);

		if (buf->len > 0)
		{
			k_fifo_put(&fifo_uart_rx_data, buf);
		}
		else
		{
			k_free(buf);
		}

		break;

	case UART_TX_ABORTED:
		LOG_DBG("UART_TX_ABORTED");
		if (!aborted_buf)
		{
			aborted_buf = (uint8_t *)evt->data.tx.buf;
		}

		aborted_len += evt->data.tx.len;
		buf = CONTAINER_OF(aborted_buf, struct uart_data_t,
						   data[0]);

		uart_tx(uart, &buf->data[aborted_len],
				buf->len - aborted_len, SYS_FOREVER_MS);

		break;

	default:
		break;
	}
}

static void uart_work_handler(struct k_work *item)
{
	struct uart_data_t *buf;

	buf = k_malloc(sizeof(*buf));
	if (buf)
	{
		buf->len = 0;
	}
	else
	{
		LOG_WRN("Not able to allocate UART receive buffer");
		k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
		return;
	}

	uart_rx_enable(uart, buf->data, sizeof(buf->data), UART_RX_TIMEOUT);
}

static int uart_init(void)
{
	int err;
	struct uart_data_t *rx;

	if (!device_is_ready(uart))
	{
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	rx = k_malloc(sizeof(*rx));
	if (rx)
	{
		rx->len = 0;
	}
	else
	{
		return -ENOMEM;
	}

	k_work_init_delayable(&uart_work, uart_work_handler);

	err = uart_callback_set(uart, uart_cb, NULL);
	if (err)
	{
		return err;
	}

	return uart_rx_enable(uart, rx->data, sizeof(rx->data),
						  UART_RX_TIMEOUT);
}

static void discovery_complete(struct bt_gatt_dm *dm,
							   void *context)
{
	struct bt_nus_client *nus = context;
	LOG_INF("Service discovery completed");

	bt_gatt_dm_data_print(dm);

	bt_nus_handles_assign(dm, nus);
	bt_nus_subscribe_receive(nus);

	bt_gatt_dm_data_release(dm);
}

static void discovery_service_not_found(struct bt_conn *conn,
										void *context)
{
	LOG_INF("Service not found");
}

static void discovery_error(struct bt_conn *conn,
							int err,
							void *context)
{
	LOG_WRN("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found = discovery_error,
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;

	if (conn != default_conn)
	{
		return;
	}

	err = bt_gatt_dm_start(conn,
						   BT_UUID_NUS_SERVICE,
						   &discovery_cb,
						   &nus_client);
	if (err)
	{
		LOG_ERR("could not start the discovery procedure, error "
				"code: %d",
				err);
	}
}

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err)
	{
		LOG_INF("MTU exchange done");
	}
	else
	{
		LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err)
	{
		LOG_INF("Failed to connect to %s (%d)", addr, conn_err);

		if (default_conn == conn)
		{
			bt_conn_unref(default_conn);
			default_conn = NULL;

			err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
			if (err)
			{
				LOG_ERR("Scanning failed to start (err %d)",
						err);
			}
		}

		return;
	}

	dk_set_led_on(CON_STATUS_LED);
	LOG_INF("Connected: %s", addr);

	static struct bt_gatt_exchange_params exchange_params;

	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err)
	{
		LOG_WRN("MTU exchange failed (err %d)", err);
	}

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (err)
	{
		LOG_WRN("Failed to set security: %d", err);

		gatt_discover(conn);
	}

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY))
	{
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}

	k_sem_give(&rssi_sem);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", addr, reason);
	dk_set_led_off(CON_STATUS_LED);

	if (default_conn != conn)
	{
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err)
	{
		LOG_ERR("Scanning failed to start (err %d)",
				err);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
							 enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err)
	{
		LOG_INF("Security changed: %s level %u", addr, level);
	}
	else
	{
		LOG_WRN("Security failed: %s level %u err %d", addr,
				level, err);
	}

	gatt_discover(conn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed};

static void scan_filter_match(struct bt_scan_device_info *device_info,
							  struct bt_scan_filter_match *filter_match,
							  bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d",
			addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
							struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}

static int nus_client_init(void)
{
	int err;
	struct bt_nus_client_init_param init = {
		.cb = {
			.received = ble_data_received,
			.sent = ble_data_sent,
		}};

	err = bt_nus_client_init(&nus_client, &init);
	if (err)
	{
		LOG_ERR("NUS Client initialization failed (err %d)", err);
		return err;
	}

	LOG_INF("NUS Client module initialized");
	return err;
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
				scan_connecting_error, scan_connecting);

static int scan_init(void)
{
	int err;
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_NUS_SERVICE);
	if (err)
	{
		LOG_ERR("Scanning filters cannot be set (err %d)", err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err)
	{
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	LOG_INF("Scan module initialized");
	return err;
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_WRN("Pairing failed conn: %s, reason %d", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed};

static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(system_updateButtons);
	if (err)
	{
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

	err = dk_leds_init();
	if (err)
	{
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}

static int configure_spi(void)
{
	int err;

	err = gpio_pin_configure_dt(&lcdcs, GPIO_OUTPUT);
	if (err)
	{
		LOG_ERR("Cannot configure LCD CS gpio");
		return -ENODEV;
	}
	gpio_pin_set_dt(&lcdcs, 0);

	if (!device_is_ready(lcd))
	{
		LOG_ERR("SPI not initialized");
		return -ENODEV;
	}

	return 0;
}

int main(void)
{
	int err;

	configure_gpio();

	configure_spi();

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err)
	{
		LOG_ERR("Failed to register authorization callbacks.");
		return 0;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err)
	{
		printk("Failed to register authorization info callbacks.\n");
		return 0;
	}

	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return 0;
	}
	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS))
	{
		settings_load();
	}

	err = uart_init();
	if (err != 0)
	{
		LOG_ERR("uart_init failed (err %d)", err);
		return 0;
	}

	err = scan_init();
	if (err != 0)
	{
		LOG_ERR("scan_init failed (err %d)", err);
		return 0;
	}

	err = nus_client_init();
	if (err != 0)
	{
		LOG_ERR("nus_client_init failed (err %d)", err);
		return 0;
	}

	printk("**STAVENG TRANSFERA** \n");
	printk("-- Searching for slaves... \n");

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err)
	{
		LOG_ERR("Scanning failed to start (err %d)", err);
		return 0;
	}

	LOG_INF("Scanning successfully started");

	system_init(lcd, &lcdcs);
	k_sem_give(&lcd_ini_ok);

	for (;;)
	{
		system_thread();
		k_sleep(K_MSEC(25));
	}
}

void sendBLE(uint8_t *data, uint8_t len)
{
	for (uint16_t pos = 0; pos != len;)
	{
		struct uart_data_t *tx = k_malloc(sizeof(*tx));

		if (!tx)
		{
			LOG_WRN("Not able to allocate UART send data buffer");
			return;
		}

		if ((len - pos) > sizeof(tx->data))
		{
			tx->len = sizeof(tx->data);
		}
		else
		{
			tx->len = (len - pos);
		}

		memcpy(tx->data, &data[pos], tx->len);

		pos += tx->len;

		k_fifo_put(&fifo_uart_rx_data, tx);
	}
}

static void update_user_interface(void)
{
	k_sem_take(&lcd_ini_ok, K_FOREVER);

	for (;;)
	{
		struct bt_conn_info info = {.state = BT_CONN_STATE_DISCONNECTED};
		if (default_conn && bt_conn_get_info(default_conn, &info))
		{
			LOG_WRN("Error getting BT connection info");
		}

		uint8_t connectionState = 0;
		if (info.state == BT_CONN_STATE_CONNECTED)
		{
			connectionState = BT_CONN_STATE_CONNECTED;
		}
		system_updateUi(connectionState);
		k_sleep(K_MSEC(500));
	}
}

static void ble_write_thread(void)
{
	for (;;)
	{
		uint8_t log = 1;
		/* Wait indefinitely for data to be sent over Bluetooth */
		struct uart_data_t *buf = k_fifo_get(&fifo_uart_rx_data,
											 K_FOREVER);

		if (bt_nus_client_send(&nus_client, buf->data, buf->len))
		{
			LOG_WRN("Failed to send data over BLE connection");
		}

		while (k_sem_take(&nus_write_sem, NUS_WRITE_TIMEOUT))
		{
			if (log)
			{
				LOG_WRN("NUS send timeout");
				log = 0;
			}
		}
	}
}

void ble_rssi_thread(void)
{
	uint16_t conn_handle = 0;
	bt_hci_get_conn_handle(default_conn, &conn_handle);
	sdc_hci_cmd_sp_read_rssi_t p_param = {conn_handle};

	while (1)
	{
		k_sem_take(&rssi_sem, K_FOREVER);
		k_sleep(K_MSEC(2000));

		for (;;)
		{
			sdc_hci_cmd_sp_read_rssi_return_t p_return = {conn_handle, 0};
			uint8_t err = sdc_hci_cmd_sp_read_rssi(&p_param, &p_return);
			if (err)
			{
				LOG_WRN("Error %d Reading RSSI", err);
				break;
			}
			else
			{
				system_setRssi(p_return.rssi);
			}

			k_sleep(K_MSEC(1000));
		}
	}
}

K_THREAD_DEFINE(ble_thread_id, STACKSIZE, ble_write_thread, NULL, NULL, NULL, PRIORITY_BLE, 0, 0);

K_THREAD_DEFINE(ui_thread_id, STACKSIZE, update_user_interface, NULL, NULL,
				NULL, PRIORITY_UI, 0, 0);

K_THREAD_DEFINE(ble_rssi_thread_id, STACKSIZE, ble_rssi_thread, NULL, NULL,
				NULL, PRIORITY_RSSI, 0, 0);
