#include "crc32.h"

static uint32_t crc32_table[256];
static int table_initialized = 0;

static void init_crc32_table(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) {
            if (c & 1) {
                c = 0xEDB88320L ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }
        crc32_table[i] = c;
    }
    table_initialized = 1;
}

uint32_t crc32_calculate(const void *data, size_t length) {
    if (!table_initialized) {
        init_crc32_table();
    }
    const uint8_t *buf = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFL;
    for (size_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFL;
}
