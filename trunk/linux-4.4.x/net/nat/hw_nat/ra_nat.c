/*Module Name:
*  ra_nat.c
*
*  Abstract:
*
*  Revision History:
*  Who         When            What
*  --------    ----------      ----------------------------------------------
*  Name        Date            Modification logs
*  Steven Liu  2011-11-11      Support MT7620 Cache mechanism
*  Steven Liu  2011-06-01      Support MT7620
*  Steven Liu  2011-04-11      Support RT6855A
*  Steven Liu  2011-02-08      Support IPv6 over PPPoE
*  Steven Liu  2010-11-25      Fix double VLAN + PPPoE header bug
*  Steven Liu  2010-11-24      Support upstream/downstream/bi-direction acceleration
*  Steven Liu  2010-11-17      Support Linux 2.6.36 kernel
*  Steven Liu  2010-07-13      Support DSCP to User Priority helper
*  Steven Liu  2010-06-03      Support skb headroom/tailroom/cb to keep HNAT information
*  Kurtis Ke   2010-03-30      Support HNAT parameter can be changed by application
*  Steven Liu  2010-04-08      Support RT3883 + RT309x concurrent AP
*  Steven Liu  2010-03-01      Support RT3352
*  Steven Liu  2009-11-26      Support WiFi pseudo interface by using VLAN tag
*  Steven Liu  2009-07-21      Support IPV6 Forwarding
*  Steven Liu  2009-04-02      Support RT3883/RT3350
*  Steven Liu  2008-03-19      Support RT3052
*  Steven Liu  2007-09-25      Support RT2880 MP
*  Steven Liu  2006-10-06      Initial version
*/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/if_vlan.h>
#include <net/ipv6.h>
#include <net/ip.h>
#include <linux/if_pppox.h>
#include <linux/ppp_defs.h>
#include <linux/pci.h>
#include <linux/errno.h>
#include <linux/inetdevice.h>
#include <net/rtnetlink.h>
#include <net/netevent.h>
#include <linux/platform_device.h>
#include "ra_nat.h"
#include "foe_fdb.h"
#include "frame_engine.h"
#include "util.h"
#include "hwnat_ioctl.h"
#include "hwnat_define.h"

