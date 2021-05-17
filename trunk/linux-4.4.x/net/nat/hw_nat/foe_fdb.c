/*   Module Name:
*    foe_fdb.c
*
*    Abstract:
*
*    Revision History:
*    Who         When            What
*    --------    ----------      ----------------------------------------------
*    Name        Date            Modification logs
*    Steven Liu  2006-10-06      Initial version
*/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/ppp_defs.h>

#include "frame_engine.h"
#include "foe_fdb.h"
#include "hwnat_ioctl.h"
#include "util.h"
#include "ra_nat.h"
#include "api.h"

#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
#include "pptp_l2tp_fdb.h"
#endif
#include "hwnat_define.h"

struct pkt_rx_parse_result ppe_parse_rx_result;
extern u8 USE_3T_UDP_FRAG;
extern int DP_GMAC1;
extern int DPORT_GMAC2;

#define DD \
{\
pr_info("%s %d\n", __func__, __LINE__); \
}

/* 4          2         0 */
/* +----------+---------+ */
/* |      DMAC[47:16]   | */
/* +--------------------+ */
/* |DMAC[15:0]| 2nd VID | */
/* +----------+---------+ */
/* 4          2         0 */
/* +----------+---------+ */
/* |      SMAC[47:16]   | */
/* +--------------------+ */
/* |SMAC[15:0]| PPPOE ID| */
/* +----------+---------+ */
/* Ex: */
/* Mac=01:22:33:44:55:66 */
/* 4          2         0 */
/* +----------+---------+ */
/* |     01:22:33:44    | */
/* +--------------------+ */
/* |  55:66   | PPPOE ID| */
/* +----------+---------+ */
void foe_set_mac_hi_info(u8 *dst, uint8_t *src)
{
	dst[3] = src[0];
	dst[2] = src[1];
	dst[1] = src[2];
	dst[0] = src[3];
}

void foe_set_mac_lo_info(u8 *dst, uint8_t *src)
{
	dst[1] = src[4];
	dst[0] = src[5];
}

static int is_request_done(void)
{
	int count = 1000;

	/* waiting for 1sec to make sure action was finished */
	do {
		if (((reg_read(CAH_CTRL) >> 8) & 0x1) == 0)
			return 1;
		usleep_range(1000, 1100);
	} while (--count);

	return 0;
}

#define MAX_CACHE_LINE_NUM		32
int foe_dump_cache_entry(void)
{
	int line = 0;
	int state = 0;
	int tag = 0;
	int cah_en = 0;
	int i = 0;

	cah_en = reg_read(CAH_CTRL) & 0x1;

	if (!cah_en) {
		pr_debug("Cache is not enabled\n");
		return 0;
	}

	/* cache disable */
	reg_modify_bits(CAH_CTRL, 0, 0, 1);

	pr_debug(" No--|---State---|----Tag-----\n");
	pr_debug("-----+-----------+------------\n");
	for (line = 0; line < MAX_CACHE_LINE_NUM; line++) {
		/* set line number */
		reg_modify_bits(CAH_LINE_RW, line, 0, 15);

		/* OFFSET_RW = 0x1F (Get Entry Number) */
		reg_modify_bits(CAH_LINE_RW, 0x1F, 16, 8);

		/* software access cache command = read */
		reg_modify_bits(CAH_CTRL, 2, 12, 2);

		/* trigger software access cache request */
		reg_modify_bits(CAH_CTRL, 1, 8, 1);

		if (is_request_done()) {
			tag = (reg_read(CAH_RDATA) & 0xFFFF);
			state = ((reg_read(CAH_RDATA) >> 16) & 0x3);
			pr_debug("%04d | %s   | %05d\n", line,
				 (state == 3) ? " Lock  " :
				 (state == 2) ? " Dirty " :
				 (state == 1) ? " Valid " : "Invalid", tag);
		} else {
			pr_debug("%s is timeout (%d)\n", __func__, line);
		}

		/* software access cache command = read */
		reg_modify_bits(CAH_CTRL, 3, 12, 2);

		reg_write(CAH_WDATA, 0);

		/* trigger software access cache request */
		reg_modify_bits(CAH_CTRL, 1, 8, 1);

		if (!is_request_done())
			pr_debug("%s is timeout (%d)\n", __func__, line);
		/* dump first 16B for each foe entry */
		pr_debug("==========<Flow Table Entry=%d >===============\n", tag);
		for (i = 0; i < 16; i++) {
			reg_modify_bits(CAH_LINE_RW, i, 16, 8);

			/* software access cache command = read */
			reg_modify_bits(CAH_CTRL, 2, 12, 2);

			/* trigger software access cache request */
			reg_modify_bits(CAH_CTRL, 1, 8, 1);

			if (is_request_done())
				pr_debug("%02d  %08X\n", i, reg_read(CAH_RDATA));
			else
				pr_debug("%s is timeout (%d)\n", __func__, line);

			/* software access cache command = write */
			reg_modify_bits(CAH_CTRL, 3, 12, 2);

			reg_write(CAH_WDATA, 0);

			/* trigger software access cache request */
			reg_modify_bits(CAH_CTRL, 1, 8, 1);

			if (!is_request_done())
				pr_debug("%s is timeout (%d)\n", __func__, line);
		}
		pr_debug("=========================================\n");
	}

	/* clear cache table before enabling cache */
	reg_modify_bits(CAH_CTRL, 1, 9, 1);
	reg_modify_bits(CAH_CTRL, 0, 9, 1);

	/* cache enable */
	reg_modify_bits(CAH_CTRL, 1, 0, 1);

	return 1;
}

