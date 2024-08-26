#include <queue.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

int q_init(queue_t *q, unsigned maxsize)
{
    q->maxsize = maxsize;
    q->data = malloc(maxsize * sizeof(int16_t));
    if (q->data == NULL)
    {
        return 0;
    }
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    q->quit = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->half_filled, NULL);

    return 1;
}

void q_deinit(queue_t *q)
{
    free(q->data);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->half_filled);
}

int q_enqueue(queue_t *q, int16_t *data, unsigned num)
{
    pthread_mutex_lock(&q->mutex);
    for (int i = 0; i < num; i++)
    {
        while (q->size == q->maxsize)
        {
            pthread_cond_wait(&q->half_filled, &q->mutex);
        }
        q->data[q->rear] = data[i];
        q->rear = (q->rear + 1) % q->maxsize;
        q->size++;
        if (q->size == q->maxsize / 2)
            pthread_cond_signal(&q->half_filled);
    }
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

int q_dequeue(queue_t *q, int16_t *data, unsigned num)
{
    if (q->quit) return 0;
    pthread_mutex_lock(&q->mutex);
    for (int i = 0; i < num; i++)
    {
        while (!q->size)
        {
            struct timespec ts = {.tv_sec = 1, .tv_nsec = 0};
            pthread_cond_timedwait(&q->half_filled, &q->mutex, &ts);
            if (q->quit) break;
        }
        if (q->quit) {
            for (int j = 0; i < num; i++) {
                data[j] = 0;
            }
            pthread_mutex_unlock(&q->mutex);
            return 0;
        }
        data[i] = q->data[q->front];
        q->front = (q->front + 1) % q->maxsize;
        q->size--;
        if (q->size == q->maxsize / 2)
            pthread_cond_signal(&q->half_filled);
    }
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

int q_get_capacity(queue_t *q)
{
    int capacity = q->maxsize - q->size;
    return capacity;
}
