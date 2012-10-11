#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* modules */
#include "sample.h"
#include "loopback.h"
#include "xmpp.h"

#define err_exit(str)	{		\
			  perror(str);	\
			  exit(errno);	\
			}

#define PPPOAT_DESCR	"PPPoAT"
#define PPPOAT_VERSION	"dev"

#define STD_LOG_PATH	"/var/log/pppoat.log"
#define STD_PPPD_PATH	"/usr/sbin/pppd"

#define ARG_MODULE	1
#define ARG_LOG_PATH	2

struct module
{
	char *name;
	int (*func)(int, char **, int, int);
};

struct module mod_tbl[] = 
{
#ifdef MOD_SAMPLE
	{"sample", &mod_sample},
#endif
#ifdef MOD_LOOP
	{"loop", &mod_loop},
#endif
#ifdef MOD_XMPP
	{"xmpp", &mod_xmpp},
#endif
	{"", NULL},
};

int quiet = 0;
char *prog_name;

static void help(char *name)
{
	printf(PPPOAT_DESCR " version " PPPOAT_VERSION "\n"
	"Usage: %s [options] [<local_ip>:<remote_ip>] [-- [pppd options]]\n"
	"\n"
	"Options:\n"
	" -h, --help\tprint this text and exit\n"
	" -l, --list\tprint list of available modules and exit\n"
	" -L <path>\tspecify file for logging\n"
	" -m <module>\tchoose module\n"
	" -q, --quiet\tdon't print anything to stdout\n", name);
	exit(0);
}

static void list_mod()
{
	int i = 0;
	while (mod_tbl[i].func) {
		printf("%s\n", mod_tbl[i].name);
		i++;
	}
	exit(0);
}

static int is_mod(char *name)
{
	int i = 0;
	while (mod_tbl[i].func) {
		if (!strcmp(name, mod_tbl[i].name))
			return i;
		i++;
	}
	return -1;
}

static int redirect_logs(char *path)
{
	int fd;
	if (!path)
		goto out;
	fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0664);
	if (fd < 0 || dup2(fd, 2) < 0)
		goto out_fd;
	close(fd);

	return 0;

out_fd:
	if (fd > 0)
		close(fd);
out:
	fprintf(stderr, "%s: can't open log file %s\n", prog_name, path);
	fprintf(stderr, "%s: logging will continue to stderr.\n", prog_name);
	return -1;
}

int main(int argc, char **argv)
{
	/* pipe descriptors */
	int pd_rd[2], pd_wr[2];
	pid_t pid;
	char *pppd = STD_PPPD_PATH;
	char *log_path = STD_LOG_PATH;
	char *ip = NULL;
	char *mod_name = NULL;
	int mod_idx;
	int i;
	int need_arg = 0;
	int mod_argc = 0;
	char **mod_argv = NULL;

	prog_name = argv[0];

	/* parsing arguments */
	if (argc < 2)
		help(argv[0]);
	for (i = 1; i < argc; i++) {
		if (need_arg) {
			switch (need_arg) {
			case ARG_MODULE:
				mod_name = argv[i];
				break;
			case ARG_LOG_PATH:
				log_path = argv[i];
				break;
			default:
				fprintf(stderr, "there is internal problem with parsing arguments\n");
				return 1;
			}
			need_arg = 0;
			continue;
		}
		if (argv[i][0] != '-') {
			/* <local_ip>:<remote_ip> */
			ip = argv[i];
			continue;
		}
		if (!strcmp("-h", argv[i]) || !strcmp("--help", argv[i]))
			/* print help and exit */
			help(argv[0]);
		if (!strcmp("-l", argv[i]) || !strcmp("--list", argv[i]))
			/* print list of available modules and exit */
			list_mod();
		if (!strcmp("-L", argv[i])) {
			/* specify file for logging */
			need_arg = ARG_LOG_PATH;
			continue;
		}
		if (!strcmp("-m", argv[i])) {
			/* choose module */
			need_arg = ARG_MODULE;
			continue;
		}
		if (!strcmp("-q", argv[i]) || !strcmp("--quiet", argv[i])) {
			/* quiet mode */
			quiet = 1;
			continue;
		}
		if (!strcmp("--", argv[i])) {
			/* TODO: set pointer to i+1, the rest options are for pppd */
			mod_argv = argv + i;
			mod_argc = argc - i;
			break;
		}
		/* unrecognized options may be addressed for the module */
	}
	if (need_arg) {
		fprintf(stderr, "incomplete arguments, see %s --help\n", argv[0]);
		return 2;
	}
	/* check whether required arguments are set */
	if (!mod_name) {
		fprintf(stderr, "you must choose a module, see %s --help\n", argv[0]);
		return 2;
	}

	/* redirect logs to a file */
	redirect_logs(log_path);

	/* check whether module name is correct */
	if ((mod_idx = is_mod(mod_name)) < 0) {
		fprintf(stderr, "there isn't such a module: %s\n", mod_name);
		return 2;
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
		execl(pppd, pppd, "nodetach", "noauth", "notty", "passive", ip, NULL);
		err_exit("execl");
	}

	close(pd_rd[1]);
	close(pd_wr[0]);

	/* run appropriate module's function */
	return mod_tbl[mod_idx].func(mod_argc, mod_argv, pd_rd[0], pd_wr[1]);
}
