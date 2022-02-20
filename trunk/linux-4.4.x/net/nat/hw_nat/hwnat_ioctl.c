/*   Module Name:
*    hwnat_ioctl.c
*
*    Abstract:
*
*    Revision History:
*    Who         When            What
*    --------    ----------      ----------------------------------------------
*    Name        Date            Modification logs
*    Steven Liu  2006-10-06      Initial version
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "ra_nat.h"

#include "frame_engine.h"
#include "hwnat_ioctl.h"
#include "foe_fdb.h"
#include "util.h"
#include "api.h"

#if defined(CONFIG_PPE_MCAST)
#include "mcast_tbl.h"
#endif

unsigned char bind_dir __read_mostly = BIDIRECTION;
unsigned short lan_vid __read_mostly = CONFIG_RA_HW_NAT_LAN_VLANID;
extern unsigned short wan_vid;
int debug_level;

#if defined (CONFIG_HW_NAT_IPI)
extern int HnatIPITimerSetup(void);
extern hnat_ipi_cfg* hnat_ipi_config;
extern hnat_ipi_s* hnat_ipi_from_extif[NR_CPUS];
extern hnat_ipi_s* hnat_ipi_from_ppehit[NR_CPUS];
extern hnat_ipi_stat* hnat_ipi_status[NR_CPUS];
#endif
extern struct net_device	*dst_port[MAX_IF_NUM];
long hw_nat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct hwnat_args *opt = (struct hwnat_args *)arg;
	struct hwnat_tuple *opt2 =(struct hwnat_tuple *)arg;
	struct hwnat_tuple *opt2_k;
	struct hwnat_ac_args *opt3 = (struct hwnat_ac_args *)arg;
	struct hwnat_ac_args *opt3_k;
	struct hwnat_config_args *opt4 = (struct hwnat_config_args *)arg;
	struct hwnat_config_args *opt4_k;
#if defined(CONFIG_PPE_MCAST)
	struct hwnat_mcast_args *opt5 = (struct hwnat_mcast_args *)arg;
	struct hwnat_mcast_args *opt5_k;
#endif

#if defined(CONFIG_PPE_MIB)
	struct hwnat_mib_args *opt6 = (struct hwnat_mib_args *)arg;
	struct hwnat_mib_args *opt6_k;
	unsigned long tx_pkt_cnt;
	unsigned long tx_byte_cnt;
	unsigned long rx_pkt_cnt;
	unsigned long rx_byte_cnt;
#endif
	struct hwnat_args *opt1;
#if defined (CONFIG_HW_NAT_IPI)
	struct hwnat_ipi_args *opt8 = (struct hwnat_ipi_args *)arg;
	struct hwnat_ipi_args *opt8_k;
	struct hwnat_ipi_args *opt7 = (struct hwnat_ipi_args *)arg;
	struct hwnat_ipi_args *opt7_k;
#endif
	int size;
#if defined (CONFIG_HW_NAT_IPI)
 	int i,j;
#endif
	size = sizeof(struct hwnat_args) + sizeof(struct hwnat_tuple) * 1024 * 16;
	switch (cmd) {
	case HW_NAT_ADD_ENTRY:
		opt2_k = kmalloc(sizeof(*opt2_k), GFP_KERNEL);
		if (copy_from_user(opt2_k, opt2, sizeof(*opt2_k)))
			pr_info("copy_from_user fail\n");
		opt2_k->result = FoeAddEntry(opt2_k);
		kfree(opt2_k);
		break;
	case HW_NAT_DEL_ENTRY:
		pr_info("HW_NAT_DEL_ENTRY\n");
		opt2_k = kmalloc(sizeof(*opt2_k), GFP_KERNEL);
		if (copy_from_user(opt2_k, opt2, sizeof(*opt2_k)))
			pr_info("copy_from_user fail\n");
		opt2_k->result = FoeDelEntry(opt2_k);
		kfree(opt2_k);
		break;
	case HW_NAT_GET_ALL_ENTRIES:

		opt1 = kmalloc(size, GFP_KERNEL);
		if (copy_from_user(opt1, opt, size))
			pr_info("copy_from_user fail\n");
		opt1->result = foe_get_all_entries(opt1);
		if (copy_to_user(opt, opt1, size))
			pr_info("copy_to_user fail\n");

		kfree(opt1);
		break;
	case HW_NAT_BIND_ENTRY:
		opt1 = kmalloc(sizeof(*opt1), GFP_KERNEL);
		if (copy_from_user(opt1, opt, sizeof(struct hwnat_args)))
			pr_debug("copy_from_user fail\n");
		opt1->result = foe_bind_entry(opt1);
		kfree(opt1);
		break;
	case HW_NAT_UNBIND_ENTRY:
		opt1 = kmalloc(sizeof(*opt1), GFP_KERNEL);
		if (copy_from_user(opt1, opt, sizeof(struct hwnat_args)))
			pr_debug("copy_from_user fail\n");
		opt1->result = foe_un_bind_entry(opt1);
		kfree(opt1);
		break;
	case HW_NAT_DROP_ENTRY:
		opt1 = kmalloc(sizeof(*opt1), GFP_KERNEL);
		if (copy_from_user(opt1, opt, sizeof(struct hwnat_args)))
			pr_debug("copy_from_user fail\n");
		opt1->result = foe_drop_entry(opt1);
		kfree(opt1);
		break;
	case HW_NAT_INVALID_ENTRY:
		opt1 = kmalloc(sizeof(*opt1), GFP_KERNEL);
		if (copy_from_user(opt1, opt, sizeof(struct hwnat_args)))
			pr_debug("copy_from_user fail\n");
		opt1->result = foe_del_entry_by_num(opt1->entry_num);
		kfree(opt1);
		break;
	case HW_NAT_DUMP_ENTRY:
		opt1 = kmalloc(size, GFP_KERNEL);
		if (copy_from_user(opt1, opt, sizeof(struct hwnat_args)))
			pr_debug("copy_from_user fail\n");
		foe_dump_entry(opt1->entry_num);
		kfree(opt1);
		break;
	case HW_NAT_DUMP_CACHE_ENTRY:
		foe_dump_cache_entry();
		break;
	case HW_NAT_DEBUG:	/* For Debug */
		opt1 = kmalloc(size, GFP_KERNEL);
		if (copy_from_user(opt1, opt, sizeof(struct hwnat_args)))
			pr_debug("copy_from_user fail\n");
		debug_level = opt1->debug;
		kfree(opt1);
		break;
	case HW_NAT_GET_AC_CNT:
		opt3_k = kmalloc(sizeof(*opt3_k), GFP_KERNEL);
		if (copy_from_user(opt3_k, opt3, sizeof(*opt3_k)))
			pr_debug("copy_from_user fail\n");
		opt3_k->result = ppe_get_agcnt(opt3_k);
		if (copy_to_user(opt3, opt3_k, sizeof(*opt3_k)))
			pr_info("copy_to_user fail\n");
		kfree(opt3_k);
		break;
	case HW_NAT_BIND_THRESHOLD:
		opt4_k = kmalloc(sizeof(*opt4_k), GFP_KERNEL);
		if (copy_from_user(opt4_k, opt4, sizeof(struct hwnat_config_args)))
			pr_debug("copy_from_user fail\n");
		opt4_k->result = ppe_set_bind_threshold(opt4_k->bind_threshold);
		kfree(opt4_k);
		break;
	case HW_NAT_MAX_ENTRY_LMT:
		opt4_k = kmalloc(sizeof(*opt4_k), GFP_KERNEL);
		if (copy_from_user(opt4_k, opt4, sizeof(struct hwnat_config_args)))
			pr_debug("copy_from_user fail\n");
		opt4_k->result =
		    ppe_set_max_entry_limit(opt4_k->foe_full_lmt,
					    opt4_k->foe_half_lmt, opt4_k->foe_qut_lmt);
		kfree(opt4_k);
		break;
	case HW_NAT_KA_INTERVAL:
		opt4_k = kmalloc(sizeof(*opt4_k), GFP_KERNEL);
		if (copy_from_user(opt4_k, opt4, sizeof(struct hwnat_config_args)))
			pr_debug("copy_from_user fail\n");
		opt4_k->result = ppe_set_ka_interval(opt4->foe_tcp_ka, opt4->foe_udp_ka);
		kfree(opt4_k);
		break;
	case HW_NAT_UB_LIFETIME:
		opt4_k = kmalloc(sizeof(*opt4_k), GFP_KERNEL);
		if (copy_from_user(opt4_k, opt4, sizeof(struct hwnat_config_args)))
			pr_debug("copy_from_user fail\n");
		opt4_k->result = ppe_set_unbind_lifetime(opt4_k->foe_unb_dlta);
		kfree(opt4_k);
		break;
	case HW_NAT_BIND_LIFETIME:
		opt4_k = kmalloc(sizeof(*opt4_k), GFP_KERNEL);
		if (copy_from_user(opt4_k, opt4, sizeof(struct hwnat_config_args)))
			pr_debug("copy_from_user fail\n");
		opt4_k->result =
		    ppe_set_bind_lifetime(opt4_k->foe_tcp_dlta,
					  opt4_k->foe_udp_dlta, opt4_k->foe_fin_dlta);
		kfree(opt4_k);
		break;
	case HW_NAT_BIND_DIRECTION:
		opt4_k = kmalloc(sizeof(*opt4_k), GFP_KERNEL);
		if (copy_from_user(opt4_k, opt4, sizeof(struct hwnat_config_args)))
			pr_debug("copy_from_user fail\n");
		bind_dir = opt4_k->bind_dir;
		kfree(opt4_k);
		break;
	case HW_NAT_VLAN_ID:
		opt4_k = kmalloc(sizeof(*opt4_k), GFP_KERNEL);
		if (copy_from_user(opt4_k, opt4, sizeof(struct hwnat_config_args)))
			pr_debug("copy_from_user fail\n");
		wan_vid = opt4_k->wan_vid;
		lan_vid = opt4_k->lan_vid;
		kfree(opt4_k);
		break;
