#include "remote.h"
#include "lcd_spiModule.h"
#include "dk_buttons_and_leds.h"
#include "commands.h"

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME remote
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define FAULT_LED 8
#define OPEN_LED 4
#define MID_LED 5
#define CLOSED_LED 6
#define BUTTON_EXECUTE_THRESHOLD 10
#define BUTTON_ESTOP_MASK 1
#define BUTTON_PARAMETER_MASK 2
#define BUTTON_CLOSE_MASK 16
#define BUTTON_MID_MASK 32
#define BUTTON_OPEN_MASK 64

static HookState_e hookState = HOOK_STATE_UNINITIALIZED;
static HookState_e hookStatePrevious = HOOK_STATE_UNINITIALIZED;
static uint8_t *stateMessage = NULL;
static uint8_t *positionString = NULL;

static uint8_t *stringError = "?";
static uint8_t *stringOpen = "OPEN";
static uint8_t *stringMid = "MID";
static uint8_t *stringClosed = "CLOSED";
static uint8_t *stringHome = ">Press any to reset";
static uint8_t *stringHomeAction = ">Seek end of stroke";
static uint8_t *stringEack = ">Press any to clear";
static uint8_t *stringEstop = ">E-STOP Activated";
static uint8_t *stringProgressOpen = "Opening...";
static uint8_t *stringProgressClose = "Closing...";

static uint8_t faultLed = 0;
static uint32_t buttonsPressed = 0;
static uint32_t buttonsExecute = 0;
static uint32_t closeButton = 0;
static uint32_t midButton = 0;
static uint32_t openButton = 0;
static int32_t rssiValue = 0;

static void updateButtons(void);
static void stateMachine(void);

void remote_init(void)
{
    stateMessage = stringError;
}

void remote_updateButtons(uint32_t button_state, uint32_t has_changed)
{
    buttonsPressed = button_state;

    if (button_state & BUTTON_ESTOP_MASK)
    {
        buttonsExecute |= BUTTON_ESTOP_MASK;
        LOG_INF("Execute button %d", buttonsExecute);
    }
    else
    {
        buttonsExecute &= ~BUTTON_ESTOP_MASK;
        // Buttons is released
        if (!(button_state & BUTTON_CLOSE_MASK) && (has_changed & BUTTON_CLOSE_MASK))
        {
            // If filter is big enough
            if (closeButton > BUTTON_EXECUTE_THRESHOLD)
            {
                buttonsExecute |= BUTTON_CLOSE_MASK;
                LOG_INF("Execute button %d", buttonsExecute);
            }
            closeButton = 0;
        }

        if (!(button_state & BUTTON_MID_MASK) && (has_changed & BUTTON_MID_MASK))
        {
            // If filter is big enough
            if (midButton > BUTTON_EXECUTE_THRESHOLD)
            {
                buttonsExecute |= BUTTON_MID_MASK;
                LOG_INF("Execute button %d", buttonsExecute);
            }
            midButton = 0;
        }

        if (!(button_state & BUTTON_OPEN_MASK) && (has_changed & BUTTON_OPEN_MASK))
        {
            // If filter is big enough
            if (openButton > BUTTON_EXECUTE_THRESHOLD)
            {
                buttonsExecute |= BUTTON_OPEN_MASK;
                LOG_INF("Execute button %d", buttonsExecute);
            }
            openButton = 0;
        }
    }
}

void remote_run(void)
{
    updateButtons();
    stateMachine();
}

void remote_setRssi(int8_t rssi)
{
    rssiValue = rssi;
}

void remote_disconnectedUi(void)
{
    hookState = HOOK_STATE_UNINITIALIZED;
    command_flush();
    lcd_set_cursor(1, 1);
    lcd_send_string(">Disconnected!");
    lcd_clear_eol();

    lcd_set_cursor(2, 1);
    lcd_send_string(">Searching for hook");
    lcd_clear_eol();

    lcd_set_cursor(3, 1);
    lcd_clear_eol();
    lcd_set_cursor(4, 1);
    lcd_clear_eol();

    dk_set_led_off(CLOSED_LED);
    dk_set_led_off(OPEN_LED);
    dk_set_led_off(MID_LED);
}

