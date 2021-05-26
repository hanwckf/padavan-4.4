/*   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   Copyright (C) 2014-2016 Sean Wang <sean.wang@mediatek.com>
 *   Copyright (C) 2016-2017 John Crispin <blogic@openwrt.org>
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/iopoll.h>

#include "hnat.h"
#include "nf_hnat_mtk.h"
#include "../mtk_eth_soc.h"

int dbg_entry_state = BIND;
typedef int (*debugfs_write_func)(int par1);
int debug_level;
int dbg_cpu_reason;
int hook_toggle;
int mape_toggle;
#if defined(CONFIG_NET_MEDIATEK_HW_QOS)
int hqos_toggle;
#endif
unsigned int dbg_cpu_reason_cnt[MAX_CRSN_NUM];

static const char * const entry_state[] = { "INVALID", "UNBIND", "BIND", "FIN" };

static const char * const packet_type[] = {
	"IPV4_HNAPT",    "IPV4_HNAT",     "IPV6_1T_ROUTE", "IPV4_DSLITE",
	"IPV6_3T_ROUTE", "IPV6_5T_ROUTE", "REV",	   "IPV6_6RD",
};

static uint8_t *show_cpu_reason(struct sk_buff *skb)
{
	static u8 buf[32];

	switch (skb_hnat_reason(skb)) {
	case TTL_0:
		return "IPv4(IPv6) TTL(hop limit)\n";
	case HAS_OPTION_HEADER:
		return "Ipv4(IPv6) has option(extension) header\n";
	case NO_FLOW_IS_ASSIGNED:
		return "No flow is assigned\n";
	case IPV4_WITH_FRAGMENT:
		return "IPv4 HNAT doesn't support IPv4 /w fragment\n";
	case IPV4_HNAPT_DSLITE_WITH_FRAGMENT:
		return "IPv4 HNAPT/DS-Lite doesn't support IPv4 /w fragment\n";
	case IPV4_HNAPT_DSLITE_WITHOUT_TCP_UDP:
		return "IPv4 HNAPT/DS-Lite can't find TCP/UDP sport/dport\n";
	case IPV6_5T_6RD_WITHOUT_TCP_UDP:
		return "IPv6 5T-route/6RD can't find TCP/UDP sport/dport\n";
	case TCP_FIN_SYN_RST:
		return "Ingress packet is TCP fin/syn/rst\n";
	case UN_HIT:
		return "FOE Un-hit\n";
	case HIT_UNBIND:
		return "FOE Hit unbind\n";
	case HIT_UNBIND_RATE_REACH:
		return "FOE Hit unbind & rate reach\n";
	case HIT_BIND_TCP_FIN:
		return "Hit bind PPE TCP FIN entry\n";
	case HIT_BIND_TTL_1:
		return "Hit bind PPE entry and TTL(hop limit) = 1 and TTL(hot limit) - 1\n";
	case HIT_BIND_WITH_VLAN_VIOLATION:
		return "Hit bind and VLAN replacement violation\n";
	case HIT_BIND_KEEPALIVE_UC_OLD_HDR:
		return "Hit bind and keep alive with unicast old-header packet\n";
	case HIT_BIND_KEEPALIVE_MC_NEW_HDR:
		return "Hit bind and keep alive with multicast new-header packet\n";
	case HIT_BIND_KEEPALIVE_DUP_OLD_HDR:
		return "Hit bind and keep alive with duplicate old-header packet\n";
	case HIT_BIND_FORCE_TO_CPU:
		return "FOE Hit bind & force to CPU\n";
	case HIT_BIND_EXCEED_MTU:
		return "Hit bind and exceed MTU\n";
	case HIT_BIND_MULTICAST_TO_CPU:
		return "Hit bind multicast packet to CPU\n";
	case HIT_BIND_MULTICAST_TO_GMAC_CPU:
		return "Hit bind multicast packet to GMAC & CPU\n";
	case HIT_PRE_BIND:
		return "Pre bind\n";
	}

	sprintf(buf, "CPU Reason Error - %X\n", skb_hnat_entry(skb));
	return buf;
}

uint32_t foe_dump_pkt(struct sk_buff *skb)
{
	struct foe_entry *entry;

	entry = &hnat_priv->foe_table_cpu[skb_hnat_entry(skb)];
	pr_info("\nRx===<FOE_Entry=%d>=====\n", skb_hnat_entry(skb));
	pr_info("RcvIF=%s\n", skb->dev->name);
	pr_info("FOE_Entry=%d\n", skb_hnat_entry(skb));
	pr_info("CPU Reason=%s", show_cpu_reason(skb));
	pr_info("ALG=%d\n", skb_hnat_alg(skb));
	pr_info("SP=%d\n", skb_hnat_sport(skb));

	/* some special alert occurred, so entry_num is useless (just skip it) */
	if (skb_hnat_entry(skb) == 0x3fff)
		return 1;

	/* PPE: IPv4 packet=IPV4_HNAT IPv6 packet=IPV6_ROUTE */
	if (IS_IPV4_GRP(entry)) {
		__be32 saddr = htonl(entry->ipv4_hnapt.sip);
		__be32 daddr = htonl(entry->ipv4_hnapt.dip);

		pr_info("Information Block 1=%x\n",
			entry->ipv4_hnapt.info_blk1);
		pr_info("SIP=%pI4\n", &saddr);
		pr_info("DIP=%pI4\n", &daddr);
		pr_info("SPORT=%d\n", entry->ipv4_hnapt.sport);
		pr_info("DPORT=%d\n", entry->ipv4_hnapt.dport);
		pr_info("Information Block 2=%x\n",
			entry->ipv4_hnapt.info_blk2);
		pr_info("State = %s, proto = %s\n", entry->bfib1.state == 0 ?
			"Invalid" : entry->bfib1.state == 1 ?
			"Unbind" : entry->bfib1.state == 2 ?
			"BIND" : entry->bfib1.state == 3 ?
			"FIN" : "Unknown",
			entry->ipv4_hnapt.bfib1.udp == 0 ?
			"TCP" : entry->ipv4_hnapt.bfib1.udp == 1 ?
			"UDP" : "Unknown");
	} else if (IS_IPV6_GRP(entry)) {
		pr_info("Information Block 1=%x\n",
			entry->ipv6_5t_route.info_blk1);
		pr_info("IPv6_SIP=%08X:%08X:%08X:%08X\n",
			entry->ipv6_5t_route.ipv6_sip0,
			entry->ipv6_5t_route.ipv6_sip1,
			entry->ipv6_5t_route.ipv6_sip2,
			entry->ipv6_5t_route.ipv6_sip3);
		pr_info("IPv6_DIP=%08X:%08X:%08X:%08X\n",
			entry->ipv6_5t_route.ipv6_dip0,
			entry->ipv6_5t_route.ipv6_dip1,
			entry->ipv6_5t_route.ipv6_dip2,
			entry->ipv6_5t_route.ipv6_dip3);
		pr_info("SPORT=%d\n", entry->ipv6_5t_route.sport);
		pr_info("DPORT=%d\n", entry->ipv6_5t_route.dport);
		pr_info("Information Block 2=%x\n",
			entry->ipv6_5t_route.info_blk2);
		pr_info("State = %s, proto = %s\n", entry->bfib1.state == 0 ?
			"Invalid" : entry->bfib1.state == 1 ?
			"Unbind" : entry->bfib1.state == 2 ?
			"BIND" : entry->bfib1.state == 3 ?
			"FIN" : "Unknown",
			entry->ipv6_5t_route.bfib1.udp == 0 ?
			"TCP" : entry->ipv6_5t_route.bfib1.udp == 1 ?
			"UDP" :	"Unknown");
	} else {
		pr_info("unknown Pkt_type=%d\n", entry->bfib1.pkt_type);
	}

	pr_info("==================================\n");
	return 1;
}

uint32_t hnat_cpu_reason_cnt(struct sk_buff *skb)
{
	switch (skb_hnat_reason(skb)) {
	case TTL_0:
		dbg_cpu_reason_cnt[0]++;
		return 0;
	case HAS_OPTION_HEADER:
		dbg_cpu_reason_cnt[1]++;
		return 0;
	case NO_FLOW_IS_ASSIGNED:
		dbg_cpu_reason_cnt[2]++;
		return 0;
	case IPV4_WITH_FRAGMENT:
		dbg_cpu_reason_cnt[3]++;
		return 0;
	case IPV4_HNAPT_DSLITE_WITH_FRAGMENT:
		dbg_cpu_reason_cnt[4]++;
		return 0;
	case IPV4_HNAPT_DSLITE_WITHOUT_TCP_UDP:
		dbg_cpu_reason_cnt[5]++;
		return 0;
	case IPV6_5T_6RD_WITHOUT_TCP_UDP:
		dbg_cpu_reason_cnt[6]++;
		return 0;
	case TCP_FIN_SYN_RST:
		dbg_cpu_reason_cnt[7]++;
		return 0;
	case UN_HIT:
		dbg_cpu_reason_cnt[8]++;
		return 0;
	case HIT_UNBIND:
		dbg_cpu_reason_cnt[9]++;
		return 0;
	case HIT_UNBIND_RATE_REACH:
		dbg_cpu_reason_cnt[10]++;
		return 0;
	case HIT_BIND_TCP_FIN:
		dbg_cpu_reason_cnt[11]++;
		return 0;
	case HIT_BIND_TTL_1:
		dbg_cpu_reason_cnt[12]++;
		return 0;
	case HIT_BIND_WITH_VLAN_VIOLATION:
		dbg_cpu_reason_cnt[13]++;
		return 0;
	case HIT_BIND_KEEPALIVE_UC_OLD_HDR:
		dbg_cpu_reason_cnt[14]++;
		return 0;
	case HIT_BIND_KEEPALIVE_MC_NEW_HDR:
		dbg_cpu_reason_cnt[15]++;
		return 0;
	case HIT_BIND_KEEPALIVE_DUP_OLD_HDR:
		dbg_cpu_reason_cnt[16]++;
		return 0;
	case HIT_BIND_FORCE_TO_CPU:
		dbg_cpu_reason_cnt[17]++;
		return 0;
	case HIT_BIND_EXCEED_MTU:
		dbg_cpu_reason_cnt[18]++;
		return 0;
	case HIT_BIND_MULTICAST_TO_CPU:
		dbg_cpu_reason_cnt[19]++;
		return 0;
	case HIT_BIND_MULTICAST_TO_GMAC_CPU:
		dbg_cpu_reason_cnt[20]++;
		return 0;
	case HIT_PRE_BIND:
		dbg_cpu_reason_cnt[21]++;
		return 0;
	}

	return 0;
}

