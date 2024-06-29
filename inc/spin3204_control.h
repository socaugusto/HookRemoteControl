#ifndef _MOTOR_CONTROLLER_H_
#define _MOTOR_CONTROLLER_H_

#include <stdint.h>

void mc_moveTo(int16_t target, int16_t speed, uint8_t seqNo);
void mc_setPositionHome(void);
void mc_setPositionUninitialized(void);
void mc_eack(void);
void mc_stop(void);
void mc_reboot(void);
void mc_setIgnoreSensorParameter(uint8_t ignore);
void mc_setCurrentLimitParameter(uint16_t value);

#endif