#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include "h/fds.h"
#include "h/mem.h"

void* fds_set(int fd, char* key, void* ptr) {
    Data* block = fds[fd];
    Data* last = NULL;

    while (block) {
        if (strcmp(block->key, key) == 0) {
            block->ptr = ptr;
            return block;
        }
        last = block;
        block = block->next;
    }

    Data* new_block = malloc(sizeof(Data));
    if (!new_block) return NULL;
    new_block->key = strdup(key);
    new_block->ptr = ptr;
    new_block->next = NULL;

    if (!last) fds[fd] = new_block;
    else last->next = new_block;

    return new_block;
}

void* fds_get(int fd, char* key) {
    Data* block = fds[fd];

    while (block) {
        if (!strcmp(block->key, key))
            return block->ptr;
        block = block->next;
    }
    return NULL;
}

void* fds_del(int fd, char* key) {
    Data* block = fds[fd];
    Data* prev = NULL;

    while (block) {
        if (!strcmp(block->key, key)) {
            if (prev == NULL) fds[fd] = block->next;
            else prev->next = block->next;

            void* ptr = block->ptr;
            free(block->key);
            free(block);
            return ptr;
        }
        prev = block;
        block = block->next;
    }

    return NULL;
}

int fds_incr(int fd, char* key) {
    int* counter = fds_get(fd, key); // get stored pointer
    if (!counter) {
        // first time, allocate and initialize
        counter = malloc(sizeof(int));
        if (!counter) return -1;    // malloc failed
        *counter = 0;
        fds_set(fd, (char*)key, counter);
        mem_add(fd, counter);
    }

    (*counter)++;
    return *counter;
}