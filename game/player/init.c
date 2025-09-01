#include "packet/building/vars.h"
#include "utils/globals.h"
#include "utils/player.h"
#include "utils/packet.h"
#include "string.h"
#include "stdio.h"
#include "init.h"

#include "intentions/status/status.h"
#include "intentions/play/init.h"

#include "utils/logger.h"

void game_player_init(int fd, Buffer *buffer) {
    Player *p = players[fd];
    if (!p) return;
    switch (player_get_int(p, "intent", -1))
    {
    case 1:
        handle_status(fd, buffer);
        break;

    case 2:
        // if (!strcmp(player_get_string(p, "state", ""), "")) player_set_string(p, "state", "login");
        // handle_play(fd, buffer);
        Buffer *packet = init_buffer();
        build_varint(packet, 0);
        build_string(packet, "[{\"text\":\"You're not supposed to join this server directly.\\n\",\"color\":\"gold\"},{\"text\":\"Please connect using: \",\"color\":\"red\"},{\"text\":\"mcords.net\",\"color\":\"aqua\",\"bold\":true}]");
        packet_send(packet, fd);

        free_buffer(packet);
        break;

    case 3:
        if (!strcmp(player_get_string(p, "state", ""), "")) player_set_string(p, "state", "login");
        handle_play(fd, buffer);
        break;

    default:
        break;
    }
}

#include "game/process/init.h"
void game_player_tick(int fd) {
    if ((players[fd] && players[fd]->process) || servers[fd]) {game_tick(fd); return;}
    Player *p = players[fd];
    if (!p) return;
    switch (player_get_int(p, "intent", -1))
    {
    // case 2:
    //     handle_play_tick(fd);
    //     break;

    case 3:
        handle_play_tick(fd);
        break;
    default:
        break;
    }
}