#ifndef NETWORKING_packet
#define NETWORKING_packet

#include "buffer.h"
#include <stdint.h>

int recv_packet(int fd, Buffer *out);
int read_varint(int fd, uint32_t *out);

#endif