#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "vars.h"

int parse_varint(Buffer *buf, int *error) {
    int result = 0;
    int shift = 0;
    int consumed = 0;
    int finished = 0;

    for (int i = 0; i < 5 && i < buf->length; i++) {
        uint8_t byte = (uint8_t)buf->buffer[i];
        result |= (byte & 0x7F) << shift;
        consumed++;

        if ((byte & 0x80) == 0) {
            finished = 1;
            break;
        }
        shift += 7;
    }

    if (!finished || consumed > buf->length) {
        *error += 1;
        return 0;
    }

    cut_buffer(buf, -consumed);  // cut from beginning
    return result;
}

char *parse_string(Buffer *buf, int max_lenght ,int *error) {
    // *error = 0;
    int te = *error;
    int str_len = parse_varint(buf, error);
    if (str_len > buf->length || str_len <= 0 || str_len > max_lenght || *error-te > 0) {
        *error += 1;
        return NULL;
    }

    // Check for null byte inside string
    for (int i = 0; i < str_len; i++) {
        if (buf->buffer[i] == '\0') {
            *error += 1;
            return NULL;
        }
    }

    // Allocate and copy the string, add C null terminator
    char *str = malloc(str_len + 1);
    if (!str) {
        *error += 1;
        return NULL;
    }

    memcpy(str, buf->buffer, str_len);
    str[str_len] = '\0';

    // Cut used bytes from buffer
    cut_buffer(buf, -str_len);

    return str;
}

int64_t parse_integer(Buffer *buf, int bytes, int is_signed, int *error) {
    if (buf->length < bytes || bytes <= 0 || bytes > 8) {
        *error += 1;
        return 0;
    }

    uint64_t value = 0;

    // Read bytes in big-endian order (network order)
    for (int i = 0; i < bytes; i++) {
        value <<= 8;
        value |= (uint8_t)buf->buffer[i];
    }

    // Cut bytes from buffer
    cut_buffer(buf, -bytes);

    // Convert to signed if needed
    if (is_signed) {
        // Sign-extend based on byte width
        switch (bytes) {
            case 1: return (int8_t)value;
            case 2: return (int16_t)value;
            case 4: return (int32_t)value;
            case 8: return (int64_t)value;
            default:
                // Manual sign extension for other sizes
                if (value & ((uint64_t)1 << (bytes * 8 - 1))) {
                    // Negative value
                    int64_t signed_val = -((~value + 1) & ((1ULL << (bytes * 8)) - 1));
                    return signed_val;
                } else {
                    return (int64_t)value;
                }
        }
    }

    return (int64_t)value;  // unsigned interpreted as positive int64_t
}

Buffer *parse_prefixed_bytes_array(Buffer *buf, int max_length, int optional, int *error) {
    int e = 0;
    Buffer *result = init_buffer();

    if (optional) optional = parse_integer(buf, 1, 1, &e);
    if (!optional || e) {
        // (*error)++;
        return result;
    }

    int length = parse_varint(buf, &e);
    if (length > max_length || e) {
        (*error)++;
        return result;
    }

    if (buf->length < length) {  // Assuming buf has `length` and `data` fields
        (*error)++;
        return result;
    }

    append_to_buffer(result, buf->buffer, length);
    cut_buffer(buf, length);  // Remove the consumed bytes from the original buffer

    return result;
}
