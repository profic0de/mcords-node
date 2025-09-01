#include "packet/parsing/vars.h"
#include "packet/building/vars.h"
#include "utils/asyncio.h"
#include "utils/logger.h"
#include "utils/globals.h"
#include "utils/player.h"
#include "utils/packet.h"
#include <string.h>
#include "stdio.h"
#include "init.h"
#include "utils/logger.h"

#include "game/player/intentions/play/states/login.h"
void handle_play(int fd, Buffer *buffer) {

    if (!bit_get(ticking_fds,fd)) {++ticking_fdc;bit_set(ticking_fds,fd,1);}
    const char *state = player_get_string(players[fd],"state","");
    if (!strcmp(state,"login")) {
        login(fd, buffer);
    }
    // LOG("Test Play");
}

void handle_play_tick(int fd) {
    if (!bit_get(ticking_fds,fd)) return;
    Buffer dummy = {.buffer = malloc(0),.length = -1};
    handle_play(fd, &dummy);
    free(dummy.buffer);
}