#if defined(CONFIG_PPE_MCAST)
	case HW_NAT_MCAST_INS:
		opt5_k = kmalloc(sizeof(*opt5_k), GFP_KERNEL);
		if (copy_from_user(opt5_k, opt5, sizeof(struct hwnat_mcast_args)))
			pr_debug("copy_from_user fail\n");
		foe_mcast_entry_ins(opt5_k->mc_vid, opt5_k->dst_mac, opt5_k->mc_px_en,
				    opt5_k->mc_px_qos_en, opt5_k->mc_qos_qid);
		kfree(opt5_k);
		break;
	case HW_NAT_MCAST_DEL:
		opt5_k = kmalloc(sizeof(*opt5_k), GFP_KERNEL);
		if (copy_from_user(opt5_k, opt5, sizeof(struct hwnat_mcast_args)))
			pr_debug("copy_from_user fail\n");
		foe_mcast_entry_del(opt5->mc_vid, opt5->dst_mac, opt5->mc_px_en, opt5->mc_px_qos_en,
				    opt5->mc_qos_qid);
		kfree(opt5_k);
		break;
	case HW_NAT_MCAST_DUMP:
		foe_mcast_entry_dump();
		break;
#endif				/* CONFIG_PPE_MCAST // */

#if defined(CONFIG_PPE_MIB)
	case HW_NAT_MIB_DUMP:
		opt6_k = kmalloc(sizeof(*opt6_k), GFP_KERNEL);
		if (copy_from_user(opt6_k, opt6, sizeof(struct hwnat_mib_args)))
			pr_debug("copy_from_user fail\n");
		ppe_mib_dump(opt6_k->entry_num, &tx_pkt_cnt, &tx_byte_cnt);
		kfree(opt6_k);
		break;
	case HW_NAT_MIB_DRAM_DUMP:
		opt6_k = kmalloc(sizeof(*opt6_k), GFP_KERNEL);
		if (copy_from_user(opt6_k, opt6, sizeof(struct hwnat_mib_args)))
			pr_debug("copy_from_user fail\n");
		ppe_mib_dram_dump(opt6_k->entry_num);
		kfree(opt6_k);
		break;
	case HW_NAT_MIB_GET:
		opt2_k = kmalloc(sizeof(*opt2_k), GFP_KERNEL);
		if (copy_from_user(opt2_k, opt2, sizeof(*opt2_k)))
			pr_info("copy_from_user fail\n");
		opt2_k->result = get_ppe_mib(opt2_k, &tx_pkt_cnt, &tx_byte_cnt, &rx_pkt_cnt, &rx_byte_cnt);
		pr_info("!!!!, tx byte = %lu\n", tx_byte_cnt);
		pr_info("!!!!, tx pkt = %lu\n", tx_pkt_cnt);
		pr_info("!!!!, rx byte = %lu\n", rx_byte_cnt);
		pr_info("!!!!, rx pkt = %lu\n", rx_pkt_cnt);
		kfree(opt2_k);
		break;		