void foe_dump_entry(uint32_t index)
{
	struct foe_entry *entry = &ppe_foe_base[index];
#if defined(CONFIG_RA_HW_NAT_PACKET_SAMPLING)
	struct ps_entry *ps_entry = &ppe_ps_base[index];
#endif
	u32 *p = (uint32_t *)entry;
	u32 i = 0;

	NAT_PRINT("==========<Flow Table Entry=%d (%p)>===============\n", index, entry);
	if (debug_level >= 2) {
#if defined(CONFIG_RA_HW_NAT_IPV6)
		for (i = 0; i < 20; i++) {
#else
		for (i = 0; i < 16; i++) {
#endif
			NAT_PRINT("%02d: %08X\n", i, *(p + i));
		}
	}
	NAT_PRINT("-----------------<Flow Info>------------------\n");
	NAT_PRINT("Information Block 1: %08X\n", entry->ipv4_hnapt.info_blk1);

	if (IS_IPV4_HNAPT(entry)) {
#if defined(CONFIG_RAETH_QDMA)
		NAT_PRINT("Information Block 2=%x (FP=%d FQOS=%d QID=%d)",
			  entry->ipv4_hnapt.info_blk2,
			  entry->ipv4_hnapt.info_blk2 >> 5 & 0x7,
			  entry->ipv4_hnapt.info_blk2 >> 4 & 0x1,
#if defined(CONFIG_ARCH_MT7622)
			  (entry->ipv4_hnapt.iblk2.qid) +
			  ((entry->ipv4_hnapt.iblk2.qid1 & 0x03) << 4));
#else
			  entry->ipv4_hnapt.iblk2.qid);
#endif
#else
		NAT_PRINT("Information Block 2: %08X (FP=%d)\n", entry->ipv4_hnapt.info_blk2,
			  entry->ipv4_hnapt.info_blk2 >> 5 & 0x7);
#endif
		NAT_PRINT("Create IPv4 HNAPT entry\n");
		NAT_PRINT
		    ("IPv4 Org IP/Port: %u.%u.%u.%u:%d->%u.%u.%u.%u:%d\n",
		     IP_FORMAT3(entry->ipv4_hnapt.sip), IP_FORMAT2(entry->ipv4_hnapt.sip),
		     IP_FORMAT1(entry->ipv4_hnapt.sip), IP_FORMAT0(entry->ipv4_hnapt.sip),
		     entry->ipv4_hnapt.sport,
		     IP_FORMAT3(entry->ipv4_hnapt.dip), IP_FORMAT2(entry->ipv4_hnapt.dip),
		     IP_FORMAT1(entry->ipv4_hnapt.dip), IP_FORMAT0(entry->ipv4_hnapt.dip),
		     entry->ipv4_hnapt.dport);
		NAT_PRINT
		    ("IPv4 New IP/Port: %u.%u.%u.%u:%d->%u.%u.%u.%u:%d\n",
		     IP_FORMAT3(entry->ipv4_hnapt.new_sip), IP_FORMAT2(entry->ipv4_hnapt.new_sip),
		     IP_FORMAT1(entry->ipv4_hnapt.new_sip), IP_FORMAT0(entry->ipv4_hnapt.new_sip),
		     entry->ipv4_hnapt.new_sport,
		     IP_FORMAT3(entry->ipv4_hnapt.new_dip), IP_FORMAT2(entry->ipv4_hnapt.new_dip),
		     IP_FORMAT1(entry->ipv4_hnapt.new_dip), IP_FORMAT0(entry->ipv4_hnapt.new_dip),
		     entry->ipv4_hnapt.new_dport);
	} else if (IS_IPV4_HNAT(entry)) {
		NAT_PRINT("Information Block 2: %08X\n", entry->ipv4_hnapt.info_blk2);
		NAT_PRINT("Create IPv4 HNAT entry\n");
		NAT_PRINT("IPv4 Org IP: %u.%u.%u.%u->%u.%u.%u.%u\n",
			  IP_FORMAT3(entry->ipv4_hnapt.sip), IP_FORMAT2(entry->ipv4_hnapt.sip),
			  IP_FORMAT1(entry->ipv4_hnapt.sip), IP_FORMAT0(entry->ipv4_hnapt.sip),
			  IP_FORMAT3(entry->ipv4_hnapt.dip), IP_FORMAT2(entry->ipv4_hnapt.dip),
			  IP_FORMAT1(entry->ipv4_hnapt.dip), IP_FORMAT0(entry->ipv4_hnapt.dip));
		NAT_PRINT("IPv4 New IP: %u.%u.%u.%u->%u.%u.%u.%u\n",
			  IP_FORMAT3(entry->ipv4_hnapt.new_sip), IP_FORMAT2(entry->ipv4_hnapt.new_sip),
			  IP_FORMAT1(entry->ipv4_hnapt.new_sip), IP_FORMAT0(entry->ipv4_hnapt.new_sip),
			  IP_FORMAT3(entry->ipv4_hnapt.new_dip), IP_FORMAT2(entry->ipv4_hnapt.new_dip),
			  IP_FORMAT1(entry->ipv4_hnapt.new_dip), IP_FORMAT0(entry->ipv4_hnapt.new_dip));
#if defined(CONFIG_RA_HW_NAT_IPV6)
	} else if (IS_IPV6_1T_ROUTE(entry)) {
		NAT_PRINT("Information Block 2: %08X\n", entry->ipv6_1t_route.info_blk2);
		NAT_PRINT("Create IPv6 Route entry\n");
		NAT_PRINT("Destination IPv6: %08X:%08X:%08X:%08X",
			  entry->ipv6_1t_route.ipv6_dip3, entry->ipv6_1t_route.ipv6_dip2,
			  entry->ipv6_1t_route.ipv6_dip1, entry->ipv6_1t_route.ipv6_dip0);
	} else if (IS_IPV4_DSLITE(entry)) {
		NAT_PRINT("Information Block 2: %08X\n", entry->ipv4_dslite.info_blk2);
		NAT_PRINT("Create IPv4 Ds-Lite entry\n");
		NAT_PRINT
		    ("IPv4 Ds-Lite: %u.%u.%u.%u.%d->%u.%u.%u.%u:%d\n ",
		     IP_FORMAT3(entry->ipv4_dslite.sip), IP_FORMAT2(entry->ipv4_dslite.sip),
		     IP_FORMAT1(entry->ipv4_dslite.sip), IP_FORMAT0(entry->ipv4_dslite.sip),
		     entry->ipv4_dslite.sport,
		     IP_FORMAT3(entry->ipv4_dslite.dip), IP_FORMAT2(entry->ipv4_dslite.dip),
		     IP_FORMAT1(entry->ipv4_dslite.dip), IP_FORMAT0(entry->ipv4_dslite.dip),
		     entry->ipv4_dslite.dport);
		NAT_PRINT("EG DIPv6: %08X:%08X:%08X:%08X->%08X:%08X:%08X:%08X\n",
			  entry->ipv4_dslite.tunnel_sipv6_0, entry->ipv4_dslite.tunnel_sipv6_1,
			  entry->ipv4_dslite.tunnel_sipv6_2, entry->ipv4_dslite.tunnel_sipv6_3,
			  entry->ipv4_dslite.tunnel_dipv6_0, entry->ipv4_dslite.tunnel_dipv6_1,
			  entry->ipv4_dslite.tunnel_dipv6_2, entry->ipv4_dslite.tunnel_dipv6_3);
	} else if (IS_IPV6_3T_ROUTE(entry)) {
		NAT_PRINT("Information Block 2: %08X\n", entry->ipv6_3t_route.info_blk2);
		NAT_PRINT("Create IPv6 3-Tuple entry\n");
		NAT_PRINT
		    ("ING SIPv6->DIPv6: %08X:%08X:%08X:%08X-> %08X:%08X:%08X:%08X (Prot=%d)\n",
		     entry->ipv6_3t_route.ipv6_sip0, entry->ipv6_3t_route.ipv6_sip1,
		     entry->ipv6_3t_route.ipv6_sip2, entry->ipv6_3t_route.ipv6_sip3,
		     entry->ipv6_3t_route.ipv6_dip0, entry->ipv6_3t_route.ipv6_dip1,
		     entry->ipv6_3t_route.ipv6_dip2, entry->ipv6_3t_route.ipv6_dip3,
		     entry->ipv6_3t_route.prot);
	} else if (IS_IPV6_5T_ROUTE(entry)) {
		NAT_PRINT("Information Block 2: %08X\n", entry->ipv6_5t_route.info_blk2);
		NAT_PRINT("Create IPv6 5-Tuple entry\n");
		if (IS_IPV6_FLAB_EBL()) {
			NAT_PRINT
			    ("ING SIPv6->DIPv6: %08X:%08X:%08X:%08X-> %08X:%08X:%08X:%08X (Flow Label=%08X)\n",
			     entry->ipv6_5t_route.ipv6_sip0, entry->ipv6_5t_route.ipv6_sip1,
			     entry->ipv6_5t_route.ipv6_sip2, entry->ipv6_5t_route.ipv6_sip3,
			     entry->ipv6_5t_route.ipv6_dip0, entry->ipv6_5t_route.ipv6_dip1,
			     entry->ipv6_5t_route.ipv6_dip2, entry->ipv6_5t_route.ipv6_dip3,
			     ((entry->ipv6_5t_route.sport << 16) | (entry->ipv6_5t_route.
								    dport)) & 0xFFFFF);
		} else {
			NAT_PRINT
			    ("ING SIPv6->DIPv6: %08X:%08X:%08X:%08X:%d-> %08X:%08X:%08X:%08X:%d\n",
			     entry->ipv6_5t_route.ipv6_sip0, entry->ipv6_5t_route.ipv6_sip1,
			     entry->ipv6_5t_route.ipv6_sip2, entry->ipv6_5t_route.ipv6_sip3,
			     entry->ipv6_5t_route.sport, entry->ipv6_5t_route.ipv6_dip0,
			     entry->ipv6_5t_route.ipv6_dip1, entry->ipv6_5t_route.ipv6_dip2,
			     entry->ipv6_5t_route.ipv6_dip3, entry->ipv6_5t_route.dport);
		}
	} else if (IS_IPV6_6RD(entry)) {
		NAT_PRINT("Information Block 2: %08X\n", entry->ipv6_6rd.info_blk2);
		NAT_PRINT("Create IPv6 6RD entry\n");
		if (IS_IPV6_FLAB_EBL()) {
			NAT_PRINT
			    ("ING SIPv6->DIPv6: %08X:%08X:%08X:%08X-> %08X:%08X:%08X:%08X (Flow Label=%08X)\n",
			     entry->ipv6_6rd.ipv6_sip0, entry->ipv6_6rd.ipv6_sip1,
			     entry->ipv6_6rd.ipv6_sip2, entry->ipv6_6rd.ipv6_sip3,
			     entry->ipv6_6rd.ipv6_dip0, entry->ipv6_6rd.ipv6_dip1,
			     entry->ipv6_6rd.ipv6_dip2, entry->ipv6_6rd.ipv6_dip3,
			     ((entry->ipv6_5t_route.sport << 16) | (entry->ipv6_5t_route.
								    dport)) & 0xFFFFF);
		} else {
			NAT_PRINT
			    ("ING SIPv6->DIPv6: %08X:%08X:%08X:%08X:%d-> %08X:%08X:%08X:%08X:%d\n",
			     entry->ipv6_6rd.ipv6_sip0, entry->ipv6_6rd.ipv6_sip1,
			     entry->ipv6_6rd.ipv6_sip2, entry->ipv6_6rd.ipv6_sip3,
			     entry->ipv6_6rd.sport, entry->ipv6_6rd.ipv6_dip0,
			     entry->ipv6_6rd.ipv6_dip1, entry->ipv6_6rd.ipv6_dip2,
			     entry->ipv6_6rd.ipv6_dip3, entry->ipv6_6rd.dport);
		}

#endif
	}
	if (IS_IPV4_HNAPT(entry) || IS_IPV4_HNAT(entry)) {
		NAT_PRINT("DMAC=%02X:%02X:%02X:%02X:%02X:%02X SMAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
			  entry->ipv4_hnapt.dmac_hi[3], entry->ipv4_hnapt.dmac_hi[2],
			  entry->ipv4_hnapt.dmac_hi[1], entry->ipv4_hnapt.dmac_hi[0],
			  entry->ipv4_hnapt.dmac_lo[1], entry->ipv4_hnapt.dmac_lo[0],
			  entry->ipv4_hnapt.smac_hi[3], entry->ipv4_hnapt.smac_hi[2],
			  entry->ipv4_hnapt.smac_hi[1], entry->ipv4_hnapt.smac_hi[0],
			  entry->ipv4_hnapt.smac_lo[1], entry->ipv4_hnapt.smac_lo[0]);
		NAT_PRINT("=========================================\n\n");
	}
#if defined(CONFIG_RA_HW_NAT_IPV6)
	else {
		NAT_PRINT("DMAC=%02X:%02X:%02X:%02X:%02X:%02X SMAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
			  entry->ipv6_5t_route.dmac_hi[3], entry->ipv6_5t_route.dmac_hi[2],
			  entry->ipv6_5t_route.dmac_hi[1], entry->ipv6_5t_route.dmac_hi[0],
			  entry->ipv6_5t_route.dmac_lo[1], entry->ipv6_5t_route.dmac_lo[0],
			  entry->ipv6_5t_route.smac_hi[3], entry->ipv6_5t_route.smac_hi[2],
			  entry->ipv6_5t_route.smac_hi[1], entry->ipv6_5t_route.smac_hi[0],
			  entry->ipv6_5t_route.smac_lo[1], entry->ipv6_5t_route.smac_lo[0]);
		NAT_PRINT("=========================================\n\n");
	}
#endif

#if defined(CONFIG_RA_HW_NAT_PACKET_SAMPLING)
	p = (uint32_t *)ps_entry;

	NAT_PRINT("==========<PS Table Entry=%d (%p)>===============\n", index, ps_entry);
	for (i = 0; i < 4; i++)
		pr_debug("%02d: %08X\n", i, *(p + i));
#endif
}

