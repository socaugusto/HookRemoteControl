#include "database.h"
#include "communications.h"
#include "encoding_checksum.h"
#include <memory.h>

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME database
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#pragma pack(1)
typedef struct stdReply_t_
{
    uint16_t voltage; // mV
    int16_t current;  // mA
    uint16_t position;
    uint8_t error;
    struct
    {
        uint8_t sequenceNumber : 3;
        uint8_t dataType : 1;
        uint8_t dataNumber : 4;
    } command;
    uint8_t dataValues[4];

} stdReply_t;

#define SIZE_OF_STDREPLY sizeof(stdReply_t)

#pragma pack(1)
typedef struct HookReply_t_
{
    uint8_t header;
    uint8_t type;
    stdReply_t data;
    uint16_t checksum;
} HookReply_t;

#define SIZE_OF_HOOKREPLY sizeof(HookReply_t)

typedef enum MotorDirection_e_
{

    CCW = -1,
    CW = 1,

} MotorDirection_e;

#define HOOK_HOMING_DIRECTION CW
#define HOOK_CLOSING_DIRECITON CW
#define HOOK_OPENING_DIRECTION CCW

uint16_t hookPosition = INT16_MAX;
static uint16_t voltage = 0;
static int16_t current = 0;
static Errors_e errorNo = ERROR_NONE;
static uint8_t sequenceNumber = 0;
static uint8_t source = 0;
static uint8_t id;
static uint8_t data[4];

static int16_t hommingSpeed = HOOK_HOMING_DIRECTION * 750;
static int16_t closingSpeed = HOOK_CLOSING_DIRECITON * 1200;
static int16_t openingSpeed = HOOK_OPENING_DIRECTION * 1200;

static uint16_t homingPosition = 0;
static uint16_t closedPosition = 1;
static uint16_t midPosition = 5000;
static uint16_t openPosition = 13200;

static uint32_t readyForLiftingTimer = 0;

void database_run(void)
{
    uint32_t count = comm_getAvailableMotorDataLength();
    HookReply_t reply;

    while (count >= sizeof(HookReply_t))
    {
        if (0xFE == comm_peekFromMotorBuffer())
        {
            comm_removeFromMotorBuffer((uint8_t *)&reply, sizeof(HookReply_t));
            count -= sizeof(HookReply_t);

            uint16_t fcs = encoding_calculateFletcher16Checksum((uint8_t *)&reply, sizeof(HookReply_t) - sizeof(uint16_t));
            if (fcs == reply.checksum)
            {
                hookPosition = reply.data.position;
                voltage = reply.data.voltage;
                current = reply.data.current;
                database_setError(reply.data.error);
                sequenceNumber = reply.data.command.sequenceNumber;
                source = reply.data.command.dataType;

                switch (source)
                {
                case 1:
                    id = reply.data.command.dataNumber;
                    memcpy(data, reply.data.dataValues, sizeof(data));
                    readyForLiftingTimer = *((uint32_t *)data);

                    LOG_INF("Timer Value %d", readyForLiftingTimer);

                case 0:
                default:
                    id = reply.data.command.dataNumber;
                    memcpy(data, reply.data.dataValues, sizeof(data));

                    break;
                }
            }
            else
            {
                LOG_INF("Invalid checksum %d != %d", fcs, reply.checksum);
            }
        }
        else
        {
            uint8_t discard;
            comm_removeFromMotorBuffer(&discard, 1);
            --count;
            LOG_INF("Discarded byte %x", discard);
        }
    }
}

HookState_e database_getState(void)
{
    HookState_e result = HOOK_STATE_UNINITIALIZED;

    if ((hookPosition <= closedPosition) || (hookPosition & 0x8000))
    {
        result = HOOK_STATE_CLOSED;
    }
    else if (hookPosition > closedPosition && hookPosition < midPosition)
    {
        result = HOOK_STATE_PARTIALLY_CLOSED;
    }
    else if (hookPosition == midPosition)
    {
        result = HOOK_STATE_MID;
    }
    else if (hookPosition > midPosition && hookPosition < openPosition)
    {
        result = HOOK_STATE_PARTIALLY_OPEN;
    }
    else if (hookPosition == openPosition)
    {
        result = HOOK_STATE_OPEN;
    }
    else if (hookPosition == INT16_MAX)
    {
        result = HOOK_STATE_UNINITIALIZED;
    }
    else
    {
        result = HOOK_STATE_ERROR;
    }

    return result;
}

uint8_t database_isAtEndStroke(void)
{
    return ((hookPosition & 0x8000U) > 0);
}

int16_t database_getHomingSpeed(void)
{
    return hommingSpeed;
}

bool database_setHomingSpeed(int16_t speed)
{
    hommingSpeed = speed;
    return false;
}

int16_t database_getClosingSpeed(void)
{
    return closingSpeed;
}

bool database_setClosingSpeed(int16_t speed)
{
    closingSpeed = speed;
    return false;
}

int16_t database_getOpeningSpeed(void)
{
    return openingSpeed;
}

bool database_setOpeningSpeed(int16_t speed)
{
    openingSpeed = speed;
    return false;
}

uint16_t database_convertTargetToValue(HookTarget_e target)
{
    uint16_t result = UINT16_MAX;

    switch (target)
    {
    case HOOK_TARGET_HOMING:
        result = homingPosition;

        break;
    case HOOK_TARGET_CLOSED:
        result = closedPosition;

        break;
    case HOOK_TARGET_MID:
        result = midPosition;

        break;
    case HOOK_TARGET_OPEN:
        result = openPosition;

        break;
    }

    return result;
}

int16_t database_getCurrent(void)
{
    return current;
}

uint16_t database_getVoltage(void)
{
    return voltage;
}

uint8_t database_getError(void)
{
    return errorNo;
}

void database_setError(Errors_e error)
{
    if (errorNo == ERROR_NONE)
    {
        errorNo = error;
    }
}

void database_eackError(void)
{
    errorNo = ERROR_NONE;
}

uint8_t database_getReplySeqNo(void)
{
    return sequenceNumber;
}

uint8_t database_getNextSeqNo(void)
{
    return ((sequenceNumber + 1) % 8);
}

uint8_t database_isReadyForLifting(void)
{
    uint8_t result = 0;

    if (readyForLiftingTimer)
    {
        result = 1;
        if (readyForLiftingTimer >= 1000)
        {
            readyForLiftingTimer = 500;
        }
        else if (readyForLiftingTimer >= 500)
        {
            readyForLiftingTimer = 0;
        }
        LOG_INF("Ready for lifting: %d", readyForLiftingTimer);
    }

    return result;
}