#ifndef _REMOTE_H_
#define _REMOTE_H_

#include <stdint.h>

void remote_init(void);
void remote_updateUi(void);
void remote_updateButtons(uint32_t button);

#endif