#include <spa/param/audio/format-utils.h>

#include <pipewire/pipewire.h>
#include <pipewire/impl.h>
#include <spa/utils/dict.h>

#include <pthread.h>
#include <queue.h>
#include <pw.h>
#include <udp.h>
#include <tcp.h>
#include <time.h>

#define DEFAULT_RATE 48000
#define DEFAULT_CHANNELS 1

#define DEBUG 0

volatile int udp_stop = 0;

struct cleanup_data
{
	struct pw_data *data;
};

static volatile struct cleanup_data cleanup;

static void do_quit(int signal_number)
{
	puts("\nstopping");
	pw_main_loop_quit(cleanup.data->loop);
}

/* [on_process] */
static void on_process(void *userdata)
{
	struct pw_data *data = userdata;
	struct pw_buffer *b;
	struct spa_buffer *buf;
	int i, c, n_frames, stride;
	int16_t *dst;

	if ((b = pw_stream_dequeue_buffer(data->stream)) == NULL)
	{
		pw_log_warn("out of buffers: %m");
		return;
	}

	buf = b->buffer;
	if ((dst = buf->datas[0].data) == NULL)
		return;

	stride = sizeof(int16_t) * DEFAULT_CHANNELS;
	n_frames = buf->datas[0].maxsize / stride;
	if (b->requested)
		n_frames = SPA_MIN(b->requested, n_frames);

	data->qbuf = malloc(n_frames * sizeof(int16_t));
	q_dequeue(&data->q, data->qbuf, n_frames);
	if (DEBUG)
		printf("dequeued %i frames\n", n_frames);
	if (data->q.quit)
		return;
	for (i = 0; i < n_frames; i++)
	{
		for (c = 0; c < DEFAULT_CHANNELS; c++)
			*dst++ = data->qbuf[i];
	}
	if (data->qbuf != NULL)
	{
		free(data->qbuf);
		data->qbuf = NULL;
	}

	buf->datas[0].chunk->offset = 0;
	buf->datas[0].chunk->stride = stride;
	buf->datas[0].chunk->size = n_frames * stride;

	pw_stream_queue_buffer(data->stream, b);
}

static const struct pw_stream_events stream_events = {
	PW_VERSION_STREAM_EVENTS,
	.process = on_process};

int start_pipewire(int *argc, char *argv[], unsigned bufsize, char *pw_target, int tcp_mode)
{
	struct pw_data data = {
		0,
	};
	struct pw_loop *l;
	data.exit_code = 0;
	if (q_init(&data.q, bufsize) == 0)
	{
		fprintf(stderr, "failed creating queue\n");
		data.exit_code = -1;
		goto exit;
	}
	const struct spa_pod *params[1];
	uint8_t buffer[1024];
	struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

	pw_init(argc, &argv);

	data.loop = pw_main_loop_new(NULL);
	if (data.loop == NULL)
	{
		fprintf(stderr, "failed creating main loop\n");
		data.exit_code = -1;
		goto exit;
	}
	l = pw_main_loop_get_loop(data.loop);
	data.context = pw_context_new(l, NULL, 0);
	if (data.context == NULL)
	{
		fprintf(stderr, "failed creating context\n");
		data.exit_code = -1;
		goto exit;
	}

	/*
	  loopback module with these arguments will create a virtual sink and a virtual source monitoring sink
	*/
	const char *loopback_args =
		"{audio.position = [ MONO ] "
		"capture.props = { "
		"media.class = Audio/Sink "
		"node.name = AWiM-sink "
		"node.description = AWiM-sink "
		"} "
		"playback.props = { "
		"media.class = Audio/Source "
		"node.name = AWiM-source "
		"node.description = AWiM-source "
		"} "
		"} ";

	data.module = pw_context_load_module(data.context, "libpipewire-module-loopback", loopback_args, NULL);
	if (data.module == NULL)
	{
		fprintf(stderr, "failed loading loopback module\n");
		data.exit_code = -1;
		goto exit;
	}

	data.stream = pw_stream_new_simple(
		pw_main_loop_get_loop(data.loop), "AWiM",
		pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
						  PW_KEY_MEDIA_CATEGORY, "Playback",
						  PW_KEY_MEDIA_ROLE, "Music",
						  PW_KEY_TARGET_OBJECT, pw_target,
						  NULL),
		&stream_events, &data);

	params[0] = spa_format_audio_raw_build(
		&b, SPA_PARAM_EnumFormat,
		&SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_S16,
								 .channels = DEFAULT_CHANNELS,
								 .rate = DEFAULT_RATE));

	if (pw_stream_connect(data.stream,
						  PW_DIRECTION_OUTPUT, PW_ID_ANY,
						  PW_STREAM_FLAG_AUTOCONNECT |
							  PW_STREAM_FLAG_MAP_BUFFERS |
							  PW_STREAM_FLAG_RT_PROCESS,
						  params, 1) < 0)
	{
		fprintf(stderr, "error connecting stream\n");
		data.exit_code = -1;
		goto exit;
	}

	cleanup.data = &data;
	signal(SIGINT, do_quit);
	signal(SIGTERM, do_quit);
	if (tcp_mode)
	{
		struct tcp_data tcp_data;
		tcp_data.q = &data.q;
		tcp_data.stop = &udp_stop;
		pthread_create(&data.thread_id, NULL, tcp_receive, (void *)&tcp_data);
	}
	else
	{
		struct udp_message_properties udp_msg;
		udp_msg.q = &data.q;
		udp_msg.stop = &udp_stop;
		pthread_create(&data.thread_id, NULL, udp_receive, (void *)&udp_msg);
	}
	pw_main_loop_run(data.loop);

exit:
	udp_stop = 1;
	if (data.q.data)
		q_deinit(&data.q);
	if (data.qbuf != NULL)
	{
		free(data.qbuf);
		data.qbuf = NULL;
	}
	if (data.module)
		pw_impl_module_destroy(data.module);
	if (data.context)
		pw_context_destroy(data.context);
	if (data.stream)
		pw_stream_destroy(data.stream);
	if (data.loop)
		pw_main_loop_destroy(data.loop);

	return data.exit_code;
}
