#ifndef GLOBALS_H
#define GLOBALS_H

#include "utils/common.h"
#include "networking/buffer.h"
#include <stdint.h>
#include <sys/epoll.h>
#include "player.h"


#define DEFAULT_EPOLL_FLAGS (EPOLLIN | EPOLLET | EPOLLRDHUP )
 // | EPOLLHUP | EPOLLERR)
#define MAX_FDS 65536

extern PacketQueue* queue[MAX_FDS * 2];
extern Player* players[MAX_FDS * 2];
extern Player* servers[MAX_FDS * 2];
extern int epoll_fd;

extern uint8_t ticking_fds[((MAX_FDS * 2) + 7) / 8];
extern int ticking_fdc;

#endif