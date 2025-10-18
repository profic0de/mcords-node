#include "h/clock.h"

int delay_repeat(double seconds_interval, int *last_tick) {
    clock_t now = clock();
    int now_ms = (int)((double)now * 1000 / CLOCKS_PER_SEC);

    if (now_ms - *last_tick >= (int)(seconds_interval * 1000)) {
        *last_tick = now_ms;
        return 1;
    }
    return 0;
}