int foe_get_all_entries(struct hwnat_args *opt1)
{
	struct foe_entry *entry;
	int hash_index = 0;
	int count = 0;		/* valid entry count */

	for (hash_index = 0; hash_index < FOE_4TB_SIZ; hash_index++) {
		entry = &ppe_foe_base[hash_index];
		if (entry->bfib1.state == opt1->entry_state) {
			opt1->entries[count].hash_index = hash_index;
			opt1->entries[count].pkt_type = entry->ipv4_hnapt.bfib1.pkt_type;

			if (IS_IPV4_HNAT(entry)) {
				opt1->entries[count].ing_sipv4 = entry->ipv4_hnapt.sip;
				opt1->entries[count].ing_dipv4 = entry->ipv4_hnapt.dip;
				opt1->entries[count].eg_sipv4 = entry->ipv4_hnapt.new_sip;
				opt1->entries[count].eg_dipv4 = entry->ipv4_hnapt.new_dip;
				count++;
			} else if (IS_IPV4_HNAPT(entry)) {
				opt1->entries[count].ing_sipv4 = entry->ipv4_hnapt.sip;
				opt1->entries[count].ing_dipv4 = entry->ipv4_hnapt.dip;
				opt1->entries[count].eg_sipv4 = entry->ipv4_hnapt.new_sip;
				opt1->entries[count].eg_dipv4 = entry->ipv4_hnapt.new_dip;
				opt1->entries[count].ing_sp = entry->ipv4_hnapt.sport;
				opt1->entries[count].ing_dp = entry->ipv4_hnapt.dport;
				opt1->entries[count].eg_sp = entry->ipv4_hnapt.new_sport;
				opt1->entries[count].eg_dp = entry->ipv4_hnapt.new_dport;
				count++;
#if defined(CONFIG_RA_HW_NAT_IPV6)
			} else if (IS_IPV6_1T_ROUTE(entry)) {
				opt1->entries[count].ing_dipv6_0 = entry->ipv6_1t_route.ipv6_dip3;
				opt1->entries[count].ing_dipv6_1 = entry->ipv6_1t_route.ipv6_dip2;
				opt1->entries[count].ing_dipv6_2 = entry->ipv6_1t_route.ipv6_dip1;
				opt1->entries[count].ing_dipv6_3 = entry->ipv6_1t_route.ipv6_dip0;
				count++;
			} else if (IS_IPV4_DSLITE(entry)) {
				opt1->entries[count].ing_sipv4 = entry->ipv4_dslite.sip;
				opt1->entries[count].ing_dipv4 = entry->ipv4_dslite.dip;
				opt1->entries[count].ing_sp = entry->ipv4_dslite.sport;
				opt1->entries[count].ing_dp = entry->ipv4_dslite.dport;
				opt1->entries[count].eg_sipv6_0 = entry->ipv4_dslite.tunnel_sipv6_0;
				opt1->entries[count].eg_sipv6_1 = entry->ipv4_dslite.tunnel_sipv6_1;
				opt1->entries[count].eg_sipv6_2 = entry->ipv4_dslite.tunnel_sipv6_2;
				opt1->entries[count].eg_sipv6_3 = entry->ipv4_dslite.tunnel_sipv6_3;
				opt1->entries[count].eg_dipv6_0 = entry->ipv4_dslite.tunnel_dipv6_0;
				opt1->entries[count].eg_dipv6_1 = entry->ipv4_dslite.tunnel_dipv6_1;
				opt1->entries[count].eg_dipv6_2 = entry->ipv4_dslite.tunnel_dipv6_2;
				opt1->entries[count].eg_dipv6_3 = entry->ipv4_dslite.tunnel_dipv6_3;
				count++;
			} else if (IS_IPV6_3T_ROUTE(entry)) {
				opt1->entries[count].ing_sipv6_0 = entry->ipv6_3t_route.ipv6_sip0;
				opt1->entries[count].ing_sipv6_1 = entry->ipv6_3t_route.ipv6_sip1;
				opt1->entries[count].ing_sipv6_2 = entry->ipv6_3t_route.ipv6_sip2;
				opt1->entries[count].ing_sipv6_3 = entry->ipv6_3t_route.ipv6_sip3;
				opt1->entries[count].ing_dipv6_0 = entry->ipv6_3t_route.ipv6_dip0;
				opt1->entries[count].ing_dipv6_1 = entry->ipv6_3t_route.ipv6_dip1;
				opt1->entries[count].ing_dipv6_2 = entry->ipv6_3t_route.ipv6_dip2;
				opt1->entries[count].ing_dipv6_3 = entry->ipv6_3t_route.ipv6_dip3;
				opt1->entries[count].prot = entry->ipv6_3t_route.prot;
				count++;
			} else if (IS_IPV6_5T_ROUTE(entry)) {
				opt1->entries[count].ing_sipv6_0 = entry->ipv6_5t_route.ipv6_sip0;
				opt1->entries[count].ing_sipv6_1 = entry->ipv6_5t_route.ipv6_sip1;
				opt1->entries[count].ing_sipv6_2 = entry->ipv6_5t_route.ipv6_sip2;
				opt1->entries[count].ing_sipv6_3 = entry->ipv6_5t_route.ipv6_sip3;
				opt1->entries[count].ing_sp = entry->ipv6_5t_route.sport;
				opt1->entries[count].ing_dp = entry->ipv6_5t_route.dport;

				opt1->entries[count].ing_dipv6_0 = entry->ipv6_5t_route.ipv6_dip0;
				opt1->entries[count].ing_dipv6_1 = entry->ipv6_5t_route.ipv6_dip1;
				opt1->entries[count].ing_dipv6_2 = entry->ipv6_5t_route.ipv6_dip2;
				opt1->entries[count].ing_dipv6_3 = entry->ipv6_5t_route.ipv6_dip3;
				opt1->entries[count].ipv6_flowlabel = IS_IPV6_FLAB_EBL();
				count++;
			} else if (IS_IPV6_6RD(entry)) {
				opt1->entries[count].ing_sipv6_0 = entry->ipv6_6rd.ipv6_sip0;
				opt1->entries[count].ing_sipv6_1 = entry->ipv6_6rd.ipv6_sip1;
				opt1->entries[count].ing_sipv6_2 = entry->ipv6_6rd.ipv6_sip2;
				opt1->entries[count].ing_sipv6_3 = entry->ipv6_6rd.ipv6_sip3;

				opt1->entries[count].ing_dipv6_0 = entry->ipv6_6rd.ipv6_dip0;
				opt1->entries[count].ing_dipv6_1 = entry->ipv6_6rd.ipv6_dip1;
				opt1->entries[count].ing_dipv6_2 = entry->ipv6_6rd.ipv6_dip2;
				opt1->entries[count].ing_dipv6_3 = entry->ipv6_6rd.ipv6_dip3;
				opt1->entries[count].ing_sp = entry->ipv6_6rd.sport;
				opt1->entries[count].ing_dp = entry->ipv6_6rd.dport;
				opt1->entries[count].ipv6_flowlabel = IS_IPV6_FLAB_EBL();

				opt1->entries[count].eg_sipv4 = entry->ipv6_6rd.tunnel_sipv4;
				opt1->entries[count].eg_dipv4 = entry->ipv6_6rd.tunnel_dipv4;
				count++;
#endif
			}
		}
	}
	opt1->num_of_entries = count;
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
	/* pptp_l2tp_fdb_dump(); */
#endif

	if (opt1->num_of_entries > 0)
		return HWNAT_SUCCESS;
	else
		return HWNAT_ENTRY_NOT_FOUND;
}

