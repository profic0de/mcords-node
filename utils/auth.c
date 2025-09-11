#include <stdio.h>
#include <string.h>
#include "networking/buffer.h"

#define TOKENS_FILE "tokens.json"
// #define URL "https://login.live.com/oauth20_desktop.srf?code=M.C538_BL2.2.U.086de2b4-c8ba-2707-0e1f-84ac88eb037c&lc=1033"

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