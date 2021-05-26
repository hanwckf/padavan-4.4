#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <linux/cpu_rmap.h>
#include "ra_nat.h"
#include "foe_fdb.h"
hnat_ipi_s* hnat_ipi_from_extif[NR_CPUS] ____cacheline_aligned_in_smp;
hnat_ipi_s* hnat_ipi_from_ppehit[NR_CPUS] ____cacheline_aligned_in_smp;
hnat_ipi_stat* hnat_ipi_status[NR_CPUS] ____cacheline_aligned_in_smp;
//hnat_ipi_cfg hnat_ipi_config_ctx ____cacheline_aligned_in_smp;
hnat_ipi_cfg* hnat_ipi_config;// = &hnat_ipi_config_ctx;

extern int get_rps_cpu(struct net_device *dev, struct sk_buff *skb,
                       struct rps_dev_flow **rflowp);
extern uint32_t ppe_extif_rx_handler(struct sk_buff * skb);
extern int hitbind_force_to_cpu_handler(struct sk_buff *skb, struct foe_entry *entry);
extern unsigned int ipidbg[NR_CPUS][10];    
extern unsigned int ipidbg2[NR_CPUS][10];            
unsigned int dbg_var;
unsigned int dbg_var2;
struct timer_list ipi_monitor_timer_from_extif;
struct timer_list ipi_monitor_timer_from_ppehit;

extern int debug_level;

int skb_get_rxhash_ipi(struct sk_buff *skb, u32 hflag)
{
	struct rps_dev_flow voidflow, *rflow = &voidflow;
  	int cpu;
  	unsigned char* old_hdr, *old_data;
	unsigned short old_proto;
   
	preempt_disable();      
	rcu_read_lock();
#if defined (CONFIG_RAETH_QDMA)
	if (hflag&HNAT_IPI_HASH_VTAG)
	{
		struct vlan_ethhdr *veth;
		uint16_t VirIfIdx;

		//veth = (struct vlan_ethhdr *)LAYER2_HEADER(skb);
		veth = (struct vlan_ethhdr *)skb_mac_header(skb);
		//something wrong
		if ((veth->h_vlan_proto != htons(ETH_P_8021Q)) && (veth->h_vlan_proto != 0x5678)){
			ipidbg[smp_processor_id()][6]++;
		}
		VirIfIdx = ntohs(veth->h_vlan_TCI);
#if defined(CONFIG_ARCH_MT7622)
		skb->hash = ((u32)VirIfIdx) << (32-FOE_4TB_BIT);
		skb->l4_hash = 1;
#else		
		skb->rxhash = ((u32)VirIfIdx) << (32-FOE_4TB_BIT);
		skb->l4_rxhash = 1;
#endif
		old_data = skb->data;
		skb->data += 4;
		old_proto = skb->protocol;
		skb->protocol = (*(u16*)(skb->data-2));
	}	
#endif	
	//old_hdr = skb->network_header;

	old_hdr = skb_network_header(skb);
	//old_hdr = skb->data;
	if (debug_level >= 2) {
		printk("00 : skb->head = %p\n", skb->head);
		printk("00 : skb->data = %p\n", skb->data);
		printk("00 : skb->mac_header = %d\n", skb->mac_header);
		printk("00 : skb->network_header = %d\n", skb->network_header);
		printk("00 : old_hdr = %p\n", old_hdr);	
	}
	cpu = get_rps_cpu(skb->dev, skb, &rflow);
	if (debug_level >= 2) {
		printk("11 : skb->head = %p\n", skb->head);
		printk("11 : skb->data = %p\n", skb->data);
		printk("11 : skb->mac_header = %d\n", skb->mac_header);
		printk("11 : skb->network_header = %d\n", skb->network_header);
		printk("11 : old_hdr = %p\n", old_hdr);	
	}
  if (cpu < 0)
	{
		cpu = smp_processor_id();
		if (hflag&HNAT_IPI_HASH_FROM_EXTIF)
			ipidbg[cpu][3]++;
		else
			ipidbg2[cpu][3]++;	
  }
#if defined (CONFIG_RAETH_QDMA)
	if (hflag&HNAT_IPI_HASH_VTAG)
	{	
		skb->data = old_data;
		skb->protocol = old_proto;
	}
#endif	
	//skb->network_header = old_hdr;

	skb_set_network_header(skb, (int)(old_hdr - skb->data));     
	if (debug_level >= 2) {
		printk("22 : skb->head = %p\n", skb->head);
		printk("22 : skb->data = %p\n", skb->data);
		printk("22 : skb->mac_header = %d\n", skb->mac_header);
		printk("22 : skb->network_header = %d\n", skb->network_header);
		printk("22 : old_hdr = %p\n", old_hdr);	
	}
	rcu_read_unlock();
	preempt_enable();
	return cpu;
}