int foe_bind_entry(struct hwnat_args *opt1)
{
	struct foe_entry *entry;

	entry = &ppe_foe_base[opt1->entry_num];

	/* restore right information block1 */
	entry->bfib1.time_stamp = reg_read(FOE_TS) & 0xFFFF;
	entry->bfib1.state = BIND;

	return HWNAT_SUCCESS;
}

int foe_un_bind_entry(struct hwnat_args *opt)
{
	struct foe_entry *entry;

	entry = &ppe_foe_base[opt->entry_num];

	entry->ipv4_hnapt.udib1.state = INVALID;
	entry->ipv4_hnapt.udib1.time_stamp = reg_read(FOE_TS) & 0xFF;

	ppe_set_cache_ebl();	/*clear HWNAT cache */

	return HWNAT_SUCCESS;
}

int _foe_drop_entry(unsigned int entry_num)
{
	struct foe_entry *entry;

	entry = &ppe_foe_base[entry_num];

	entry->ipv4_hnapt.iblk2.dp = 7;

	ppe_set_cache_ebl();	/*clear HWNAT cache */

	return HWNAT_SUCCESS;
}
EXPORT_SYMBOL(_foe_drop_entry);

int foe_drop_entry(struct hwnat_args *opt)
{
	return _foe_drop_entry(opt->entry_num);
}

int foe_del_entry_by_num(uint32_t entry_num)
{
	struct foe_entry *entry;

	entry = &ppe_foe_base[entry_num];
	memset(entry, 0, sizeof(struct foe_entry));
	ppe_set_cache_ebl();	/*clear HWNAT cache */
	
	return HWNAT_SUCCESS;
}

void foe_tbl_clean(void)
{
	u32 foe_tbl_size;

	foe_tbl_size = FOE_4TB_SIZ * sizeof(struct foe_entry);
	memset(ppe_foe_base, 0, foe_tbl_size);
	ppe_set_cache_ebl();	/*clear HWNAT cache */
}
EXPORT_SYMBOL(foe_tbl_clean);

void hw_nat_l2_info(struct foe_entry *entry, struct hwnat_tuple *opt)
{

	if ((opt->pkt_type) == IPV4_NAPT) {
		foe_set_mac_hi_info(entry->ipv4_hnapt.dmac_hi, opt->dmac);
		foe_set_mac_lo_info(entry->ipv4_hnapt.dmac_lo, opt->dmac);
		foe_set_mac_hi_info(entry->ipv4_hnapt.smac_hi, opt->smac);
		foe_set_mac_lo_info(entry->ipv4_hnapt.smac_lo, opt->smac);
		entry->ipv4_hnapt.vlan1=opt->vlan1;
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
		/*mt7622 wifi hwnat not support vlan2*/
#else
		entry->ipv4_hnapt.vlan2=opt->vlan2;
#endif
		entry->ipv4_hnapt.pppoe_id=opt->pppoe_id;
	} else if ((opt->pkt_type) == IPV6_ROUTING) {
#if defined(CONFIG_RA_HW_NAT_IPV6)
		foe_set_mac_hi_info(entry->ipv6_5t_route.dmac_hi, opt->dmac);
		foe_set_mac_lo_info(entry->ipv6_5t_route.dmac_lo, opt->dmac);
		foe_set_mac_hi_info(entry->ipv6_5t_route.smac_hi, opt->smac);
		foe_set_mac_lo_info(entry->ipv6_5t_route.smac_lo, opt->smac);
		entry->ipv6_5t_route.vlan1=opt->vlan1;
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
		/*mt7622 wifi hwnat not support vlan2*/
#else
		entry->ipv6_5t_route.vlan2=opt->vlan2;
#endif
		entry->ipv6_5t_route.pppoe_id=opt->pppoe_id;
#endif
	}
}
void hw_nat_l3_info(struct foe_entry *entry, struct hwnat_tuple *opt)
{
	if ((opt->pkt_type) == IPV4_NAPT) {
		entry->ipv4_hnapt.sip=opt->ing_sipv4; 
		entry->ipv4_hnapt.dip=opt->ing_dipv4;
		entry->ipv4_hnapt.new_sip=opt->eg_sipv4;
		entry->ipv4_hnapt.new_dip=opt->eg_dipv4;
	} else if ((opt->pkt_type) == IPV6_ROUTING) {
#if defined(CONFIG_RA_HW_NAT_IPV6)
		entry->ipv6_5t_route.ipv6_sip0 = opt->ing_sipv6_0;
		entry->ipv6_5t_route.ipv6_sip1 = opt->ing_sipv6_1;
		entry->ipv6_5t_route.ipv6_sip2 = opt->ing_sipv6_2;
		entry->ipv6_5t_route.ipv6_sip3 = opt->ing_sipv6_3;

		entry->ipv6_5t_route.ipv6_dip0 = opt->ing_dipv6_0;
		entry->ipv6_5t_route.ipv6_dip1 = opt->ing_dipv6_1;
		entry->ipv6_5t_route.ipv6_dip2 = opt->ing_dipv6_2;
		entry->ipv6_5t_route.ipv6_dip3 = opt->ing_dipv6_3;
#endif
/*
		printk("opt->ing_sipv6_0 = %x\n", opt->ing_sipv6_0);
		printk("opt->ing_sipv6_1 = %x\n", opt->ing_sipv6_1);
		printk("opt->ing_sipv6_2 = %x\n", opt->ing_sipv6_2);
		printk("opt->ing_sipv6_3 = %x\n", opt->ing_sipv6_3);
		printk("opt->ing_dipv6_0 = %x\n", opt->ing_dipv6_0);
		printk("opt->ing_dipv6_1 = %x\n", opt->ing_dipv6_1);
		printk("opt->ing_dipv6_2 = %x\n", opt->ing_dipv6_2);
		printk("opt->ing_dipv6_3 = %x\n", opt->ing_dipv6_3);
				
		printk("entry->ipv6_5t_route.ipv6_sip0 = %x\n", entry->ipv6_5t_route.ipv6_sip0);
		printk("entry->ipv6_5t_route.ipv6_sip1 = %x\n", entry->ipv6_5t_route.ipv6_sip1);
		printk("entry->ipv6_5t_route.ipv6_sip2 = %x\n", entry->ipv6_5t_route.ipv6_sip2);
		printk("entry->ipv6_5t_route.ipv6_sip3 = %x\n", entry->ipv6_5t_route.ipv6_sip3);
		printk("entry->ipv6_5t_route.ipv6_dip0 = %x\n", entry->ipv6_5t_route.ipv6_dip0);
		printk("entry->ipv6_5t_route.ipv6_dip1 = %x\n", entry->ipv6_5t_route.ipv6_dip1);
		printk("entry->ipv6_5t_route.ipv6_dip2 = %x\n", entry->ipv6_5t_route.ipv6_dip2);
		printk("entry->ipv6_5t_route.ipv6_dip3 = %x\n", entry->ipv6_5t_route.ipv6_dip3);
*/
	}
}

