#include "utils/asyncio.h"
#include "utils/globals.h"
#include "utils/logger.h"
#include "utils/packet.h"
#include "utils/player.h"
#include "init.h"

#include <string.h>
#include <stdio.h>

#include "packet/building/vars.h"
#include "packet/parsing/vars.h"

void serverbound_login(int fd, int packet_id, Buffer *buffer, Player *p);
void clientbound_login(int fd, int packet_id, Buffer *buffer, Player *p);

void game_init(int fd, Buffer *buffer) {
    char *direction = (servers[fd]) ? "clientbound" : "serverbound";
    int error = 0;
    int packet_id = parse_varint(buffer, &error);
    printf("Packet from fd %d (%s), packet_id: %d\n", fd, direction, packet_id);
    const char *state = player_get_string(!strcmp(direction, "serverbound") ? players[fd] : servers[fd], "state", "");
    if (!strcmp(state, "login")) {
        if (!strcmp(direction, "serverbound")) serverbound_login(players[fd]->server_fd, packet_id, buffer, players[fd]);
        if (!strcmp(direction, "clientbound")) clientbound_login(servers[fd]->fd, packet_id, buffer, servers[fd]);
    }
}
void game_tick(int fd) {
    // LOG("Tick for fd %d (%s)", fd, (servers[fd]) ? "server" : "client");
}

void serverbound_login(int fd, int packet_id, Buffer *buffer, Player *p) {
    START_PLAYER(p);

    Buffer *packet = init_buffer();

    build_varint(packet, packet_id);
    append_to_buffer(packet, buffer->buffer, buffer->length);

    packet_send(packet, fd);
    free_buffer(packet);
    // await;

    // again;
    END_PLAYER;
}

void clientbound_login(int fd, int packet_id, Buffer *buffer, Player *p) {
    START_PLAYER(p);
    if (packet_id == 0x01) {
        player_set_string(p, "auth", "10");
    }

    Buffer *packet = init_buffer();

    build_varint(packet, packet_id);
    append_to_buffer(packet, buffer->buffer, buffer->length);

    packet_send(packet, fd);

    free_buffer(packet);
    // await;
    // again;
    END_PLAYER;
}
