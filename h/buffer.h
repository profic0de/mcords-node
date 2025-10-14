#ifndef NETWORKING_buffer
#define NETWORKING_buffer

#include "h/common.h"
#include <stdint.h>

#define bit_array(name, num_bits) uint8_t name[((num_bits) + 7) / 8] = {0}

#define bit_set(arr, i, v) \
    do { \
        if (v) \
            (arr)[(i) / 8] |=  (1 << ((i) % 8)); \
        else \
            (arr)[(i) / 8] &= ~(1 << ((i) % 8)); \
    } while (0)

#define bit_get(arr, i) (((arr)[(i) / 8] >> ((i) % 8)) & 1)

void cut_buffer(Buffer *buf, int count);
void append_to_buffer(Buffer *buf, const char *data, int data_len);
void prepend_to_buffer(Buffer *buf, const char *data, int data_len);

Buffer *init_buffer();
void clear_buffer(Buffer *buf);
void free_buffer(Buffer *buf);

char *hex(const Buffer *buf);
void print_hex(const Buffer *buf);
void print_readable(const Buffer *buf);

#endif