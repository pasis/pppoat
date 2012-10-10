#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>

#include "loopback.h"

typedef struct {
	int rd;
	int wr;
} arg_t;

static void *mod_loop_run(void *arg)
{
	arg_t *fd = (arg_t *)arg;
	ssize_t c;

	while (1) {
		c = splice(fd->rd, NULL, fd->wr, NULL, LOOP_BUFSIZ, 0);
		if (!c)
			break;
		else if (c < 0)
			perror("splice");
	}

	return NULL;
}

static int check_fifo_stat(char *path)
{
	struct stat buf;
	if (stat(path, &buf) < 0)
		return -errno;
	if (!(buf.st_mode & S_IFIFO))
		return -EBADF;

	return 0;
}

static int get_fd_master(int *rd, int *wr)
{
	*rd = open(LOOP_FIFO_RD, O_RDONLY);
	if (*rd < 0)
		return -EBADF;
	*wr = open(LOOP_FIFO_WR, O_WRONLY);
	if (*wr < 0) {	/* rd is open */
		close(*rd);
		return -EBADF;
	}

	return 0;
}

static int get_fd_slave(int *rd, int *wr)
{
	*wr = open(LOOP_FIFO_RD, O_WRONLY | O_NDELAY);
	if (*wr < 0)
		return get_fd_master(rd, wr);
	*rd = open(LOOP_FIFO_WR, O_RDONLY | O_NDELAY);
	if (*rd < 0) {	/* wr is open */
		close(*wr);
		return -EBADF;
	}

	return 0;
}

int mod_loop(int argc, char **argv, int rd, int wr)
{
	int fifo_rd, fifo_wr;
	int fifo_creat = 0;
	pthread_t thread_rd, thread_wr;
	arg_t arg_rd, arg_wr;

	if (!mknod(LOOP_FIFO_RD, S_IFIFO | 0660, 0))
		fifo_creat = 1;
	else if (errno != EEXIST || check_fifo_stat(LOOP_FIFO_RD))
		return -EBADF;

	if (!fifo_creat && check_fifo_stat(LOOP_FIFO_WR))
		return -EBADF;
	if (fifo_creat && mknod(LOOP_FIFO_WR, S_IFIFO | 0660, 0)) {
		unlink(LOOP_FIFO_RD);
		return -EBADF;
	}

	if (fifo_creat)
		get_fd_master(&fifo_rd, &fifo_wr);
	else
		get_fd_slave(&fifo_rd, &fifo_wr);

	arg_rd.rd = rd;
	arg_rd.wr = fifo_wr;
	arg_wr.rd = fifo_rd;
	arg_wr.wr = wr;

	pthread_create(&thread_rd, NULL, &mod_loop_run, (void *)&arg_rd);
	pthread_create(&thread_wr, NULL, &mod_loop_run, (void *)&arg_wr);

	pthread_join(thread_rd, NULL);
	pthread_join(thread_wr, NULL);

	return 0;
}
