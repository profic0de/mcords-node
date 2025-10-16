#include "h/globals.h"
#include "h/packet.h"
#include "h/requests.h"
#include <errno.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <malloc.h>

#define DEFAULT_EPOLL_FLAGS (EPOLLIN | EPOLLET | EPOLLRDHUP)

int epoll_fd;
int exitbool = 0;

Packet** packet_queue;
int packets;

typedef struct config config;
config* load(char* file, int len, char* fallback);
char* get_config(config* config, char* key);

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
                break; // all pending connections accepted
            perror("accept");
            break;
        }

        fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK);

        struct epoll_event ev = { .events = DEFAULT_EPOLL_FLAGS };
        ev.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0)
            perror("epoll_ctl (client)");

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        printf("New client connected: %s:%d (fd=%d)\n", ip, ntohs(client_addr.sin_port), client_fd);
    }
}

void handle_packet(int fd, int epoll_fd) {

    //packet_queue[i]->state
    //0x00 - finished
    //0x01 - unfinished

    Packet* packet = NULL;
    for (int i = 0; i++; i < packets) {
        if (packet_queue[i]->state && packet_queue[i]->from == fd) {
            packet = packet_queue[i];
            break;
        }
    }

    if (!packet) {
        if (!packet_queue) packet_queue = malloc(sizeof(Packet*));
        else packet_queue = realloc(packet_queue, sizeof(Packet*)*++packets);
        packet = malloc(sizeof(Packet));
        packet->buf = init_buffer();
        packet->from = fd;
        packet->state = 0x00;

        packet_queue[packets] = packet;
    }

    while (1) {
        char buf[4096];
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n == 0) {
            close_connection(fd, epoll_fd);
            break;
        } else if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break; // fully read for now
            perror("recv");
            close_connection(fd, epoll_fd);
            break;
        }
        append_to_buffer(packet->buf, buf, n);
        // Example: handle your packet data here
        printf("[fd=%d] Received %zd bytes\n", fd, n);
    }

    // uint32_t val = 0;
    // int shift = 0, i = 0;
    // for (; i < len; i++) {
    //     uint8_t b = buf[i];
    //     val |= (b & 0x7F) << shift;
    //     if (!(b & 0x80)) break;
    //     shift += 7;
    // }
    // if (i == len) return; // not enough data
    // printf("VarInt = %u (took %d bytes)\n", val, i + 1);

}

int main() {
    packets = 0;
    config* c = load("server.properties", 0,
        "# Default Minecraft server properties\n"
        "motd=My Minecraft Server\n"
        "server-port=25568\n"
        "max-players=20\n"
        "online-mode=true\n"
        "view-distance=10\n"
        "max-fds=65536\n"
        "max-events=1024\n");

    int port;
    char* ports = get_config(c, "server-port");
    sscanf(ports ? ports : "25568", "%i", &port);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen");
        return 1;
    }

    fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL, 0) | O_NONBLOCK);
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        close(server_fd);
        return 1;
    }

    http_init(); // keep your http subsystem

    int max_events;
    char* events_str = get_config(c, "max-events");
    sscanf(events_str ? events_str : "1024", "%d", &max_events);

    struct epoll_event ev = { .events = DEFAULT_EPOLL_FLAGS };
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        perror("epoll_ctl (server_fd)");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    struct epoll_event* events = malloc(sizeof(struct epoll_event) * max_events);
    if (!events) {
        perror("malloc (events)");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    printf("Node listening on port %d (EPOLLET, non-blocking, timeout=0)\n", port);

    while (!exitbool) {
        int nfds = epoll_wait(epoll_fd, events, max_events, 0); // non-blocking!
        if (nfds < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            uint32_t ev_flags = events[i].events;

            if (fd == server_fd) {
                accept_connection(server_fd, epoll_fd);
            } else if (ev_flags & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                close_connection(fd, epoll_fd);
            } else if (ev_flags & EPOLLIN) {
                handle_packet(fd, epoll_fd);
            } else if (ev_flags & EPOLLOUT) {
                packet_queue_send(fd);
            }
        }

        // Optionally yield CPU a bit
        // usleep(1000);

        // Your periodic tasks can go here
        // http_perform();
        // auth_loop();
    }

    http_cleanup();
    close(server_fd);
    close(epoll_fd);
    return 0;
}