#endif
	case HW_NAT_TBL_CLEAR:
		ppe_tbl_clear();
		break;
	case HW_NAT_IPI_CTRL_FROM_EXTIF:
#if defined (CONFIG_HW_NAT_IPI)				
		/* Dora */
		opt8_k = kmalloc(sizeof(*opt8_k), GFP_KERNEL);
		if (copy_from_user(opt8_k, opt8, sizeof(*opt8_k)))
			pr_info("copy_from_user fail\n");
		local_irq_disable();
		if ((opt8->hnat_ipi_enable==1) && (hnat_ipi_config->enable_from_extif!=1))
		{
			hnat_ipi_config->enable_from_extif = opt8_k->hnat_ipi_enable;
			HnatIPITimerSetup();
		}
		else
			hnat_ipi_config->enable_from_extif = opt8_k->hnat_ipi_enable;	

		hnat_ipi_config->queue_thresh_from_extif = opt8_k->queue_thresh;
		hnat_ipi_config->drop_pkt_from_extif = opt8_k->drop_pkt;
		hnat_ipi_config->ipi_cnt_mod_from_extif = opt8_k->ipi_cnt_mod;
		local_irq_enable();
		printk("*** [FromExtIf]hnat_ipi_enable=%d, queue_thresh=%d, drop_pkt=%d ***\n", \
							hnat_ipi_config->enable_from_extif, 
							hnat_ipi_config->queue_thresh_from_extif, 
							hnat_ipi_config->drop_pkt_from_extif);
		if (hnat_ipi_config->enable_from_extif == 1) {
			hnat_ipi_s* phnat_ipi;
			hnat_ipi_stat* phnat_ipi_status;		
		//if (1) {
			extern unsigned int ipidbg[NR_CPUS][10];
			for (i=0; i<NR_CPUS; i++)
			{
				phnat_ipi = hnat_ipi_from_extif[i];
				phnat_ipi_status = hnat_ipi_status[i];	
#if defined (HNAT_IPI_DQ)				
				printk("skbQueue[%d].qlen=%d,%d, dropPktNum[%d]=%d, cpu_status[%d]=%d, smp_call_cnt[%d]=%d\n", i, \
								phnat_ipi->skbInputQueue.qlen,phnat_ipi->skbProcessQueue.qlen, i, \
								phnat_ipi_status->dropPktNum_from_extif,i,\
								atomic_read(&phnat_ipi_status->cpu_status_from_extif),i, \
								phnat_ipi_status->smp_call_cnt_from_extif);
#elif defined	(HNAT_IPI_RXQUEUE)
				printk("RxQueueNum[%d]=%d, dropPktNum[%d]=%d, cpu_status[%d]=%d, smp_call_cnt[%d]=%d\n", i, \
								phnat_ipi->RxQueueNum, i, \
								phnat_ipi_status->dropPktNum_from_extif,i,\
								atomic_read(&phnat_ipi_status->cpu_status_from_extif),i, \
								phnat_ipi_status->smp_call_cnt_from_extif);
#else
				printk("skbIpiQueue[%d].qlen=%d, dropPktNum[%d]=%d, cpu_status[%d]=%d, smp_call_cnt[%d]=%d\n", i, \
								skb_queue_len(&phnat_ipi->skbIpiQueue), i,\
								phnat_ipi_status->dropPktNum_from_extif,i,\
								atomic_read(&phnat_ipi_status->cpu_status_from_extif),i,\
								phnat_ipi_status->smp_call_cnt_from_extif);
#endif
				phnat_ipi_status->dropPktNum_from_extif=0;
				phnat_ipi_status->smp_call_cnt_from_extif=0;
			}
			for(i=0;i<10;i++){
				for(j=0; j<NR_CPUS; j++) {
					printk("dbg[%d][%d]=%d,",j, i, ipidbg[j][i]);
					if (j == 3)
						printk("\n");
				}
			}
			memset(ipidbg, 0, sizeof(ipidbg));
		}
		kfree(opt8_k);