unsigned short wan_vid __read_mostly = CONFIG_RA_HW_NAT_WAN_VLANID;
module_param(wan_vid, ushort, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(wan_vid, "VLAN ID for WAN traffic");

struct timer_list hwnat_clear_entry_timer;
static void hwnat_clear_entry(unsigned long data)
{
	//printk("HW_NAT work normally\n");
	reg_modify_bits(PPE_FOE_CFG, FWD_CPU_BUILD_ENTRY, 4, 2);
	//del_timer_sync(&hwnat_clear_entry_timer);

}
void foe_clear_entry(struct neighbour *neigh);
#if defined(CONFIG_RAETH_QDMA) && defined(CONFIG_PPE_MCAST)
#include "mcast_tbl.h"
#endif

/*#include "../../../drivers/net/raeth/ra_ioctl.h"*/
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
#include "fast_path.h"
#endif
u8 USE_3T_UDP_FRAG;
struct foe_entry *ppe_foe_base;
EXPORT_SYMBOL(ppe_foe_base);
dma_addr_t ppe_phy_foe_base;
#if defined(CONFIG_RA_HW_NAT_PACKET_SAMPLING)
struct ps_entry *ppe_ps_base;
dma_addr_t ppe_phy_ps_base;
#endif
#if defined(CONFIG_PPE_MIB)
struct mib_entry *ppe_mib_base;
dma_addr_t ppe_phy_mib_base;
#endif
struct pkt_parse_result ppe_parse_result;
#ifdef CONFIG_RA_HW_NAT_ACCNT_MAINTAINER
struct hwnat_ac_args ac_info[64];	/* 1 for LAN, 2 for WAN */
#endif
#ifndef CONFIG_PSEUDO_SUPPORT	/* Web UI used */
#ifndef CONFIG_RAETH_HW_VLAN_TX
struct net_device *dev;
#endif
#endif
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
int fast_bind;
u8 hash_cnt;
#endif
#if defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
int DP_GMAC1;
int DPORT_GMAC2;
#endif

/* #define DSCP_REMARK_TEST */
/* #define PREBIND_TEST */
#define DD \
{\
pr_info("%s %d\n", __func__, __LINE__); \
}

#define HWSFQUP 3
#define HWSFQDL 1

#if defined (CONFIG_HW_NAT_IPI)
unsigned int ipidbg[NR_CPUS][10];    
unsigned int ipidbg2[NR_CPUS][10];
extern int32_t HnatIPIExtIfHandler(struct sk_buff * skb);
extern int32_t HnatIPIForceCPU(struct sk_buff * skb);

//extern int HnatIPIInit();
//extern int HnatIPIDeInit();
#endif
#if defined(CONFIG_ARCH_MT7622)
#define USE_UDP_FRAG
#endif
uint16_t IS_IF_PCIE_WLAN(struct sk_buff *skb)
{
	if (IS_MAGIC_TAG_PROTECT_VALID_HEAD(skb))
		return IS_IF_PCIE_WLAN_HEAD(skb);
	else if (IS_MAGIC_TAG_PROTECT_VALID_TAIL(skb))
		return IS_IF_PCIE_WLAN_TAIL(skb);
	else if (IS_MAGIC_TAG_PROTECT_VALID_CB(skb))
		return IS_IF_PCIE_WLAN_CB(skb);
	else
		return 0;
}

uint16_t IS_IF_PCIE_WLAN_RX(struct sk_buff *skb)
{
	return IS_IF_PCIE_WLAN_HEAD(skb);
}

uint16_t IS_MAGIC_TAG_PROTECT_VALID(struct sk_buff *skb)
{
	if (IS_MAGIC_TAG_PROTECT_VALID_HEAD(skb))
		return IS_MAGIC_TAG_PROTECT_VALID_HEAD(skb);
	else if (IS_MAGIC_TAG_PROTECT_VALID_TAIL(skb))
		return IS_MAGIC_TAG_PROTECT_VALID_TAIL(skb);
	else if (IS_MAGIC_TAG_PROTECT_VALID_CB(skb))
		return IS_MAGIC_TAG_PROTECT_VALID_CB(skb);
	else
		return 0;
}

unsigned char *FOE_INFO_START_ADDR(struct sk_buff *skb)
{
	if (IS_MAGIC_TAG_PROTECT_VALID_HEAD(skb))
		return FOE_INFO_START_ADDR_HEAD(skb);
	else if (IS_MAGIC_TAG_PROTECT_VALID_TAIL(skb))
		return FOE_INFO_START_ADDR_TAIL(skb);
	else if (IS_MAGIC_TAG_PROTECT_VALID_CB(skb))
		return FOE_INFO_START_ADDR_CB(skb);

	pr_info("!!!FOE_INFO_START_ADDR Error!!!!\n");
	return FOE_INFO_START_ADDR_HEAD(skb);
}

void FOE_INFO_DUMP(struct sk_buff *skb)
{
	pr_info("FOE_INFO_START_ADDR(skb) =%p\n", FOE_INFO_START_ADDR(skb));
	pr_info("FOE_TAG_PROTECT(skb) =%x\n", FOE_TAG_PROTECT(skb));
	pr_info("FOE_ENTRY_NUM(skb) =%x\n", FOE_ENTRY_NUM(skb));
	pr_info("FOE_ALG(skb) =%x\n", FOE_ALG(skb));
	pr_info("FOE_AI(skb) =%x\n", FOE_AI(skb));
	pr_info("FOE_SP(skb) =%x\n", FOE_SP(skb));
	pr_info("FOE_MAGIC_TAG(skb) =%x\n", FOE_MAGIC_TAG(skb));
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
	pr_info("FOE_WDMA_ID(skb) =%x\n", FOE_WDMA_ID(skb));
	pr_info("FOE_RX_ID(skb) =%x\n", FOE_RX_ID(skb));
	pr_info("FOE_WC_ID(skb) =%x\n", FOE_WC_ID(skb));
	pr_info("FOE_FOE_BSS_IDIF(skb) =%x\n", FOE_BSS_ID(skb));
#endif
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
	pr_info("FOE_SOURCE(skb) =%x\n", FOE_SOURCE(skb));
	pr_info("FOE_SOURCE(skb) =%x\n", FOE_SOURCE(skb));
#endif
}

void FOE_INFO_DUMP_TAIL(struct sk_buff *skb)
{
	pr_info("FOE_INFO_START_ADDR_TAIL(skb) =%p\n", FOE_INFO_START_ADDR_TAIL(skb));
	pr_info("FOE_TAG_PROTECT_TAIL(skb) =%x\n", FOE_TAG_PROTECT_TAIL(skb));
	pr_info("FOE_ENTRY_NUM_TAIL(skb) =%x\n", FOE_ENTRY_NUM_TAIL(skb));
	pr_info("FOE_ALG_TAIL(skb) =%x\n", FOE_ALG_TAIL(skb));
	pr_info("FOE_AI_TAIL(skb) =%x\n", FOE_AI_TAIL(skb));
	pr_info("FOE_SP_TAIL(skb) =%x\n", FOE_SP_TAIL(skb));
	pr_info("FOE_MAGIC_TAG_TAIL(skb) =%x\n", FOE_MAGIC_TAG_TAIL(skb));
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
	pr_info("FOE_WDMA_ID_TAIL(skb) =%x\n", FOE_WDMA_ID_TAIL(skb));
	pr_info("FOE_RX_ID_TAIL(skb) =%x\n", FOE_RX_ID_TAIL(skb));
	pr_info("FOE_WC_ID_TAIL(skb) =%x\n", FOE_WC_ID_TAIL(skb));
	pr_info("FOE_FOE_BSS_IDIF_TAIL(skb) =%x\n", FOE_BSS_ID_TAIL(skb));
#endif
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
	pr_info("FOE_SOURCE_TAIL(skb) =%x\n", FOE_SOURCE_TAIL(skb));
	pr_info("FOE_SOURCE_TAIL(skb) =%x\n", FOE_SOURCE_TAIL(skb));
#endif
}

int hwnat_info_region;
uint16_t tx_decide_which_region(struct sk_buff *skb)
{
	if (IS_MAGIC_TAG_PROTECT_VALID_HEAD(skb) && IS_SPACE_AVAILABLE_HEAD(skb)) {
		FOE_INFO_START_ADDR(skb);
		FOE_TAG_PROTECT(skb) = FOE_TAG_PROTECT_HEAD(skb);
		FOE_ENTRY_NUM(skb) = FOE_ENTRY_NUM_HEAD(skb);
		FOE_ALG(skb) = FOE_ALG_HEAD(skb);
		FOE_AI(skb) = FOE_AI_HEAD(skb);
		FOE_SP(skb) = FOE_SP_HEAD(skb);
		FOE_MAGIC_TAG(skb) = FOE_MAGIC_TAG_HEAD(skb);
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
		FOE_WDMA_ID(skb) = FOE_WDMA_ID_HEAD(skb);
		FOE_RX_ID(skb) = FOE_RX_ID_HEAD(skb);
		FOE_WC_ID(skb) = FOE_WC_ID_HEAD(skb);
		FOE_BSS_ID(skb) = FOE_BSS_ID_HEAD(skb);
#endif
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
		FOE_SOURCE(skb) = FOE_SOURCE_HEAD(skb);
		FOE_DEST(skb) = FOE_DEST_HEAD(skb);
#endif
		hwnat_info_region = USE_HEAD_ROOM;
		return USE_HEAD_ROOM;	/* use headroom */
	} else if (IS_MAGIC_TAG_PROTECT_VALID_TAIL(skb) && IS_SPACE_AVAILABLE_TAIL(skb)) {
		FOE_INFO_START_ADDR(skb);
		FOE_TAG_PROTECT(skb) = FOE_TAG_PROTECT_TAIL(skb);
		FOE_ENTRY_NUM(skb) = FOE_ENTRY_NUM_TAIL(skb);
		FOE_ALG(skb) = FOE_ALG_TAIL(skb);
		FOE_AI(skb) = FOE_AI_TAIL(skb);
		FOE_SP(skb) = FOE_SP_TAIL(skb);
		
		FOE_MAGIC_TAG(skb) = FOE_MAGIC_TAG_TAIL(skb);	
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
		FOE_WDMA_ID(skb) = FOE_WDMA_ID_TAIL(skb);
		FOE_RX_ID(skb) = FOE_RX_ID_TAIL(skb);
		FOE_WC_ID(skb) = FOE_WC_ID_TAIL(skb);
		FOE_BSS_ID(skb) = FOE_BSS_ID_TAIL(skb);
#endif
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
		FOE_SOURCE(skb) = FOE_SOURCE_TAIL(skb);
		FOE_DEST(skb) = FOE_DEST_TAIL(skb);
#endif
		hwnat_info_region = USE_TAIL_ROOM;
		return USE_TAIL_ROOM;	/* use tailroom */
	} else if (IS_MAGIC_TAG_PROTECT_VALID_CB(skb)) {
		FOE_INFO_START_ADDR(skb);
		FOE_TAG_PROTECT(skb) = FOE_TAG_PROTECT_CB0(skb);
		FOE_ENTRY_NUM(skb) = FOE_ENTRY_NUM_CB(skb);
		FOE_ALG(skb) = FOE_ALG_CB(skb);
		FOE_AI(skb) = FOE_AI_CB(skb);
		FOE_SP(skb) = FOE_SP_CB(skb);
		FOE_MAGIC_TAG(skb) = FOE_MAGIC_TAG_CB(skb);
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
		FOE_WDMA_ID(skb) = FOE_WDMA_ID_CB(skb);
		FOE_RX_ID(skb) = FOE_RX_ID_CB(skb);
		FOE_WC_ID(skb) = FOE_WC_ID_CB(skb);
		FOE_BSS_ID(skb) = FOE_BSS_ID_CB(skb);
#endif
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
		FOE_SOURCE(skb) = FOE_SOURCE_CB(skb);
		FOE_DEST(skb) = FOE_DEST_CB(skb);
#endif
		hwnat_info_region = USE_CB;
		return USE_CB;	/* use CB */
	}
	hwnat_info_region = ALL_INFO_ERROR;
	return ALL_INFO_ERROR;
}

uint16_t remove_vlan_tag(struct sk_buff *skb)
{
	struct ethhdr *eth;
	struct vlan_ethhdr *veth;
	u16 vir_if_idx;

	if (skb_vlan_tag_present(skb)) { //hw vlan rx enable
		vir_if_idx = skb_vlan_tag_get(skb) & 0x3fff;
		skb->vlan_proto = 0;
		skb->vlan_tci = 0;
		return vir_if_idx;
	}

	veth = (struct vlan_ethhdr *)skb_mac_header(skb);
	/* something wrong */
	if ((veth->h_vlan_proto != htons(ETH_P_8021Q)) && (veth->h_vlan_proto != 0x5678)) {
		//if (pr_debug_ratelimited())
		//	pr_info("HNAT: Reentry packet is untagged frame?\n");
		return 65535;
	}
	/*we just want to get vid*/
	vir_if_idx = ntohs(veth->h_vlan_TCI) & 0x3fff;

	if (skb_cloned(skb) || skb_shared(skb)) {
		struct sk_buff *new_skb;

		new_skb = skb_copy(skb, GFP_ATOMIC);
		kfree_skb(skb);
		if (!new_skb)
			return 65535;
		skb = new_skb;
		/*logic error*/
		//kfree_skb(new_skb);
	}

	/* remove VLAN tag */
	skb->data = skb_mac_header(skb);
	skb->mac_header = skb->mac_header + VLAN_HLEN;
	memmove(skb_mac_header(skb), skb->data, ETH_ALEN * 2);

	skb_pull(skb, VLAN_HLEN);
	skb->data += ETH_HLEN;	/* pointer to layer3 header */
	eth = (struct ethhdr *)skb_mac_header(skb);

	skb->protocol = eth->h_proto;

	return vir_if_idx;
}

static int foe_alloc_tbl(uint32_t num_of_entry, struct device *dev)
{
	u32 foe_tbl_size;
#if defined(CONFIG_RA_HW_NAT_PACKET_SAMPLING)
	u32 ps_tbl_size;
#endif
#if defined(CONFIG_RA_HW_NAT_IPV6) && defined(CONFIG_RALINK_MT7621)
	struct foe_entry *entry;
	int boundary_entry_offset[7] = { 12, 25, 38, 51, 76, 89, 102 };
	/*these entries are bad every 128 entries */
	int entry_base = 0;
	int bad_entry, i, j;
#endif
	dma_addr_t ppe_phy_foebase_tmp;
#if defined(CONFIG_ARCH_MT7622)
#if defined(CONFIG_PPE_MIB)
	u32 mib_tbl_size;
#endif
#endif

	foe_tbl_size = num_of_entry * sizeof(struct foe_entry);
	ppe_phy_foebase_tmp = reg_read(PPE_FOE_BASE);

	if (ppe_phy_foebase_tmp) {
		ppe_phy_foe_base = ppe_phy_foebase_tmp;
		ppe_foe_base = (struct foe_entry *)ppe_virt_foe_base_tmp;
		pr_info("***ppe_foe_base = %p\n", ppe_foe_base);
		pr_info("***PpeVirtFoeBase_tmp = %p\n", ppe_virt_foe_base_tmp);
		if (!ppe_foe_base) {
			pr_info("PPE_FOE_BASE=%x\n", reg_read(PPE_FOE_BASE));
			pr_info("ppe_foe_base ioremap fail!!!!\n");
			return 0;
		}
	} else {
#if defined(CONFIG_ARCH_MT7622)
		ppe_foe_base =
		    dma_alloc_coherent(dev, foe_tbl_size, &ppe_phy_foe_base, GFP_KERNEL);		
#else
		ppe_foe_base = dma_alloc_coherent(NULL, foe_tbl_size, &ppe_phy_foe_base, GFP_KERNEL);

#endif

		ppe_virt_foe_base_tmp = ppe_foe_base;
		pr_info("init PpeVirtFoeBase_tmp = %p\n", ppe_virt_foe_base_tmp);
		pr_info("init ppe_foe_base = %p\n", ppe_foe_base);
		
		if (!ppe_foe_base) {
			pr_info("first ppe_phy_foe_base fail\n");
			return 0;
		}
	}

	if (!ppe_foe_base) {
		pr_info("ppe_foe_base== NULL\n");
		return 0;
	}

	reg_write(PPE_FOE_BASE, ppe_phy_foe_base);
	memset(ppe_foe_base, 0, foe_tbl_size);

//#if defined(CONFIG_PPE_MIB)
	//for (i = 0; i < num_of_entry; i++) {
		//entry = &ppe_foe_base[i];
		//entry->ipv4_hnapt.iblk2.mibf = 1;
	//}
//#endif

#if defined(CONFIG_RA_HW_NAT_IPV6) && defined(CONFIG_RALINK_MT7621)
	for (i = 0; entry_base < num_of_entry; i++) {
		/* set bad entries as static */
		for (j = 0; j < 7; j++) {
			bad_entry = entry_base + boundary_entry_offset[j];
			entry = &ppe_foe_base[bad_entry];
			entry->udib1.sta = 1;
		}
		entry_base = (i + 1) * 128;
	}
#endif

#if defined(CONFIG_RA_HW_NAT_PACKET_SAMPLING)
	ps_tbl_size = num_of_entry * sizeof(struct ps_entry);
#if defined(CONFIG_ARCH_MT7622)
	ppe_ps_base = dma_alloc_coherent(dev, ps_tbl_size, &ppe_phy_ps_base, GFP_KERNEL);
#else
	ppe_ps_base = dma_alloc_coherent(NULL, foe_tbl_size, &ppe_phy_ps_base, GFP_KERNEL);
#endif

	if (!ppe_ps_base)
		return 0;
	reg_write(PS_TB_BASE, ppe_phy_ps_base);
	memset(ppe_ps_base, 0, foe_tbl_size);
#endif
#if defined(CONFIG_PPE_MIB)
	mib_tbl_size = num_of_entry * sizeof(struct mib_entry);
	pr_info("num_of_entry: foe_tbl_size = %d\n", foe_tbl_size);
	ppe_mib_base = dma_alloc_coherent(dev, mib_tbl_size, &ppe_phy_mib_base, GFP_KERNEL);
	if (!ppe_mib_base) {
		pr_info("PPE MIB allocate memory fail");
		return 0;
	}
	pr_info("ppe_mib_base = %p\n",  ppe_mib_base);
	pr_info("ppe_phy_mib_base = %llu\n",  ppe_phy_mib_base);
	pr_info("num_of_entry = %u\n",  num_of_entry);
	pr_info("sizeof(struct mib_entry) = %lu\n",  sizeof(struct mib_entry));
	pr_info("mib_tbl_size = %d\n",  mib_tbl_size);
	reg_write(MIB_TB_BASE, ppe_phy_mib_base);
	memset(ppe_mib_base, 0, mib_tbl_size);
#endif

	return 1;
}

static uint8_t *show_cpu_reason(struct sk_buff *skb)
{
	static u8 buf[32];

	switch (FOE_AI(skb)) {
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
#if defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || (CONFIG_ARCH_MT7623)
	case HIT_BIND_MULTICAST_TO_GMAC_CPU:
		return "Hit bind multicast packet to GMAC & CPU\n";
	case HIT_PRE_BIND:
		return "Pre bind\n";
#endif
	}

	sprintf(buf, "CPU Reason Error - %X\n", FOE_AI(skb));
	return buf;
}

#if(0)
uint32_t foe_dump_pkt_tx(struct sk_buff *skb)
{
	struct foe_entry *entry = &ppe_foe_base[FOE_ENTRY_NUM(skb)];
	int i;

	NAT_PRINT("\nTx===<FOE_Entry=%d>=====\n", FOE_ENTRY_NUM(skb));
	pr_info("Tx handler skb_headroom size = %u, skb->head = %p, skb->data = %p\n",
		 skb_headroom(skb), skb->head, skb->data);
	for (i = 0; i < skb_headroom(skb); i++) {
		pr_info("tx_skb->head[%d]=%x\n", i, *(unsigned char *)(skb->head + i));
		/* pr_info("%02X-",*((unsigned char*)i)); */
	}

	NAT_PRINT("==================================\n");
	return 1;
}
#endif

uint32_t foe_dump_pkt(struct sk_buff *skb)
{
	struct foe_entry *entry = &ppe_foe_base[FOE_ENTRY_NUM(skb)];

	NAT_PRINT("\nRx===<FOE_Entry=%d>=====\n", FOE_ENTRY_NUM(skb));
	NAT_PRINT("RcvIF=%s\n", skb->dev->name);
	NAT_PRINT("FOE_Entry=%d\n", FOE_ENTRY_NUM(skb));
	NAT_PRINT("CPU Reason=%s", show_cpu_reason(skb));
	NAT_PRINT("ALG=%d\n", FOE_ALG(skb));
	NAT_PRINT("SP=%d\n", FOE_SP(skb));

	/* some special alert occurred, so entry_num is useless (just skip it) */
	if (FOE_ENTRY_NUM(skb) == 0x3fff)
		return 1;

	/* PPE: IPv4 packet=IPV4_HNAT IPv6 packet=IPV6_ROUTE */
	if (IS_IPV4_GRP(entry)) {
		NAT_PRINT("Information Block 1=%x\n", entry->ipv4_hnapt.info_blk1);
		NAT_PRINT("SIP=%s\n", ip_to_str(entry->ipv4_hnapt.sip));
		NAT_PRINT("DIP=%s\n", ip_to_str(entry->ipv4_hnapt.dip));
		NAT_PRINT("SPORT=%d\n", entry->ipv4_hnapt.sport);
		NAT_PRINT("DPORT=%d\n", entry->ipv4_hnapt.dport);
		NAT_PRINT("Information Block 2=%x\n", entry->ipv4_hnapt.info_blk2);
	}
#if defined(CONFIG_RA_HW_NAT_IPV6)
	else if (IS_IPV6_GRP(entry)) {
		NAT_PRINT("Information Block 1=%x\n", entry->ipv6_5t_route.info_blk1);
		NAT_PRINT("IPv6_SIP=%08X:%08X:%08X:%08X\n",
			  entry->ipv6_5t_route.ipv6_sip0,
			  entry->ipv6_5t_route.ipv6_sip1,
			  entry->ipv6_5t_route.ipv6_sip2, entry->ipv6_5t_route.ipv6_sip3);
		NAT_PRINT("IPv6_DIP=%08X:%08X:%08X:%08X\n",
			  entry->ipv6_5t_route.ipv6_dip0,
			  entry->ipv6_5t_route.ipv6_dip1,
			  entry->ipv6_5t_route.ipv6_dip2, entry->ipv6_5t_route.ipv6_dip3);
		if (IS_IPV6_FLAB_EBL()) {
			NAT_PRINT("Flow Label=%08X\n", (entry->ipv6_5t_route.sport << 16) |
				  (entry->ipv6_5t_route.dport));
		} else {
			NAT_PRINT("SPORT=%d\n", entry->ipv6_5t_route.sport);
			NAT_PRINT("DPORT=%d\n", entry->ipv6_5t_route.dport);
		}
		NAT_PRINT("Information Block 2=%x\n", entry->ipv6_5t_route.info_blk2);
	}
#endif
	else
		NAT_PRINT("unknown Pkt_type=%d\n", entry->bfib1.pkt_type);

	NAT_PRINT("==================================\n");
	return 1;
}

int getBrLan = 0;
uint32_t brNetmask;
uint32_t br0Ip;
char br0_mac_address[6];
int get_bridge_info(void)
{
	struct net_device *br0_dev; 
	struct in_device *br0_in_dev;
#if 0
	br0_dev = dev_get_by_name(&init_net,"br-lan");
#else
	br0_dev = dev_get_by_name(&init_net,"br0");
#endif	
	if (br0_dev == NULL) {
		pr_info("br0_dev = NULL\n");
		return 1;
	}
	br0_in_dev = in_dev_get(br0_dev);
	if (br0_in_dev == NULL) {
		pr_info("br0_in_dev = NULL\n");
		return 1;
	}
	brNetmask = ntohl(br0_in_dev->ifa_list->ifa_mask);
	br0Ip = ntohl(br0_in_dev->ifa_list->ifa_address);
	if(br0_dev != NULL) {
		dev_put(br0_dev);
	}
	
	if (br0_in_dev != NULL) 
		in_dev_put(br0_in_dev);
	else
		pr_info("br0_in_dev = NULL\n");
	
	pr_debug("br0Ip = %x\n", br0Ip);
	pr_debug("brNetmask = %x\n", brNetmask);
	getBrLan = 1;
	
	return 0;
}
int bridge_lan_subnet(struct sk_buff *skb)
{
	struct iphdr *iph = NULL;
	uint32_t daddr = 0;
	uint32_t saddr = 0;
	u32 eth_type; 
	u32 ppp_tag = 0;
	struct vlan_hdr *vh = NULL;
	struct ethhdr *eth = NULL;
	struct pppoe_hdr *peh = NULL;
	u8 vlan1_gap = 0;
	u8 vlan2_gap = 0;
	u8 pppoe_gap = 0;
	int ret;

#ifdef CONFIG_RAETH_HW_VLAN_TX
	struct vlan_hdr pseudo_vhdr;
#endif

	eth = (struct ethhdr *)skb->data;
	if(is_multicast_ether_addr(&eth->h_dest[0]))
		return 0;
	eth_type = eth->h_proto;
	if ((eth_type == htons(ETH_P_8021Q)) ||
	    (((eth_type) & 0x00FF) == htons(ETH_P_8021Q)) || hwnat_vlan_tx_tag_present(skb)) {

#ifdef CONFIG_RAETH_HW_VLAN_TX
		pseudo_vhdr.h_vlan_TCI = htons(hwnat_vlan_tag_get(skb));
		pseudo_vhdr.h_vlan_encapsulated_proto = eth->h_proto;
		vh = (struct vlan_hdr *)&pseudo_vhdr;
		vlan1_gap = VLAN_HLEN;
#else
		vlan1_gap = VLAN_HLEN;
		vh = (struct vlan_hdr *)(skb->data + ETH_HLEN);
#endif

		/* VLAN + PPPoE */
		if (ntohs(vh->h_vlan_encapsulated_proto) == ETH_P_PPP_SES) {
			pppoe_gap = 8;
			eth_type = vh->h_vlan_encapsulated_proto;
			/* Double VLAN = VLAN + VLAN */
		} else if ((vh->h_vlan_encapsulated_proto == htons(ETH_P_8021Q)) || 
			   ((vh->h_vlan_encapsulated_proto) & 0x00FF) == htons(ETH_P_8021Q)) {
	
			vlan2_gap = VLAN_HLEN;
			vh = (struct vlan_hdr *)(skb->data + ETH_HLEN + VLAN_HLEN);
			/* VLAN + VLAN + PPPoE */
			if (ntohs(vh->h_vlan_encapsulated_proto) == ETH_P_PPP_SES) {
				pppoe_gap = 8;
				eth_type = vh->h_vlan_encapsulated_proto;
			}else
				eth_type = vh->h_vlan_encapsulated_proto; 
		}
	} else if (ntohs(eth_type) == ETH_P_PPP_SES) {
		/* PPPoE + IP */
		pppoe_gap = 8;
		peh = (struct pppoe_hdr *)(skb->data + ETH_HLEN + vlan1_gap);
		ppp_tag = peh->tag[0].tag_type;
	}

	if (getBrLan == 0) {
		ret = get_bridge_info(); /*return 1 br0 get fail*/
		if (ret == 1)
			return 0;
	}
	/* set layer4 start addr */
	if ((eth_type == htons(ETH_P_IP)) || (eth_type == htons(ETH_P_PPP_SES) && ppp_tag == htons(PPP_IP))) {	
		iph = (struct iphdr *)(skb->data + ETH_HLEN + vlan1_gap + vlan2_gap + pppoe_gap);
		daddr = ntohl(iph->daddr);
		saddr = ntohl(iph->saddr);
	}

	if (((br0Ip & brNetmask) == (daddr & brNetmask)) &&
	    ((daddr & brNetmask) == (saddr & brNetmask)))
		return 1;
	return 0;
}

int bridge_short_cut_rx(struct sk_buff *skb)
{
	struct iphdr *iph = NULL;
	uint32_t daddr;
	int ret;
	
	if (getBrLan == 0) {
		ret = get_bridge_info(); /*return 1 get br0 fail*/
		if (ret == 1)
			return 0;
	}

	iph = (struct iphdr *)(skb->data);
	daddr = ntohl(iph->daddr);
	if ((br0Ip & brNetmask) == (daddr & brNetmask))
		return 1;
	else 
		return 0;
}

/* push different VID for WiFi pseudo interface or USB external NIC */
uint32_t ppe_extif_rx_handler(struct sk_buff *skb)
{
#if defined(CONFIG_RA_HW_NAT_WIFI) || defined(CONFIG_RA_HW_NAT_NIC_USB)
	u16 vir_if_idx = 0;
#if defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
	int i = 0;
	int dev_match = 0;
#endif
	struct ethhdr *eth = (struct ethhdr *)skb_mac_header(skb);

	/* PPE can only handle IPv4/IPv6/PPP packets */
	if (((skb->protocol != htons(ETH_P_8021Q)) &&
	     (skb->protocol != htons(ETH_P_IP)) && (skb->protocol != htons(ETH_P_IPV6)) &&
	     (skb->protocol != htons(ETH_P_PPP_SES)) && (skb->protocol != htons(ETH_P_PPP_DISC))) ||
	    is_multicast_ether_addr(&eth->h_dest[0])) {
		return 1;
	}
#if !defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
	if (skb->dev == dst_port[DP_RA0])
		vir_if_idx = DP_RA0;
#if defined(CONFIG_RT2860V2_AP_MBSS) || defined(CONFIG_RTPCI_AP_MBSS) || defined(CONFIG_MBSS_SUPPORT)
	else if (skb->dev == dst_port[DP_RA1])
		vir_if_idx = DP_RA1;
	else if (skb->dev == dst_port[DP_RA2])
		vir_if_idx = DP_RA2;
	else if (skb->dev == dst_port[DP_RA3])
		vir_if_idx = DP_RA3;
	else if (skb->dev == dst_port[DP_RA4])
		vir_if_idx = DP_RA4;
	else if (skb->dev == dst_port[DP_RA5])
		vir_if_idx = DP_RA5;
	else if (skb->dev == dst_port[DP_RA6])
		vir_if_idx = DP_RA6;
	else if (skb->dev == dst_port[DP_RA7])
		vir_if_idx = DP_RA7;
	else if (skb->dev == dst_port[DP_RA8])
		vir_if_idx = DP_RA8;
	else if (skb->dev == dst_port[DP_RA9])
		vir_if_idx = DP_RA9;
	else if (skb->dev == dst_port[DP_RA10])
		vir_if_idx = DP_RA10;
	else if (skb->dev == dst_port[DP_RA11])
		vir_if_idx = DP_RA11;
	else if (skb->dev == dst_port[DP_RA12])
		vir_if_idx = DP_RA12;
	else if (skb->dev == dst_port[DP_RA13])
		vir_if_idx = DP_RA13;
	else if (skb->dev == dst_port[DP_RA14])
		vir_if_idx = DP_RA14;
	else if (skb->dev == dst_port[DP_RA15])
		vir_if_idx = DP_RA15;
#endif				/* CONFIG_RT2860V2_AP_MBSS || CONFIG_RTPCI_AP_MBSS // */
#if defined(CONFIG_RT2860V2_AP_WDS) || defined(CONFIG_RTPCI_AP_WDS) || defined(CONFIG_WDS_SUPPORT)
	else if (skb->dev == dst_port[DP_WDS0])
		vir_if_idx = DP_WDS0;
	else if (skb->dev == dst_port[DP_WDS1])
		vir_if_idx = DP_WDS1;
	else if (skb->dev == dst_port[DP_WDS2])
		vir_if_idx = DP_WDS2;
	else if (skb->dev == dst_port[DP_WDS3])
		vir_if_idx = DP_WDS3;
#endif
#if defined(CONFIG_RT2860V2_AP_APCLI) || defined(CONFIG_RTPCI_AP_APCLI) || defined(CONFIG_APCLI_SUPPORT)
	else if (skb->dev == dst_port[DP_APCLI0])
		vir_if_idx = DP_APCLI0;
#endif				/* CONFIG_RT2860V2_AP_APCLI // */
#if defined(CONFIG_RT2860V2_AP_MESH) || defined(CONFIG_RTPCI_AP_MESH)
	else if (skb->dev == dst_port[DP_MESH0])
		vir_if_idx = DP_MESH0;
#endif				/* CONFIG_RT2860V2_AP_MESH // */
#if defined(CONFIG_RTDEV_MII) || defined(CONFIG_RTDEV_USB) || \
defined(CONFIG_RTDEV_PCI) || defined(CONFIG_RTDEV) || \
defined(CONFIG_RLT_AP_SUPPORT)
	else if (skb->dev == dst_port[DP_RAI0])
		vir_if_idx = DP_RAI0;
#if defined(CONFIG_RT3090_AP_MBSS) || defined(CONFIG_RT5392_AP_MBSS) || \
defined(CONFIG_RT3572_AP_MBSS) || defined(CONFIG_RT5572_AP_MBSS) || \
defined(CONFIG_RT5592_AP_MBSS) || defined(CONFIG_RT3593_AP_MBSS) || \
defined(CONFIG_RTPCI_AP_MBSS)  || defined(CONFIG_MBSS_SUPPORT)
	else if (skb->dev == dst_port[DP_RAI1])
		vir_if_idx = DP_RAI1;
	else if (skb->dev == dst_port[DP_RAI2])
		vir_if_idx = DP_RAI2;
	else if (skb->dev == dst_port[DP_RAI3])
		vir_if_idx = DP_RAI3;
	else if (skb->dev == dst_port[DP_RAI4])
		vir_if_idx = DP_RAI4;
	else if (skb->dev == dst_port[DP_RAI5])
		vir_if_idx = DP_RAI5;
	else if (skb->dev == dst_port[DP_RAI6])
		vir_if_idx = DP_RAI6;
	else if (skb->dev == dst_port[DP_RAI7])
		vir_if_idx = DP_RAI7;
	else if (skb->dev == dst_port[DP_RAI8])
		vir_if_idx = DP_RAI8;
	else if (skb->dev == dst_port[DP_RAI9])
		vir_if_idx = DP_RAI9;
	else if (skb->dev == dst_port[DP_RAI10])
		vir_if_idx = DP_RAI10;
	else if (skb->dev == dst_port[DP_RAI11])
		vir_if_idx = DP_RAI11;
	else if (skb->dev == dst_port[DP_RAI12])
		vir_if_idx = DP_RAI12;
	else if (skb->dev == dst_port[DP_RAI13])
		vir_if_idx = DP_RAI13;
	else if (skb->dev == dst_port[DP_RAI14])
		vir_if_idx = DP_RAI14;
	else if (skb->dev == dst_port[DP_RAI15])
		vir_if_idx = DP_RAI15;
#endif				/* CONFIG_RTDEV_AP_MBSS || CONFIG_RTPCI_AP_MBSS // */
#endif				/* CONFIG_RTDEV_MII || CONFIG_RTDEV_USB || CONFIG_RTDEV_PCI */
#if defined(CONFIG_RT3090_AP_APCLI) || defined(CONFIG_RT5392_AP_APCLI) || \
defined(CONFIG_RT3572_AP_APCLI) || defined(CONFIG_RT5572_AP_APCLI) || \
defined(CONFIG_RT5592_AP_APCLI) || defined(CONFIG_RT3593_AP_APCLI) || \
defined(CONFIG_MT7610_AP_APCLI) || defined(CONFIG_APCLI_SUPPORT)
	else if (skb->dev == dst_port[DP_APCLII0])
		vir_if_idx = DP_APCLII0;
#endif				/* CONFIG_RTDEV_AP_APCLI // */
#if defined(CONFIG_RT3090_AP_WDS) || defined(CONFIG_RT5392_AP_WDS) || \
defined(CONFIG_RT3572_AP_WDS) || defined(CONFIG_RT5572_AP_WDS) || \
defined(CONFIG_RT5592_AP_WDS) || defined(CONFIG_RT3593_AP_WDS) || \
defined(CONFIG_MT7610_AP_WDS) || defined(CONFIG_WDS_SUPPORT)
	else if (skb->dev == dst_port[DP_WDSI0])
		vir_if_idx = DP_WDSI0;
	else if (skb->dev == dst_port[DP_WDSI1])
		vir_if_idx = DP_WDSI1;
	else if (skb->dev == dst_port[DP_WDSI2])
		vir_if_idx = DP_WDSI2;
	else if (skb->dev == dst_port[DP_WDSI3])
		vir_if_idx = DP_WDSI3;
#endif				/* CONFIG_RTDEV_AP_APCLI // */
#if defined(CONFIG_RT3090_AP_MESH) || defined(CONFIG_RT5392_AP_MESH) || \
defined(CONFIG_RT3572_AP_MESH) || defined(CONFIG_RT5572_AP_MESH) || \
defined(CONFIG_RT5592_AP_MESH) || defined(CONFIG_RT3593_AP_MESH) || \
defined(CONFIG_MT7610_AP_MESH)
	else if (skb->dev == dst_port[DP_MESHI0])
		vir_if_idx = DP_MESHI0;
#endif				/* CONFIG_RTDEV_AP_MESH // */
#if defined(CONFIG_RA_HW_NAT_NIC_USB)
	else if (skb->dev == dst_port[DP_PCI])
		vir_if_idx = DP_PCI;
	else if (skb->dev == dst_port[DP_USB])
		vir_if_idx = DP_USB;
#endif
	else
		pr_info("HNAT: %s The interface %s is unknown, vir_if_idx = %d\n", __func__, (char *)skb->dev, vir_if_idx);
#endif
	skb_set_network_header(skb, 0);

#if defined(CONFIG_SUPPORT_WLAN_OPTIMIZE)
	/*ppe_rx_parse_layer_info(skb);*/
	if (bridge_short_cut_rx(skb))
		return 1;	/* Bridge ==> sw path (rps) */
#endif
#if defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
	for (i = 0; i < MAX_IF_NUM; i++) {
		if (dst_port[i] == skb->dev) {
			vir_if_idx = i;
			dev_match = 1;
			/* pr_info("%s : Interface=%s, vir_if_idx=%x\n", __func__, skb->dev, vir_if_idx); */
			break;
		}
	}
	if (dev_match == 0) {
		if (debug_level >= 1)
			pr_info("%s UnKnown Interface, vir_if_idx=%x\n", __func__, vir_if_idx);
		return 1;
	}
#endif
	/* push vlan tag to stand for actual incoming interface, */
	/* so HNAT module can know the actual incoming interface from vlan id. */
	skb_push(skb, ETH_HLEN);/* pointer to layer2 header before calling hard_start_xmit */
#ifdef CONFIG_SUPPORT_WLAN_QOS
	set_qid(skb);
#endif
	skb->vlan_proto = htons(ETH_P_8021Q);
#ifdef CONFIG_RAETH_HW_VLAN_TX
	skb->vlan_tci |= VLAN_TAG_PRESENT;
	skb->vlan_tci |= vir_if_idx;
#else
	skb = __vlan_put_tag(skb, skb->vlan_proto, vir_if_idx);
#endif

	/* redirect to PPE */
	FOE_AI_HEAD(skb) = UN_HIT;
	FOE_AI_TAIL(skb) = UN_HIT;
	FOE_TAG_PROTECT_HEAD(skb) = TAG_PROTECT;
	FOE_TAG_PROTECT_TAIL(skb) = TAG_PROTECT;	
	FOE_MAGIC_TAG_HEAD(skb) = FOE_MAGIC_PPE;
	FOE_MAGIC_TAG_TAIL(skb) = FOE_MAGIC_PPE;

#if defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
	skb->dev = dst_port[DP_GMAC1];	/* we use GMAC1 to send the packet to PPE */
#else
	skb->dev = dst_port[DP_GMAC];	/* we use GMAC1 to send the packet to PPE */
#endif

#ifdef CONFIG_SUPPORT_WLAN_QOS
	if (debug_level >= 2)
		    	pr_info("skb->dev = %s\n", skb->dev);
	if ((skb->dev == NULL) || ((skb->dev != dst_port[DPORT_GMAC2]) && (skb->dev != dst_port[DP_GMAC1]))) {
#if defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
		skb->dev = dst_port[DP_GMAC1];	/* we use GMAC1 to send the packet to PPE */
#else
		skb->dev = dst_port[DP_GMAC];	/* we use GMAC1 to send the packet to PPE */
#endif
	}
#endif
	dev_queue_xmit(skb);
	return 0;
#else

	return 1;
#endif				/* CONFIG_RA_HW_NAT_WIFI || CONFIG_RA_HW_NAT_NIC_USB // */
}

uint32_t ppe_extif_pingpong_handler(struct sk_buff *skb)
{
#if defined(CONFIG_RA_HW_NAT_WIFI) || defined(CONFIG_RA_HW_NAT_NIC_USB)
	struct ethhdr *eth = NULL;
	u16 vir_if_idx = 0;
	struct net_device *dev;

	vir_if_idx = remove_vlan_tag(skb);

	/* recover to right incoming interface */
	if (vir_if_idx < MAX_IF_NUM && dst_port[vir_if_idx]) {
		skb->dev = dst_port[vir_if_idx];
	} else {
		if (debug_level >= 1)
			pr_info("%s : HNAT: unknown interface (vir_if_idx=%d)\n", __func__, vir_if_idx);
		return 1;
	}

	eth = (struct ethhdr *)skb_mac_header(skb);

	if (eth->h_dest[0] & 1) {
		if (skb->dev != NULL) {
			if (memcmp(eth->h_dest, skb->dev->broadcast, ETH_ALEN) == 0) {
				skb->pkt_type = PACKET_BROADCAST;
			} else {
				skb->pkt_type = PACKET_MULTICAST;
			}
		} else {
			if (debug_level >= 1)
				pr_info("%s skb->dev = NULL\n", __func__);
		}
	} else {

		skb->pkt_type = PACKET_OTHERHOST;
		for (vir_if_idx = 0; vir_if_idx < MAX_IF_NUM; vir_if_idx++) {
			dev = dst_port[vir_if_idx];
			if (dev !=NULL && memcmp(eth->h_dest, dev->dev_addr, ETH_ALEN) == 0) {
				skb->pkt_type = PACKET_HOST;
				break;
			}
		}
	}

#endif
	return 1;
}

uint32_t keep_alive_handler(struct sk_buff *skb, struct foe_entry *entry)
{
	struct ethhdr *eth = NULL;
	u16 eth_type = ntohs(skb->protocol);
	u32 vlan1_gap = 0;
	u32 vlan2_gap = 0;
	u32 pppoe_gap = 0;
	struct vlan_hdr *vh;
	struct iphdr *iph = NULL;
	struct tcphdr *th = NULL;
	struct udphdr *uh = NULL;

/* try to recover to original SMAC/DMAC, but we don't have such information.*/
/* just use SMAC as DMAC and set Multicast address as SMAC.*/
	eth = (struct ethhdr *)(skb->data - ETH_HLEN);

	hwnat_memcpy(eth->h_dest, eth->h_source, ETH_ALEN);
	hwnat_memcpy(eth->h_source, eth->h_dest, ETH_ALEN);
	eth->h_source[0] = 0x1;	/* change to multicast packet, make bridge not learn this packet */
	if (eth_type == ETH_P_8021Q) {
		vlan1_gap = VLAN_HLEN;
		vh = (struct vlan_hdr *)skb->data;

		if (ntohs(vh->h_vlan_TCI) == wan_vid) {
			/* It make packet like coming from LAN port */
			vh->h_vlan_TCI = htons(lan_vid);

		} else {
			/* It make packet like coming from WAN port */
			vh->h_vlan_TCI = htons(wan_vid);
		}

		if (ntohs(vh->h_vlan_encapsulated_proto) == ETH_P_PPP_SES) {
			pppoe_gap = 8;
		} else if (ntohs(vh->h_vlan_encapsulated_proto) == ETH_P_8021Q) {
			vlan2_gap = VLAN_HLEN;
			vh = (struct vlan_hdr *)(skb->data + VLAN_HLEN);

			/* VLAN + VLAN + PPPoE */
			if (ntohs(vh->h_vlan_encapsulated_proto) == ETH_P_PPP_SES) {
				pppoe_gap = 8;
			} else {
				/* VLAN + VLAN + IP */
				eth_type = ntohs(vh->h_vlan_encapsulated_proto);
			}
		} else {
			/* VLAN + IP */
			eth_type = ntohs(vh->h_vlan_encapsulated_proto);
		}
	}

	/* Only Ipv4 NAT need KeepAlive Packet to refresh iptable */
	if (eth_type == ETH_P_IP) {
		iph = (struct iphdr *)(skb->data + vlan1_gap + vlan2_gap + pppoe_gap);
		/* Recover to original layer 4 header */
		if (iph->protocol == IPPROTO_TCP) {
			th = (struct tcphdr *)((uint8_t *)iph + iph->ihl * 4);
			foe_to_org_tcphdr(entry, iph, th);

		} else if (iph->protocol == IPPROTO_UDP) {
			uh = (struct udphdr *)((uint8_t *)iph + iph->ihl * 4);
			foe_to_org_udphdr(entry, iph, uh);
		}
		/* Recover to original layer 3 header */
		foe_to_org_iphdr(entry, iph);
		skb->pkt_type = PACKET_HOST;
	} else if (eth_type == ETH_P_IPV6) {
		skb->pkt_type = PACKET_HOST;
	} else {
		skb->pkt_type = PACKET_HOST;
	}
/* Ethernet driver will call eth_type_trans() to update skb->pkt_type.*/
/* If(destination mac != my mac)*/
/*   skb->pkt_type=PACKET_OTHERHOST;*/
/* In order to pass ip_rcv() check, we change pkt_type to PACKET_HOST here*/
/*	skb->pkt_type = PACKET_HOST;*/
	return 1;
}

uint32_t keep_alive_old_pkt_handler(struct sk_buff *skb)
{
	struct ethhdr *eth = NULL;
	u16 vir_if_idx = 0;
	struct net_device *dev;

	if ((FOE_SP(skb) == 0) || (FOE_SP(skb) == 5)) {
		vir_if_idx = remove_vlan_tag(skb);
		/* recover to right incoming interface */
		if (vir_if_idx < MAX_IF_NUM && dst_port[vir_if_idx]) {
			skb->dev = dst_port[vir_if_idx];
		} else {
			pr_info("%s unknown If (vir_if_idx=%d)\n",  __func__, vir_if_idx);
			return 1;
		}
	}

	eth = (struct ethhdr *)skb_mac_header(skb);

	if (eth->h_dest[0] & 1) {
		if (ether_addr_equal(eth->h_dest, skb->dev->broadcast) == 0)
			skb->pkt_type = PACKET_BROADCAST;
		else
			skb->pkt_type = PACKET_MULTICAST;
	} else {
		skb->pkt_type = PACKET_OTHERHOST;
		for (vir_if_idx = 0; vir_if_idx < MAX_IF_NUM; vir_if_idx++) {
			dev = dst_port[vir_if_idx];
			if (dev && ether_addr_equal(eth->h_dest, dev->dev_addr) == 0) {
				skb->pkt_type = PACKET_HOST;
				break;
			}
		}
	}
	return 0;
}

int hitbind_force_to_cpu_handler(struct sk_buff *skb, struct foe_entry *entry)
{
#if defined(CONFIG_RAETH_QDMA)
	u16 vir_if_idx = 0;
	vir_if_idx = remove_vlan_tag(skb);
	if (vir_if_idx != 65535) {
		if (vir_if_idx >= FOE_4TB_SIZ) {
			pr_info("%s, entry_index error(%u)\n", __func__, vir_if_idx);
			vir_if_idx = FOE_ENTRY_NUM(skb);
			kfree_skb(skb);
			return 0;
		}
		entry = &ppe_foe_base[vir_if_idx];
	}
#endif
	if (IS_IPV4_HNAT(entry) || IS_IPV4_HNAPT(entry))
		skb->dev = dst_port[entry->ipv4_hnapt.act_dp];
#if defined(CONFIG_RA_HW_NAT_IPV6)
	else if (IS_IPV4_DSLITE(entry))
		skb->dev = dst_port[entry->ipv4_dslite.act_dp];
	else if (IS_IPV6_3T_ROUTE(entry))
		skb->dev = dst_port[entry->ipv6_3t_route.act_dp];
	else if (IS_IPV6_5T_ROUTE(entry))
		skb->dev = dst_port[entry->ipv6_5t_route.act_dp];
	else if (IS_IPV6_6RD(entry))
		skb->dev = dst_port[entry->ipv6_6rd.act_dp];
	else
		return 1;
#endif
	/* interface is unknown */
	if (!skb->dev) {
		if (debug_level >= 1)
			pr_info("%s, interface is unknown\n", __func__);
		kfree_skb(skb);
		return 0;
	}
	skb_set_network_header(skb, 0);
	skb_push(skb, ETH_HLEN);	/* pointer to layer2 header */
	dev_queue_xmit(skb);
	return 0;
}

int hitbind_force_mcast_to_wifi_handler(struct sk_buff *skb)
{
#if defined(CONFIG_RA_HW_NAT_WIFI) || defined(CONFIG_RA_HW_NAT_NIC_USB)
#if !defined(CONFIG_RA_HW_NAT_NIC_USB)
	int i = 0;
#endif
	struct sk_buff *skb2;

#if !defined(CONFIG_RAETH_GMAC2)
/*if we only use GMAC1, we need to use vlan id to identify LAN/WAN port*/
/*otherwise, CPU send untag packet to switch so we don't need to*/
/*remove vlan tag before sending to WiFi interface*/
	remove_vlan_tag(skb);	/* pointer to layer3 header */
#endif
	skb_set_network_header(skb, 0);
	skb_push(skb, ETH_HLEN);	/* pointer to layer2 header */

#if defined(CONFIG_RA_HW_NAT_WIFI)
	for (i = DP_RA0; i < MAX_WIFI_IF_NUM; i++) {
		if (dst_port[i]) {
			skb2 = skb_clone(skb, GFP_ATOMIC);

			if (!skb2)
				return -ENOMEM;

			skb2->dev = dst_port[i];
			dev_queue_xmit(skb2);
		}
	}
#elif defined(CONFIG_RA_HW_NAT_NIC_USB)
	if (dst_port[DP_PCI]) {
		skb2 = skb_clone(skb, GFP_ATOMIC);

		if (!skb2)
			return -ENOMEM;

		skb2->dev = dst_port[DP_PCI];
		dev_queue_xmit(skb2);
	}
#endif
#endif
	kfree_skb(skb);

	return 0;
}

int32_t ppe_rx_handler(struct sk_buff *skb)
{
	struct foe_entry *entry = &ppe_foe_base[FOE_ENTRY_NUM(skb)];
	struct ethhdr *eth = (struct ethhdr *)(skb->data - ETH_HLEN);
#if defined(CONFIG_RAETH_QDMA)
	struct vlan_ethhdr *veth;
#endif

#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
	unsigned int addr = 0;
	unsigned int src_ip = 0;
#endif
	if (debug_level >= 7)
		foe_dump_pkt(skb);

/*FP to-lan on/off*/
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
	if (pptp_fast_path && (skb->len != (124 - 14))) {
		int ret = 1000;
		/*remove pptp/l2tp header, tx to PPE */
		ret = hnat_pptp_lan(skb);
		/*ret 0, remove header ok */
		if (ret == 0)
			return ret;
	}
	if (l2tp_fast_path && (skb->len != (124 - 14))) {
		int ret = 1000;
		/*remove pptp/l2tp header, tx to PPE */
		ret = hnat_l2tp_lan(skb);
		if (ret == 0)
			return ret;
	}
#endif

#if defined(CONFIG_RAETH_QDMA)
	/* QDMA QoS remove CPU reason, we use special tag to identify force to CPU
	 * Notes: CPU reason & Entry ID fileds are invalid at this moment
	 */
	if (FOE_SP(skb) == 5) {
		veth = (struct vlan_ethhdr *)skb_mac_header(skb);

		if (veth->h_vlan_proto == 0x5678) {
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
			u16 vir_if_idx = 0;

			vir_if_idx = remove_vlan_tag(skb);
			entry = &ppe_foe_base[vir_if_idx];
#else
#if defined (CONFIG_HW_NAT_IPI)
			return HnatIPIForceCPU(skb);
#else
			return hitbind_force_to_cpu_handler(skb, entry);
#endif
#endif
		}
	}
#endif

	/* the incoming packet is from PCI or WiFi interface */
	//if (IS_IF_PCIE_WLAN_RX(skb)) {
		//return ppe_extif_rx_handler(skb);
	if (((FOE_MAGIC_TAG(skb) == FOE_MAGIC_PCI)
	     || (FOE_MAGIC_TAG(skb) == FOE_MAGIC_WLAN))) {
#if defined (CONFIG_HW_NAT_IPI)
		return HnatIPIExtIfHandler(skb);			
#else
		return ppe_extif_rx_handler(skb);
#endif
	} else if (FOE_AI(skb) == HIT_BIND_FORCE_TO_CPU) {
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
		if (pptp_fast_path) {
			/*to ppp0, add pptp/l2tp header and send out */
			int ret = 1000;
			/* pr_info("PPTP LAN->WAN  get bind packet!!\n"); */
			ret = hnat_pptp_wan(skb);
			return ret;
		} else if (l2tp_fast_path) {
			/*to ppp0, add pptp/l2tp header and send out */
			int ret = 1000;

			ret = hnat_l2tp_wan(skb);
			/* pr_info("L2TP LAN->WAN fast send bind packet and return %d!!\n", ret); */
			return ret;
		}
#endif

#if defined (CONFIG_HW_NAT_IPI)
		return HnatIPIForceCPU(skb);
#else
		return hitbind_force_to_cpu_handler(skb, entry);
#endif

		/* handle the incoming packet which came back from PPE */
#if defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || (CONFIG_ARCH_MT7623)
	} else if ((IS_IF_PCIE_WLAN_RX(skb) && ((FOE_SP(skb) == 0) || (FOE_SP(skb) == 5))) &&
		   (FOE_AI(skb) != HIT_BIND_KEEPALIVE_UC_OLD_HDR) &&
		   (FOE_AI(skb) != HIT_BIND_KEEPALIVE_MC_NEW_HDR) &&
		   (FOE_AI(skb) != HIT_BIND_KEEPALIVE_DUP_OLD_HDR)) {
#else
	} else if ((FOE_SP(skb) == 6) &&
		   (FOE_AI(skb) != HIT_BIND_KEEPALIVE_UC_OLD_HDR) &&
		   (FOE_AI(skb) != HIT_BIND_KEEPALIVE_MC_NEW_HDR) &&
		   (FOE_AI(skb) != HIT_BIND_KEEPALIVE_DUP_OLD_HDR)) {
#endif

#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
		/* PPTP/L2TP use this pingpong tech */
		/* pr_info("get fast_path ping pong skb_length is %d\n", skb->len); */
		if ((pptp_fast_path || l2tp_fast_path) && ((skb->len == 110) || (skb->len == 98))) {
			if (FOE_AI(skb) == UN_HIT) {
				dev_kfree_skb_any(skb);	/*avoid memory leak */
				return 0;
			}
			/* wan->lan ping-pong, pass up to tx handler for binding */
			/* pr_info("Parse ping pong packets FOE_AI(skb)= 0x%2x!!\n", FOE_AI(skb)); */
			/* get start addr for each layer */
			if (pptp_tolan_parse_layer_info(skb)) {
				memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
				dev_kfree_skb_any(skb);
				return 0;
			}
			if (debug_level >= 1)
				pr_info("ppe_fill_L2_info\n");
			/* Set Layer2 Info */
			if (ppe_fill_L2_info(skb, entry)) {
				memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
				dev_kfree_skb_any(skb);
				return 0;
			}

			if (debug_level >= 1)
				pr_info("ppe_fill_L3_info\n");
			/* Set Layer3 Info */
			if (ppe_fill_L3_info(skb, entry)) {
				memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
				dev_kfree_skb_any(skb);
				return 0;
			}

			if (debug_level >= 1)
				pr_info("ppe_fill_L4_info\n");
			/* Set Layer4 Info */
			if (ppe_fill_L4_info(skb, entry)) {
				memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
				dev_kfree_skb_any(skb);
				return 0;
			}

			/* Set force port info to 1  */
			/* if (ppe_setforce_port_info(skb, entry, gmac_no)) { */
			if (ppe_setforce_port_info(skb, entry, 1)) {
				memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
				dev_kfree_skb_any(skb);
				return 0;
			}
#if defined(CONFIG_RALINK_MT7621)
			entry->ipv4_hnapt.iblk2.fqos = 0;	/*not goes to QoS */
#endif
			/* Enter binding state */
			ppe_set_entry_bind(skb, entry);
			fast_bind = 1;
			addr =
			    ((htons(entry->ipv4_hnapt.sport) << 16) |
			     htons(entry->ipv4_hnapt.dport));
			src_ip = (htonl(entry->ipv4_hnapt.sip));
			pptp_l2tp_fdb_update(ppe_parse_result.iph.protocol, addr, src_ip,
					     FOE_ENTRY_NUM(skb));
			/*free pingpong packet */
			dev_kfree_skb_any(skb);
			return 0;
		}
#endif
		return ppe_extif_pingpong_handler(skb);
	} else if (FOE_AI(skb) == HIT_BIND_KEEPALIVE_UC_OLD_HDR) {
		if (debug_level >= 3)
			pr_info("Got HIT_BIND_KEEPALIVE_UC_OLD_HDR packet (hash index=%d)\n",
				 FOE_ENTRY_NUM(skb));
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
		if (pptp_fast_path) {
			dev_kfree_skb_any(skb);
			return 0;
		}
#endif
		return 1;
	} else if (FOE_AI(skb) == HIT_BIND_MULTICAST_TO_CPU ||
		   FOE_AI(skb) == HIT_BIND_MULTICAST_TO_GMAC_CPU) {
		return hitbind_force_mcast_to_wifi_handler(skb);
	} else if (FOE_AI(skb) == HIT_BIND_KEEPALIVE_MC_NEW_HDR) {
		if (debug_level >= 3) {
			pr_info("Got HIT_BIND_KEEPALIVE_MC_NEW_HDR packet (hash index=%d)\n",
				 FOE_ENTRY_NUM(skb));
		}
		if (keep_alive_handler(skb, entry))
			return 1;
	} else if (FOE_AI(skb) == HIT_BIND_KEEPALIVE_DUP_OLD_HDR) {
		if (debug_level >= 3)
			pr_info("RxGot HIT_BIND_KEEPALIVE_DUP_OLD_HDR packe (hash index=%d)\n",
				FOE_ENTRY_NUM(skb));

		keep_alive_old_pkt_handler(skb);
		/*change to multicast packet, make bridge not learn this packet */
		/*after kernel-2.6.36 src mac = multicast will drop by bridge,*/
		/*so we need recover correcet interface*/
		/*eth->h_source[0] = 0x1;*/

#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
		if (pptp_fast_path) {
			dev_kfree_skb_any(skb);
			return 0;
		}
#endif
		return 1;
	}
	return 1;
}

int32_t get_pppoe_sid(struct sk_buff *skb, uint32_t vlan_gap, u16 *sid, uint16_t *ppp_tag)
{
	struct pppoe_hdr *peh = NULL;

	peh = (struct pppoe_hdr *)(skb->data + ETH_HLEN + vlan_gap);

	if (debug_level >= 6) {
		NAT_PRINT("\n==============\n");
		NAT_PRINT(" Ver=%d\n", peh->ver);
		NAT_PRINT(" Type=%d\n", peh->type);
		NAT_PRINT(" Code=%d\n", peh->code);
		NAT_PRINT(" sid=%x\n", ntohs(peh->sid));
		NAT_PRINT(" Len=%d\n", ntohs(peh->length));
		NAT_PRINT(" tag_type=%x\n", ntohs(peh->tag[0].tag_type));
		NAT_PRINT(" tag_len=%d\n", ntohs(peh->tag[0].tag_len));
		NAT_PRINT("=================\n");
	}

	*ppp_tag = peh->tag[0].tag_type;
#if defined(CONFIG_RA_HW_NAT_IPV6)
	if (peh->ver != 1 || peh->type != 1 ||
	    (*ppp_tag != htons(PPP_IP) &&
	    *ppp_tag != htons(PPP_IPV6))) {
#else
	if (peh->ver != 1 || peh->type != 1 || *ppp_tag != htons(PPP_IP)) {
#endif
		return 1;
	}

	*sid = peh->sid;
	return 0;
}

/* HNAT_V2 can push special tag */
int32_t is_special_tag(uint16_t eth_type)
{
	/* Please modify this function to speed up the packet with special tag
	 * Ex:
	 *    Ralink switch = 0x81xx
	 *    Realtek switch = 0x8899
	 */
	if ((eth_type & 0x00FF) == htons(ETH_P_8021Q)) {	/* Ralink Special Tag: 0x81xx */
		ppe_parse_result.vlan_tag = eth_type;
		return 1;
	} else {
		return 0;
	}
}

int32_t is8021Q(uint16_t eth_type)
{
	if (eth_type == htons(ETH_P_8021Q)) {
		ppe_parse_result.vlan_tag = eth_type;
		return 1;
	} else {
		return 0;
	}
}

int32_t is_hw_vlan_tx(struct sk_buff *skb)
{
#ifdef CONFIG_RAETH_HW_VLAN_TX
	if (hwnat_vlan_tx_tag_present(skb)) {
		ppe_parse_result.vlan_tag = htons(ETH_P_8021Q);
		return 1;
	} else {
		return 0;
	}
#else
	return 0;
#endif
}

int32_t ppe_parse_layer_info(struct sk_buff *skb)
{
	struct vlan_hdr *vh = NULL;
	struct ethhdr *eth = NULL;
	struct iphdr *iph = NULL;
	struct ipv6hdr *ip6h = NULL;
	struct tcphdr *th = NULL;
	struct udphdr *uh = NULL;
	u8 ipv6_head_len = 0;
#ifdef CONFIG_RAETH_HW_VLAN_TX
	struct vlan_hdr pseudo_vhdr;
#endif
	memset(&ppe_parse_result, 0, sizeof(ppe_parse_result));

	eth = (struct ethhdr *)skb->data;
	hwnat_memcpy(ppe_parse_result.dmac, eth->h_dest, ETH_ALEN);
	hwnat_memcpy(ppe_parse_result.smac, eth->h_source, ETH_ALEN);
	ppe_parse_result.eth_type = eth->h_proto;
/* we cannot speed up multicase packets because both wire and wireless PCs might join same multicast group. */
#if (defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)) && defined(CONFIG_PPE_MCAST)
	if (is_multicast_ether_addr(&eth->h_dest[0]))
		ppe_parse_result.is_mcast = 1;
	else
		ppe_parse_result.is_mcast = 0;
#else
	if (is_multicast_ether_addr(&eth->h_dest[0]))
		return 1;
#endif
	if (is8021Q(ppe_parse_result.eth_type) ||
	    is_special_tag(ppe_parse_result.eth_type) || is_hw_vlan_tx(skb)) {
#ifdef CONFIG_RAETH_HW_VLAN_TX
		ppe_parse_result.vlan1_gap = 0;
		ppe_parse_result.vlan_layer++;
		pseudo_vhdr.h_vlan_TCI = htons(hwnat_vlan_tag_get(skb));
		pseudo_vhdr.h_vlan_encapsulated_proto = eth->h_proto;
		vh = (struct vlan_hdr *)&pseudo_vhdr;
#else
		ppe_parse_result.vlan1_gap = VLAN_HLEN;
		ppe_parse_result.vlan_layer++;
		vh = (struct vlan_hdr *)(skb->data + ETH_HLEN);
#endif
		ppe_parse_result.vlan1 = vh->h_vlan_TCI;
		/* VLAN + PPPoE */
		if (ntohs(vh->h_vlan_encapsulated_proto) == ETH_P_PPP_SES) {
			ppe_parse_result.pppoe_gap = 8;
			if (get_pppoe_sid(skb, ppe_parse_result.vlan1_gap,
					  &ppe_parse_result.pppoe_sid,
					  &ppe_parse_result.ppp_tag)) {
				return 1;
			}
			ppe_parse_result.eth_type = vh->h_vlan_encapsulated_proto;
			/* Double VLAN = VLAN + VLAN */
		} else if (is8021Q(vh->h_vlan_encapsulated_proto) ||
			   is_special_tag(vh->h_vlan_encapsulated_proto)) {
			ppe_parse_result.vlan2_gap = VLAN_HLEN;
			ppe_parse_result.vlan_layer++;
			vh = (struct vlan_hdr *)(skb->data + ETH_HLEN + ppe_parse_result.vlan1_gap);
			ppe_parse_result.vlan2 = vh->h_vlan_TCI;

			/* VLAN + VLAN + PPPoE */
			if (ntohs(vh->h_vlan_encapsulated_proto) == ETH_P_PPP_SES) {
				ppe_parse_result.pppoe_gap = 8;
				if (get_pppoe_sid
				    (skb,
				     (ppe_parse_result.vlan1_gap + ppe_parse_result.vlan2_gap),
				     &ppe_parse_result.pppoe_sid, &ppe_parse_result.ppp_tag)) {
					return 1;
				}
				ppe_parse_result.eth_type = vh->h_vlan_encapsulated_proto;
			} else if (is8021Q(vh->h_vlan_encapsulated_proto)) {
				/* VLAN + VLAN + VLAN */
				ppe_parse_result.vlan_layer++;
				vh = (struct vlan_hdr *)(skb->data + ETH_HLEN +
							 ppe_parse_result.vlan1_gap + VLAN_HLEN);

				/* VLAN + VLAN + VLAN */
				if (is8021Q(vh->h_vlan_encapsulated_proto))
					ppe_parse_result.vlan_layer++;
			} else {
				/* VLAN + VLAN + IP */
				ppe_parse_result.eth_type = vh->h_vlan_encapsulated_proto;
			}
		} else {
			/* VLAN + IP */
			ppe_parse_result.eth_type = vh->h_vlan_encapsulated_proto;
		}
	} else if (ntohs(ppe_parse_result.eth_type) == ETH_P_PPP_SES) {
		/* PPPoE + IP */
		ppe_parse_result.pppoe_gap = 8;
		if (get_pppoe_sid(skb, ppe_parse_result.vlan1_gap,
				  &ppe_parse_result.pppoe_sid,
				  &ppe_parse_result.ppp_tag)) {
			return 1;
		}
	}
	/* set layer2 start addr */

	skb_set_mac_header(skb, 0);

	/* set layer3 start addr */

	skb_set_network_header(skb, ETH_HLEN + ppe_parse_result.vlan1_gap +
			       ppe_parse_result.vlan2_gap + ppe_parse_result.pppoe_gap);

	/* set layer4 start addr */
	if ((ppe_parse_result.eth_type == htons(ETH_P_IP)) ||
	    (ppe_parse_result.eth_type == htons(ETH_P_PPP_SES) &&
	    (ppe_parse_result.ppp_tag == htons(PPP_IP)))) {
		iph = (struct iphdr *)skb_network_header(skb);
		memcpy(&ppe_parse_result.iph, iph, sizeof(struct iphdr));

		if (iph->protocol == IPPROTO_TCP) {
			skb_set_transport_header(skb, ETH_HLEN + ppe_parse_result.vlan1_gap +
						 ppe_parse_result.vlan2_gap +
						 ppe_parse_result.pppoe_gap + (iph->ihl * 4));
			th = (struct tcphdr *)skb_transport_header(skb);

			memcpy(&ppe_parse_result.th, th, sizeof(struct tcphdr));
			ppe_parse_result.pkt_type = IPV4_HNAPT;
			if (iph->frag_off & htons(IP_MF | IP_OFFSET))
				return 1;
		} else if (iph->protocol == IPPROTO_UDP) {
			skb_set_transport_header(skb, ETH_HLEN + ppe_parse_result.vlan1_gap +
						 ppe_parse_result.vlan2_gap +
						 ppe_parse_result.pppoe_gap + (iph->ihl * 4));
			uh = (struct udphdr *)skb_transport_header(skb);
			memcpy(&ppe_parse_result.uh, uh, sizeof(struct udphdr));
			ppe_parse_result.pkt_type = IPV4_HNAPT;
			if (iph->frag_off & htons(IP_MF | IP_OFFSET))
			{
				if (USE_3T_UDP_FRAG == 0)
					return 1;
			}
		} else if (iph->protocol == IPPROTO_GRE) {
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
#else
			/* do nothing */
			return 1;
#endif
		}
#if defined(CONFIG_RA_HW_NAT_IPV6)
		else if (iph->protocol == IPPROTO_IPV6) {
			ip6h = (struct ipv6hdr *)((uint8_t *)iph + iph->ihl * 4);
			memcpy(&ppe_parse_result.ip6h, ip6h, sizeof(struct ipv6hdr));

			if (ip6h->nexthdr == NEXTHDR_TCP) {
				skb_set_transport_header(skb, ETH_HLEN + ppe_parse_result.vlan1_gap +
							 ppe_parse_result.vlan2_gap +
							 ppe_parse_result.pppoe_gap +
							 (sizeof(struct ipv6hdr)));

				th = (struct tcphdr *)skb_transport_header(skb);

				memcpy(&ppe_parse_result.th.source, &th->source, sizeof(th->source));
				memcpy(&ppe_parse_result.th.dest, &th->dest, sizeof(th->dest));
			} else if (ip6h->nexthdr == NEXTHDR_UDP) {
				skb_set_transport_header(skb, ETH_HLEN + ppe_parse_result.vlan1_gap +
							 ppe_parse_result.vlan2_gap +
							 ppe_parse_result.pppoe_gap +
							 (sizeof(struct ipv6hdr)));

				uh = (struct udphdr *)skb_transport_header(skb);
				memcpy(&ppe_parse_result.uh.source, &uh->source, sizeof(uh->source));
				memcpy(&ppe_parse_result.uh.dest, &uh->dest, sizeof(uh->dest));
			}
			ppe_parse_result.pkt_type = IPV6_6RD;
#if defined(CONFIG_RALINK_MT7620) || defined(CONFIG_RALINK_MT7621)
/* identification field in outer ipv4 header is zero*/
/*after erntering binding state.*/
/* some 6rd relay router will drop the packet */
			return 1;
#endif
		}
#endif
		else {
			/* Packet format is not supported */
			return 1;
		}

	} else if (ppe_parse_result.eth_type == htons(ETH_P_IPV6) ||
		   (ppe_parse_result.eth_type == htons(ETH_P_PPP_SES) &&
		    ppe_parse_result.ppp_tag == htons(PPP_IPV6))) {
		ip6h = (struct ipv6hdr *)skb_network_header(skb);
		memcpy(&ppe_parse_result.ip6h, ip6h, sizeof(struct ipv6hdr));

		if (ip6h->nexthdr == NEXTHDR_TCP) {
			skb_set_transport_header(skb, ETH_HLEN + ppe_parse_result.vlan1_gap +
						 ppe_parse_result.vlan2_gap +
						 ppe_parse_result.pppoe_gap +
						 (sizeof(struct ipv6hdr)));

			th = (struct tcphdr *)skb_transport_header(skb);
			memcpy(&ppe_parse_result.th, th, sizeof(struct tcphdr));
			ppe_parse_result.pkt_type = IPV6_5T_ROUTE;
		} else if (ip6h->nexthdr == NEXTHDR_UDP) {
			skb_set_transport_header(skb, ETH_HLEN + ppe_parse_result.vlan1_gap +
						 ppe_parse_result.vlan2_gap +
						 ppe_parse_result.pppoe_gap +
						 (sizeof(struct ipv6hdr)));
			uh = (struct udphdr *)skb_transport_header(skb);
			memcpy(&ppe_parse_result.uh, uh, sizeof(struct udphdr));
			ppe_parse_result.pkt_type = IPV6_5T_ROUTE;
		} else if (ip6h->nexthdr == NEXTHDR_IPIP) {
			ipv6_head_len = sizeof(struct iphdr);
			memcpy(&ppe_parse_result.iph, ip6h + ipv6_head_len,
			       sizeof(struct iphdr));
			ppe_parse_result.pkt_type = IPV4_DSLITE;
		} else {
			ppe_parse_result.pkt_type = IPV6_3T_ROUTE;
		}

	} else {
		return 1;
	}

	if (debug_level >= 6) {
		pr_info("--------------\n");
		pr_info("DMAC:%02X:%02X:%02X:%02X:%02X:%02X\n",
			 ppe_parse_result.dmac[0], ppe_parse_result.dmac[1],
			 ppe_parse_result.dmac[2], ppe_parse_result.dmac[3],
			 ppe_parse_result.dmac[4], ppe_parse_result.dmac[5]);
		pr_info("SMAC:%02X:%02X:%02X:%02X:%02X:%02X\n",
			 ppe_parse_result.smac[0], ppe_parse_result.smac[1],
			 ppe_parse_result.smac[2], ppe_parse_result.smac[3],
			 ppe_parse_result.smac[4], ppe_parse_result.smac[5]);
		pr_info("Eth_Type=%x\n", ppe_parse_result.eth_type);
		if (ppe_parse_result.vlan1_gap > 0)
			pr_info("VLAN1 ID=%x\n", ntohs(ppe_parse_result.vlan1));

		if (ppe_parse_result.vlan2_gap > 0)
			pr_info("VLAN2 ID=%x\n", ntohs(ppe_parse_result.vlan2));

		if (ppe_parse_result.pppoe_gap > 0) {
			pr_info("PPPOE Session ID=%x\n", ppe_parse_result.pppoe_sid);
			pr_info("PPP Tag=%x\n", ntohs(ppe_parse_result.ppp_tag));
		}
		pr_info("PKT_TYPE=%s\n",
			 ppe_parse_result.pkt_type ==
			 0 ? "IPV4_HNAT" : ppe_parse_result.pkt_type ==
			 1 ? "IPV4_HNAPT" : ppe_parse_result.pkt_type ==
			 3 ? "IPV4_DSLITE" : ppe_parse_result.pkt_type ==
			 5 ? "IPV6_ROUTE" : ppe_parse_result.pkt_type == 7 ? "IPV6_6RD" : "Unknown");
		if (ppe_parse_result.pkt_type == IPV4_HNAT) {
			pr_info("SIP=%s\n", ip_to_str(ntohl(ppe_parse_result.iph.saddr)));
			pr_info("DIP=%s\n", ip_to_str(ntohl(ppe_parse_result.iph.daddr)));
			pr_info("TOS=%x\n", ntohs(ppe_parse_result.iph.tos));
		} else if (ppe_parse_result.pkt_type == IPV4_HNAPT) {
			pr_info("SIP=%s\n", ip_to_str(ntohl(ppe_parse_result.iph.saddr)));
			pr_info("DIP=%s\n", ip_to_str(ntohl(ppe_parse_result.iph.daddr)));
			pr_info("TOS=%x\n", ntohs(ppe_parse_result.iph.tos));

			if (ppe_parse_result.iph.protocol == IPPROTO_TCP) {
				pr_info("TCP SPORT=%d\n", ntohs(ppe_parse_result.th.source));
				pr_info("TCP DPORT=%d\n", ntohs(ppe_parse_result.th.dest));
			} else if (ppe_parse_result.iph.protocol == IPPROTO_UDP) {
				pr_info("UDP SPORT=%d\n", ntohs(ppe_parse_result.uh.source));
				pr_info("UDP DPORT=%d\n", ntohs(ppe_parse_result.uh.dest));
			}
		} else if (ppe_parse_result.pkt_type == IPV6_5T_ROUTE) {
			pr_info("ING SIPv6->DIPv6: %08X:%08X:%08X:%08X:%d-> %08X:%08X:%08X:%08X:%d\n",
			     ntohl(ppe_parse_result.ip6h.saddr.s6_addr32[0]), ntohl(ppe_parse_result.ip6h.saddr.s6_addr32[1]),
			     ntohl(ppe_parse_result.ip6h.saddr.s6_addr32[2]), ntohl(ppe_parse_result.ip6h.saddr.s6_addr32[3]),
			     ntohs(ppe_parse_result.th.source), ntohl(ppe_parse_result.ip6h.daddr.s6_addr32[0]),
			     ntohl(ppe_parse_result.ip6h.daddr.s6_addr32[1]), ntohl(ppe_parse_result.ip6h.daddr.s6_addr32[2]),
			     ntohl(ppe_parse_result.ip6h.daddr.s6_addr32[3]), ntohs(ppe_parse_result.th.dest));
		} else if (ppe_parse_result.pkt_type == IPV6_6RD) {
			/* fill in ipv4 6rd entry */
			pr_info("packet_type = IPV6_6RD\n");
			pr_info("SIP=%s\n", ip_to_str(ntohl(ppe_parse_result.iph.saddr)));
			pr_info("DIP=%s\n", ip_to_str(ntohl(ppe_parse_result.iph.daddr)));

			pr_info("Checksum=%x\n", ntohs(ppe_parse_result.iph.check));
			pr_info("ipV4 ID =%x\n", ntohs(ppe_parse_result.iph.id));
			pr_info("Flag=%x\n", ntohs(ppe_parse_result.iph.frag_off) >> 13);
			pr_info("TTL=%x\n", ppe_parse_result.iph.ttl);
			pr_info("TOS=%x\n", ppe_parse_result.iph.tos);
		}
	}

	return 0;
}

int32_t ppe_fill_L2_info(struct sk_buff *skb, struct foe_entry *entry)
{
	/* if this entry is already in binding state, skip it */
	if (entry->bfib1.state == BIND)
		return 1;

	/* Set VLAN Info - VLAN1/VLAN2 */
	/* Set Layer2 Info - DMAC, SMAC */
	if ((ppe_parse_result.pkt_type == IPV4_HNAT) || (ppe_parse_result.pkt_type == IPV4_HNAPT)) {
		if (entry->ipv4_hnapt.bfib1.pkt_type == IPV4_DSLITE) {	/* DS-Lite WAN->LAN */
#if defined(CONFIG_RA_HW_NAT_IPV6)
			foe_set_mac_hi_info(entry->ipv4_dslite.dmac_hi, ppe_parse_result.dmac);
			foe_set_mac_lo_info(entry->ipv4_dslite.dmac_lo, ppe_parse_result.dmac);
			foe_set_mac_hi_info(entry->ipv4_dslite.smac_hi, ppe_parse_result.smac);
			foe_set_mac_lo_info(entry->ipv4_dslite.smac_lo, ppe_parse_result.smac);
			entry->ipv4_dslite.vlan1 = ntohs(ppe_parse_result.vlan1);
			entry->ipv4_dslite.pppoe_id = ntohs(ppe_parse_result.pppoe_sid);
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
			entry->ipv4_dslite.vlan2_winfo = ntohs(ppe_parse_result.vlan2);
#else
			entry->ipv4_dslite.vlan2 = ntohs(ppe_parse_result.vlan2);
#endif
			entry->ipv4_dslite.etype = ntohs(ppe_parse_result.vlan_tag);
#else
			return 1;
#endif
		} else {	/* IPv4 WAN<->LAN */
			foe_set_mac_hi_info(entry->ipv4_hnapt.dmac_hi, ppe_parse_result.dmac);
			foe_set_mac_lo_info(entry->ipv4_hnapt.dmac_lo, ppe_parse_result.dmac);
			foe_set_mac_hi_info(entry->ipv4_hnapt.smac_hi, ppe_parse_result.smac);
			foe_set_mac_lo_info(entry->ipv4_hnapt.smac_lo, ppe_parse_result.smac);
			entry->ipv4_hnapt.vlan1 = ntohs(ppe_parse_result.vlan1);
#ifdef VPRI_REMARK_TEST
			/* VPRI=0x7 */
			entry->ipv4_hnapt.vlan1 |= (7 << 13);
#endif
			entry->ipv4_hnapt.pppoe_id = ntohs(ppe_parse_result.pppoe_sid);
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
			entry->ipv4_hnapt.vlan2_winfo = ntohs(ppe_parse_result.vlan2);
#else
			entry->ipv4_hnapt.vlan2 = ntohs(ppe_parse_result.vlan2);
#endif
			entry->ipv4_hnapt.etype = ntohs(ppe_parse_result.vlan_tag);
		}
	} else {
#if defined(CONFIG_RA_HW_NAT_IPV6)
		foe_set_mac_hi_info(entry->ipv6_5t_route.dmac_hi, ppe_parse_result.dmac);
		foe_set_mac_lo_info(entry->ipv6_5t_route.dmac_lo, ppe_parse_result.dmac);
		foe_set_mac_hi_info(entry->ipv6_5t_route.smac_hi, ppe_parse_result.smac);
		foe_set_mac_lo_info(entry->ipv6_5t_route.smac_lo, ppe_parse_result.smac);
		entry->ipv6_5t_route.vlan1 = ntohs(ppe_parse_result.vlan1);
		entry->ipv6_5t_route.pppoe_id = ntohs(ppe_parse_result.pppoe_sid);
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
		entry->ipv6_5t_route.vlan2_winfo = ntohs(ppe_parse_result.vlan2);
#else
		entry->ipv6_5t_route.vlan2 = ntohs(ppe_parse_result.vlan2);
#endif
		entry->ipv6_5t_route.etype = ntohs(ppe_parse_result.vlan_tag);
#else
		return 1;
#endif
	}

/* VLAN Layer:*/
/* 0: outgoing packet is untagged packet*/
/* 1: outgoing packet is tagged packet*/
/* 2: outgoing packet is double tagged packet*/
/* 3: outgoing packet is triple tagged packet*/
/* 4: outgoing packet is fourfold tagged packet*/
	entry->bfib1.vlan_layer = ppe_parse_result.vlan_layer;

#ifdef VLAN_LAYER_TEST
	/* outgoing packet is triple tagged packet */
	entry->bfib1.vlan_layer = 3;
	entry->ipv4_hnapt.vlan1 = 2;
	entry->ipv4_hnapt.vlan2 = 1;
#endif
	if (ppe_parse_result.pppoe_gap)
		entry->bfib1.psn = 1;
	else
		entry->bfib1.psn = 0;
#if defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)
	entry->ipv4_hnapt.bfib1.vpm = 1;	/* 0x8100 */
#else
#ifdef FORCE_UP_TEST
	entry->ipv4_hnapt.bfib1.dvp = 0;	/* let switch decide VPRI */
#else
	/* we set VID and VPRI in foe entry already, so we have to inform switch of keeping VPRI */
	entry->ipv4_hnapt.bfib1.dvp = 1;
#endif
#endif
	return 0;
}

#if defined(CONFIG_RA_HW_NAT_IPV6)
static uint16_t ppe_get_chkbase(struct iphdr *iph)
{
	u16 org_chksum = ntohs(iph->check);
	u16 org_tot_len = ntohs(iph->tot_len);
	u16 org_id = ntohs(iph->id);
	u16 chksum_tmp, tot_len_tmp, id_tmp;
	u32 tmp = 0;
	u16 chksum_base = 0;

	chksum_tmp = ~(org_chksum);
	tot_len_tmp = ~(org_tot_len);
	id_tmp = ~(org_id);
	tmp = chksum_tmp + tot_len_tmp + id_tmp;
	tmp = ((tmp >> 16) & 0x7) + (tmp & 0xFFFF);
	tmp = ((tmp >> 16) & 0x7) + (tmp & 0xFFFF);
	chksum_base = tmp & 0xFFFF;

	return chksum_base;
}
#endif

int32_t ppe_fill_L3_info(struct sk_buff *skb, struct foe_entry *entry)
{
	/* IPv4 or IPv4 over PPPoE */
	if ((ppe_parse_result.eth_type == htons(ETH_P_IP)) ||
	    (ppe_parse_result.eth_type == htons(ETH_P_PPP_SES) &&
	     ppe_parse_result.ppp_tag == htons(PPP_IP))) {
		if ((ppe_parse_result.pkt_type == IPV4_HNAT) ||
		    (ppe_parse_result.pkt_type == IPV4_HNAPT)) {
			if (entry->ipv4_hnapt.bfib1.pkt_type == IPV4_DSLITE) {	/* DS-Lite WAN->LAN */
#if defined(CONFIG_RA_HW_NAT_IPV6)
#if defined(CONFIG_RALINK_MT7620)
				entry->ipv4_dslite.bfib1.drm = 1;	/* switch will keep dscp */
#endif
#if defined (CONFIG_PPE_MIB)
				entry->ipv4_dslite.iblk2.mibf = 1;
#endif
				entry->ipv4_dslite.bfib1.rmt = 1;	/* remove outer IPv6 header */
				entry->ipv4_dslite.iblk2.dscp = ppe_parse_result.iph.tos;
#endif

			} else {
#if defined(CONFIG_RALINK_MT7620)
				entry->ipv4_hnapt.bfib1.drm = 1;	/* switch will keep dscp */
#endif
				entry->ipv4_hnapt.new_sip = ntohl(ppe_parse_result.iph.saddr);
				entry->ipv4_hnapt.new_dip = ntohl(ppe_parse_result.iph.daddr);
				entry->ipv4_hnapt.iblk2.dscp = ppe_parse_result.iph.tos;
#ifdef DSCP_REMARK_TEST
				entry->ipv4_hnapt.iblk2.dscp = 0xff;
#endif
#if defined (CONFIG_PPE_MIB)
				entry->ipv4_hnapt.iblk2.mibf = 1;
#endif
			}
		}
#if defined(CONFIG_RA_HW_NAT_IPV6)
		else if (ppe_parse_result.pkt_type == IPV6_6RD) {
			/* fill in ipv4 6rd entry */
			entry->ipv6_6rd.tunnel_sipv4 = ntohl(ppe_parse_result.iph.saddr);
			entry->ipv6_6rd.tunnel_dipv4 = ntohl(ppe_parse_result.iph.daddr);
			entry->ipv6_6rd.hdr_chksum = ppe_get_chkbase(&ppe_parse_result.iph);
			entry->ipv6_6rd.flag = (ntohs(ppe_parse_result.iph.frag_off) >> 13);
			entry->ipv6_6rd.ttl = ppe_parse_result.iph.ttl;
			entry->ipv6_6rd.dscp = ppe_parse_result.iph.tos;
#if defined (CONFIG_PPE_MIB)
			entry->ipv6_6rd.iblk2.mibf = 1;
#endif
#if defined(CONFIG_MACH_MT7623) || defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)
			reg_modify_bits(PPE_HASH_SEED, ntohs(ppe_parse_result.iph.id), 0, 16);
			entry->ipv6_6rd.per_flow_6rd_id = 1;
#endif
			/* IPv4 DS-Lite and IPv6 6RD shall be turn on by SW during initialization */
			entry->bfib1.pkt_type = IPV6_6RD;
		}
#endif
	}
#if defined(CONFIG_RA_HW_NAT_IPV6)
	/* IPv6 or IPv6 over PPPoE */
	else if (ppe_parse_result.eth_type == htons(ETH_P_IPV6) ||
		 (ppe_parse_result.eth_type == htons(ETH_P_PPP_SES) &&
		  ppe_parse_result.ppp_tag == htons(PPP_IPV6))) {
		if (ppe_parse_result.pkt_type == IPV6_3T_ROUTE ||
		    ppe_parse_result.pkt_type == IPV6_5T_ROUTE) {
			/* incoming packet is 6RD and need to remove outer IPv4 header */
			if (entry->bfib1.pkt_type == IPV6_6RD) {
#if defined(CONFIG_RALINK_MT7620)
				entry->ipv6_3t_route.bfib1.drm = 1;	/* switch will keep dscp */
#endif
				entry->ipv6_3t_route.bfib1.rmt = 1;
				entry->ipv6_3t_route.iblk2.dscp =
				    (ppe_parse_result.ip6h.
				     priority << 4 | (ppe_parse_result.ip6h.flow_lbl[0] >> 4));
#if defined (CONFIG_PPE_MIB)
				entry->ipv6_3t_route.iblk2.mibf = 1;
#endif
			} else {
				/* fill in ipv6 routing entry */
#if defined(CONFIG_RALINK_MT7620)
				entry->ipv6_3t_route.bfib1.drm = 1;	/* switch will keep dscp */
#endif
				entry->ipv6_3t_route.ipv6_sip0 =
				    ntohl(ppe_parse_result.ip6h.saddr.s6_addr32[0]);
				entry->ipv6_3t_route.ipv6_sip1 =
				    ntohl(ppe_parse_result.ip6h.saddr.s6_addr32[1]);
				entry->ipv6_3t_route.ipv6_sip2 =
				    ntohl(ppe_parse_result.ip6h.saddr.s6_addr32[2]);
				entry->ipv6_3t_route.ipv6_sip3 =
				    ntohl(ppe_parse_result.ip6h.saddr.s6_addr32[3]);

				entry->ipv6_3t_route.ipv6_dip0 =
				    ntohl(ppe_parse_result.ip6h.daddr.s6_addr32[0]);
				entry->ipv6_3t_route.ipv6_dip1 =
				    ntohl(ppe_parse_result.ip6h.daddr.s6_addr32[1]);
				entry->ipv6_3t_route.ipv6_dip2 =
				    ntohl(ppe_parse_result.ip6h.daddr.s6_addr32[2]);
				entry->ipv6_3t_route.ipv6_dip3 =
				    ntohl(ppe_parse_result.ip6h.daddr.s6_addr32[3]);
				entry->ipv6_3t_route.iblk2.dscp =
				    (ppe_parse_result.ip6h.
				     priority << 4 | (ppe_parse_result.ip6h.flow_lbl[0] >> 4));
#ifdef DSCP_REMARK_TEST
				entry->ipv6_3t_route.iblk2.dscp = 0xff;
#endif
#if defined (CONFIG_PPE_MIB)
				entry->ipv6_3t_route.iblk2.mibf = 1;
#endif
			}
		} else if (ppe_parse_result.pkt_type == IPV4_DSLITE) {
			/* fill in DSLite entry */
			entry->ipv4_dslite.tunnel_sipv6_0 =
			    ntohl(ppe_parse_result.ip6h.saddr.s6_addr32[0]);
			entry->ipv4_dslite.tunnel_sipv6_1 =
			    ntohl(ppe_parse_result.ip6h.saddr.s6_addr32[1]);
			entry->ipv4_dslite.tunnel_sipv6_2 =
			    ntohl(ppe_parse_result.ip6h.saddr.s6_addr32[2]);
			entry->ipv4_dslite.tunnel_sipv6_3 =
			    ntohl(ppe_parse_result.ip6h.saddr.s6_addr32[3]);

			entry->ipv4_dslite.tunnel_dipv6_0 =
			    ntohl(ppe_parse_result.ip6h.daddr.s6_addr32[0]);
			entry->ipv4_dslite.tunnel_dipv6_1 =
			    ntohl(ppe_parse_result.ip6h.daddr.s6_addr32[1]);
			entry->ipv4_dslite.tunnel_dipv6_2 =
			    ntohl(ppe_parse_result.ip6h.daddr.s6_addr32[2]);
			entry->ipv4_dslite.tunnel_dipv6_3 =
			    ntohl(ppe_parse_result.ip6h.daddr.s6_addr32[3]);
#if defined (CONFIG_PPE_MIB)
			entry->ipv4_dslite.iblk2.mibf = 1;
#endif

			memcpy(entry->ipv4_dslite.flow_lbl, ppe_parse_result.ip6h.flow_lbl,
			       sizeof(ppe_parse_result.ip6h.flow_lbl));
			entry->ipv4_dslite.priority = ppe_parse_result.ip6h.priority;
			entry->ipv4_dslite.hop_limit = ppe_parse_result.ip6h.hop_limit;
			/* IPv4 DS-Lite and IPv6 6RD shall be turn on by SW during initialization */
			entry->bfib1.pkt_type = IPV4_DSLITE;
		};
	}
#endif				/* CONFIG_RA_HW_NAT_IPV6 // */
	else
		return 1;

	return 0;
}

int32_t ppe_fill_L4_info(struct sk_buff *skb, struct foe_entry *entry)
{
#if defined(CONFIG_RALINK_RT3052)
	u32 phy_val;
#endif

	if (ppe_parse_result.pkt_type == IPV4_HNAPT) {
		/* DS-LIte WAN->LAN */
		if (entry->ipv4_hnapt.bfib1.pkt_type == IPV4_DSLITE)
			return 0;
		/* Set Layer4 Info - NEW_SPORT, NEW_DPORT */
		if (ppe_parse_result.iph.protocol == IPPROTO_TCP) {
			entry->ipv4_hnapt.new_sport = ntohs(ppe_parse_result.th.source);
			entry->ipv4_hnapt.new_dport = ntohs(ppe_parse_result.th.dest);
			entry->ipv4_hnapt.bfib1.udp = TCP;
		} else if (ppe_parse_result.iph.protocol == IPPROTO_UDP) {
			entry->ipv4_hnapt.new_sport = ntohs(ppe_parse_result.uh.source);
			entry->ipv4_hnapt.new_dport = ntohs(ppe_parse_result.uh.dest);
			entry->ipv4_hnapt.bfib1.udp = UDP;

#if defined(CONFIG_RALINK_MT7620)
			if ((reg_read(0xB000000C) & 0xf) < 0x5)
				return 1;
#else
			/* if UDP checksum is zero, it cannot be accelerated by HNAT */
			/* we found some protocols, such as IPSEC-NAT-T, are possible to hybrid
			 * udp zero checksum and non-zero checksum in the same session,
			 * so we disable HNAT acceleration for all UDP flows
			 */
			/* if(entry->ipv4_hnapt.new_sport==4500 && entry->ipv4_hnapt.new_dport==4500) */
#endif
		}
	} else if (ppe_parse_result.pkt_type == IPV4_HNAT) {
		/* do nothing */
	} else if (ppe_parse_result.pkt_type == IPV6_1T_ROUTE) {
		/* do nothing */
#if defined(CONFIG_RA_HW_NAT_IPV6)
	} else if (ppe_parse_result.pkt_type == IPV6_3T_ROUTE) {
		/* do nothing */
	} else if (ppe_parse_result.pkt_type == IPV6_5T_ROUTE) {
		/* do nothing */
#endif
	}

	return 0;
}

static void ppe_set_infoblk2(struct _info_blk2 *iblk2, uint32_t fpidx, uint32_t port_mg,
			     uint32_t port_ag)
{
#ifdef FORCE_UP_TEST
	u32 reg;

	iblk2->fp = 1;
	iblk2->up = 7;

	/* Replace 802.1Q priority by user priority */
	reg = reg_read(RALINK_ETH_SW_BASE + 0x2704);
	reg |= (0x1 << 11);
	reg_write(RALINK_ETH_SW_BASE + 0x2704, reg);
#endif

#if defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)
	/* we need to lookup another multicast table if this is multicast flow */
	if (ppe_parse_result.is_mcast) {
		iblk2->mcast = 1;
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
		if (fpidx == 3)
			fpidx = 0;	/* multicast flow not go to WDMA */
#endif
	} else {
		iblk2->mcast = 0;
	}

/* 0:PSE,1:GSW, 2:GMAC,4:PPE,5:QDMA,7=DROP */
	    iblk2->dp = fpidx;
#elif defined(CONFIG_RALINK_MT7620)
	iblk2->fpidx = fpidx;
#endif

#if !defined(CONFIG_RAETH_QDMA)
	iblk2->fqos = 0;	/* PDMA MODE should not goes to QoS */
#endif

#if defined(CONFIG_ARCH_MT7622)
	iblk2->acnt = port_ag;
#else
	iblk2->port_mg = port_mg;
	iblk2->port_ag = port_ag;
#endif
}

/*for 16 queue test*/
unsigned char queue_number;
/* Set force port info */
int32_t ppe_setforce_port_info(struct sk_buff *skb, struct foe_entry *entry, int gmac_no)
{
#if defined(CONFIG_RAETH_QDMA)
	unsigned int qidx;
	if (IS_IPV4_GRP(entry)) {
		if (skb->mark > 63)
			skb->mark = 0;
		qidx = M2Q_table[skb->mark];
#if defined(CONFIG_ARCH_MT7622) 
		entry->ipv4_hnapt.iblk2.qid1 = ((qidx & 0x30) >> 4);
#endif
		entry->ipv4_hnapt.iblk2.qid = (qidx & 0x0f);
#ifdef CONFIG_PSEUDO_SUPPORT
		if (lan_wan_separate == 1 && gmac_no == 2) {
			entry->ipv4_hnapt.iblk2.qid += 8;
#if defined(CONFIG_HW_SFQ)
			if (web_sfq_enable == 1 && (skb->mark == 2))
				entry->ipv4_hnapt.iblk2.qid = HWSFQUP;
#endif
		}
		if ((lan_wan_separate == 1) && (gmac_no == 1)) {
#if defined(CONFIG_HW_SFQ)
			if (web_sfq_enable == 1 && (skb->mark == 2))
				entry->ipv4_hnapt.iblk2.qid = HWSFQDL;
#endif
		}
#endif				/* end CONFIG_PSEUDO_SUPPORT */
	}
#if defined(CONFIG_RA_HW_NAT_IPV6)
	else if (IS_IPV6_GRP(entry)) {
		if (skb->mark > 63)
			skb->mark = 0;
#ifdef CONFIG_PSEUDO_SUPPORT
		qidx = M2Q_table[skb->mark];
#if defined(CONFIG_ARCH_MT7622) 
		entry->ipv6_3t_route.iblk2.qid1 = ((qidx & 0x30) >> 4);
#endif
		entry->ipv6_3t_route.iblk2.qid = (qidx & 0x0f);
		if (lan_wan_separate == 1 && gmac_no == 2) {
			entry->ipv6_3t_route.iblk2.qid += 8;
#if defined(CONFIG_HW_SFQ)
			if (web_sfq_enable == 1 && (skb->mark == 2))
				entry->ipv6_3t_route.iblk2.qid = HWSFQUP;
#endif
		}
		if ((lan_wan_separate == 1) && (gmac_no == 1)) {
#if defined(CONFIG_HW_SFQ)
			if (web_sfq_enable == 1 && (skb->mark == 2))
				entry->ipv6_3t_route.iblk2.qid = HWSFQDL;
#endif
		}
#endif
	}
#endif				/* CONFIG_RA_HW_NAT_IPV6 // */
#endif				/* CONFIG_RAETH_QDMA // */

	/* CPU need to handle traffic between WLAN/PCI and GMAC port */
#if(0)
	if ((strncmp(skb->dev->name, "ra", 2) == 0) ||
	    (strncmp(skb->dev->name, "wds", 3) == 0) ||
	    (strncmp(skb->dev->name, "mesh", 4) == 0) ||
	    (strncmp(skb->dev->name, "apcli", 5) == 0) ||
	    (strncmp(skb->dev->name, "wlan3", 5) == 0) ||
	    (skb->dev == dst_port[DP_PCI]) || (skb->dev == dst_port[DP_USB]) || (gmac_no == 777)) {
#endif
	if ((strncmp(skb->dev->name, "eth", 3) != 0)) {
#if defined(CONFIG_RA_HW_NAT_WIFI) || defined(CONFIG_RA_HW_NAT_NIC_USB) || defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
/*PPTP/L2TP LAN->WAN  bind to CPU*/
#if defined(CONFIG_RALINK_MT7620)
		if (IS_IPV4_GRP(entry))
			ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 6, 0x3F, 0x3F);	/* force to cpu */
#if defined(CONFIG_RA_HW_NAT_IPV6)
		else if (IS_IPV6_GRP(entry))
			ppe_set_infoblk2(&entry->ipv6_3t_route.iblk2, 6, 0x3F, 0x3F);	/* force to cpu */
#endif

#elif defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)
		if (IS_IPV4_GRP(entry)) {
#if defined(CONFIG_RAETH_QDMA)
			entry->ipv4_hnapt.bfib1.vpm = 0;	/* etype remark */

#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
			if (gmac_no == 3) {
				entry->ipv4_hnapt.iblk2.fqos = 0;/* MT7622 wifi hw_nat not support QoS */
				ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 3, 0x3F, 0x3F);	/* 3=WDMA */
			} else {
				ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 0, 0x3F, 0x3F);	/* 0=PDMA */
#if defined(CONFIG_WAN_TO_WLAN_SUPPORT_QOS)
				entry->ipv4_hnapt.iblk2.fqos = 1;
#else
				entry->ipv4_hnapt.iblk2.fqos = 0;
#endif
			}

#else
#if defined(CONFIG_RAETH_QDMATX_QDMARX)
			ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 5, 0x3F, 0x3F);	/* 5=QDMA */
#else
			ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 0, 0x3F, 0x3F);	/* 0=PDMA */
#endif
			if (FOE_SP(skb) == 5) {	/* wifi to wifi not go to pse port6 */
				entry->ipv4_hnapt.iblk2.fqos = 0;
			} else {
#if defined(CONFIG_WAN_TO_WLAN_SUPPORT_QOS)
				entry->ipv4_hnapt.iblk2.fqos = 1;
#else
				entry->ipv4_hnapt.iblk2.fqos = 0;
#endif
			}
#endif
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
			if (gmac_no == 3) {
				entry->ipv4_hnapt.iblk2.wdmaid = (FOE_WDMA_ID(skb) & 0x01);
				entry->ipv4_hnapt.iblk2.winfo = 1;
				entry->ipv4_hnapt.vlan2_winfo =
				    ((FOE_RX_ID(skb) & 0x03) << 14) | ((FOE_WC_ID(skb) & 0xff) << 6) |
				    (FOE_BSS_ID(skb) & 0x3f);
			}else {
				if (ppe_parse_result.vlan1 == 0) {
					entry->ipv4_hnapt.vlan1 = FOE_ENTRY_NUM(skb);
					entry->ipv4_hnapt.etype = ntohs(0x5678);
					entry->bfib1.vlan_layer = 1;
				}else if (ppe_parse_result.vlan2 == 0) {
					entry->ipv4_hnapt.vlan1 = FOE_ENTRY_NUM(skb);
					entry->ipv4_hnapt.etype = ntohs(0x5678);
					entry->ipv4_hnapt.vlan2_winfo = ntohs(ppe_parse_result.vlan1);
					entry->bfib1.vlan_layer = 2;
				} else {
					entry->ipv4_hnapt.vlan1 = FOE_ENTRY_NUM(skb);
					entry->ipv4_hnapt.etype = ntohs(0x5678);
					entry->ipv4_hnapt.vlan2_winfo = ntohs(ppe_parse_result.vlan1);
					entry->bfib1.vlan_layer = 3;
				}	
			}
#else
			if (ppe_parse_result.vlan1 == 0) {
				entry->ipv4_hnapt.vlan1 = FOE_ENTRY_NUM(skb);
				entry->ipv4_hnapt.etype = ntohs(0x5678);
				entry->bfib1.vlan_layer = 1;
			}else if (ppe_parse_result.vlan2 == 0) {
				entry->ipv4_hnapt.vlan1 = FOE_ENTRY_NUM(skb);
				entry->ipv4_hnapt.etype = ntohs(0x5678);
				entry->ipv4_hnapt.vlan2 = ntohs(ppe_parse_result.vlan1);
				entry->bfib1.vlan_layer = 2;
			} else {
				entry->ipv4_hnapt.vlan1 = FOE_ENTRY_NUM(skb);
				entry->ipv4_hnapt.etype = ntohs(0x5678);
				entry->ipv4_hnapt.vlan2 = ntohs(ppe_parse_result.vlan1);
				entry->bfib1.vlan_layer = 3;
			}
#endif

#else
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
			if (gmac_no == 3) {
				ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 3, 0x3F, 0x3F);	/* 3=WDMA */
				entry->ipv4_hnapt.iblk2.fqos = 0;
				entry->ipv4_hnapt.iblk2.wdmaid = (FOE_WDMA_ID(skb) & 0x01);
				entry->ipv4_hnapt.iblk2.winfo = 1;
				entry->ipv4_hnapt.vlan2_winfo =
				    ((FOE_RX_ID(skb) & 0x03) << 14) | ((FOE_WC_ID(skb) & 0xff) << 6) |
				    (FOE_BSS_ID(skb) & 0x3f);
			} else
				ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 0, 0x3F, 0x3F);	/* 0=CPU mt7622 fastpath*/
