// packet.c
#include "h/packet.h"
#include <sys/socket.h>

PacketQueue** queue;

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

    QPacket *packet = (QPacket *)malloc(sizeof(QPacket));
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

    QPacket *old = q->head;
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
        QPacket *packet = q->head;
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

#include "h/build/packet.h"
#include "h/packet.h"

int packet_send(Buffer *buffer, int fd) {
    // Step 1: Push packet to queue
    Buffer *temp = init_buffer();
    build_varint(temp, buffer->length);
    prepend_to_buffer(buffer, temp->buffer, temp->length);
    free_buffer(temp);

    // if (player_get_int(get_player_by_fd(fd), "auth", 0) > 2) {
    //     RSA_CryptoContext *ctx = get_context_by_fd(fd);
    //     if (!ctx) return -2;
    //     RSA_encrypt(ctx, buffer, buffer);
    // }

    packet_queue_push(buffer, fd);

    // Step 2: Try to send immediately
    int result = packet_queue_send(fd);

    // Step 3: If not fully sent, enable EPOLLOUT
    if (result == 0) {
        modify_epoll(epoll_fd, fd, DEFAULT_EPOLL_FLAGS | EPOLLOUT);
    }

    return result;
}

#define ERROR(code) do { free_buffer(temp); return code; } while(0)
int read_varint(int fd, uint32_t *out) {
    Buffer *temp = init_buffer();
    *out = 0;
    int bytes_read = 0;
    uint8_t byte;

    while (bytes_read < 5) {
        ssize_t res = recv(fd, &byte, 1, 0);
        if (res == 0) ERROR(0); // disconnected
        if (res < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) ERROR(-2);
            ERROR(-1);
        }
        // if (player_get_int(get_player_by_fd(fd), "auth", 0) > 2) {
        //     append_to_buffer(temp, (char*)&byte, 1);
        //     RSA_CryptoContext *ctx = get_context_by_fd(fd);
        //     if (!ctx) ERROR(-4);
        //     AES_decrypt(ctx, temp, temp);
        //     byte = temp->buffer[0];
        //     cut_buffer(temp, 1);
        // }

        *out |= (byte & 0x7F) << (7 * bytes_read);
        bytes_read++;

        if ((byte & 0x80) == 0) break;
    }

    if (bytes_read == 5 && (byte & 0x80)) ERROR(-3);

    ERROR(1);
}
#undef ERROR

// Receives only the payload (excluding the VarInt length)
int recv_packet(int fd, Buffer *out) {
    uint32_t payload_len;
    int status = read_varint(fd, &payload_len);
    if (status <= 0) return status;

    out->length = payload_len;
    out->buffer = malloc(payload_len);
    if (!out->buffer) return -1;

    int total_read = 0;
    while (total_read < (int)payload_len) {
        ssize_t n = recv(fd, out->buffer + total_read, payload_len - total_read, 0);
        if (n == 0) {
            free(out->buffer);
            out->buffer = NULL;
            return 0; // disconnected
        } else if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -2;
            free(out->buffer);
            out->buffer = NULL;
            return -1;
        }
        total_read += n;
    }

    // if (player_get_int(get_player_by_fd(fd), "auth", 0) > 2) {
    //     RSA_CryptoContext *ctx = get_context_by_fd(fd);
    //     if (!ctx) return -3;
    //     AES_decrypt(ctx, out, out);
    // }
    return 1; // success
}
