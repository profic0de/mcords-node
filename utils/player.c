#include "player.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static PlayerKV* find_or_create(PlayerData* pd, const char* key) {
    for (int i = 0; i < pd->count; ++i) {
        if (strcmp(pd->entries[i].key, key) == 0) {
            return &pd->entries[i];
        }
    }

    if (pd->count >= PLAYER_KV_MAX) return NULL;

    PlayerKV* kv = &pd->entries[pd->count++];
    strncpy(kv->key, key, sizeof(kv->key) - 1);
    kv->key[sizeof(kv->key) - 1] = '\0';
    kv->data.s = NULL;
    return kv;
}

// -------------------------
// Player core
// -------------------------

Player* player_create(int id, const char* name) {
    Player* p = malloc(sizeof(Player));
    if (!p) return NULL;

    p->label = NULL;
    p->asyncio = NULL;
    p->asyncio_count = 0;
    p->asyncio_capacity = 0;

    p->process = 0;

    p->id = id;
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';

    p->data = calloc(1, sizeof(PlayerData));
    return p;
}

void player_destroy(Player* p) {
    if (!p) return;

    for (int i = 0; i < p->data->count; ++i) {
        if (p->data->entries[i].type == TYPE_STRING && p->data->entries[i].data.s) {
            free(p->data->entries[i].data.s);
        }
    }

    free(p->asyncio);
    p->asyncio = NULL;
    p->asyncio_count = 0;
    p->asyncio_capacity = 0;

    free(p->data);
    free(p);
}

// -------------------------
// Player data setters
// -------------------------

void player_set_int(Player* p, const char* key, int value) {
    PlayerKV* kv = find_or_create(p->data, key);
    if (!kv) return;
    if (kv->type == TYPE_STRING && kv->data.s) free(kv->data.s);

    kv->type = TYPE_INT;
    kv->data.i = value;
}

void player_set_float(Player* p, const char* key, float value) {
    PlayerKV* kv = find_or_create(p->data, key);
    if (!kv) return;
    if (kv->type == TYPE_STRING && kv->data.s) free(kv->data.s);

    kv->type = TYPE_FLOAT;
    kv->data.f = value;
}

void player_set_string(Player* p, const char* key, const char* value) {
    PlayerKV* kv = find_or_create(p->data, key);
    if (!kv) return;
    if (kv->type == TYPE_STRING && kv->data.s) free(kv->data.s);

    kv->type = TYPE_STRING;
    kv->data.s = strdup(value);
}

// -------------------------
// Player data getters
// -------------------------

int player_get_int(Player* p, const char* key, int fallback) {
    for (int i = 0; i < p->data->count; ++i) {
        PlayerKV* kv = &p->data->entries[i];
        if (strcmp(kv->key, key) == 0 && kv->type == TYPE_INT)
            return kv->data.i;
    }
    return fallback;
}

float player_get_float(Player* p, const char* key, float fallback) {
    for (int i = 0; i < p->data->count; ++i) {
        PlayerKV* kv = &p->data->entries[i];
        if (strcmp(kv->key, key) == 0 && kv->type == TYPE_FLOAT)
            return kv->data.f;
    }
    return fallback;
}

const char* player_get_string(Player* p, const char* key, const char* fallback) {
    for (int i = 0; i < p->data->count; ++i) {
        PlayerKV* kv = &p->data->entries[i];
        if (strcmp(kv->key, key) == 0 && kv->type == TYPE_STRING)
            return kv->data.s;
    }
    return fallback;
}

void player_print_data(Player* p) {
    if (!p || !p->data) {
        printf("Player has no data.\n");
        return;
    }

    printf("Player [%d] \"%s\" data:\n", p->id, p->name);
    for (int i = 0; i < p->data->count; ++i) {
        PlayerKV* kv = &p->data->entries[i];
        printf("  %s = ", kv->key);
        switch (kv->type) {
            case TYPE_INT:
                printf("%d (int)\n", kv->data.i);
                break;
            case TYPE_FLOAT:
                printf("%.2f (float)\n", kv->data.f);
                break;
            case TYPE_STRING:
                printf("\"%s\" (string)\n", kv->data.s);
                break;
            default:
                printf("Unknown type\n");
                break;
        }
    }
}
