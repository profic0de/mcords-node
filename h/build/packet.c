#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "packet.h"

void build_varint(Buffer *out, int value) {
    char temp[5];  // use char[] instead of uint8_t[] to avoid warnings
    int len = 0;

    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value != 0)
            byte |= 0x80;
        temp[len++] = (char)byte;
    } while (value != 0 && len < 5);

    append_to_buffer(out, temp, len);
}

void build_string(Buffer *out, const char *str) {
    int len = strlen(str);

    // Write length prefix
    build_varint(out, len);

    // Write UTF-8 bytes (without null terminator)
    append_to_buffer(out, str, len);
}

void build_integer(Buffer *out, int64_t value, int bytes, int is_signed) {
    char temp[8] = {0};

    if (is_signed) {
        for (int i = 0; i < bytes; i++) {
            temp[bytes - 1 - i] = (char)((value >> (i * 8)) & 0xFF);
        }
    } else {
        uint64_t uval = (uint64_t)value;
        for (int i = 0; i < bytes; i++) {
            temp[bytes - 1 - i] = (char)((uval >> (i * 8)) & 0xFF);
        }
    }

    append_to_buffer(out, temp, bytes);
}
