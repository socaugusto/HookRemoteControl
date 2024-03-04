#ifndef _MOTOR_CONTROLLER_H_
#define _MOTOR_CONTROLLER_H_

#include <stdint.h>

typedef enum MotorControllerMode_e_
{
    MC_MODE_NONE,
    MC_MODE_CONSTANT_SPEED,
} MotorControllerMode_e;

void mc_moveTo(int16_t target, int16_t speed, MotorControllerMode_e mode);
void mc_eack(void);
void mc_stop(void);
void mc_reboot(void);

#endif