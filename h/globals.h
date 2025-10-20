#ifndef GLOBALS_H
#define GLOBALS_H

#include "h/common.h"
#include "h/buffer.h"
#include <stdint.h>
#include <sys/epoll.h>

#define DEFAULT_EPOLL_FLAGS (EPOLLIN | EPOLLET | EPOLLRDHUP)

extern PacketQueue** queue;
extern int epoll_fd;
extern int exitbool;
extern int max_fds;

typedef struct {
    int from;
    int len;
    char state;
    Buffer* buf;
} Packet;

typedef struct Data Data;
struct Data{
    char* key;
    void* ptr;
    Data* next;
};

extern Data** fds;
extern Packet** packet_queue;
extern int packets;

#define LOG(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#endif