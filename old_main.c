// #include "utils/socket_util.h"
// #include "utils/connection.h"
// #include "utils/player.h"
// #include "utils/config.h"
#include "h/globals.h"
#include "h/packet.h"
// #include "utils/logger.h"
#include "h/requests.h"
// #include "game/player/init.h"
#include <errno.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
// #include "utils/clock.h"
// #include "utils/ram.h"
#include <malloc.h>

// #include "utils/console.h"

// #define MAX_EVENTS 1024
// #define LISTEN_PORT 25568
// #define TARGET_IP "127.0.0.1"
// #define TARGET_PORT 25565

// uint32_t events = {EPOLLIN | EPOLLET | EPOLLRDHUP};
// Player* players[MAX_FDS * 2];
// Player* servers[MAX_FDS * 2];
// bit_array(ticking_fds, MAX_FDS * 2);
// int ticking_fdc = 0;
int epoll_fd;
int exitbool = 0;

Packet* packet_queue;
int packets;

int auth_loop();

typedef struct config config;

config* load(char* file, int len, char* fallback);
char* get_config(config* config, char* key);

void handle_packet(int fd, int epoll_fd) {
    LOG("Input");
}

void close_connection(int fd, int epoll_fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    printf("Closed connection (fd=%d)\n", fd);
}

void accept_connection(int server_fd, int epoll_fd) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break; // no more clients
            perror("accept");
            break;
        }

        fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK); //Makes non-blocking

        struct epoll_event ev = { .events = DEFAULT_EPOLL_FLAGS };
        ev.data.fd = client_fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0)
            perror("epoll_ctl (client)");

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        printf("New client connected: %s:%d (fd=%d)\n", ip, ntohs(client_addr.sin_port), client_fd);
    }
}

int main() {
    config* c = load("server.properties", 0,
        "# Default Minecraft server properties\n"
        "motd=My Minecraft Server\n"
        "server-port=25568\n"
        "max-players=20\n"
        "online-mode=true\n"
        "view-distance=10\n"
        "max-fds=65536\n");

    int port;
    char* ports = get_config(c, "server-port");
    sscanf(ports?ports:"25568", "%i", &port);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL, 0) | O_NONBLOCK); //Makes non-blocking

    // printf("server_fd flags = %d\n", fcntl(server_fd, F_GETFL, 0));

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(port) };
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, SOMAXCONN);

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
    // console_init(epoll_fd);

    int max_events;
    char* events_str = get_config(c, "max-events");
    sscanf(events_str ? events_str : "1024", "%d", &max_events);

    // printf("%i",max_events);

    struct epoll_event ev;
    struct epoll_event* events = malloc(sizeof(struct epoll_event) * max_events);
    if (!events) {
        perror("malloc (events)");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    ev.events = DEFAULT_EPOLL_FLAGS;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        perror("epoll_ctl (server_fd)");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    printf("Node listening on port %d\n", port);

    // int last_tick = 0;
    // int ram_free = 0;
    // int auth = 0;
    // printf("Server ready, epoll_fd=%d, server_fd=%d, waiting for connections...\n", epoll_fd, server_fd);

    while (!exitbool) {
        // int count = 0;
        // for (int i = 0; i < MAX_FDS*2 && count < ticking_fdc; i++) {
        //     if (bit_get(ticking_fds,i)) {
        //         game_player_tick(i);
        //         // LOG("ticking fd found: %i",i);
        //         ++count;
        //     }
        // }

        // if (delay_repeat(1, &last_tick)) print_live_memory_usage();

        //---
        // if (delay_repeat(5, &ram_free)) malloc_trim(0);
        // http_perform();
        // if (delay_repeat(1, &auth)) auth_loop();
        //---

        // int count = 0;
        // for (int i = 0; i < MAX_FDS; i++)
        //     if (players[i] != NULL)
        //         count++;
        // printf("Players online: %i\n",count);

        int nfds = epoll_wait(epoll_fd, events, max_events, 0);
        if (nfds < 0) {
            perror("epoll_wait");
            break;
        }

        // if (nfds) printf("epoll_wait returned %d events\n", nfds);

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            uint32_t ev_flags = events[i].events;

            if (fd == server_fd) {
                accept_connection(server_fd, epoll_fd);
            // } else if (events[i].data.fd == STDIN_FILENO) {
            //     console_handle_input();
            } else if (ev_flags & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                close_connection(fd, epoll_fd);
            } else if (ev_flags & EPOLLIN) {
                handle_packet(fd, epoll_fd);
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
