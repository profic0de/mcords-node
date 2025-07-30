#ifndef NETWORKING_init
#define NETWORKING_init

#include "buffer.h"

void onPacket(int fd, Buffer *buffer);

#endif