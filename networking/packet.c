#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "packet.h"
#include "buffer.h"

// Reads a Minecraft-style VarInt from a socket
int read_varint(int fd, uint32_t *out) {
    *out = 0;
    int bytes_read = 0;
    uint8_t byte;

    while (bytes_read < 5) {
        ssize_t res = recv(fd, &byte, 1, 0);
        if (res == 0) return 0; // disconnected
        if (res < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -2;
            return -1;
        }

        *out |= (byte & 0x7F) << (7 * bytes_read);
        bytes_read++;

        if ((byte & 0x80) == 0) break;
    }

    if (bytes_read == 5 && (byte & 0x80)) return -3;

    return 1;
}

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

    return 1; // success
}
