#include "h/buffer.h"
#include "h/requests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

CURLM *multi = NULL;
size_t request_count = 0;
HttpResponse **requests = NULL;

size_t write_callback(void *data, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    Buffer *buf = (Buffer *)userp;

    char *ptr = realloc(buf->buffer, buf->length + realsize + 1);
    if (!ptr) return 0;

    buf->buffer = ptr;
    memcpy(buf->buffer + buf->length, data, realsize);
    buf->length += realsize;
    buf->buffer[buf->length] = '\0';

    return realsize;
}

void http_init(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    multi = curl_multi_init();
}

void http_cleanup(void) {
    for (size_t i = 0; i < request_count; i++) {
        HttpResponse *req = requests[i];
        if (req->easy) curl_easy_cleanup(req->easy);
        if (req->headers) curl_slist_free_all(req->headers);
        free(req->buf->buffer);
        free(req->buf);
        free(req);
    }
    free(requests);
    curl_multi_cleanup(multi);
    curl_global_cleanup();
    requests = NULL;
    request_count = 0;
}

void http_free(HttpResponse *req) {
    if (!req || !requests) return;

    // Find the request in the array
    for (size_t i = 0; i < request_count; i++) {
        if (requests[i] == req) {
            // Free the request itself
            if (!req) return;

            if (req->easy) curl_easy_cleanup(req->easy);
            if (req->headers) curl_slist_free_all(req->headers);

            if (req->url) free(req->url);
            if (req->post_data) free(req->post_data);
            if (req->content_type) free(req->content_type);

            if (req->buf) {
                if (req->buf->buffer) free(req->buf->buffer);
                free(req->buf);
            }

            free(req);

            // Shift remaining requests left
            if (i < request_count - 1) {
                memmove(&requests[i], &requests[i + 1],
                        (request_count - i - 1) * sizeof(HttpResponse *));
            }

            request_count--;

            // Shrink the array
            if (request_count > 0) {
                HttpResponse **tmp = realloc(requests, request_count * sizeof(HttpResponse *));
                if (tmp) {
                    requests = tmp; // only replace if realloc succeeded
                }
            } else {
                free(requests);
                requests = NULL;
            }

            return;
        }
    }
}

// Only queue, do not create the CURL handle yet
HttpResponse *http_post(const char *url, const char *data, const char *content_type) {
    HttpResponse *req = calloc(1, sizeof(HttpResponse));
    req->buf = init_buffer();
    req->done = 0;

    req->url = strdup(url);
    req->post_data = strdup(data);
    if (content_type) req->content_type = strdup(content_type);

    requests = realloc(requests, sizeof(HttpResponse*) * (request_count + 1));
    requests[request_count++] = req;

    return req;
}

void http_perform(void) {
    if (!multi) return;

    // Initialize any queued request that hasn't started yet
    for (size_t i = 0; i < request_count; i++) {
        HttpResponse *req = requests[i];
        if (!req->easy) {
            req->easy = curl_easy_init();
            curl_easy_setopt(req->easy, CURLOPT_URL, req->url);
            curl_easy_setopt(req->easy, CURLOPT_POSTFIELDS, req->post_data);
            curl_easy_setopt(req->easy, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(req->easy, CURLOPT_WRITEDATA, req->buf);

            if (req->content_type) {
                char buf[128];
                snprintf(buf, sizeof(buf), "Content-Type: %s", req->content_type);
                req->headers = curl_slist_append(NULL, buf);
                curl_easy_setopt(req->easy, CURLOPT_HTTPHEADER, req->headers);
            }

            curl_easy_setopt(req->easy, CURLOPT_PRIVATE, req);
            curl_multi_add_handle(multi, req->easy);
        }
    }

    int still_running = 0;
    curl_multi_perform(multi, &still_running);

    int numfds = 0;
    curl_multi_wait(multi, NULL, 0, 100, &numfds);

    // Check for completed requests
    int msgs_left;
    CURLMsg *msg;
    while ((msg = curl_multi_info_read(multi, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            HttpResponse *req = NULL;
            curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, (void**)&req);
            if (req) req->done = 1;
        }
    }
}
