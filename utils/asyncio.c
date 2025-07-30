#include "asyncio.h"
#include <stdlib.h>
#include <string.h>

AsyncState *asyncio_get_state(Player *p, const char *file, const char *func) {
    if (!p) return NULL;

    // Reuse finished coroutine slot or return existing
    for (int i = 0; i < p->asyncio_count; i++) {
        AsyncState *a = &p->asyncio[i];

        // Reuse finished slot
        if (a->label == NULL) {
            a->file = file;
            a->func = func;
            return a;
        }

        // Already running coroutine found
        if (strcmp(a->file, file) == 0 && strcmp(a->func, func) == 0) {
            return a;
        }
    }

    // Need to add a new coroutine state
    if (p->asyncio_count == p->asyncio_capacity) {
        p->asyncio_capacity = p->asyncio_capacity ? p->asyncio_capacity * 2 : 4;
        p->asyncio = realloc(p->asyncio, p->asyncio_capacity * sizeof(AsyncState));
        if (!p->asyncio) {
            // Allocation failed, fatal or handle gracefully
            return NULL;
        }
    }

    AsyncState *a = &p->asyncio[p->asyncio_count++];
    a->file = file;
    a->func = func;
    a->label = NULL;
    return a;
}