#endif		
		
		break;
	case HW_NAT_IPI_CTRL_FROM_PPEHIT:
		/* Dora */
#if defined (CONFIG_HW_NAT_IPI)
		opt7_k = kmalloc(sizeof(*opt7_k), GFP_KERNEL);
		if (copy_from_user(opt7_k, opt7, sizeof(*opt7_k)))
			pr_info("copy_from_user fail\n");
		local_irq_disable();
		printk("*** [FromPPE]hnat_ipi_enable=%d, queue_thresh=%d, drop_pkt=%d ***\n", \
          				hnat_ipi_config->enable_from_ppehit, \
          				hnat_ipi_config->queue_thresh_from_ppehit, \
          				hnat_ipi_config->drop_pkt_from_ppehit);
		if ((opt7->hnat_ipi_enable==1) && (hnat_ipi_config->enable_from_ppehit!=1))
		{
			hnat_ipi_config->enable_from_ppehit = opt7_k->hnat_ipi_enable;
			HnatIPITimerSetup();
		}
		else
			hnat_ipi_config->enable_from_ppehit = opt7_k->hnat_ipi_enable;
			
		hnat_ipi_config->queue_thresh_from_ppehit = opt7_k->queue_thresh;
		hnat_ipi_config->drop_pkt_from_ppehit = opt7_k->drop_pkt;
		hnat_ipi_config->ipi_cnt_mod_from_ppehit = opt7_k->ipi_cnt_mod;
		local_irq_enable();
		
		if (hnat_ipi_config->enable_from_ppehit == 1) {
			hnat_ipi_s* phnat_ipi;
			hnat_ipi_stat* phnat_ipi_status;		
		//if (1) {
			extern unsigned int ipidbg2[NR_CPUS][10];
			for (i=0; i<NR_CPUS; i++)
			{
				phnat_ipi = hnat_ipi_from_ppehit[i];
				phnat_ipi_status = hnat_ipi_status[i];	
#if defined (HNAT_IPI_DQ)				
				printk("skbQueue[%d].qlen=%d,%d, dropPktNum[%d]=%d, cpu_status[%d]=%d, smp_call_cnt[%d]=%d\n", i, \
								phnat_ipi->skbInputQueue.qlen,phnat_ipi->skbProcessQueue.qlen, i, \
								phnat_ipi_status->dropPktNum_from_ppehit,i,\
								atomic_read(&phnat_ipi_status->cpu_status_from_ppehit),i, \
								phnat_ipi_status->smp_call_cnt_from_ppehit);
#elif defined	(HNAT_IPI_RXQUEUE)
				printk("RxQueueNum[%d]=%d, dropPktNum[%d]=%d, cpu_status[%d]=%d, smp_call_cnt[%d]=%d\n", i, \
								phnat_ipi->RxQueueNum, i, \
								phnat_ipi_status->dropPktNum_from_ppehit,i,\
								atomic_read(&phnat_ipi_status->cpu_status_from_ppehit),i, \
								phnat_ipi_status->smp_call_cnt_from_ppehit);
#else
				printk("skbIpiQueue[%d].qlen=%d, dropPktNum[%d]=%d, cpu_status[%d]=%d, smp_call_cnt[%d]=%d\n", i, \
								skb_queue_len(&phnat_ipi->skbIpiQueue), i,\
								phnat_ipi_status->dropPktNum_from_ppehit,i,\
								atomic_read(&phnat_ipi_status->cpu_status_from_ppehit),i,\
								phnat_ipi_status->smp_call_cnt_from_ppehit);
#endif
				phnat_ipi_status->dropPktNum_from_ppehit=0;
				phnat_ipi_status->smp_call_cnt_from_ppehit=0;
			}
			for(i=0;i<10;i++){
				for(j=0; j<NR_CPUS; j++) {
					printk("dbg2[%d][%d]=%d,",j, i, ipidbg2[j][i]);
					if (j == 3)
						printk("\n");
				}
			}
			memset(ipidbg2, 0, sizeof(ipidbg2));
		}
		kfree(opt7_k);
