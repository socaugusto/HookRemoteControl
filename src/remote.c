#include "remote.h"
#include "lcd_spiModule.h"
#include "adt_cbuffer.h"
#include "dk_buttons_and_leds.h"
#include "database.h"

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME remote
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define FAULT_LED 8

static uint8_t *stringError = "?    ";
// static uint8_t *stringUnknown = "UNKNOWN";
// static uint8_t *stringOpen = "OPEN";
// static uint8_t *stringMid = "MID";
// static uint8_t *stringClosed = "CLOSED";

static uint8_t *positionString = NULL;
static uint8_t faultLed = 0;

void remote_init(void)
{
    positionString = stringError;
}

void remote_updateButtons(uint32_t button)
{

    if (button == 16)
    {
        // cmd.close_pos = 1;
    }
    else if (button == 32)
    {
        // cmd.mid_pos = 1;
    }
    else if (button == 64)
    {
        // cmd.open_pos = 1;
    }
    // hook_remote_send_cmd(&cmd);
}

void remote_updateUi(void)
{
    float voltage = ((float)database_getVoltage() / 1000.0f);
    float current = ((float)database_getCurrent() / 1000.0f);

    lcd_set_cursor(1, 1);
    lcd_print(">Batt(V):%0.2f", voltage);
    lcd_clear_eol();
    lcd_set_cursor(2, 1);
    lcd_print(">Load(A):%0.2f", current);
    lcd_clear_eol();
    lcd_set_cursor(3, 1);
    lcd_send_string(">HookPos:");
    lcd_send_string(positionString);
    lcd_clear_eol();
    lcd_set_cursor(4, 1);
    lcd_print(">RSSI(dBm):%.0f", 0);
    lcd_clear_eol();

    if (faultLed)
    {
        dk_set_led_on(FAULT_LED);
    }
    else
    {
        dk_set_led_off(FAULT_LED);
    }
}
