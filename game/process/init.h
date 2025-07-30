#ifndef PROCESS_INIT
#define PROCESS_INIT

#include "networking/buffer.h"

void game_init(int fd, Buffer *buffer);
void game_tick(int fd);

#endif