#else
			ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 0, 0x3F, 0x3F);	/* 0=CPU */
#endif
			entry->ipv4_hnapt.iblk2.fqos = 0;	/* PDMA MODE should not goes to QoS */
#endif
		}
#if defined(CONFIG_RA_HW_NAT_IPV6)
		else if (IS_IPV6_GRP(entry)) {
#if defined(CONFIG_RAETH_QDMA)
#if defined(CONFIG_RAETH_QDMATX_QDMARX)
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
			if (gmac_no == 3) {
				ppe_set_infoblk2(&entry->ipv6_3t_route.iblk2, 3, 0x3F, 0x3F);	/* 3=WDMA */
				entry->ipv6_3t_route.iblk2.fqos = 0;	/* MT7622 wifi hw_nat not support qos */
			} else
				ppe_set_infoblk2(&entry->ipv6_3t_route.iblk2, 5, 0x3F, 0x3F);	/* 0=CPU mt7622 fastpath */
				
#else
			ppe_set_infoblk2(&entry->ipv6_3t_route.iblk2, 5, 0x3F, 0x3F);	/* 5=QDMA */
			if (FOE_SP(skb) == 5) {
				entry->ipv6_3t_route.iblk2.fqos = 0;	/* wifi to wifi not go to pse port6 */
			} else {
#if defined(CONFIG_WAN_TO_WLAN_SUPPORT_QOS)
				entry->ipv6_3t_route.iblk2.fqos = 1;
#else
				entry->ipv6_3t_route.iblk2.fqos = 0;
#endif
			}
#endif
#else
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
			if (gmac_no == 3) {
				ppe_set_infoblk2(&entry->ipv6_3t_route.iblk2, 3, 0x3F, 0x3F);/* 3=WDMA */
				entry->ipv6_3t_route.iblk2.fqos = 0;	/* MT7622 wifi hw_nat not support qos */
			} else {
				ppe_set_infoblk2(&entry->ipv6_3t_route.iblk2, 0, 0x3F, 0x3F);
				/* 0=cpu mt7622 fastpath */
#if defined(CONFIG_WAN_TO_WLAN_SUPPORT_QOS)
				entry->ipv6_3t_route.iblk2.fqos = 1;
#else
				entry->ipv6_3t_route.iblk2.fqos = 0;
#endif
			}
#else
			ppe_set_infoblk2(&entry->ipv6_3t_route.iblk2, 0, 0x3F, 0x3F);	/* 0=PDMA */
			if (FOE_SP(skb) == 5) {
				entry->ipv6_3t_route.iblk2.fqos = 0;	/* wifi to wifi not go to pse port6 */
			} else {
#if defined(CONFIG_WAN_TO_WLAN_SUPPORT_QOS)
				entry->ipv6_3t_route.iblk2.fqos = 1;
#else
				entry->ipv6_3t_route.iblk2.fqos = 0;
#endif
			}
#endif
#endif

#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
			if (gmac_no == 3) {
				entry->ipv6_3t_route.iblk2.wdmaid = (FOE_WDMA_ID(skb) & 0x01);
				entry->ipv6_3t_route.iblk2.winfo = 1;
				entry->ipv6_3t_route.vlan2_winfo =
				    ((FOE_RX_ID(skb) & 0x03) << 14) | ((FOE_WC_ID(skb) & 0xff) << 6) |
				    (FOE_BSS_ID(skb) & 0x3f);
			}else {
				if (ppe_parse_result.vlan1 == 0) {
					entry->ipv6_3t_route.vlan1 = FOE_ENTRY_NUM(skb);
					entry->ipv6_3t_route.etype = ntohs(0x5678);
					entry->bfib1.vlan_layer = 1;
				}else if (ppe_parse_result.vlan2 == 0) {
					entry->ipv6_3t_route.vlan1 = FOE_ENTRY_NUM(skb);
					entry->ipv6_3t_route.etype = ntohs(0x5678);
					entry->ipv6_3t_route.vlan2_winfo = ntohs(ppe_parse_result.vlan1);
					entry->bfib1.vlan_layer = 2;
				} else {
					entry->ipv6_3t_route.vlan1 = FOE_ENTRY_NUM(skb);
					entry->ipv6_3t_route.etype = ntohs(0x5678);
					entry->ipv6_3t_route.vlan2_winfo = ntohs(ppe_parse_result.vlan1);
					entry->bfib1.vlan_layer = 3;
				}
			}
#else

				if (ppe_parse_result.vlan1 == 0) {
					entry->ipv6_3t_route.vlan1 = FOE_ENTRY_NUM(skb);
					entry->ipv6_3t_route.etype = ntohs(0x5678);
					entry->bfib1.vlan_layer = 1;
				}else if (ppe_parse_result.vlan2 == 0) {
					entry->ipv6_3t_route.vlan1 = FOE_ENTRY_NUM(skb);
					entry->ipv6_3t_route.etype = ntohs(0x5678);
					entry->ipv6_3t_route.vlan2 = ntohs(ppe_parse_result.vlan1);
					entry->bfib1.vlan_layer = 2;
				} else {
					entry->ipv6_3t_route.vlan1 = FOE_ENTRY_NUM(skb);
					entry->ipv6_3t_route.etype = ntohs(0x5678);
					entry->ipv6_3t_route.vlan2 = ntohs(ppe_parse_result.vlan1);
					entry->bfib1.vlan_layer = 3;
				}
#endif
#else
#if defined(CONFIG_ARCH_MT7622_WIFI_HW_NAT)
			if (gmac_no == 3) {
				ppe_set_infoblk2(&entry->ipv6_3t_route.iblk2, 3, 0x3F, 0x3F);/* 3=WDMA */
				entry->ipv6_3t_route.iblk2.wdmaid = (FOE_WDMA_ID(skb) & 0x01);
				entry->ipv6_3t_route.iblk2.winfo = 1;
				entry->ipv6_3t_route.vlan2_winfo =
				    ((FOE_RX_ID(skb) & 0x03) << 14) | ((FOE_WC_ID(skb) & 0xff) << 6) |
				    (FOE_BSS_ID(skb) & 0x3f);
			} else
				ppe_set_infoblk2(&entry->ipv6_3t_route.iblk2, 0, 0x3F, 0x3F);
				/* 0=cpu fastpath */
#else
			ppe_set_infoblk2(&entry->ipv6_3t_route.iblk2, 0, 0x3F, 0x3F);	/* 0=CPU */
#endif
#endif
		}
