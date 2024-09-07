#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum HookState_e_
{
    HOOK_STATE_UNINITIALIZED,
    HOOK_STATE_CLOSED,
    HOOK_STATE_PARTIALLY_CLOSED,
    HOOK_STATE_MID,
    HOOK_STATE_PARTIALLY_OPEN,
    HOOK_STATE_OPEN,
    HOOK_STATE_ERROR,
} HookState_e;

typedef enum HookTarget_e_
{
    HOOK_TARGET_HOMING,
    HOOK_TARGET_CLOSED,
    HOOK_TARGET_MID,
    HOOK_TARGET_OPEN,
} HookTarget_e;

typedef enum Errors_e_
{
    ERROR_NONE,
    ERROR_INVALID_PARAMETER,
    ERROR_FAILED_TO_START_MOTOR,
    ERROR_OVERLOAD,
    ERROR_COMMAND_TIMEOUT,
    ERROR_INVALID_SEQUENCE_NUMBER,
    ERROR_ESTOP,
    ERROR_PROTECTION_ACTIVATED,
    ERROR_MOTOR_JAMMED,

} Errors_e;

typedef enum DataSource_e_
{
    SOURCE_SPIN3204 = 0,
    SOURCE_LOADMASTER = 1,
} DataSource_e;

typedef enum CurrentLimitValues_e_
{
    CURRENT_LIMIT_RECOVERY = 0,
    CURRENT_LIMIT_OPERATION,
} CurrentLimitValues_e;

void database_run(void);

int16_t database_getHomingSpeed(void);
bool database_setHomingSpeed(int16_t);
int16_t database_getClosingSpeed(void);
bool database_setClosingSpeed(int16_t);
int16_t database_getOpeningSpeed(void);
bool database_setOpeningSpeed(int16_t speed);
uint8_t database_getReplySeqNo(void);
uint8_t database_getNextSeqNo(void);

uint16_t database_convertTargetToValue(HookTarget_e);
HookState_e database_getState(void);
uint8_t database_isAtEndStroke(void);
bool database_isProtectionTriggered(void);
bool database_isPositionEncoderHome(void);
uint16_t database_getCurrentAt(CurrentLimitValues_e value);

int16_t database_getCurrent(void);
uint16_t database_getVoltage(void);
uint8_t database_getError(void);
void database_setError(Errors_e error);
void database_eackError(void);
uint8_t database_isReadyForLifting(void);

bool database_ignoreProtection(void);
void database_advanceProtectionRecovery(void);
bool database_requestEnableRecovery(void);
void database_resetPosition(void);
void database_printHookPosition(void);
bool database_isStopped(void);

#endif