#ifndef COMMON
#define COMMON

// typedef struct {
//     const char *file;
//     const char *func;
//     void *label;  // resume point, NULL if coroutine ended
// } AsyncState;

typedef struct {
    int length;
    char *buffer;
} Buffer;

typedef struct Packet {
    Buffer data;
    int sent; // How many bytes have been sent
    struct Packet *next;
} Packet;

typedef struct PacketQueue {
    Packet *head;
    Packet *tail;
} PacketQueue;

// typedef struct {
//     Buffer *public_key;
//     Buffer *secret_key;

//     Buffer *shared_secret;
// } KeyPair;

#endif