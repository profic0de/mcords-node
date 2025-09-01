#include "requests.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static CURLM *multi_handle = NULL;

static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    HttpResponse *resp = (HttpResponse *)userp;

    append_to_buffer(resp->buf, contents, total);
    return total;
}

void http_init() {
    curl_global_init(CURL_GLOBAL_ALL);
    multi_handle = curl_multi_init();
}

void http_cleanup() {
    curl_multi_cleanup(multi_handle);
    curl_global_cleanup();
}

CURL *http_get(const char *url, HttpResponse *resp) {
    resp->buf = init_buffer();
    resp->done = 0;
    resp->status_code = 0;

    CURL *easy = curl_easy_init();
    curl_easy_setopt(easy, CURLOPT_URL, url);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, resp);
    curl_easy_setopt(easy, CURLOPT_PRIVATE, resp);
    curl_easy_setopt(easy, CURLOPT_USERAGENT, "C-EpollClient/1.0");

    curl_multi_add_handle(multi_handle, easy);
    return easy;
}

CURL *http_post(const char *url, const char *post_fields, HttpResponse *resp) {
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

    curl_multi_add_handle(multi_handle, easy);
    return easy;
}

CURL *http_post_json(const char *url, const char *json_data, HttpResponse *resp) {
    resp->buf = init_buffer();
    resp->done = 0;
    resp->status_code = 0;

    CURL *easy = curl_easy_init();
    curl_easy_setopt(easy, CURLOPT_URL, url);
    curl_easy_setopt(easy, CURLOPT_POSTFIELDS, json_data);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, resp);
    curl_easy_setopt(easy, CURLOPT_PRIVATE, resp);
    curl_easy_setopt(easy, CURLOPT_USERAGENT, "C-EpollClient/1.0");

    // Important: set JSON header
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);

    curl_multi_add_handle(multi_handle, easy);
    return easy;
}

void http_perform() {
    int still_running = 0;
    curl_multi_perform(multi_handle, &still_running);

    CURLMsg *msg;
    int msgs_left;
    while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            HttpResponse *resp;
            curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, (char **)&resp);
            curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &resp->status_code);

            resp->done = 1;

            curl_multi_remove_handle(multi_handle, msg->easy_handle);
            curl_easy_cleanup(msg->easy_handle);
        }
    }
}
