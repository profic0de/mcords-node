#include <stdio.h>
#include <string.h>
#include "networking/buffer.h"

#define TOKENS_FILE "tokens.json"

char *authenticate(char *url) {
    int len, start;
    sscanf(url, "https://login.live.com/oauth20_desktop.srf?code=%n%*[^&]%n", &start, &len);
    len -= start;
    char code[len + 1];
    sscanf(url, "https://login.live.com/oauth20_desktop.srf?code=%[^&]", code);

    if (len <= 0) return "Invalid URL"; // Invalid URL

    printf("Got code from the URL: %s (%d bytes)\n", code, len);

    return NULL;
}