#include "packet/parsing/vars.h"
#include "packet/building/vars.h"
#include "utils/asyncio.h"
#include "utils/logger.h"
#include "utils/globals.h"
#include "utils/player.h"
#include "utils/packet.h"
#include "status.h"
#include "stdio.h"

void handle_status(int fd, Buffer *buffer) {
    // Player *p = players[fd];
    int error;
    int packet_id = parse_varint(buffer, &error);
    Buffer *out = init_buffer();
    switch (packet_id)
    {
    case 0:
        build_varint(out, 0);
        build_string(out, "{\"version\":{\"name\":\"1.21.5\",\"protocol\":770},\"players\":{\"max\":100,\"online\":5,\"sample\":[{\"name\":\"thinkofdeath\",\"id\":\"4566e69f-c907-48ee-8d71-d7ba5aa00d20\"}]},\"description\":{\"text\":\"Hello, world!\"},\"favicon\":\"data:image/png;base64:<data>\",\"enforcesSecureChat\":false}");
        packet_send(out, fd);
        // LOG("Buffer: %s", hex(out));
        break;

    case 1:
        int error = 0;
        build_varint(out, 1);
        build_integer(out, parse_integer(buffer, 8, 1,&error), 8, 1);
        // LOG("Error: %i", error);
        packet_send(out, fd);
        break;

    default:
        break;
    }
    free(out->buffer);
    // printf("a\n");
}
