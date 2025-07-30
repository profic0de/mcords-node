#ifndef PLAY_H
#define PLAY_H

#include "networking/buffer.h"

void handle_play(int fd, Buffer *buffer);
void handle_play_tick(int fd);

#endif // MYFUNCTIONS_H
