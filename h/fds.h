#ifndef H_FDS
#define H_FDS

#include "h/globals.h"

void* fds_set(int fd, char* key, void* ptr);
void* fds_get(int fd, char* key);
void* fds_del(int fd, char* key);
int fds_incr(int fd, char* key);

#endif