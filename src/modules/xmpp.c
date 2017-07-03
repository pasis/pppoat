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
#include <stdbool.h>
#include <string.h>
#include <strophe.h>
#include <unistd.h>

#include "trace.h"
#include "modules/xmpp.h"
#include "base64.h"
#include "conf.h"
#include "log.h"
#include "memory.h"
#include "pppoat.h"
#include "util.h"

#define PPPOAT_XMPP_TIMEOUT  1
#define PPPOAT_XMPP_BUF_SIZE 4096

struct pppoat_xmpp_ctx {
	pppoat_node_type_t  xc_type;
	xmpp_log_t          xc_log;
	xmpp_ctx_t         *xc_ctx;
	xmpp_conn_t        *xc_conn;
	const char         *xc_jid;
	const char         *xc_passwd;
	char               *xc_to;
	bool                xc_connected;
	bool                xc_stop;
	bool                xc_to_trusted;
	int                 xc_rd;
	int                 xc_wr;
};

static void pppoat_xmpp_log(void                  *userdata,
			    const xmpp_log_level_t level,
			    const char * const     area,
			    const char * const     msg)
{
	pppoat_log_level_t l = PPPOAT_DEBUG;

	switch (level) {
	case XMPP_LEVEL_DEBUG:
		l = PPPOAT_DEBUG;
		break;
	case XMPP_LEVEL_INFO:
		l = PPPOAT_INFO;
		break;
	case XMPP_LEVEL_WARN:
		l = PPPOAT_INFO;
		break;
	case XMPP_LEVEL_ERROR:
		l = PPPOAT_ERROR;
		break;
	}
	pppoat_log(l, area, msg);
}

static void pppoat_xmpp_parse_args(struct pppoat_conf      *conf,
				   struct pppoat_xmpp_ctx  *ctx)
{
	const char *opt;

	opt = pppoat_conf_get(conf, "server");
	ctx->xc_type = opt != NULL && pppoat_conf_obj_is_true(opt) ?
		       PPPOAT_NODE_MASTER : PPPOAT_NODE_SLAVE;

	ctx->xc_jid    = pppoat_conf_get(conf, "xmpp.jid");
	ctx->xc_passwd = pppoat_conf_get(conf, "xmpp.passwd");
	opt            = pppoat_conf_get(conf, "xmpp.to");
	ctx->xc_to     = opt == NULL ? NULL : pppoat_strdup(opt);
	PPPOAT_ASSERT(ctx->xc_jid != NULL);
	PPPOAT_ASSERT(ctx->xc_passwd != NULL);
	PPPOAT_ASSERT(ctx->xc_type == PPPOAT_NODE_MASTER || ctx->xc_to != NULL);
}

static int pppoat_xmpp_send_buf(struct pppoat_xmpp_ctx *ctx,
				const unsigned char    *buf,
				size_t                  len)
{
	xmpp_stanza_t *message;
	char          *b64;
	int            rc;

	if (ctx->xc_to == NULL)
		return 0; /* XXX */

	b64 = xmpp_base64_encode(ctx->xc_ctx, buf, len);
	message = xmpp_message_new(ctx->xc_ctx, "chat", ctx->xc_to, NULL);
	rc = xmpp_message_set_body(message, b64);
	PPPOAT_ASSERT(rc == XMPP_EOK);
	xmpp_send(ctx->xc_conn, message);
	xmpp_stanza_release(message);
	xmpp_free(ctx->xc_ctx, b64);

	return 0;
}

static int module_xmpp_init(struct pppoat_conf *conf, void **userdata)
{
	struct pppoat_xmpp_ctx *ctx;
	char                   *resource;
	int                     rc;

	ctx = pppoat_alloc(sizeof(*ctx));
	rc  = ctx == NULL ? P_ERR(-ENOMEM) : 0;
	if (rc == 0) {
		pppoat_xmpp_parse_args(conf, ctx);
		xmpp_initialize();
		ctx->xc_log = (xmpp_log_t){
			.handler = &pppoat_xmpp_log,
		};
		ctx->xc_ctx = xmpp_ctx_new(NULL, &ctx->xc_log);
		PPPOAT_ASSERT(ctx->xc_ctx != NULL);
		ctx->xc_conn = xmpp_conn_new(ctx->xc_ctx);
		PPPOAT_ASSERT(ctx->xc_conn != NULL);
		ctx->xc_connected  = false;
		ctx->xc_stop       = false;
		ctx->xc_to_trusted = false;

		resource = ctx->xc_to == NULL ? NULL :
			   xmpp_jid_resource(ctx->xc_ctx, ctx->xc_to);
		if (resource != NULL) {
			ctx->xc_to_trusted = true;
			xmpp_free(ctx->xc_ctx, resource);
		}
	}
	if (rc == 0) {
		*userdata = ctx;
	}
	return rc;
}