#endif		
		
		break;
	case HW_NAT_DPORT:
		dump_dport();
		break;
	default:
		break;
	}
	return 0;
}

const struct file_operations hw_nat_fops = {
unlocked_ioctl:hw_nat_ioctl,
};

int ppe_reg_ioctl_handler(void)
{
	int result = 0;

	result = register_chrdev(HW_NAT_MAJOR, HW_NAT_DEVNAME, &hw_nat_fops);
	if (result < 0) {
		NAT_PRINT(KERN_WARNING "hw_nat: can't get major %d\n", HW_NAT_MAJOR);
		return result;
	}

	if (HW_NAT_MAJOR == 0)
		pr_debug("HNAT Major num=%d\n", result);

	return 0;
}

void ppe_unreg_ioctl_handler(void)
{
	unregister_chrdev(HW_NAT_MAJOR, HW_NAT_DEVNAME);
}

int reply_entry_idx(struct hwnat_tuple *opt, unsigned int entry_num)
{
	struct foe_entry *entry = &ppe_foe_base[entry_num];
	struct foe_pri_key key;
	int32_t hash_index;
	
	if(opt->pkt_type == IPV4_NAPT) {
		key.ipv4_hnapt.sip=entry->ipv4_hnapt.new_dip;
		key.ipv4_hnapt.dip=entry->ipv4_hnapt.new_sip;
		key.ipv4_hnapt.sport=entry->ipv4_hnapt.new_dport;
		key.ipv4_hnapt.dport=entry->ipv4_hnapt.new_sport;
		key.ipv4_hnapt.is_udp=opt->is_udp;
		
	} else if (opt->pkt_type == IPV6_ROUTING) {
#if defined(CONFIG_RA_HW_NAT_IPV6)
		key.ipv6_routing.sip0 = entry->ipv6_5t_route.ipv6_dip0;
		key.ipv6_routing.sip1 = entry->ipv6_5t_route.ipv6_dip1;
		key.ipv6_routing.sip2 = entry->ipv6_5t_route.ipv6_dip2;
		key.ipv6_routing.sip3 = entry->ipv6_5t_route.ipv6_dip3;
		key.ipv6_routing.dip0 = entry->ipv6_5t_route.ipv6_sip0;
		key.ipv6_routing.dip1 = entry->ipv6_5t_route.ipv6_sip1;
		key.ipv6_routing.dip2 = entry->ipv6_5t_route.ipv6_sip2;
		key.ipv6_routing.dip3 = entry->ipv6_5t_route.ipv6_sip3;
		key.ipv6_routing.sport = entry->ipv6_5t_route.dport;
		key.ipv6_routing.dport = entry->ipv6_5t_route.sport;
		key.ipv6_routing.is_udp=opt->is_udp;
#endif
	}
	entry = NULL;
	key.pkt_type=opt->pkt_type;
	hash_index = get_mib_entry_idx(&key, entry);
	if (debug_level >= 1) {
		pr_info("reply entry idx = %d\n", hash_index);
	}
	return hash_index;
}