void hw_nat_l4_info(struct foe_entry *entry, struct hwnat_tuple *opt)
{
	if ((opt->pkt_type) == IPV4_NAPT) {
		entry->ipv4_hnapt.dport = opt->ing_dp;
		entry->ipv4_hnapt.sport = opt->ing_sp;
		entry->ipv4_hnapt.new_dport = opt->eg_dp;
		entry->ipv4_hnapt.new_sport = opt->eg_sp;
	} else if ((opt->pkt_type) == IPV6_ROUTING) {
#if defined(CONFIG_RA_HW_NAT_IPV6)
		entry->ipv6_5t_route.dport = opt->ing_dp;
		entry->ipv6_5t_route.sport = opt->ing_sp;
#endif

	}
}

void hw_nat_ib1_info(struct foe_entry *entry, struct hwnat_tuple *opt)
{
	if ((opt->pkt_type) == IPV4_NAPT) {
		entry->ipv4_hnapt.bfib1.pkt_type = IPV4_NAPT;
		entry->ipv4_hnapt.bfib1.sta=1;
		entry->ipv4_hnapt.bfib1.udp=opt->is_udp; /* tcp/udp */
		entry->ipv4_hnapt.bfib1.state=BIND; 
		entry->ipv4_hnapt.bfib1.ka=1; /* keepalive */
		entry->ipv4_hnapt.bfib1.ttl=0; /* TTL-1 */
		entry->ipv4_hnapt.bfib1.psn=opt->pppoe_act; /* insert / remove */
		entry->ipv4_hnapt.bfib1.vlan_layer = opt->vlan_layer;
		entry->ipv4_hnapt.bfib1.time_stamp=reg_read(FOE_TS)&0xFFFF;
	} else if ((opt->pkt_type) == IPV6_ROUTING) {
#if defined(CONFIG_RA_HW_NAT_IPV6)
		entry->ipv6_5t_route.bfib1.pkt_type = IPV6_ROUTING;
		entry->ipv6_5t_route.bfib1.sta=1;
		entry->ipv6_5t_route.bfib1.udp=opt->is_udp; /* tcp/udp */
		entry->ipv6_5t_route.bfib1.state=BIND; 
		entry->ipv6_5t_route.bfib1.ka=1; /* keepalive */
		entry->ipv6_5t_route.bfib1.ttl=0; /* TTL-1 */
		entry->ipv6_5t_route.bfib1.psn=opt->pppoe_act; /* insert / remove */
		entry->ipv6_5t_route.bfib1.vlan_layer = opt->vlan_layer;
		entry->ipv6_5t_route.bfib1.time_stamp=reg_read(FOE_TS)&0xFFFF;
#endif
	}
}

void hw_nat_ib2_info(struct foe_entry *entry, struct hwnat_tuple *opt)
{
	if ((opt->pkt_type) == IPV4_NAPT) {
		entry->ipv4_hnapt.iblk2.dp=opt->dst_port; /* 0:cpu, 1:GE1 */
		entry->ipv4_hnapt.iblk2.dscp = opt->dscp;
#if defined(CONFIG_ARCH_MT7622)
		entry->ipv4_hnapt.iblk2.acnt = opt->dst_port;
#else
		entry->ipv4_hnapt.iblk2.port_mg = 0x3f;
		entry->ipv4_hnapt.iblk2.port_ag = opt->dst_port;
#endif

	} else if ((opt->pkt_type) == IPV6_ROUTING) {
#if defined(CONFIG_RA_HW_NAT_IPV6)
		entry->ipv6_5t_route.iblk2.dp=opt->dst_port; /* 0:cpu, 1:GE1 */
		entry->ipv6_5t_route.iblk2.dscp = opt->dscp;
#if defined(CONFIG_ARCH_MT7622)
		entry->ipv6_5t_route.iblk2.acnt = opt->dst_port;
#else
		entry->ipv6_5t_route.iblk2.port_mg = 0x3f;
		entry->ipv6_5t_route.iblk2.port_ag = opt->dst_port;
#endif

#endif
	}
}

void hw_nat_semi_bind(struct foe_entry *entry, struct hwnat_tuple *opt)
{
	uint32_t current_time;
	if ((opt->pkt_type) == IPV4_NAPT) {
		/* Set Current time to time_stamp field in information block 1 */
		current_time = reg_read(FOE_TS) & 0xFFFF;
		entry->bfib1.time_stamp = (uint16_t) current_time;
		/* Ipv4: TTL / Ipv6: Hot Limit filed */
		entry->ipv4_hnapt.bfib1.ttl = DFL_FOE_TTL_REGEN;
		/* enable cache by default */
		entry->ipv4_hnapt.bfib1.cah = 1;
    		/* Change Foe Entry State to Binding State */
		entry->bfib1.state = BIND;
	} else if ((opt->pkt_type) == IPV6_ROUTING) {
#if defined(CONFIG_RA_HW_NAT_IPV6)
    		/* Set Current time to time_stamp field in information block 1 */
		current_time = reg_read(FOE_TS) & 0xFFFF;
		entry->bfib1.time_stamp = (uint16_t) current_time;
    		/* Ipv4: TTL / Ipv6: Hot Limit filed */
		entry->ipv4_hnapt.bfib1.ttl = DFL_FOE_TTL_REGEN;
    		/* enable cache by default */
		entry->ipv4_hnapt.bfib1.cah = 1;
    		/* Change Foe Entry State to Binding State */
		entry->bfib1.state = BIND;
#endif
	}
}

#if defined (CONFIG_HW_NAT_SEMI_AUTO_MODE)

int set_done_bit_zero(struct foe_entry * foe_entry)
{
	if (IS_IPV4_HNAT(foe_entry) || IS_IPV4_HNAPT(foe_entry)) {
		foe_entry->ipv4_hnapt.resv1 = 0;
	}
#if defined (CONFIG_RA_HW_NAT_IPV6)
	else if (IS_IPV4_DSLITE(foe_entry)) {
		foe_entry->ipv4_dslite.resv1 = 0;
	} else if (IS_IPV6_3T_ROUTE(foe_entry)) {
		foe_entry->ipv6_3t_route.resv1 = 0;
	} else if (IS_IPV6_5T_ROUTE(foe_entry)) {
		foe_entry->ipv6_5t_route.resv1 = 0;
	} else if (IS_IPV6_6RD(foe_entry)) {
		foe_entry->ipv6_6rd.resv1 = 0;
	} else {
		printk("%s:get packet format something wrong\n", __func__);
		return -1;
	}
#endif
	return 0;
}

int get_entry_done_bit(struct foe_entry * foe_entry)
{
	int done_bit;
	
	if (IS_IPV4_HNAT(foe_entry) || IS_IPV4_HNAPT(foe_entry)) {
		done_bit = foe_entry->ipv4_hnapt.resv1;
	}
#if defined (CONFIG_HNAT_V2)
#if defined (CONFIG_RA_HW_NAT_IPV6)
	else if (IS_IPV4_DSLITE(foe_entry)) {
		done_bit = foe_entry->ipv4_dslite.resv1;
	} else if (IS_IPV6_3T_ROUTE(foe_entry)) {
		done_bit = foe_entry->ipv6_3t_route.resv1;
	} else if (IS_IPV6_5T_ROUTE(foe_entry)) {
		done_bit = foe_entry->ipv6_5t_route.resv1;
	} else if (IS_IPV6_6RD(foe_entry)) {
		done_bit = foe_entry->ipv6_6rd.resv1;
	} else {
		printk("%s:get packet format something wrong\n", __func__);
		return -1;
	}
#endif
#endif
	return done_bit;
}
#endif

