#pragma once

#include <stdint.h>
#include <pthread.h>

// implementation of a ring buffer
typedef struct queue
{
    int front;
    int rear;
    int16_t *data;
    unsigned size;
    unsigned maxsize;
    pthread_mutex_t mutex;
    pthread_cond_t half_filled;
} queue_t;

/*
    init a queue
    returns 1 on success, 0 on failure
*/
int q_init(queue_t *q, unsigned size);

void q_deinit(queue_t *q);

/*
    append num elements from data to queue
*/
int q_enqueue(queue_t *q, int16_t *data, unsigned num);

/*
    dequeue num elements from the queue and put them into data
*/
int q_dequeue(queue_t *q, int16_t *data, unsigned num);

/*
    get a number of elements the queue can currently store
*/
int q_get_capacity(queue_t *q);
