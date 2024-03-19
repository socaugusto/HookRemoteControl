#ifndef CHECKSUM_H_
#define CHECKSUM_H_

#ifdef __cplusplus
extern "C" { // AUTO-EXTERN_C
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Fletcher checksum (wikipedia)
 *
 * @param data pointer to data
 * @param bytes number of bytes
 * @param seed gives the start values for the checksum
 *
 * The seed can be used to start the calculation on a different value than 0 or for combining
 * fletcher16 checksums of multiple sources. E.g. an array can be split and calculating the
 * checksum for the first half and using this as seed for the second half will give the same
 * result as a calculation for the entire array. This can be used to combine checksums for
 * different parts of a message e.g. the header and the payload.
 *
 * @return uint16_t checksum
 */
uint16_t encoding_calculateFletcher16ChecksumWithSeed(const uint8_t *data,
                                                      uint32_t bytes,
                                                      uint16_t seed);

/**
 * @brief Fletcher checksum without seed
 *
 * Same as flecher with seed set to 0
 *
 * @param data pointer to data
 * @param bytes number of bytes
 *
 * @return uint16_t checksum
 */
uint16_t encoding_calculateFletcher16Checksum(const uint8_t *data, uint32_t bytes);

/**
 * @brief Check if the received checksum data matches the received checksumed data
 *
 * @param rxData pointer to received data
 * @param bytes number of bytes
 * @param rxChecksum received checksum
 * @param seed starting point for checksum calculations
 * @return true if valid, false if not
 */
bool encoding_isFletcher16ChecksumValidWithSeed(const uint8_t *rxData,
                                                uint32_t bytes,
                                                uint16_t rxChecksum,
                                                uint16_t seed);

/**
 * @brief Check if the received checksum data matches the received checksumed data
 *
 * Same as validitiy check with seed set to 0
 *
 * @param rxData pointer to received data
 * @param bytes number of bytes
 * @param rxChecksum received checksum
 * @return true if valid, false if not
 */
bool encoding_isFletcher16ChecksumValid(const uint8_t *rxData, uint32_t bytes, uint16_t rxChecksum);

#ifdef __cplusplus
} // AUTO-EXTERN_C
#endif
#endif /* CHECKSUM_H_ */