int FoeAddEntry(struct hwnat_tuple *opt)
{
	struct foe_pri_key key;
	struct foe_entry *entry = NULL;
	int32_t hash_index;
#if defined (CONFIG_HW_NAT_SEMI_AUTO_MODE)
	int done_bit;
#endif
	if ((opt->pkt_type) == IPV4_NAPT) {
		key.ipv4_hnapt.sip=opt->ing_sipv4;
		key.ipv4_hnapt.dip=opt->ing_dipv4;
		key.ipv4_hnapt.sport=opt->ing_sp;
		key.ipv4_hnapt.dport=opt->ing_dp;
		key.ipv4_hnapt.is_udp=opt->is_udp;
	} else if ((opt->pkt_type) == IPV6_ROUTING) {
		key.ipv6_routing.sip0=opt->ing_sipv6_0;
		key.ipv6_routing.sip1=opt->ing_sipv6_1;
		key.ipv6_routing.sip2=opt->ing_sipv6_2;
		key.ipv6_routing.sip3=opt->ing_sipv6_3;
		key.ipv6_routing.dip0=opt->ing_dipv6_0;
		key.ipv6_routing.dip1=opt->ing_dipv6_1;
		key.ipv6_routing.dip2=opt->ing_dipv6_2;
		key.ipv6_routing.dip3=opt->ing_dipv6_3;
		key.ipv6_routing.sport=opt->ing_sp;
		key.ipv6_routing.dport=opt->ing_dp;
		key.ipv6_routing.is_udp=opt->is_udp;
	}

	key.pkt_type=opt->pkt_type;
#if defined (CONFIG_HW_NAT_MANUAL_MODE)
	hash_index = GetPpeEntryIdx(&key, entry, 0);
#else
	hash_index = GetPpeEntryIdx(&key, entry, 1);
#endif	
	if(hash_index != -1) {

		opt->hash_index=hash_index;
		entry=&ppe_foe_base[hash_index];
#if defined (CONFIG_HW_NAT_MANUAL_MODE)
		hw_nat_l2_info(entry, opt);
		hw_nat_l3_info(entry, opt);
		hw_nat_l4_info(entry, opt);
		hw_nat_ib1_info(entry, opt);
		hw_nat_ib2_info(entry, opt);
#endif
#if defined (CONFIG_HW_NAT_SEMI_AUTO_MODE)
		done_bit = get_entry_done_bit(entry);
	    	if (done_bit == 1) {
	    		printk("mtk_entry_add number =%d\n", hash_index);
	    	} else if (done_bit == 0){
	    		printk("ppe table not ready\n");
	    		return HWNAT_FAIL;
	    	} else {
	    		printk("%s: done_bit something wrong\n", __func__);
	    		return HWNAT_FAIL;
	    	}
	    	hw_nat_semi_bind(entry, opt);
#endif
		foe_dump_entry(hash_index);
		return HWNAT_SUCCESS;
	}

	return HWNAT_FAIL;

}

int FoeDelEntry(struct hwnat_tuple *opt)
{
	struct foe_pri_key key;
	int32_t hash_index;
	struct foe_entry *entry = NULL;
	int32_t rply_idx;
#if defined (CONFIG_HW_NAT_SEMI_AUTO_MODE)
	int done_bit;
#endif

	if ((opt->pkt_type) == IPV4_NAPT) {
		key.ipv4_hnapt.sip=opt->ing_sipv4;
		key.ipv4_hnapt.dip=opt->ing_dipv4;
		key.ipv4_hnapt.sport=opt->ing_sp;
		key.ipv4_hnapt.dport=opt->ing_dp;
		//key.ipv4_hnapt.is_udp=opt->is_udp;
	} else if ((opt->pkt_type) == IPV6_ROUTING) {
		key.ipv6_routing.sip0=opt->ing_sipv6_0;
		key.ipv6_routing.sip1=opt->ing_sipv6_1;
		key.ipv6_routing.sip2=opt->ing_sipv6_2;
		key.ipv6_routing.sip3=opt->ing_sipv6_3;
		key.ipv6_routing.dip0=opt->ing_dipv6_0;
		key.ipv6_routing.dip1=opt->ing_dipv6_1;
		key.ipv6_routing.dip2=opt->ing_dipv6_2;
		key.ipv6_routing.dip3=opt->ing_dipv6_3;
		key.ipv6_routing.sport=opt->ing_sp;
		key.ipv6_routing.dport=opt->ing_dp;
		//key.ipv6_routing.is_udp=opt->is_udp;
	}

	key.pkt_type=opt->pkt_type;
	
	// find bind entry                 
	//hash_index = FoeHashFun(&key,BIND);
	hash_index = GetPpeEntryIdx(&key, entry, 1);;
	if(hash_index != -1) {
		opt->hash_index=hash_index;
		rply_idx = reply_entry_idx(opt, hash_index);
#if defined (CONFIG_HW_NAT_SEMI_AUTO_MODE)
		entry=&ppe_foe_base[hash_index];
		done_bit = get_entry_done_bit(entry);
	    	if (done_bit == 1) {
	    		set_done_bit_zero(entry);
	    	} else if (done_bit == 0){
	    		printk("%s : ppe table not ready\n", __func__);
	    		return HWNAT_FAIL;
	    	} else {
	    		printk("%s: done_bit something wrong\n", __func__);
	    		set_done_bit_zero(entry);
	    		return HWNAT_FAIL;
	    	}
#endif
		foe_del_entry_by_num(hash_index);
		printk("Clear Entry index = %d\n", hash_index);
		if(rply_idx != -1) {
		printk("Clear Entry index = %d\n", rply_idx);
			foe_del_entry_by_num(rply_idx);
		}

		return HWNAT_SUCCESS;
	}
	printk("HWNAT ENTRY NOT FOUND\n");
	return HWNAT_ENTRY_NOT_FOUND;
}
EXPORT_SYMBOL(FoeDelEntry);

