#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <stdint.h>

void system_init(const void *lcd_dev, const void *cs_dev);
void system_updateUi(void);
void system_thread(void);
void system_receiveUpdate(const uint8_t *data, uint32_t length);
void system_updateButtons(uint32_t button_state, uint32_t has_changed);
void system_setRssi(int8_t rssi);

#endif