int hnat_set_usage(int level)
{
	debug_level = level;
	pr_info("Read cpu_reason count: cat /sys/kernel/debug/hnat/cpu_reason\n\n");
	pr_info("====================Advanced Settings====================\n");
	pr_info("Usage: echo [type] [option] > /sys/kernel/debug/hnat/cpu_reason\n\n");
	pr_info("Commands:   [type] [option]\n");
	pr_info("              0       0~7      Set debug_level(0~7), current debug_level=%d\n",
		debug_level);
	pr_info("              1    cpu_reason  Track entries of the set cpu_reason\n");
	pr_info("                               Set type=1 will change debug_level=7\n");
	pr_info("cpu_reason list:\n");
	pr_info("                       2       IPv4(IPv6) TTL(hop limit) = 0\n");
	pr_info("                       3       IPv4(IPv6) has option(extension) header\n");
	pr_info("                       7       No flow is assigned\n");
	pr_info("                       8       IPv4 HNAT doesn't support IPv4 /w fragment\n");
	pr_info("                       9       IPv4 HNAPT/DS-Lite doesn't support IPv4 /w fragment\n");
	pr_info("                      10       IPv4 HNAPT/DS-Lite can't find TCP/UDP sport/dport\n");
	pr_info("                      11       IPv6 5T-route/6RD can't find TCP/UDP sport/dport\n");
	pr_info("                      12       Ingress packet is TCP fin/syn/rst\n");
	pr_info("                      13       FOE Un-hit\n");
	pr_info("                      14       FOE Hit unbind\n");
	pr_info("                      15       FOE Hit unbind & rate reach\n");
	pr_info("                      16       Hit bind PPE TCP FIN entry\n");
	pr_info("                      17       Hit bind PPE entry and TTL(hop limit) = 1\n");
	pr_info("                      18       Hit bind and VLAN replacement violation\n");
	pr_info("                      19       Hit bind and keep alive with unicast old-header packet\n");
	pr_info("                      20       Hit bind and keep alive with multicast new-header packet\n");
	pr_info("                      21       Hit bind and keep alive with duplicate old-header packet\n");
	pr_info("                      22       FOE Hit bind & force to CPU\n");
	pr_info("                      23       HIT_BIND_WITH_OPTION_HEADER\n");
	pr_info("                      24       Switch clone multicast packet to CPU\n");
	pr_info("                      25       Switch clone multicast packet to GMAC1 & CPU\n");
	pr_info("                      26       HIT_PRE_BIND\n");
	pr_info("                      27       HIT_BIND_PACKET_SAMPLING\n");
	pr_info("                      28       Hit bind and exceed MTU\n");

	return 0;
}

int hnat_cpu_reason(int cpu_reason)
{
	dbg_cpu_reason = cpu_reason;
	debug_level = 7;
	pr_info("show cpu reason = %d\n", cpu_reason);

	return 0;
}

int entry_set_usage(int level)
{
	debug_level = level;
	pr_info("Show all entries(default state=bind): cat /sys/kernel/debug/hnat/hnat_entry\n\n");
	pr_info("====================Advanced Settings====================\n");
	pr_info("Usage: echo [type] [option] > /sys/kernel/debug/hnat/hnat_entry\n\n");
	pr_info("Commands:   [type] [option]\n");
	pr_info("              0       0~7      Set debug_level(0~7), current debug_level=%d\n",
		debug_level);
	pr_info("              1       0~3      Change tracking state\n");
	pr_info("                               (0:invalid; 1:unbind; 2:bind; 3:fin)\n");
	pr_info("              2   <entry_idx>  Show specific foe entry info. of assigned <entry_idx>\n");
	pr_info("              3   <entry_idx>  Delete specific foe entry of assigned <entry_idx>\n");

	return 0;
}

int entry_set_state(int state)
{
	dbg_entry_state = state;
	pr_info("ENTRY STATE = %s\n", dbg_entry_state == 0 ?
		"Invalid" : dbg_entry_state == 1 ?
		"Unbind" : dbg_entry_state == 2 ?
		"BIND" : dbg_entry_state == 3 ?
		"FIN" : "Unknown");
	return 0;
}

int entry_detail(int index)
{
	struct foe_entry *entry;
	struct mtk_hnat *h = hnat_priv;
	u32 *p;
	u32 i = 0;
	u32 print_cnt;
	unsigned char h_dest[ETH_ALEN];
	unsigned char h_source[ETH_ALEN];
	__be32 saddr, daddr, nsaddr, ndaddr;

	entry = h->foe_table_cpu + index;
	saddr = htonl(entry->ipv4_hnapt.sip);
	daddr = htonl(entry->ipv4_hnapt.dip);
	nsaddr = htonl(entry->ipv4_hnapt.new_sip);
	ndaddr = htonl(entry->ipv4_hnapt.new_dip);
	p = (uint32_t *)entry;
	pr_info("==========<Flow Table Entry=%d (%p)>===============\n", index,
		entry);
	if (debug_level >= 2) {
		print_cnt = 20;
		for (i = 0; i < print_cnt; i++)
			pr_info("%02d: %08X\n", i, *(p + i));
	}
	pr_info("-----------------<Flow Info>------------------\n");
	pr_info("Information Block 1: %08X\n", entry->ipv4_hnapt.info_blk1);

	if (IS_IPV4_HNAPT(entry)) {
		pr_info("Information Block 2: %08X (FP=%d FQOS=%d QID=%d)",
			entry->ipv4_hnapt.info_blk2,
			entry->ipv4_hnapt.info_blk2 >> 5 & 0x7,
			entry->ipv4_hnapt.info_blk2 >> 4 & 0x1,
			entry->ipv4_hnapt.iblk2.qid);
		pr_info("Create IPv4 HNAPT entry\n");
		pr_info("IPv4 Org IP/Port: %pI4:%d->%pI4:%d\n", &saddr,
			entry->ipv4_hnapt.sport, &daddr,
			entry->ipv4_hnapt.dport);
		pr_info("IPv4 New IP/Port: %pI4:%d->%pI4:%d\n", &nsaddr,
			entry->ipv4_hnapt.new_sport, &ndaddr,
			entry->ipv4_hnapt.new_dport);
	} else if (IS_IPV4_HNAT(entry)) {
		pr_info("Information Block 2: %08X\n",
			entry->ipv4_hnapt.info_blk2);
		pr_info("Create IPv4 HNAT entry\n");
		pr_info("IPv4 Org IP: %pI4->%pI4\n", &saddr, &daddr);
		pr_info("IPv4 New IP: %pI4->%pI4\n", &nsaddr, &ndaddr);
	} else if (IS_IPV4_DSLITE(entry)) {
		pr_info("Information Block 2: %08X\n",
			entry->ipv4_dslite.info_blk2);
		pr_info("Create IPv4 Ds-Lite entry\n");
		pr_info("IPv4 Ds-Lite: %pI4:%d->%pI4:%d\n", &saddr,
			entry->ipv4_dslite.sport, &daddr,
			entry->ipv4_dslite.dport);
		pr_info("EG DIPv6: %08X:%08X:%08X:%08X->%08X:%08X:%08X:%08X\n",
			entry->ipv4_dslite.tunnel_sipv6_0,
			entry->ipv4_dslite.tunnel_sipv6_1,
			entry->ipv4_dslite.tunnel_sipv6_2,
			entry->ipv4_dslite.tunnel_sipv6_3,
			entry->ipv4_dslite.tunnel_dipv6_0,
			entry->ipv4_dslite.tunnel_dipv6_1,
			entry->ipv4_dslite.tunnel_dipv6_2,
			entry->ipv4_dslite.tunnel_dipv6_3);
	} else if (IS_IPV6_3T_ROUTE(entry)) {
		pr_info("Information Block 2: %08X\n",
			entry->ipv6_3t_route.info_blk2);
		pr_info("Create IPv6 3-Tuple entry\n");
		pr_info("ING SIPv6->DIPv6: %08X:%08X:%08X:%08X-> %08X:%08X:%08X:%08X (Prot=%d)\n",
			entry->ipv6_3t_route.ipv6_sip0,
			entry->ipv6_3t_route.ipv6_sip1,
			entry->ipv6_3t_route.ipv6_sip2,
			entry->ipv6_3t_route.ipv6_sip3,
			entry->ipv6_3t_route.ipv6_dip0,
			entry->ipv6_3t_route.ipv6_dip1,
			entry->ipv6_3t_route.ipv6_dip2,
			entry->ipv6_3t_route.ipv6_dip3,
			entry->ipv6_3t_route.prot);
	} else if (IS_IPV6_5T_ROUTE(entry)) {
		pr_info("Information Block 2: %08X\n",
			entry->ipv6_5t_route.info_blk2);
		pr_info("Create IPv6 5-Tuple entry\n");
		pr_info("ING SIPv6->DIPv6: %08X:%08X:%08X:%08X:%d-> %08X:%08X:%08X:%08X:%d\n",
			entry->ipv6_5t_route.ipv6_sip0,
			entry->ipv6_5t_route.ipv6_sip1,
			entry->ipv6_5t_route.ipv6_sip2,
			entry->ipv6_5t_route.ipv6_sip3,
			entry->ipv6_5t_route.sport,
			entry->ipv6_5t_route.ipv6_dip0,
			entry->ipv6_5t_route.ipv6_dip1,
			entry->ipv6_5t_route.ipv6_dip2,
			entry->ipv6_5t_route.ipv6_dip3,
			entry->ipv6_5t_route.dport);
	} else if (IS_IPV6_6RD(entry)) {
		pr_info("Information Block 2: %08X\n",
			entry->ipv6_6rd.info_blk2);
		pr_info("Create IPv6 6RD entry\n");
		pr_info("ING SIPv6->DIPv6: %08X:%08X:%08X:%08X:%d-> %08X:%08X:%08X:%08X:%d\n",
			entry->ipv6_6rd.ipv6_sip0, entry->ipv6_6rd.ipv6_sip1,
			entry->ipv6_6rd.ipv6_sip2, entry->ipv6_6rd.ipv6_sip3,
			entry->ipv6_6rd.sport, entry->ipv6_6rd.ipv6_dip0,
			entry->ipv6_6rd.ipv6_dip1, entry->ipv6_6rd.ipv6_dip2,
			entry->ipv6_6rd.ipv6_dip3, entry->ipv6_6rd.dport);
	}
	if (IS_IPV4_HNAPT(entry) || IS_IPV4_HNAT(entry)) {
		*((u32 *)h_source) = swab32(entry->ipv4_hnapt.smac_hi);
		*((u16 *)&h_source[4]) = swab16(entry->ipv4_hnapt.smac_lo);
		*((u32 *)h_dest) = swab32(entry->ipv4_hnapt.dmac_hi);
		*((u16 *)&h_dest[4]) = swab16(entry->ipv4_hnapt.dmac_lo);
		pr_info("SMAC=%pM => DMAC=%pM\n", h_source, h_dest);
		pr_info("State = %s, ",	entry->bfib1.state == 0 ?
			"Invalid" : entry->bfib1.state == 1 ?
			"Unbind" : entry->bfib1.state == 2 ?
			"BIND" : entry->bfib1.state == 3 ?
			"FIN" : "Unknown");
		pr_info("Vlan_Layer = %u, ", entry->bfib1.vlan_layer);
		pr_info("Eth_type = 0x%x, Vid1 = 0x%x, Vid2 = 0x%x\n",
			entry->ipv4_hnapt.etype, entry->ipv4_hnapt.vlan1,
			entry->ipv4_hnapt.vlan2);
		pr_info("multicast = %d, pppoe = %d, proto = %s\n",
			entry->ipv4_hnapt.iblk2.mcast,
			entry->ipv4_hnapt.bfib1.psn,
			entry->ipv4_hnapt.bfib1.udp == 0 ?
			"TCP" :	entry->ipv4_hnapt.bfib1.udp == 1 ?
			"UDP" : "Unknown");
		pr_info("=========================================\n\n");
	} else {
		*((u32 *)h_source) = swab32(entry->ipv6_5t_route.smac_hi);
		*((u16 *)&h_source[4]) = swab16(entry->ipv6_5t_route.smac_lo);
		*((u32 *)h_dest) = swab32(entry->ipv6_5t_route.dmac_hi);
		*((u16 *)&h_dest[4]) = swab16(entry->ipv6_5t_route.dmac_lo);
		pr_info("SMAC=%pM => DMAC=%pM\n", h_source, h_dest);
		pr_info("State = %s, ",	entry->bfib1.state == 0 ?
			"Invalid" : entry->bfib1.state == 1 ?
			"Unbind" : entry->bfib1.state == 2 ?
			"BIND" : entry->bfib1.state == 3 ?
			"FIN" : "Unknown");

		pr_info("Vlan_Layer = %u, ", entry->bfib1.vlan_layer);
		pr_info("Eth_type = 0x%x, Vid1 = 0x%x, Vid2 = 0x%x\n",
			entry->ipv6_5t_route.etype, entry->ipv6_5t_route.vlan1,
			entry->ipv6_5t_route.vlan2);
		pr_info("multicast = %d, pppoe = %d, proto = %s\n",
			entry->ipv6_5t_route.iblk2.mcast,
			entry->ipv6_5t_route.bfib1.psn,
			entry->ipv6_5t_route.bfib1.udp == 0 ?
			"TCP" :	entry->ipv6_5t_route.bfib1.udp == 1 ?
			"UDP" :	"Unknown");
		pr_info("=========================================\n\n");
	}
	return 0;
}

