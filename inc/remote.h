#ifndef _REMOTE_H_
#define _REMOTE_H_

#include <stdint.h>
#include "database.h"

void remote_init(void);
void remote_updateUi(void);
void remote_disconnectedUi(void);
void remote_updateButtons(uint32_t button_state, uint32_t has_changed);
void remote_updateHookState(HookState_e state);
void remote_run(void);
void remote_setRssi(int8_t rssi);
void remote_enableProtectionError(void);

#endif