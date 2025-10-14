// packet.h
#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "h/globals.h"
#include "h/buffer.h"
#include "h/common.h"

void packet_queue_init(int fd);
void packet_queue_push(Buffer *buffer, int fd);
void packet_queue_free(int fd);
void packet_queue_pop(int fd);
int packet_queue_send(int fd);
int is_queue_empty(int fd);

int modify_epoll(int epoll_fd, int fd, uint32_t events);
int packet_send(Buffer *buffer, int fd);

#endif // PACKET_H