#if defined(CONFIG_PPE_MIB)
void ppe_mib_dram_dump(uint32_t entry_num)
{
	struct mib_entry *mib_entry = &ppe_mib_base[entry_num];

	pr_info("***********DRAM PPE Entry = %d*********\n", entry_num);
	pr_info("PpeMibBase = %p\n", ppe_mib_base);
	
	pr_info("DRAM Packet_CNT H = %u\n", mib_entry->pkt_cnt_h);
	pr_info("DRAM Packet_CNT L = %u\n", mib_entry->pkt_cnt_l);
	pr_info("DRAM Byte_CNT H = %u\n", mib_entry->byt_cnt_h);
	pr_info("DRAM Byte_CNT L = %u\n", mib_entry->byt_cnt_l);	

}

void ppe_mib_dump(unsigned int entry_num, unsigned long *pkt_cnt, unsigned long *byte_cnt)
{
	unsigned int byt_l = 0;
	unsigned long long byt_h = 0;
	unsigned int pkt_l = 0;

	unsigned int long pkt_h = 0;
	reg_write(MIB_SER_CR, entry_num | (1 << 16));
	while (1) {
		if (!((reg_read(MIB_SER_CR) & 0x10000) >> 16))
			break;
	}
	/*make sure write dram correct*/
	wmb();
	byt_l = reg_read(MIB_SER_R0);	/* byte cnt bit31~ bit0 */
	byt_h = reg_read(MIB_SER_R1) & 0xffff;	/* byte cnt bit47 ~ bit0 */
	pkt_l = (reg_read(MIB_SER_R1) & 0xffff0000) >> 16;
	pkt_h = reg_read(MIB_SER_R2) & 0xffffff;	/* packet cnt bit39 ~ bit16 */
	if (debug_level >= 1) {
		pr_info("************PPE Entry = %d ************\n", entry_num);
		pr_info("Packet Cnt = %lu\n", (pkt_h << 16) + pkt_l);
		pr_info("Byte Cnt = %llu\n", (byt_h << 32) + byt_l);
	}
	*pkt_cnt = (pkt_h << 16) + pkt_l;
	*byte_cnt = (byt_h << 32) + byt_l;
}

