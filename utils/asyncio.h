#ifndef ASYNCIO_H
#define ASYNCIO_H

#include <stdint.h>
#include "utils/player.h"
// Forward declaration of Player

#include "utils/common.h"

AsyncState *asyncio_get_state(Player *p, const char *file, const char *func);

// Coroutine macros
#define START_PLAYER(player)                                           \
    AsyncState *__a = asyncio_get_state((player), __FILE__, __func__); \
    if (__a && __a->label) goto *((__a->label));                       \
    if (__a) __a->label = &&START;                                     \
    goto START;                                                        \
    START:

#define await                                                          \
    do {                                                               \
        if (__a) __a->label = &&_AWAIT_##__LINE__;                     \
        return;                                                        \
        _AWAIT_##__LINE__:;                                            \
    } while (0)

#define END_PLAYER                                                     \
    do { if (__a) __a->label = NULL; return; } while (0)

#define again return

#endif // ASYNCIO_H
