#include "spin3204_control.h"
#include <zephyr/kernel.h>

extern void sendBLE(uint8_t *data, uint8_t len);

#define TX_BUFFER_LENGTH 64

typedef enum Command_e_
{
    SPIN_COMMAND_NONE,
    SPIN_COMMAND_MOVE,
    // Reserved from 1 to 9
    SPIN_COMMAND_STOP = 10,
    SPIN_COMMAND_EACK,
    SPIN_COMMAND_REBOOT,
    SPIN_COMMAND_SET_PARAMETER,
    SPIN_COMMAND_READ_PARAMETER,
    SPIN_COMMAND_READY_FOR_LOADING,
} Command_e;

#pragma pack(1)
typedef struct RemoteCommand_t_
{
    uint8_t operation;
    int16_t Parameter1;
    int16_t Parameter2;
    int16_t Parameter3;

} RemoteCommand_t;

typedef enum Parameters_e_
{
    PARAMETER_NONE,
    PARAMETER_KP,
    PARAMETER_KI,
    PARAMETER_KD,
    PARAMETER_PID_SCALING_SHIFT,
    PARAMETER_PID_OUTPUT_MIN,
    PARAMETER_PID_OUTPUT_MAX,
    PARAMETER_CURRENT_LIMIT_VALUE,
    PARAMETER_CURRENT_LIMIT_TYPE,
    PARAMETER_CURRENT_LIMIT_ADC_FILTER_VALUE,

} Parameters_e;

static bool sendRemoteRequest(uint8_t *data, uint8_t length);

void mc_moveTo(int16_t target, int16_t speed, MotorControllerMode_e mode)
{
    RemoteCommand_t cmd = {.operation = SPIN_COMMAND_MOVE,
                           .Parameter1 = target,
                           .Parameter2 = speed,
                           .Parameter3 = (int16_t)mode};

    sendRemoteRequest((uint8_t *)&cmd, sizeof(RemoteCommand_t));
}

void mc_eack(void)
{
    RemoteCommand_t cmd = {.operation = SPIN_COMMAND_EACK,
                           .Parameter1 = 0,
                           .Parameter2 = 0,
                           .Parameter3 = 0};

    sendRemoteRequest((uint8_t *)&cmd, sizeof(RemoteCommand_t));
}

void mc_stop(void)
{
    RemoteCommand_t cmd = {.operation = SPIN_COMMAND_STOP,
                           .Parameter1 = 0,
                           .Parameter2 = 0,
                           .Parameter3 = 0};

    sendRemoteRequest((uint8_t *)&cmd, sizeof(RemoteCommand_t));
}

void mc_reboot(void)
{
    RemoteCommand_t cmd = {.operation = SPIN_COMMAND_REBOOT,
                           .Parameter1 = 0,
                           .Parameter2 = 0,
                           .Parameter3 = 0};

    sendRemoteRequest((uint8_t *)&cmd, sizeof(RemoteCommand_t));
}

static bool sendRemoteRequest(uint8_t *data, uint8_t length)
{
    if (length > TX_BUFFER_LENGTH)
        return true;
    uint8_t txBuffer[TX_BUFFER_LENGTH];
    txBuffer[0] = 0xFE;
    memcpy(&txBuffer[1], data, length);
    sendBLE(txBuffer, length + 1);

    return false;
}