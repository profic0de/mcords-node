#ifndef GAME_INIT
#define GAME_INIT

#include "networking/buffer.h"

void game_player_init(int fd, Buffer *buffer);
void game_player_tick(int fd);

#endif
