#include <stdlib.h>
#include "packet.h"

char *buffer_to_string(Buffer *buf, int *out_len) {
    if (!buf) return NULL;

    // inline VarInt encoding
    unsigned char varint[5];
    int value = buf->length;
    int varint_len = 0;
    do {
        unsigned char temp = value & 0x7F;
        value >>= 7;
        if (value != 0) temp |= 0x80;
        varint[varint_len++] = temp;
    } while (value != 0);

    int total_len = varint_len + buf->length;
    char *out = malloc(total_len);
    if (!out) return NULL;

    memcpy(out, varint, varint_len);
    memcpy(out + varint_len, buf->buffer, buf->length);

    if (out_len) *out_len = total_len;
    return out; // caller must free()
}

Buffer *string_to_buffer(const char *data, int *consumed, int *error) {
    if (!data) {
        if (error) (*error)++;
        return NULL;
    }

    // decode VarInt
    int value = 0;
    int shift = 0;
    int i = 0;
    unsigned char byte;

    do {
        byte = (unsigned char)data[i++];
        value |= (byte & 0x7F) << shift;
        shift += 7;
        if (i > 5) { // VarInt too long
            if (error) (*error)++;
            return NULL;
        }
    } while (byte & 0x80);

    int length = value;

    // build buffer directly
    Buffer *buf = init_buffer();
    append_to_buffer(buf, data + i, length);

    if (consumed) *consumed = i + length; // how many bytes we used
    return buf;
}
