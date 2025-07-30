#include "connection.h"
#include "socket_util.h"
#include "packet.h"
#include "player.h"
#include "config.h"
#include "utils/logger.h"
#include "packet/building/vars.h"

#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>  // for inet_ntop
#include <netinet/in.h> // for sockaddr_in

#define BUFFER_SIZE 4096
// #define MAX_FDS 65536

// typedef struct {
//     int peer_fd;
// } Connection;

#include "globals.h"
#include <string.h>
// Player* servers[MAX_FDS];
void transfer_back(int fd) {
    const char *ip = "127.0.0.1";
    const int port = 25565;

    players[fd]->process = -1;
    bit_set(ticking_fds, fd, 0);

    Buffer *packet = init_buffer();
    // LOG("Player is %s\n", (players));

    const char *state = player_get_string(players[fd], "state", " ");
    if (!strcmp(state, "login")) {
        build_varint(packet, 2); // Packet ID for login success
        build_integer(packet, 0, 8, 0); // UUID (0)
        build_integer(packet, 0, 8, 0); // UUID (1)
        build_string(packet, "null"); // Empty username
        build_varint(packet, 0); // Empty array
        packet_send(packet, fd);
        free_buffer(packet);
    }

    int packet_id = 11; // Packet ID
    if (!strcmp(state, "play")) packet_id = 122; // Packet ID for play
    packet = init_buffer();
    build_varint(packet, packet_id); // Packet ID for transfer
    build_string(packet, ip); // Transfer to mcords.network
    build_varint(packet, port); // Default port
    packet_send(packet, fd);
    free_buffer(packet);

    // close_connection(fd, epoll_fd);
}

void close_connection(int fd, int epoll_fd) {
    // printf("Closing connection for fd %d\n", fd);
    if (players[fd]) { //|| servers[fd]
        int client_fd = players[fd]->fd;
        int server_fd = players[fd]->server_fd;

        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);

        if (server_fd > 0) close_connection(server_fd, epoll_fd);

        // if (read_config_bool("destroy-player-on-leave"))
        // int i = players[fd]->ticking_index;
        if (bit_get(ticking_fds,fd)) {
            bit_set(ticking_fds,fd,0);
            --ticking_fdc;
        }

        player_destroy(players[fd]);
        players[fd] = NULL;
    }
    if (servers[fd]) {
        // LOG("Server closed connection");
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        int pfd = servers[fd]->fd;
        servers[fd]->server_fd = 0;
        servers[fd] = NULL;
        transfer_back(pfd);
        if (bit_get(ticking_fds,fd)) {
            bit_set(ticking_fds,fd,0);
            --ticking_fdc;
        }
    }
    packet_queue_free(fd);
    printf("Connection closed\n");
}

void accept_and_connect(int server_fd, int epoll_fd, const char *target_ip, int target_port) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;  // No more servers to accept
            perror("accept");
            break;
        }

        // Convert IP to string
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, sizeof(ip_str));
        // printf("New connection from %s:%d\n", ip_str, ntohs(client_addr.sin_port));

        set_nonblocking(client_fd);
        int target_fd = -1;
        if (read_config_bool("enforce-target-connection")) {
            target_fd = connect_to_target(target_ip, target_port);
            if (target_fd < 0) {
                // uint8_t data[] = {0x10, 0x00, 0x0f, 0x7B, 0x22, 0x74, 0x65, 0x78, 0x74, 0x22, 0x3A, 0x22, 0x42, 0x79, 0x65, 0x21, 0x22, 0x7D};
                // ssize_t sent = send(client_fd, data, sizeof(data), 0);
                // if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                //     // socket buffer full, try later
                //     printf("Socket full :(\n");
                // }
                // shutdown(client_fd, SHUT_WR);
                close(client_fd);
                continue;
            }
        }
        if (read_config_bool("enforce-target-connection")) {
            set_nonblocking(target_fd);
        }


        Player* p = player_create(client_fd, "unknown");
        printf("New player registred\n");
        // servers[client_fd] = malloc(sizeof(Player));
        if (read_config_bool("enforce-target-connection")) {
            servers[target_fd] = malloc(sizeof(Player));
            // servers[client_fd]->peer_fd = target_fd;
            servers[target_fd] = p;
        }
        
        players[client_fd] = p;
        p->fd = client_fd;

        p->server_fd = -1;
        if (read_config_bool("enforce-target-connection")) {
            p->server_fd = target_fd;        
        }
        player_set_string(p,"target-ip", ip_str);
        player_set_int(p,"target-port", ntohs(client_addr.sin_port));
        // ip_str, ntohs(client_addr.sin_port);
        // player_print_data(p);
        // print_all_players(players, 100);

        struct epoll_event ev = { .events = DEFAULT_EPOLL_FLAGS };

        ev.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0)
            perror("epoll_ctl (client)");

        if (read_config_bool("enforce-target-connection")) {
            ev.data.fd = target_fd;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, target_fd, &ev) < 0)
                perror("epoll_ctl (target)");    
        }
    }
}

#include <ctype.h>  // for isprint
#include "networking/packet.h"
#include "networking/init.h"

void handle_packets(int fd, int epoll_fd) {
    // int peer_fd = servers[fd] ? servers[fd]->fd : (players[fd] ? players[fd]->server_fd : -1);
    // if (peer_fd < 0) return;

    Buffer packet;

    int result = recv_packet(fd, &packet);
    if (result == 1) {
        // printf("Received packet of %d bytes\n", packet.length);
        onPacket(fd, &packet);
    } else if (result == -2) {
        // printf("Would block — try again later\n");
        close_connection(fd, epoll_fd);
    } else {
        close_connection(fd, epoll_fd);
        perror("recv_packet_payload failed\n");
    }

    free(packet.buffer);

    // int send_len = process_packet(fd, buffer, (int)len);
    // if (send_len > 0) {
    //     send(peer_fd, buffer, send_len, 0);
    // }
}
