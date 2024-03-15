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

} Errors_e;

typedef enum DataSource_e_
{
    SOURCE_SPIN3204 = 0,
    SOURCE_LOADMASTER = 1,
} DataSource_e;

void database_run(void);

int16_t database_getHomingSpeed(void);
bool database_setHomingSpeed(int16_t);
int16_t database_getClosingSpeed(void);
bool database_setClosingSpeed(int16_t);
int16_t database_getOpeningSpeed(void);
bool database_setOpeningSpeed(int16_t speed);

uint16_t database_convertTargetToValue(HookTarget_e);
HookState_e database_getState(void);
uint8_t database_isAtEndStroke(void);

int16_t database_getCurrent(void);
uint16_t database_getVoltage(void);
uint8_t database_getError(void);
void database_setError(Errors_e error);
void database_eackError(void);
uint8_t database_isReadyForLifting(void);

#endif