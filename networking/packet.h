#ifndef NETWORKING_packet
#define NETWORKING_packet

#include "utils/player.h"
#include "buffer.h"
#include <stdint.h>

Player* get_player_by_fd(int fd);
const char* get_side_by_fd(int fd);
RSA_CryptoContext* get_context_by_fd(int fd);

int recv_packet(int fd, Buffer *out);
int read_varint(int fd, uint32_t *out);

#endif