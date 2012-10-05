#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

/* modules */
#include "sample.h"
#include "loopback.h"

#define err_exit(str)	{		\
			  perror(str);	\
			  exit(errno);	\
			}

#define STD_LOG_NAME	"/var/log/pppoat.log"
#define STD_PPPD_PATH	"/usr/sbin/pppd"

struct module
{
	char *name;
	int (*func)(int, char **, int, int);
};

struct module mod_tbl[] = 
{
	{"sample", &mod_sample},
	{"loop", &mod_loop},
	{"", NULL},
};

int quiet = 0;
char log_name[] = STD_LOG_NAME;
char *prog_name;

void err_log()
{
	fprintf(stderr, "%s: can't open log file %s\n", prog_name, log_name);
	fprintf(stderr, "%s: logging will continue to stderr.\n", prog_name);
}

int main(int argc, char **argv)
{
	/* pipe descriptors */
	int pd_rd[2], pd_wr[2];
	/* file descritor for logs */
	int fd;
	pid_t pid;
	char pppd[] = STD_PPPD_PATH;

	prog_name = argv[0];

	/* TODO: deal with arguments here */
	if (argc < 2) { /* arguments ain't implemented yet... just write some words */
		printf("pppoat dev version\n"
		       "currently supports only loopback module\n"
		       "\n"
		       "usage: %s {local_ip}\n"
		       "local_ip\t':' must follows the ip address; for testing purpose\n",
		       argv[0]);
		return 2;
	}

	/* redirect logs to a file */
	fd = open(log_name, O_WRONLY | O_CREAT | O_APPEND, 0664);
	if (fd < 0)
		err_log();
	else {
		if (dup2(fd, 2) < 0)
			err_log();
		close(fd);
	}

	/* create pipes for communication with pppd */
	if (pipe(pd_rd) < 0)
		err_exit("pipe");
	if (pipe(pd_wr) < 0)
		err_exit("pipe");

	/* exec pppd */
	if ((pid = fork()) < 0)
		err_exit("fork");
	if (!pid) {
		if (dup2(pd_rd[1], 1) < 0)
			err_exit("dup2");
		if (dup2(pd_wr[0], 0) < 0)
			err_exit("dup2");
		close(pd_rd[0]);
		close(pd_rd[1]);
		close(pd_wr[0]);
		close(pd_wr[1]);
		execl(pppd, pppd, "nodetach", "noauth", "notty", "passive", argv[1], NULL);
		err_exit("execl");
	}

	close(pd_rd[1]);
	close(pd_wr[0]);

	/* TODO: run appropriate module's function here */
	return mod_loop(argc, argv, pd_rd[0], pd_wr[1]);
}