int entry_delete(int index)
{
	struct foe_entry *entry;
	struct mtk_hnat *h = hnat_priv;

	entry = h->foe_table_cpu + index;
	memset(entry, 0, sizeof(struct foe_entry));

	/* clear HWNAT cache */
	cr_set_field(hnat_priv->ppe_base + PPE_CAH_CTRL, CAH_X_MODE, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_CAH_CTRL, CAH_X_MODE, 0);
	cr_set_field(hnat_priv->ppe_base + PPE_CAH_CTRL, CAH_EN, 1);

	pr_info("delete entry idx = %d\n", index);

	return 0;
}
EXPORT_SYMBOL(entry_delete);

int cr_set_usage(int level)
{
	debug_level = level;
	pr_info("Dump hnat CR: cat /sys/kernel/debug/hnat/hnat_setting\n\n");
	pr_info("====================Advanced Settings====================\n");
	pr_info("Usage: echo [type] [option] > /sys/kernel/debug/hnat/hnat_setting\n\n");
	pr_info("Commands:   [type] [option]\n");
	pr_info("              0     0~7        Set debug_level(0~7), current debug_level=%d\n",
		debug_level);
	pr_info("              1     0~65535    Set binding threshold\n");
	pr_info("              2     0~65535    Set TCP bind lifetime\n");
	pr_info("              3     0~65535    Set FIN bind lifetime\n");
	pr_info("              4     0~65535    Set UDP bind lifetime\n");
	pr_info("              5     0~255      Set TCP keep alive interval\n");
	pr_info("              6     0~255      Set UDP keep alive interval\n");

	return 0;
}

int binding_threshold(int threshold)
{
	pr_info("Binding Threshold =%d\n", threshold);
	writel(threshold, hnat_priv->ppe_base + PPE_BNDR);
	return 0;
}

int tcp_bind_lifetime(int tcp_life)
{
	pr_info("tcp_life = %d\n", tcp_life);
	/* set Delta time for aging out an bind TCP FOE entry */
	cr_set_field(hnat_priv->ppe_base + PPE_BND_AGE_1, TCP_DLTA, tcp_life);

	return 0;
}

int fin_bind_lifetime(int fin_life)
{
	pr_info("fin_life = %d\n", fin_life);
	/* set Delta time for aging out an bind TCP FIN FOE entry */
	cr_set_field(hnat_priv->ppe_base + PPE_BND_AGE_1, FIN_DLTA, fin_life);

	return 0;
}

int udp_bind_lifetime(int udp_life)
{
	pr_info("udp_life = %d\n", udp_life);
	/* set Delta time for aging out an bind UDP FOE entry */
	cr_set_field(hnat_priv->ppe_base + PPE_BND_AGE_0, UDP_DLTA, udp_life);

	return 0;
}

int tcp_keep_alive(int tcp_interval)
{
	if (tcp_interval > 255) {
		tcp_interval = 255;
		pr_info("TCP keep alive max interval = 255\n");
	} else {
		pr_info("tcp_interval = %d\n", tcp_interval);
	}
	/* Keep alive time for bind FOE TCP entry */
	cr_set_field(hnat_priv->ppe_base + PPE_KA, TCP_KA, tcp_interval);

	return 0;
}

int udp_keep_alive(int udp_interval)
{
	if (udp_interval > 255) {
		udp_interval = 255;
		pr_info("TCP/UDP keep alive max interval = 255\n");
	} else {
		pr_info("udp_interval = %d\n", udp_interval);
	}
	/* Keep alive timer for bind FOE UDP entry */
	cr_set_field(hnat_priv->ppe_base + PPE_KA, UDP_KA, udp_interval);

	return 0;
}

static const debugfs_write_func hnat_set_func[] = {
	[0] = hnat_set_usage,
	[1] = hnat_cpu_reason,
};

static const debugfs_write_func entry_set_func[] = {
	[0] = entry_set_usage,
	[1] = entry_set_state,
	[2] = entry_detail,
	[3] = entry_delete,
};

static const debugfs_write_func cr_set_func[] = {
	[0] = cr_set_usage,      [1] = binding_threshold,
	[2] = tcp_bind_lifetime, [3] = fin_bind_lifetime,
	[4] = udp_bind_lifetime, [5] = tcp_keep_alive,
	[6] = udp_keep_alive,
};

static struct hnat_accounting *hnat_get_count(struct mtk_hnat *h, u32 index)
{
	struct hnat_accounting *acount;
	u32 val, cnt_r0, cnt_r1, cnt_r2;
	int ret = -1;

	if (!hnat_priv->data->per_flow_accounting)
		return NULL;

	writel(index | (1 << 16), h->ppe_base + PPE_MIB_SER_CR);
	ret = readx_poll_timeout(readl, h->ppe_base + PPE_MIB_SER_CR, val,
				 !(val & BIT_MIB_BUSY), 20, 10000);
	if (ret < 0) {
		pr_notice("mib busy,please check later\n");
		return NULL;
	}
	cnt_r0 = readl(h->ppe_base + PPE_MIB_SER_R0);
	cnt_r1 = readl(h->ppe_base + PPE_MIB_SER_R1);
	cnt_r2 = readl(h->ppe_base + PPE_MIB_SER_R2);
	acount = &h->acct[index];
	acount->bytes += cnt_r0 + ((u64)(cnt_r1 & 0xffff) << 32);
	acount->packets +=
		((cnt_r1 & 0xffff0000) >> 16) + ((cnt_r2 & 0xffffff) << 16);

