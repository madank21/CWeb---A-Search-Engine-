#ifndef CWEB_CRC32_H
#define CWEB_CRC32_H

#include <stddef.h>
#include <stdint.h>

uint32_t crc32_calculate(const void *data, size_t length);

#endif /* CWEB_CRC32_H */
