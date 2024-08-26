#pragma once

#include <pthread.h>
#include <queue.h>

struct tcp_data
{
    queue_t *q;
    volatile int *stop;
};

int tcp_init(char *address, unsigned port, pthread_t main_thread);
void *tcp_receive(void *p);
