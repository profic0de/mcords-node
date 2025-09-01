#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdint.h>

typedef struct Node {
    void *ptr;
    size_t size;
    struct Node *next;
} Node;

static Node *allocs = NULL;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Real libc functions
static void* (*real_malloc)(size_t) = NULL;
static void  (*real_free)(void*) = NULL;
static void* (*real_calloc)(size_t, size_t) = NULL;
static void* (*real_realloc)(void*, size_t) = NULL;

// ---------------- Helpers ----------------
static void init_real_funcs() {
    if (!real_malloc)   real_malloc   = dlsym(RTLD_NEXT, "malloc");
    if (!real_free)     real_free     = dlsym(RTLD_NEXT, "free");
    if (!real_calloc)   real_calloc   = dlsym(RTLD_NEXT, "calloc");
    if (!real_realloc)  real_realloc  = dlsym(RTLD_NEXT, "realloc");
}

static void add_alloc(void *ptr, size_t size) {
    Node *n = real_malloc(sizeof(Node));
    if (!n) return;
    n->ptr = ptr;
    n->size = size;
    pthread_mutex_lock(&lock);
    n->next = allocs;
    allocs = n;
    pthread_mutex_unlock(&lock);
}

static void remove_alloc(void *ptr) {
    pthread_mutex_lock(&lock);
    Node **curr = &allocs;
    while (*curr) {
        if ((*curr)->ptr == ptr) {
            Node *tmp = *curr;
            *curr = (*curr)->next;
            real_free(tmp);
            break;
        }
        curr = &(*curr)->next;
    }
    pthread_mutex_unlock(&lock);
}

static size_t total_allocated() {
    size_t total = 0;
    pthread_mutex_lock(&lock);
    Node *curr = allocs;
    while (curr) {
        total += curr->size;
        curr = curr->next;
    }
    pthread_mutex_unlock(&lock);
    return total;
}

// ---------------- Hooked functions ----------------
void* malloc(size_t size) {
    init_real_funcs();
    void *p = real_malloc(size);
    if (p) add_alloc(p, size);
    return p;
}

void free(void *ptr) {
    if (!ptr) return;
    init_real_funcs();
    remove_alloc(ptr);
    real_free(ptr);
}

void* calloc(size_t nmemb, size_t size) {
    init_real_funcs();
    void *p = real_calloc(nmemb, size);
    if (p) add_alloc(p, nmemb * size);
    return p;
}

void* realloc(void *ptr, size_t size) {
    init_real_funcs();
    if (ptr) remove_alloc(ptr);
    void *p = real_realloc(ptr, size);
    if (p) add_alloc(p, size);
    return p;
}

// ---------------- Destructor for reporting ----------------
__attribute__((destructor))
static void report() {
    size_t total = total_allocated();
    fprintf(stderr, "\n[MEMHOOK] Bytes still allocated: %zu\n", total);
    fflush(stderr);
}
