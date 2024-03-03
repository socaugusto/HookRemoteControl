#ifndef ADT_ADTCODES_H_
#define ADT_ADTCODES_H_

typedef enum
{
    ADT_OK,
    ADT_OVERFLOW,
    ADT_UNDERRUN,
    ADT_EMPTY,
    ADT_OUT_OF_SPACE,
    ADT_OUT_OF_BOUNDS,
    ADT_NOT_READY,
    ADT_ERROR,
    ADT_BUSY
} Adt_Result_e;

typedef enum
{
    ADT_UNINITIALIZED,
    ADT_INITIALIZED
} Adt_InitState_e;

#endif /* ADT_ADTCODES_H_ */