	return acount;
}

#define PRINT_COUNT(m, acount) {if (acount) \
		seq_printf(m, "bytes=%llu|packets=%llu|", \
			   acount->bytes, acount->packets); }
static int hnat_debug_show(struct seq_file *m, void *private)
{
	struct mtk_hnat *h = hnat_priv;
	struct foe_entry *entry, *end;
	unsigned char h_dest[ETH_ALEN];
	unsigned char h_source[ETH_ALEN];
	struct hnat_accounting *acount;
	u32 entry_index = 0;

	entry = h->foe_table_cpu;
	end = h->foe_table_cpu + hnat_priv->foe_etry_num;
	while (entry < end) {
		if (!entry->bfib1.state) {
			entry++;
			entry_index++;
			continue;
		}
		acount = hnat_get_count(h, entry_index);
		if (IS_IPV4_HNAPT(entry)) {
			__be32 saddr = htonl(entry->ipv4_hnapt.sip);
			__be32 daddr = htonl(entry->ipv4_hnapt.dip);
			__be32 nsaddr = htonl(entry->ipv4_hnapt.new_sip);
			__be32 ndaddr = htonl(entry->ipv4_hnapt.new_dip);

			*((u32 *)h_source) = swab32(entry->ipv4_hnapt.smac_hi);
			*((u16 *)&h_source[4]) =
				swab16(entry->ipv4_hnapt.smac_lo);
			*((u32 *)h_dest) = swab32(entry->ipv4_hnapt.dmac_hi);
			*((u16 *)&h_dest[4]) =
				swab16(entry->ipv4_hnapt.dmac_lo);
			PRINT_COUNT(m, acount);
			seq_printf(m,
				   "index=0x%05x|state=%s|type=%s|%pI4:%d->%pI4:%d=>%pI4:%d->%pI4:%d|%pM=>%pM|etype=0x%04x|info1=0x%x|info2=0x%x|vlan1=%d|vlan2=%d\n",
				   ei(entry, end), es(entry), pt(entry), &saddr,
				   entry->ipv4_hnapt.sport, &daddr,
				   entry->ipv4_hnapt.dport, &nsaddr,
				   entry->ipv4_hnapt.new_sport, &ndaddr,
				   entry->ipv4_hnapt.new_dport, h_source, h_dest,
				   ntohs(entry->ipv4_hnapt.etype),
				   entry->ipv4_hnapt.info_blk1,
				   entry->ipv4_hnapt.info_blk2,
				   entry->ipv4_hnapt.vlan1,
				   entry->ipv4_hnapt.vlan2);
		} else if (IS_IPV4_HNAT(entry)) {
			__be32 saddr = htonl(entry->ipv4_hnapt.sip);
			__be32 daddr = htonl(entry->ipv4_hnapt.dip);
			__be32 nsaddr = htonl(entry->ipv4_hnapt.new_sip);
			__be32 ndaddr = htonl(entry->ipv4_hnapt.new_dip);

			*((u32 *)h_source) = swab32(entry->ipv4_hnapt.smac_hi);
			*((u16 *)&h_source[4]) =
				swab16(entry->ipv4_hnapt.smac_lo);
			*((u32 *)h_dest) = swab32(entry->ipv4_hnapt.dmac_hi);
			*((u16 *)&h_dest[4]) =
				swab16(entry->ipv4_hnapt.dmac_lo);
			PRINT_COUNT(m, acount);
			seq_printf(m,
				   "index=0x%05x|state=%s|type=%s|%pI4->%pI4=>%pI4->%pI4|%pM=>%pM|etype=0x%04x|info1=0x%x|info2=0x%x|vlan1=%d|vlan2=%d\n",
				   ei(entry, end), es(entry), pt(entry), &saddr,
				   &daddr, &nsaddr, &ndaddr, h_source, h_dest,
				   ntohs(entry->ipv4_hnapt.etype),
				   entry->ipv4_hnapt.info_blk1,
				   entry->ipv4_hnapt.info_blk2,
				   entry->ipv4_hnapt.vlan1,
				   entry->ipv4_hnapt.vlan2);
		} else if (IS_IPV6_5T_ROUTE(entry)) {
			u32 ipv6_sip0 = entry->ipv6_3t_route.ipv6_sip0;
			u32 ipv6_sip1 = entry->ipv6_3t_route.ipv6_sip1;
			u32 ipv6_sip2 = entry->ipv6_3t_route.ipv6_sip2;
			u32 ipv6_sip3 = entry->ipv6_3t_route.ipv6_sip3;
			u32 ipv6_dip0 = entry->ipv6_3t_route.ipv6_dip0;
			u32 ipv6_dip1 = entry->ipv6_3t_route.ipv6_dip1;
			u32 ipv6_dip2 = entry->ipv6_3t_route.ipv6_dip2;
			u32 ipv6_dip3 = entry->ipv6_3t_route.ipv6_dip3;

			*((u32 *)h_source) =
				swab32(entry->ipv6_5t_route.smac_hi);
			*((u16 *)&h_source[4]) =
				swab16(entry->ipv6_5t_route.smac_lo);
			*((u32 *)h_dest) = swab32(entry->ipv6_5t_route.dmac_hi);
			*((u16 *)&h_dest[4]) =
				swab16(entry->ipv6_5t_route.dmac_lo);
			PRINT_COUNT(m, acount);
			seq_printf(m,
				   "index=0x%05x|state=%s|type=%s|SIP=%08x:%08x:%08x:%08x(sp=%d)->DIP=%08x:%08x:%08x:%08x(dp=%d)|%pM=>%pM|etype=0x%04x|info1=0x%x|info2=0x%x\n",
				   ei(entry, end), es(entry), pt(entry), ipv6_sip0,
				   ipv6_sip1, ipv6_sip2, ipv6_sip3,
				   entry->ipv6_5t_route.sport, ipv6_dip0,
				   ipv6_dip1, ipv6_dip2, ipv6_dip3,
				   entry->ipv6_5t_route.dport, h_source, h_dest,
				   ntohs(entry->ipv6_5t_route.etype),
				   entry->ipv6_5t_route.info_blk1,
				   entry->ipv6_5t_route.info_blk2);
		} else if (IS_IPV6_3T_ROUTE(entry)) {
			u32 ipv6_sip0 = entry->ipv6_3t_route.ipv6_sip0;
			u32 ipv6_sip1 = entry->ipv6_3t_route.ipv6_sip1;
			u32 ipv6_sip2 = entry->ipv6_3t_route.ipv6_sip2;
			u32 ipv6_sip3 = entry->ipv6_3t_route.ipv6_sip3;
			u32 ipv6_dip0 = entry->ipv6_3t_route.ipv6_dip0;
			u32 ipv6_dip1 = entry->ipv6_3t_route.ipv6_dip1;
			u32 ipv6_dip2 = entry->ipv6_3t_route.ipv6_dip2;
			u32 ipv6_dip3 = entry->ipv6_3t_route.ipv6_dip3;

			*((u32 *)h_source) =
				swab32(entry->ipv6_5t_route.smac_hi);
			*((u16 *)&h_source[4]) =
				swab16(entry->ipv6_5t_route.smac_lo);
			*((u32 *)h_dest) = swab32(entry->ipv6_5t_route.dmac_hi);
			*((u16 *)&h_dest[4]) =
				swab16(entry->ipv6_5t_route.dmac_lo);
			PRINT_COUNT(m, acount);
			seq_printf(m,
				   "index=0x%05x|state=%s|type=%s|SIP=%08x:%08x:%08x:%08x->DIP=%08x:%08x:%08x:%08x|%pM=>%pM|etype=0x%04x|info1=0x%x|info2=0x%x\n",
				   ei(entry, end), es(entry), pt(entry), ipv6_sip0,
				   ipv6_sip1, ipv6_sip2, ipv6_sip3, ipv6_dip0,
				   ipv6_dip1, ipv6_dip2, ipv6_dip3, h_source,
				   h_dest, ntohs(entry->ipv6_5t_route.etype),
				   entry->ipv6_5t_route.info_blk1,
				   entry->ipv6_5t_route.info_blk2);
		} else if (IS_IPV6_6RD(entry)) {
			u32 ipv6_sip0 = entry->ipv6_3t_route.ipv6_sip0;
			u32 ipv6_sip1 = entry->ipv6_3t_route.ipv6_sip1;
			u32 ipv6_sip2 = entry->ipv6_3t_route.ipv6_sip2;
			u32 ipv6_sip3 = entry->ipv6_3t_route.ipv6_sip3;
			u32 ipv6_dip0 = entry->ipv6_3t_route.ipv6_dip0;
			u32 ipv6_dip1 = entry->ipv6_3t_route.ipv6_dip1;
			u32 ipv6_dip2 = entry->ipv6_3t_route.ipv6_dip2;
			u32 ipv6_dip3 = entry->ipv6_3t_route.ipv6_dip3;
			__be32 tsaddr = htonl(entry->ipv6_6rd.tunnel_sipv4);
			__be32 tdaddr = htonl(entry->ipv6_6rd.tunnel_dipv4);

			*((u32 *)h_source) =
				swab32(entry->ipv6_5t_route.smac_hi);
			*((u16 *)&h_source[4]) =
				swab16(entry->ipv6_5t_route.smac_lo);
			*((u32 *)h_dest) = swab32(entry->ipv6_5t_route.dmac_hi);
			*((u16 *)&h_dest[4]) =
				swab16(entry->ipv6_5t_route.dmac_lo);
			PRINT_COUNT(m, acount);
			seq_printf(m,
				   "index=0x%05x|state=%s|type=%s|SIP=%08x:%08x:%08x:%08x(sp=%d)->DIP=%08x:%08x:%08x:%08x(dp=%d)|TSIP=%pI4->TDIP=%pI4|%pM=>%pM|etype=0x%04x|info1=0x%x|info2=0x%x\n",
				   ei(entry, end), es(entry), pt(entry), ipv6_sip0,
				   ipv6_sip1, ipv6_sip2, ipv6_sip3,
				   entry->ipv6_5t_route.sport, ipv6_dip0,
				   ipv6_dip1, ipv6_dip2, ipv6_dip3,
				   entry->ipv6_5t_route.dport, &tsaddr, &tdaddr,
				   h_source, h_dest,
				   ntohs(entry->ipv6_5t_route.etype),
				   entry->ipv6_5t_route.info_blk1,
				   entry->ipv6_5t_route.info_blk2);
		} else if (IS_IPV4_DSLITE(entry)) {
			__be32 saddr = htonl(entry->ipv4_hnapt.sip);
			__be32 daddr = htonl(entry->ipv4_hnapt.dip);
			u32 ipv6_tsip0 = entry->ipv4_dslite.tunnel_sipv6_0;
			u32 ipv6_tsip1 = entry->ipv4_dslite.tunnel_sipv6_1;
			u32 ipv6_tsip2 = entry->ipv4_dslite.tunnel_sipv6_2;
			u32 ipv6_tsip3 = entry->ipv4_dslite.tunnel_sipv6_3;
			u32 ipv6_tdip0 = entry->ipv4_dslite.tunnel_dipv6_0;
			u32 ipv6_tdip1 = entry->ipv4_dslite.tunnel_dipv6_1;
			u32 ipv6_tdip2 = entry->ipv4_dslite.tunnel_dipv6_2;
			u32 ipv6_tdip3 = entry->ipv4_dslite.tunnel_dipv6_3;

			*((u32 *)h_source) = swab32(entry->ipv4_hnapt.smac_hi);
			*((u16 *)&h_source[4]) =
				swab16(entry->ipv4_hnapt.smac_lo);
			*((u32 *)h_dest) = swab32(entry->ipv4_hnapt.dmac_hi);
			*((u16 *)&h_dest[4]) =
				swab16(entry->ipv4_hnapt.dmac_lo);
			PRINT_COUNT(m, acount);
			seq_printf(m,
				   "index=0x%05x|state=%s|type=%s|SIP=%pI4->DIP=%pI4|TSIP=%08x:%08x:%08x:%08x->TDIP=%08x:%08x:%08x:%08x|%pM=>%pM|etype=0x%04x|info1=0x%x|info2=0x%x\n",
				   ei(entry, end), es(entry), pt(entry), &saddr,
				   &daddr, ipv6_tsip0, ipv6_tsip1, ipv6_tsip2,
				   ipv6_tsip3, ipv6_tdip0, ipv6_tdip1, ipv6_tdip2,
				   ipv6_tdip3, h_source, h_dest,
				   ntohs(entry->ipv6_5t_route.etype),
				   entry->ipv6_5t_route.info_blk1,
				   entry->ipv6_5t_route.info_blk2);
		} else
			seq_printf(m, "index=0x%05x state=%s\n", ei(entry, end),
				   es(entry));
		entry++;
		entry_index++;
	}

	return 0;
}