#endif
#else
		entry->ipv4_hnapt.iblk2.dp = 0;	/* cpu */
#endif				/* CONFIG_RALINK_MT7620 // */
#else
		return 1;
#endif				/* CONFIG_RA_HW_NAT_WIFI || CONFIG_RA_HW_NAT_NIC_USB // */

	} else {
#if defined(CONFIG_RAETH_QDMA)
		if (IS_IPV4_GRP(entry)) {
			if (((FOE_MAGIC_TAG(skb) == FOE_MAGIC_PCI) || 
		   	    (FOE_MAGIC_TAG(skb) == FOE_MAGIC_WLAN))) {
#if defined(CONFIG_WAN_TO_WLAN_SUPPORT_QOS)
				entry->ipv4_hnapt.iblk2.fqos = 1;
#else
				entry->ipv4_hnapt.iblk2.fqos = 0;
#endif
			} else {
				if (FOE_SP(skb) == 5) {
					entry->ipv4_hnapt.iblk2.fqos = 0;
				} else {
#if defined(CONFIG_QDMA_SUPPORT_QOS)
					entry->ipv4_hnapt.iblk2.fqos = 1;
#else
					entry->ipv4_hnapt.iblk2.fqos = 0;
#endif
				}
			}
		}
#if defined(CONFIG_RA_HW_NAT_IPV6)
		else if (IS_IPV6_GRP(entry)) {
			if (((FOE_MAGIC_TAG(skb) == FOE_MAGIC_PCI) || 
		   	    (FOE_MAGIC_TAG(skb) == FOE_MAGIC_WLAN))) {
#if defined(CONFIG_WAN_TO_WLAN_SUPPORT_QOS)
				entry->ipv6_5t_route.iblk2.fqos = 1;
#else
				entry->ipv6_5t_route.iblk2.fqos = 0;
#endif
			} else {
				if (FOE_SP(skb) == 5) {
					entry->ipv6_5t_route.iblk2.fqos = 0;
				} else {
#if defined(CONFIG_QDMA_SUPPORT_QOS)
					entry->ipv6_5t_route.iblk2.fqos = 1;
#else
					entry->ipv6_5t_route.iblk2.fqos = 0;
#endif
				}
			}
		}
#endif				/* CONFIG_RA_HW_NAT_IPV6 // */
#endif				/* CONFIG_RAETH_QDMA // */

		/* RT3883/MT7621 with 2xGMAC - Assuming GMAC2=WAN  and GMAC1=LAN */
#if defined(CONFIG_RAETH_GMAC2)
		if (gmac_no == 1) {
			if ((bind_dir == DOWNSTREAM_ONLY) || (bind_dir == BIDIRECTION)) {
#if defined(CONFIG_RALINK_MT7621) ||  defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)
				if (IS_IPV4_GRP(entry))
					ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 1, 0x3F, 1);
#if defined(CONFIG_RA_HW_NAT_IPV6)
				else if (IS_IPV6_GRP(entry))
					ppe_set_infoblk2(&entry->ipv6_5t_route.iblk2, 1, 0x3F, 1);
#endif
#else
				entry->ipv4_hnapt.iblk2.dp = 1;
#endif
			} else {
				return 1;
			}
		} else if (gmac_no == 2) {
			if ((bind_dir == UPSTREAM_ONLY) || (bind_dir == BIDIRECTION)) {
#if defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)
				if (IS_IPV4_GRP(entry))
					ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 2, 0x3F, 2);
#if defined(CONFIG_RA_HW_NAT_IPV6)
				else if (IS_IPV6_GRP(entry))
					ppe_set_infoblk2(&entry->ipv6_5t_route.iblk2, 2, 0x3F, 2);
#endif
#else
				entry->ipv4_hnapt.iblk2.dp = 2;
#endif
			} else {
				return 1;
			}
		}