void smp_func_call_BH_handler_from_extif(unsigned long data)
{
	struct sk_buff *skb_deq;
	unsigned int  cpu_num = smp_processor_id();
	unsigned int reSchedule_cnt=0;
	unsigned int bReschedule = 0;
	hnat_ipi_s* phnat_ipi = hnat_ipi_from_extif[cpu_num];
	hnat_ipi_stat* phnat_ipi_status = hnat_ipi_status[cpu_num];	
			
	atomic_set(&phnat_ipi_status->cpu_status_from_extif, 1);
#if defined (HNAT_IPI_DQ)
	while (skb_queue_len(&phnat_ipi->skbProcessQueue)> 0) {	
#elif defined (HNAT_IPI_RXQUEUE)
	//spin_lock(&phnat_ipi->ipilock);	
	while (atomic_read(&phnat_ipi->RxQueueNum) > 0) {
#else		
	while ((skb_queue_len(&phnat_ipi->skbIpiQueue) > 0) && (hnat_ipi_config->enable_from_extif==1)) {	
#endif

#if defined (HNAT_IPI_DQ)
		skb_deq = __skb_dequeue(&phnat_ipi->skbProcessQueue);
#elif defined (HNAT_IPI_RXQUEUE)
		skb_deq = phnat_ipi->RxQueue[phnat_ipi->RxQueueRIdx];
		phnat_ipi->RxQueue[phnat_ipi->RxQueueRIdx] = NULL;
		phnat_ipi->RxQueueRIdx = (phnat_ipi->RxQueueRIdx+1)%1024;
		atomic_sub(1, &phnat_ipi->RxQueueNum);
#else		
		skb_deq = skb_dequeue(&phnat_ipi->skbIpiQueue);
#endif	
		if (skb_deq)
		{	
			ipidbg[cpu_num][8]++;
		  ppe_extif_rx_handler(skb_deq);
		}
		else {
			break;
		}
		
		reSchedule_cnt++;
		if (reSchedule_cnt > hnat_ipi_config->queue_thresh_from_extif)
		{
			ipidbg[cpu_num][9]++;
			bReschedule = 1;
			break;
		}
	}
#if defined (HNAT_IPI_DQ)
	spin_lock(&phnat_ipi->ipilock);	
	if (skb_queue_len(&phnat_ipi->skbProcessQueue)==0)
	{	
		unsigned int qlen = skb_queue_len(&phnat_ipi->skbInputQueue);
		if (qlen)
			skb_queue_splice_tail_init(&phnat_ipi->skbInputQueue,
						   &phnat_ipi->skbProcessQueue);
	}
	spin_unlock(&phnat_ipi->ipilock);	
#endif						   
#ifdef HNAT_IPI_RXQUEUE
	//spin_unlock(&phnat_ipi->ipilock);
#endif

	//atomic_set(&phnat_ipi_status->cpu_status_from_extif, 0);
	if (bReschedule==1)
		tasklet_hi_schedule(&phnat_ipi->smp_func_call_tsk);
	else
		atomic_set(&phnat_ipi_status->cpu_status_from_extif, 0);
	return;
}

static void smp_func_call_from_extif(void *info)
{
	unsigned int cpu = smp_processor_id();
	hnat_ipi_s* phnat_ipi = hnat_ipi_from_extif[cpu];	
	phnat_ipi->smp_func_call_tsk.data = cpu;
	ipidbg[cpu][5]++;
	if ((hnat_ipi_config->enable_from_extif ==1) && (phnat_ipi))
		tasklet_hi_schedule(&phnat_ipi->smp_func_call_tsk);
	return;
}

void smp_func_call_BH_handler_from_ppehit(unsigned long data)
{
	struct sk_buff *skb_deq;
	unsigned int  cpu_num = smp_processor_id();
	unsigned int reSchedule_cnt=0;
	struct foe_entry *entry;
	unsigned int bReschedule = 0;
	hnat_ipi_s* phnat_ipi = hnat_ipi_from_ppehit[cpu_num];
	hnat_ipi_stat* phnat_ipi_status = hnat_ipi_status[cpu_num];	
			
	atomic_set(&phnat_ipi_status->cpu_status_from_ppehit, 1);
#if defined (HNAT_IPI_DQ)
	while (skb_queue_len(&phnat_ipi->skbProcessQueue) > 0) {	
#elif defined (HNAT_IPI_RXQUEUE)
	//spin_lock(&phnat_ipi->ipilock);	
	while (atomic_read(&phnat_ipi->RxQueueNum) > 0) {
#else		
	while ((skb_queue_len(&phnat_ipi->skbIpiQueue) > 0) && (hnat_ipi_config->enable_from_ppehit==1)) {	
#endif		
#if defined (HNAT_IPI_DQ)
		skb_deq = __skb_dequeue(&phnat_ipi->skbProcessQueue);
#elif defined (HNAT_IPI_RXQUEUE)
		skb_deq = phnat_ipi->RxQueue[phnat_ipi->RxQueueRIdx];
		phnat_ipi->RxQueue[phnat_ipi->RxQueueRIdx] = NULL;
		phnat_ipi->RxQueueRIdx = (phnat_ipi->RxQueueRIdx+1)%1024;
		atomic_sub(1, &phnat_ipi->RxQueueNum);
#else		
		skb_deq = skb_dequeue(&phnat_ipi->skbIpiQueue);
#endif			
		if (skb_deq)
		{
#if defined (CONFIG_RAETH_QDMA)
			entry = NULL;
#else
			entry = &PpeFoeBase[FOE_ENTRY_NUM(skb_deq)];
#endif			
			hitbind_force_to_cpu_handler(skb_deq, entry);
		}
		else {
			break;
		}
		
		reSchedule_cnt++;
		if (reSchedule_cnt > hnat_ipi_config->queue_thresh_from_ppehit)
		{
			ipidbg2[cpu_num][9]++;
			bReschedule = 1;
			break;
		}
	}
	
#if defined (HNAT_IPI_DQ)
	spin_lock(&phnat_ipi->ipilock);		
	if (skb_queue_len(&phnat_ipi->skbProcessQueue)==0)
	{	
		unsigned int qlen = skb_queue_len(&phnat_ipi->skbInputQueue);
		if (qlen)
			skb_queue_splice_tail_init(&phnat_ipi->skbInputQueue,
						   &phnat_ipi->skbProcessQueue);
	}
	spin_unlock(&phnat_ipi->ipilock);	
#endif	
#ifdef HNAT_IPI_RXQUEUE
        //spin_unlock(&phnat_ipi->ipilock);
#endif

	//atomic_set(&phnat_ipi_status->cpu_status_from_ppehit, 0);
	if (bReschedule==1)
		tasklet_hi_schedule(&phnat_ipi->smp_func_call_tsk);
	else
		atomic_set(&phnat_ipi_status->cpu_status_from_ppehit, 0);
	return;
}

static void smp_func_call_from_ppehit(void *info)
{
	unsigned int cpu = smp_processor_id();
	hnat_ipi_s* phnat_ipi = hnat_ipi_from_ppehit[cpu];	
	phnat_ipi->smp_func_call_tsk.data = cpu;
	ipidbg2[cpu][5]++;
	if ((hnat_ipi_config->enable_from_ppehit ==1) && phnat_ipi)
		tasklet_hi_schedule(&phnat_ipi->smp_func_call_tsk);
	return;
}
int32_t HnatIPIExtIfHandler(struct sk_buff * skb)
{
#if defined (CONFIG_HNAT_V2)
		struct ethhdr *eth = (struct ethhdr *)(skb->data - ETH_HLEN);
#endif
		unsigned int cpu_num;
		unsigned int kickoff_ipi = 1;
		int is_thecpu = 0;
		dbg_var++;
		if (dbg_var ==20)
			printk("=== [FromExtIf]hnat_ipi_enable=%d, queue_thresh=%d, drop_pkt=%d ===\n", \
							hnat_ipi_config->enable_from_extif, 
							hnat_ipi_config->queue_thresh_from_extif, 
							hnat_ipi_config->drop_pkt_from_extif);
		if (hnat_ipi_config->enable_from_extif ==1) {
			//unsigned long delta;
			/*unsigned long cur_jiffies = jiffies;*/
			hnat_ipi_s* phnat_ipi;
			hnat_ipi_stat* phnat_ipi_stat;
			if (((skb->protocol != htons(ETH_P_8021Q)) &&
	     			(skb->protocol != htons(ETH_P_IP)) && (skb->protocol != htons(ETH_P_IPV6)) && 
	     			(skb->protocol != htons(ETH_P_PPP_SES)) && (skb->protocol != htons(ETH_P_PPP_DISC))) ||
	     			is_multicast_ether_addr(&eth->h_dest[0])) 
				return 1;
	
			cpu_num = skb_get_rxhash_ipi(skb,HNAT_IPI_HASH_NORMAL|HNAT_IPI_HASH_FROM_EXTIF);
			if (debug_level >= 1) {
				printk("%s: cpu_num =%d\n", __func__, cpu_num);
			}
			phnat_ipi_stat = hnat_ipi_status[cpu_num];
			if (phnat_ipi_stat==NULL)
			{	
				goto DISABLE_EXTIF_IPI;
			}
			phnat_ipi = hnat_ipi_from_extif[cpu_num];
			if (phnat_ipi==NULL)
			{	
				goto DISABLE_EXTIF_IPI;
			}
			phnat_ipi_stat->smp_call_cnt_from_extif++;
			phnat_ipi->ipi_accum++;
			
#if 0
			if (phnat_ipi->recv_time > cur_jiffies)
			{
				delta = 0xFFFFFFFF - (phnat_ipi->recv_time-cur_jiffies+1);
			}
			else
			{
				delta = (cur_jiffies-phnat_ipi->recv_time+1);
			}
			phnat_ipi->recv_time = cur_jiffies;	
			if (delta > 1)
				kickoff_ipi = 1;
			else
			{	
#endif
				if (phnat_ipi->ipi_accum >= hnat_ipi_config->ipi_cnt_mod_from_extif) {
					kickoff_ipi = 1;
					phnat_ipi->ipi_accum = 0;
				} else
					kickoff_ipi = 0;
#if 0
			}
#endif

			if (cpu_num == smp_processor_id())
			{
				//return ppe_extif_rx_handler(skb);
				is_thecpu = 1;
			}

#if defined (HNAT_IPI_DQ)
			if (skb_queue_len(&phnat_ipi->skbInputQueue) > hnat_ipi_config->drop_pkt_from_extif) {
#elif defined (HNAT_IPI_RXQUEUE) 
			if (atomic_read(&phnat_ipi->RxQueueNum) >= (hnat_ipi_config->drop_pkt_from_extif-1)) {
#else
			if (skb_queue_len(&phnat_ipi->skbIpiQueue) > hnat_ipi_config->drop_pkt_from_extif) {
#endif				

				dev_kfree_skb_any(skb);
				phnat_ipi_stat->dropPktNum_from_extif++;
				if (atomic_read(&phnat_ipi_stat->cpu_status_from_extif) <= 0){
					if (is_thecpu==1)
					tasklet_hi_schedule(&phnat_ipi->smp_func_call_tsk);
					else
					smp_call_function_single(cpu_num, smp_func_call_from_extif, NULL, 0);
					phnat_ipi->time_rec = jiffies;	
				}
				return 0; /* Drop packet */
			} else {
				if (atomic_read(&phnat_ipi_stat->cpu_status_from_extif) <= 0) {

					/* idle state */
#if (defined (HNAT_IPI_DQ) || defined (HNAT_IPI_RXQUEUE))			
					spin_lock(&phnat_ipi->ipilock);
#endif					
#if defined (HNAT_IPI_DQ)
					__skb_queue_tail(&phnat_ipi->skbInputQueue, skb);
#elif defined (HNAT_IPI_RXQUEUE)
					phnat_ipi->RxQueue[phnat_ipi->RxQueueWIdx] = skb;
					phnat_ipi->RxQueueWIdx = (phnat_ipi->RxQueueWIdx+1)%1024;
					atomic_add(1, &phnat_ipi->RxQueueNum);
#else
					skb_queue_tail(&phnat_ipi->skbIpiQueue, skb);
#endif
#if (defined (HNAT_IPI_DQ) || defined (HNAT_IPI_RXQUEUE))			
					spin_unlock(&phnat_ipi->ipilock);
#endif					
					if (kickoff_ipi == 1) {
						if (is_thecpu==1)  {
							tasklet_hi_schedule(&phnat_ipi->smp_func_call_tsk);
						} else {

							smp_call_function_single(cpu_num, smp_func_call_from_extif, NULL, 0);
						}
						phnat_ipi->time_rec = jiffies;
			 		}
				} else {
#if (defined (HNAT_IPI_DQ) || defined (HNAT_IPI_RXQUEUE))			
					spin_lock(&phnat_ipi->ipilock);
#endif					
#if defined (HNAT_IPI_DQ)
					__skb_queue_tail(&phnat_ipi->skbInputQueue, skb);
#elif defined (HNAT_IPI_RXQUEUE)
					phnat_ipi->RxQueue[phnat_ipi->RxQueueWIdx] = skb;
					phnat_ipi->RxQueueWIdx = (phnat_ipi->RxQueueWIdx+1)%1024;
					atomic_add(1, &phnat_ipi->RxQueueNum);
#else
					skb_queue_tail(&phnat_ipi->skbIpiQueue, skb);
#endif
#if (defined (HNAT_IPI_DQ) || defined (HNAT_IPI_RXQUEUE))			
					spin_unlock(&phnat_ipi->ipilock);
#endif
				}
			}
			if (debug_level >= 1) {
				printk("%s, return 0\n", __func__);
			}
			return 0;
		}
		else {
DISABLE_EXTIF_IPI:			
			return ppe_extif_rx_handler(skb);
		}								
}

int32_t HnatIPIForceCPU(struct sk_buff * skb)
{
  unsigned int cpu_num;
#if defined (CONFIG_RAETH_QDMA)
	struct foe_entry *entry = NULL;	
#else				  	
	//struct foe_entry *entry = &PpeFoeBase[FOE_ENTRY_NUM(skb)];	
	struct foe_entry *entry = &ppe_foe_base[FOE_ENTRY_NUM(skb)];				
#endif
	unsigned int kickoff_ipi = 1;
	int is_thecpu = 0;
  	dbg_var2++;
  	if (dbg_var2 ==20)
          	printk("=== [FromPPE]hnat_ipi_enable=%d, queue_thresh=%d, drop_pkt=%d ===\n", \
          				hnat_ipi_config->enable_from_ppehit, \
          				hnat_ipi_config->queue_thresh_from_ppehit, \
          				hnat_ipi_config->drop_pkt_from_ppehit);
  	if (hnat_ipi_config->enable_from_ppehit ==1) {
      		/*unsigned long cur_jiffies = jiffies;*/ 
      		//unsigned long delta = 0;
		hnat_ipi_s* phnat_ipi;
		hnat_ipi_stat* phnat_ipi_stat;
      		cpu_num = skb_get_rxhash_ipi(skb, HNAT_IPI_HASH_VTAG|HNAT_IPI_HASH_FROM_GMAC);
			if (debug_level >= 1) {
				printk("%s: cpu_num =%d\n", __func__, cpu_num);
			}
      		phnat_ipi = hnat_ipi_from_ppehit[cpu_num];
		phnat_ipi_stat = hnat_ipi_status[cpu_num];
		if (phnat_ipi_stat==NULL)
		{
			goto DISABLE_PPEHIT_IPI;
		}			
		if (phnat_ipi==NULL)
		{
			goto DISABLE_PPEHIT_IPI;
		}
      		phnat_ipi_stat->smp_call_cnt_from_ppehit++;
      		phnat_ipi->ipi_accum++;
     
#if 0
		if (phnat_ipi->recv_time > cur_jiffies)
      		{
          		delta = 0xFFFFFFFF - (phnat_ipi->recv_time-cur_jiffies+1);
      		}
      		else
      		{
          		delta = (cur_jiffies-phnat_ipi->recv_time+1);
      		}
      		phnat_ipi->recv_time = cur_jiffies;
      		if (delta > 1)
          		kickoff_ipi = 1;
      		else{
#endif
      			if (phnat_ipi->ipi_accum >= hnat_ipi_config->ipi_cnt_mod_from_ppehit) {
          			kickoff_ipi = 1;
          			phnat_ipi->ipi_accum = 0;
      			} else
          			kickoff_ipi = 0;
#if 0
		} 
#endif
		 
	
		if (cpu_num == smp_processor_id())
		{				  	
			//return  hitbind_force_to_cpu_handler(skb, foe_entry);
            		is_thecpu = 1;
		}


#if defined (HNAT_IPI_DQ)
      if (skb_queue_len(&phnat_ipi->skbInputQueue) > hnat_ipi_config->drop_pkt_from_ppehit) {
#elif defined (HNAT_IPI_RXQUEUE)
      if (atomic_read(&phnat_ipi->RxQueueNum) >= (hnat_ipi_config->drop_pkt_from_ppehit-1)) {
#else
      if (skb_queue_len(&phnat_ipi->skbIpiQueue) > hnat_ipi_config->drop_pkt_from_ppehit) {
#endif

          dev_kfree_skb_any(skb);
          phnat_ipi_stat->dropPktNum_from_ppehit++;
          if (atomic_read(&phnat_ipi_stat->cpu_status_from_ppehit) <= 0){
	  		if (is_thecpu==1)
				tasklet_hi_schedule(&phnat_ipi->smp_func_call_tsk);
			else
	                  smp_call_function_single(cpu_num, smp_func_call_from_ppehit, NULL, 0);
                  phnat_ipi->time_rec = jiffies;
          }
          return 0; /* Drop packet */
      } else {
          if (atomic_read(&phnat_ipi_stat->cpu_status_from_ppehit) <= 0) {

#if (defined (HNAT_IPI_DQ) || defined (HNAT_IPI_RXQUEUE))
          		spin_lock(&phnat_ipi->ipilock);
#endif
              /* idle state */
#if defined (HNAT_IPI_DQ)
          		__skb_queue_tail(&phnat_ipi->skbInputQueue, skb);
#elif defined (HNAT_IPI_RXQUEUE)
          		phnat_ipi->RxQueue[phnat_ipi->RxQueueWIdx] = skb;
          		phnat_ipi->RxQueueWIdx = (phnat_ipi->RxQueueWIdx+1)%1024;
          		atomic_add(1, &phnat_ipi->RxQueueNum);
#else
          		skb_queue_tail(&phnat_ipi->skbIpiQueue, skb);
#endif
#if (defined (HNAT_IPI_DQ) || defined (HNAT_IPI_RXQUEUE))
          		spin_unlock(&phnat_ipi->ipilock);
#endif
          		if (kickoff_ipi == 1) {
				if (is_thecpu==1) {
					tasklet_hi_schedule(&phnat_ipi->smp_func_call_tsk);
				} else {
          				smp_call_function_single(cpu_num, smp_func_call_from_ppehit, NULL, 0);
          			}
            		phnat_ipi->time_rec = jiffies;
          		}
      		} else {
#if (defined (HNAT_IPI_DQ) || defined (HNAT_IPI_RXQUEUE))
      				spin_lock(&phnat_ipi->ipilock);
#endif
#if defined (HNAT_IPI_DQ)
      				__skb_queue_tail(&phnat_ipi->skbInputQueue, skb);
#elif defined (HNAT_IPI_RXQUEUE)
							phnat_ipi->RxQueue[phnat_ipi->RxQueueWIdx] = skb;
							phnat_ipi->RxQueueWIdx = (phnat_ipi->RxQueueWIdx+1)%1024;
							atomic_add(1, &phnat_ipi->RxQueueNum);
#else
							skb_queue_tail(&phnat_ipi->skbIpiQueue, skb);
#endif
#if (defined (HNAT_IPI_DQ) || defined (HNAT_IPI_RXQUEUE))
      				spin_unlock(&phnat_ipi->ipilock);
#endif
      		}
			}

 			return 0;
  }
  else
  {
DISABLE_PPEHIT_IPI:  	
			return hitbind_force_to_cpu_handler(skb, entry);
  }

}

void ipi_monitor_from_extif(unsigned long data)
{
	int i;
	unsigned long delta;
	unsigned long cur_time;

	if (hnat_ipi_config->enable_from_extif==1)
	{
		hnat_ipi_s* phnat_ipi;
		hnat_ipi_stat* phnat_ipi_status;		
		cur_time = jiffies;
		
		for (i=0; i<NR_CPUS; i++) {
			phnat_ipi = hnat_ipi_from_extif[i];
			phnat_ipi_status = hnat_ipi_status[i];	
#if defined (HNAT_IPI_DQ)
			if ( ((skb_queue_len(&phnat_ipi->skbInputQueue) > 0) || \
				(skb_queue_len(&phnat_ipi->skbProcessQueue) > 0)) && \
				(atomic_read(&phnat_ipi_status->cpu_status_from_extif) <= 0)) {
#elif defined (HNAT_IPI_RXQUEUE)
			spin_lock(&phnat_ipi->ipilock);
			if ((atomic_read(&phnat_ipi->RxQueueNum) > 0) && (atomic_read(&phnat_ipi_status->cpu_status_from_extif) <= 0)) {
#else
			if ((skb_queue_len(&phnat_ipi->skbIpiQueue) > 0) && (atomic_read(&phnat_ipi_status->cpu_status_from_extif) <= 0)) {
#endif				
				delta = cur_time - phnat_ipi->time_rec;
				if (delta > 1) {
					smp_call_function_single(i, smp_func_call_from_extif, NULL, 0);
					phnat_ipi->time_rec = jiffies;
				}
			}
#ifdef HNAT_IPI_RXQUEUE			
			spin_unlock(&phnat_ipi->ipilock);
#endif			
		}
		mod_timer(&ipi_monitor_timer_from_extif, jiffies + 1);

	}
}

void ipi_monitor_from_ppehit(unsigned long data)
{
	int i;
	unsigned long delta;
	unsigned long cur_time;

	if (hnat_ipi_config->enable_from_ppehit==1)
	{
		hnat_ipi_s* phnat_ipi;
		hnat_ipi_stat* phnat_ipi_status;		
		cur_time = jiffies;
		for (i=0; i<NR_CPUS; i++) {
			phnat_ipi = hnat_ipi_from_ppehit[i];
			phnat_ipi_status = hnat_ipi_status[i];	
#if defined (HNAT_IPI_DQ)
			if ( ((skb_queue_len(&phnat_ipi->skbInputQueue) > 0)|| \
				(skb_queue_len(&phnat_ipi->skbProcessQueue) > 0)) &&\
				(atomic_read(&phnat_ipi_status->cpu_status_from_ppehit) <= 0)) {
#elif defined (HNAT_IPI_RXQUEUE)
			spin_lock(&phnat_ipi->ipilock);
			if ((atomic_read(&phnat_ipi->RxQueueNum) > 0) && (atomic_read(&phnat_ipi_status->cpu_status_from_ppehit) <= 0)) {
#else
			if ((skb_queue_len(&phnat_ipi->skbIpiQueue) > 0) && (atomic_read(&phnat_ipi_status->cpu_status_from_ppehit) <= 0)) {
#endif
				delta = cur_time - phnat_ipi->time_rec;
				if (delta > 1) {
					smp_call_function_single(i, smp_func_call_from_ppehit, NULL, 0);
					phnat_ipi->time_rec = jiffies;
				}
			}
#ifdef HNAT_IPI_RXQUEUE		
			spin_unlock(&phnat_ipi->ipilock);
#endif
		}
	  mod_timer(&ipi_monitor_timer_from_ppehit, jiffies + 1);

	}
		
}	
int HnatIPIInit(void)
{
		int i;
	   //printk("========= %s(%d)[%s]: init HNAT IPI [%d CPUs](%d) =========\n\n", __func__, __LINE__,__TIME__,NR_CPUS,sizeof(hnat_ipi_s));
	   printk("========= %s: init HNAT IPI [%d CPUs](%lu) =========\n\n", __func__, NR_CPUS,sizeof(hnat_ipi_s));
	  //hnat_ipi_config = &hnat_ipi_config_ctx;
//    hnat_ipi_from_extif[0] = kzalloc(sizeof(hnat_ipi_s)*NR_CPUS, GFP_ATOMIC);
 //   hnat_ipi_from_ppehit[0] = kzalloc(sizeof(hnat_ipi_s)*NR_CPUS, GFP_ATOMIC);
 //   hnat_ipi_status[0] = kzalloc(sizeof(hnat_ipi_stat)*NR_CPUS, GFP_ATOMIC);
   
	hnat_ipi_from_extif[0] = kzalloc((sizeof(hnat_ipi_s)*2+sizeof(hnat_ipi_stat))*NR_CPUS \
				+ sizeof(hnat_ipi_config), GFP_ATOMIC);
hnat_ipi_from_ppehit[0] = ((hnat_ipi_s*)hnat_ipi_from_extif[0]) + sizeof(hnat_ipi_s)*NR_CPUS;
hnat_ipi_status[0] = ((hnat_ipi_stat*)hnat_ipi_from_ppehit[0]) + sizeof(hnat_ipi_s)*NR_CPUS;
hnat_ipi_config = ((hnat_ipi_cfg*)hnat_ipi_status[0]) + sizeof(hnat_ipi_stat)*NR_CPUS; 
    if ((hnat_ipi_from_extif[0]==NULL)||(hnat_ipi_from_ppehit[0]==NULL) || \
    		(hnat_ipi_status[0]==NULL) || (hnat_ipi_config==NULL))
    {
    		if (hnat_ipi_from_extif[0])
    			kfree(hnat_ipi_from_extif[0]);
//    		if (hnat_ipi_from_ppehit[0])
  //  			kfree(hnat_ipi_from_ppehit[0]);
  //  		if (hnat_ipi_status[0])
  //  			kfree(hnat_ipi_status[0]);
    		printk("Hnat IPI allocation failed\n");
    		return -1;
    }
    memset(hnat_ipi_config, 0 , sizeof(hnat_ipi_cfg));		
    for (i=0; i<NR_CPUS; i++){
    	hnat_ipi_from_extif[i] = hnat_ipi_from_extif[0]+1*i;
    	hnat_ipi_from_ppehit[i] = hnat_ipi_from_ppehit[0]+1*i;
    	hnat_ipi_status[i] = hnat_ipi_status[0]+1*i;
    	//printk("hnat_ipi_from_extif[%d]=0x%x\n",i,hnat_ipi_from_extif[i]);
    	//printk("hnat_ipi_from_ppehit[%d]=0x%x\n",i,hnat_ipi_from_ppehit[i]);
    	//printk("hnat_ipi_status[%d]=0x%x\n",i,hnat_ipi_status[i]);

#if (defined (HNAT_IPI_RXQUEUE) || defined (HNAT_IPI_DQ))
			spin_lock_init(&hnat_ipi_from_extif[i]->ipilock);
			spin_lock_init(&hnat_ipi_from_ppehit[i]->ipilock);
#endif
#if defined (HNAT_IPI_RXQUEUE)
			hnat_ipi_from_extif[i]->RxQueue = (struct sk_buff**)kmalloc(sizeof(struct sk_buff)*1024, GFP_KERNEL);
			atomic_set(&hnat_ipi_from_extif[i]->RxQueueNum, 0);
			hnat_ipi_from_extif[i]->RxQueueWIdx = 0;
			hnat_ipi_from_extif[i]->RxQueueRIdx = 0;
			  
			hnat_ipi_from_ppehit[i]->RxQueue = (struct sk_buff**)kmalloc(sizeof(struct sk_buff)*1024, GFP_KERNEL);
			atomic_set(&hnat_ipi_from_ppehit[i]->RxQueueNum, 0);
			hnat_ipi_from_ppehit[i]->RxQueueWIdx = 0;
			hnat_ipi_from_ppehit[i]->RxQueueRIdx = 0;
	
#elif defined (HNAT_IPI_DQ)
			skb_queue_head_init(&hnat_ipi_from_extif[i]->skbInputQueue);
			skb_queue_head_init(&hnat_ipi_from_extif[i]->skbProcessQueue);
			
			skb_queue_head_init(&hnat_ipi_from_ppehit[i]->skbInputQueue);
			skb_queue_head_init(&hnat_ipi_from_ppehit[i]->skbProcessQueue);
#else	
			skb_queue_head_init(&hnat_ipi_from_extif[i]->skbIpiQueue);
			skb_queue_head_init(&hnat_ipi_from_ppehit[i]->skbIpiQueue);
#endif
			atomic_set(&hnat_ipi_status[i]->cpu_status_from_extif, 0);
			hnat_ipi_status[i]->dropPktNum_from_extif = 0;
			hnat_ipi_status[i]->smp_call_cnt_from_extif = 0;
			tasklet_init(&hnat_ipi_from_extif[i]->smp_func_call_tsk, smp_func_call_BH_handler_from_extif, 0);
			
			atomic_set(&hnat_ipi_status[i]->cpu_status_from_ppehit,0);
			hnat_ipi_status[i]->dropPktNum_from_ppehit = 0;
			hnat_ipi_status[i]->smp_call_cnt_from_ppehit = 0;
			tasklet_init(&hnat_ipi_from_ppehit[i]->smp_func_call_tsk, smp_func_call_BH_handler_from_ppehit, 0);
    }

    memset(ipidbg, 0, sizeof(ipidbg));
    memset(ipidbg2, 0, sizeof(ipidbg2));
 
    ipi_monitor_timer_from_extif.function = NULL;
		ipi_monitor_timer_from_ppehit.function = NULL;
		
    printk("========= %s(%d): init HNAT IPI =========\n\n", __func__, __LINE__);

		return 0;
}

int HnatIPIDeInit(void)
{
		int i,j;
		struct sk_buff* skb_deq = NULL;
		unsigned int current_ipi_enable_from_extif = hnat_ipi_config->enable_from_extif;
		unsigned int current_ipi_enable_from_ppehit = hnat_ipi_config->enable_from_ppehit;

		hnat_ipi_config->enable_from_extif = 0;
		hnat_ipi_config->enable_from_ppehit = 0;
		if (ipi_monitor_timer_from_extif.function)
			del_timer_sync(&ipi_monitor_timer_from_extif);
		if (ipi_monitor_timer_from_ppehit.function)
			del_timer_sync(&ipi_monitor_timer_from_ppehit);

    for(i=0; i<NR_CPUS; i++) {
    	//int qlen;
    	hnat_ipi_s* phnat_ipi_from_extif = hnat_ipi_from_extif[i];
    	hnat_ipi_s* phnat_ipi_from_ppehit = hnat_ipi_from_ppehit[i];
    	hnat_ipi_stat* phnat_ipi_status = hnat_ipi_status[i];
    	if (current_ipi_enable_from_extif==1)	
    		while(atomic_read(&phnat_ipi_status->cpu_status_from_extif)>=1);
    	if (current_ipi_enable_from_ppehit)
    		while(atomic_read(&phnat_ipi_status->cpu_status_from_ppehit)>=1);

    	if (current_ipi_enable_from_extif==1)
    		tasklet_kill(&phnat_ipi_from_extif->smp_func_call_tsk);
    	if (current_ipi_enable_from_ppehit==1)
    		tasklet_kill(&phnat_ipi_from_ppehit->smp_func_call_tsk);
    	
#if defined (HNAT_IPI_DQ)
			for(j=0; j<phnat_ipi_from_extif->skbInputQueue.qlen; j++) {
	    	skb_deq = skb_dequeue(&phnat_ipi_from_extif->skbInputQueue);
    	    	if(skb_deq)
   	        	dev_kfree_skb_any(skb_deq); 
    	}
    
    	for(j=0; j<phnat_ipi_from_ppehit->skbInputQueue.qlen; j++) {
	    	skb_deq = skb_dequeue(&phnat_ipi_from_ppehit->skbInputQueue);
    	    	if(skb_deq)
   	        	dev_kfree_skb_any(skb_deq);
   	        else
   	        	break;	
   	  }      	
   	  for(j=0; j<phnat_ipi_from_extif->skbProcessQueue.qlen; j++) {
	    	skb_deq = skb_dequeue(&phnat_ipi_from_extif->skbProcessQueue);
    	    	if(skb_deq)
   	        	dev_kfree_skb_any(skb_deq); 
    	}
    	for(j=0; j<phnat_ipi_from_ppehit->skbProcessQueue.qlen; j++) {
	    	skb_deq = skb_dequeue(&phnat_ipi_from_ppehit->skbProcessQueue);
    	    	if(skb_deq)
   	        	dev_kfree_skb_any(skb_deq);
   	  }      	      	
#elif defined (HNAT_IPI_RXQUEUE)
			qlen = atomic_read(&phnat_ipi_from_extif->RxQueueNum);
			for(j=0; j<qlen; j++) {
	    	skb_deq = phnat_ipi_from_extif->RxQueue[phnat_ipi_from_extif->RxQueueRIdx];
    	    	if(skb_deq)
   	        	dev_kfree_skb_any(skb_deq);
   	    phnat_ipi_from_extif->RxQueue[phnat_ipi_from_extif->RxQueueRIdx] = NULL;
   	    phnat_ipi_from_extif->RxQueueRIdx = (phnat_ipi_from_extif->RxQueueRIdx+1)%1024;    	
   	  }
   	 	qlen = atomic_read(&phnat_ipi_from_ppehit->RxQueueNum);
      	
   	  for(j=0; j<qlen; j++) {
	    	skb_deq = phnat_ipi_from_ppehit->RxQueue[phnat_ipi_from_ppehit->RxQueueRIdx];
    	    	if(skb_deq)
   	        	dev_kfree_skb_any(skb_deq);
   	    phnat_ipi_from_ppehit->RxQueue[phnat_ipi_from_ppehit->RxQueueRIdx] = NULL;
   	    phnat_ipi_from_ppehit->RxQueueRIdx = (phnat_ipi_from_ppehit->RxQueueRIdx+1)%1024;       	
   	  }      	 
   	  kfree(phnat_ipi_from_extif->RxQueue);
   	  kfree(phnat_ipi_from_ppehit->RxQueue);      	     	
#else
			qlen = skb_queue_len(&phnat_ipi_from_extif->skbIpiQueue);
    	for(j=0; j<qlen; j++) {
	    	skb_deq = skb_dequeue(&phnat_ipi_from_extif->skbIpiQueue);
    	    	if(skb_deq)
   	        	dev_kfree_skb_any(skb_deq);
   	        else
   	        	break;	 
    	}
    	qlen = skb_queue_len(&phnat_ipi_from_ppehit->skbIpiQueue);  
    	for(j=0; j<qlen; j++) {
	    	skb_deq = skb_dequeue(&phnat_ipi_from_ppehit->skbIpiQueue);
    	    	if(skb_deq)
   	        	dev_kfree_skb_any(skb_deq);
   	        else
   	        	break;	
   	  }      	
#endif
		}   	        	
		
		{
		hnat_ipi_s* phnat_ipi = hnat_ipi_from_extif[0];

		//hnat_ipi_stat* phnat_ipi_status = hnat_ipi_status[0];

		if (phnat_ipi)
			kfree(phnat_ipi);
//		phnat_ipi = hnat_ipi_from_ppehit[0];
//		if (phnat_ipi)
//			kfree(phnat_ipi);
//		if (phnat_ipi_status)
//			kfree(phnat_ipi_status);
			
		ipi_monitor_timer_from_extif.function = NULL;
		ipi_monitor_timer_from_ppehit.function = NULL;	
		}
	
	return 0;
}
int HnatIPITimerSetup(void)
{
	if ((hnat_ipi_config->enable_from_extif==1) && \
			(ipi_monitor_timer_from_extif.function==NULL))
	{	
		
		init_timer(&ipi_monitor_timer_from_extif);
		ipi_monitor_timer_from_extif.function = ipi_monitor_from_extif;
		ipi_monitor_timer_from_extif.expires = jiffies + 1;
		add_timer(&ipi_monitor_timer_from_extif);
		return 0;		
	}
	if ((hnat_ipi_config->enable_from_ppehit==1) && \
			(ipi_monitor_timer_from_ppehit.function==NULL))
	{	
		init_timer(&ipi_monitor_timer_from_ppehit);
		ipi_monitor_timer_from_ppehit.function = ipi_monitor_from_ppehit;
		ipi_monitor_timer_from_ppehit.expires = jiffies + 1;
		add_timer(&ipi_monitor_timer_from_ppehit);
		return 0;
	}
	return 0;
}

