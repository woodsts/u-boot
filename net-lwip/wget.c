// SPDX-License-Identifier: GPL-2.0+
/* Copyright (C) 2024 Linaro Ltd. */

#include <command.h>
#include <console.h>
#include <display_options.h>
#include <image.h>
#include <lwip/apps/http_client.h>
#include <lwip/timeouts.h>
#include <net.h>
#include <time.h>

#define SERVER_NAME_SIZE 200
#define HTTP_PORT_DEFAULT 80
#define PROGRESS_PRINT_STEP_BYTES (100 * 1024)

enum done_state {
        NOT_DONE = 0,
        SUCCESS = 1,
        FAILURE = 2
};

struct wget_ctx {
	ulong daddr;
	ulong saved_daddr;
	ulong size;
	ulong prevsize;
	ulong start_time;
	enum done_state done;
};

static int parse_url(char *url, char *host, u16 *port, char **path)
{
	char *p, *pp;
	long lport;

	p = strstr(url, "http://");
	if (!p)
		return -EINVAL;

	p += strlen("http://");

	/* Parse hostname */
	pp = strchr(p, ':');
	if (!pp)
		pp = strchr(p, '/');
	if (!pp)
		return -EINVAL;

	if (p + SERVER_NAME_SIZE <= pp)
		return -EINVAL;

	memcpy(host, p, pp - p);
	host[pp - p + 1] = '\0';

	if (*pp == ':') {
		/* Parse port number */
		p = pp + 1;
		lport = simple_strtol(p, &pp, 10);
		if (pp && *pp != '/')
			return -EINVAL;
		if (lport > 65535)
			return -EINVAL;
		*port = (u16)lport;
	} else {
		*port = HTTP_PORT_DEFAULT;
	}
	if (*pp != '/')
		return -EINVAL;
	*path = pp;

	return 0;
}

static err_t httpc_recv_cb(void *arg, struct altcp_pcb *pcb, struct pbuf *pbuf,
			   err_t err)
{
	struct wget_ctx *ctx = arg;
	struct pbuf *buf;

	if (!pbuf)
		return ERR_BUF;

	for (buf = pbuf; buf; buf = buf->next) {
		memcpy((void *)ctx->daddr, buf->payload, buf->len);
		ctx->daddr += buf->len;
		ctx->size += buf->len;
		if (ctx->size - ctx->prevsize > PROGRESS_PRINT_STEP_BYTES) {
			printf("#");
			ctx->prevsize = ctx->size;
		}
	}

	altcp_recved(pcb, pbuf->tot_len);
	pbuf_free(pbuf);
	return ERR_OK;
}

static void httpc_result_cb(void *arg, httpc_result_t httpc_result,
			    u32_t rx_content_len, u32_t srv_res, err_t err)
{
	struct wget_ctx *ctx = arg;
	ulong elapsed;

	if (httpc_result != HTTPC_RESULT_OK) {
		log_err("\nHTTP client error %d\n", httpc_result);
		ctx->done = FAILURE;
		return;
	}

	elapsed = get_timer(ctx->start_time);
	if (rx_content_len > PROGRESS_PRINT_STEP_BYTES)
		printf("\n");
        printf("%u bytes transferred in %lu ms (", rx_content_len,
	       get_timer(ctx->start_time));
        print_size(rx_content_len / elapsed * 1000, "/s)\n");

	if (env_set_hex("filesize", rx_content_len) ||
	    env_set_hex("fileaddr", ctx->saved_daddr)) {
		log_err("Could not set filesize or fileaddr\n");
		ctx->done = FAILURE;
		return;
	}

	ctx->done = SUCCESS;
}

int wget_with_dns(ulong dst_addr, char *uri)
{
	char server_name[SERVER_NAME_SIZE];
	httpc_connection_t conn;
	httpc_state_t *state;
	struct netif *netif;
	struct wget_ctx ctx;
	char *path;
	u16 port;

	ctx.daddr = dst_addr;
	ctx.saved_daddr = dst_addr;
	ctx.done = NOT_DONE;
	ctx.size = 0;
	ctx.prevsize = 0;

	if (parse_url(uri, server_name, &port, &path))
		return CMD_RET_USAGE;

	netif = net_lwip_new_netif();
	if (!netif)
		return -1;

	memset(&conn, 0, sizeof(conn));
	conn.result_fn = httpc_result_cb;
	ctx.start_time = get_timer(0);
	if (httpc_get_file_dns(server_name, port, path, &conn, httpc_recv_cb,
			       &ctx, &state)) {
		net_lwip_remove_netif(netif);
		return CMD_RET_FAILURE;
	}

	while (!ctx.done) {
		eth_rx();
		sys_check_timeouts();
		if (ctrlc())
			break;
	}

	net_lwip_remove_netif(netif);

	if (ctx.done == SUCCESS)
		return 0;

	return -1;
}

int do_wget(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	char *end;
	char *url;
	ulong dst_addr;

	if (argc < 2 || argc > 3)
		return CMD_RET_USAGE;

	dst_addr = hextoul(argv[1], &end);
        if (end == (argv[1] + strlen(argv[1]))) {
		if (argc < 3)
			return CMD_RET_USAGE;
		url = argv[2];
	} else {
		dst_addr = image_load_addr;
		url = argv[1];
	}

	if (wget_with_dns(dst_addr, url))
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

/**
 * wget_validate_uri() - validate the uri for wget
 *
 * @uri:	uri string
 *
 * This function follows the current U-Boot wget implementation.
 * scheme: only "http:" is supported
 * authority:
 *   - user information: not supported
 *   - host: supported
 *   - port: not supported(always use the default port)
 *
 * Uri is expected to be correctly percent encoded.
 * This is the minimum check, control codes(0x1-0x19, 0x7F, except '\0')
 * and space character(0x20) are not allowed.
 *
 * TODO: stricter uri conformance check
 *
 * Return:	true on success, false on failure
 */
bool wget_validate_uri(char *uri)
{
	char c;
	bool ret = true;
	char *str_copy, *s, *authority;

	for (c = 0x1; c < 0x21; c++) {
		if (strchr(uri, c)) {
			log_err("invalid character is used\n");
			return false;
		}
	}
	if (strchr(uri, 0x7f)) {
		log_err("invalid character is used\n");
		return false;
	}

	if (strncmp(uri, "http://", 7)) {
		log_err("only http:// is supported\n");
		return false;
	}
	str_copy = strdup(uri);
	if (!str_copy)
		return false;

	s = str_copy + strlen("http://");
	authority = strsep(&s, "/");
	if (!s) {
		log_err("invalid uri, no file path\n");
		ret = false;
		goto out;
	}
	s = strchr(authority, '@');
	if (s) {
		log_err("user information is not supported\n");
		ret = false;
		goto out;
	}
	s = strchr(authority, ':');
	if (s) {
		log_err("user defined port is not supported\n");
		ret = false;
		goto out;
	}

out:
	free(str_copy);

	return ret;
}
