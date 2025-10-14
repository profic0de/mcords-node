#ifndef VARS_PARSE
#define VARS_PARSE

#include "h/buffer.h"
#include <stdint.h>

int parse_varint(Buffer *buf, int *error);
char *parse_string(Buffer *buf, int max_lenght ,int *error);
int64_t parse_integer(Buffer *buf, int bytes, int is_signed, int *error);
Buffer *parse_prefixed_bytes_array(Buffer *buf, int max_length, int optional, int *error);

#endif