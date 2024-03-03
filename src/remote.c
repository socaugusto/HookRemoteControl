#include "remote.h"
#include "lcd_spiModule.h"
#include "adt_cbuffer.h"

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME remote
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

// Headers
#define HDR '$'
#define H_HDR 'H'
#define R_HDR 'R'
#define CR '\r'
#define LF '\n'

/**@brief structure for remote command data. */
#pragma pack(1)
typedef struct _hook_remote_cmd_t
{

    uint8_t r_header[2];
    uint8_t open_pos;
    uint8_t mid_pos;
    uint8_t close_pos;
    uint8_t cr, lf;

} hook_remote_cmd_t;

/**@brief structure for hook data. */
#pragma pack(1)
typedef struct _hook_data_t
{

    uint8_t h_header[2];
    float batt_voltage_m;
    float load_current_m;
    uint8_t open_pos_led;
    uint8_t mid_pos_led;
    uint8_t close_pos_led;
    uint8_t fault_led;
    uint8_t *lcd_hook_status;
    int8_t rssi;

} hook_data_t;

static uint8_t *stringError = "?    ";
static uint8_t *stringUnknown = "UNKNOWN";
static uint8_t *stringOpen = "OPEN";
static uint8_t *stringMid = "MID";
static uint8_t *stringClosed = "CLOSED";

static hook_data_t hd;
static Adt_CBuffer_t radio;
#define BUFFER_SIZE 1024
static uint8_t radioBuffer[BUFFER_SIZE];

extern void sendBLE(uint8_t *data, uint8_t len);
static void hook_data_get(const uint8_t *const data);

void remote_init(void)
{
    hd.lcd_hook_status = stringError;
    adt_cbuffer_init(&radio, radioBuffer, sizeof(uint8_t), BUFFER_SIZE);
}

void comm_addToRadioBuffer(const uint8_t *const data, uint32_t length)
{
    adt_cbuffer_push(&radio, data, length);
}

static void hook_data_get(const uint8_t *const data)
{
    uint8_t v1; // Voltage value before point xx from xx.yy (V)
    uint8_t v2; // Voltage value after point yy from xx.yy (V)
    uint8_t c1; // Current value before point xx from xx.yy (A)
    uint8_t c2; // Current value after point yy from xx.yy (A)

    if (data[0] == HDR && data[1] == H_HDR)
    {
        // Voltage formatting
        v1 = (uint8_t)data[2] - 48; // Convert data to int
        v2 = (uint8_t)data[3] - 48; // Convert data to int

        hd.batt_voltage_m = (float)((v1 * 100) + v2) / 100.0; // Battery voltage monitor value

        // Current formatting
        c1 = (uint8_t)data[4] - 48;
        c2 = (uint8_t)data[5] - 48;

        hd.load_current_m = (float)((c1 * 100) + c2) / 100.0; // Load current monitor value

        // LED indicator formatting
        hd.open_pos_led = (uint8_t)data[6] - 48;  // Open Pos LED indicator condition value
        hd.mid_pos_led = (uint8_t)data[7] - 48;   // Mid Pos LED indicator condition value
        hd.close_pos_led = (uint8_t)data[8] - 48; // Close Pos LED indicator condition value
        hd.fault_led = (uint8_t)data[9] - 48;     // Fault LED indicator condition value
    }

    if (hd.open_pos_led == 2 && hd.mid_pos_led == 2 && hd.close_pos_led == 2)
    {
        hd.lcd_hook_status = stringUnknown;
    }
    else if (hd.open_pos_led == 1 && hd.mid_pos_led == 0 && hd.close_pos_led == 0)
    {
        hd.lcd_hook_status = stringOpen;
    }
    else if (hd.open_pos_led == 0 && hd.mid_pos_led == 1 && hd.close_pos_led == 0)
    {
        hd.lcd_hook_status = stringMid;
    }
    else if (hd.open_pos_led == 0 && hd.mid_pos_led == 0 && hd.close_pos_led == 1)
    {
        hd.lcd_hook_status = stringClosed;
    }
    else
    {
        hd.lcd_hook_status = stringError;
    }
}

static void hook_remote_send_cmd(hook_remote_cmd_t *rcmd)
{
    uint8_t rdata[7];

    rdata[0] = rcmd->r_header[0];
    rdata[1] = rcmd->r_header[1];
    rdata[2] = rcmd->open_pos + '0';
    rdata[3] = rcmd->mid_pos + '0';
    rdata[4] = rcmd->close_pos + '0';
    rdata[5] = rcmd->cr;
    rdata[6] = rcmd->lf;

    LOG_INF("%s", rdata);

    sendBLE(rdata, sizeof(rdata));
}

void remote_updateButtons(uint32_t button)
{
    hook_remote_cmd_t cmd;
    cmd.r_header[0] = HDR;
    cmd.r_header[1] = R_HDR;
    cmd.cr = CR;
    cmd.lf = LF;
    cmd.mid_pos = 0;
    cmd.close_pos = 0;
    cmd.open_pos = 0;

    if (button == 16)
    {
        cmd.close_pos = 1;
    }
    else if (button == 32)
    {
        cmd.mid_pos = 1;
    }
    else if (button == 64)
    {
        cmd.open_pos = 1;
    }
    hook_remote_send_cmd(&cmd);
}

#define SIZE_OF_HOOK_DATA 12
void remote_process(void)
{

    while (adt_cbuffer_getLength(&radio) >= SIZE_OF_HOOK_DATA)
    {
        uint8_t data[SIZE_OF_HOOK_DATA];
        adt_cbuffer_poll(&radio, data, SIZE_OF_HOOK_DATA);
        hook_data_get(data);
    }
}

void remote_updateUi(void)
{
    lcd_set_cursor(1, 1);
    lcd_print(">Batt(V):%0.2f", hd.batt_voltage_m);
    lcd_clear_eol();
    lcd_set_cursor(2, 1);
    lcd_print(">Load(A):%0.2f", hd.load_current_m);
    lcd_clear_eol();
    lcd_set_cursor(3, 1);
    lcd_send_string(">HookPos:");
    lcd_send_string(hd.lcd_hook_status);
    lcd_clear_eol();
    lcd_set_cursor(4, 1);
    lcd_print(">RSSI(dBm):%.0f", 0);
    lcd_clear_eol();
}