void remote_updateUi(void)
{
    float voltage = ((float)database_getVoltage() / 1000.0f);
    //    float current = ((float)database_getCurrent() / 1000.0f);

    lcd_set_cursor(1, 1);
    lcd_print(">RSSI(dBm):%.0f", rssiValue);
    lcd_clear_eol();
    lcd_set_cursor(2, 1);
    lcd_print(">Batt(V):%0.2f", voltage);
    lcd_clear_eol();

    switch (hookState)
    {
    case HOOK_STATE_UNINITIALIZED:

        break;
    case HOOK_STATE_CLOSED:
        dk_set_led_on(CLOSED_LED);
        dk_set_led_off(OPEN_LED);
        dk_set_led_off(MID_LED);
        positionString = stringClosed;

        break;
    case HOOK_STATE_PARTIALLY_CLOSED:
        if (hookStatePrevious == HOOK_STATE_CLOSED)
        {
            positionString = stringProgressOpen;
        }
        else if (hookStatePrevious == HOOK_STATE_MID)
        {
            positionString = stringProgressClose;
        }

        break;
    case HOOK_STATE_MID:
        dk_set_led_off(CLOSED_LED);
        dk_set_led_off(OPEN_LED);
        dk_set_led_on(MID_LED);
        positionString = stringMid;

        break;
    case HOOK_STATE_PARTIALLY_OPEN:
        if (hookStatePrevious == HOOK_STATE_MID)
        {
            positionString = stringProgressOpen;
        }
        else if (hookStatePrevious == HOOK_STATE_OPEN)
        {
            positionString = stringProgressClose;
        }

        break;
    case HOOK_STATE_OPEN:
        dk_set_led_off(CLOSED_LED);
        dk_set_led_on(OPEN_LED);
        dk_set_led_off(MID_LED);
        positionString = stringOpen;

        break;
    default:
    case HOOK_STATE_ERROR:
        positionString = stringError;
        dk_set_led_off(CLOSED_LED);
        dk_set_led_off(OPEN_LED);
        dk_set_led_off(MID_LED);

        break;
    }

    lcd_set_cursor(3, 1);
    if (database_getError())
    {
        lcd_print(">Error Number:%.0f", database_getError());
    }
    else if (database_isReadyForLifting())
    {
        lcd_send_string(">READY FOR LOADING");
    }
    else if (hookState != HOOK_STATE_UNINITIALIZED)
    {
        lcd_send_string(">Hook:");
        lcd_send_string(positionString);
    }
    lcd_clear_eol();

    lcd_set_cursor(4, 1);
    if (stateMessage)
    {
        lcd_send_string(stateMessage);
    }
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

static void updateButtons(void)
{
    if (buttonsPressed & BUTTON_CLOSE_MASK)
    {
        ++closeButton;
    }
    if (buttonsPressed & BUTTON_MID_MASK)
    {
        ++midButton;
    }
    if (buttonsPressed & BUTTON_OPEN_MASK)
    {
        ++openButton;
    }
}

void remote_updateHookState(HookState_e state)
{
    hookState = state;
}

static void stateMachine(void)
{
    if (hookState != HOOK_STATE_UNINITIALIZED)
    {
        hookState = database_getError() ? HOOK_STATE_ERROR : database_getState();
    }
    else if (buttonsExecute & BUTTON_ESTOP_MASK)
    {
        database_setError(ERROR_ESTOP);
        hookState = HOOK_STATE_ERROR;
    }

    switch (hookState)
    {
    case HOOK_STATE_UNINITIALIZED:
        stateMessage = stringHome;
        uint32_t buttons = (buttonsExecute & (BUTTON_CLOSE_MASK | BUTTON_MID_MASK | BUTTON_OPEN_MASK));

        if (buttons && !command_isInExecution())
        {
            CommandInput_t cmd = {.operation = COMMAND_HOMING};
            command_addToBuffer(&cmd);
            LOG_INF("Executing homing...");
        }
        else if (command_isInExecution())
        {
            stateMessage = stringHomeAction;
        }
        faultLed = 0;
        buttonsExecute = 0;
        hookStatePrevious = HOOK_STATE_UNINITIALIZED;

        break;
    case HOOK_STATE_CLOSED:
        stateMessage = NULL;
        if ((buttonsExecute & BUTTON_MID_MASK) && !command_isInExecution())
        {
            CommandInput_t cmd = {.operation = COMMAND_HOOK_MID_OPEN};
            command_addToBuffer(&cmd);
            LOG_INF("Executing going to mid...");
        }
        else if ((buttonsExecute & BUTTON_OPEN_MASK) && !command_isInExecution())
        {
            CommandInput_t cmd = {.operation = COMMAND_HOOK_OPEN};
            command_addToBuffer(&cmd);
            LOG_INF("Executing going to open...");
        }
        else if (command_isInExecution())
        {
        }
        buttonsExecute = 0;
        hookStatePrevious = HOOK_STATE_CLOSED;

        break;
    case HOOK_STATE_PARTIALLY_CLOSED:

        break;
    case HOOK_STATE_MID:
        stateMessage = NULL;
        if ((buttonsExecute & BUTTON_CLOSE_MASK) && !command_isInExecution())
        {
            CommandInput_t cmd = {.operation = COMMAND_HOOK_CLOSE};
            command_addToBuffer(&cmd);
            LOG_INF("Executing going to closed...");
        }
        else if ((buttonsExecute & BUTTON_OPEN_MASK) && !command_isInExecution())
        {
            CommandInput_t cmd = {.operation = COMMAND_HOOK_OPEN};
            command_addToBuffer(&cmd);
            LOG_INF("Executing going to open...");
        }
        else if (command_isInExecution())
        {
        }
        buttonsExecute = 0;
        hookStatePrevious = HOOK_STATE_MID;

        break;
    case HOOK_STATE_PARTIALLY_OPEN:

        break;
    case HOOK_STATE_OPEN:
        stateMessage = NULL;
        if ((buttonsExecute & BUTTON_MID_MASK) && !command_isInExecution())
        {
            CommandInput_t cmd = {.operation = COMMAND_HOOK_MID_CLOSE};
            command_addToBuffer(&cmd);
            LOG_INF("Executing going to mid...");
        }
        else if ((buttonsExecute & BUTTON_CLOSE_MASK) && !command_isInExecution())
        {
            CommandInput_t cmd = {.operation = COMMAND_HOOK_CLOSE};
            command_addToBuffer(&cmd);
            LOG_INF("Executing going to closed...");
        }
        else if (command_isInExecution())
        {
        }
        buttonsExecute = 0;
        hookStatePrevious = HOOK_STATE_OPEN;

        break;
    case HOOK_STATE_ERROR:
        stateMessage = stringEack;

        if (buttonsExecute & BUTTON_ESTOP_MASK)
        {
            stateMessage = stringEstop;
        }
        else if (buttonsExecute && !command_isInExecution())
        {
            CommandInput_t cmd = {.operation = COMMAND_EACK};
            command_addToBuffer(&cmd);
            LOG_INF("Executing eack...");
        }
        else if (command_isInExecution())
        {
            stateMessage = NULL;
        }

        if (!(buttonsExecute & BUTTON_ESTOP_MASK))
        {
            buttonsExecute = 0;
        }
        hookStatePrevious = HOOK_STATE_ERROR;
    default:
        faultLed = 1;

        break;
    }
}