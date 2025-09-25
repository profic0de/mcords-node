#include "networking/requests.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    http_init();

    AsyncRequest *req1 = http_post("https://httpbin.org/post", "foo=bar", "application/x-www-form-urlencoded");
    AsyncRequest *req2 = http_post("https://httpbin.org/post", "baz=qux", "application/x-www-form-urlencoded");

    while (!req1->finished || !req2->finished) {
        http_tick(); // Non-blocking tick
        printf("Tick...\n");
        usleep(100 * 1000); // 100ms delay for demo
    }

    printf("Req1: %s\n", req1->chunk.response);
    printf("Req2: %s\n", req2->chunk.response);

    http_cleanup();
    return 0;
}
