#include "ram.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void print_live_memory_usage() {
    FILE* file = fopen("/proc/self/status", "r");
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            printf("Live RAM usage: %s", line);
            break;
        }
    }
    fclose(file);
}
