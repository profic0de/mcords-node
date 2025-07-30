#ifndef SOCKET_UTIL_H
#define SOCKET_UTIL_H

#include "player.h"
#include "globals.h"

// void print_all_players(int max_fds);

int set_nonblocking(int fd);
int create_server_socket(int port);
int connect_to_target(const char *ip, int port);
void print_all_players(int max_fds);

#endif