int get_ppe_mib(struct hwnat_tuple *opt, unsigned long *tx_pkt_cnt, unsigned long *tx_byte_cnt,
		  unsigned long *rx_pkt_cnt, unsigned long *rx_byte_cnt)
{
	struct foe_pri_key key;
	struct foe_entry *entry = NULL;
	int32_t hash_index;
	int32_t rply_idx;
	/*pr_info("sip = %x, dip=%x, sp=%d, dp=%d\n", opt->ing_sipv4, opt->ing_dipv4, opt->ing_sp, opt->ing_dp);*/
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
	hash_index = get_mib_entry_idx(&key, entry);
	
	if(hash_index != -1) {
		ppe_mib_dump(hash_index, tx_pkt_cnt, tx_byte_cnt);
		rply_idx = reply_entry_idx(opt, hash_index);
		if(rply_idx != -1) {
			ppe_mib_dump(rply_idx, rx_pkt_cnt, rx_byte_cnt);
		} else if (rply_idx == -1) {
			*rx_pkt_cnt = 0;
			*rx_byte_cnt = 0;
		}
		return HWNAT_SUCCESS;
	}

	return HWNAT_FAIL;

}
EXPORT_SYMBOL(get_ppe_mib);
#endif

int ppe_get_agcnt(struct hwnat_ac_args *opt3)
{
	unsigned int ag_idx = 0;
	ag_idx = opt3->ag_index;
	if (ag_idx > 63)
		return HWNAT_FAIL;
#ifdef CONFIG_RA_HW_NAT_ACCNT_MAINTAINER
extern struct hwnat_ac_args ac_info[64];
#if defined(CONFIG_RALINK_MT7620)
	ac_info[ag_idx].ag_byte_cnt += reg_read(AC_BASE + ag_idx * 8);	/* Low bytes */
	ac_info[ag_idx].ag_pkt_cnt += reg_read(AC_BASE + ag_idx * 8 + 4);	/* High bytes */
#elif defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || defined(CONFIG_ARCH_MT7622)
	ac_info[ag_idx].ag_byte_cnt += reg_read(AC_BASE + ag_idx * 16);	/* 64bit bytes cnt */
	ac_info[ag_idx].ag_byte_cnt +=
	    ((unsigned long long)(reg_read(AC_BASE + ag_idx * 16 + 4)) << 32);
	ac_info[ag_idx].ag_pkt_cnt += reg_read(AC_BASE + ag_idx * 16 + 8);	/* 32bites packet cnt */
#endif
	opt3->ag_byte_cnt = ac_info[ag_idx].ag_byte_cnt;
	opt3->ag_pkt_cnt = ac_info[ag_idx].ag_pkt_cnt;
#else
#if defined(CONFIG_RALINK_MT7620)
	opt3->ag_byte_cnt = reg_read(AC_BASE + ag_idx * 8);	/* Low bytes */
	opt3->ag_pkt_cnt = reg_read(AC_BASE + ag_idx * 8 + 4);	/* High bytes */
#elif defined(CONFIG_RALINK_MT7621) || defined(CONFIG_MACH_MT7623) || defined(CONFIG_ARCH_MT7622)
	opt3->ag_byte_cnt = reg_read(AC_BASE + ag_idx * 16);	/* 64bit bytes cnt */
	opt3->ag_byte_cnt +=
	    ((unsigned long long)(reg_read(AC_BASE + ag_idx * 16 + 4)) << 32);
	opt3->ag_pkt_cnt = reg_read(AC_BASE + ag_idx * 16 + 8);	/* 32bites packet cnt */
#endif
#endif

	return HWNAT_SUCCESS;
}

