/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __NET_LWIP_H__
#define __NET_LWIP_H__

#include <lwip/ip4.h>
#include <lwip/netif.h>

struct netif *net_lwip_new_netif(void);
struct netif *net_lwip_new_netif_noip(void);
void net_lwip_remove_netif(struct netif *netif);
struct netif *net_lwip_get_netif(void);

int do_dhcp(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]);
int do_dns(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]);
int do_ping(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]);

#endif /* __NET_LWIP_H__ */
