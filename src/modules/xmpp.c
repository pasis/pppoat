/* modules/xmpp.c
 * PPP over Any Transport -- XMPP module
 *
 * Copyright (C) 2012-2015 Dmitry Podgorny <pasis.ua@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <string.h>
#include <strophe.h>

#include "modules/xmpp.h"
#include "base64.h"
#include "pppoat.h"

struct pppoat_xmpp_ctx {
	int unused;
};

static int module_xmpp_init(int argc, char **argv, void **userdata)
{
	return -ENOSYS;
}

static void module_xmpp_fini(void *userdata)
{
}

static int module_xmpp_run(int rd, int wr, int ctrl, void *userdata)
{
	return -ENOSYS;
}

const struct pppoat_module pppoat_module_xmpp = {
	.m_name  = "xmpp",
	.m_descr = "PPP over Jabber",
	.m_init  = &module_xmpp_init,
	.m_fini  = &module_xmpp_fini,
	.m_run   = &module_xmpp_run,
};

#if 0 /* XXX from old version */
#define pr_log(msg, ...) fprintf(stderr, __FILE__ ": " msg, ##__VA_ARGS__)

typedef struct descr {
	int rd;
	int wr;
	xmpp_ctx_t *ctx;
	xmpp_conn_t *conn;
	char *to;
} descr_t;

char *rmt_jid = NULL;
pthread_t thread;
pthread_mutex_t mtx;

static inline void set_jid_safe(char *jid)
{
	pthread_mutex_lock(&mtx);
	rmt_jid = jid;
	pthread_mutex_unlock(&mtx);
}

static inline char *get_jid_safe(descr_t *fd)
{
	pthread_mutex_lock(&mtx);
	fd->to = rmt_jid;
	pthread_mutex_unlock(&mtx);
	return fd->to;
}

static void *run(void *data)
{
	xmpp_stanza_t *txt, *msg, *body;
	descr_t *fd = (descr_t *)data;
	char buf[2048];
	char *buf2;
	ssize_t len;

	while (1) {
		len = read(fd->rd, buf, 2048);
		if (len < 0)
			break;
		if (!fd->to && !get_jid_safe(fd))
			continue; /* drop */
		buf2 = b64_encode(buf, len);
		if (!buf2)
			continue;
		txt = xmpp_stanza_new(fd->ctx);
		msg = xmpp_stanza_new(fd->ctx);
		body = xmpp_stanza_new(fd->ctx);
		xmpp_stanza_set_text(txt, buf2);
		xmpp_stanza_set_name(msg, "message");
		xmpp_stanza_set_type(msg, "chat");
		xmpp_stanza_set_attribute(msg, "to", fd->to);
		xmpp_stanza_set_name(body, "body");
		xmpp_stanza_add_child(body,txt);
		xmpp_stanza_add_child(msg, body);
		xmpp_send(fd->conn, msg);
		xmpp_stanza_release(msg);
		free(buf2);
	}

	return NULL;
}

static int message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
		void * const userdata)
{
	char *txt, *buf;
	size_t len;
	ssize_t s;
	descr_t *fd = (descr_t *)userdata;
	char *jid;

	if (!xmpp_stanza_get_child_by_name(stanza, "body"))
		return 1;
	if (!strcmp(xmpp_stanza_get_attribute(stanza, "type"), "error"))
		return 1;

	jid = xmpp_stanza_get_attribute(stanza, "from");
	if (!fd->to) {
		fd->to = malloc(strlen(jid) + 1);
		strcpy(fd->to, jid);
		set_jid_safe(fd->to);
	}
	if (strcmp(fd->to, jid)) /* unknown jid - drop */
		return 1;

	txt = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stanza, "body"));
	buf = b64_decode(txt, &len);
	s = write(fd->wr, buf, len);
	if (s < 0)
		perror("write");

	xmpp_free(fd->ctx, txt);
	free(buf);

	return 1;
}

/* define a handler for connection events */
static void
conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status,
		const int error, xmpp_stream_error_t * const stream_error,
		void * const userdata)
{
	descr_t *fd = (descr_t *)userdata;
	xmpp_ctx_t *ctx = (*fd).ctx;
	xmpp_stanza_t* pres;

	if (status == XMPP_CONN_CONNECT) {
		pr_log("Connected to jabber server\n");
		/* init mutex */
		if (pthread_mutex_init(&mtx, NULL)) {
			pr_log("Can't initialize mutex");
			return;
		}
		xmpp_handler_add(conn,message_handler, NULL, "message", NULL, &fd[0]);
		pthread_create(&thread, NULL, run, &fd[1]);

		pres = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(pres, "presence");
		xmpp_send(conn, pres);
		xmpp_stanza_release(pres);
	} else {
		pr_log("Disconnected from jabber server\n");
		xmpp_stop(ctx);
	}
}

int mod_xmpp(int argc, char **argv, int rd, int wr)
{
	xmpp_ctx_t *ctx;
	xmpp_conn_t *conn;
	xmpp_log_t *log;
	char *jid = NULL;
	char *pass = NULL;
	descr_t fd[2];
	int opt;

	if (!argv) {
		pr_log("xmpp module needs aeguments\n");
		return -1;
	}

	while ((opt = getopt(argc, argv, "u:p:r:")) > 0) {
		switch (opt) {
		case 'u':
			jid = optarg;
			break;
		case 'p':
			pass = optarg;
			break;
		case 'r':
			rmt_jid = optarg;
			break;
		default:
			fprintf(stderr, __FILE__ ": Unrecognized option -%c\n", optopt);
			break;
		}
	}

	if (!jid || !pass) {
		pr_log("JID and password must be set\n");
		return -1;
	}

	/* init library */
	xmpp_initialize();

#ifdef DEBUG
	log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG);
#else
	log = NULL;
#endif

	/* create a context */
	ctx = xmpp_ctx_new(NULL, log);

	/* create a connection */
	conn = xmpp_conn_new(ctx);

	/* setup authentication information */
	xmpp_conn_set_jid(conn, jid);
	xmpp_conn_set_pass(conn, pass);

	fd[0].rd = fd[1].rd = rd;
	fd[0].wr = fd[1].wr = wr;
	fd[0].ctx = fd[1].ctx = ctx;
	fd[0].conn = fd[1].conn = conn;
	/* set `to' only for sender function
	 * receiver function will remember JID from the first packet
	 */
	fd[0].to = NULL;
	fd[1].to = rmt_jid;

	/* initiate connection */
	xmpp_connect_client(conn, NULL, 0, &conn_handler, (void *)fd);

	/* enter the event loop */
	xmpp_run(ctx);

	/* release our connection and context */
	xmpp_conn_release(conn);
	xmpp_ctx_free(ctx);

	/* final shutdown of the library */
	xmpp_shutdown();

	return 0;
}
#endif