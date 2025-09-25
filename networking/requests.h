#ifndef REQUESTS_H
#define REQUESTS_H

#include <curl/curl.h>

typedef struct {
    CURL *easy;
    struct Memory {
        char *response;
        size_t size;
    } chunk;
    int finished;
    struct curl_slist *headers;
} AsyncRequest;

void http_init(void);
void http_cleanup(void);
AsyncRequest *http_post(const char *url, const char *data, const char *content_type);
void http_perform(void); // Tick all requests

#define JSON_PAIR(key, val) _Generic((val), \
    char*: "\"" key "\":\"" val "\"", \
    const char*: "\"" key "\":\"" val "\"", \
    default: "\"" key "\":" #val \
)

#endif
