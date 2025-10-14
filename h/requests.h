#ifndef REQUESTS_H
#define REQUESTS_H

#include <curl/curl.h>
#include "h/buffer.h"
#include <stddef.h>

// Represents a queued HTTP request / response
typedef struct {
    CURL *easy;               // CURL easy handle (created in perform)
    Buffer *buf;              // Response buffer
    int done;                 // Mark finished
    struct curl_slist *headers; // Optional headers
    char *url;                // Request URL (stored for delayed sending)
    char *post_data;          // POST body (stored for delayed sending)
    char *content_type;       // Content-Type header
} HttpResponse;

// Global multi-handle and request array
extern CURLM *multi;
extern size_t request_count;
extern HttpResponse **requests;

// Init / cleanup
void http_init(void);
void http_cleanup(void);

// Queue a POST request (does not send yet)
HttpResponse *http_post(const char *url, const char *data, const char *content_type);
void http_free(HttpResponse *req);

// Tick all queued requests (send/receive)
void http_perform(void);

// Helper macro to generate JSON key/value pair strings
#define JSON_PAIR(key, val) _Generic((val), \
    char*: "\"" key "\":\"" val "\"", \
    const char*: "\"" key "\":\"" val "\"", \
    default: "\"" key "\":" #val \
)

#endif
