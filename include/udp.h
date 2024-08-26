#pragma once
#include <pw.h>
#include <queue.h>

struct udp_message_properties
{
    queue_t *q;
    volatile int *stop;
};

int udp_init(char *server_address, unsigned port);
void *udp_receive(void *p);