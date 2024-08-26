#pragma once
#include <pipewire/pipewire.h>
#include <queue.h>

struct pw_data
{
        struct pw_main_loop *loop;
        struct pw_stream *stream;
        struct pw_impl_module *module;
        struct pw_context *context;
        pthread_t thread_id;
        int exit_code;
        queue_t q;
        int16_t *qbuf;
};

int start_pipewire(int *argc, char *argv[], unsigned packet_size, char* pw_target, int tcp_mode);