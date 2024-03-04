#ifndef _COMMUNICATIONS_H_
#define _COMMUNICATIONS_H_

#include <stdint.h>

void comm_init(void);

void comm_addToMotorBuffer(const uint8_t *const data, uint32_t length);
void comm_removeFromMotorBuffer(uint8_t *buffer, uint32_t length);
uint32_t comm_getAvailableMotorDataLength(void);
uint8_t comm_peekFromMotorBuffer(void);

#endif