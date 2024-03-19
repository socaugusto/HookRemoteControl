
#include "encoding_checksum.h"

uint16_t encoding_calculateFletcher16Checksum(const uint8_t *data, uint32_t bytes)
{
    return encoding_calculateFletcher16ChecksumWithSeed(data, bytes, 0);
}

uint16_t encoding_calculateFletcher16ChecksumWithSeed(const uint8_t *data,
                                                      uint32_t bytes,
                                                      uint16_t seed)
{
    // Optimized version, as found on wikipedia
    if (data == nullptr || bytes == 0)
        return 0;

    uint32_t c0 = seed & 0xff;
    uint32_t c1 = (seed & 0xff00) >> 8;

    // Maximum number of uint8s that can be processed in one go without modulo
    // Found by solving for c1 overflow:
    // n > 0 and n * (n+1) / 2 * (2^8-1) < (2^32-1).
    static constexpr uint32_t max_blocklen = 5802;
    do
    {
        uint32_t blocklen = bytes > max_blocklen ? max_blocklen : bytes;
        bytes -= blocklen;
        do
        {
            c0 += *data++;
            c1 += c0;
        } while (--blocklen);
        c0 %= 255;
        c1 %= 255;
    } while (bytes > 0);
    return static_cast<uint16_t>(c1 << 8 | c0);
}

bool encoding_isFletcher16ChecksumValidWithSeed(const uint8_t *rxData,
                                                uint32_t bytes,
                                                uint16_t rxChecksum,
                                                uint16_t seed)
{
    return rxChecksum == encoding_calculateFletcher16ChecksumWithSeed(rxData, bytes, seed);
}

bool encoding_isFletcher16ChecksumValid(const uint8_t *rxData, uint32_t bytes, uint16_t rxChecksum)
{
    return encoding_isFletcher16ChecksumValidWithSeed(rxData, bytes, rxChecksum, 0);
}
