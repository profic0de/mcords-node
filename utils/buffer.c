#include "h/buffer.h"
#include <stdlib.h>
#include <string.h>

void cut_buffer(Buffer *buf, int count) {
    if (count == 0 || abs(count) > buf->length) return;

    int new_len = buf->length - abs(count);
    char *new_buf = malloc(new_len);
    if (!new_buf) return;  // malloc failed

    if (count < 0) {
        // Negative: cut from the beginning
        memcpy(new_buf, buf->buffer + (-count), new_len);
    } else {
        // Positive: cut from the end
        memcpy(new_buf, buf->buffer, new_len);
    }

    free(buf->buffer);
    buf->buffer = new_buf;
    buf->length = new_len;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void append_to_buffer(Buffer *buf, const char *data, int data_len) {
    if (data_len <= 0) return;

    char *new_buf = realloc(buf->buffer, buf->length + data_len);
    if (!new_buf) return;  // allocation failed

    memcpy(new_buf + buf->length, data, data_len);

    buf->buffer = new_buf;
    buf->length += data_len;
}

void prepend_to_buffer(Buffer *buf, const char *data, int data_len) {
    if (data_len <= 0) return;

    char *new_buf = malloc(buf->length + data_len);
    if (!new_buf) return;

    memcpy(new_buf, data, data_len);                     // insert new first
    memcpy(new_buf + data_len, buf->buffer, buf->length); // then old

    free(buf->buffer);
    buf->buffer = new_buf;
    buf->length += data_len;
}

Buffer *init_buffer() {
    Buffer *buf = malloc(sizeof(Buffer));
    if (!buf) return NULL;

    buf->length = 0;
    buf->buffer = NULL;
    return buf;
}

void clear_buffer(Buffer *buf) {
    if (!buf) return;        // Safety check
    free(buf->buffer);       // Free the internal buffer
    buf->buffer = NULL;      // Reset pointer
    buf->length = 0;         // Reset length
}

void free_buffer(Buffer *buf) {
    if (buf) {
        free(buf->buffer);
        free(buf);
    }
}

#include <stdio.h>

char *hex(const Buffer *buf) {
    if (!buf || !buf->buffer || buf->length == 0) return NULL;

    size_t hexlen = buf->length * 2;
    size_t totallen = hexlen + 1; // +1 for null terminator

    char *hexstr = malloc(totallen);
    if (!hexstr) return NULL;

    for (size_t i = 0; i < buf->length; ++i) {
        sprintf(hexstr + i * 2, "%02X", (unsigned char)buf->buffer[i]);
    }
    hexstr[hexlen] = '\0';
    return hexstr;
}

void print_hex(const Buffer *buf) {
    if (!buf || !buf->buffer || buf->length == 0) return;

    printf("%s","");
    for (size_t i = 0; i < buf->length; ++i) {
        fprintf(stdout, "%02X", (unsigned char)buf->buffer[i]);
    }
    fprintf(stdout, "\n");
}

void print_readable(const Buffer *buf) {
    if (!buf || !buf->buffer || buf->length == 0) return;

    printf("%s","");
    for (size_t i = 0; i < buf->length; i++) {
        unsigned char c = buf->buffer[i];
        if (c >= 32 && c <= 126) {
            fputc(c, stdout);  // printable ASCII
        } else {
            fputc('.', stdout); // non-printable -> dot
        }
    }
    fputc('\n', stdout);
    // printf("");
}
