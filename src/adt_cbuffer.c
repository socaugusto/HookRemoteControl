#include "adt_cbuffer.h"
#include <string.h>

enum readMode
{
    CBUFFER_POLL,
    CBUFFER_PEEK
};

static Adt_Result_e read_cbuffer(Adt_CBuffer_t *handle,
                                 void *buffer,
                                 uint16_t count,
                                 enum readMode mode);

Adt_Result_e adt_cbuffer_init(Adt_CBuffer_t *handle, void *buffer, uint8_t size, uint16_t length)
{
    if (!handle || !buffer || !size || length < 2)
    {
        if (handle) // Set unintialized if possible.
        {
            handle->initState = ADT_UNINITIALIZED;
        }

        return ADT_ERROR;
    }

    handle->buffer = (uint8_t *)buffer;
    handle->dataTypeSize = size;
    handle->length = length;
    handle->endOfBuffer = 0;
    handle->dataIn = (uint8_t *)buffer;
    handle->dataOut = (uint8_t *)buffer;
    memset(handle->buffer, 0, size * length);

    handle->initState = ADT_INITIALIZED;
    return ADT_OK;
}

int32_t adt_cbuffer_getLength(const Adt_CBuffer_t *handle)
{
    if (!handle || handle->initState == ADT_UNINITIALIZED)
    {
        return -1;
    }

    // Full buffer
    if (handle->endOfBuffer)
    {
        return handle->length;
    }

    // Wrapped buffer
    if (handle->dataIn < handle->dataOut)
    {
        int32_t wrapSize = ((int32_t)(handle->dataOut - handle->dataIn)) / handle->dataTypeSize;
        return handle->length - wrapSize;
    }
    // Normal state
    else
    {
        return ((int32_t)(handle->dataIn - handle->dataOut)) / handle->dataTypeSize;
    }
}

/*
 * data can be array of multiple elements
 */
Adt_Result_e adt_cbuffer_push(Adt_CBuffer_t *handle, const void *const buffer, uint16_t count)
{
    // Assert
    if (!handle || !buffer || !count)
    {
        return ADT_ERROR;
    }
    if (handle->initState == ADT_UNINITIALIZED)
    {
        return ADT_NOT_READY;
    }

    uint32_t writeSize = handle->dataTypeSize * count;
    uint32_t bufferSize = adt_cbuffer_getLength(handle) * handle->dataTypeSize + writeSize;
    if (bufferSize > (uint32_t)(handle->dataTypeSize * handle->length))
    {
        return ADT_OVERFLOW;
    }
    if (bufferSize == (uint32_t)(handle->dataTypeSize * handle->length))
    {
        handle->endOfBuffer = 0x01; // end of buffer (in == out)
    }

    if (writeSize == 1)
    { // Save some resources if only pushing 1
        memcpy(handle->dataIn, buffer, writeSize);
        handle->dataIn += writeSize;
        if (handle->dataIn >= &handle->buffer[handle->length * handle->dataTypeSize])
        { // Wrap around
            handle->dataIn = handle->buffer;
        }
        return ADT_OK;
    }
    else
    { // Can be expensive if count is large
        uint32_t chunkSize = 0;
        if (handle->dataIn + writeSize <= &handle->buffer[handle->length * handle->dataTypeSize])
        { // No wraparound
            /*
             * no wraparoud
             * writeSize = 4
             *      o  i
             * [----====----]
             * = data
             * - no data
             */
            chunkSize = writeSize;
            memcpy(handle->dataIn, buffer, writeSize);
        }
        else
        {
            /*
             * wraparound
             * writeSize = 8
             *   c1 i  o c2
             * [====----====]
             *  b	       e
             * = data
             * - no data
             * c chunk
             */

            // len of first chunk e - o
            chunkSize = &handle->buffer[handle->length * handle->dataTypeSize] - handle->dataIn;
            memcpy(handle->dataIn,
                   buffer, // c2 o->e
                   chunkSize);
            // Move buffer pointer, cast to char for 8-bit alignment
            const char *alignedData = (const char *)buffer;
            alignedData += chunkSize;
            memcpy(handle->buffer,
                   alignedData, // c1 handle->i
                   writeSize - chunkSize);
        }
        handle->dataIn += writeSize;
        if (handle->dataIn >= &handle->buffer[handle->length * handle->dataTypeSize])
        { // Check wrap
            handle->dataIn = handle->buffer + (writeSize - chunkSize);
        }
    }

    return ADT_OK;
}

