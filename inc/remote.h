#ifndef _REMOTE_H_
#define _REMOTE_H_

#include <stdint.h>

void remote_init(void);
void remote_updateUi(void);
void remote_updateButtons(uint32_t button);
void comm_addToRadioBuffer(const uint8_t *const data, uint32_t length);
void remote_process(void);

#endif