#include "communications.h"
#include "adt_cbuffer.h"

#define BUFFER_SIZE 1024
uint8_t motorBuffer[BUFFER_SIZE];
uint8_t motorPeek;

static Adt_CBuffer_t motor;

void comm_init(void)
{
    adt_cbuffer_init(&motor, motorBuffer, sizeof(uint8_t), BUFFER_SIZE);
}

void comm_addToMotorBuffer(const uint8_t *const data, uint32_t length)
{
    adt_cbuffer_push(&motor, data, length);
}

uint32_t comm_getAvailableMotorDataLength(void)
{
    return adt_cbuffer_getLength(&motor);
}

void comm_removeFromMotorBuffer(uint8_t *buffer, uint32_t length)
{
    adt_cbuffer_poll(&motor, buffer, length);
}

uint8_t comm_peekFromMotorBuffer(void)
{
    adt_cbuffer_peek(&motor, &motorPeek);
    return motorPeek;
}
