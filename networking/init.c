#include "packet/parsing/vars.h"
#include "utils/connection.h"
#include "utils/globals.h"
#include "utils/player.h"
#include "buffer.h"
#include <stdlib.h>
#include <stdio.h>
#include "init.h"

#include "game/player/init.h"
#include "utils/asyncio.h"

#include "game/process/init.h"

void onPacket(int fd, Buffer *buffer) {
    if (fd < 0) return;
    Player* p = players[fd];
    if (players[fd] && players[fd]->process == -1) return;
    if ((players[fd] && players[fd]->process == 1) || servers[fd]) {game_init(fd, buffer); return;}
    if (!p) return;

    // printf("fd=%d, p=%p, p->label=%p\n", fd, (void*)p, p ? p->label : NULL);
    if (player_get_int(p, "loop1", 0)) goto LOOP;

    int error = 0;
    // START_PLAYER(p);
    int packet_id = parse_varint(buffer, &error);

    if (packet_id != 0) {
        close_connection(fd, epoll_fd);
        return;
    }
    int protocol = parse_varint(buffer, &error);
    parse_string(buffer, 255, &error);
    cut_buffer(buffer,-2);
    int intent = parse_varint(buffer, &error);

    if (error > 0) {
        close_connection(fd, epoll_fd);
        return;
    }

    printf("Hanshake recieved: [version=%i],[intent=%i]\n",protocol,intent);

    player_set_int(p, "version", protocol);
    player_set_int(p, "intent", intent);

    // await;
    // printf("packet id is: %i\n", parse_varint(buffer, &error));
    player_set_int(p, "loop1", 1);

    LOOP:
    game_player_init(fd, buffer);
    return;
    printf("END\n");
    // printf("%02X\n", (unsigned char)buffer->buffer[0]);

    // if (player_get_string(&p, "state", "") == "") player_set_string(&p, "state", "");

    // if (player_get_string(&p, "state", ""))
    //     login(fd, buffer);
    // for (int i = 0; i < buffer->length; i++)
    //     printf("%02X ", (unsigned char)buffer->buffer[i]);
    // printf("\n");
    // await(p->label);
    // END_PLAYER;
}