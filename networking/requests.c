#include "requests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static CURLM *multi = NULL;
static size_t request_count = 0;
static AsyncRequest **requests = NULL;

static size_t write_callback(void *data, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;
    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (!ptr) return 0;
    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;
    return realsize;
}

void http_init(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    multi = curl_multi_init();
}

void http_cleanup(void) {
    for (size_t i = 0; i < request_count; i++) {
        AsyncRequest *req = requests[i];
        curl_multi_remove_handle(multi, req->easy);
        curl_easy_cleanup(req->easy);
        if (req->headers) curl_slist_free_all(req->headers);
        free(req->chunk.response);
        free(req);
    }
    free(requests);
    curl_multi_cleanup(multi);
    curl_global_cleanup();
    requests = NULL;
    request_count = 0;
}

AsyncRequest *http_post(const char *url, const char *data, const char *content_type) {
    AsyncRequest *req = calloc(1, sizeof(AsyncRequest));
    req->chunk.response = NULL;

    req->easy = curl_easy_init();
    curl_easy_setopt(req->easy, CURLOPT_URL, url);
    curl_easy_setopt(req->easy, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(req->easy, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(req->easy, CURLOPT_WRITEDATA, &req->chunk);

    if (content_type) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Content-Type: %s", content_type);
        req->headers = curl_slist_append(NULL, buf);
        curl_easy_setopt(req->easy, CURLOPT_HTTPHEADER, req->headers);
    }

    // Add to global multi-handle
    curl_multi_add_handle(multi, req->easy);

    // Keep track in global array
    requests = realloc(requests, sizeof(AsyncRequest*) * (request_count + 1));
    requests[request_count++] = req;

    return req;
}

void http_perform(void) {
    if (!multi) return;

    int still_running = 0;
    curl_multi_perform(multi, &still_running);

    // Wait for activity on the sockets (non-blocking, short timeout)
    int numfds = 0;
    curl_multi_wait(multi, NULL, 0, 100, &numfds); // 100ms max

    // Check for completed requests
    CURLMsg *msg;
    int msgs_left;
    for (size_t i = 0; i < request_count; i++) {
        AsyncRequest *req = requests[i];
        if (!req->finished) {
            while ((msg = curl_multi_info_read(multi, &msgs_left))) {
                if (msg->msg == CURLMSG_DONE && msg->easy_handle == req->easy) {
                    req->finished = 1;
                }
            }
        }
    }
}