#else				/* only one GMAC */
		if (IS_IPV4_GRP(entry)) {
#if defined(CONFIG_RAETH_SPECIAL_TAG)
#ifdef CONFIG_WAN_AT_P4
			if (((entry->ipv4_hnapt.vlan1 & VLAN_VID_MASK) == 1) ||
			    ((entry->ipv4_hnapt.vlan1 & VLAN_VID_MASK) == 2) ||
			    ((entry->ipv4_hnapt.vlan1 & VLAN_VID_MASK) == 3) ||
			    ((entry->ipv4_hnapt.vlan1 & VLAN_VID_MASK) == 4)) {
#else
			if (((entry->ipv4_hnapt.vlan1 & VLAN_VID_MASK) == 2) ||
			    ((entry->ipv4_hnapt.vlan1 & VLAN_VID_MASK) == 3) ||
			    ((entry->ipv4_hnapt.vlan1 & VLAN_VID_MASK) == 4) ||
			    ((entry->ipv4_hnapt.vlan1 & VLAN_VID_MASK) == 5)) {
#endif
#else
			if ((entry->ipv4_hnapt.vlan1 & VLAN_VID_MASK) == lan_vid) {
#endif
				if ((bind_dir == DOWNSTREAM_ONLY) || (bind_dir == BIDIRECTION)) {
#if defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)
					ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 1, 0x3F, 1);
#else				/* MT7620 */
					ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 8, 0x3F, 1);
#endif
				} else {
					return 1;
				}
#if defined(CONFIG_RAETH_SPECIAL_TAG)
#ifdef CONFIG_WAN_AT_P4
			} else if ((entry->ipv4_hnapt.vlan1 & VLAN_VID_MASK) == 5) {
#else
			} else if ((entry->ipv4_hnapt.vlan1 & VLAN_VID_MASK) == 1) {
#endif
#else
			} else if ((entry->ipv4_hnapt.vlan1 & VLAN_VID_MASK) == wan_vid) {
#endif
				if ((bind_dir == UPSTREAM_ONLY) || (bind_dir == BIDIRECTION)) {
#if defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622)|| defined(CONFIG_ARCH_MT7623)
					ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 1, 0x3F, 2);
#else				/* MT7620 */
					ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 8, 0x3F, 2);
#endif
				} else {
					return 1;
				}
			} else {	/* one-arm */
				ppe_set_infoblk2(&entry->ipv4_hnapt.iblk2, 8, 0x3F, 1);
			}
		}
#if defined(CONFIG_RA_HW_NAT_IPV6)
		else if (IS_IPV6_GRP(entry)) {
#if defined(CONFIG_RAETH_SPECIAL_TAG)
#ifdef CONFIG_WAN_AT_P4
			if (((entry->ipv6_5t_route.vlan1 & VLAN_VID_MASK) == 1) ||
			    ((entry->ipv6_5t_route.vlan1 & VLAN_VID_MASK) == 2) ||
			    ((entry->ipv6_5t_route.vlan1 & VLAN_VID_MASK) == 3) ||
			    ((entry->ipv6_5t_route.vlan1 & VLAN_VID_MASK) == 4)) {
#else
			if (((entry->ipv6_5t_route.vlan1 & VLAN_VID_MASK) == 2) ||
			    ((entry->ipv6_5t_route.vlan1 & VLAN_VID_MASK) == 3) ||
			    ((entry->ipv6_5t_route.vlan1 & VLAN_VID_MASK) == 4) ||
			    ((entry->ipv6_5t_route.vlan1 & VLAN_VID_MASK) == 5)) {
#endif
#else
			if ((entry->ipv6_5t_route.vlan1 & VLAN_VID_MASK) == lan_vid) {
#endif
				if ((bind_dir == DOWNSTREAM_ONLY) || (bind_dir == BIDIRECTION)) {
#if defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622)|| defined(CONFIG_ARCH_MT7623)
					ppe_set_infoblk2(&entry->ipv6_5t_route.iblk2, 1, 0x3F, 1);
#else				/* MT7620 */
					ppe_set_infoblk2(&entry->ipv6_5t_route.iblk2, 8, 0x3F, 1);
#endif
				} else {
					return 1;
				}
#if defined(CONFIG_RAETH_SPECIAL_TAG)
#ifdef CONFIG_WAN_AT_P4
			} else if ((entry->ipv6_5t_route.vlan1 & VLAN_VID_MASK) == 5) {
#else
			} else if ((entry->ipv6_5t_route.vlan1 & VLAN_VID_MASK) == 1) {
#endif
#else
			} else if ((entry->ipv6_5t_route.vlan1 & VLAN_VID_MASK) == wan_vid) {
#endif
				if ((bind_dir == UPSTREAM_ONLY) || (bind_dir == BIDIRECTION)) {
#if defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)
					ppe_set_infoblk2(&entry->ipv6_5t_route.iblk2, 1, 0x3F, 2);
#else				/* MT7620 */
					ppe_set_infoblk2(&entry->ipv6_5t_route.iblk2, 8, 0x3F, 2);
#endif
				} else {
					return 1;
				}
			} else {	/* one-arm */
#if defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622)|| defined(CONFIG_ARCH_MT7623)
				ppe_set_infoblk2(&entry->ipv6_5t_route.iblk2, 1, 0x3F, 1);
#else
				ppe_set_infoblk2(&entry->ipv6_5t_route.iblk2, 8, 0x3F, 1);
#endif
			}
		}
#endif				/* CONFIG_RA_HW_NAT_IPV6 // */

#endif
	}

	return 0;
}

uint32_t ppe_set_ext_if_num(struct sk_buff *skb, struct foe_entry *entry)
{
#if defined(CONFIG_RA_HW_NAT_WIFI) || defined(CONFIG_RA_HW_NAT_NIC_USB)
	u32 offset = 0;
#if !defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
	/* This is ugly soultion to support WiFi pseudo interface.
	 * Please double check the definition is the same as include/rt_linux.h
	 */
#define CB_OFF  10
#define RTMP_GET_PACKET_IF(skb)                 skb->cb[CB_OFF + 6]
#define MIN_NET_DEVICE_FOR_MBSSID               0x00
#define MIN_NET_DEVICE_FOR_WDS                  0x10
#define MIN_NET_DEVICE_FOR_APCLI                0x20
#define MIN_NET_DEVICE_FOR_MESH                 0x30

	/* Set actual output port info */
#if defined(CONFIG_RTDEV_MII) || defined(CONFIG_RTDEV_USB) || \
defined(CONFIG_RTDEV_PCI) || defined(CONFIG_RTDEV)
	if (strncmp(skb->dev->name, "rai", 3) == 0) {
#if defined(CONFIG_RT3090_AP_MESH) || defined(CONFIG_RT5392_AP_MESH) || \
defined(CONFIG_RT3572_AP_MESH) || defined(CONFIG_RT5572_AP_MESH) || \
defined(CONFIG_RT5592_AP_MESH) || defined(CONFIG_RT3593_AP_MESH)
		if (RTMP_GET_PACKET_IF(skb) >= MIN_NET_DEVICE_FOR_MESH)
			offset = (RTMP_GET_PACKET_IF(skb) - MIN_NET_DEVICE_FOR_MESH + DP_MESHI0);
		else
			offset = RTMP_GET_PACKET_IF(skb) + DP_RAI0;
#endif				/* CONFIG_RTDEV_AP_MESH // */

#if defined(CONFIG_RT3090_AP_APCLI) || defined(CONFIG_RT5392_AP_APCLI) || \
defined(CONFIG_RT3572_AP_APCLI) || defined(CONFIG_RT5572_AP_APCLI) || \
defined(CONFIG_RT5592_AP_APCLI) || defined(CONFIG_RT3593_AP_APCLI) || \
defined(CONFIG_MT7610_AP_APCLI) || defined(CONFIG_APCLI_SUPPORT)
		if (RTMP_GET_PACKET_IF(skb) >= MIN_NET_DEVICE_FOR_APCLI)
			offset = (RTMP_GET_PACKET_IF(skb) - MIN_NET_DEVICE_FOR_APCLI + DP_APCLII0);
		else
			offset = RTMP_GET_PACKET_IF(skb) + DP_RAI0;
#endif				/* CONFIG_RTDEV_AP_APCLI // */
#if defined(CONFIG_RT3090_AP_WDS) || defined(CONFIG_RT5392_AP_WDS) || \
		defined(CONFIG_RT3572_AP_WDS) || defined(CONFIG_RT5572_AP_WDS) || \
		defined(CONFIG_RT5592_AP_WDS) || defined(CONFIG_RT3593_AP_WDS) || \
		defined(CONFIG_MT7610_AP_WDS) || defined(CONFIG_WDS_SUPPORT)
		if (RTMP_GET_PACKET_IF(skb) >= MIN_NET_DEVICE_FOR_WDS)
			offset = (RTMP_GET_PACKET_IF(skb) - MIN_NET_DEVICE_FOR_WDS + DP_WDSI0);
		else
			offset = RTMP_GET_PACKET_IF(skb) + DP_RAI0;
#endif				/* CONFIG_RTDEV_AP_WDS // */

	} else
#endif				/* CONFIG_RTDEV_MII || CONFIG_RTDEV_USB || CONFIG_RTDEV_PCI || CONFIG_RTDEV */

	if (strncmp(skb->dev->name, "ra", 2) == 0) {
#if defined(CONFIG_RT2860V2_AP_MESH)
		if (RTMP_GET_PACKET_IF(skb) >= MIN_NET_DEVICE_FOR_MESH)
			offset = (RTMP_GET_PACKET_IF(skb) - MIN_NET_DEVICE_FOR_MESH + DP_MESH0);
		else
			offset = RTMP_GET_PACKET_IF(skb) + DP_RA0;
#endif				/* CONFIG_RT2860V2_AP_MESH // */
#if defined(CONFIG_RT2860V2_AP_APCLI) || defined(CONFIG_APCLI_SUPPORT)
		if (RTMP_GET_PACKET_IF(skb) >= MIN_NET_DEVICE_FOR_APCLI)
			offset = (RTMP_GET_PACKET_IF(skb) - MIN_NET_DEVICE_FOR_APCLI + DP_APCLI0);
		else
			offset = RTMP_GET_PACKET_IF(skb) + DP_RA0;
#endif				/* CONFIG_RT2860V2_AP_APCLI // */
#if defined(CONFIG_RT2860V2_AP_WDS) || defined(CONFIG_WDS_SUPPORT)
		if (RTMP_GET_PACKET_IF(skb) >= MIN_NET_DEVICE_FOR_WDS)
			offset = (RTMP_GET_PACKET_IF(skb) - MIN_NET_DEVICE_FOR_WDS + DP_WDS0);
		else
			offset = RTMP_GET_PACKET_IF(skb) + DP_RA0;
#endif				/* CONFIG_RT2860V2_AP_WDS // */
	}
#if 0
	else if (strncmp(skb->dev->name, "eth0", 4) == 0)
		offset = DP_GMAC;
#ifdef CONFIG_RAETH_GMAC2
	else if (strncmp(skb->dev->name, "eth1", 4) == 0)
		offset = DP_GMAC2;
#endif
#else
#if defined(CONFIG_RA_HW_NAT_NIC_USB)
	else if (strncmp(skb->dev->name, "eth0", 4) == 0)
		offset = DP_PCI;
	else if (strncmp(skb->dev->name, "eth1", 4) == 0)
		offset = DP_USB;
#endif				/* CONFIG_RA_HW_NAT_NIC_USB // */
	else if (strncmp(skb->dev->name, "eth2", 4) == 0)
		offset = DP_GMAC;
#ifdef CONFIG_RAETH_GMAC2
	else if (strncmp(skb->dev->name, "eth3", 4) == 0)
		offset = DP_GMAC2;
#endif
#endif
	else {
		if (pr_debug_ratelimited())
			pr_info("HNAT: unknown interface %s\n", skb->dev->name);
		return 1;
	}
#endif				/* !(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH) */
#if defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
	u32 i = 0;
	int dev_match = 0;

	for (i = 0; i < MAX_IF_NUM; i++) {
		if (dst_port[i] == skb->dev) {
			offset = i;
			dev_match = 1;
			if (debug_level >= 1)
				pr_info("dev match offset, name=%s ifined=%x\n", skb->dev->name, i);
			break;
		}
	}
	if (dev_match == 0) {
		if (debug_level >= 1)
			pr_info("%s UnKnown Interface, offset =%x\n", __func__, i);
		return 1;
	}
#endif				/* (CONFIG_RA_HW_NAT_WIFI_NEW_ARCH) */

	if (IS_IPV4_HNAT(entry) || IS_IPV4_HNAPT(entry)) {
		entry->ipv4_hnapt.act_dp = offset;
#if defined(CONFIG_ARCH_MT7622)
		entry->ipv4_hnapt.iblk2.acnt = offset;
#else
		entry->ipv4_hnapt.iblk2.port_mg = 0x3f;
		entry->ipv4_hnapt.iblk2.port_ag = offset;
#endif
	}
#if defined(CONFIG_RA_HW_NAT_IPV6)
	else if (IS_IPV4_DSLITE(entry)) {
		entry->ipv4_dslite.act_dp = offset;
#if defined(CONFIG_ARCH_MT7622)
		entry->ipv4_dslite.iblk2.acnt = offset;
#else
		entry->ipv4_dslite.iblk2.port_mg = 0x3f;
		entry->ipv4_dslite.iblk2.port_ag = offset;
#endif
	} else if (IS_IPV6_3T_ROUTE(entry)) {
		entry->ipv6_3t_route.act_dp = offset;
#if defined(CONFIG_ARCH_MT7622)
		entry->ipv6_3t_route.iblk2.acnt = offset;
#else
		entry->ipv6_3t_route.iblk2.port_mg = 0x3f;
		entry->ipv6_3t_route.iblk2.port_ag = offset;
#endif
	} else if (IS_IPV6_5T_ROUTE(entry)) {
		entry->ipv6_5t_route.act_dp = offset;
#if defined(CONFIG_ARCH_MT7622)
		entry->ipv6_5t_route.iblk2.acnt = offset;
#else
		entry->ipv6_5t_route.iblk2.port_mg = 0x3f;
		entry->ipv6_5t_route.iblk2.port_ag = offset;
#endif
	} else if (IS_IPV6_6RD(entry)) {
		entry->ipv6_6rd.act_dp = offset;
#if defined(CONFIG_ARCH_MT7622)
		entry->ipv6_6rd.iblk2.acnt = offset;
#else
		entry->ipv6_6rd.iblk2.port_mg = 0x3f;
		entry->ipv6_6rd.iblk2.port_ag = offset;
#endif
	} else {
		return 1;
	}
#endif				/* CONFIG_RA_HW_NAT_IPV6 // */
#endif				/* CONFIG_RA_HW_NAT_WIFI || CONFIG_RA_HW_NAT_NIC_USB // */

	return 0;
}

