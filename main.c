#include "h/globals.h"
#include "h/mem.h"
#include "h/packet.h"
#include "h/requests.h"
#include "h/clock.h"
#include "engine/tick.h"
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
int max_fds = 0;

Data** fds;
Packet** packet_queue;
int packets;

typedef struct config config;
config* load(char* file, int len, char* fallback);
char* get_config(config* config, char* key);
int free_config(config* config);

void close_connection(int fd, int epoll_fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);

    mem_free(fd);
    Data* block = fds[fd];
    while (block) {
        Data* next = block->next;
        free(block->key);
        free(block);
        block = next;
    }
    fds[fd] = NULL;

    packet_queue_free(fd);
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
        queue[client_fd] = NULL;
        fds[client_fd] = NULL;
    }
}

int handle_packet(int fd, int epoll_fd) {

    //packet_queue[i]->state
    //0x00 - 
    //0x02 - len unknownfinished
    //0x01 - unfinished

    if (!packet_queue) {
        packet_queue = malloc(sizeof(Packet*));
        packet_queue[0] = NULL;
        packets = 1;
    }

    Packet* packet;
    while (1) {
        packet = NULL;
        for (int i = 0; i < packets; i++) {
            if (packet_queue[i] && packet_queue[i]->state && packet_queue[i]->from == fd) {
                packet = packet_queue[i];
                break;
            }
        }

        if (!packet) {
            packet = malloc(sizeof(Packet));
            packet->buf = init_buffer();
            packet->from = fd;
            packet->state = 0x02;

            packet_queue = realloc(packet_queue, sizeof(Packet*) * (packets + 1));
            if (!packet_queue) { perror("realloc"); exit(1); }
            packet_queue[packets++] = packet;
        }

        char buf[4096];
        int amout = (packet->state==0x01 && packet->len > sizeof(buf))? sizeof(buf) : (packet->state==0x02)? 1 : packet->len - packet->buf->length;
        ssize_t n = recv(fd, buf, amout, 0);
        if (n == 0) {
            close_connection(fd, epoll_fd);
            for (int i = 0; i < packets; i++) {
                if (packet_queue && packet_queue[i]->from==fd) {
                    free_buffer(packet_queue[i]->buf);
                    free(packet_queue[i]);
                    packet_queue[i] = NULL;
                }
            }
            return 1;
        } else if (n < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv");
                close_connection(fd, epoll_fd);
                return 2;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            }
        } else {
            append_to_buffer(packet->buf, buf, n);
        }
        // Example: handle your packet data here
        // printf("[fd=%d] Received %zd bytes\n", fd, n);

        if (packet->state == 0x02) {
            uint32_t packet_len = 0;
            char err = 0;
            int i = 0;
            int shift = 0;
            for (; i < packet->buf->length; i++) {
                uint8_t b = packet->buf->buffer[i];
                packet_len |= (b & 0x7F) << shift;
                if (!(b & 0x80)) break;
                shift += 7;
                if (shift >= 32) {err = 1; break;}
            } if (!err) {
                if (packet_len > 0) {
                    packet->len = packet_len;
                    packet->state = 0x01;
                    cut_buffer(packet->buf, -(i+1));
                }
            }
        }
        if (packet->buf->length == packet->len && packet->state == 0x01) {
            packet->state = 0x00;
            printf("[fd=%i] Got a minecraft packet:\n",fd);
            print_hex(packet->buf);
        }
        // if (packet->state == 0x00) break;
    }
    return 0;
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

    char* temp_fds = get_config(c, "max-fds");
    sscanf(temp_fds ? temp_fds : "65536", "%i", &max_fds);

    queue = malloc(sizeof(PacketQueue*)*max_fds);
    fds = malloc(sizeof(Data*)*max_fds);

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
    int ticks = 0;
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

        if(delay_repeat(0.05, &ticks)) tick();
    }

    free(events);
    free_config(c);
    http_cleanup();
    close(server_fd);
    close(epoll_fd);
    return 0;
}