int ppe_set_bind_threshold(uint32_t threshold)
{
	/* Set reach bind rate for unbind state */
	reg_write(PPE_FOE_BNDR, threshold);

	return HWNAT_SUCCESS;
}

int ppe_set_max_entry_limit(u32 full, uint32_t half, uint32_t qurt)
{
	/* Allowed max entries to be build during a time stamp unit */

	/* smaller than 1/4 of total entries */
	reg_modify_bits(PPE_FOE_LMT1, qurt, 0, 14);

	/* between 1/2 and 1/4 of total entries */
	reg_modify_bits(PPE_FOE_LMT1, half, 16, 14);

	/* between full and 1/2 of total entries */
	reg_modify_bits(PPE_FOE_LMT2, full, 0, 14);

	return HWNAT_SUCCESS;
}

int ppe_set_ka_interval(u8 tcp_ka, uint8_t udp_ka)
{
	/* Keep alive time for bind FOE TCP entry */
	reg_modify_bits(PPE_FOE_KA, tcp_ka, 16, 8);

	/* Keep alive timer for bind FOE UDP entry */
	reg_modify_bits(PPE_FOE_KA, udp_ka, 24, 8);

	return HWNAT_SUCCESS;
}

int ppe_set_unbind_lifetime(uint8_t lifetime)
{
	/* set Delta time for aging out an unbind FOE entry */
	reg_modify_bits(PPE_FOE_UNB_AGE, lifetime, 0, 8);

	return HWNAT_SUCCESS;
}

int ppe_set_bind_lifetime(u16 tcp_life, uint16_t udp_life, uint16_t fin_life)
{
	/* set Delta time for aging out an bind UDP FOE entry */
	reg_modify_bits(PPE_FOE_BND_AGE0, udp_life, 0, 16);

	/* set Delta time for aging out an bind TCP FIN FOE entry */
	reg_modify_bits(PPE_FOE_BND_AGE1, fin_life, 16, 16);

	/* set Delta time for aging out an bind TCP FOE entry */
	reg_modify_bits(PPE_FOE_BND_AGE1, tcp_life, 0, 16);

	return HWNAT_SUCCESS;
}

int ppe_tbl_clear(void)
{
	u32 foe_tbl_size;

	reg_modify_bits(PPE_FOE_CFG, ONLY_FWD_CPU, 4, 2);
	foe_tbl_size = FOE_4TB_SIZ * sizeof(struct foe_entry);
	memset(ppe_foe_base, 0, foe_tbl_size);
	ppe_set_cache_ebl();	/*clear HWNAT cache */
	reg_modify_bits(PPE_FOE_CFG, FWD_CPU_BUILD_ENTRY, 4, 2);

	return HWNAT_SUCCESS;
}

void dump_dport(void)
{
	int i;
	for (i = 0; i < MAX_IF_NUM; i++) {
		pr_info("dst_port[%d] = %s\n", i, dst_port[i]->name);
	}
}