void ppe_set_entry_bind(struct sk_buff *skb, struct foe_entry *entry)
{
	u32 current_time;
	/* Set Current time to time_stamp field in information block 1 */
	current_time = reg_read(FOE_TS) & 0xFFFF;
	entry->bfib1.time_stamp = (uint16_t)current_time;

	/* Ipv4: TTL / Ipv6: Hot Limit filed */
	entry->ipv4_hnapt.bfib1.ttl = DFL_FOE_TTL_REGEN;
	/* enable cache by default */
	entry->ipv4_hnapt.bfib1.cah = 1;

#if defined(CONFIG_RA_HW_NAT_ACL2UP_HELPER)
	/*set user priority */
	entry->ipv4_hnapt.iblk2.up = FOE_SP(skb);
	entry->ipv4_hnapt.iblk2.fp = 1;
#endif

#if defined(CONFIG_RA_HW_NAT_PACKET_SAMPLING)
	entry->ipv4_hnapt.bfib1.ps = 1;
#endif

#if defined(CONFIG_RA_HW_NAT_PREBIND)
	entry->udib1.preb = 1;
#else
	/* Change Foe Entry State to Binding State */
	entry->bfib1.state = BIND;
	/* Dump Binding Entry */
	if (debug_level >= 1)
		foe_dump_entry(FOE_ENTRY_NUM(skb));
#endif

	/*make sure wrie dram correct*/
	wmb();
}

#if defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
void ppe_dev_reg_handler(struct net_device *dev)
{
	int i;

	/* skip apcli interface */
	if (strncmp(dev->name, "apcli", 5) == 0)
		return;

	for (i = 0; i < MAX_IF_NUM; i++) {
		if (dst_port[i] == dev) {
			pr_debug("%s : %s dst_port table has beed registered(%d)\n", __func__, dev->name, i);
			return;
		}
		if (dst_port[i] == NULL) {
			dst_port[i] = dev;
			break;
		}
	}
	pr_info("%s : ineterface %s register (%d)\n", __func__, dev->name, i);
}

void ppe_dev_unreg_handler(struct net_device *dev)
{
	int i;
	for (i = 0; i < MAX_IF_NUM; i++) {
		if (dst_port[i] == dev) {
			dst_port[i] = NULL;
			break;
		}
	}
	pr_info("%s : ineterface %s set null (%d)\n", __func__, dev->name, i);
}
#endif

#if defined (CONFIG_HW_NAT_SEMI_AUTO_MODE)
int get_done_bit(struct sk_buff *skb, struct foe_entry * entry)
{
	int done_bit;
	if (IS_IPV4_HNAT(entry) || IS_IPV4_HNAPT(entry)) {
		done_bit = entry->ipv4_hnapt.resv1;
	}
#if defined (CONFIG_HNAT_V2)
#if defined (CONFIG_RA_HW_NAT_IPV6)
	else if (IS_IPV4_DSLITE(entry)) {
		done_bit = entry->ipv4_dslite.resv1;
	} else if (IS_IPV6_3T_ROUTE(entry)) {
		done_bit = entry->ipv6_3t_route.resv1;
	} else if (IS_IPV6_5T_ROUTE(entry)) {
		done_bit = entry->ipv6_5t_route.resv1;
	} else if (IS_IPV6_6RD(entry)) {
		done_bit = entry->ipv6_6rd.resv1;
	} else {
		printk("get packet format something wrong\n");
		return 0;
	}
#endif
#endif
	if ((done_bit != 0) && (done_bit !=1)){
		printk("done bit something wrong, done_bit = %d\n", done_bit);
		done_bit = 0;
	}
	//printk("index = %d, done_bit=%d\n", FOE_ENTRY_NUM(skb), done_bit);
	return done_bit;
}

void set_ppe_table_done(struct foe_entry * entry)
{
	if (IS_IPV4_HNAT(entry) || IS_IPV4_HNAPT(entry)) {
		entry->ipv4_hnapt.resv1 = 1;
	}
#if defined (CONFIG_HNAT_V2)
#if defined (CONFIG_RA_HW_NAT_IPV6)
	else if (IS_IPV4_DSLITE(entry)) {
		entry->ipv4_dslite.resv1 = 1;
	} else if (IS_IPV6_3T_ROUTE(entry)) {
		entry->ipv6_3t_route.resv1 = 1;
	} else if (IS_IPV6_5T_ROUTE(entry)) {
		entry->ipv6_5t_route.resv1 = 1;
	} else if (IS_IPV6_6RD(entry)) {
		entry->ipv6_6rd.resv1 = 1;
	} else {
		printk("set packet format something wrong\n");
	}
#endif
#endif
}
#endif

