#ifndef CRC32_H
#define CRC32_H
/**
 * Definition for hardware-accelerated CRC32 computation, using the Castagnoli
 * polynomial.
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <unistd.h>

/**
 * Compute a CRC-32C.  If the crc32 instruction is available, use the hardware
 * version.  Otherwise, use the software version.
 */
uint32_t crc32c(uint32_t crc, const void *buf, size_t len);

#ifdef __cplusplus
}
#endif
#endif
