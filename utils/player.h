#ifndef PLAYER_H
#define PLAYER_H

#include <stddef.h>
// Forward declare AsyncState to avoid circular include
#include "utils/common.h"
// -------------------------
// Player dynamic data system
// -------------------------

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING
} ValueType;

typedef struct {
    char key[32];
    ValueType type;
    union {
        int i;
        float f;
        char* s;
    } data;
} PlayerKV;

#define PLAYER_KV_MAX 64

typedef struct {
    PlayerKV entries[PLAYER_KV_MAX];
    int count;
} PlayerData;

// -------------------------
// Player definition
// -------------------------

typedef struct {
    int id;
    char name[32];
    PlayerData* data;

    void* label;

    int fd;
    int server_fd;

    int process;
    //Asyncio
    AsyncState *asyncio;
    int asyncio_count;
    int asyncio_capacity;
} Player;

// -------------------------
// Player API
// -------------------------

Player* player_create(int id, const char* name);
void   player_destroy(Player* p);

// Data access
void player_set_int(Player* p, const char* key, int value);
int  player_get_int(Player* p, const char* key, int fallback);

void player_set_float(Player* p, const char* key, float value);
float player_get_float(Player* p, const char* key, float fallback);

void player_set_string(Player* p, const char* key, const char* value);
const char* player_get_string(Player* p, const char* key, const char* fallback);

void player_print_data(Player* p);

#endif
