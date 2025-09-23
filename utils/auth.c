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
    HttpResponse *response;
    int step;
} AuthBlock;

AuthBlock *auth_requests[MAX_REQUESTS];
// bit_array(requests_bools, MAX_REQUESTS);

char *authenticate(char *url, AuthBlock *block);

int auth_loop() {
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (auth_requests[i] && auth_requests[i]->response->done) {
            authenticate(NULL, auth_requests[i]);
            free_buffer(auth_requests[i]->response->buf);
        }
    }
    return 0;
}

// static CURLM *multi_handle = NULL;
static inline size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    HttpResponse *resp = (HttpResponse *)userp;

    append_to_buffer(resp->buf, contents, total);
    return total;
}
static inline CURL *http_post_0(const char *url, const char *post_fields, HttpResponse *resp) {
    resp->buf = init_buffer();
    resp->done = 0;
    resp->status_code = 0;

    CURL *easy = curl_easy_init();
    curl_easy_setopt(easy, CURLOPT_URL, url);
    curl_easy_setopt(easy, CURLOPT_POSTFIELDS, post_fields);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, resp);
    curl_easy_setopt(easy, CURLOPT_PRIVATE, resp);
    curl_easy_setopt(easy, CURLOPT_USERAGENT, "C-EpollClient/1.0");

    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);

    curl_multi_add_handle(multi_handle, easy);
    return easy;
}

static inline int auth_post0(const char *code) {
    HttpResponse *resp = malloc(sizeof(HttpResponse));

    const char *url = "https://login.live.com/oauth20_token.srf";

    
    char post_fields[1024];
    snprintf(post_fields, sizeof(post_fields),
            "client_id=00000000402b5328&redirect_uri=https://login.live.com/oauth20_desktop.srf&grant_type=authorization_code&code=%s",
            code);
    printf("POST body: %s\n", post_fields);

    
    http_post_0(url, post_fields, resp);
    
    AuthBlock *block = malloc(sizeof(AuthBlock));
    block->response = resp;
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

    auth_requests[slot]->response->done = 0;
    auth_requests[slot] = block;

    return 0;
}

char *authenticate(char *url, AuthBlock *block) {
    int step = (block == NULL ? 0 : block->step);
    switch (step)
    {
    case 0: {
        // LOG("Called");
        int len, start;
        sscanf(url, "https://login.live.com/oauth20_desktop.srf?code=%n%*[^&]%n", &start, &len);
        len -= start;
        char code[len + 1];
        sscanf(url, "https://login.live.com/oauth20_desktop.srf?code=%[^&]", code);

        if (len <= 0) return "Invalid URL"; // Invalid URL

        printf("Got code from the URL: %s (%d bytes)\n", code, len);
        auth_post0(code);
        break;}

    case 1: {
        printf("Step 1 response:\n");
        print_hex(block->response->buf);



        break;}
    }
    return NULL;
}