Adt_Result_e adt_cbuffer_poll(Adt_CBuffer_t *handle, void *buffer, uint16_t count)
{
    if (handle && buffer)
        return read_cbuffer(handle, buffer, count, CBUFFER_POLL);
    else
        return ADT_ERROR;
}

Adt_Result_e adt_cbuffer_peek(Adt_CBuffer_t *handle, void *buffer)
{
    if (handle && buffer)
        return read_cbuffer(handle, buffer, 1, CBUFFER_PEEK);
    else
        return ADT_ERROR;
}

Adt_Result_e adt_cbuffer_reset(Adt_CBuffer_t *handle)
{
    if (!handle || handle->initState == ADT_UNINITIALIZED)
        return ADT_ERROR;

    handle->endOfBuffer = 0;
    handle->dataIn = handle->buffer;
    handle->dataOut = handle->buffer;

    return ADT_OK;
}

static Adt_Result_e read_cbuffer(Adt_CBuffer_t *handle,
                                 void *buffer,
                                 uint16_t count,
                                 enum readMode mode)
{
    // Assert
    if (handle->initState == ADT_UNINITIALIZED)
    {
        return ADT_NOT_READY;
    }
    if (!count)
    {
        return ADT_ERROR;
    }
    uint32_t writeSize = handle->dataTypeSize * count;
    uint32_t bufferSize = adt_cbuffer_getLength(handle) * handle->dataTypeSize;
    if (bufferSize < writeSize)
    {
        if (!bufferSize)
        {
            return ADT_EMPTY;
        }
        return ADT_UNDERRUN;
    }

    if (writeSize == 1)
    { // Save some resources if only polling 1
        memcpy(buffer, handle->dataOut, 1);

        if (mode == CBUFFER_POLL)
        {                               // Move pointer only if polling
            handle->endOfBuffer = 0x00; // Reset EOB flag
            handle->dataOut += writeSize;
            if (handle->dataOut >= &handle->buffer[handle->length * handle->dataTypeSize])
            { // Wrap around
                handle->dataOut = handle->buffer;
            }
        }

        return ADT_OK;
    }
    else
    { // Can be expensive if count is large
        uint32_t chunkSize = 0;
        if (handle->dataOut + writeSize <= &handle->buffer[handle->length * handle->dataTypeSize])
        { // No wraparound
            chunkSize = writeSize;
            memcpy(buffer,
                   handle->dataOut, // Copy first section before wraparound
                   writeSize);
        }
        else
        {
            // len of first chunk e - o
            chunkSize = &handle->buffer[handle->length * handle->dataTypeSize] - handle->dataOut;
            memcpy(buffer,
                   handle->dataOut, // c2 o->e
                   chunkSize);
            // Move data pointer, cast to char for 8-bit alignment
            char *alignedBuffer = (char *)buffer;
            alignedBuffer += chunkSize;
            memcpy(alignedBuffer,
                   handle->buffer, // c1 buffer->i
                   writeSize - chunkSize);
        }

        if (mode == CBUFFER_POLL)
        {                               // Move pointer only if polling
            handle->endOfBuffer = 0x00; // Reset EOB flag
            handle->dataOut += writeSize;
            if (handle->dataOut >= &handle->buffer[handle->length * handle->dataTypeSize])
            {
                handle->dataOut = handle->buffer + (writeSize - chunkSize);
            }
        }

        return ADT_OK;
    }
}
