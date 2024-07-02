#ifndef _MOTOR_CONTROLLER_H_
#define _MOTOR_CONTROLLER_H_

#include <stdint.h>

typedef enum Parameters_e_
{
    PARAMETER_NONE,
    PARAMETER_KP,
    PARAMETER_KI,
    PARAMETER_KD,
    PARAMETER_PID_SCALING_SHIFT,
    PARAMETER_PID_OUTPUT_MIN,
    PARAMETER_PID_OUTPUT_MAX,
    PARAMETER_CURRENT_LIMIT_VALUE,
    PARAMETER_CURRENT_LIMIT_TYPE,
    PARAMETER_CURRENT_LIMIT_ADC_FILTER_VALUE,
    PARAMETER_CURRENT_PIN_CONFIG,
    PARAMETER_IGNORE_SENSOR,

} Parameters_e;

void mc_moveTo(int16_t target, int16_t speed, uint8_t seqNo);
void mc_setPositionHome(void);
void mc_setPositionUninitialized(void);
void mc_eack(void);
void mc_stop(void);
void mc_reboot(void);
void mc_setIgnoreSensorParameter(uint8_t ignore);
void mc_setCurrentLimitParameter(uint16_t value);
void mc_readParameter(Parameters_e number);

#endif