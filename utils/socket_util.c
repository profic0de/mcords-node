#include "socket_util.h"
#include "player.h"
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>

int set_nonblocking(int fd) {
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

int create_server_socket(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    set_nonblocking(sock);

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(port) };
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(sock, SOMAXCONN);
    return sock;
}

int connect_to_target(const char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    set_nonblocking(sock);
    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(port) };
    inet_pton(AF_INET, ip, &addr.sin_addr);
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    return sock;
}

void print_all_players(int max_fds) {
    for (int i = 0; i < max_fds; i++) {
        Player* p = players[i];
        if (p != NULL) {
            printf("Player at fd %d: fd=%d, name=%s\n", i, p->fd, p->name);
        }
    }
}
