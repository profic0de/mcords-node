#include "engine/packets.h"
#include "build/packet.h"
#include <stdio.h>
#include <stdlib.h>
#include "h/fds.h"
#include "h/mem.h"
#include "h/packet.h"

int process_packet(Packet* packet) {
    print_readable(packet->buf);
    // printf("%i\n",fds_incr(packet->from, "count"));

    // Buffer* buf = init_buffer();
    // build_varint(buf, 0x00);
    // build_string(buf, "{'text':'test'}");
    // packet_send(buf, packet->from);
    // free_buffer(buf);

    return 0;
}
