#include <stdlib.h>
#include "h/fds.h"
#include "h/mem.h"

void* mem_add(int fd, void* ptr) {
    Memory* mem = fds_get(fd, "__memory");

    if (!mem) {
        mem = malloc(sizeof(Memory));
        if (!mem) return NULL;

        mem->ptr = ptr;
        mem->next = NULL;
        fds_set(fd, "__memory", mem);
        return ptr;
    }
    Memory* current = mem;
    while (current) {
        if (current->ptr == ptr)
            return ptr;
        if (!current->next)
            break;

        current = current->next;
    }

    // Add new block to the end
    Memory* new_mem = malloc(sizeof(Memory));
    if (!new_mem) return NULL;
    new_mem->ptr = ptr;
    new_mem->next = NULL;

    current->next = new_mem;
    return ptr;
}

void mem_free(int fd) {
    Memory* mem = fds_get(fd, "__memory");
    while (mem) {
        free(mem->ptr);
        Memory* next = mem->next;
        free(mem);
        mem = next;
    }
    fds_del(fd, "__memory");
}