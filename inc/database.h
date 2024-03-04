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

void database_run(void);

int16_t database_getHomingSpeed(void);
bool database_setHomingSpeed(int16_t);

uint16_t database_convertTargetToValue(HookTarget_e);
HookState_e database_getState(void);

int16_t database_getCurrent(void);
uint16_t database_getVoltage(void);
uint8_t database_getError(void);

#endif