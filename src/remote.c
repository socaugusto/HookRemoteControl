#include "remote.h"
#include "lcd_spiModule.h"
#include "dk_buttons_and_leds.h"
#include "database.h"
#include "commands.h"

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME remote
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define FAULT_LED 8

static uint8_t *stringError = "?    ";
static uint8_t *stringUnknown = "UNKNOWN";
static uint8_t *stringOpen = "OPEN";
static uint8_t *stringMid = "MID";
static uint8_t *stringClosed = "CLOSED";

static uint8_t *positionString = NULL;
static uint8_t faultLed = 0;
static uint32_t buttonsPressed = 0;
static uint32_t closeButton = 0;
static uint32_t midButton = 0;
static uint32_t openButton = 0;
static int32_t rssiValue = 0;

void remote_init(void)
{
    positionString = stringError;
}

void remote_updateButtons(uint32_t button_state, uint32_t has_changed)
{
    buttonsPressed = button_state;

    // Buttons is released
    if (!(button_state & 16) && (has_changed & 16))
    {
        // If filter is big enough
        if (closeButton > 20)
        {
            CommandInput_t cmd = {.operation = COMMAND_HOOK_CLOSE};
            command_addToBuffer(&cmd);
        }
        closeButton = 0;
    }
    LOG_INF("Buttons pressed: %d", button_state);
}

void remote_run(void)
{
    if (buttonsPressed & 16)
    {
        ++closeButton;
    }
    if (buttonsPressed & 32)
    {
        ++midButton;
    }
    if (buttonsPressed & 64)
    {
        ++openButton;
    }
}

void remote_setRssi(int8_t rssi)
{
    rssiValue = rssi;
}

void remote_updateUi(void)
{
    float voltage = ((float)database_getVoltage() / 1000.0f);
    float current = ((float)database_getCurrent() / 1000.0f);

    switch (database_getState())
    {
    case HOOK_STATE_UNINITIALIZED:
        positionString = stringUnknown;
        break;
    case HOOK_STATE_CLOSED:
        positionString = stringClosed;

        break;
    case HOOK_STATE_PARTIALLY_CLOSED:
        positionString = stringMid;

        break;
    case HOOK_STATE_MID:
        positionString = stringMid;

        break;
    case HOOK_STATE_PARTIALLY_OPEN:
        positionString = stringMid;

        break;
    case HOOK_STATE_OPEN:
        positionString = stringOpen;

        break;
    default:
    case HOOK_STATE_ERROR:
        positionString = stringError;

        break;
    }

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
    lcd_print(">RSSI(dBm):%.0f", rssiValue);
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
