#ifndef ADT_CBUFFER_H_
#define ADT_CBUFFER_H_

#include "adt_codes.h"
#include <stdint.h>

/*
 * Circular buffer ADT
 *
 * Data is designated in 8-bit chunks, adding a void*
 * will split the data up in uint8_t. It is expected
 * that all the data is aligned to tsize.
 */

/**
 * @brief Structure for cbuffer internals.
 */
typedef struct Adt_CBuffer_t_ Adt_CBuffer_t;

/**
 * @brief Initialize the cbuffer for use.
 *
 * @param[in] handle pointer to an allocated buffer struct.
 * @param[in] buffer pointer to an allocated data array of the size required (@ref length).
 * @param[in] size type size of a single element of the data array. Eg. sizeof(uint8_t).
 * @param[in] length number of elements in the data array.
 * @returns ADT_OK on success or ADT_ERROR on failure / bad parameters.
 */
Adt_Result_e adt_cbuffer_init(Adt_CBuffer_t *handle, void *buffer, uint8_t size, uint16_t length);

/**
 * @brief Length of the cbuffer (number of stored items).
 *
 * @param[in] handle pointer to an allocated and initialized buffer struct.
 * @returns number of elements in the cbuffer, or -1 if uninitialized or error.
 */
int32_t adt_cbuffer_getLength(const Adt_CBuffer_t *handle);

/**
 * @brief Push an item (or items) into the cbuffer.
 *
 * @param[in] handle pointer to an allocated and initialized buffer struct.
 * @param[in] buffer array of data to store in the cbuffer. The array elements must be partitioned
 * in chunks of typeSize, per the adt_cbuffer_init(...) parameter, or behavior is undefined.
 * @param[in] count number of items from the data array to store. Must be less than or equal to the
 * data array length and less than or equal to the available free space in the cbuffer.
 * @returns ADT_OK on success or ADT_ERROR/ADT_OVERFLOW on failure / bad parameters.
 */
Adt_Result_e adt_cbuffer_push(Adt_CBuffer_t *handle, const void *const buffer, uint16_t count);

/**
 * @brief Read an item (or items) from the cbuffer.
 *
 * This removes the items from the buffer and increases available space for writing.
 *
 * @param[in] handle pointer to an allocated and initialized buffer struct.
 * @param[out] buffer allocated array to store the read elements. The array elements must be
 * partitioned in chunks of typeSize, per the adt_cbuffer_init(...) parameter, or behavior
 * is undefined.
 * @param[in] count number of items to read from the buffer. Must be less than or equal to the
 * number of items stored in the cbuffer.
 * @returns ADT_OK on success or ADT_ERROR/ADT_UNDDERUN on failure / bad parameters.
 */
Adt_Result_e adt_cbuffer_poll(Adt_CBuffer_t *handle, void *buffer, uint16_t count);

/**
 * @brief Read a single item from the cbuffer.
 *
 * This peeks at the first item ready to be read from the buffer. This does not remove the item and
 * does not increases available space for writing.
 *
 * @param handle pointer to an allocated and initialized buffer struct.
 * @param buffer pointer to a variable to store the read element. The variable must match typeSize,
 * per the adt_cbuffer_init(...) parameter, or behavior is undefined.
 * @returns ADT_OK on success or ADT_ERROR/ADT_UNDDERUN on failure / bad parameters.
 */
Adt_Result_e adt_cbuffer_peek(Adt_CBuffer_t *handle, void *buffer);

/**
 * @brief Reset the cbuffer to default state.
 *
 * This clears the internal state of the cbuffer, allowing full write space to be available again.
 * It will not delete or clear actual memory in order for the call to be as fast as possible, but
 * the buffer will behave as if empty for subsequent length/peek/poll/write commands.
 *
 * @param handle pointer to an allocated and initialized buffer struct.
 * @returns ADT_OK on success or ADT_ERROR on failure.
 */
Adt_Result_e adt_cbuffer_reset(Adt_CBuffer_t *handle);

///
///
/// NOTE: Below structures and objects are private, and should not be used
///       outside of the API.
///
///

typedef struct Adt_CBuffer_t_
{
    uint8_t *dataIn;
    uint8_t *dataOut;
    uint8_t *buffer;

    Adt_InitState_e initState;

    uint16_t length;
    uint8_t dataTypeSize;
    uint8_t endOfBuffer;
} Adt_CBuffer_t;

#endif /* ADT_CBUFFER_H_ */
