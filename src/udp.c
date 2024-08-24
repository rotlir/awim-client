#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <udp.h>
#include <errno.h>
#include <time.h>

struct sockaddr_in server;
int socket_desc;
char *server_addr;
unsigned int server_struct_length;

int udp_init(char *server_address, unsigned port)
{
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_desc < 0)
    {
        fprintf(stderr, "error while creating socket.\n");
        return 0;
    }
    struct timeval timeout = {.tv_sec = 0, .tv_usec = 100000};
    if (setsockopt(socket_desc, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        fprintf(stderr, "failed setting socket send timeout\n");
        return 0;
    }
    if (setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        fprintf(stderr, "failed setting socket receive timeout\n");
        return 0;
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(server_address);
    server_addr = server_address;
    server_struct_length = sizeof(server);

    return 1;
}

void *udp_receive(void *p)
{
    struct udp_message_properties *data = p;
    int16_t *udp_buf = NULL;
    while (1)
    {
        if (*data->stop)
        {
            free(udp_buf);
            return NULL;
        }
        int capacity = q_get_capacity(data->q);
        if (capacity > data->q->maxsize / 2)
        {
            int size = capacity * sizeof(int16_t);
            udp_buf = malloc(size);
            // ask the server to provide a specific amount of data
            if (sendto(socket_desc, &size, sizeof(size), 0, (struct sockaddr *)&server, sizeof(server)) == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    continue;
                }
            }
            if (recvfrom(socket_desc, udp_buf, size, 0, (struct sockaddr *)&server, &server_struct_length) == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // fprintf(stderr, "timed out waiting for data from server\n");
                    continue;
                }
            }
            q_enqueue(data->q, udp_buf, capacity);
            free(udp_buf);
            udp_buf = NULL;
        }
    }
}