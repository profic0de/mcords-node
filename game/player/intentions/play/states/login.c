#include "packet/building/vars.h"
#include "packet/parsing/vars.h"
#include "utils/connection.h"
#include "utils/asyncio.h"
#include "utils/globals.h"
#include "utils/player.h"
#include "utils/logger.h"
#include "utils/packet.h"
#include <arpa/inet.h>
#include "utils/str.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "login.h"

void uuid_to_string(const uint8_t uuid[16], char uuid_str[37]);
void generate_offline_uuid(const char *username, uint8_t out_uuid[16]);
int connect_server_fd(const char *ip, int port, int fd, int epoll_fd);
int get_ip_address(const char *domain, char *out);

void slogin(int fd, Buffer *buffer) {
    Player *p = players[fd];
    int error = 0;
    START_PLAYER(p);
    if (buffer->length==-1) again; //Waits untill a packet
    int packet_id = parse_varint(buffer, &error);
    if (error||packet_id) again;
    const char *username = parse_string(buffer, 16, &error);
    player_set_string(p, "username", username);
    if (error||strlen(username)>16) again;
    if (buffer->length<16) again;
    uint8_t uuid[16];
    memcpy(uuid, buffer->buffer, 16);
    cut_buffer(buffer, -16);
    LOG("Username: %s",username);
    uint8_t offline_uuid[16];generate_offline_uuid(username, offline_uuid);
    LOG("Logged in: %s", memcmp(uuid, offline_uuid, 16) ? "true" : "false");
    // return 0;

    Buffer *packet = init_buffer();
    build_varint(packet, 5);
    build_string(packet, "mcords:cookie");

    packet_send(packet, fd);
    free_buffer(packet);

    await; //Wait for a new packet
    if (buffer->length==-1) again; //Waits untill a packet
    packet_id = parse_varint(buffer, &error);
    if (packet_id!=4||error) again;
    const char *cookie_id = parse_string(buffer, 32767, &error);
    if (strcmp(cookie_id, "mcords:cookie")||error) again;
    Buffer *buf = parse_prefixed_bytes_array(buffer, 5120, 1, &error);
    if (error) again;
    const char zero = '\0';
    append_to_buffer(buf, &zero, 1);
    // if (buf->length>1) LOG("Cookie: %s", buf->buffer);
    int out_count = 0;
    char **cookie_parts = split(buf->buffer, ':', &out_count);
    LOG("Connecting to: %s:%s", cookie_parts[0], (out_count > 1 && cookie_parts[1]) ? cookie_parts[1] : "<invalid>");

    char addr[INET_ADDRSTRLEN];
    int skip = 0;
    if (!!get_ip_address(cookie_parts[0], addr)) {
        LOG("Failed to resolve domain: %s", cookie_parts[0]);
        skip += 1;
    }
    if (!!skip || 0 > connect_server_fd(addr, (out_count > 1 && cookie_parts[1]) ? atoi(cookie_parts[1]) : 25565, fd, epoll_fd)) {
        LOG("Failed to connect to server: %s:%s", cookie_parts[0], (out_count > 1 && cookie_parts[1]) ? cookie_parts[1] : "<invalid>");
        LOG("Transferring back to mcords.network.");
        skip += 1;
    }
    if (!!skip) transfer_back(fd);
    if (!skip) players[fd]->process = 1;
    free_split(cookie_parts, out_count);
    free_buffer(buf);
    END_PLAYER;
}

void uuid_to_string(const uint8_t uuid[16], char uuid_str[37]) {
    sprintf(uuid_str,
        "%02x%02x%02x%02x-"
        "%02x%02x-"
        "%02x%02x-"
        "%02x%02x-"
        "%02x%02x%02x%02x%02x%02x",
        uuid[0], uuid[1], uuid[2], uuid[3],
        uuid[4], uuid[5],
        uuid[6], uuid[7],
        uuid[8], uuid[9],
        uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]
    );
}

#include <openssl/md5.h>
void generate_offline_uuid(const char *username, uint8_t out_uuid[16]) {
    char input[256];
    snprintf(input, sizeof(input), "OfflinePlayer:%s", username);

    unsigned char digest[16];
    MD5((const unsigned char *)input, strlen(input), digest);

    // Set RFC 4122 version 3 (name-based MD5)
    digest[6] = (digest[6] & 0x0F) | 0x30;  // version 3
    digest[8] = (digest[8] & 0x3F) | 0x80;  // variant RFC 4122

    memcpy(out_uuid, digest, 16);
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "utils/packet.h"
int connect_server_fd(const char *ip, int port, int fd, int epoll_fd) {
    LOG("Connecting to server %s:%d", ip, port);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        // LOG("Failed to create socket");
        return -1;
    }
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        close(sockfd);
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return -1;
    }
    players[fd]->server_fd = sockfd;
    servers[sockfd] = players[fd];

    // Register the new server socket with epoll
    struct epoll_event ev;
    ev.events = DEFAULT_EPOLL_FLAGS;
    ev.data.fd = sockfd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        // LOG("epoll_ctl failed for server_fd %d", sockfd);
        close(sockfd);
        return -1;
    }
    Buffer *packet = init_buffer();
    build_varint(packet, 0); // Packet ID for handshake
    build_varint(packet, player_get_int(players[fd], "version", 0)); // Protocol version
    build_string(packet, ip); // Server IP
    build_integer(packet, port, 2, 0); // Server port
    build_varint(packet, 2); // Intent (2 for login)
    packet_send(packet, sockfd);
    free_buffer(packet);

    packet = init_buffer();
    build_varint(packet, 0); // Packet ID for login
    build_string(packet, player_get_string(players[fd], "username", "")); // Username
    build_integer(packet, 0, 8, 0); // UUID (0)
    build_integer(packet, 0, 8, 0); // UUID (1)

    packet_send(packet, sockfd);
    free_buffer(packet);
    // LOG("Connected to server %s:%d (server_fd=%d)", ip, port, sockfd);
    return sockfd;
}

#include <stdio.h>
#include <netdb.h>

int get_ip_address(const char *domain, char *out) {
    struct hostent *host_info = gethostbyname(domain);
    if (!host_info || !host_info->h_addr_list[0]) {
        return -1; // DNS resolution failed
    }

    struct in_addr *address = (struct in_addr *) host_info->h_addr_list[0];
    const char *ip_str = inet_ntoa(*address);

    if (ip_str)
        strncpy(out, ip_str, INET_ADDRSTRLEN - 1);
    out[INET_ADDRSTRLEN - 1] = '\0'; // Ensure null-termination

    return 0;
}
