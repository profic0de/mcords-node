#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <curl/curl.h>
#include "networking/buffer.h"

typedef struct {
    Buffer *buf;
    int done; // 0 = in progress, 1 = finished
    int status_code; // HTTP status
} HttpResponse;

void http_init();
void http_cleanup();

CURL *http_get(const char *url, HttpResponse *resp);
CURL *http_post(const char *url, const char *post_fields, HttpResponse *resp);
CURL *http_post_json(const char *url, const char *json_data, HttpResponse *resp);

void http_perform();

#define JSON_PAIR(key, val) _Generic((val), \
    char*: "\"" key "\":\"" val "\"", \
    const char*: "\"" key "\":\"" val "\"", \
    default: "\"" key "\":" #val \
)

#endif
