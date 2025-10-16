#ifndef GLOBALS_H
#define GLOBALS_H

#include "h/common.h"
#include "h/buffer.h"
#include <stdint.h>
#include <sys/epoll.h>
// #include "player.h"

#define DEFAULT_EPOLL_FLAGS (EPOLLIN | EPOLLET | EPOLLRDHUP)
 // | EPOLLHUP | EPOLLERR)
#define MAX_FDS 65536

extern PacketQueue* queue[MAX_FDS * 2];
// extern Player* players[MAX_FDS * 2];
// extern Player* servers[MAX_FDS * 2];
extern int epoll_fd;
// extern uint8_t ticking_fds[((MAX_FDS * 2) + 7) / 8];
// extern int ticking_fdc;
extern int exitbool;

typedef struct {
    int from;
    int len;
    char state;
    Buffer* buf;
} Packet;

extern Packet* packet_queue;
extern int packets;

#define LOG(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#endif