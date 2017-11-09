#include <stdio.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>

#include <ctype.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "test-client.h"
#include <vs4l.h>

char node_name[30] = "/dev/vertex0";

int vpu_open(struct client *client)
{
	int ret = 0;

	client->fd = open(node_name, O_RDWR, 0);
	if (client->fd <= 0) {
		printf("%s():%d\n", __func__, client->fd);
		ret = client->fd;
	}

	return ret;
}

int vpu_s_graph(struct client *client, struct vs4l_graph *graph)
{
	int ret = 0;

	if (graph->id == 0) graph->id = client->fd;

	ret = ioctl(client->fd, VS4L_VERTEXIOC_S_GRAPH, graph);
	if (ret)
		printf("%s():%d\n", __func__, ret);

	return ret;
}

int vpu_s_format(struct client *client, struct vs4l_format_list *flist)
{
	int ret = 0;

	ret = ioctl(client->fd, VS4L_VERTEXIOC_S_FORMAT, flist);
	if (ret)
		printf("%s():%d\n", __func__, ret);

	return ret;
}

int vpu_s_param(struct client *client, struct vs4l_param_list *plist)
{
	int ret = 0;

	ret = ioctl(client->fd, VS4L_VERTEXIOC_S_PARAM, plist);
	if (ret)
		printf("%s():%d\n", __func__, ret);

	return ret;
}

int vpu_s_ctrl(struct client *client, struct vs4l_ctrl *ctrl)
{
	int ret = 0;

	ret = ioctl(client->fd, VS4L_VERTEXIOC_S_CTRL, ctrl);
	if (ret)
		printf("%s():%d\n", __func__, ret);

	return ret;
}

int vpu_stream_on(struct client *client)
{
	int ret = 0;

	ret = ioctl(client->fd, VS4L_VERTEXIOC_STREAM_ON, NULL);
	if (ret)
		printf("%s():%d\n", __func__, ret);

	return ret;
}

int vpu_stream_off(struct client *client)
{
	int ret = 0;

	ret = ioctl(client->fd, VS4L_VERTEXIOC_STREAM_OFF, NULL);
	if (ret)
		printf("%s():%d\n", __func__, ret);

	return ret;
}

int vpu_poll(struct client *client)
{
	int ret = 0;
	struct pollfd poll_events;

	poll_events.fd = client->fd;
	poll_events.events = POLLIN | POLLOUT | POLLERR;
	poll_events.revents = 0;

	ret = poll(&poll_events, 1, 1000);
	if (!ret) {
		printf("%s():%d\n", __func__, ret);
		goto p_err;
	}

	if (!(poll_events.revents & POLLIN)) {
		printf("POLLIN error\n");
		ret = -EINVAL;
		goto p_err;
	}

	if (!(poll_events.revents & POLLOUT)) {
		printf("POLLOUT error\n");
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

int vpu_qbuf(struct client *client, struct vs4l_container_list *c)
{
	int ret = 0;

	ret = ioctl(client->fd, VS4L_VERTEXIOC_QBUF, c);
	if (ret)
		printf("%s():%d\n", __func__, ret);

	return ret;
}

int vpu_dqbuf(struct client *client, struct vs4l_container_list *c)
{
	int ret = 0;

	ret = ioctl(client->fd, VS4L_VERTEXIOC_DQBUF, c);
	if (ret) {
		printf("%s():%d\n", __func__, ret);
		goto p_err;
	}

	if (c->flags & (1 << VS4L_CL_FLAG_INVALID)) {
		printf("%s(%d):INVALID(0x%X)\n", __func__, c->index, c->flags);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

int vpu_close(struct client *client)
{
	close(client->fd);
	return 0;
}
