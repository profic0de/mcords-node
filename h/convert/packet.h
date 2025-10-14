#ifndef VARS_CONVERT
#define VARS_CONVERT

#include "h/buffer.h"
#include <string.h>
#include <string.h>

char *buffer_to_string(Buffer *buf, int *out_len);
Buffer *string_to_buffer(const char *data, int *consumed, int *error);

#endif