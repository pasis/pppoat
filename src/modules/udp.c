/* modules/udp.c
 * PPP over Any Transport -- UDP transport
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
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "trace.h"
#include "log.h"
#include "memory.h"
#include "pppoat.h"
#include "util.h"

#define UDP_PORT_MASTER 0xc001
#define UDP_PORT_SLAVE  0xc001
#define UDP_HOST_MASTER "192.168.4.1"
#define UDP_HOST_SLAVE  "192.168.4.10"

#define UDP_TIMEOUT 1000000

struct pppoat_udp_ctx {
	pppoat_node_type_t  uc_type;
	struct addrinfo    *uc_ainfo;
	int                 uc_sock;
};

static void udp_parse_args(int argc, char **argv, pppoat_node_type_t *type)
{
	*type = argc > 1 && strcmp(argv[1], "-s") == 0 ? PPPOAT_NODE_MASTER :
							 PPPOAT_NODE_SLAVE;
}

static int udp_ainfo_get(struct addrinfo **ainfo,
			 const char       *host,
			 unsigned short    port)
{
	struct addrinfo hints;
	char            service[6];
	int             rc;

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags    = host == NULL ? AI_PASSIVE : 0;
#ifdef AI_ADDRCONFIG
	hints.ai_flags   |= AI_ADDRCONFIG;
#endif /* AI_ADDRCONFIG */
	hints.ai_family   = AF_UNSPEC;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_socktype = SOCK_DGRAM;

	snprintf(service, sizeof(service), "%u", port);
	rc = getaddrinfo(host, service, &hints, ainfo);
	if (rc != 0) {
		pppoat_error("udp", "getaddrinfo rc=%d: %s",
			     rc, gai_strerror(rc));
		rc = P_ERR(-ENOPROTOOPT);
		*ainfo = NULL;
	}
	return rc;
}

static void udp_ainfo_put(struct addrinfo *ainfo)
{
	freeaddrinfo(ainfo);
}

static int udp_sock_new(unsigned short port, int *sock)
{
	struct addrinfo *ainfo;
	int              rc;

	rc = udp_ainfo_get(&ainfo, NULL, port);
	if (rc == 0) {
		*sock = socket(ainfo->ai_family, ainfo->ai_socktype,
			       ainfo->ai_protocol);
		rc = *sock < 0 ? P_ERR(-errno) : 0;
	}
	if (rc == 0) {
		rc = bind(*sock, ainfo->ai_addr, ainfo->ai_addrlen);
		rc = rc != 0 ? P_ERR(-errno) : 0;
		if (rc != 0)
			(void)close(*sock);
	}
	if (ainfo != NULL) {
		udp_ainfo_put(ainfo);
	}
	return rc;
}

static int module_udp_init(int argc, char **argv, void **userdata)
{
	struct pppoat_udp_ctx *ctx;
	pppoat_node_type_t     type;
	unsigned short         sport;
	unsigned short         dport;
	const char            *dhost;
	int                    rc;

	/* XXX use hardcoded config for now */
	udp_parse_args(argc, argv, &type);
	if (type == PPPOAT_NODE_MASTER) {
		sport = UDP_PORT_MASTER;
		dport = UDP_PORT_SLAVE;
		dhost = UDP_HOST_SLAVE;
	} else {
		sport = UDP_PORT_SLAVE;
		dport = UDP_PORT_MASTER;
		dhost = UDP_HOST_MASTER;
	}

	ctx = pppoat_alloc(sizeof(*ctx));
	rc  = ctx == NULL ? P_ERR(-ENOMEM) : 0;
	if (rc == 0) {
		ctx->uc_type = type;
		rc = udp_ainfo_get(&ctx->uc_ainfo, dhost, dport);
		rc = rc ?: udp_sock_new(sport, &ctx->uc_sock);
		if (rc != 0) {
			if (ctx->uc_ainfo != NULL)
				udp_ainfo_put(ctx->uc_ainfo);
			pppoat_free(ctx);
		}
	}
	if (rc == 0) {
		*userdata = ctx;
	}
	return rc;
}

static void module_udp_fini(void *userdata)
{
	struct pppoat_udp_ctx *ctx = userdata;

	(void)close(ctx->uc_sock);
	udp_ainfo_put(ctx->uc_ainfo);
	pppoat_free(ctx);
}

static bool udp_error_is_recoverable(int error)
{
	return (error == -EAGAIN ||
		error == -EINTR  ||
		error == -EWOULDBLOCK);
}

static int module_udp_run(int rd, int wr, int ctrl, void *userdata)
{
	struct pppoat_udp_ctx *ctx   = userdata;
	struct addrinfo       *ainfo = ctx->uc_ainfo;
	struct timeval         tv;
	unsigned long          timeout;
	unsigned char          buf[4096];
	ssize_t                len;
	ssize_t                len2;
	fd_set                 rfds;
	fd_set                 wfds;
	int                    sock = ctx->uc_sock;
	int                    max;
	int                    rc;

	timeout = UDP_TIMEOUT; /* XXX */
	while (true) {
		tv.tv_sec  = timeout / 1000000;
		tv.tv_usec = timeout % 1000000;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(rd, &rfds);
		FD_SET(sock, &rfds);
		max = pppoat_max(rd, sock);

		rc = select(max + 1, &rfds, &wfds, NULL, &tv);
		if (rc < 0 && !udp_error_is_recoverable(-errno)) {
			rc = P_ERR(-errno);
			break;
		}
		if (FD_ISSET(rd, &rfds)) {
			len = read(rd, buf, sizeof(buf));
			PPPOAT_ASSERT(len != 0); /* XXX */
			if (len < 0 && !udp_error_is_recoverable(-errno)) {
				rc = P_ERR(-errno);
				break;
			}
			if (len > 0) {
				len2 = sendto(sock, buf, len, 0,
					     ainfo->ai_addr, ainfo->ai_addrlen);
				PPPOAT_ASSERT(len2 == len); /* XXX */
			}
		}
		if (FD_ISSET(sock, &rfds)) {
			len = recv(sock, buf, sizeof(buf), 0); /* XXX use recvfrom */
			PPPOAT_ASSERT(len != 0); /* XXX */
			if (len < 0 && !udp_error_is_recoverable(-errno)) {
				rc = P_ERR(-errno);
				break;
			}
			if (len > 0) {
				len2 = write(wr, buf, len);
				PPPOAT_ASSERT(len2 == len); /* XXX */
			}
		}
	}
	return rc;
}

const struct pppoat_module pppoat_module_udp = {
	.m_name  = "udp",
	.m_descr = "PPP over UDP",
	.m_init  = &module_udp_init,
	.m_fini  = module_udp_fini,
	.m_run   = module_udp_run,
};