int get_five_tule(struct sk_buff *skb)
{
	struct ethhdr *eth = NULL;
	struct iphdr *iph = NULL;
	struct ipv6hdr *ip6h = NULL;
	struct tcphdr *th = NULL;
	struct udphdr *uh = NULL;
	u8 ipv6_head_len = 0;
	
	memset(&ppe_parse_rx_result, 0, sizeof(ppe_parse_rx_result));
	eth = (struct ethhdr *)skb->data;
	ppe_parse_rx_result.eth_type = eth->h_proto;
	/* set layer4 start addr */
	if ((ppe_parse_rx_result.eth_type == htons(ETH_P_IP)) ||
	    (ppe_parse_rx_result.eth_type == htons(ETH_P_PPP_SES) &&
	    (ppe_parse_rx_result.ppp_tag == htons(PPP_IP)))) {

		iph = (struct iphdr *)(skb->data + ETH_HLEN);
		memcpy(&ppe_parse_rx_result.iph, iph, sizeof(struct iphdr));
		if (iph->protocol == IPPROTO_TCP) {
			skb_set_transport_header(skb, ETH_HLEN + (iph->ihl * 4));
			th = (struct tcphdr *)skb_transport_header(skb);
			memcpy(&ppe_parse_rx_result.th, th, sizeof(struct tcphdr));
			ppe_parse_rx_result.pkt_type = IPV4_HNAPT;
			if (iph->frag_off & htons(IP_MF | IP_OFFSET)) {
				if (debug_level >= 2) 
					DD;
				return 1;
			}
		} else if (iph->protocol == IPPROTO_UDP) {
			skb_set_transport_header(skb, ETH_HLEN + (iph->ihl * 4));		
			uh = (struct udphdr *)skb_transport_header(skb);
			memcpy(&ppe_parse_rx_result.uh, uh, sizeof(struct udphdr));
			ppe_parse_rx_result.pkt_type = IPV4_HNAPT;
			if (iph->frag_off & htons(IP_MF | IP_OFFSET))
			{
				if (debug_level >= 2) 
					DD;
				if (USE_3T_UDP_FRAG == 0)
					return 1;
			}
		} else if (iph->protocol == IPPROTO_GRE) {
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
#else
				if (debug_level >= 2) 
					DD;
				/* do nothing */
				return 1;
#endif
		}
#if defined(CONFIG_RA_HW_NAT_IPV6)
		else if (iph->protocol == IPPROTO_IPV6) {
			ip6h = (struct ipv6hdr *)((uint8_t *)iph + iph->ihl * 4);
			memcpy(&ppe_parse_rx_result.ip6h, ip6h, sizeof(struct ipv6hdr));
			if (ip6h->nexthdr == NEXTHDR_TCP) {
				skb_set_transport_header(skb, ETH_HLEN + (sizeof(struct ipv6hdr)));
				th = (struct tcphdr *)skb_transport_header(skb);
				memcpy(&ppe_parse_rx_result.th.source, &th->source, sizeof(th->source));
				memcpy(&ppe_parse_rx_result.th.dest, &th->dest, sizeof(th->dest));
			} else if (ip6h->nexthdr == NEXTHDR_UDP) {
				skb_set_transport_header(skb, ETH_HLEN + (sizeof(struct ipv6hdr)));
				uh = (struct udphdr *)skb_transport_header(skb);
				memcpy(&ppe_parse_rx_result.uh.source, &uh->source, sizeof(uh->source));
				memcpy(&ppe_parse_rx_result.uh.dest, &uh->dest, sizeof(uh->dest));
			}
				ppe_parse_rx_result.pkt_type = IPV6_6RD;
#if defined(CONFIG_RALINK_MT7620) || defined(CONFIG_RALINK_MT7621)
	/* identification field in outer ipv4 header is zero*/
	/*after erntering binding state.*/
	/* some 6rd relay router will drop the packet */
				if (debug_level >= 2) 
					DD;
				return 1;
#endif
		}
#endif
		else {
				if (debug_level >= 2) 
					DD;
				/* Packet format is not supported */
				return 1;
		}

	} else if (ppe_parse_rx_result.eth_type == htons(ETH_P_IPV6) ||
		   (ppe_parse_rx_result.eth_type == htons(ETH_P_PPP_SES) &&
		    ppe_parse_rx_result.ppp_tag == htons(PPP_IPV6))) {
		ip6h = (struct ipv6hdr *)skb_network_header(skb);
		memcpy(&ppe_parse_rx_result.ip6h, ip6h, sizeof(struct ipv6hdr));
		if (ip6h->nexthdr == NEXTHDR_TCP) {
			skb_set_transport_header(skb, ETH_HLEN + (sizeof(struct ipv6hdr)));
			th = (struct tcphdr *)skb_transport_header(skb);
			memcpy(&ppe_parse_rx_result.th, th, sizeof(struct tcphdr));
			ppe_parse_rx_result.pkt_type = IPV6_5T_ROUTE;
		} else if (ip6h->nexthdr == NEXTHDR_UDP) {
			skb_set_transport_header(skb, ETH_HLEN + (sizeof(struct ipv6hdr)));
			uh = (struct udphdr *)skb_transport_header(skb);
			memcpy(&ppe_parse_rx_result.uh, uh, sizeof(struct udphdr));
			ppe_parse_rx_result.pkt_type = IPV6_5T_ROUTE;
		} else if (ip6h->nexthdr == NEXTHDR_IPIP) {
			ipv6_head_len = sizeof(struct iphdr);
			memcpy(&ppe_parse_rx_result.iph, ip6h + ipv6_head_len,
			       sizeof(struct iphdr));
			ppe_parse_rx_result.pkt_type = IPV4_DSLITE;
		} else {
			ppe_parse_rx_result.pkt_type = IPV6_3T_ROUTE;
		}

	} else {
				if (debug_level >= 2) 
					DD;
		return 1;
	}
#if(0)
if (debug_level >= 2) {
		pr_info("--------------\n");
		pr_info("DMAC:%02X:%02X:%02X:%02X:%02X:%02X\n",
			 ppe_parse_rx_result.dmac[0], ppe_parse_rx_result.dmac[1],
			 ppe_parse_rx_result.dmac[2], ppe_parse_rx_result.dmac[3],
			 ppe_parse_rx_result.dmac[4], ppe_parse_rx_result.dmac[5]);
		pr_info("SMAC:%02X:%02X:%02X:%02X:%02X:%02X\n",
			 ppe_parse_rx_result.smac[0], ppe_parse_rx_result.smac[1],
			 ppe_parse_rx_result.smac[2], ppe_parse_rx_result.smac[3],
			 ppe_parse_rx_result.smac[4], ppe_parse_rx_result.smac[5]);
		pr_info("Eth_Type=%x\n", ppe_parse_rx_result.eth_type);

		pr_info("PKT_TYPE=%s\n",
			 ppe_parse_rx_result.pkt_type ==
			 0 ? "IPV4_HNAT" : ppe_parse_rx_result.pkt_type ==
			 1 ? "IPV4_HNAPT" : ppe_parse_rx_result.pkt_type ==
			 3 ? "IPV4_DSLITE" : ppe_parse_rx_result.pkt_type ==
			 4 ? "IPV6_ROUTE" : ppe_parse_rx_result.pkt_type == 5 ? "IPV6_6RD" : "Unknown");
		if (ppe_parse_rx_result.pkt_type == IPV4_HNAT) {
			pr_info("SIP=%s\n", ip_to_str(ntohl(ppe_parse_rx_result.iph.saddr)));
			pr_info("DIP=%s\n", ip_to_str(ntohl(ppe_parse_rx_result.iph.daddr)));
			pr_info("TOS=%x\n", ntohs(ppe_parse_rx_result.iph.tos));
		} else if (ppe_parse_rx_result.pkt_type == IPV4_HNAPT) {
			pr_info("SIP=%s\n", ip_to_str(ntohl(ppe_parse_rx_result.iph.saddr)));
			pr_info("DIP=%s\n", ip_to_str(ntohl(ppe_parse_rx_result.iph.daddr)));
			pr_info("TOS=%x\n", ntohs(ppe_parse_rx_result.iph.tos));

			if (ppe_parse_rx_result.iph.protocol == IPPROTO_TCP) {
				pr_info("TCP SPORT=%d\n", ntohs(ppe_parse_rx_result.th.source));
				pr_info("TCP DPORT=%d\n", ntohs(ppe_parse_rx_result.th.dest));
			} else if (ppe_parse_rx_result.iph.protocol == IPPROTO_UDP) {
				pr_info("UDP SPORT=%d\n", ntohs(ppe_parse_rx_result.uh.source));
				pr_info("UDP DPORT=%d\n", ntohs(ppe_parse_rx_result.uh.dest));
			}
		} else if (ppe_parse_rx_result.pkt_type == IPV6_6RD) {
			/* fill in ipv4 6rd entry */
			pr_info("packet_type = IPV6_6RD\n");
			pr_info("SIP=%s\n", ip_to_str(ntohl(ppe_parse_rx_result.iph.saddr)));
			pr_info("DIP=%s\n", ip_to_str(ntohl(ppe_parse_rx_result.iph.daddr)));

			pr_info("Checksum=%x\n", ntohs(ppe_parse_rx_result.iph.check));
			pr_info("ipV4 ID =%x\n", ntohs(ppe_parse_rx_result.iph.id));
			pr_info("Flag=%x\n", ntohs(ppe_parse_rx_result.iph.frag_off) >> 13);
			pr_info("TTL=%x\n", ppe_parse_rx_result.iph.ttl);
			pr_info("TOS=%x\n", ppe_parse_rx_result.iph.tos);
		}
	}
#endif
	return 0;
}

