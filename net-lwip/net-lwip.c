// SPDX-License-Identifier: GPL-2.0

/* Copyright (C) 2024 Linaro Ltd. */

#include <command.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <lwip/ip4_addr.h>
#include <lwip/err.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include <lwip/etharp.h>
#include <lwip/prot/etharp.h>
#include <net.h>

/* xx:xx:xx:xx:xx:xx\0 */
#define MAC_ADDR_STRLEN 18

#if defined(CONFIG_API) || defined(CONFIG_EFI_LOADER)
void (*push_packet)(void *, int len) = 0;
#endif
int net_restart_wrap;
static uchar net_pkt_buf[(PKTBUFSRX+1) * PKTSIZE_ALIGN + PKTALIGN];
uchar *net_rx_packets[PKTBUFSRX];
uchar *net_rx_packet;
uchar *net_tx_packet;

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
        int err;

	/*
	 * lwIP is alwys configured to use one device, the active one, so
	 * there is no need to use the netif parameter.
	 */

        /* switch dev to active state */
        eth_init_state_only();

        err = eth_send(p->payload, p->len);
        if (err) {
                log_err("eth_send error %d\n", err);
                return ERR_ABRT;
        }
        return ERR_OK;
}

static err_t net_lwip_if_init(struct netif *netif)
{
#if LWIP_IPV4
	netif->output = etharp_output;
#endif
	netif->linkoutput = low_level_output;
	netif->mtu = 1500;
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

	return ERR_OK;
}

static void eth_init_rings(void)
{
        static bool called;
	int i;

        if (called)
		return;
	called = true;

	net_tx_packet = &net_pkt_buf[0] + (PKTALIGN - 1);
	net_tx_packet -= (ulong)net_tx_packet % PKTALIGN;
	for (i = 0; i < PKTBUFSRX; i++)
		net_rx_packets[i] = net_tx_packet + (i + 1) * PKTSIZE_ALIGN;
}

struct netif *net_lwip_get_netif(void)
{
	struct netif *netif, *found = NULL;

	NETIF_FOREACH(netif) {
		if (!found)
			found = netif;
		else
			printf("Error: more than one netif in lwIP\n");
	}
	return found;
}

static int get_udev_ipv4_info(struct udevice *dev, ip4_addr_t *ip,
			      ip4_addr_t *mask, ip4_addr_t *gw)
{
	char ipstr[9] = { 'i', 'p', 'a' , 'd', 'd', 'r', };
	char maskstr[10] = { 'n', 'e', 't', 'm', 'a', 's', 'k', };
	char gwstr[12] = { 'g', 'a', 't', 'e', 'w', 'a', 'y', 'i', 'p', };
	char *env;
	int ret;

	if (dev_seq(dev) > 0) {
		ret = snprintf(ipstr, sizeof(ipstr), "ipaddr%d", dev_seq(dev));
		if (ret < 0 || ret >= sizeof(ipstr))
			return -1;
		snprintf(maskstr, sizeof(maskstr), "netmask%d", dev_seq(dev));
		if (ret < 0 || ret >= sizeof(maskstr))
			return -1;
		snprintf(gwstr, sizeof(gwstr), "gw%d", dev_seq(dev));
		if (ret < 0 || ret >= sizeof(gwstr))
			return -1;
	}

	ip4_addr_set_zero(ip);
	ip4_addr_set_zero(mask);
	ip4_addr_set_zero(gw);

	env = env_get(ipstr);
	if (env)
		ipaddr_aton(env, ip);

	env = env_get(maskstr);
	if (env)
		ipaddr_aton(env, mask);

	env = env_get(gwstr);
	if (env)
		ipaddr_aton(env, gw);

	return 0;
}

static struct netif *new_netif(bool with_ip)
{
	unsigned char enetaddr[ARP_HLEN];
	char hwstr[MAC_ADDR_STRLEN];
	ip4_addr_t ip, mask, gw;
	struct udevice *dev;
	struct netif *netif;
	int ret;
	static bool first_call = true;

	eth_init_rings();

	if (first_call) {
		if (eth_init()) {
			printf("eth_init() error\n");
			return NULL;
		}
		first_call = false;
	}

	netif_remove(net_lwip_get_netif());

	eth_set_current();

	dev = eth_get_dev();
	if (!dev)
		return NULL;

	ip4_addr_set_zero(&ip);
	ip4_addr_set_zero(&mask);
	ip4_addr_set_zero(&gw);

	if (with_ip)
		if (get_udev_ipv4_info(dev, &ip, &mask, &gw) < 0)
			return NULL;

	eth_env_get_enetaddr_by_index("eth", dev_seq(dev), enetaddr);
	ret = snprintf(hwstr, MAC_ADDR_STRLEN, "%pM",  enetaddr);
	if (ret < 0 || ret >= MAC_ADDR_STRLEN)
		return NULL;

	netif = calloc(1, sizeof(struct netif));
	if (!netif)
		return NULL;

	netif->name[0] = 'e';
	netif->name[1] = 't';

	string_to_enetaddr(hwstr, netif->hwaddr);
	netif->hwaddr_len = ETHARP_HWADDR_LEN;

	if (!netif_add(netif, &ip, &mask, &gw, netif, net_lwip_if_init,
		       netif_input)) {
		printf("error: netif_add() failed\n");
		free(netif);
		return NULL;
	}

	netif_set_up(netif);
	netif_set_link_up(netif);
	/* Routing: use this interface to reach the default gateway */
	netif_set_default(netif);

	return netif;
}

/* Configure lwIP to use the currently active network device */
struct netif *net_lwip_new_netif()
{
	return new_netif(true);
}

struct netif *net_lwip_new_netif_noip()
{

	return new_netif(false);
}

void net_lwip_remove_netif(struct netif *netif)
{
	netif_remove(netif);
	free(netif);
}

int net_init(void)
{
	net_lwip_new_netif();

	return 0;
}

static struct pbuf *alloc_pbuf_and_copy(uchar *data, int len)
{
        struct pbuf *p, *q;

        /* We allocate a pbuf chain of pbufs from the pool. */
        p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
        if (!p) {
                LINK_STATS_INC(link.memerr);
                LINK_STATS_INC(link.drop);
                return NULL;
        }

        for (q = p; q != NULL; q = q->next) {
                memcpy(q->payload, data, q->len);
                data += q->len;
        }

        LINK_STATS_INC(link.recv);

        return p;
}

void net_process_received_packet(uchar *in_packet, int len)
{
	struct netif *netif;
	struct pbuf *pbuf;

	if (len < ETHER_HDR_SIZE)
		return;

#if defined(CONFIG_API) || defined(CONFIG_EFI_LOADER)
	if (push_packet) {
		(*push_packet)(in_packet, len);
		return;
	}
#endif

	netif = net_lwip_get_netif();
	if (!netif)
		return;

	pbuf = alloc_pbuf_and_copy(in_packet, len);
	if (!pbuf)
		return;

	netif->input(pbuf, netif);
}

u32_t sys_now(void)
{
	return get_timer(0);
}
