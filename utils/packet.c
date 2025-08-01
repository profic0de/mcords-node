// packet.c
#include "packet.h"
#include <sys/socket.h>

PacketQueue* queue[MAX_FDS * 2];

void packet_queue_init(int fd) {
    PacketQueue *q = queue[fd];
    if (!queue[fd]) {
        queue[fd] = malloc(sizeof(PacketQueue));
    }
    q->head = NULL;
    q->tail = NULL;
}

void packet_queue_push(Buffer *buffer, int fd) {
    if (!buffer || !buffer->buffer || buffer->length <= 0) return;
    PacketQueue *q = queue[fd];
    if (!q) {
        q = calloc(1, sizeof(PacketQueue)); // zero-initialize
        if (!q) return;
        queue[fd] = q;
    }

    Packet *packet = (Packet *)malloc(sizeof(Packet));
    if (!packet) return;

    packet->data.buffer = (char *)malloc(buffer->length);
    if (!packet->data.buffer) {
        free(packet);
        return;
    }

    memcpy(packet->data.buffer, buffer->buffer, buffer->length);
    packet->data.length = buffer->length;
    packet->sent = 0;
    packet->next = NULL;

    if (q->tail) {
        q->tail->next = packet;
    } else {
        q->head = packet;
    }
    q->tail = packet;
}

void packet_queue_pop(int fd) {
    PacketQueue *q = queue[fd];
    if (!q->head) return;

    Packet *old = q->head;
    q->head = old->next;
    if (!q->head) {
        q->tail = NULL;
    }
    free(old->data.buffer);
    free(old);
}

int packet_queue_send(int fd) {
    PacketQueue *q = queue[fd];
    while (q->head) {
        Packet *packet = q->head;
        int to_send = packet->data.length - packet->sent;
        ssize_t sent = send(fd, packet->data.buffer + packet->sent, to_send, 0);

        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0; // try again later
            }
            return -1; // error
        }

        packet->sent += sent;
        if (packet->sent >= packet->data.length) {
            packet_queue_pop(fd);
        } else {
            return 0; // not fully sent yet
        }
    }
    modify_epoll(epoll_fd, fd, DEFAULT_EPOLL_FLAGS);
    return 1; // all sent
}

void packet_queue_free(int fd) {
    PacketQueue *q = queue[fd];
    if (!q) return;
    while (q->head) {
        packet_queue_pop(fd);
    }
    free(q);
    queue[fd] = NULL;
}

int is_queue_empty(int fd) {
    PacketQueue *q = queue[fd];
    return q->head == NULL;
}

#include <sys/epoll.h>
#include <stdio.h>

int modify_epoll(int epoll_fd, int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        perror("epoll_ctl MOD failed");
        return -1;
    }

    return 0;
}

#include "packet/building/vars.h"

int packet_send(Buffer *buffer, int fd) {
    // Step 1: Push packet to queue
    Buffer *temp = init_buffer();
    build_varint(temp, buffer->length);
    prepend_to_buffer(buffer, temp->buffer, temp->length);
    free(temp->buffer);

    packet_queue_push(buffer, fd);

    // Step 2: Try to send immediately
    int result = packet_queue_send(fd);

    // Step 3: If not fully sent, enable EPOLLOUT
    if (result == 0) {
        modify_epoll(epoll_fd, fd, DEFAULT_EPOLL_FLAGS | EPOLLOUT);
    }

    return result;
}
