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

typedef struct QPacket {
    Buffer data;
    int sent; // How many bytes have been sent
    struct QPacket *next;
} QPacket;

typedef struct PacketQueue {
    QPacket *head;
    QPacket *tail;
} PacketQueue;

// typedef struct {
//     Buffer *public_key;
//     Buffer *secret_key;

//     Buffer *shared_secret;
// } KeyPair;

#endif