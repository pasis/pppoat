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

	rc = pppoat_base64_enc_new(buf, len, &b64);
	PPPOAT_ASSERT(rc == 0);
	message = xmpp_message_new(ctx->xc_ctx, "chat", ctx->xc_to, NULL);
	PPPOAT_ASSERT(message != NULL);
	rc = xmpp_message_set_body(message, b64);
	PPPOAT_ASSERT(rc == XMPP_EOK);
	xmpp_send(ctx->xc_conn, message);
	xmpp_stanza_release(message);
	pppoat_free(b64);

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
	int                     rc;

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
	rc = pppoat_base64_dec_new(b64, strlen(b64), &raw, &raw_len);
	PPPOAT_ASSERT(rc == 0);

	written = write(ctx->xc_wr, raw, raw_len);
	PPPOAT_ASSERT_INFO(written == raw_len, "written=%zi", written);

	pppoat_free(raw);
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
		pppoat_debug("xmpp", "Disconnected with error=%d, "
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