static void module_xmpp_fini(void *userdata)
{
	struct pppoat_xmpp_ctx *ctx = userdata;

	pppoat_free(ctx->xc_to);
	xmpp_conn_release(ctx->xc_conn);
	xmpp_ctx_free(ctx->xc_ctx);
	pppoat_free(ctx);
	xmpp_shutdown();
}

static int message_handler(xmpp_conn_t * const   conn,
			   xmpp_stanza_t * const stanza,
			   void * const          userdata)
{
	struct pppoat_xmpp_ctx *ctx = userdata;
	unsigned char          *raw;
	const char             *from;
	char                   *b64;
	char                   *bare;
	ssize_t                 written;
	size_t                  raw_len;

	from = xmpp_stanza_get_from(stanza);
	PPPOAT_ASSERT(from != NULL);
	if (!ctx->xc_to_trusted && ctx->xc_to != NULL) {
		bare = xmpp_jid_bare(ctx->xc_ctx, from);
		if (strcmp(ctx->xc_to, bare) == 0) {
			pppoat_free(ctx->xc_to);
			ctx->xc_to = NULL;
			/* Next if statement will be true */
		}
		xmpp_free(ctx->xc_ctx, bare);
	}
	if (!ctx->xc_to_trusted && ctx->xc_to == NULL) {
		ctx->xc_to = pppoat_strdup(from);
		PPPOAT_ASSERT(ctx->xc_to != NULL);
		ctx->xc_to_trusted = true;
	}

	/* XXX FOR TESTS: quit on "quit" message from any source */
	b64 = xmpp_message_get_body(stanza);
	if (b64 && strcmp(b64, "quit") == 0) {
		xmpp_free(ctx->xc_ctx, b64);
		xmpp_disconnect(conn);
		return 0;
	}
	xmpp_free(ctx->xc_ctx, b64);

	if (strcmp(from, ctx->xc_to) != 0) {
		pppoat_debug("xmpp", "Ignore unkown source");
		return 1;
	}

	b64 = xmpp_message_get_body(stanza);
	if (b64 == NULL) {
		pppoat_debug("xmpp", "Skipping incomplete message");
		return 1;
	}
	xmpp_base64_decode_bin(ctx->xc_ctx, b64, strlen(b64), &raw, &raw_len);
	PPPOAT_ASSERT(raw != NULL);

	written = write(ctx->xc_wr, raw, raw_len);
	PPPOAT_ASSERT_INFO(written == raw_len, "written=%zi", written);

	xmpp_free(ctx->xc_ctx, raw);
	xmpp_free(ctx->xc_ctx, b64);

	return 1;
}

static void conn_handler(xmpp_conn_t * const         conn,
			 const xmpp_conn_event_t     status,
			 const int                   error,
			 xmpp_stream_error_t * const stream_error,
			 void * const                userdata)
{
	struct pppoat_xmpp_ctx *ctx = userdata;
	xmpp_stanza_t          *presence;

	if (status == XMPP_CONN_CONNECT) {
		xmpp_handler_add(conn, message_handler, NULL, "message",
				 NULL, ctx);
		presence = xmpp_presence_new(ctx->xc_ctx);
		PPPOAT_ASSERT(presence != NULL);
		xmpp_send(conn, presence);
		xmpp_stanza_release(presence);
		ctx->xc_connected = true;
	}
	if (status == XMPP_CONN_DISCONNECT) {
		pppoat_debug("xmpp", "Disconnected with error=%d,"
				     "stream_error=%d", error, stream_error);
		ctx->xc_stop = true;
	}
}

static int module_xmpp_run(int rd, int wr, int ctrl, void *userdata)
{
	struct pppoat_xmpp_ctx *ctx  = userdata;
	xmpp_conn_t            *conn = ctx->xc_conn;
	unsigned char          *buf;
	ssize_t                 len;
	int                     rc;

	ctx->xc_rd = rd;
	ctx->xc_wr = wr;

	rc = pppoat_util_fd_nonblock_set(rd, true);
	PPPOAT_ASSERT(rc == 0);
	buf = pppoat_alloc(PPPOAT_XMPP_BUF_SIZE);
	PPPOAT_ASSERT(buf != NULL);

	xmpp_conn_set_jid(conn, ctx->xc_jid);
	xmpp_conn_set_pass(conn, ctx->xc_passwd);
	xmpp_connect_client(conn, NULL, 0, &conn_handler, userdata);

	while (!ctx->xc_stop) {
		xmpp_run_once(ctx->xc_ctx, PPPOAT_XMPP_TIMEOUT);

		if (!ctx->xc_connected)
			continue;

		len = read(rd, buf, PPPOAT_XMPP_BUF_SIZE);
		if (len > 0) {
			rc = pppoat_xmpp_send_buf(ctx, buf, (size_t)len);
			PPPOAT_ASSERT(rc == 0);
		}
	}
	return 0;
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
