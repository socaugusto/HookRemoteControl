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
    SPIN_COMMAND_SET_POSITION,
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

static bool sendRemoteRequest(uint8_t *data, uint8_t length);

void mc_moveTo(int16_t target, int16_t speed, uint8_t seqNo)
{
    RemoteCommand_t cmd = {.operation = SPIN_COMMAND_MOVE + seqNo,
                           .Parameter1 = target,
                           .Parameter2 = speed,
                           .Parameter3 = 0};

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

void mc_setPositionHome(void)
{
    RemoteCommand_t cmd = {.operation = SPIN_COMMAND_SET_POSITION,
                           .Parameter1 = 0,
                           .Parameter2 = 0,
                           .Parameter3 = 0};

    sendRemoteRequest((uint8_t *)&cmd, sizeof(RemoteCommand_t));
}

void mc_setPositionUninitialized(void)
{
    RemoteCommand_t cmd = {.operation = SPIN_COMMAND_SET_POSITION,
                           .Parameter1 = INT16_MAX,
                           .Parameter2 = 0,
                           .Parameter3 = 0};

    sendRemoteRequest((uint8_t *)&cmd, sizeof(RemoteCommand_t));
}

void mc_setIgnoreSensorParameter(uint8_t ignore)
{
    if (ignore)
    {
        ignore = 1;
    }

    RemoteCommand_t cmd = {.operation = SPIN_COMMAND_SET_PARAMETER,
                           .Parameter1 = PARAMETER_IGNORE_SENSOR,
                           .Parameter2 = ignore,
                           .Parameter3 = 0};

    sendRemoteRequest((uint8_t *)&cmd, sizeof(RemoteCommand_t));
}

void mc_readParameter(Parameters_e number)
{
    RemoteCommand_t cmd = {.operation = SPIN_COMMAND_READ_PARAMETER,
                           .Parameter1 = number,
                           .Parameter2 = 0,
                           .Parameter3 = 0};

    sendRemoteRequest((uint8_t *)&cmd, sizeof(RemoteCommand_t));
}

void mc_setCurrentLimitParameter(uint16_t value)
{

    RemoteCommand_t cmd = {.operation = SPIN_COMMAND_SET_PARAMETER,
                           .Parameter1 = PARAMETER_CURRENT_LIMIT_VALUE,
                           .Parameter2 = value,
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