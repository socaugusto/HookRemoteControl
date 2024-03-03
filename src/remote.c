#include "remote.h"
#include "lcd_spiModule.h"

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
typedef struct _hook_remote_cmd_t
{

    uint8_t r_header[2];
    uint8_t open_pos;
    uint8_t mid_pos;
    uint8_t close_pos;
    uint8_t cr, lf;

} hook_remote_cmd_t;

/**@brief structure for hook data. */
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

extern void sendBLE(uint8_t *data, uint8_t len);

void hook_remote_cmd_init(hook_remote_cmd_t *rcmd)
{

    rcmd->r_header[0] = HDR;
    rcmd->r_header[1] = R_HDR;
    rcmd->open_pos = 0;
    rcmd->mid_pos = 0;
    rcmd->close_pos = 0;
    rcmd->cr = CR;
    rcmd->lf = LF;
}

void hook_remote_send_cmd(hook_remote_cmd_t *rcmd)
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

void update_buttons(uint32_t button)
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

void update_ui(void)
{
    lcd_set_cursor(1, 1);
    lcd_print(">Batt(V):%0.2f", 23.25f);
    lcd_clear_eol();
    lcd_set_cursor(2, 1);
    lcd_print(">Load(A):%0.2f", 1.25f);
    lcd_clear_eol();
    lcd_set_cursor(3, 1);
    lcd_send_string(">HookPos:");
    lcd_send_string("Unknown");
    lcd_clear_eol();
    lcd_set_cursor(4, 1);
    lcd_print(">RSSI(dBm):%.0f", -25);
    lcd_clear_eol();
}
