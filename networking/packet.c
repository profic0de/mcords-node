#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "packet.h"
#include "buffer.h"

#include "utils/globals.h"
#include "utils/player.h"

#include "networking/encryption.h"

Player* get_player_by_fd(int fd) {
    if (players[fd]) return players[fd];
    return servers[fd];
}

const char* get_side_by_fd(int fd) {
    if (players[fd]) return "client";
    if (servers[fd]) return "server";
    return "unknown";
}

RSA_CryptoContext* get_context_by_fd(int fd) {
    if (!strcmp(get_side_by_fd(fd), "unknown")) return NULL;
    if (!strcmp(get_side_by_fd(fd), "server")) return get_player_by_fd(fd)->server_context;
    if (!strcmp(get_side_by_fd(fd), "client")) return get_player_by_fd(fd)->client_context;
    return NULL;
}

// Reads a Minecraft-style VarInt from a socket
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
        if (player_get_int(get_player_by_fd(fd), "auth", 0) > 2) {
            append_to_buffer(temp, (char*)&byte, 1);
            RSA_CryptoContext *ctx = get_context_by_fd(fd);
            if (!ctx) ERROR(-4);
            AES_decrypt(ctx, temp, temp);
            byte = temp->buffer[0];
            cut_buffer(temp, 1);
        }

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

    if (player_get_int(get_player_by_fd(fd), "auth", 0) > 2) {
        RSA_CryptoContext *ctx = get_context_by_fd(fd);
        if (!ctx) return -3;
        AES_decrypt(ctx, out, out);
    }
    return 1; // success
}
