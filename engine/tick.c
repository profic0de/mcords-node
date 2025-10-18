#include "engine/packets.h"
#include "engine/tick.h"
#include "globals.h"
#include <malloc.h>
#include <stdio.h>

// int ticks = 0;

void tick() {
    // if (++ticks%100==0) exitbool = 1;
    Packet** packet_ql = malloc(sizeof(Packet*));
    int packet_l = 0;
    for (int i = 0; i < packets; i++) {
        if (!packet_queue[i]) continue;
        if (packet_queue[i]->state) {
            packet_ql[packet_l] = packet_queue[i];
            packet_ql = realloc(packet_ql, sizeof(Packet*)*++packet_l);
        } else {
            process_packet(packet_queue[i]);
            free_buffer(packet_queue[i]->buf);
            free(packet_queue[i]);
        }
    }

    free(packet_queue);
    packet_queue = packet_l?packet_ql:NULL;
    packets = packet_l?packet_l:0;
    if (!packet_l) free(packet_ql);
}