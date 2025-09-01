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

#include "packet/converting/vars.h"
#include "networking/encryption.h"

void clientbound_login(int fd, int packet_id, Buffer *buffer, Player *p) {
    START_PLAYER(p);
    Buffer *packet = init_buffer();
    build_varint(packet, packet_id);
    int len = 0;
    int error = 0;

    char *h = hex(buffer);
    printf("%s\n", h);
    free(h);

    if (packet_id == 0x01) {
        char *str = parse_string(buffer, 20, &error);
        if (error) {free(str); goto AGAIN;}
        build_string(packet, str);
        LOG("Server id: %s", str);
        free(str);
        
        Buffer *public_key = parse_prefixed_bytes_array(buffer, -1, 0, &error);
        if (error) {free_buffer(public_key); goto AGAIN;}

        str = buffer_to_string(public_key, &len);
        printf("Public key (%d bytes)\n", public_key->length);
        append_to_buffer(packet, str, len);
        free(str);
        RSA_generate(p->context, public_key);
        free_buffer(public_key);

        Buffer *vtoken = parse_prefixed_bytes_array(buffer, -1, 0, &error);
        if (error) {free_buffer(vtoken); goto AGAIN;}

        char *vtoken_str = buffer_to_string(vtoken, &len);
        append_to_buffer(packet, vtoken_str, len);
        player_set_string(p, "vtoken", vtoken_str);
        free(vtoken_str);
        free_buffer(vtoken);

        int auth = parse_varint(buffer, &error);
        LOG("Auth: %d", auth);
        if (error) goto AGAIN;
        player_set_int(p, "auth", 1 + auth);
        if (auth != 0 && auth != 1) goto AGAIN;
        build_varint(packet, 0);
        // free(server_id);
        // free_buffer(array);
    } else if (packet_id == 0x02) {
        player_set_string(p, "state", "config");
    } else if (packet_id == 0x03) {
        int compression = parse_varint(buffer, &error);
        if (error) goto AGAIN;
        player_set_int(p, "compression", compression);
    }

    packet_send(packet, fd);
    LOG("Sent packet %d to fd %d", packet_id, fd);

AGAIN:
    free_buffer(packet);
    END_PLAYER;
}