int get_skb_interface(struct sk_buff *skb)
{
	if ((strncmp(skb->dev->name, "rai", 3) == 0) ||
	    (strncmp(skb->dev->name, "apclii", 6) == 0) ||
	    (strncmp(skb->dev->name, "wdsi", 4) == 0) ||
	    (strncmp(skb->dev->name, "wlan", 4) == 0))
		return 1;
	else
		return 0;
}
int32_t ppe_tx_handler(struct sk_buff *skb, int gmac_no)
{
	struct foe_entry *entry;
#if defined(CONFIG_RA_HW_NAT_PACKET_SAMPLING)
	struct ps_entry *ps_entry;
#endif
	u8 which_region;
#if defined(CONFIG_PPE_MIB)
	int count = 100000;
#endif
	which_region = tx_decide_which_region(skb);
	if (which_region == ALL_INFO_ERROR) {
		if (pr_debug_ratelimited())
			pr_info("ppe_tx_handler : ALL_INFO_ERROR\n");
		return 1;
	}

	entry = &ppe_foe_base[FOE_ENTRY_NUM(skb)];
#if defined(CONFIG_RA_HW_NAT_PACKET_SAMPLING)
	ps_entry = &ppe_ps_base[FOE_ENTRY_NUM(skb)];
#endif
	if (FOE_ENTRY_NUM(skb) == 0x3fff) {
		/* pr_info("FOE_ENTRY_NUM(skb)=%x\n", FOE_ENTRY_NUM(skb)); */
		return 1;
	}
	
	if (FOE_ENTRY_NUM(skb) >= FOE_4TB_SIZ) {
		/* pr_info("FOE_ENTRY_NUM(skb)=%x\n", FOE_ENTRY_NUM(skb)); */
		return 1;
	}

	 /* Packet is interested by ALG?*/
	 /* Yes: Don't enter binind state*/
	 /* No: If flow rate exceed binding threshold, enter binding state.*/
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
	if (pptp_fast_path) {
		if (FOE_MAGIC_TAG(skb) == FOE_MAGIC_FASTPATH) {
			FOE_MAGIC_TAG(skb) = 0;
			if (debug_level >= 1)
				/* pr_info("ppe_tx_handler FOE_MAGIC_FASTPATH\n"); */
			hash_cnt++;
			if ((FOE_AI(skb) == HIT_UNBIND_RATE_REACH) &&
			    (skb->len > 128) && (hash_cnt % 32 == 1)) {
				send_hash_pkt(skb);
			}
			memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
			return 1;
		}
	}

/*WAN->LAN accelerate*/
	if (l2tp_fast_path) {
		if (FOE_MAGIC_TAG(skb) == FOE_MAGIC_FASTPATH) {
			FOE_MAGIC_TAG(skb) = 0;
			if (debug_level >= 1)
				pr_info("ppe_tx_handler FOE_MAGIC_FASTPATH\n");
			hash_cnt++;
			/* if((FOE_AI(skb) == HIT_UNBIND_RATE_REACH) && (skb->len > 1360)) */
			if ((FOE_AI(skb) == HIT_UNBIND_RATE_REACH) &&
			    (skb->len > 1360) && (hash_cnt % 32 == 1)) {
				/* pr_info("hash_cnt is %d\n", hash_cnt); */
				send_L2TP_hash_pkt(skb);
			}
			memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
			return 1;
		}
	}
	if (IS_MAGIC_TAG_PROTECT_VALID(skb) &&
	    (FOE_AI(skb) == HIT_UNBIND_RATE_REACH) &&
	    (FOE_ALG(skb) == 0)) {
		struct iphdr *iph = NULL;
		struct udphdr *uh = NULL;

		iph = (struct iphdr *)(skb->data + 14 + VLAN_LEN);
		uh = (struct udphdr *)(skb->data + 14 + VLAN_LEN + 20);

		/* pr_info("iph->protocol is 0x%2x\n",iph->protocol); */
		/* pr_info("uh->dest is %4x\n",uh->dest); */
		/* pr_info("uh->source is %4x\n",uh->source); */
		/* pr_info("FOE_AI(skb) is 0x%x\n",FOE_AI(skb)); */

		/* skb_dump(skb); */
		if ((iph->protocol == IPPROTO_GRE) || (ntohs(uh->dest) == 1701)) {
			/*BIND flow using pptp/l2tp packets info */
			/* skb_dump(skb); */
			/* pr_info("LAN->WAN  TxH HIT_UNBIND_RATE_REACH\n"); */
			/* foe_dump_entry(FOE_ENTRY_NUM(skb)); */
			if (debug_level >= 1)
				pr_info("LAN->WAN  TxH Ppptp_towan_parse_layerinfo\n");

			if (pptp_fast_path) {
				if (pptp_towan_parse_layerinfo(skb)) {
					memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
					return 1;
				}
			}

			if (l2tp_fast_path) {
				if (l2tp_towan_parse_layerinfo(skb)) {
					memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
					return 1;
				}
			}

			/*layer2 keep original */
			if (debug_level >= 1)
				pr_info("Lan -> Wan ppe_fill_L2_info\n");
			/* Set Layer2 Info */
			if (ppe_fill_L2_info(skb, entry)) {
				memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
				/*already bind packet, return 1 to go out */
				return 1;
			}

			/* Set Layer3 Info */
			if (ppe_fill_L3_info(skb, entry)) {
				memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
				return 1;
			}

			/* Set Layer4 Info */
			if (ppe_fill_L4_info(skb, entry)) {
				memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
				return 1;
			}

			/* Set force port to CPU!!!  */
			if (ppe_setforce_port_info(skb, entry, 777)) {
				memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
				return 1;
			}
			if (debug_level >= 1)
				pr_info("ppe_set_entry_bind\n");
			/* Set Pseudo Interface info in Foe entry */
			/* Enter binding state */
			ppe_set_entry_bind(skb, entry);
			return 1;
		}
	}
#endif


#if defined (CONFIG_HW_NAT_SEMI_AUTO_MODE)
	if (IS_MAGIC_TAG_PROTECT_VALID(skb) &&
	    (FOE_AI(skb) == HIT_UNBIND_RATE_REACH) &&
	    (FOE_ALG(skb) == 0) && (get_done_bit(skb, entry) == 0)) {
    		printk ("ppe driver set entry index = %d\n", FOE_ENTRY_NUM(skb));
#else
	if (IS_MAGIC_TAG_PROTECT_VALID(skb) &&
	    (FOE_AI(skb) == HIT_UNBIND_RATE_REACH) &&
	    (FOE_ALG(skb) == 0)) {
#endif

#if defined(CONFIG_SUPPORT_WLAN_OPTIMIZE)
	if (bridge_lan_subnet(skb)) {
		if (!get_skb_interface(skb)) {
			USE_3T_UDP_FRAG = 0;
			return 1;
		} else 
			USE_3T_UDP_FRAG = 1;
	} else
		USE_3T_UDP_FRAG = 0;
#endif

#if !defined(CONFIG_SUPPORT_WLAN_OPTIMIZE)
#if defined (USE_UDP_FRAG)
		if (bridge_lan_subnet(skb))
			USE_3T_UDP_FRAG = 1;
		else
			USE_3T_UDP_FRAG = 0;
#endif
#endif


		if (debug_level >= 6) {
			pr_info(" which_region = %d\n", which_region);
		}

		/* get start addr for each layer */
		if (ppe_parse_layer_info(skb)) {
			memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
			return 1;
		}
		/* Set Layer2 Info */
		if (ppe_fill_L2_info(skb, entry)) {
			memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
			return 1;
		}
		/* Set Layer3 Info */
		if (ppe_fill_L3_info(skb, entry)) {
			memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
			return 1;
		}

		/* Set Layer4 Info */
		if (ppe_fill_L4_info(skb, entry)) {
			memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
			return 1;
		}

		/* Set force port info */
		if (ppe_setforce_port_info(skb, entry, gmac_no)) {
			memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
			return 1;
		}

		/* Set Pseudo Interface info in Foe entry */
		if (ppe_set_ext_if_num(skb, entry)) {
			memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
			return 1;
		}
#if defined(CONFIG_RAETH_QDMA) && defined(CONFIG_PPE_MCAST)
		if (ppe_parse_result.is_mcast) {
			foe_mcast_entry_qid(ppe_parse_result.vlan1,
					    ppe_parse_result.dmac,
					    M2Q_table[skb->mark]);
#ifdef CONFIG_PSEUDO_SUPPORT
			if (lan_wan_separate == 1 && gmac_no == 2) {
				foe_mcast_entry_qid(ppe_parse_result.vlan1,
						    ppe_parse_result.dmac,
						    M2Q_table[skb->mark] + 8);
#if defined(CONFIG_HW_SFQ)
				if (web_sfq_enable == 1 && (skb->mark == 2))
					foe_mcast_entry_qid(ppe_parse_result.vlan1,
							    ppe_parse_result.dmac,
							    HWSFQUP);
					/* queue3 */
#endif
			}
			if ((lan_wan_separate == 1) && (gmac_no == 1)) {
#if defined(CONFIG_HW_SFQ)
				if (web_sfq_enable == 1 && (skb->mark == 2))
					foe_mcast_entry_qid(ppe_parse_result.vlan1,
							    ppe_parse_result.dmac,
							    HWSFQDL);
					/* queue0 */
#endif
			}
#endif				/* CONFIG_PSEUDO_SUPPORT */

		}
#endif
#if defined(CONFIG_PPE_MIB)
/*clear mib counter*/
		reg_write(MIB_SER_CR, FOE_ENTRY_NUM(skb) | (1 << 16));
		do{
			if (!((reg_read(MIB_SER_CR) & 0x10000) >> 16))
			break;
			//usleep_range(100, 110);
		}while (--count);
		reg_read(MIB_SER_R0);
		reg_read(MIB_SER_R1);
		reg_read(MIB_SER_R1);
		reg_read(MIB_SER_R2);
#endif
#if defined (CONFIG_HW_NAT_AUTO_MODE)
		/* Enter binding state */
		ppe_set_entry_bind(skb, entry);
#elif defined (CONFIG_HW_NAT_SEMI_AUTO_MODE)
		set_ppe_table_done(entry);
		wmb();
#endif
#if defined(CONFIG_RA_HW_NAT_PACKET_SAMPLING)
/*add sampling policy here*/
		ps_entry->en = 0x1 << 1;
		ps_entry->pkt_cnt = 0x10;
#endif

#if defined(CONFIG_MACH_MT7623) || defined(CONFIG_ARCH_MT7622) || \
defined(CONFIG_ARCH_MT7623)
	} else if (IS_MAGIC_TAG_PROTECT_VALID(skb) &&
		  (FOE_AI(skb) == HIT_BIND_PACKET_SAMPLING)) {
		/* this is duplicate packet in PS function*/
		/* just drop it */
		pr_info("PS drop#%d\n", FOE_ENTRY_NUM(skb));
		memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
		return 0;
#endif
	} else if (IS_MAGIC_TAG_PROTECT_VALID(skb) &&
		  (FOE_AI(skb) == HIT_BIND_KEEPALIVE_MC_NEW_HDR ||
		  (FOE_AI(skb) == HIT_BIND_KEEPALIVE_DUP_OLD_HDR))) {
		/*this is duplicate packet in keepalive new header mode*/
		/*just drop it */
			if (debug_level >= 3)
				pr_info("TxGot HITBIND_KEEPALIVE_DUP_OLD packe (%d)\n",
					 FOE_ENTRY_NUM(skb));
		memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
		return 0;
	} else if (IS_MAGIC_TAG_PROTECT_VALID(skb) &&
		  (FOE_AI(skb) == HIT_UNBIND_RATE_REACH) &&
		  (FOE_ALG(skb) == 1)) {
		if (debug_level >= 3)
			NAT_PRINT("FOE_ALG=1 (Entry=%d)\n", FOE_ENTRY_NUM(skb));
#if defined(CONFIG_RA_HW_NAT_PREBIND)
	} else if (FOE_AI(skb) == HIT_PRE_BIND) {
#ifdef PREBIND_TEST
		if (jiffies % 2 == 0) {
			pr_info("drop prebind packet jiffies=%lu\n", jiffies);
			memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
			return 0;
		}
#endif

		if (entry->udib1.preb && entry->bfib1.state != BIND) {
			entry->bfib1.state = BIND;
			entry->udib1.preb = 0;
			/* Dump Binding Entry */
			if (debug_level >= 1)
				foe_dump_entry(FOE_ENTRY_NUM(skb));
		} else {
			/* drop duplicate prebind notify packet */
			memset(FOE_INFO_START_ADDR(skb), 0, FOE_INFO_LEN);
			return 0;
		}
#endif
	}

	return 1;
}

void ppe_setfoe_ebl(uint32_t foe_ebl)
{
	u32 ppe_flow_set = 0;

	ppe_flow_set = reg_read(PPE_FLOW_SET);

	/* FOE engine need to handle unicast/multicast/broadcast flow */
	if (foe_ebl == 1) {
		ppe_flow_set |= (BIT_IPV4_NAPT_EN | BIT_IPV4_NAT_EN);

		ppe_flow_set |= (BIT_IPV4_NAT_FRAG_EN | BIT_UDP_IP4F_NAT_EN);	/* ip fragment */
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
#else
		ppe_flow_set |= (BIT_IPV4_HASH_GREK);
#endif
#if defined(CONFIG_RA_HW_NAT_IPV6)
		ppe_flow_set |=
		    (BIT_IPV4_DSL_EN | BIT_IPV6_6RD_EN | BIT_IPV6_3T_ROUTE_EN |
		     BIT_IPV6_5T_ROUTE_EN);
		/* ppe_flow_set |= (BIT_IPV6_HASH_FLAB); // flow label */
		ppe_flow_set |= (BIT_IPV6_HASH_GREK);
#endif

	} else {
		ppe_flow_set &= ~(BIT_IPV4_NAPT_EN | BIT_IPV4_NAT_EN);
		ppe_flow_set &= ~(BIT_IPV4_NAT_FRAG_EN);
#if defined(CONFIG_RA_HW_NAT_IPV6)
		ppe_flow_set &=
		    ~(BIT_IPV4_DSL_EN | BIT_IPV6_6RD_EN | BIT_IPV6_3T_ROUTE_EN |
		      BIT_IPV6_5T_ROUTE_EN);
		/* ppe_flow_set &= ~(BIT_IPV6_HASH_FLAB); */
		ppe_flow_set &= ~(BIT_IPV6_HASH_GREK);
#else
		ppe_flow_set &= ~(BIT_FUC_FOE | BIT_FMC_FOE | BIT_FBC_FOE);
#if defined(CONFIG_RA_HW_NAT_IPV6)
		ppe_flow_set &= ~(BIT_IPV6_FOE_EN);
#endif

#endif
	}

	reg_write(PPE_FLOW_SET, ppe_flow_set);
}

static int ppe_setfoe_hash_mode(uint32_t hash_mode, struct device *dev)
{
	/* Allocate FOE table base */
	if (!foe_alloc_tbl(FOE_4TB_SIZ, dev))
		return 0;

	switch (FOE_4TB_SIZ) {
	case 1024:
		reg_modify_bits(PPE_FOE_CFG, FOE_TBL_SIZE_1K, 0, 3);
		break;
	case 2048:
		reg_modify_bits(PPE_FOE_CFG, FOE_TBL_SIZE_2K, 0, 3);
		break;
	case 4096:
		reg_modify_bits(PPE_FOE_CFG, FOE_TBL_SIZE_4K, 0, 3);
		break;
	case 8192:
		reg_modify_bits(PPE_FOE_CFG, FOE_TBL_SIZE_8K, 0, 3);
		break;
	case 16384:
		reg_modify_bits(PPE_FOE_CFG, FOE_TBL_SIZE_16K, 0, 3);
		break;
	}

	/* Set Hash Mode */
	reg_modify_bits(PPE_FOE_CFG, hash_mode, 14, 2);
	reg_write(PPE_HASH_SEED, HASH_SEED);
#if defined(CONFIG_RA_HW_NAT_HASH_DBG_IPV6_SIP)
	reg_modify_bits(PPE_FOE_CFG, 3, 18, 2);	/* ipv6_sip */
#elif defined(CONFIG_RA_HW_NAT_HASH_DBG_IPV4_SIP)
	reg_modify_bits(PPE_FOE_CFG, 2, 18, 2);	/* ipv4_sip */
#elif defined(CONFIG_RA_HW_NAT_HASH_DBG_SPORT)
	reg_modify_bits(PPE_FOE_CFG, 1, 18, 2);	/* sport */
#else
	reg_modify_bits(PPE_FOE_CFG, 0, 18, 2);	/* disable */
#endif

#if defined(CONFIG_RA_HW_NAT_IPV6)
	reg_modify_bits(PPE_FOE_CFG, 1, 3, 1);	/* entry size = 80bytes */
#else
	reg_modify_bits(PPE_FOE_CFG, 0, 3, 1);	/* entry size = 64bytes */
#endif

#if defined(CONFIG_RA_HW_NAT_PREBIND)
	reg_modify_bits(PPE_FOE_CFG, 1, 6, 1);	/* pre-bind age enable */
#endif

	/* Set action for FOE search miss */
	reg_modify_bits(PPE_FOE_CFG, FWD_CPU_BUILD_ENTRY, 4, 2);

	return 1;
}

static void ppe_setage_out(void)
{
	/* set Bind Non-TCP/UDP Age Enable */
	reg_modify_bits(PPE_FOE_CFG, DFL_FOE_NTU_AGE, 7, 1);

	/* set Unbind State Age Enable */
	reg_modify_bits(PPE_FOE_CFG, DFL_FOE_UNB_AGE, 8, 1);

	/* set min threshold of packet count for aging out at unbind state */
	reg_modify_bits(PPE_FOE_UNB_AGE, DFL_FOE_UNB_MNP, 16, 16);

	/* set Delta time for aging out an unbind FOE entry */
	reg_modify_bits(PPE_FOE_UNB_AGE, DFL_FOE_UNB_DLTA, 0, 8);

#if !defined (CONFIG_HW_NAT_MANUAL_MODE)
	/* set Bind TCP Age Enable */
	reg_modify_bits(PPE_FOE_CFG, DFL_FOE_TCP_AGE, 9, 1);

	/* set Bind UDP Age Enable */
	reg_modify_bits(PPE_FOE_CFG, DFL_FOE_UDP_AGE, 10, 1);

	/* set Bind TCP FIN Age Enable */
	reg_modify_bits(PPE_FOE_CFG, DFL_FOE_FIN_AGE, 11, 1);

	/* set Delta time for aging out an bind UDP FOE entry */
	reg_modify_bits(PPE_FOE_BND_AGE0, DFL_FOE_UDP_DLTA, 0, 16);

	/* set Delta time for aging out an bind Non-TCP/UDP FOE entry */
	reg_modify_bits(PPE_FOE_BND_AGE0, DFL_FOE_NTU_DLTA, 16, 16);

	/* set Delta time for aging out an bind TCP FIN FOE entry */
	reg_modify_bits(PPE_FOE_BND_AGE1, DFL_FOE_FIN_DLTA, 16, 16);

	/* set Delta time for aging out an bind TCP FOE entry */
	reg_modify_bits(PPE_FOE_BND_AGE1, DFL_FOE_TCP_DLTA, 0, 16);
#else
	/* fix TCP last ACK issue */
	/* Only need to enable Bind TCP FIN aging out function */
	reg_modify_bits(PPE_FOE_CFG, DFL_FOE_FIN_AGE, 11, 1);
	/* set Delta time for aging out an bind TCP FIN FOE entry */
	reg_modify_bits(PPE_FOE_BND_AGE1, DFL_FOE_FIN_DLTA, 16, 16);
#endif   
}

static void ppe_setfoe_ka(void)
{
	/* set Keep alive packet with new/org header */
	reg_modify_bits(PPE_FOE_CFG, DFL_FOE_KA, 12, 2);

	/* Keep alive timer value */
	reg_modify_bits(PPE_FOE_KA, DFL_FOE_KA_T, 0, 16);

	/* Keep alive time for bind FOE TCP entry */
	reg_modify_bits(PPE_FOE_KA, DFL_FOE_TCP_KA, 16, 8);

	/* Keep alive timer for bind FOE UDP entry */
	reg_modify_bits(PPE_FOE_KA, DFL_FOE_UDP_KA, 24, 8);

	/* Keep alive timer for bind Non-TCP/UDP entry */
	reg_modify_bits(PPE_BIND_LMT_1, DFL_FOE_NTU_KA, 16, 8);

#if defined(CONFIG_RA_HW_NAT_PREBIND)
	reg_modify_bits(PPE_BIND_LMT_1, DFL_PBND_RD_LMT, 24, 8);
#endif
}

static void ppe_setfoe_bind_rate(uint32_t foe_bind_rate)
{
	/* Allowed max entries to be build during a time stamp unit */

	/* smaller than 1/4 of total entries */
	reg_modify_bits(PPE_FOE_LMT1, DFL_FOE_QURT_LMT, 0, 14);

	/* between 1/2 and 1/4 of total entries */
	reg_modify_bits(PPE_FOE_LMT1, DFL_FOE_HALF_LMT, 16, 14);

	/* between full and 1/2 of total entries */
	reg_modify_bits(PPE_FOE_LMT2, DFL_FOE_FULL_LMT, 0, 14);

	/* Set reach bind rate for unbind state */
	reg_modify_bits(PPE_FOE_BNDR, foe_bind_rate, 0, 16);
#if defined(CONFIG_RA_HW_NAT_PREBIND)
	reg_modify_bits(PPE_FOE_BNDR, DFL_PBND_RD_PRD, 16, 16);
#endif
}

static void ppe_setfoe_glocfg_ebl(uint32_t ebl)
{
#if defined(CONFIG_RALINK_MT7620)
	u32 tpf = 0;
#endif

	if (ebl == 1) {
#if defined(CONFIG_RALINK_MT7620)
		/* 1. Remove P7 on forwarding ports */
		/* It's chip default setting */

		/* 2. PPE Forward Control Register: PPE_PORT=Port7 */
		reg_modify_bits(PFC, 7, 0, 3);

		/* 3. Select P7 as PPE port (PPE_EN=1) */
		reg_modify_bits(PFC, 1, 3, 1);

		/* TO_PPE Forwarding Register */
		tpf = IPV4_PPE_MYUC | IPV4_PPE_MC | IPV4_PPE_IPM | IPV4_PPE_UC | IPV4_PPE_UN;

#if defined(CONFIG_RA_HW_NAT_IPV6)
		tpf |= (IPV6_PPE_MYUC | IPV6_PPE_MC | IPV6_PPE_IPM | IPV6_PPE_UC | IPV6_PPE_UN);
#endif

/*TP PPE off, use ACL instead*/
		if ((reg_read(0xB000000C) & 0xf) >= 0x5) {
			reg_write(TPF0, tpf);
			reg_write(TPF1, tpf);
			reg_write(TPF2, tpf);
			reg_write(TPF3, tpf);
			reg_write(TPF4, tpf);
			reg_write(TPF5, tpf);
		}
		/* Forced Port7 link up, 1Gbps, and Full duplex  */
		reg_write(PMCR_P7, 0x5e33b);

		/* Disable SA Learning */
		reg_modify_bits(PSC_P7, 1, 4, 1);

		/* Use default values on P7 */
#elif defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)
		/* PPE Engine Enable */
		reg_modify_bits(PPE_GLO_CFG, 1, 0, 1);

#if (defined(CONFIG_MACH_MT7623) || defined(CONFIG_ARCH_MT7622)) || \
defined(CONFIG_ARCH_MT7623) && defined(CONFIG_RA_HW_NAT_IPV6)
		/* TSID Enable */
		pr_info("TSID Enable\n");
		reg_modify_bits(PPE_GLO_CFG, 1, 1, 1);
#endif

#if defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)
#if defined(CONFIG_PPE_MCAST)
		/* Enable multicast table lookup */
		reg_modify_bits(PPE_GLO_CFG, 1, 7, 1);
		reg_modify_bits(PPE_GLO_CFG, 0, 12, 2);	/* Decide by PPE entry hash index */
		reg_modify_bits(PPE_MCAST_PPSE, 0, 0, 4);	/* multicast port0 map to PDMA */
		reg_modify_bits(PPE_MCAST_PPSE, 1, 4, 4);	/* multicast port1 map to GMAC1 */
		reg_modify_bits(PPE_MCAST_PPSE, 2, 8, 4);	/* multicast port2 map to GMAC2 */
		reg_modify_bits(PPE_MCAST_PPSE, 5, 12, 4);	/* multicast port3 map to QDMA */
#endif				/* CONFIG_PPE_MCAST // */

#if defined(CONFIG_RAETH_QDMATX_QDMARX)
		reg_write(PPE_DFT_CPORT, 0x55555555);	/* default CPU port is port5 (QDMA) */
#else
		reg_write(PPE_DFT_CPORT, 0);	/* default CPU port is port0 (PDMA) */
#endif				/* CONFIG_RAETH_QDMA // */

#if (defined(CONFIG_MACH_MT7623) || defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)) && \
defined(CONFIG_RA_HW_NAT_IPV6)
		reg_modify_bits(PPE_DFT_CPORT, 1, 31, 1);
#endif

#if defined(CONFIG_RA_HW_NAT_PACKET_SAMPLING)
		/* reg_write(PS_CFG, 1); //Enable PacketSampling */
		reg_write(PS_CFG, 0x3);	/* Enable PacketSampling, Disable Aging */
#endif

#if defined(CONFIG_PPE_MIB)
		reg_write(MIB_CFG, 0x03);	/* Enable MIB & read clear */
		reg_write(MIB_CAH_CTRL, 0x01);	/* enable mib cache */
#endif

#endif				/* CONFIG_RALINK_MT7621 // */
#endif				/* CONFIG_HNAT_V2 // */

		/* PPE Packet with TTL=0 */
		reg_modify_bits(PPE_GLO_CFG, DFL_TTL0_DRP, 4, 1);

	} else {
#if defined(CONFIG_RALINK_MT7620)
		/* 1. Select P7 as PPE port (PPE_EN=1) */
		reg_modify_bits(PFC, 0, 3, 1);

		/* TO_PPE Forwarding Register */
		reg_write(TPF0, 0);
		reg_write(TPF1, 0);
		reg_write(TPF2, 0);
		reg_write(TPF3, 0);
		reg_write(TPF4, 0);
		reg_write(TPF5, 0);

		/* Forced Port7 link down */
		reg_write(PMCR_P7, 0x5e330);

		/* Enable SA Learning */
		reg_modify_bits(PSC_P7, 0, 4, 1);

#elif defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7623) || defined(CONFIG_ARCH_MT7622)
		/* PPE Engine Disable */
		reg_modify_bits(PPE_GLO_CFG, 0, 0, 1);
#endif

#if defined(CONFIG_RA_HW_NAT_PACKET_SAMPLING)
		reg_write(PS_CFG, 0);	/* Disable PacketSampling */
#endif
#if defined(CONFIG_PPE_MIB)
		reg_write(MIB_CFG, 0x00);	/* Disable MIB */
#endif
	}
}

#if(0)
static void foe_free_tbl(uint32_t num_of_entry)
{
	u32 foe_tbl_size;

	foe_tbl_size = num_of_entry * sizeof(struct foe_entry);
	dma_free_coherent(NULL, foe_tbl_size, ppe_foe_base, ppe_phy_foe_base);
	reg_write(PPE_FOE_BASE, 0);
}
#endif

static int32_t ppe_eng_start(void)
{
	/* Set PPE Flow Set */
	ppe_setfoe_ebl(1);

	/* Set Auto Age-Out Function */
	ppe_setage_out();

	/* Set PPE FOE KEEPALIVE TIMER */
	ppe_setfoe_ka();

	/* Set PPE FOE Bind Rate */
	ppe_setfoe_bind_rate(DFL_FOE_BNDR);

	/* Set PPE Global Configuration */
	ppe_setfoe_glocfg_ebl(1);
	return 0;
}

#if(0)
static int32_t ppe_eng_stop(void)
{
	/* Set PPE FOE ENABLE */
	ppe_setfoe_glocfg_ebl(0);

	/* Set PPE Flow Set */
	ppe_setfoe_ebl(0);

	/* Free FOE table */
	foe_free_tbl(FOE_4TB_SIZ);

	return 0;
}
#endif

struct net_device *ra_dev_get_by_name(const char *name)
{
	return dev_get_by_name(&init_net, name);
}

static void ppe_set_dst_port(uint32_t ebl)
{
	if (ebl) {
#if !defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
#if defined(CONFIG_RA_HW_NAT_WIFI)
		dst_port[DP_RA0] = ra_dev_get_by_name("ra0");
#if defined(CONFIG_RT2860V2_AP_MBSS) || defined(CONFIG_RTPCI_AP_MBSS) || defined(CONFIG_MBSS_SUPPORT)
		dst_port[DP_RA1] = ra_dev_get_by_name("ra1");
		dst_port[DP_RA2] = ra_dev_get_by_name("ra2");
		dst_port[DP_RA3] = ra_dev_get_by_name("ra3");
		dst_port[DP_RA4] = ra_dev_get_by_name("ra4");
		dst_port[DP_RA5] = ra_dev_get_by_name("ra5");
		dst_port[DP_RA6] = ra_dev_get_by_name("ra6");
		dst_port[DP_RA7] = ra_dev_get_by_name("ra7");
#if defined(CONFIG_RALINK_RT3883) || defined(CONFIG_RALINK_RT3352) || \
defined(CONFIG_RALINK_RT6855A) || defined(CONFIG_RALINK_MT7620) || \
defined(CONFIG_MBSS_SUPPORT)
		dst_port[DP_RA8] = ra_dev_get_by_name("ra8");
		dst_port[DP_RA9] = ra_dev_get_by_name("ra9");
		dst_port[DP_RA10] = ra_dev_get_by_name("ra10");
		dst_port[DP_RA11] = ra_dev_get_by_name("ra11");
		dst_port[DP_RA12] = ra_dev_get_by_name("ra12");
		dst_port[DP_RA13] = ra_dev_get_by_name("ra13");
		dst_port[DP_RA14] = ra_dev_get_by_name("ra14");
		dst_port[DP_RA15] = ra_dev_get_by_name("ra15");
#endif
#endif
#if defined(CONFIG_RT2860V2_AP_WDS) || defined(CONFIG_WDS_SUPPORT)
		dst_port[DP_WDS0] = ra_dev_get_by_name("wds0");
		dst_port[DP_WDS1] = ra_dev_get_by_name("wds1");
		dst_port[DP_WDS2] = ra_dev_get_by_name("wds2");
		dst_port[DP_WDS3] = ra_dev_get_by_name("wds3");
#endif
#if defined(CONFIG_RT2860V2_AP_APCLI) || defined(CONFIG_APCLI_SUPPORT)
		dst_port[DP_APCLI0] = ra_dev_get_by_name("apcli0");
#endif
#if defined(CONFIG_RT2860V2_AP_MESH)
		dst_port[DP_MESH0] = ra_dev_get_by_name("mesh0");
#endif
#if defined(CONFIG_RTDEV_MII) || defined(CONFIG_RTDEV_USB) || \
defined(CONFIG_RTDEV_PCI) || defined(CONFIG_RTDEV)
		dst_port[DP_RAI0] = ra_dev_get_by_name("rai0");
#if defined(CONFIG_RT3090_AP_MBSS) || defined(CONFIG_RT5392_AP_MBSS) || \
defined(CONFIG_RT3572_AP_MBSS) || defined(CONFIG_RT5572_AP_MBSS) || \
defined(CONFIG_RT5592_AP_MBSS) || defined(CONFIG_RT3593_AP_MBSS) || \
defined(CONFIG_RTPCI_AP_MBSS) || defined(CONFIG_MT7610_AP_MBSS) || \
defined(CONFIG_MBSS_SUPPORT)
		dst_port[DP_RAI1] = ra_dev_get_by_name("rai1");
		dst_port[DP_RAI2] = ra_dev_get_by_name("rai2");
		dst_port[DP_RAI3] = ra_dev_get_by_name("rai3");
		dst_port[DP_RAI4] = ra_dev_get_by_name("rai4");
		dst_port[DP_RAI5] = ra_dev_get_by_name("rai5");
		dst_port[DP_RAI6] = ra_dev_get_by_name("rai6");
		dst_port[DP_RAI7] = ra_dev_get_by_name("rai7");
		dst_port[DP_RAI8] = ra_dev_get_by_name("rai8");
		dst_port[DP_RAI9] = ra_dev_get_by_name("rai9");
		dst_port[DP_RAI10] = ra_dev_get_by_name("rai10");
		dst_port[DP_RAI11] = ra_dev_get_by_name("rai11");
		dst_port[DP_RAI12] = ra_dev_get_by_name("rai12");
		dst_port[DP_RAI13] = ra_dev_get_by_name("rai13");
		dst_port[DP_RAI14] = ra_dev_get_by_name("rai14");
		dst_port[DP_RAI15] = ra_dev_get_by_name("rai15");
#endif				/* CONFIG_RTDEV_AP_MBSS // */
#endif				/* CONFIG_RTDEV_MII || CONFIG_RTDEV_USB || CONFIG_RTDEV_PCI */
#if defined(CONFIG_RT3090_AP_APCLI) || defined(CONFIG_RT5392_AP_APCLI) || \
defined(CONFIG_RT3572_AP_APCLI) || defined(CONFIG_RT5572_AP_APCLI) || \
defined(CONFIG_RT5592_AP_APCLI) || defined(CONFIG_RT3593_AP_APCLI) || \
defined(CONFIG_MT7610_AP_APCLI) || defined(CONFIG_APCLI_SUPPORT)
		dst_port[DP_APCLII0] = ra_dev_get_by_name("apclii0");
#endif				/* CONFIG_RTDEV_AP_APCLI // */
#if defined(CONFIG_RT3090_AP_WDS) || defined(CONFIG_RT5392_AP_WDS) || \
defined(CONFIG_RT3572_AP_WDS) || defined(CONFIG_RT5572_AP_WDS) || \
defined(CONFIG_RT5592_AP_WDS) || defined(CONFIG_RT3593_AP_WDS) || \
defined(CONFIG_MT7610_AP_WDS) || defined(CONFIG_WDS_SUPPORT)
		dst_port[DP_WDSI0] = ra_dev_get_by_name("wdsi0");
		dst_port[DP_WDSI1] = ra_dev_get_by_name("wdsi1");
		dst_port[DP_WDSI2] = ra_dev_get_by_name("wdsi2");
		dst_port[DP_WDSI3] = ra_dev_get_by_name("wdsi3");
#endif

#if defined(CONFIG_RT3090_AP_MESH) || defined(CONFIG_RT5392_AP_MESH) || \
defined(CONFIG_RT3572_AP_MESH) || defined(CONFIG_RT5572_AP_MESH) || \
defined(CONFIG_RT5592_AP_MESH) || defined(CONFIG_RT3593_AP_MESH) || \
defined(CONFIG_MT7610_AP_MESH)
		dst_port[DP_MESHI0] = ra_dev_get_by_name("meshi0");
#endif				/* CONFIG_RTDEV_AP_MESH // */
#endif				/* CONFIG_RA_HW_NAT_WIFI // */
#endif				/* !CONFIG_RA_HW_NAT_WIFI_NEW_ARCH */

#if defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
		struct net_device *dev;
		int i;
#if 0
		dev = ra_dev_get_by_name("eth0");
		ppe_dev_reg_handler(dev);
		for (i = 0; i < MAX_IF_NUM; i++) {
			if (dst_port[i] == dev) {
				pr_info("%s :dst_port[%d] =%s\n", __func__, i, dev->name);
				DP_GMAC1 = i;
				break;
			}
		}
#ifdef CONFIG_RAETH_GMAC2
		dev = ra_dev_get_by_name("eth1");
		ppe_dev_reg_handler(dev);
		for (i = 0; i < MAX_IF_NUM; i++) {
			if (dst_port[i] == dev) {
				pr_info("%s :dst_port[%d] =%s\n", __func__, i, dev->name);
				DPORT_GMAC2 = i;
				break;
			}
		}
#endif
#else
		dev = ra_dev_get_by_name("eth2");
		ppe_dev_reg_handler(dev);
		for (i = 0; i < MAX_IF_NUM; i++) {
			if (dst_port[i] == dev) {
				pr_info("%s :dst_port[%d] =%s\n", __func__, i, dev->name);
				DP_GMAC1 = i;
				break;
			}
		}
#ifdef CONFIG_RAETH_GMAC2
		dev = ra_dev_get_by_name("eth3");
		ppe_dev_reg_handler(dev);
		for (i = 0; i < MAX_IF_NUM; i++) {
			if (dst_port[i] == dev) {
				pr_info("%s :dst_port[%d] =%s\n", __func__, i, dev->name);
				DPORT_GMAC2 = i;
				break;
			}
		}
#endif
#endif

#else
#if 0
		dst_port[DP_GMAC] = ra_dev_get_by_name("eth0");
#ifdef CONFIG_RAETH_GMAC2
		dst_port[DP_GMAC2] = ra_dev_get_by_name("eth1");
#endif
#else
		dst_port[DP_GMAC] = ra_dev_get_by_name("eth2");
#ifdef CONFIG_RAETH_GMAC2
		dst_port[DP_GMAC2] = ra_dev_get_by_name("eth3");
#endif
#endif
#endif				/* CONFIG_RA_HW_NAT_WIFI_NEW_ARCH */
#if defined(CONFIG_RA_HW_NAT_NIC_USB)
		dst_port[DP_PCI] = ra_dev_get_by_name("eth0");	/* PCI interface name */
		dst_port[DP_USB] = ra_dev_get_by_name("eth1");	/* USB interface name */
#endif				/* CONFIG_RA_HW_NAT_NIC_USB // */
	} else {
#if defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
		int j = 0;
		dev_put(dst_port[DP_GMAC1]);
		dev_put(dst_port[DPORT_GMAC2]);
		for (j = 0; j < MAX_IF_NUM; j++) {
			if (dst_port[j] != NULL)
				dst_port[j] = NULL;
		}

#endif
#if !defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
#if defined(CONFIG_RA_HW_NAT_WIFI)
		if (dst_port[DP_RA0])
			dev_put(dst_port[DP_RA0]);
#if defined(CONFIG_RT2860V2_AP_MBSS) || defined(CONFIG_RTPCI_AP_MBSS) || \
defined(CONFIG_MBSS_SUPPORT)
		if (dst_port[DP_RA1])
			dev_put(dst_port[DP_RA1]);
		if (dst_port[DP_RA2])
			dev_put(dst_port[DP_RA2]);
		if (dst_port[DP_RA3])
			dev_put(dst_port[DP_RA3]);
		if (dst_port[DP_RA4])
			dev_put(dst_port[DP_RA4]);
		if (dst_port[DP_RA5])
			dev_put(dst_port[DP_RA5]);
		if (dst_port[DP_RA6])
			dev_put(dst_port[DP_RA6]);
		if (dst_port[DP_RA7])
			dev_put(dst_port[DP_RA7]);
#if defined(CONFIG_RALINK_RT3883) || defined(CONFIG_RALINK_RT3352) || \
defined(CONFIG_RALINK_RT6855A) || defined(CONFIG_RALINK_MT7620) || \
defined(CONFIG_MBSS_SUPPORT)

		if (dst_port[DP_RA8])
			dev_put(dst_port[DP_RA8]);
		if (dst_port[DP_RA9])
			dev_put(dst_port[DP_RA9]);
		if (dst_port[DP_RA10])
			dev_put(dst_port[DP_RA10]);
		if (dst_port[DP_RA11])
			dev_put(dst_port[DP_RA11]);
		if (dst_port[DP_RA12])
			dev_put(dst_port[DP_RA12]);
		if (dst_port[DP_RA13])
			dev_put(dst_port[DP_RA13]);
		if (dst_port[DP_RA14])
			dev_put(dst_port[DP_RA14]);
		if (dst_port[DP_RA15])
			dev_put(dst_port[DP_RA15]);
#endif
#endif
#if defined(CONFIG_RT2860V2_AP_WDS) || defined(CONFIG_WDS_SUPPORT)
		if (dst_port[DP_WDS0])
			dev_put(dst_port[DP_WDS0]);
		if (dst_port[DP_WDS1])
			dev_put(dst_port[DP_WDS1]);
		if (dst_port[DP_WDS2])
			dev_put(dst_port[DP_WDS2]);
		if (dst_port[DP_WDS3])
			dev_put(dst_port[DP_WDS3]);
#endif
#if defined(CONFIG_RT2860V2_AP_APCLI) || defined(CONFIG_APCLI_SUPPORT)
		if (dst_port[DP_APCLI0])
			dev_put(dst_port[DP_APCLI0]);
#endif
#if defined(CONFIG_RT2860V2_AP_MESH)
		if (dst_port[DP_MESH0])
			dev_put(dst_port[DP_MESH0]);
#endif
#if defined(CONFIG_RTDEV_MII) || defined(CONFIG_RTDEV_USB) || \
defined(CONFIG_RTDEV_PCI) || defined(CONFIG_RTDEV)
		if (dst_port[DP_RAI0])
			dev_put(dst_port[DP_RAI0]);
#if defined(CONFIG_RT3090_AP_MBSS) || defined(CONFIG_RT5392_AP_MBSS) || \
defined(CONFIG_RT3572_AP_MBSS) || defined(CONFIG_RT5572_AP_MBSS) || \
defined(CONFIG_RT5592_AP_MBSS) || defined(CONFIG_RT3593_AP_MBSS) || \
defined(CONFIG_MT7610_AP_MBSS) || defined(CONFIG_RTPCI_AP_MBSS)  || \
defined(CONFIG_MBSS_SUPPORT)
		if (dst_port[DP_RAI1])
			dev_put(dst_port[DP_RAI1]);
		if (dst_port[DP_RAI2])
			dev_put(dst_port[DP_RAI2]);
		if (dst_port[DP_RAI3])
			dev_put(dst_port[DP_RAI3]);
		if (dst_port[DP_RAI4])
			dev_put(dst_port[DP_RAI4]);
		if (dst_port[DP_RAI5])
			dev_put(dst_port[DP_RAI5]);
		if (dst_port[DP_RAI6])
			dev_put(dst_port[DP_RAI6]);
		if (dst_port[DP_RAI7])
			dev_put(dst_port[DP_RAI7]);
		if (dst_port[DP_RAI8])
			dev_put(dst_port[DP_RAI8]);
		if (dst_port[DP_RAI9])
			dev_put(dst_port[DP_RAI9]);
		if (dst_port[DP_RAI10])
			dev_put(dst_port[DP_RAI10]);
		if (dst_port[DP_RAI11])
			dev_put(dst_port[DP_RAI11]);
		if (dst_port[DP_RAI12])
			dev_put(dst_port[DP_RAI12]);
		if (dst_port[DP_RAI13])
			dev_put(dst_port[DP_RAI13]);
		if (dst_port[DP_RAI14])
			dev_put(dst_port[DP_RAI14]);
		if (dst_port[DP_RAI15])
			dev_put(dst_port[DP_RAI15]);
#endif				/* CONFIG_RTDEV_AP_MBSS // */
#endif				/* CONFIG_RTDEV_MII || CONFIG_RTDEV_USB || CONFIG_RTDEV_PCI */
#if defined(CONFIG_RT3090_AP_APCLI) || defined(CONFIG_RT5392_AP_APCLI) || \
defined(CONFIG_RT3572_AP_APCLI) || defined(CONFIG_RT5572_AP_APCLI) || \
defined(CONFIG_RT5592_AP_APCLI) || defined(CONFIG_RT3593_AP_APCLI) || \
defined(CONFIG_MT7610_AP_APCLI) || defined(CONFIG_APCLI_SUPPORT)
		if (dst_port[DP_APCLII0])
			dev_put(dst_port[DP_APCLII0]);
#endif				/* CONFIG_RTDEV_AP_APCLI // */
#if defined(CONFIG_RT3090_AP_WDS) || defined(CONFIG_RT5392_AP_WDS) || \
defined(CONFIG_RT3572_AP_WDS) || defined(CONFIG_RT5572_AP_WDS) || \
defined(CONFIG_RT5592_AP_WDS) || defined(CONFIG_RT3593_AP_WDS) || \
defined(CONFIG_MT7610_AP_WDS) || defined(CONFIG_WDS_SUPPORT)
		if (dst_port[DP_WDSI0])
			dev_put(dst_port[DP_WDSI0]);
		if (dst_port[DP_WDSI1])
			dev_put(dst_port[DP_WDSI1]);
		if (dst_port[DP_WDSI2])
			dev_put(dst_port[DP_WDSI2]);
		if (dst_port[DP_WDSI3])
			dev_put(dst_port[DP_WDSI3]);
#endif

#if defined(CONFIG_RT3090_AP_MESH) || defined(CONFIG_RT5392_AP_MESH) || \
defined(CONFIG_RT3572_AP_MESH) || defined(CONFIG_RT5572_AP_MESH) || \
defined(CONFIG_RT5592_AP_MESH) || defined(CONFIG_RT3593_AP_MESH) || \
defined(CONFIG_MT7610_AP_MESH)

		if (dst_port[DP_MESHI0])
			dev_put(dst_port[DP_MESHI0]);
#endif				/* CONFIG_RTDEV_AP_MESH // */
		if (dst_port[DP_GMAC])
			dev_put(dst_port[DP_GMAC]);
#ifdef CONFIG_RAETH_GMAC2
		if (dst_port[DP_GMAC2])
			dev_put(dst_port[DP_GMAC2]);
#endif
#endif				/* CONFIG_RA_HW_NAT_WIFI // */
#if defined(CONFIG_RA_HW_NAT_NIC_USB)
		if (dst_port[DP_PCI])
			dev_put(dst_port[DP_PCI]);
		if (dst_port[DP_USB])
			dev_put(dst_port[DP_USB]);
#endif				/* CONFIG_RA_HW_NAT_NIC_USB // */
#endif				/* (CONFIG_RA_HW_NAT_WIFI_NEW_ARCH) */
	}
}

#if defined(CONFIG_RALINK_MT7620)
static void set_acl_fwd(u16 TP, uint8_t WORD_OFFSET, uint16_t BIT_CMP, uint16_t CMP_PAT,
			uint8_t INDEX)
{
	unsigned int i, value;

	value = CMP_PAT;
	value |= (BIT_CMP << 16);	/* compare mask */

	reg_write(RALINK_ETH_SW_BASE + REG_ESW_VLAN_VAWD1, value);

	pr_info("REG_ESW_VLAN_VAWD1 value is 0x%x\n\r", value);

	value = 0x3f << 8;	/* w_port_map */
	value |= 0x1 << 19;	/* enable */
	value |= TP << 16;	/* ip header */
	value |= WORD_OFFSET << 1;	/* word offset */
	reg_write(RALINK_ETH_SW_BASE + REG_ESW_VLAN_VAWD2, value);

	pr_info("REG_ESW_VLAN_VAWD2 value is 0x%x\n\r", value);

	value = (0x80005000 + INDEX);	/* w_acl entry 0 */
	reg_write(RALINK_ETH_SW_BASE + REG_ESW_VLAN_VTCR, value);

	pr_info("REG_ESW_VLAN_VTCR value is 0x%x\n\r\n\r", value);

	for (i = 0; i < 20; i++) {
		value = reg_read(RALINK_ETH_SW_BASE + REG_ESW_VLAN_VTCR);
		if ((value & 0x80000000) == 0) {	/* table busy */
			break;
		}
		usleep_range(1000, 1100);
	}
	if (i == 20) {
		pr_info("timeout.\n");
		pr_info("VTCR is 0x%x\n!!!!!", value);
	}
}

static void set_acl_control(u32 HIT_PAT, uint8_t RULE_NUM, uint16_t RULE_ACTION)
{
	unsigned int i, value;

	/* set pattern */
	value = HIT_PAT;	/* bit0,1 */
	/* value |= 1;//valid */
	reg_write(RALINK_ETH_SW_BASE + REG_ESW_VLAN_VAWD1, value);
	pr_info("REG_ESW_VLAN_VAWD1 value is 0x%x\n\r", value);

	value = 0;		/* bit32~63 */
	reg_write(RALINK_ETH_SW_BASE + REG_ESW_VLAN_VAWD2, value);

	pr_info("REG_ESW_VLAN_VAWD2 value is 0x%x\n\r", value);

	value = (0x80009000 + RULE_NUM);	/* w_acl control rule  RULE_NUM */
	reg_write(RALINK_ETH_SW_BASE + REG_ESW_VLAN_VTCR, value);

	pr_info("REG_ESW_VLAN_VTCR value is 0x%x\n\r\n\r", value);

	for (i = 0; i < 20; i++) {
		value = reg_read(RALINK_ETH_SW_BASE + REG_ESW_VLAN_VTCR);
		if ((value & 0x80000000) == 0) {	/* table busy */
			break;
		}
		usleep_range(1000, 1100);
	}
	if (i == 20)
		pr_info("timeout.\n");

	/* set action */
	value = RULE_ACTION;	/*  */
	/* value = 0x0; //default. Nodrop */
	/* value |= 1 << 28;//acl intterupt enable */
	/* value |= 1 << 27;//acl hit count */
	/* value |= 6 << 4;//acl UP */
	/* value |= 6 << 16;//eg-tag tagged */
	reg_write(RALINK_ETH_SW_BASE + REG_ESW_VLAN_VAWD1, value);
	pr_info("RALINK_ETH_SW_BASE + REG_ESW_VLAN_VAWD1 value is 0x%x\n\r", value);

	value = 0;		/* bit32~63 */
	reg_write(RALINK_ETH_SW_BASE + REG_ESW_VLAN_VAWD2, value);
	pr_info("REG_ESW_VLAN_VAWD2 value is 0x%x\n\r", value);

	value = (0x8000b000 + RULE_NUM);	/* w_acl rule action control RULE_NUM */
	reg_write(RALINK_ETH_SW_BASE + REG_ESW_VLAN_VTCR, value);

	pr_info("REG_ESW_VLAN_VTCR value is 0x%x\n\r\n\r", value);

	for (i = 0; i < 20; i++) {
		value = reg_read(RALINK_ETH_SW_BASE + REG_ESW_VLAN_VTCR);
		if ((value & 0x80000000) == 0) {	/* table busy */
			break;
		}
		usleep_range(1000, 1100);
	}
	if (i == 20)
		pr_info("timeout.\n");
}

struct net_device *lan_int;
struct net_device *wan_int;
static void set_acl_fwd(uint32_t ebl)
{
	u16 mac_pattern;
	unsigned int i, value;

	if (ebl) {
#if 0
#if defined(CONFIG_RAETH_SPECIAL_TAG)
#if defined(CONFIG_WAN_AT_P4)
		wan_int = ra_dev_get_by_name("eth0.5");
#else
		wan_int = ra_dev_get_by_name("eth0.1");
#endif
#else
		wan_int = ra_dev_get_by_name("eth0.2");
#endif
#else
#if defined(CONFIG_RAETH_SPECIAL_TAG)
#if defined(CONFIG_WAN_AT_P4)
		wan_int = ra_dev_get_by_name("eth2.5");
#else
		wan_int = ra_dev_get_by_name("eth2.1");
#endif
#else
		wan_int = ra_dev_get_by_name("eth2.2");
#endif
#endif
		lan_int = ra_dev_get_by_name("br0");
		for (i = 0; i < 6; i++) {
			value = reg_read(RALINK_ETH_SW_BASE + 0x2004 + (i * 0x100));
			value |= (0x1 << 10);
			reg_write(RALINK_ETH_SW_BASE + 0x2004 + (i * 0x100), value);
		}
	} else {
		for (i = 0; i < 6; i++) {
			value = reg_read(RALINK_ETH_SW_BASE + 0x2004 + (i * 0x100));
			value &= ~(0x1 << 10);
			reg_write(RALINK_ETH_SW_BASE + 0x2004 + (i * 0x100), value);
		}
		dev_put(wan_int);
		dev_put(lan_int);
		return;
	}

	pr_info("LAN dev address is %2x %2x %2x %2x %2x %2x\n", lan_int->dev_addr[0],
		 lan_int->dev_addr[1], lan_int->dev_addr[2], lan_int->dev_addr[3], lan_int->dev_addr[4],
		 lan_int->dev_addr[5]);
	pr_info("WAN dev address is %2x %2x %2x %2x %2x %2x\n", wan_int->dev_addr[0],
		 wan_int->dev_addr[1], wan_int->dev_addr[2], wan_int->dev_addr[3], wan_int->dev_addr[4],
		 wan_int->dev_addr[5]);

	mac_pattern = (lan_int->dev_addr[0] << 8) | lan_int->dev_addr[1];
	set_acl_fwd(0, 0, 0xffff, mac_pattern, 0);
	mac_pattern = (lan_int->dev_addr[2] << 8) | lan_int->dev_addr[3];
	set_acl_fwd(0, 1, 0xffff, mac_pattern, 1);
	mac_pattern = (lan_int->dev_addr[4] << 8) | lan_int->dev_addr[5];
	set_acl_fwd(0, 2, 0xffff, mac_pattern, 2);
	set_acl_fwd(0, 0, 0x0300, 0x0100, 3);
	mac_pattern = (wan_int->dev_addr[0] << 8) | wan_int->dev_addr[1];
	set_acl_fwd(0, 0, 0xffff, mac_pattern, 4);
	mac_pattern = (wan_int->dev_addr[2] << 8) | wan_int->dev_addr[3];
	set_acl_fwd(0, 1, 0xffff, mac_pattern, 5);
	mac_pattern = (wan_int->dev_addr[4] << 8) | wan_int->dev_addr[5];
	set_acl_fwd(0, 2, 0xffff, mac_pattern, 6);

	set_acl_fwd(2, 1, 0xfffc, 0x0048, 8);
	set_acl_fwd(2, 1, 0x0, 0x0, 9);
	set_acl_fwd(3, 2, 0xfffc, 0x0020, 10);
	set_acl_fwd(3, 2, 0x0, 0x0, 11);

	set_acl_control(0x107, 0, 0x4080);
	set_acl_control(0x207, 1, 0x8080);
	set_acl_control(0x170, 2, 0x4080);
	set_acl_control(0x270, 3, 0x8080);
	set_acl_control(0x108, 4, 0x7F80);	/* Multicast inc. CPU */
	set_acl_control(0x208, 5, 0x8F80);	/* Multicast inc. PPE ?????? */
	set_acl_control(0x407, 6, 0x4080);
	set_acl_control(0x807, 7, 0x8080);
	set_acl_control(0x470, 8, 0x4080);
	set_acl_control(0x870, 9, 0x8080);
	set_acl_control(0x408, 0xa, 0x7F80);	/* /Multicast inc. CPU */
	set_acl_control(0x808, 0xb, 0x8F80);	/* Multicast inc. PPE ?????? */
}

#endif

uint32_t set_gdma_fwd(uint32_t ebl)
{
#if defined(CONFIG_RALINK_MT7620)
	u32 data = 0;

	data = reg_read(GDM2_FWD_CFG);

	if (ebl) {
		data &= ~0x7777;
		data |= GDM1_OFRC_P_CPU;
		data |= GDM1_MFRC_P_CPU;
		data |= GDM1_BFRC_P_CPU;
		data |= GDM1_UFRC_P_CPU;
	} else {
		data |= 0x7777;
	}

	reg_write(GDM2_FWD_CFG, data);
#else
	u32 data = 0;

	data = reg_read(FE_GDMA1_FWD_CFG);

	if (ebl) {
		data &= ~0x7777;
		/* Uni-cast frames forward to PPE */
		data |= GDM1_UFRC_P_PPE;
		/* Broad-cast MAC address frames forward to PPE */
		data |= GDM1_BFRC_P_PPE;
		/* Multi-cast MAC address frames forward to PPE */
		data |= GDM1_MFRC_P_PPE;
		/* Other MAC address frames forward to PPE */
		data |= GDM1_OFRC_P_PPE;

	} else {
		data &= ~0x7777;
		/* Uni-cast frames forward to CPU */
		data |= GDM1_UFRC_P_CPU;
		/* Broad-cast MAC address frames forward to CPU */
		data |= GDM1_BFRC_P_CPU;
		/* Multi-cast MAC address frames forward to CPU */
		data |= GDM1_MFRC_P_CPU;
		/* Other MAC address frames forward to CPU */
		data |= GDM1_OFRC_P_CPU;
	}

	reg_write(FE_GDMA1_FWD_CFG, data);

#ifdef CONFIG_RAETH_GMAC2
	data = reg_read(FE_GDMA2_FWD_CFG);

	if (ebl) {
		data &= ~0x7777;
		/* Uni-cast frames forward to PPE */
		data |= GDM1_UFRC_P_PPE;
		/* Broad-cast MAC address frames forward to PPE */
		data |= GDM1_BFRC_P_PPE;
		/* Multi-cast MAC address frames forward to PPE */
		data |= GDM1_MFRC_P_PPE;
		/* Other MAC address frames forward to PPE */
		data |= GDM1_OFRC_P_PPE;

	} else {
		data &= ~0x7777;
		/* Uni-cast frames forward to CPU */
		data |= GDM1_UFRC_P_CPU;
		/* Broad-cast MAC address frames forward to CPU */
		data |= GDM1_BFRC_P_CPU;
		/* Multi-cast MAC address frames forward to CPU */
		data |= GDM1_MFRC_P_CPU;
		/* Other MAC address frames forward to CPU */
		data |= GDM1_OFRC_P_CPU;
	}
	reg_write(FE_GDMA2_FWD_CFG, data);
#endif
#endif

	return 0;
}

#if defined(CONFIG_HNAT_V2)
void ppe_set_cache_ebl(void)
{
	/* clear cache table before enabling cache */
	reg_modify_bits(CAH_CTRL, 1, 9, 1);
	reg_modify_bits(CAH_CTRL, 0, 9, 1);

	/* Cache enable */
	reg_modify_bits(CAH_CTRL, 1, 0, 1);
}

static void ppe_set_sw_vlan_chk(int ebl)
{
#if defined(CONFIG_RALINK_MT7620)
	u32 reg;
	/* port6&7: fall back mode / same port matrix group */
	if (ebl) {
		reg = reg_read(RALINK_ETH_SW_BASE + 0x2604);
		reg |= 0xff0003;
		reg_write(RALINK_ETH_SW_BASE + 0x2604, reg);

		reg = reg_read(RALINK_ETH_SW_BASE + 0x2704);
		reg |= 0xff0003;
		reg_write(RALINK_ETH_SW_BASE + 0x2704, reg);
	} else {
		reg = reg_read(RALINK_ETH_SW_BASE + 0x2604);
		reg &= ~0xff0003;
		reg |= 0xc00001;
		reg_write(RALINK_ETH_SW_BASE + 0x2604, reg);

		reg = reg_read(RALINK_ETH_SW_BASE + 0x2704);
		reg &= ~0xff0003;
		reg |= 0xc00001;
		reg_write(RALINK_ETH_SW_BASE + 0x2704, reg);
	}
#endif
}

static void ppe_set_fp_BMAP(void)
{
#if defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)
	 /*TODO*/
#else
	/* index 0 = force port 0
	 * index 1 = force port 1
	 * ...........
	 * index 7 = force port 7
	 * index 8 = no force port
	 * index 9 = force to all ports
	 */
	reg_write(PPE_FP_BMAP_0, 0x00020001);
	reg_write(PPE_FP_BMAP_1, 0x00080004);
	reg_write(PPE_FP_BMAP_2, 0x00200010);
	reg_write(PPE_FP_BMAP_3, 0x00800040);
	reg_write(PPE_FP_BMAP_4, 0x003F0000);
#endif
}

static void ppe_set_ip_prot(void)
{
	/* IP Protocol Field for IPv4 NAT or IPv6 3-tuple flow */
	/* Don't forget to turn on related bits in PPE_IP_PROT_CHK register if you want to support
	 * another IP protocol.
	 */
	/* FIXME: enable it to support IP fragement */
	reg_write(PPE_IP_PROT_CHK, 0xFFFFFFFF);	/* IPV4_NXTH_CHK and IPV6_NXTH_CHK */
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
	reg_modify_bits(PPE_IP_PROT_0, IPPROTO_GRE, 0, 8);
#endif
	/* reg_modify_bits(PPE_IP_PROT_0, IPPROTO_GRE, 0, 8); */
	/* reg_modify_bits(PPE_IP_PROT_0, IPPROTO_TCP, 8, 8); */
	/* reg_modify_bits(PPE_IP_PROT_0, IPPROTO_UDP, 16, 8); */
	/* reg_modify_bits(PPE_IP_PROT_0, IPPROTO_IPV6, 24, 8); */
}
#endif

#ifdef CONFIG_RA_HW_NAT_ACCNT_MAINTAINER
DEFINE_TIMER(update_foe_ac_timer, update_foe_ac_timer_handler, 0, 0);

void update_foe_ac_timer_handler(unsigned long unused)
{
#if defined(CONFIG_RALINK_MT7620)
	ac_info[1].ag_byte_cnt += reg_read(AC_BASE + 1 * 8);	/* Low bytes */
	ac_info[1].ag_pkt_cnt += reg_read(AC_BASE + 1 * 8 + 4);	/* High bytes */
	ac_info[2].ag_byte_cnt += reg_read(AC_BASE + 2 * 8);	/* Low bytes */
	ac_info[2].ag_pkt_cnt += reg_read(AC_BASE + 2 * 8 + 4);	/* High bytes */
#elif defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || \
defined(CONFIG_ARCH_MT7622) || defined(CONFIG_ARCH_MT7623)
	ac_info[1].ag_byte_cnt += reg_read(AC_BASE + 1 * 16);	/* 64bit bytes cnt */
	ac_info[1].ag_byte_cnt += ((unsigned long long)(reg_read(AC_BASE + 1 * 16 + 4)) << 32);	/* 64bit bytes cnt */
	ac_info[1].ag_pkt_cnt += reg_read(AC_BASE + 1 * 16 + 8);	/* 32bites packet cnt */
	ac_info[2].ag_byte_cnt += reg_read(AC_BASE + 2 * 16);	/* 64bit bytes cnt */
	ac_info[2].ag_byte_cnt += ((unsigned long long)(reg_read(AC_BASE + 2 * 16 + 4)) << 32);	/* 64bit bytes cnt */
	ac_info[2].ag_pkt_cnt += reg_read(AC_BASE + 2 * 16 + 8);	/* 32bites packet cnt */
#endif

	update_foe_ac_timer.expires = jiffies + 16 * HZ;
	add_timer(&update_foe_ac_timer);
}

void update_foe_ac_init(void)
{
	ac_info[1].ag_byte_cnt = 0;
	ac_info[1].ag_pkt_cnt = 0;
	ac_info[2].ag_byte_cnt = 0;
	ac_info[2].ag_pkt_cnt = 0;
	ac_info[3].ag_byte_cnt = 0;
	ac_info[3].ag_pkt_cnt = 0;
	ac_info[4].ag_byte_cnt = 0;
	ac_info[4].ag_pkt_cnt = 0;
	ac_info[5].ag_byte_cnt = 0;
	ac_info[5].ag_pkt_cnt = 0;
	ac_info[6].ag_byte_cnt = 0;
	ac_info[6].ag_pkt_cnt = 0;
}
#endif

void foe_ac_update_ebl(int ebl)
{
#ifdef CONFIG_RA_HW_NAT_ACCNT_MAINTAINER
	if (ebl) {
		update_foe_ac_init();
		update_foe_ac_timer.expires = jiffies + HZ;
		add_timer(&update_foe_ac_timer);
	} else {
		if (timer_pending(&update_foe_ac_timer))
			del_timer_sync(&update_foe_ac_timer);
	}
#endif
}
void foe_clear_entry(struct neighbour *neigh)
{
	int hash_index, clear;
	struct foe_entry *entry;
        u32 * daddr = (u32 *)neigh->primary_key;
	const u8 *addrtmp;
	u8 mac0,mac1,mac2,mac3,mac4,mac5;
	u32 dip;
	dip = (u32)(*daddr);
	clear = 0;
	addrtmp = neigh->ha;
	mac0 = (u8)(*addrtmp);
	mac1 = (u8)(*(addrtmp+1));
	mac2 = (u8)(*(addrtmp+2));
	mac3 = (u8)(*(addrtmp+3));
	mac4 = (u8)(*(addrtmp+4));
	mac5 = (u8)(*(addrtmp+5));
	
        for (hash_index = 0; hash_index < FOE_4TB_SIZ; hash_index++) {
		entry = &ppe_foe_base[hash_index];
		if(entry->bfib1.state == BIND) {
			/*printk("before old mac= %x:%x:%x:%x:%x:%x, new_dip=%x\n",
				entry->ipv4_hnapt.dmac_hi[3],
				entry->ipv4_hnapt.dmac_hi[2],
				entry->ipv4_hnapt.dmac_hi[1],
				entry->ipv4_hnapt.dmac_hi[0],
				entry->ipv4_hnapt.dmac_lo[1],
				entry->ipv4_hnapt.dmac_lo[0], entry->ipv4_hnapt.new_dip);
			*/
			if (entry->ipv4_hnapt.new_dip == ntohl(dip)) {
				if ((entry->ipv4_hnapt.dmac_hi[3] != mac0) || 
				    (entry->ipv4_hnapt.dmac_hi[2] != mac1) || 
				    (entry->ipv4_hnapt.dmac_hi[1] != mac2) ||
				    (entry->ipv4_hnapt.dmac_hi[0] != mac3) ||
				    (entry->ipv4_hnapt.dmac_lo[1] != mac4) ||
				    (entry->ipv4_hnapt.dmac_lo[0] != mac5)) {
				    	//printk("%s: state=%d\n",__func__,neigh->nud_state);
				    	reg_modify_bits(PPE_FOE_CFG, ONLY_FWD_CPU, 4, 2);
				    	
				  	entry->ipv4_hnapt.udib1.state = INVALID;
					entry->ipv4_hnapt.udib1.time_stamp = reg_read(FOE_TS) & 0xFF;
					ppe_set_cache_ebl();
					mod_timer(&hwnat_clear_entry_timer, jiffies + 3 * HZ);
				/*
					printk("delete old entry: dip =%x\n", ntohl(dip));
							
				    	printk("old mac= %x:%x:%x:%x:%x:%x, dip=%x\n", 
				    		entry->ipv4_hnapt.dmac_hi[3],
				    		entry->ipv4_hnapt.dmac_hi[2],
				    		entry->ipv4_hnapt.dmac_hi[1],
				    		entry->ipv4_hnapt.dmac_hi[0],
				    		entry->ipv4_hnapt.dmac_lo[1],
				    		entry->ipv4_hnapt.dmac_lo[0],
				    		ntohl(dip));
				    	printk("new mac= %x:%x:%x:%x:%x:%x, dip=%x\n", mac0, mac1, mac2, mac3, mac4, mac5, ntohl(dip));
				*/
				}
			}
		}
	}
}
static int wh2_netevent_handler(struct notifier_block *unused,
                                unsigned long event, void *ptr)
{
        struct net_device *dev = NULL;
        struct neighbour *neigh = NULL;

        switch (event) {
        case NETEVENT_NEIGH_UPDATE:
                neigh = ptr;
                dev = neigh->dev;
                if (dev)
			foe_clear_entry(neigh);
                break;
        }

        return NOTIFY_DONE;
}
static struct notifier_block Hnat_netevent_nb __read_mostly = {
        .notifier_call = wh2_netevent_handler,
};
/*PPE Enabled: GMAC<->PPE<->CPU*/
/*PPE Disabled: GMAC<->CPU*/
static int32_t ppe_init_mod(void)
{
	struct platform_device *pdev;
	int j;
	
	NAT_PRINT("Ralink HW NAT Module Enabled\n");

	for (j = 0; j < MAX_IF_NUM; j++)
		dst_port[j] = NULL;

	pdev = platform_device_alloc("HW_NAT", PLATFORM_DEVID_AUTO);
	if (!pdev)
		return -ENOMEM;
#if defined(CONFIG_ARCH_MT7622)
	hwnat_setup_dma_ops(&pdev->dev, FALSE);
#endif
	/* Set PPE FOE Hash Mode */
	if (!ppe_setfoe_hash_mode(DFL_FOE_HASH_MODE, &pdev->dev)) {
		pr_info("memory allocation failed\n");
		return -ENOMEM;	/* memory allocation failed */
	}

	/* Get net_device structure of Dest Port */
	ppe_set_dst_port(1);

	/* Register ioctl handler */
	ppe_reg_ioctl_handler();

	ppe_set_fp_BMAP();
	ppe_set_ip_prot();
	ppe_set_cache_ebl();
	ppe_set_sw_vlan_chk(0);
	foe_ac_update_ebl(1);

	/* 0~63 Metering group */
	/* PpeSetMtrByteInfo(1, 500, 3); //TokenRate=500=500KB/s, MaxBkSize= 3 (32K-1B) */
	/* PpeSetMtrPktInfo(1, 5, 3);  //1 pkts/sec, MaxBkSize=3 (32K-1B) */

	/* Initialize PPE related register */
	ppe_eng_start();

	/* In manual mode, PPE always reports UN-HIT CPU reason, so we don't need to process it */
	/* Register RX/TX hook point */
#if ! defined (CONFIG_HW_NAT_MANUAL_MODE)
	ra_sw_nat_hook_tx = ppe_tx_handler;
	ra_sw_nat_hook_rx = ppe_rx_handler;
#endif
#if defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
	ppe_dev_register_hook = ppe_dev_reg_handler;
	ppe_dev_unregister_hook = ppe_dev_unreg_handler;
#endif
	/* Set GMAC fowrards packet to PPE */
#if defined(CONFIG_RALINK_MT7620)
	if ((reg_read(0xB000000C) & 0xf) < 0x5) {
		set_acl_fwd(1);
	} else {
		u32 reg;

		/* Turn On UDP Control */
		reg = reg_read(0xB0100F80);
		reg &= ~(0x1 << 30);
		reg_write(0xB0100F80, reg);
	}
#endif
	set_gdma_fwd(1);

#if defined(CONFIG_RALINK_MT7620)
/*For FastPath disable crc check*/
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
	reg_write(0xb0100600, 0x20010000);
	reg_write(0xb0100d00, 0x0);
#endif
#endif

#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
	hnat_pptp_l2tpinit();
#endif
	register_netevent_notifier(&Hnat_netevent_nb);
	init_timer(&hwnat_clear_entry_timer);
	hwnat_clear_entry_timer.function = hwnat_clear_entry;

#if defined (CONFIG_HW_NAT_IPI)
	HnatIPIInit();
#endif

	return 0;
}

static void ppe_cleanup_mod(void)
{
	NAT_PRINT("Ralink HW NAT Module Disabled\n");

	/* Set GMAC fowrards packet to CPU */
#if defined(CONFIG_RALINK_MT7620)
	if ((reg_read(0xB000000C) & 0xf) < 0x5)
		set_acl_fwd(0);
#endif
	set_gdma_fwd(0);

	/* Unregister RX/TX hook point */
	ra_sw_nat_hook_rx = NULL;
	ra_sw_nat_hook_tx = NULL;
#if defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
	ppe_dev_register_hook = NULL;
	ppe_dev_unregister_hook = NULL;
#endif

	/* Restore PPE related register */
	/* ppe_eng_stop(); */
	/* iounmap(ppe_foe_base); */

	/* Unregister ioctl handler */
	ppe_unreg_ioctl_handler();
	ppe_set_sw_vlan_chk(1);
	foe_ac_update_ebl(0);
#if defined(CONFIG_RAETH_QDMA) && defined(CONFIG_PPE_MCAST)
	foe_mcast_entry_del_all();
#endif

	/* Release net_device structure of Dest Port */
	ppe_set_dst_port(0);

#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
	hnat_pptp_l2tp_clean();
#endif
#if defined (CONFIG_HW_NAT_IPI)
	HnatIPIDeInit();
#endif
    unregister_netevent_notifier(&Hnat_netevent_nb);
}

module_init(ppe_init_mod);
module_exit(ppe_cleanup_mod);

MODULE_AUTHOR("Steven Liu/Kurtis Ke");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Ralink Hardware NAT v2.50\n");
