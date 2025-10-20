#ifndef H_MEM
#define H_MEM

typedef struct Memory Memory;
struct Memory {
    void* ptr;
    Memory* next;
};

void* mem_add(int fd, void* ptr);
void mem_free(int fd);

#endif