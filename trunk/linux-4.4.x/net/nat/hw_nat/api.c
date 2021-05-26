/* Copyright  2017 MediaTek Inc.
 * Author: Harry Huang <harry.huang@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/skbuff.h>

#include "ra_nat.h"
#include "foe_fdb.h"
#include "frame_engine.h"
#include "sys_rfrw.h"
#include "util.h"
#include "hwnat_ioctl.h"
#include "api.h"
#include "hwnat_define.h"

extern struct foe_entry		*ppe_foe_base;

#if defined(CONFIG_RA_HW_NAT_IPV6)
int hash_ipv6(struct foe_pri_key *key, struct foe_entry *entry, int del)
{
	uint32_t t_hvt_31, t_hvt_63, t_hvt_95, t_hvt_sd;
	uint32_t t_hvt_sd_23, t_hvt_sd_31_24, t_hash_32, t_hashs_16, t_ha16k, hash_index;	
	uint32_t ppeSaddr_127_96, ppeSaddr_95_64, ppeSaddr_63_32, ppeSaddr_31_0;
	uint32_t ppeDaddr_127_96, ppeDaddr_95_64, ppeDaddr_63_32, ppeDaddr_31_0;
	uint32_t ipv6_sip_127_96, ipv6_sip_95_64, ipv6_sip_63_32, ipv6_sip_31_0;
	uint32_t ipv6_dip_127_96, ipv6_dip_95_64, ipv6_dip_63_32, ipv6_dip_31_0;
	uint32_t sport, dport, ppeSportV6, ppeDportV6;
	
	ipv6_sip_127_96 = key->ipv6_routing.sip0;
	ipv6_sip_95_64 = key->ipv6_routing.sip1;
	ipv6_sip_63_32 = key->ipv6_routing.sip2;
	ipv6_sip_31_0 = key->ipv6_routing.sip3;
	ipv6_dip_127_96 = key->ipv6_routing.dip0;
	ipv6_dip_95_64 = key->ipv6_routing.dip1;
	ipv6_dip_63_32 = key->ipv6_routing.dip2;
	ipv6_dip_31_0 = key->ipv6_routing.dip3;
	sport = key->ipv6_routing.sport;
	dport = key->ipv6_routing.dport;

	t_hvt_31 = ipv6_sip_31_0 ^ ipv6_dip_31_0 ^ (sport << 16 | dport);
	t_hvt_63 = ipv6_sip_63_32 ^ ipv6_dip_63_32 ^ ipv6_dip_127_96;
	t_hvt_95 = ipv6_sip_95_64 ^ ipv6_dip_95_64 ^ ipv6_sip_127_96;
	if (DFL_FOE_HASH_MODE == 1)	// hash mode 1
		t_hvt_sd = (t_hvt_31 & t_hvt_63) | ((~t_hvt_31) & t_hvt_95);
	else                            // hash mode 2
		t_hvt_sd = t_hvt_63 ^ ( t_hvt_31 & (~t_hvt_95));
			
	t_hvt_sd_23 = t_hvt_sd & 0xffffff;
	t_hvt_sd_31_24 = t_hvt_sd & 0xff000000;
	t_hash_32 = t_hvt_31 ^ t_hvt_63 ^ t_hvt_95 ^ ( (t_hvt_sd_23 << 8) | (t_hvt_sd_31_24 >> 24));
	t_hashs_16 = ((t_hash_32 & 0xffff0000) >> 16 ) ^ (t_hash_32 & 0xfffff);

	if (FOE_4TB_SIZ == 16384)
		t_ha16k = t_hashs_16 & 0x1fff;  // FOE_16k
	else if (FOE_4TB_SIZ == 8192)
		t_ha16k = t_hashs_16 & 0xfff;  // FOE_8k
	else if (FOE_4TB_SIZ == 4096)
		t_ha16k = t_hashs_16 & 0x7ff;  // FOE_4k
	else if (FOE_4TB_SIZ == 2048)
		t_ha16k = t_hashs_16 & 0x3ff;  // FOE_2k
	else
		t_ha16k = t_hashs_16 & 0x1ff;  // FOE_1k
	hash_index = (uint32_t)t_ha16k *2;

	entry = &ppe_foe_base[hash_index];
	ppeSaddr_127_96 = entry->ipv6_5t_route.ipv6_sip0;
	ppeSaddr_95_64 = entry->ipv6_5t_route.ipv6_sip1;
	ppeSaddr_63_32 = entry->ipv6_5t_route.ipv6_sip2;
	ppeSaddr_31_0 = entry->ipv6_5t_route.ipv6_sip3;
		
	ppeDaddr_127_96 = entry->ipv6_5t_route.ipv6_dip0;
	ppeDaddr_95_64 = entry->ipv6_5t_route.ipv6_dip1;
	ppeDaddr_63_32 = entry->ipv6_5t_route.ipv6_dip2;
	ppeDaddr_31_0 = entry->ipv6_5t_route.ipv6_dip3;
		
	ppeSportV6 = entry->ipv6_5t_route.sport;
	ppeDportV6 = entry->ipv6_5t_route.dport;
	if (del !=1) {
		if (entry->ipv6_5t_route.bfib1.state == BIND)
		{
			printk("IPV6 Hash collision, hash index +1\n");
			hash_index = hash_index + 1;
			entry = &ppe_foe_base[hash_index];
		}
		if (entry->ipv6_5t_route.bfib1.state == BIND)
		{
			printk("IPV6 Hash collision can not bind\n");
			return -1;
		}
	}else if(del == 1) {
		if ((ipv6_sip_127_96 == ppeSaddr_127_96) && (ipv6_sip_95_64 == ppeSaddr_95_64) 
			&& (ipv6_sip_63_32 == ppeSaddr_63_32) && (ipv6_sip_31_0 == ppeSaddr_31_0) && 
			(ipv6_dip_127_96 == ppeDaddr_127_96) && (ipv6_dip_95_64 == ppeDaddr_95_64) 
			&& (ipv6_dip_63_32 == ppeDaddr_63_32) && (ipv6_dip_31_0 == ppeDaddr_31_0) &&
			(sport == ppeSportV6) && (dport == ppeDportV6)) {
		} else {
			hash_index = hash_index + 1;
			entry = &ppe_foe_base[hash_index];
			ppeSaddr_127_96 = entry->ipv6_5t_route.ipv6_sip0;
			ppeSaddr_95_64 = entry->ipv6_5t_route.ipv6_sip1;
			ppeSaddr_63_32 = entry->ipv6_5t_route.ipv6_sip2;
			ppeSaddr_31_0 = entry->ipv6_5t_route.ipv6_sip3;
			
			ppeDaddr_127_96 = entry->ipv6_5t_route.ipv6_dip0;
			ppeDaddr_95_64 = entry->ipv6_5t_route.ipv6_dip1;
			ppeDaddr_63_32 = entry->ipv6_5t_route.ipv6_dip2;
			ppeDaddr_31_0 = entry->ipv6_5t_route.ipv6_dip3;
			
			ppeSportV6 = entry->ipv6_5t_route.sport;
			ppeDportV6 = entry->ipv6_5t_route.dport;
			if ((ipv6_sip_127_96 == ppeSaddr_127_96) && (ipv6_sip_95_64 == ppeSaddr_95_64) 
				&& (ipv6_sip_63_32 == ppeSaddr_63_32) && (ipv6_sip_31_0 == ppeSaddr_31_0) && 
				(ipv6_dip_127_96 == ppeDaddr_127_96) && (ipv6_dip_95_64 == ppeDaddr_95_64) 
				&& (ipv6_dip_63_32 == ppeDaddr_63_32) && (ipv6_dip_31_0 == ppeDaddr_31_0) &&
				(sport == ppeSportV6) && (dport == ppeDportV6)) {
			} else {
#if defined (CONFIG_HW_NAT_SEMI_AUTO_MODE)
				printk("Ipv6 Entry delete : Entry Not found\n");
#elif defined (CONFIG_HW_NAT_MANUAL_MODE)
				printk("Ipv6 hash collision hwnat can not found\n");
#endif
				return -1;
			}
		}

	}
	return hash_index;
}

int hash_mib_ipv6(struct foe_pri_key *key, struct foe_entry *entry)
{
	uint32_t t_hvt_31, t_hvt_63, t_hvt_95, t_hvt_sd;
	uint32_t t_hvt_sd_23, t_hvt_sd_31_24, t_hash_32, t_hashs_16, t_ha16k, hash_index;	
	uint32_t ppeSaddr_127_96, ppeSaddr_95_64, ppeSaddr_63_32, ppeSaddr_31_0;
	uint32_t ppeDaddr_127_96, ppeDaddr_95_64, ppeDaddr_63_32, ppeDaddr_31_0;
	uint32_t ipv6_sip_127_96, ipv6_sip_95_64, ipv6_sip_63_32, ipv6_sip_31_0;
	uint32_t ipv6_dip_127_96, ipv6_dip_95_64, ipv6_dip_63_32, ipv6_dip_31_0;
	uint32_t sport, dport, ppeSportV6, ppeDportV6;
	
	ipv6_sip_127_96 = key->ipv6_routing.sip0;
	ipv6_sip_95_64 = key->ipv6_routing.sip1;
	ipv6_sip_63_32 = key->ipv6_routing.sip2;
	ipv6_sip_31_0 = key->ipv6_routing.sip3;
	ipv6_dip_127_96 = key->ipv6_routing.dip0;
	ipv6_dip_95_64 = key->ipv6_routing.dip1;
	ipv6_dip_63_32 = key->ipv6_routing.dip2;
	ipv6_dip_31_0 = key->ipv6_routing.dip3;
	sport = key->ipv6_routing.sport;
	dport = key->ipv6_routing.dport;

	t_hvt_31 = ipv6_sip_31_0 ^ ipv6_dip_31_0 ^ (sport << 16 | dport);
	t_hvt_63 = ipv6_sip_63_32 ^ ipv6_dip_63_32 ^ ipv6_dip_127_96;
	t_hvt_95 = ipv6_sip_95_64 ^ ipv6_dip_95_64 ^ ipv6_sip_127_96;
	if (DFL_FOE_HASH_MODE == 1)	// hash mode 1
		t_hvt_sd = (t_hvt_31 & t_hvt_63) | ((~t_hvt_31) & t_hvt_95);
	else                            // hash mode 2
		t_hvt_sd = t_hvt_63 ^ ( t_hvt_31 & (~t_hvt_95));
			
	t_hvt_sd_23 = t_hvt_sd & 0xffffff;
	t_hvt_sd_31_24 = t_hvt_sd & 0xff000000;
	t_hash_32 = t_hvt_31 ^ t_hvt_63 ^ t_hvt_95 ^ ( (t_hvt_sd_23 << 8) | (t_hvt_sd_31_24 >> 24));
	t_hashs_16 = ((t_hash_32 & 0xffff0000) >> 16 ) ^ (t_hash_32 & 0xfffff);

	if (FOE_4TB_SIZ == 16384)
		t_ha16k = t_hashs_16 & 0x1fff;  // FOE_16k
	else if (FOE_4TB_SIZ == 8192)
		t_ha16k = t_hashs_16 & 0xfff;  // FOE_8k
	else if (FOE_4TB_SIZ == 4096)
		t_ha16k = t_hashs_16 & 0x7ff;  // FOE_4k
	else if (FOE_4TB_SIZ == 2048)
		t_ha16k = t_hashs_16 & 0x3ff;  // FOE_2k
	else
		t_ha16k = t_hashs_16 & 0x1ff;  // FOE_1k
	hash_index = (uint32_t)t_ha16k *2;

	entry = &ppe_foe_base[hash_index];
	ppeSaddr_127_96 = entry->ipv6_5t_route.ipv6_sip0;
	ppeSaddr_95_64 = entry->ipv6_5t_route.ipv6_sip1;
	ppeSaddr_63_32 = entry->ipv6_5t_route.ipv6_sip2;
	ppeSaddr_31_0 = entry->ipv6_5t_route.ipv6_sip3;
		
	ppeDaddr_127_96 = entry->ipv6_5t_route.ipv6_dip0;
	ppeDaddr_95_64 = entry->ipv6_5t_route.ipv6_dip1;
	ppeDaddr_63_32 = entry->ipv6_5t_route.ipv6_dip2;
	ppeDaddr_31_0 = entry->ipv6_5t_route.ipv6_dip3;
		
	ppeSportV6 = entry->ipv6_5t_route.sport;
	ppeDportV6 = entry->ipv6_5t_route.dport;

	if ((ipv6_sip_127_96 == ppeSaddr_127_96) && (ipv6_sip_95_64 == ppeSaddr_95_64) 
		&& (ipv6_sip_63_32 == ppeSaddr_63_32) && (ipv6_sip_31_0 == ppeSaddr_31_0) && 
		(ipv6_dip_127_96 == ppeDaddr_127_96) && (ipv6_dip_95_64 == ppeDaddr_95_64) 
		&& (ipv6_dip_63_32 == ppeDaddr_63_32) && (ipv6_dip_31_0 == ppeDaddr_31_0) &&
		(sport == ppeSportV6) && (dport == ppeDportV6)) {
		return hash_index;
	} else {
			hash_index = hash_index + 1;
			entry = &ppe_foe_base[hash_index];
			ppeSaddr_127_96 = entry->ipv6_5t_route.ipv6_sip0;
			ppeSaddr_95_64 = entry->ipv6_5t_route.ipv6_sip1;
			ppeSaddr_63_32 = entry->ipv6_5t_route.ipv6_sip2;
			ppeSaddr_31_0 = entry->ipv6_5t_route.ipv6_sip3;
			
			ppeDaddr_127_96 = entry->ipv6_5t_route.ipv6_dip0;
			ppeDaddr_95_64 = entry->ipv6_5t_route.ipv6_dip1;
			ppeDaddr_63_32 = entry->ipv6_5t_route.ipv6_dip2;
			ppeDaddr_31_0 = entry->ipv6_5t_route.ipv6_dip3;
			
			ppeSportV6 = entry->ipv6_5t_route.sport;
			ppeDportV6 = entry->ipv6_5t_route.dport;
			if ((ipv6_sip_127_96 == ppeSaddr_127_96) && (ipv6_sip_95_64 == ppeSaddr_95_64) 
				&& (ipv6_sip_63_32 == ppeSaddr_63_32) && (ipv6_sip_31_0 == ppeSaddr_31_0) && 
				(ipv6_dip_127_96 == ppeDaddr_127_96) && (ipv6_dip_95_64 == ppeDaddr_95_64) 
				&& (ipv6_dip_63_32 == ppeDaddr_63_32) && (ipv6_dip_31_0 == ppeDaddr_31_0) &&
				(sport == ppeSportV6) && (dport == ppeDportV6)) {
					return hash_index;
			} else {
				if (debug_level >= 1)
					pr_info("mib: ipv6 entry not found\n");
				return -1;
			}
	}

	
	return -1;
}
#endif

int hash_ipv4(struct foe_pri_key *key, struct foe_entry *entry, int del)
{
	uint32_t t_hvt_31;
	uint32_t t_hvt_63;
	uint32_t t_hvt_95;
	uint32_t t_hvt_sd;

	uint32_t t_hvt_sd_23;
	uint32_t t_hvt_sd_31_24;
	uint32_t t_hash_32;
	uint32_t t_hashs_16; 
	uint32_t t_ha16k;
	uint32_t hash_index;	
	uint32_t ppeSaddr, ppeDaddr, ppeSport, ppeDport, saddr, daddr, sport, dport;
        saddr = key->ipv4_hnapt.sip;
        daddr = key->ipv4_hnapt.dip;
        sport = key->ipv4_hnapt.sport;
        dport = key->ipv4_hnapt.dport;

	t_hvt_31 = sport << 16 | dport;
	t_hvt_63 = daddr;
	t_hvt_95 = saddr;
	
	//printk("saddr = %x, daddr=%x, sport=%d, dport=%d\n", saddr, daddr, sport, dport);
	if (DFL_FOE_HASH_MODE == 1)	// hash mode 1
		t_hvt_sd = (t_hvt_31 & t_hvt_63) | ((~t_hvt_31) & t_hvt_95);
	else                            // hash mode 2
		t_hvt_sd = t_hvt_63 ^ ( t_hvt_31 & (~t_hvt_95));
			
	t_hvt_sd_23 = t_hvt_sd & 0xffffff;
	t_hvt_sd_31_24 = t_hvt_sd & 0xff000000;
	t_hash_32 = t_hvt_31 ^ t_hvt_63 ^ t_hvt_95 ^ ( (t_hvt_sd_23 << 8) | (t_hvt_sd_31_24 >> 24));
	t_hashs_16 = ((t_hash_32 & 0xffff0000) >> 16 ) ^ (t_hash_32 & 0xfffff);

	if (FOE_4TB_SIZ == 16384)
		t_ha16k = t_hashs_16 & 0x1fff;  // FOE_16k
	else if (FOE_4TB_SIZ == 8192)
		t_ha16k = t_hashs_16 & 0xfff;  // FOE_8k
	else if (FOE_4TB_SIZ == 4096)
		t_ha16k = t_hashs_16 & 0x7ff;  // FOE_4k
	else if (FOE_4TB_SIZ == 2048)
		t_ha16k = t_hashs_16 & 0x3ff;  // FOE_2k
	else
		t_ha16k = t_hashs_16 & 0x1ff;  // FOE_1k
	hash_index = (uint32_t)t_ha16k *2;

	entry = &ppe_foe_base[hash_index];
	ppeSaddr = entry->ipv4_hnapt.sip;
	ppeDaddr = entry->ipv4_hnapt.dip;
	ppeSport = entry->ipv4_hnapt.sport;
	ppeDport = entry->ipv4_hnapt.dport;

	if (del !=1) {
		if (entry->ipv4_hnapt.bfib1.state == BIND)
		{
			printk("Hash collision, hash index +1\n");
			hash_index = hash_index + 1;
			entry = &ppe_foe_base[hash_index];
		}
		if (entry->ipv4_hnapt.bfib1.state == BIND)
		{
			printk("Hash collision can not bind\n");
			return -1;
		}
	} else if(del == 1) {
		if ((saddr == ppeSaddr) && (daddr == ppeDaddr) && (sport == ppeSport)
			&& (dport == ppeDport)){
		} else {
			hash_index = hash_index + 1;
			entry = &ppe_foe_base[hash_index];
			ppeSaddr = entry->ipv4_hnapt.sip;
			ppeDaddr = entry->ipv4_hnapt.dip;
			ppeSport = entry->ipv4_hnapt.sport;
			ppeDport = entry->ipv4_hnapt.dport;
			if ((saddr == ppeSaddr) && (daddr == ppeDaddr) && (sport == ppeSport)
				&& (dport == ppeDport)){
			} else {
#if defined (CONFIG_HW_NAT_SEMI_AUTO_MODE)
				printk("hash collision hwnat can not foundn");	
#elif defined (CONFIG_HW_NAT_MANUAL_MODE)
				printk("Entry delete : Entry Not found\n");							
#endif
				return -1;
			}			
		}
	}
	return hash_index;
}

int hash_mib_ipv4(struct foe_pri_key *key, struct foe_entry *entry)
{
	uint32_t t_hvt_31;
	uint32_t t_hvt_63;
	uint32_t t_hvt_95;
	uint32_t t_hvt_sd;

	uint32_t t_hvt_sd_23;
	uint32_t t_hvt_sd_31_24;
	uint32_t t_hash_32;
	uint32_t t_hashs_16; 
	uint32_t t_ha16k;
	uint32_t hash_index;	
	uint32_t ppeSaddr, ppeDaddr, ppeSport, ppeDport, saddr, daddr, sport, dport;
        saddr = key->ipv4_hnapt.sip;
        daddr = key->ipv4_hnapt.dip;
        sport = key->ipv4_hnapt.sport;
        dport = key->ipv4_hnapt.dport;

	t_hvt_31 = sport << 16 | dport;
	t_hvt_63 = daddr;
	t_hvt_95 = saddr;
	
	//printk("saddr = %x, daddr=%x, sport=%d, dport=%d\n", saddr, daddr, sport, dport);
	if (DFL_FOE_HASH_MODE == 1)	// hash mode 1
		t_hvt_sd = (t_hvt_31 & t_hvt_63) | ((~t_hvt_31) & t_hvt_95);
	else                            // hash mode 2
		t_hvt_sd = t_hvt_63 ^ ( t_hvt_31 & (~t_hvt_95));
			
	t_hvt_sd_23 = t_hvt_sd & 0xffffff;
	t_hvt_sd_31_24 = t_hvt_sd & 0xff000000;
	t_hash_32 = t_hvt_31 ^ t_hvt_63 ^ t_hvt_95 ^ ( (t_hvt_sd_23 << 8) | (t_hvt_sd_31_24 >> 24));
	t_hashs_16 = ((t_hash_32 & 0xffff0000) >> 16 ) ^ (t_hash_32 & 0xfffff);

	if (FOE_4TB_SIZ == 16384)
		t_ha16k = t_hashs_16 & 0x1fff;  // FOE_16k
	else if (FOE_4TB_SIZ == 8192)
		t_ha16k = t_hashs_16 & 0xfff;  // FOE_8k
	else if (FOE_4TB_SIZ == 4096)
		t_ha16k = t_hashs_16 & 0x7ff;  // FOE_4k
	else if (FOE_4TB_SIZ == 2048)
		t_ha16k = t_hashs_16 & 0x3ff;  // FOE_2k
	else
		t_ha16k = t_hashs_16 & 0x1ff;  // FOE_1k
	hash_index = (uint32_t)t_ha16k *2;

	entry = &ppe_foe_base[hash_index];
	ppeSaddr = entry->ipv4_hnapt.sip;
	ppeDaddr = entry->ipv4_hnapt.dip;
	ppeSport = entry->ipv4_hnapt.sport;
	ppeDport = entry->ipv4_hnapt.dport;

	

	if ((saddr == ppeSaddr) && (daddr == ppeDaddr) && (sport == ppeSport)
		&& (dport == ppeDport)){
			return hash_index;
	} else {
			hash_index = hash_index + 1;
			entry = &ppe_foe_base[hash_index];
			ppeSaddr = entry->ipv4_hnapt.sip;
			ppeDaddr = entry->ipv4_hnapt.dip;
			ppeSport = entry->ipv4_hnapt.sport;
			ppeDport = entry->ipv4_hnapt.dport;
			if ((saddr == ppeSaddr) && (daddr == ppeDaddr) && (sport == ppeSport)
				&& (dport == ppeDport)){
				return hash_index;
			} else {
				if (debug_level >= 1)
					pr_info("mib: ipv4 entry not found\n");
				return -1;
			}			
	}
	
	return -1;
}

int GetPpeEntryIdx(struct foe_pri_key *key, struct foe_entry *entry, int del)
{
	if ((key->pkt_type) == IPV4_NAPT)
		return hash_ipv4(key, entry, del);
#if defined(CONFIG_RA_HW_NAT_IPV6)
	else if((key->pkt_type) == IPV6_ROUTING)
		return hash_ipv6(key, entry, del);
#endif
	else
		return -1;
}

int get_mib_entry_idx(struct foe_pri_key *key, struct foe_entry *entry)
{
	if ((key->pkt_type) == IPV4_NAPT)
		return hash_mib_ipv4(key, entry);
#if defined(CONFIG_RA_HW_NAT_IPV6)
	else if((key->pkt_type) == IPV6_ROUTING)
		return hash_mib_ipv6(key, entry);
#endif
	else
		return -1;
}
EXPORT_SYMBOL(get_mib_entry_idx);


