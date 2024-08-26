#include <arpa/inet.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include <queue.h>
#include <tcp.h>

static int sockfd;
static struct sockaddr_in servaddr;
static unsigned servlen;
static pthread_t main_thread;

int tcp_init(char *address, unsigned port, pthread_t thread)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        fprintf(stderr, "failed creating TCP socket\n");
        return 0;
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(address);
    servaddr.sin_port = htons(port);
    struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        fprintf(stderr, "failed setting socket send timeout\n");
        return 0;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        fprintf(stderr, "failed setting socket receive timeout\n");
        return 0;
    }
    if (connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        fprintf(stderr, "failed connecting to server\n");
        return 0;
    }
    main_thread = thread;
    servlen = sizeof(servaddr);
    puts("Connected");
    return 1;
}

void *tcp_receive(void *p)
{
    struct tcp_data *data = p;
    int16_t *buf = NULL;

    while (1)
    {
        if (*data->stop)
        {
            pthread_kill(main_thread, SIGTERM); // send signal to main thread
            free(buf);
            close(sockfd);
            *data->stop = 2;
            data->q->quit = 1;
            return NULL;
        }
        int capacity = q_get_capacity(data->q);

        if (capacity >= data->q->maxsize / 2)
        {
            int size = capacity * sizeof(uint16_t);
            buf = malloc(size);
            // ask the server to provide a specific amount of data
            if (sendto(sockfd, &size, sizeof(size), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
            {

                fprintf(stderr, "failed sending data to server\n");
                *data->stop = 1;
                continue;
            }
            ssize_t n;
            if ((n = recvfrom(sockfd, buf, size, 0, (struct sockaddr *)&servaddr, &servlen)) <= 0)
            {
                if (n == 0)
                {
                    puts("Connection closed");
                    *data->stop = 1;
                    continue;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    fprintf(stderr, "timed out waiting for data from server\n");
                    *data->stop = 1;
                    continue;
                }
                if (errno == ECONNRESET)
                {
                    fprintf(stderr, "connection reset\n");
                    *data->stop = 1;
                    continue;
                }
            }
            q_enqueue(data->q, buf, capacity);
            free(buf);
            buf = NULL;
        }
    }
}