int decide_qid(uint16_t hash_index, struct sk_buff * skb)
{
	struct foe_entry *entry; 
	uint32_t saddr;
	uint32_t daddr;

	uint32_t ppeSaddr;
	uint32_t ppeDaddr;
	uint32_t ppeSport;
	uint32_t ppeDport;
	
	uint32_t sport;
	uint32_t dport;

	uint32_t ipv6_sip_127_96;
	uint32_t ipv6_sip_95_64;
	uint32_t ipv6_sip_63_32;
	uint32_t ipv6_sip_31_0;
		
	uint32_t ipv6_dip_127_96;
	uint32_t ipv6_dip_95_64;
	uint32_t ipv6_dip_63_32;
	uint32_t ipv6_dip_31_0;
								
	uint32_t ppeSaddr_127_96;
	uint32_t ppeSaddr_95_64;
	uint32_t ppeSaddr_63_32;
	uint32_t ppeSaddr_31_0;
		
	uint32_t ppeDaddr_127_96;
	uint32_t ppeDaddr_95_64;
	uint32_t ppeDaddr_63_32;
	uint32_t ppeDaddr_31_0;
		
	uint32_t ppeSportV6;
	uint32_t ppeDportV6;

	entry = &ppe_foe_base[hash_index];
	if (IS_IPV4_HNAPT(entry)) {
		saddr = ntohl(ppe_parse_rx_result.iph.saddr);
		daddr = ntohl(ppe_parse_rx_result.iph.daddr);
		if (ppe_parse_rx_result.iph.protocol == IPPROTO_TCP) {
			sport = ntohs(ppe_parse_rx_result.th.source);
			dport = ntohs(ppe_parse_rx_result.th.dest);
		}else if(ppe_parse_rx_result.iph.protocol == IPPROTO_UDP) {
			sport = ntohs(ppe_parse_rx_result.uh.source);
			dport = ntohs(ppe_parse_rx_result.uh.dest);
		}
		ppeSaddr = entry->ipv4_hnapt.sip;
		ppeDaddr = entry->ipv4_hnapt.dip;
		ppeSport = entry->ipv4_hnapt.sport;
		ppeDport = entry->ipv4_hnapt.dport;
		if (debug_level >= 2) {
			pr_info("ppeSaddr = %x, ppeDaddr=%x, ppeSport=%d, ppeDport=%d, saddr=%x, daddr=%x, sport= %d, dport=%d\n", 
				ppeSaddr, ppeDaddr, ppeSport, ppeDport, saddr, daddr, sport, dport);
		}
		if ((saddr == ppeSaddr) && (daddr == ppeDaddr) 
			&& (sport == ppeSport) && (dport == ppeDport) && (entry->bfib1.state == BIND)){
			
		    	if (entry->ipv4_hnapt.iblk2.dp == 2){
#if defined (CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
				skb->dev = dst_port[DPORT_GMAC2];
#else
		    		skb->dev = dst_port[DP_GMAC2];
#endif
		    		//if (entry->ipv4_hnapt.iblk2.qid >= 11)
		    		if (debug_level >= 2)
		    			pr_info("qid = %d\n", entry->ipv4_hnapt.iblk2.qid);
				skb->mark = entry->ipv4_hnapt.iblk2.qid;
		    	}else{
#if defined (CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
				skb->dev = dst_port[DP_GMAC1];
#else
				skb->dev = dst_port[DP_GMAC];
#endif	
				if (debug_level >= 2)
		    			pr_info("qid = %d\n", entry->ipv4_hnapt.iblk2.qid);
				skb->mark = entry->ipv4_hnapt.iblk2.qid;
		    	}			
			return 0;
		}else {
			return -1;
		}

	}
#if defined (CONFIG_RA_HW_NAT_IPV6) 
	else if (IS_IPV6_5T_ROUTE(entry)) {
		ipv6_sip_127_96 = ntohl(ppe_parse_rx_result.ip6h.saddr.s6_addr32[0]);
		ipv6_sip_95_64 = ntohl(ppe_parse_rx_result.ip6h.saddr.s6_addr32[1]);
		ipv6_sip_63_32 = ntohl(ppe_parse_rx_result.ip6h.saddr.s6_addr32[2]);
		ipv6_sip_31_0 = ntohl(ppe_parse_rx_result.ip6h.saddr.s6_addr32[3]);
		
		ipv6_dip_127_96 = ntohl(ppe_parse_rx_result.ip6h.daddr.s6_addr32[0]);;
		ipv6_dip_95_64 = ntohl(ppe_parse_rx_result.ip6h.daddr.s6_addr32[1]);
		ipv6_dip_63_32 = ntohl(ppe_parse_rx_result.ip6h.daddr.s6_addr32[2]);
		ipv6_dip_31_0 = ntohl(ppe_parse_rx_result.ip6h.daddr.s6_addr32[3]);
					
				
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
		if (ppe_parse_rx_result.iph.protocol == IPPROTO_TCP) {
			sport = ntohs(ppe_parse_rx_result.th.source);
			dport = ntohs(ppe_parse_rx_result.th.dest);
		}else if(ppe_parse_rx_result.iph.protocol == IPPROTO_UDP) {
			sport = ntohs(ppe_parse_rx_result.uh.source);
			dport = ntohs(ppe_parse_rx_result.uh.dest);
		}	
		if ((ipv6_sip_127_96 == ppeSaddr_127_96) && (ipv6_sip_95_64 == ppeSaddr_95_64) 
			&& (ipv6_sip_63_32 == ppeSaddr_63_32) && (ipv6_sip_31_0 == ppeSaddr_31_0) && 
			(ipv6_dip_127_96 == ppeDaddr_127_96) && (ipv6_dip_95_64 == ppeDaddr_95_64) 
			&& (ipv6_dip_63_32 == ppeDaddr_63_32) && (ipv6_dip_31_0 == ppeDaddr_31_0) &&
			(sport == ppeSportV6) && (dport == ppeDportV6) &&
			(entry->bfib1.state == BIND)){
		    	if (entry->ipv6_5t_route.iblk2.dp == 2){
#if defined (CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
				skb->dev = dst_port[DPORT_GMAC2];
#else
		    		skb->dev = dst_port[DP_GMAC2];
#endif
		    			//if (entry->ipv6_3t_route.iblk2.qid >= 11)
		    		skb->mark = (entry->ipv6_3t_route.iblk2.qid);
		    	}else{
#if defined (CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
				skb->dev = dst_port[DP_GMAC1];
#else
			    	skb->dev = dst_port[DP_GMAC];
#endif	
		    		skb->mark = (entry->ipv6_3t_route.iblk2.qid );
		    	}	
		}else {
			return -1;
		}
		
	}
#endif
	return 0;
}

void set_qid(struct sk_buff *skb)
{
	struct foe_pri_key key;
	int32_t hash_index;
	struct foe_entry *entry = NULL;
	
	get_five_tule(skb);
	if (ppe_parse_rx_result.pkt_type == IPV4_HNAPT){
		key.ipv4_hnapt.sip = ntohl(ppe_parse_rx_result.iph.saddr);
		key.ipv4_hnapt.dip = ntohl(ppe_parse_rx_result.iph.daddr);

		if (ppe_parse_rx_result.iph.protocol == IPPROTO_TCP) {
			key.ipv4_hnapt.sport = ntohs(ppe_parse_rx_result.th.source);
			key.ipv4_hnapt.dport = ntohs(ppe_parse_rx_result.th.dest);
		}else if(ppe_parse_rx_result.iph.protocol == IPPROTO_UDP) {
			key.ipv4_hnapt.sport = ntohs(ppe_parse_rx_result.uh.source);
			key.ipv4_hnapt.dport = ntohs(ppe_parse_rx_result.uh.dest);
		}
		//key.ipv4_hnapt.is_udp=opt->is_udp;
	}else if (ppe_parse_rx_result.pkt_type == IPV6_5T_ROUTE){
		key.ipv6_routing.sip0 = ntohl(ppe_parse_rx_result.ip6h.saddr.s6_addr32[0]);
		key.ipv6_routing.sip1 = ntohl(ppe_parse_rx_result.ip6h.saddr.s6_addr32[1]);
		key.ipv6_routing.sip2 = ntohl(ppe_parse_rx_result.ip6h.saddr.s6_addr32[2]);
		key.ipv6_routing.sip3= ntohl(ppe_parse_rx_result.ip6h.saddr.s6_addr32[3]);
		key.ipv6_routing.dip0 = ntohl(ppe_parse_rx_result.ip6h.daddr.s6_addr32[0]);;
		key.ipv6_routing.dip1 = ntohl(ppe_parse_rx_result.ip6h.daddr.s6_addr32[1]);
		key.ipv6_routing.dip2 = ntohl(ppe_parse_rx_result.ip6h.daddr.s6_addr32[2]);
		key.ipv6_routing.dip3 = ntohl(ppe_parse_rx_result.ip6h.daddr.s6_addr32[3]);
		if (ppe_parse_rx_result.ip6h.nexthdr == IPPROTO_TCP) {
			key.ipv6_routing.sport = ntohs(ppe_parse_rx_result.th.source);
			key.ipv6_routing.dport = ntohs(ppe_parse_rx_result.th.dest);
		}else if(ppe_parse_rx_result.ip6h.nexthdr == IPPROTO_UDP) {
			key.ipv6_routing.sport = ntohs(ppe_parse_rx_result.uh.source);
			key.ipv6_routing.dport = ntohs(ppe_parse_rx_result.uh.dest);
		}		
		
	
	}

	key.pkt_type = ppe_parse_rx_result.pkt_type;
	
	// find bind entry                 
	//hash_index = FoeHashFun(&key,BIND);
	hash_index = GetPpeEntryIdx(&key, entry, 1);
	if(hash_index != -1)
		decide_qid(hash_index, skb);
	if (debug_level >= 6) 
		pr_info("hash_index = %d\n", hash_index);
}