static int hnat_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, hnat_debug_show, file->private_data);
}

static const struct file_operations hnat_debug_fops = {
	.open = hnat_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int hnat_whnat_show(struct seq_file *m, void *private)
{
	int i;
	struct net_device *dev;

	for (i = 0; i < MAX_IF_NUM; i++) {
		dev = hnat_priv->wifi_hook_if[i];
		if (dev)
			seq_printf(m, "%d:%s\n", i, dev->name);
		else
			continue;
	}

	return 0;
}

static int hnat_whnat_open(struct inode *inode, struct file *file)
{
	return single_open(file, hnat_whnat_show, file->private_data);
}

static const struct file_operations hnat_whnat_fops = {
	.open = hnat_whnat_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int cpu_reason_read(struct seq_file *m, void *private)
{
	int i;

	pr_info("============ CPU REASON =========\n");
	pr_info("(2)IPv4(IPv6) TTL(hop limit) = %u\n", dbg_cpu_reason_cnt[0]);
	pr_info("(3)Ipv4(IPv6) has option(extension) header = %u\n",
		dbg_cpu_reason_cnt[1]);
	pr_info("(7)No flow is assigned = %u\n", dbg_cpu_reason_cnt[2]);
	pr_info("(8)IPv4 HNAT doesn't support IPv4 /w fragment = %u\n",
		dbg_cpu_reason_cnt[3]);
	pr_info("(9)IPv4 HNAPT/DS-Lite doesn't support IPv4 /w fragment = %u\n",
		dbg_cpu_reason_cnt[4]);
	pr_info("(10)IPv4 HNAPT/DS-Lite can't find TCP/UDP sport/dport = %u\n",
		dbg_cpu_reason_cnt[5]);
	pr_info("(11)IPv6 5T-route/6RD can't find TCP/UDP sport/dport = %u\n",
		dbg_cpu_reason_cnt[6]);
	pr_info("(12)Ingress packet is TCP fin/syn/rst = %u\n",
		dbg_cpu_reason_cnt[7]);
	pr_info("(13)FOE Un-hit = %u\n", dbg_cpu_reason_cnt[8]);
	pr_info("(14)FOE Hit unbind = %u\n", dbg_cpu_reason_cnt[9]);
	pr_info("(15)FOE Hit unbind & rate reach = %u\n",
		dbg_cpu_reason_cnt[10]);
	pr_info("(16)Hit bind PPE TCP FIN entry = %u\n",
		dbg_cpu_reason_cnt[11]);
	pr_info("(17)Hit bind PPE entry and TTL(hop limit) = 1 and TTL(hot limit) - 1 = %u\n",
		dbg_cpu_reason_cnt[12]);
	pr_info("(18)Hit bind and VLAN replacement violation = %u\n",
		dbg_cpu_reason_cnt[13]);
	pr_info("(19)Hit bind and keep alive with unicast old-header packet = %u\n",
		dbg_cpu_reason_cnt[14]);
	pr_info("(20)Hit bind and keep alive with multicast new-header packet = %u\n",
		dbg_cpu_reason_cnt[15]);
	pr_info("(21)Hit bind and keep alive with duplicate old-header packet = %u\n",
		dbg_cpu_reason_cnt[16]);
	pr_info("(22)FOE Hit bind & force to CPU = %u\n",
		dbg_cpu_reason_cnt[17]);
	pr_info("(28)Hit bind and exceed MTU =%u\n", dbg_cpu_reason_cnt[18]);
	pr_info("(24)Hit bind multicast packet to CPU = %u\n",
		dbg_cpu_reason_cnt[19]);
	pr_info("(25)Hit bind multicast packet to GMAC & CPU = %u\n",
		dbg_cpu_reason_cnt[20]);
	pr_info("(26)Pre bind = %u\n", dbg_cpu_reason_cnt[21]);

	for (i = 0; i < 22; i++)
		dbg_cpu_reason_cnt[i] = 0;
	return 0;
}

static int cpu_reason_open(struct inode *inode, struct file *file)
{
	return single_open(file, cpu_reason_read, file->private_data);
}

ssize_t cpu_reason_write(struct file *file, const char __user *buffer,
			 size_t count, loff_t *data)
{
	char buf[32];
	char *p_buf;
	int len = count;
	long arg0 = 0, arg1 = 0;
	char *p_token = NULL;
	char *p_delimiter = " \t";
	int ret;

	if (len >= sizeof(buf)) {
		pr_info("input handling fail!\n");
		len = sizeof(buf) - 1;
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);
	if (!p_token)
		arg0 = 0;
	else
		ret = kstrtol(p_token, 10, &arg0);

	switch (arg0) {
	case 0:
	case 1:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		break;
	default:
		pr_info("no handler defined for command id(0x%08lx)\n\r", arg0);
		arg0 = 0;
		arg1 = 0;
		break;
	}

	(*hnat_set_func[arg0])(arg1);

	return len;
}

static const struct file_operations cpu_reason_fops = {
	.open = cpu_reason_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = cpu_reason_write,
	.release = single_release,
};

void dbg_dump_entry(struct seq_file *m, struct foe_entry *entry,
		    uint32_t index)
{
	__be32 saddr, daddr, nsaddr, ndaddr;

	saddr = htonl(entry->ipv4_hnapt.sip);
	daddr = htonl(entry->ipv4_hnapt.dip);
	nsaddr = htonl(entry->ipv4_hnapt.new_sip);
	ndaddr = htonl(entry->ipv4_hnapt.new_dip);

	if (IS_IPV4_HNAPT(entry)) {
		seq_printf(m,
			   "NAPT(%d): %pI4:%d->%pI4:%d => %pI4:%d->%pI4:%d\n",
			   index, &saddr, entry->ipv4_hnapt.sport, &daddr,
			   entry->ipv4_hnapt.dport, &nsaddr,
			   entry->ipv4_hnapt.new_sport, &ndaddr,
			   entry->ipv4_hnapt.new_dport);
	} else if (IS_IPV4_HNAT(entry)) {
		seq_printf(m, "NAT(%d): %pI4->%pI4 => %pI4->%pI4\n",
			   index, &saddr, &daddr, &nsaddr, &ndaddr);
	}

	if (IS_IPV4_DSLITE(entry)) {
		seq_printf(m,
			   "IPv4 Ds-Lite(%d): %pI4:%d->%pI4:%d => %08X:%08X:%08X:%08X->%08X:%08X:%08X:%08X\n",
			   index, &saddr, entry->ipv4_dslite.sport, &daddr,
			   entry->ipv4_dslite.dport,
			   entry->ipv4_dslite.tunnel_sipv6_0,
			   entry->ipv4_dslite.tunnel_sipv6_1,
			   entry->ipv4_dslite.tunnel_sipv6_2,
			   entry->ipv4_dslite.tunnel_sipv6_3,
			   entry->ipv4_dslite.tunnel_dipv6_0,
			   entry->ipv4_dslite.tunnel_dipv6_1,
			   entry->ipv4_dslite.tunnel_dipv6_2,
			   entry->ipv4_dslite.tunnel_dipv6_3);
	} else if (IS_IPV6_3T_ROUTE(entry)) {
		seq_printf(m,
			   "IPv6_3T(%d): %08X:%08X:%08X:%08X => %08X:%08X:%08X:%08X (Prot=%d)\n",
			   index, entry->ipv6_3t_route.ipv6_sip0,
			   entry->ipv6_3t_route.ipv6_sip1,
			   entry->ipv6_3t_route.ipv6_sip2,
			   entry->ipv6_3t_route.ipv6_sip3,
			   entry->ipv6_3t_route.ipv6_dip0,
			   entry->ipv6_3t_route.ipv6_dip1,
			   entry->ipv6_3t_route.ipv6_dip2,
			   entry->ipv6_3t_route.ipv6_dip3,
			   entry->ipv6_3t_route.prot);
	} else if (IS_IPV6_5T_ROUTE(entry)) {
		seq_printf(m,
			   "IPv6_5T(%d): %08X:%08X:%08X:%08X:%d => %08X:%08X:%08X:%08X:%d\n",
			   index, entry->ipv6_5t_route.ipv6_sip0,
			   entry->ipv6_5t_route.ipv6_sip1,
			   entry->ipv6_5t_route.ipv6_sip2,
			   entry->ipv6_5t_route.ipv6_sip3,
			   entry->ipv6_5t_route.sport,
			   entry->ipv6_5t_route.ipv6_dip0,
			   entry->ipv6_5t_route.ipv6_dip1,
			   entry->ipv6_5t_route.ipv6_dip2,
			   entry->ipv6_5t_route.ipv6_dip3,
			   entry->ipv6_5t_route.dport);
	} else if (IS_IPV6_6RD(entry)) {
		seq_printf(m,
			   "IPv6_6RD(%d): %08X:%08X:%08X:%08X:%d => %08X:%08X:%08X:%08X:%d\n",
			   index, entry->ipv6_6rd.ipv6_sip0,
			   entry->ipv6_6rd.ipv6_sip1, entry->ipv6_6rd.ipv6_sip2,
			   entry->ipv6_6rd.ipv6_sip3, entry->ipv6_6rd.sport,
			   entry->ipv6_6rd.ipv6_dip0, entry->ipv6_6rd.ipv6_dip1,
			   entry->ipv6_6rd.ipv6_dip2, entry->ipv6_6rd.ipv6_dip3,
			   entry->ipv6_6rd.dport);
	}
}

int hnat_entry_read(struct seq_file *m, void *private)
{
	struct mtk_hnat *h = hnat_priv;
	struct foe_entry *entry, *end;
	int hash_index;
	int cnt;

	hash_index = 0;
	cnt = 0;
	entry = h->foe_table_cpu;
	end = h->foe_table_cpu + hnat_priv->foe_etry_num;

	while (entry < end) {
		if (entry->bfib1.state == dbg_entry_state) {
			cnt++;
			dbg_dump_entry(m, entry, hash_index);
		}
		hash_index++;
		entry++;
	}

	seq_printf(m, "Total State = %s cnt = %d\n",
		   dbg_entry_state == 0 ?
		   "Invalid" : dbg_entry_state == 1 ?
		   "Unbind" : dbg_entry_state == 2 ?
		   "BIND" : dbg_entry_state == 3 ?
		   "FIN" : "Unknown", cnt);

	return 0;
}

ssize_t hnat_entry_write(struct file *file, const char __user *buffer,
			 size_t count, loff_t *data)
{
	char buf[32];
	char *p_buf;
	int len = count;
	long arg0 = 0, arg1 = 0;
	char *p_token = NULL;
	char *p_delimiter = " \t";
	int ret;

	if (len >= sizeof(buf)) {
		pr_info("input handling fail!\n");
		len = sizeof(buf) - 1;
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);
	if (!p_token)
		arg0 = 0;
	else
		ret = kstrtol(p_token, 10, &arg0);

	switch (arg0) {
	case 0:
	case 1:
	case 2:
	case 3:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		break;
	default:
		pr_info("no handler defined for command id(0x%08lx)\n\r", arg0);
		arg0 = 0;
		arg1 = 0;
		break;
	}

	(*entry_set_func[arg0])(arg1);

	return len;
}

static int hnat_entry_open(struct inode *inode, struct file *file)
{
	return single_open(file, hnat_entry_read, file->private_data);
}

static const struct file_operations hnat_entry_fops = {
	.open = hnat_entry_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = hnat_entry_write,
	.release = single_release,
};

int hnat_setting_read(struct seq_file *m, void *private)
{
	struct mtk_hnat *h = hnat_priv;
	int i;
	int cr_max;

	cr_max = 319 * 4;
	for (i = 0; i < cr_max; i = i + 0x10) {
		pr_info("0x%p : 0x%08x 0x%08x 0x%08x 0x%08x\n",
			(void *)h->foe_table_dev + i, readl(h->ppe_base + i),
			readl(h->ppe_base + i + 4), readl(h->ppe_base + i + 8),
			readl(h->ppe_base + i + 0xc));
	}

	return 0;
}

static int hnat_setting_open(struct inode *inode, struct file *file)
{
	return single_open(file, hnat_setting_read, file->private_data);
}

ssize_t hnat_setting_write(struct file *file, const char __user *buffer,
			   size_t count, loff_t *data)
{
	char buf[32];
	char *p_buf;
	int len = count;
	long arg0 = 0, arg1 = 0;
	char *p_token = NULL;
	char *p_delimiter = " \t";
	int ret;

	if (len >= sizeof(buf)) {
		pr_info("input handling fail!\n");
		len = sizeof(buf) - 1;
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);
	if (!p_token)
		arg0 = 0;
	else
		ret = kstrtol(p_token, 10, &arg0);

	switch (arg0) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		break;
	default:
		pr_info("no handler defined for command id(0x%08lx)\n\r", arg0);
		arg0 = 0;
		arg1 = 0;
		break;
	}

	(*cr_set_func[arg0])(arg1);

	return len;
}

static const struct file_operations hnat_setting_fops = {
	.open = hnat_setting_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = hnat_setting_write,
	.release = single_release,
};

int mcast_table_dump(struct seq_file *m, void *private)
{
	struct mtk_hnat *h = hnat_priv;
	struct ppe_mcast_h mcast_h;
	struct ppe_mcast_l mcast_l;
	u8 i, max;
	void __iomem *reg;

	if (!h->pmcast)
		return 0;

	max = h->pmcast->max_entry;
	pr_info("MAC | VID | PortMask | QosPortMask\n");
	for (i = 0; i < max; i++) {
		if (i < 0x10) {
			reg = h->ppe_base + PPE_MCAST_H_0 + i * 8;
			mcast_h.u.value = readl(reg);
			reg = h->ppe_base + PPE_MCAST_L_0 + i * 8;
			mcast_l.addr = readl(reg);
		} else {
			reg = h->fe_base + PPE_MCAST_H_10 + (i - 0x10) * 8;
			mcast_h.u.value = readl(reg);
			reg = h->fe_base + PPE_MCAST_L_10 + (i - 0x10) * 8;
			mcast_l.addr = readl(reg);
		}
		pr_info("%08x %d %c%c%c%c %c%c%c%c (QID=%d, mc_mpre_sel=%d)\n",
			mcast_l.addr,
			mcast_h.u.info.mc_vid,
			(mcast_h.u.info.mc_px_en & 0x08) ? '1' : '-',
			(mcast_h.u.info.mc_px_en & 0x04) ? '1' : '-',
			(mcast_h.u.info.mc_px_en & 0x02) ? '1' : '-',
			(mcast_h.u.info.mc_px_en & 0x01) ? '1' : '-',
			(mcast_h.u.info.mc_px_qos_en & 0x08) ? '1' : '-',
			(mcast_h.u.info.mc_px_qos_en & 0x04) ? '1' : '-',
			(mcast_h.u.info.mc_px_qos_en & 0x02) ? '1' : '-',
			(mcast_h.u.info.mc_px_qos_en & 0x01) ? '1' : '-',
			mcast_h.u.info.mc_qos_qid +
			((mcast_h.u.info.mc_qos_qid54) << 4),
			mcast_h.u.info.mc_mpre_sel);
	}

	return 0;
}

static int mcast_table_open(struct inode *inode, struct file *file)
{
	return single_open(file, mcast_table_dump, file->private_data);
}

static const struct file_operations hnat_mcast_fops = {
	.open = mcast_table_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int hnat_ext_show(struct seq_file *m, void *private)
{
	int i;
	struct extdev_entry *ext_entry;

	for (i = 0; i < MAX_EXT_DEVS && hnat_priv->ext_if[i]; i++) {
		ext_entry = hnat_priv->ext_if[i];
		if (ext_entry->dev)
			seq_printf(m, "ext devices [%d] = %s  (dev=%p, ifindex=%d)\n",
				   i, ext_entry->name, ext_entry->dev,
				   ext_entry->dev->ifindex);
	}

	return 0;
}

static int hnat_ext_open(struct inode *inode, struct file *file)
{
	return single_open(file, hnat_ext_show, file->private_data);
}

static const struct file_operations hnat_ext_fops = {
	.open = hnat_ext_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t hnat_sched_show(struct file *file, char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	long id = (long)file->private_data;
	struct mtk_hnat *h = hnat_priv;
	u32 qdma_tx_sch;
	int enable;
	int scheduling;
	int max_rate;
	char *buf;
	unsigned int len = 0, buf_len = 1500;
	ssize_t ret_cnt;
	int scheduler, i;
	u32 sch_reg;

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (hnat_priv->data->num_of_sch == 4)
		qdma_tx_sch = readl(h->fe_base + QDMA_TX_4SCH_BASE(id));
	else
		qdma_tx_sch = readl(h->fe_base + QDMA_TX_2SCH_BASE);

	if (id & 0x1)
		qdma_tx_sch >>= 16;
	qdma_tx_sch &= 0xffff;
	enable = !!(qdma_tx_sch & BIT(11));
	scheduling = !!(qdma_tx_sch & BIT(15));
	max_rate = ((qdma_tx_sch >> 4) & 0x7f);
	qdma_tx_sch &= 0xf;
	while (qdma_tx_sch--)
		max_rate *= 10;

	len += scnprintf(buf + len, buf_len - len,
			 "EN\tScheduling\tMAX\tQueue#\n%d\t%s%16d\t", enable,
			 (scheduling == 1) ? "WRR" : "SP", max_rate);

	for (i = 0; i < MTK_QDMA_TX_NUM; i++) {
		cr_set_field(h->fe_base + QDMA_PAGE, QTX_CFG_PAGE,
			     (i / NUM_OF_Q_PER_PAGE));
		sch_reg = readl(h->fe_base + QTX_SCH(i % NUM_OF_Q_PER_PAGE));
		if (hnat_priv->data->num_of_sch == 4)
			scheduler = (sch_reg >> 30) & 0x3;
		else
			scheduler = !!(sch_reg & BIT(31));
		if (id == scheduler)
			len += scnprintf(buf + len, buf_len - len, "%d  ", i);
	}

	len += scnprintf(buf + len, buf_len - len, "\n");
	if (len > buf_len)
		len = buf_len;

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);
	return ret_cnt;
}

static ssize_t hnat_sched_write(struct file *file, const char __user *buf,
				size_t length, loff_t *offset)
{
	long id = (long)file->private_data;
	struct mtk_hnat *h = hnat_priv;
	char line[64];
	int enable, rate, exp = 0, shift = 0;
	char scheduling[32];
	size_t size;
	u32 qdma_tx_sch;
	u32 val = 0;

	if (length > sizeof(line))
		return -EINVAL;

	if (copy_from_user(line, buf, length))
		return -EFAULT;

	if (sscanf(line, "%d %s %d", &enable, scheduling, &rate) != 3)
		return -EFAULT;

	while (rate > 127) {
		rate /= 10;
		exp++;
	}

	if (enable)
		val |= BIT(11);
	if (strcmp(scheduling, "sp") != 0)
		val |= BIT(15);
	val |= (rate & 0x7f) << 4;
	val |= exp & 0xf;
	if (id & 0x1)
		shift = 16;

	if (hnat_priv->data->num_of_sch == 4)
		qdma_tx_sch = readl(h->fe_base + QDMA_TX_4SCH_BASE(id));
	else
		qdma_tx_sch = readl(h->fe_base + QDMA_TX_2SCH_BASE);

	qdma_tx_sch &= ~(0xffff << shift);
	qdma_tx_sch |= val << shift;
	if (hnat_priv->data->num_of_sch == 4)
		writel(qdma_tx_sch, h->fe_base + QDMA_TX_4SCH_BASE(id));
	else
		writel(qdma_tx_sch, h->fe_base + QDMA_TX_2SCH_BASE);

	size = strlen(line);
	*offset += size;

	return length;
}

static const struct file_operations hnat_sched_fops = {
	.open = simple_open,
	.read = hnat_sched_show,
	.write = hnat_sched_write,
	.llseek = default_llseek,
};

static ssize_t hnat_queue_show(struct file *file, char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	struct mtk_hnat *h = hnat_priv;
	long id = (long)file->private_data;
	u32 qtx_sch;
	u32 qtx_cfg;
	int scheduler;
	int min_rate_en;
	int min_rate;
	int min_rate_exp;
	int max_rate_en;
	int max_weight;
	int max_rate;
	int max_rate_exp;
	char *buf;
	unsigned int len = 0, buf_len = 1500;
	ssize_t ret_cnt;

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	cr_set_field(h->fe_base + QDMA_PAGE, QTX_CFG_PAGE, (id / NUM_OF_Q_PER_PAGE));
	qtx_cfg = readl(h->fe_base + QTX_CFG(id % NUM_OF_Q_PER_PAGE));
	qtx_sch = readl(h->fe_base + QTX_SCH(id % NUM_OF_Q_PER_PAGE));
	if (hnat_priv->data->num_of_sch == 4)
		scheduler = (qtx_sch >> 30) & 0x3;
	else
		scheduler = !!(qtx_sch & BIT(31));
	min_rate_en = !!(qtx_sch & BIT(27));
	min_rate = (qtx_sch >> 20) & 0x7f;
	min_rate_exp = (qtx_sch >> 16) & 0xf;
	max_rate_en = !!(qtx_sch & BIT(11));
	max_weight = (qtx_sch >> 12) & 0xf;
	max_rate = (qtx_sch >> 4) & 0x7f;
	max_rate_exp = qtx_sch & 0xf;
	while (min_rate_exp--)
		min_rate *= 10;

	while (max_rate_exp--)
		max_rate *= 10;

	len += scnprintf(buf + len, buf_len - len,
			 "scheduler: %d\nhw resv: %d\nsw resv: %d\n", scheduler,
			 (qtx_cfg >> 8) & 0xff, qtx_cfg & 0xff);

	if (hnat_priv->data->version != MTK_HNAT_V1) {
		/* Switch to debug mode */
		cr_set_field(h->fe_base + QTX_MIB_IF, MIB_ON_QTX_CFG, 1);
		cr_set_field(h->fe_base + QTX_MIB_IF, VQTX_MIB_EN, 1);
		qtx_cfg = readl(h->fe_base + QTX_CFG(id % NUM_OF_Q_PER_PAGE));
		qtx_sch = readl(h->fe_base + QTX_SCH(id % NUM_OF_Q_PER_PAGE));
		len += scnprintf(buf + len, buf_len - len,
				 "packet count: %u\n", qtx_cfg);
		len += scnprintf(buf + len, buf_len - len,
				 "packet drop: %u\n\n", qtx_sch);

		/* Recover to normal mode */
		cr_set_field(hnat_priv->fe_base + QTX_MIB_IF,
			     MIB_ON_QTX_CFG, 0);
		cr_set_field(hnat_priv->fe_base + QTX_MIB_IF, VQTX_MIB_EN, 0);
	}

	len += scnprintf(buf + len, buf_len - len,
			 "      EN     RATE     WEIGHT\n");
	len += scnprintf(buf + len, buf_len - len,
			 "----------------------------\n");
	len += scnprintf(buf + len, buf_len - len,
			 "max%5d%9d%9d\n", max_rate_en, max_rate, max_weight);
	len += scnprintf(buf + len, buf_len - len,
			 "min%5d%9d        -\n", min_rate_en, min_rate);

	if (len > buf_len)
		len = buf_len;

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);
	return ret_cnt;
}

static ssize_t hnat_queue_write(struct file *file, const char __user *buf,
				size_t length, loff_t *offset)
{
	long id = (long)file->private_data;
	struct mtk_hnat *h = hnat_priv;
	char line[64];
	int max_enable, max_rate, max_exp = 0;
	int min_enable, min_rate, min_exp = 0;
	int weight;
	int resv;
	int scheduler;
	size_t size;
	u32 qtx_sch;

	cr_set_field(h->fe_base + QDMA_PAGE, QTX_CFG_PAGE, (id / NUM_OF_Q_PER_PAGE));
	qtx_sch = readl(h->fe_base + QTX_SCH(id % NUM_OF_Q_PER_PAGE));
	if (length > sizeof(line))
		return -EINVAL;

	if (copy_from_user(line, buf, length))
		return -EFAULT;

	if (sscanf(line, "%d %d %d %d %d %d %d", &scheduler, &min_enable, &min_rate,
		   &max_enable, &max_rate, &weight, &resv) != 7)
		return -EFAULT;

	while (max_rate > 127) {
		max_rate /= 10;
		max_exp++;
	}

	while (min_rate > 127) {
		min_rate /= 10;
		min_exp++;
	}

	qtx_sch &= 0x70000000;
	if (hnat_priv->data->num_of_sch == 4)
		qtx_sch |= (scheduler & 0x3) << 30;
	else
		qtx_sch |= (scheduler & 0x1) << 31;
	if (min_enable)
		qtx_sch |= BIT(27);
	qtx_sch |= (min_rate & 0x7f) << 20;
	qtx_sch |= (min_exp & 0xf) << 16;
	if (max_enable)
		qtx_sch |= BIT(11);
	qtx_sch |= (weight & 0xf) << 12;
	qtx_sch |= (max_rate & 0x7f) << 4;
	qtx_sch |= max_exp & 0xf;
	writel(qtx_sch, h->fe_base + QTX_SCH(id % NUM_OF_Q_PER_PAGE));

	resv &= 0xff;
	qtx_sch = readl(h->fe_base + QTX_CFG(id % NUM_OF_Q_PER_PAGE));
	qtx_sch &= 0xffff0000;
	qtx_sch |= (resv << 8) | resv;
	writel(qtx_sch, h->fe_base + QTX_CFG(id % NUM_OF_Q_PER_PAGE));

	size = strlen(line);
	*offset += size;

	return length;
}

static const struct file_operations hnat_queue_fops = {
	.open = simple_open,
	.read = hnat_queue_show,
	.write = hnat_queue_write,
	.llseek = default_llseek,
};

static int hnat_mape_toggle_read(struct seq_file *m, void *private)
{
	pr_info("value=%d, %s is enabled now!\n", mape_toggle, (mape_toggle) ? "mape" : "ds-lite");

	return 0;
}

static int hnat_mape_toggle_open(struct inode *inode, struct file *file)
{
	return single_open(file, hnat_mape_toggle_read, file->private_data);
}

static ssize_t hnat_mape_toggle_write(struct file *file, const char __user *buffer,
				      size_t count, loff_t *data)
{
	char buf;
	int len = count;

	if (copy_from_user(&buf, buffer, len))
		return -EFAULT;

	if (buf == '1' && !mape_toggle) {
		pr_info("mape is going to be enabled, ds-lite is going to be disabled !\n");
		mape_toggle = 1;
	} else if (buf == '0' && mape_toggle) {
		pr_info("ds-lite is going to be enabled, mape is going to be disabled !\n");
		mape_toggle = 0;
	}

	return len;
}

static const struct file_operations hnat_mape_toggle_fops = {
	.open = hnat_mape_toggle_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = hnat_mape_toggle_write,
	.release = single_release,
};

static int hnat_hook_toggle_read(struct seq_file *m, void *private)
{
	pr_info("value=%d, hook is %s now!\n", hook_toggle, (hook_toggle) ? "enabled" : "disabled");

	return 0;
}

static int hnat_hook_toggle_open(struct inode *inode, struct file *file)
{
	return single_open(file, hnat_hook_toggle_read, file->private_data);
}

static ssize_t hnat_hook_toggle_write(struct file *file, const char __user *buffer,
				      size_t count, loff_t *data)
{
	char buf;
	int len = count;

	if (copy_from_user(&buf, buffer, len))
		return -EFAULT;

	if (buf == '1' && !hook_toggle) {
		pr_info("hook is going to be enabled !\n");
		hnat_enable_hook();
	} else if (buf == '0' && hook_toggle) {
		pr_info("hook is going to be disabled !\n");
		hnat_disable_hook();
	}

	return len;
}

static const struct file_operations hnat_hook_toggle_fops = {
	.open = hnat_hook_toggle_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = hnat_hook_toggle_write,
	.release = single_release,
};

static int hnat_version_read(struct seq_file *m, void *private)
{
	pr_info("HNAT SW version : %s\nHNAT HW version : %d\n", HNAT_SW_VER, hnat_priv->data->version);

	return 0;
}

static int hnat_version_open(struct inode *inode, struct file *file)
{
	return single_open(file, hnat_version_read, file->private_data);
}

static const struct file_operations hnat_version_fops = {
	.open = hnat_version_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int get_ppe_mib(int index, u64 *pkt_cnt, u64 *byte_cnt)
{
	struct mtk_hnat *h = hnat_priv;
	struct hnat_accounting *acount;
	struct foe_entry *entry;

	acount = hnat_get_count(h, index);
	entry = hnat_priv->foe_table_cpu + index;

	if (!acount)
		return -1;

	if (entry->bfib1.state != BIND)
		return -1;

	*pkt_cnt = acount->packets;
	*byte_cnt = acount->bytes;

	return 0;
}
EXPORT_SYMBOL(get_ppe_mib);

int is_entry_binding(int index)
{
	struct foe_entry *entry;

	entry = hnat_priv->foe_table_cpu + index;

	return entry->bfib1.state == BIND;
}
EXPORT_SYMBOL(is_entry_binding);

#define dump_register(nm)                                                      \
	{                                                                      \
		.name = __stringify(nm), .offset = PPE_##nm,                   \
	}

#if defined(CONFIG_NET_MEDIATEK_HW_QOS)
static int hnat_hqos_toggle_read(struct seq_file *m, void *private)
{
	pr_info("value=%d, hqos is %s now!\n", hqos_toggle, (hqos_toggle) ? "enabled" : "disabled");

	return 0;
}

static int hnat_hqos_toggle_open(struct inode *inode, struct file *file)
{
	return single_open(file, hnat_hqos_toggle_read, file->private_data);
}

static ssize_t hnat_hqos_toggle_write(struct file *file, const char __user *buffer,
                                      size_t count, loff_t *data)
{
	char buf;
	int len = count;

	if (copy_from_user(&buf, buffer, len))
		return -EFAULT;

	if (buf == '1' && !hqos_toggle) {
		pr_info("hqos is going to be enabled !\n");
		hqos_toggle = 1;
	} else if (buf == '0' && hqos_toggle) {
		pr_info("hqos is going to be disabled !\n");
		hqos_toggle = 0;
	}

	return len;
}

static const struct file_operations hnat_hqos_toggle_fops = {
	.open = hnat_hqos_toggle_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = hnat_hqos_toggle_write,
    .release = single_release,
};
#endif


static const struct debugfs_reg32 hnat_regs[] = {
	dump_register(GLO_CFG),     dump_register(FLOW_CFG),
	dump_register(IP_PROT_CHK), dump_register(IP_PROT_0),
	dump_register(IP_PROT_1),   dump_register(IP_PROT_2),
	dump_register(IP_PROT_3),   dump_register(TB_CFG),
	dump_register(TB_BASE),     dump_register(TB_USED),
	dump_register(BNDR),	dump_register(BIND_LMT_0),
	dump_register(BIND_LMT_1),  dump_register(KA),
	dump_register(UNB_AGE),     dump_register(BND_AGE_0),
	dump_register(BND_AGE_1),   dump_register(HASH_SEED),
	dump_register(DFT_CPORT),   dump_register(MCAST_PPSE),
	dump_register(MCAST_L_0),   dump_register(MCAST_H_0),
	dump_register(MCAST_L_1),   dump_register(MCAST_H_1),
	dump_register(MCAST_L_2),   dump_register(MCAST_H_2),
	dump_register(MCAST_L_3),   dump_register(MCAST_H_3),
	dump_register(MCAST_L_4),   dump_register(MCAST_H_4),
	dump_register(MCAST_L_5),   dump_register(MCAST_H_5),
	dump_register(MCAST_L_6),   dump_register(MCAST_H_6),
	dump_register(MCAST_L_7),   dump_register(MCAST_H_7),
	dump_register(MCAST_L_8),   dump_register(MCAST_H_8),
	dump_register(MCAST_L_9),   dump_register(MCAST_H_9),
	dump_register(MCAST_L_A),   dump_register(MCAST_H_A),
	dump_register(MCAST_L_B),   dump_register(MCAST_H_B),
	dump_register(MCAST_L_C),   dump_register(MCAST_H_C),
	dump_register(MCAST_L_D),   dump_register(MCAST_H_D),
	dump_register(MCAST_L_E),   dump_register(MCAST_H_E),
	dump_register(MCAST_L_F),   dump_register(MCAST_H_F),
	dump_register(MTU_DRP),     dump_register(MTU_VLYR_0),
	dump_register(MTU_VLYR_1),  dump_register(MTU_VLYR_2),
	dump_register(VPM_TPID),    dump_register(VPM_TPID),
	dump_register(CAH_CTRL),    dump_register(CAH_TAG_SRH),
	dump_register(CAH_LINE_RW), dump_register(CAH_WDATA),
	dump_register(CAH_RDATA),
};

int __init hnat_init_debugfs(struct mtk_hnat *h)
{
	int ret = 0;
	struct dentry *root;
	struct dentry *file;
	long i;
	char name[16];

	root = debugfs_create_dir("hnat", NULL);
	if (!root) {
		dev_notice(h->dev, "%s:err at %d\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto err0;
	}
	h->root = root;
	h->regset = kzalloc(sizeof(*h->regset), GFP_KERNEL);
	if (!h->regset) {
		dev_notice(h->dev, "%s:err at %d\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto err1;
	}
	h->regset->regs = hnat_regs;
	h->regset->nregs = ARRAY_SIZE(hnat_regs);
	h->regset->base = h->ppe_base;

	file = debugfs_create_regset32("regdump", S_IRUGO, root, h->regset);
	if (!file) {
		dev_notice(h->dev, "%s:err at %d\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto err1;
	}
	debugfs_create_file("all_entry", S_IRUGO, root, h, &hnat_debug_fops);
	debugfs_create_file("external_interface", S_IRUGO, root, h,
			    &hnat_ext_fops);
	debugfs_create_file("whnat_interface", S_IRUGO, root, h,
			    &hnat_whnat_fops);
	debugfs_create_file("cpu_reason", S_IFREG | S_IRUGO, root, h,
			    &cpu_reason_fops);
	debugfs_create_file("hnat_entry", S_IRUGO | S_IRUGO, root, h,
			    &hnat_entry_fops);
	debugfs_create_file("hnat_setting", S_IRUGO | S_IRUGO, root, h,
			    &hnat_setting_fops);
	debugfs_create_file("mcast_table", S_IRUGO | S_IRUGO, root, h,
			    &hnat_mcast_fops);
	debugfs_create_file("hook_toggle", S_IRUGO | S_IRUGO, root, h,
			    &hnat_hook_toggle_fops);
	debugfs_create_file("mape_toggle", S_IRUGO | S_IRUGO, root, h,
			    &hnat_mape_toggle_fops);
	debugfs_create_file("hnat_version", S_IRUGO | S_IRUGO, root, h,
			    &hnat_version_fops);
#if defined(CONFIG_NET_MEDIATEK_HW_QOS)	
	debugfs_create_file("hqos_toggle", S_IRUGO | S_IRUGO, root, h,
				&hnat_hqos_toggle_fops);
#endif


	for (i = 0; i < hnat_priv->data->num_of_sch; i++) {
		snprintf(name, sizeof(name), "qdma_sch%ld", i);
		debugfs_create_file(name, S_IRUGO, root, (void *)i,
				    &hnat_sched_fops);
	}

	for (i = 0; i < MTK_QDMA_TX_NUM; i++) {
		snprintf(name, sizeof(name), "qdma_txq%ld", i);
		debugfs_create_file(name, S_IRUGO, root, (void *)i,
				    &hnat_queue_fops);
	}

	return 0;

err1:
	debugfs_remove_recursive(root);
err0:
	return ret;
}

void hnat_deinit_debugfs(struct mtk_hnat *h)
{
	debugfs_remove_recursive(h->root);
	h->root = NULL;
	kfree(h->regset);
}
