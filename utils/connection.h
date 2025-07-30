#ifndef CONNECTION_H
#define CONNECTION_H

void accept_and_connect(int server_fd, int epoll_fd, const char *target_ip, int target_port);
void handle_packets(int fd, int epoll_fd);
void close_connection(int fd, int epoll_fd);
void transfer_back(int fd);

#endif
