#ifndef VARS_BUILD
#define VARS_BUILD

#include "h/buffer.h"
#include <stdint.h>

void build_varint(Buffer *out, int value);
void build_string(Buffer *out, const char *str);
void build_integer(Buffer *out, int64_t value, int bytes, int is_signed);

#endif