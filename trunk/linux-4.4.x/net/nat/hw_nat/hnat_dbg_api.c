/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2017 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <net/ra_nat.h>

#include "frame_engine.h"
#include "foe_fdb.h"
#include "hwnat_ioctl.h"
#include "util.h"
#include "api.h"
#include "hwnat_config.h"
#include "hwnat_define.h"
#include "hnat_dbg_proc.h"

#define DD \
{\
pr_info("%s %d\n", __func__, __LINE__); \
}

void dbg_dump_entry(uint32_t index)
{
	struct foe_entry *entry = &ppe_foe_base[index];

	if (IS_IPV4_HNAPT(entry)) {
		NAT_PRINT
		    ("NAPT(%d): %u.%u.%u.%u:%d->%u.%u.%u.%u:%d ->", index,
		     IP_FORMAT3(entry->ipv4_hnapt.sip),
		     IP_FORMAT2(entry->ipv4_hnapt.sip),
		     IP_FORMAT1(entry->ipv4_hnapt.sip),
		     IP_FORMAT0(entry->ipv4_hnapt.sip),
		     entry->ipv4_hnapt.sport,
		     IP_FORMAT3(entry->ipv4_hnapt.dip), IP_FORMAT2(entry->ipv4_hnapt.dip),
		     IP_FORMAT1(entry->ipv4_hnapt.dip), IP_FORMAT0(entry->ipv4_hnapt.dip),
		     entry->ipv4_hnapt.dport);
		NAT_PRINT
		    (" %u.%u.%u.%u:%d->%u.%u.%u.%u:%d\n",
		     IP_FORMAT3(entry->ipv4_hnapt.new_sip),
		     IP_FORMAT2(entry->ipv4_hnapt.new_sip),
		     IP_FORMAT1(entry->ipv4_hnapt.new_sip),
		     IP_FORMAT0(entry->ipv4_hnapt.new_sip), entry->ipv4_hnapt.new_sport,
		     IP_FORMAT3(entry->ipv4_hnapt.new_dip),
		     IP_FORMAT2(entry->ipv4_hnapt.new_dip),
		     IP_FORMAT1(entry->ipv4_hnapt.new_dip),
		     IP_FORMAT0(entry->ipv4_hnapt.new_dip), entry->ipv4_hnapt.new_dport);
	} else if (IS_IPV4_HNAT(entry)) {
		NAT_PRINT("NAT(%d): %u.%u.%u.%u->%u.%u.%u.%u ->", index,
			  IP_FORMAT3(entry->ipv4_hnapt.sip),
			  IP_FORMAT2(entry->ipv4_hnapt.sip),
			  IP_FORMAT1(entry->ipv4_hnapt.sip),
			  IP_FORMAT0(entry->ipv4_hnapt.sip),
			  IP_FORMAT3(entry->ipv4_hnapt.dip),
			  IP_FORMAT2(entry->ipv4_hnapt.dip),
			  IP_FORMAT1(entry->ipv4_hnapt.dip), IP_FORMAT0(entry->ipv4_hnapt.dip));
		NAT_PRINT(" %u.%u.%u.%u->%u.%u.%u.%u\n",
			  IP_FORMAT3(entry->ipv4_hnapt.new_sip),
			  IP_FORMAT2(entry->ipv4_hnapt.new_sip),
			  IP_FORMAT1(entry->ipv4_hnapt.new_sip),
			  IP_FORMAT0(entry->ipv4_hnapt.new_sip),
			  IP_FORMAT3(entry->ipv4_hnapt.new_dip),
			  IP_FORMAT2(entry->ipv4_hnapt.new_dip),
			  IP_FORMAT1(entry->ipv4_hnapt.new_dip),
			  IP_FORMAT0(entry->ipv4_hnapt.new_dip));
	}

	if (IS_IPV6_1T_ROUTE(entry)) {
		NAT_PRINT("IPv6_1T(%d): %08X:%08X:%08X:%08X\n", index,
			  entry->ipv6_1t_route.ipv6_dip3,
			  entry->ipv6_1t_route.ipv6_dip2,
			  entry->ipv6_1t_route.ipv6_dip1, entry->ipv6_1t_route.ipv6_dip0);
	} else if (IS_IPV4_DSLITE(entry)) {
		NAT_PRINT
		    ("IPv4 Ds-Lite(%d): %u.%u.%u.%u.%d->%u.%u.%u.%u:%d ->", index,
		     IP_FORMAT3(entry->ipv4_dslite.sip),
		     IP_FORMAT2(entry->ipv4_dslite.sip),
		     IP_FORMAT1(entry->ipv4_dslite.sip),
		     IP_FORMAT0(entry->ipv4_dslite.sip), entry->ipv4_dslite.sport,
		     IP_FORMAT3(entry->ipv4_dslite.dip),
		     IP_FORMAT2(entry->ipv4_dslite.dip),
		     IP_FORMAT1(entry->ipv4_dslite.dip),
		     IP_FORMAT0(entry->ipv4_dslite.dip), entry->ipv4_dslite.dport);
		NAT_PRINT(" %08X:%08X:%08X:%08X->%08X:%08X:%08X:%08X\n",
			  entry->ipv4_dslite.tunnel_sipv6_0,
			  entry->ipv4_dslite.tunnel_sipv6_1,
			  entry->ipv4_dslite.tunnel_sipv6_2,
			  entry->ipv4_dslite.tunnel_sipv6_3,
			  entry->ipv4_dslite.tunnel_dipv6_0,
			  entry->ipv4_dslite.tunnel_dipv6_1,
			  entry->ipv4_dslite.tunnel_dipv6_2, entry->ipv4_dslite.tunnel_dipv6_3);
	} else if (IS_IPV6_3T_ROUTE(entry)) {
		NAT_PRINT
		    ("IPv6_3T(%d): %08X:%08X:%08X:%08X-> %08X:%08X:%08X:%08X (Prot=%d)\n",
		     index,
		     entry->ipv6_3t_route.ipv6_sip0,
		     entry->ipv6_3t_route.ipv6_sip1,
		     entry->ipv6_3t_route.ipv6_sip2,
		     entry->ipv6_3t_route.ipv6_sip3,
		     entry->ipv6_3t_route.ipv6_dip0,
		     entry->ipv6_3t_route.ipv6_dip1,
		     entry->ipv6_3t_route.ipv6_dip2,
		     entry->ipv6_3t_route.ipv6_dip3, entry->ipv6_3t_route.prot);
	} else if (IS_IPV6_5T_ROUTE(entry)) {
		if (IS_IPV6_FLAB_EBL()) {
			NAT_PRINT
			    ("IPv6_5T(%d): %08X:%08X:%08X:%08X-> %08X:%08X:%08X:%08X",
			     index,
			     entry->ipv6_5t_route.ipv6_sip0,
			     entry->ipv6_5t_route.ipv6_sip1,
			     entry->ipv6_5t_route.ipv6_sip2,
			     entry->ipv6_5t_route.ipv6_sip3,
			     entry->ipv6_5t_route.ipv6_dip0,
			     entry->ipv6_5t_route.ipv6_dip1,
			     entry->ipv6_5t_route.ipv6_dip2, entry->ipv6_5t_route.ipv6_dip3);
			NAT_PRINT("(Flow Label=%08X)\n",
				  ((entry->ipv6_5t_route.
				    sport << 16) | (entry->ipv6_5t_route.dport)) & 0xFFFFF);
		} else {
			NAT_PRINT
			    ("IPv6_5T(%d): %08X:%08X:%08X:%08X:%d-> ",
			     index,
			     entry->ipv6_5t_route.ipv6_sip0,
			     entry->ipv6_5t_route.ipv6_sip1,
			     entry->ipv6_5t_route.ipv6_sip2,
			     entry->ipv6_5t_route.ipv6_sip3, entry->ipv6_5t_route.sport);
			NAT_PRINT("%08X:%08X:%08X:%08X:%d\n",
				  entry->ipv6_5t_route.ipv6_dip0,
				  entry->ipv6_5t_route.ipv6_dip1,
				  entry->ipv6_5t_route.ipv6_dip2,
				  entry->ipv6_5t_route.ipv6_dip3, entry->ipv6_5t_route.dport);
		}
	} else if (IS_IPV6_6RD(entry)) {
		if (IS_IPV6_FLAB_EBL()) {
			NAT_PRINT
			    ("IPv6_6RD(%d): %08X:%08X:%08X:%08X-> %08X:%08X:%08X:%08X",
			     index,
			     entry->ipv6_6rd.ipv6_sip0, entry->ipv6_6rd.ipv6_sip1,
			     entry->ipv6_6rd.ipv6_sip2, entry->ipv6_6rd.ipv6_sip3,
			     entry->ipv6_6rd.ipv6_dip0, entry->ipv6_6rd.ipv6_dip1,
			     entry->ipv6_6rd.ipv6_dip2, entry->ipv6_6rd.ipv6_dip3);
			NAT_PRINT("(Flow Label=%08X)\n",
				  ((entry->ipv6_5t_route.
				    sport << 16) | (entry->ipv6_5t_route.dport)) & 0xFFFFF);
		} else {
			NAT_PRINT
			    ("IPv6_6RD(%d): %08X:%08X:%08X:%08X:%d-> ",
			     index,
			     entry->ipv6_6rd.ipv6_sip0, entry->ipv6_6rd.ipv6_sip1,
			     entry->ipv6_6rd.ipv6_sip2, entry->ipv6_6rd.ipv6_sip3,
			     entry->ipv6_6rd.sport);
			NAT_PRINT(" %08X:%08X:%08X:%08X:%d\n", entry->ipv6_6rd.ipv6_dip0,
				  entry->ipv6_6rd.ipv6_dip1, entry->ipv6_6rd.ipv6_dip2,
				  entry->ipv6_6rd.ipv6_dip3, entry->ipv6_6rd.dport);
		}
	}
}

void dbg_dump_cr(void)
{
	unsigned int cr_base;
	int i;
	int cr_max;

	if (hnat_chip_name & (MT7622_HWNAT | LEOPARD_HWNAT))
		cr_base = 0x1B100C00;
	else
		cr_base = 0x1B100E00;
	cr_max = 319 * 4;
	for (i = 0; i < cr_max; i = i + 0x10) {
		pr_info("0x%08x : 0x%08x 0x%08x 0x%08x 0x%08x\n", cr_base + i,
			reg_read(PPE_MCAST_L_10 + i), reg_read(PPE_MCAST_L_10 + i + 4),
			reg_read(PPE_MCAST_L_10 + i + 8), reg_read(PPE_MCAST_L_10 + i + 0xc));
	}
}
