#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "networking/requests.h"
#include "utils/logger.h"
#include "networking/buffer.h"
#include "networking/requests.h"

#define TOKENS_FILE "tokens.json"

#define MAX_REQUESTS 16

typedef struct{
    HttpResponse *request;
    int step;
} AuthBlock;

AuthBlock *auth_requests[MAX_REQUESTS];
// bit_array(requests_bools, MAX_REQUESTS);

char *authenticate(char *url, AuthBlock *block);

int auth_loop() {
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (auth_requests[i] && auth_requests[i]->request->done) {
            authenticate(NULL, auth_requests[i]);
            free_buffer(auth_requests[i]->request->buf);
            free(auth_requests[i]->request);
            free(auth_requests[i]);
            auth_requests[i] = NULL;
        }
    }
    return 0;
}

// static CURLM *multi_handle = NULL;

static inline int auth_post0(const char *code) {
    char post_fields[512];
    const char *redirect_uri = "https%3A%2F%2Flogin.live.com%2Foauth20_desktop.srf"; 
    // const char *code = "M.C538_BAY.2.U.62a35b0c-b452-2eb3-7247-162c19c8472a"; // hardcoded

    printf("Auth post 0 with code: %s\n", code);

    snprintf(post_fields, sizeof(post_fields), "client_id=00000000402b5328&redirect_uri=%s&grant_type=authorization_code&code=%s",redirect_uri, code);

    HttpResponse *resp = http_post("https://login.live.com/oauth20_token.srf", post_fields, "application/x-www-form-urlencoded");

    AuthBlock *block = malloc(sizeof(AuthBlock));
    block->request = resp;
    block->step = 1;
    
    int slot = -1;
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (!auth_requests[i]) {
            auth_requests[i] = block;
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        fprintf(stderr, "No free request slots!\n");
        free(resp);
        free(block);
        return -1;
    }

    auth_requests[slot] = block;

    return 0;
}

char *authenticate(char *url, AuthBlock *block) {
    int step = (block == NULL ? 0 : block->step);
    switch (step)
    {
    case 0: {
        // LOG("Called");
        const char *code_start = strstr(url, "code=");
        if (!code_start) return "Invalid URL";
        code_start += 5; // skip "code="

        const char *amp = strchr(code_start, '&');
        size_t code_len = amp ? (size_t)(amp - code_start) : strlen(code_start);

        char code[256]; // make sure big enough
        if (code_len >= sizeof(code)) code_len = sizeof(code)-1;
        memcpy(code, code_start, code_len);
        code[code_len] = '\0'; // null-terminate

        if (code_len <= 0) return "Invalid URL"; // Invalid URL

        printf("Got code from the URL: %s (%d bytes)\n", code, code_len);
        auth_post0(code);
        break;}

    case 1: {
        printf("Step 1 response:\n");
        print_readable(block->request->buf);

        break;}
    }
    return NULL;
}