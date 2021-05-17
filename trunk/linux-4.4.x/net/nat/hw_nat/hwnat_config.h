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
#ifndef _HNAT_CONFIG_WANTED
#define _HNAT_CONFIG_WANTED

#include "raeth_config.h"

#if defined(CONFIG_ARCH_MT7622)
#define USE_UDP_FRAG
#endif

#ifdef CONFIG_RALINK_MT7620
#define MT7620_HWNAT	BIT(0)
#else
#define MT7620_HWNAT	(0)
#endif

#ifdef CONFIG_RALINK_MT7621
#define MT7621_HWNAT	BIT(1)
#else
#define MT7621_HWNAT	(0)
#endif
#ifdef CONFIG_ARCH_MT7622
#define MT7622_HWNAT	BIT(2)
#else
#define MT7622_HWNAT	(0)
#endif
#ifdef CONFIG_ARCH_MT7623
#define MT7623_HWNAT	BIT(3)
#else
#define MT7623_HWNAT	(0)
#endif
#ifdef CONFIG_MACH_LEOPARD
#define LEOPARD_HWNAT	BIT(4)
#else
#define LEOPARD_HWNAT	(0)
#endif

#ifdef	CONFIG_RAETH_GMAC2
#define GE2_SUPPORT	BIT(0)
#else
#define GE2_SUPPORT	(0)
#endif

#ifdef	CONFIG_RA_HW_NAT_IPV6
#define HNAT_IPV6	BIT(1)
#else
#define HNAT_IPV6	(0)
#endif

#ifdef	CONFIG_RAETH_HW_VLAN_TX
#define HNAT_VLAN_TX	BIT(2)
#else
#define HNAT_VLAN_TX	(0)
#endif

#ifdef	CONFIG_PPE_MCAST
#define HNAT_MCAST	BIT(3)
#else
#define HNAT_MCAST	(0)
#endif

#ifdef	CONFIG_RAETH_QDMA
#define HNAT_QDMA	BIT(4)
#else
#define HNAT_QDMA	(0)
#endif

#ifdef	CONFIG_ARCH_MT7622_WIFI_HW_NAT
#define WARP_WHNAT	BIT(5)
#else
#define WARP_WHNAT	(0)
#endif

#ifdef	CONFIG_RA_HW_NAT_WIFI
#define WIFI_HNAT	BIT(6)
#else
#define WIFI_HNAT	(0)
#endif

#ifdef	CONFIG_RA_HW_NAT_WIFI_NEW_ARCH
#define WIFI_HNAT	BIT(6)
#else
#define WIFI_HNAT	(0)
#endif

#ifdef	CONFIG_WAN_AT_P4
#define HNAT_WAN_P4	BIT(7)
#else
#define HNAT_WAN_P4	(0)
#endif

#ifdef	CONFIG_WAN_TO_WLAN_SUPPORT_QOS
#define WAN_TO_WLAN_QOS	BIT(8)
#else
#define WAN_TO_WLAN_QOS		(0)
#endif

#ifdef	CONFIG_RAETH_SPECIAL_TAG
#define HNAT_SP_TAG	BIT(9)
#else
#define HNAT_SP_TAG		(0)
#endif

#ifdef CONFIG_RAETH_QDMATX_QDMARX
#define QDMA_TX_RX	BIT(10)
#else
#define QDMA_TX_RX		(0)
#endif

#ifdef CONFIG_PPE_MIB
#define PPE_MIB	BIT(11)
#else
#define PPE_MIB		(0)
#endif

#ifdef CONFIG_RA_HW_NAT_PACKET_SAMPLING
#define PACKET_SAMPLING	BIT(12)
#else
#define PACKET_SAMPLING		(0)
#endif

#if 0
#define HNAT_OPENWRT	BIT(13)
#else
#define HNAT_OPENWRT		(0)
#endif

#ifdef CONFIG_SUPPORT_WLAN_QOS
#define HNAT_WLAN_QOS	BIT(14)
#else
#define HNAT_WLAN_QOS		(0)
#endif

#ifdef CONFIG_SUPPORT_WLAN_OPTIMIZE
#define WLAN_OPTIMIZE	BIT(15)
#else
#define WLAN_OPTIMIZE		(0)
#endif

#ifdef USE_UDP_FRAG
#define UDP_FRAG	BIT(16)
#else
#define UDP_FRAG		(0)
#endif

#ifdef CONFIG_HW_NAT_AUTO_MODE
#define AUTO_MODE	BIT(17)
#else
#define AUTO_MODE		(0)
#endif

#ifdef CONFIG_HW_NAT_SEMI_AUTO_MODE
#define SEMI_AUTO_MODE	BIT(18)
#else
#define SEMI_AUTO_MODE		(0)
#endif

#ifdef CONFIG_HW_NAT_MANUAL_MODE
#define MANUAL_MODE	BIT(19)
#else
#define MANUAL_MODE		(0)
#endif

#ifdef CONFIG_RA_HW_NAT_PREBIND
#define PRE_BIND	BIT(20)
#else
#define PRE_BIND		(0)
#endif

#ifdef CONFIG_RA_HW_NAT_ACCNT_MAINTAINER
#define ACCNT_MAINTAINER	BIT(21)
#else
#define ACCNT_MAINTAINER		(0)
#endif

#ifdef CONFIG_HW_NAT_IPI
#define HNAT_IPI	BIT(21)
#else
#define HNAT_IPI		(0)
#endif

#ifdef CONFIG_RA_HW_NAT_HASH_DBG_IPV6_SIP
#define DBG_IPV6_SIP	BIT(22)
#else
#define DBG_IPV6_SIP		(0)
#endif

#ifdef CONFIG_RA_HW_NAT_HASH_DBG_IPV4_SIP
#define DBG_IPV4_SIP	BIT(23)
#else
#define DBG_IPV4_SIP		(0)
#endif

#ifdef CONFIG_RA_HW_NAT_HASH_DBG_SPORT
#define DBG_SP	BIT(24)
#else
#define DBG_SP		(0)
#endif

#ifdef CONFIG_QDMA_SUPPORT_QOS
#define ETH_QOS	BIT(25)
#else
#define ETH_QOS		(0)
#endif

#endif

