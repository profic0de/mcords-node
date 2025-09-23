#include "utils/socket_util.h"
#include "utils/connection.h"
#include "utils/player.h"
#include "utils/config.h"
#include "utils/globals.h"
#include "utils/packet.h"
#include "utils/logger.h"
#include "networking/requests.h"
#include "game/player/init.h"
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "utils/clock.h"
#include "utils/ram.h"
#include <malloc.h>

#include "utils/console.h"

#define MAX_EVENTS 1024
#define LISTEN_PORT 25568
#define TARGET_IP "127.0.0.1"
#define TARGET_PORT 25565

// uint32_t events = {EPOLLIN | EPOLLET | EPOLLRDHUP};
Player* players[MAX_FDS * 2];
Player* servers[MAX_FDS * 2];
bit_array(ticking_fds, MAX_FDS * 2);
int ticking_fdc = 0;
int epoll_fd;
int exitbool = 0;

int auth_loop();

int main() {
    int server_fd = create_server_socket(LISTEN_PORT);
    if (server_fd < 0) {
        perror("Failed to create server socket");
        return 1;
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("Failed to create epoll instance");
        close(server_fd);
        return 1;
    }

    http_init();
    console_init(epoll_fd);

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = DEFAULT_EPOLL_FLAGS;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        perror("epoll_ctl (server_fd)");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    printf("Node listening on port %d\n", LISTEN_PORT);

    // int last_tick = 0;
    int ram_free = 0;
    int auth = 0;

    while (!exitbool) {
        http_perform();

        int count = 0;
        for (int i = 0; i < MAX_FDS*2 && count < ticking_fdc; i++) {
            if (bit_get(ticking_fds,i)) {
                game_player_tick(i);
                // LOG("ticking fd found: %i",i);
                ++count;
            }
        }

        // if (delay_repeat(1, &last_tick)) print_live_memory_usage();
        if (delay_repeat(5, &ram_free)) malloc_trim(0);
        if (delay_repeat(5, &auth)) auth_loop();

        // int count = 0;
        // for (int i = 0; i < MAX_FDS; i++)
        //     if (players[i] != NULL)
        //         count++;
        // printf("Players online: %i\n",count);

        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 0);
        if (nfds < 0) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            uint32_t ev_flags = events[i].events;

            if (fd == server_fd) {
                accept_and_connect(server_fd, epoll_fd, TARGET_IP, TARGET_PORT);
            } else if (events[i].data.fd == STDIN_FILENO) {
                console_handle_input();
            } else if (ev_flags & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                close_connection(fd, epoll_fd);
            } else if (ev_flags & EPOLLIN) {
                handle_packets(fd, epoll_fd);
            } else if (ev_flags & EPOLLOUT) {
                packet_queue_send(fd);
            }
        }

    }

    http_cleanup();
    close(server_fd);
    close(epoll_fd);
    return 0;
}
