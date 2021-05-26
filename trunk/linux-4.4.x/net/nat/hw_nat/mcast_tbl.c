#include <linux/version.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/if_vlan.h>
#include "frame_engine.h"
#include "mcast_tbl.h"
#include "util.h"
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,4,0)
spinlock_t mtbl_lock;
#else
DECLARE_MUTEX(mtbl_lock);
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,4,0)
#define UP(x) spin_unlock(&x)
#define DOWN(x) spin_lock(&x)
#else
#define UP(x) up(&x)
#define DOWN(x) down(&x)
#endif




int32_t mcast_entry_get(uint16_t vlan_id, uint8_t *dst_mac) 
{
    int i=0;

    for(i=0;i<MAX_MCAST_ENTRY;i++) {
	if((GET_PPE_MCAST_H(i)->mc_vid == vlan_id ) && 
		GET_PPE_MCAST_L(i)->mc_mac_addr[3] == dst_mac[2] &&
		GET_PPE_MCAST_L(i)->mc_mac_addr[2] == dst_mac[3] &&
		GET_PPE_MCAST_L(i)->mc_mac_addr[1] == dst_mac[4] &&
		GET_PPE_MCAST_L(i)->mc_mac_addr[0] == dst_mac[5]) {
		if(GET_PPE_MCAST_H(i)->mc_mpre_sel==0) {
			if(dst_mac[0]==0x1 && dst_mac[1]==0x00) {
				return i;
			}
		}else if(GET_PPE_MCAST_H(i)->mc_mpre_sel==1) {
			if(dst_mac[0]==0x33 && dst_mac[1]==0x33) {
				return i;
			}
		}else 
			continue;
	}
    }
#if  defined (CONFIG_ARCH_MT7622)
    for(i=0;i<MAX_MCAST_ENTRY16_63;i++) {
	if((GET_PPE_MCAST_H10(i)->mc_vid == vlan_id ) && 
		GET_PPE_MCAST_L10(i)->mc_mac_addr[3] == dst_mac[2] &&
		GET_PPE_MCAST_L10(i)->mc_mac_addr[2] == dst_mac[3] &&
		GET_PPE_MCAST_L10(i)->mc_mac_addr[1] == dst_mac[4] &&
		GET_PPE_MCAST_L10(i)->mc_mac_addr[0] == dst_mac[5]) {
		if(GET_PPE_MCAST_H10(i)->mc_mpre_sel==0) {
			if(dst_mac[0]==0x1 && dst_mac[1]==0x00) {
				return (i + 16);
			}
		}else if(GET_PPE_MCAST_H10(i)->mc_mpre_sel==1) {
			if(dst_mac[0]==0x33 && dst_mac[1]==0x33) {
				return (i + 16);
			}
		}else 
			continue;
	}
    }
#endif
    return -1;
}

/*
  mc_px_en: enable multicast to port x
  mc_px_qos_en: enable QoS for multicast to port x
  
  - multicast port0 map to PDMA
  - multicast port1 map to GMAC1
  - multicast port2 map to GMAC2
  - multicast port3 map to QDMA
*/
int foe_mcast_entry_ins(uint16_t vlan_id, uint8_t *dst_mac, uint8_t mc_px_en, uint8_t mc_px_qos_en, uint8_t mc_qos_qid)
{
    int i=0;
    int entry_num;
    ppe_mcast_h *mcast_h;
    ppe_mcast_l *mcast_l;
#if !defined (CONFIG_ARCH_MT7622)
    DOWN(mtbl_lock);
#endif

    printk("%s: vid=%x mac=%x:%x:%x:%x:%x:%x mc_px_en=%x mc_px_qos_en=%x, mc_qos_qid=%d \n", __FUNCTION__, vlan_id, dst_mac[0],dst_mac[1],dst_mac[2],dst_mac[3],dst_mac[4],dst_mac[5], mc_px_en, mc_px_qos_en,mc_qos_qid );
    //update exist entry
    if((entry_num = mcast_entry_get(vlan_id, dst_mac)) >= 0) {
    	printk("update exist entry %d\n", entry_num);
    	if(entry_num <16) {
		mcast_h = GET_PPE_MCAST_H(entry_num);
		mcast_l = GET_PPE_MCAST_L(entry_num);
	
		if(dst_mac[0]==0x1 && dst_mac[1]==0x00)
			mcast_h->mc_mpre_sel = 0;
		else if(dst_mac[0]==0x33 && dst_mac[1]==0x33)
			mcast_h->mc_mpre_sel = 1;
		else  {
	#if !defined (CONFIG_ARCH_MT7622)
		    UP(mtbl_lock);
	#endif
		    return 0;
		}
	
		mcast_h->mc_px_en = mc_px_en;
		mcast_h->mc_px_qos_en = mc_px_qos_en;
		//mcast_h->mc_qos_qid = mc_qos_qid;
		if (mc_qos_qid < 16){                                           
			mcast_h->mc_qos_qid = mc_qos_qid;                      
		}else if (mc_qos_qid > 15){
#if defined (CONFIG_ARCH_MT7622)                                     
                        mcast_h->mc_qos_qid = mc_qos_qid & 0xf;               
	    	        mcast_h->mc_qos_qid54 = (mc_qos_qid & 0x30) >> 4;
#endif
	    	}
	}
#if defined (CONFIG_ARCH_MT7622)	
	else{
		mcast_h = GET_PPE_MCAST_H10(entry_num - 16);
		mcast_l = GET_PPE_MCAST_L10(entry_num - 16);
	
		if(dst_mac[0]==0x1 && dst_mac[1]==0x00)
			mcast_h->mc_mpre_sel = 0;
		else if(dst_mac[0]==0x33 && dst_mac[1]==0x33)
			mcast_h->mc_mpre_sel = 1;
		else  {
		    return 0;
		}
	
		mcast_h->mc_px_en = mc_px_en;
		mcast_h->mc_px_qos_en = mc_px_qos_en;
		//mcast_h->mc_qos_qid = mc_qos_qid;
		if (mc_qos_qid < 16){                                           
			mcast_h->mc_qos_qid = mc_qos_qid;                      
		}else if (mc_qos_qid > 15){                                     
                        mcast_h->mc_qos_qid = mc_qos_qid & 0xf;               
	    	        mcast_h->mc_qos_qid54 = (mc_qos_qid & 0x30) >> 4;
	    	}
	}
#endif
#if !defined (CONFIG_ARCH_MT7622)
	UP(mtbl_lock);
#endif
	return 1;
    } else { //create new entry
		
	    for(i=0;i<MAX_MCAST_ENTRY;i++) { // entry0 ~ entry15

		    mcast_h = GET_PPE_MCAST_H(i);
		    mcast_l = GET_PPE_MCAST_L(i);
		    if(mcast_h->valid == 0) {
	    
			    if(dst_mac[0]==0x1 && dst_mac[1]==0x00)
				    mcast_h->mc_mpre_sel = 0;
			    else if(dst_mac[0]==0x33 && dst_mac[1]==0x33)
				    mcast_h->mc_mpre_sel = 1;
			    else {
#if !defined (CONFIG_ARCH_MT7622)
				UP(mtbl_lock);
#endif
				return 0;
			    }

			    mcast_h->mc_vid = vlan_id;
			    mcast_h->mc_px_en = mc_px_en;
			    mcast_h->mc_px_qos_en = mc_px_qos_en;
			    mcast_l->mc_mac_addr[3] = dst_mac[2];
			    mcast_l->mc_mac_addr[2] = dst_mac[3];
			    mcast_l->mc_mac_addr[1] = dst_mac[4];
			    mcast_l->mc_mac_addr[0] = dst_mac[5];
			    mcast_h->valid = 1;
			   // mcast_h->mc_qos_qid = mc_qos_qid;
			   if (mc_qos_qid < 16){                                           
			   	mcast_h->mc_qos_qid = mc_qos_qid;                      
		           }else if (mc_qos_qid > 15){
#if defined (CONFIG_ARCH_MT7622)                                   
                           	mcast_h->mc_qos_qid = mc_qos_qid & 0xf;               
	    	                mcast_h->mc_qos_qid54 = (mc_qos_qid & 0x30) >> 4;
#endif
	    	           }
#if !defined (CONFIG_ARCH_MT7622)
			    UP(mtbl_lock);
#endif
			    return 1;
		    }
	    }

#if defined (CONFIG_ARCH_MT7622)
	    for(i=0;i<MAX_MCAST_ENTRY16_63;i++) { // entry16 ~ entry63

		    mcast_h = GET_PPE_MCAST_H10(i);
		    mcast_l = GET_PPE_MCAST_L10(i);


	    	      if(mcast_h->valid == 0){	
			    if(dst_mac[0]==0x1 && dst_mac[1]==0x00)
				    mcast_h->mc_mpre_sel = 0;
			    else if(dst_mac[0]==0x33 && dst_mac[1]==0x33)
				    mcast_h->mc_mpre_sel = 1;
			    else {
				return 0;
			    }

			    mcast_h->mc_vid = vlan_id;
			    mcast_h->mc_px_en = mc_px_en;
			    mcast_h->mc_px_qos_en = mc_px_qos_en;
			    mcast_l->mc_mac_addr[3] = dst_mac[2];
			    mcast_l->mc_mac_addr[2] = dst_mac[3];
			    mcast_l->mc_mac_addr[1] = dst_mac[4];
			    mcast_l->mc_mac_addr[0] = dst_mac[5];
			    mcast_h->valid = 1;
			    //mcast_h->mc_qos_qid = mc_qos_qid;
			   if (mc_qos_qid < 16){                                           
			   	mcast_h->mc_qos_qid = mc_qos_qid;                      
		           }else if (mc_qos_qid > 15){                                     
                           	mcast_h->mc_qos_qid = mc_qos_qid & 0xf;               
	    	                mcast_h->mc_qos_qid54 = (mc_qos_qid & 0x30) >> 4;
	    	           }
			    return 1;
		      }

	    }
#endif
  
    }

    MCAST_PRINT("HNAT: Multicast Table is FULL!!\n");
#if !defined (CONFIG_ARCH_MT7622)
   	UP(mtbl_lock);
#endif
    return 0;
}

int foe_mcast_entry_qid(uint16_t vlan_id, uint8_t *dst_mac, uint8_t mc_qos_qid)
{
    int entry_num;
    ppe_mcast_h *mcast_h;
#if !defined (CONFIG_ARCH_MT7622)
    DOWN(mtbl_lock);
#endif
    printk("%s: vid=%x mac=%x:%x:%x:%x:%x:%x mc_qos_qid=%d\n", __FUNCTION__, vlan_id, dst_mac[0],dst_mac[1],dst_mac[2],dst_mac[3],dst_mac[4],dst_mac[5], mc_qos_qid);
    //update exist entry
    if((entry_num = mcast_entry_get(vlan_id, dst_mac)) >= 0) {
#if defined (CONFIG_ARCH_MT7622)
    	if(entry_num <= 15)
		mcast_h = GET_PPE_MCAST_H(entry_num);
	else
		mcast_h = GET_PPE_MCAST_H10(entry_num - 16);

			if (mc_qos_qid < 16){
				//mcast_h->mc_qos_qid = mc_qos_qid;
			}else if (mc_qos_qid > 15){
				//mcast_h->mc_qos_qid = mc_qos_qid & 0xf;
				//mcast_h->mc_qos_qid54 = (mc_qos_qid & 0x30) >> 4;	
			}else {
				printk("Error qid = %d\n", mc_qos_qid);
				return 0;
			}
#else
			mcast_h = GET_PPE_MCAST_H(entry_num);
			mcast_h->mc_qos_qid = mc_qos_qid;			
#endif
			

#if !defined (CONFIG_ARCH_MT7622)
			UP(mtbl_lock);
#endif
	return 1;
    } 
#if !defined (CONFIG_ARCH_MT7622)
    	UP(mtbl_lock);
#endif
    return 0;
}

/*
 * Return:
 *	    0: entry found
 *	    1: entry not found
 */
int foe_mcast_entry_del(uint16_t vlan_id, uint8_t *dst_mac, uint8_t mc_px_en, uint8_t mc_px_qos_en, uint8_t mc_qos_qid)
{
    int entry_num;
    ppe_mcast_h *mcast_h;
    ppe_mcast_l *mcast_l;
#if !defined (CONFIG_ARCH_MT7622)
    DOWN(mtbl_lock);
#endif
    printk("%s: vid=%x mac=%x:%x:%x:%x:%x:%x mc_px_en=%x mc_px_qos_en=%x mc_qos_qid=%d\n", __FUNCTION__, vlan_id, dst_mac[0],dst_mac[1],dst_mac[2],dst_mac[3],dst_mac[4],dst_mac[5], mc_px_en, mc_px_qos_en, mc_qos_qid);
    if((entry_num = mcast_entry_get(vlan_id, dst_mac)) >= 0) {
    	printk("entry_num = %d\n", entry_num);
    	if (entry_num <= 15){
		mcast_h = GET_PPE_MCAST_H(entry_num);
		mcast_l = GET_PPE_MCAST_L(entry_num);
		mcast_h->mc_px_en &= ~mc_px_en;
		mcast_h->mc_px_qos_en &= ~mc_px_qos_en;
		if(mcast_h->mc_px_en == 0 && mcast_h->mc_px_qos_en == 0) {
				mcast_h->valid = 0;
			mcast_h->mc_vid = 0;
			mcast_h->mc_qos_qid = 0;
#if defined (CONFIG_ARCH_MT7622)
			mcast_h->mc_qos_qid54 = 0;
#endif
			memset(&mcast_l->mc_mac_addr, 0, 4);
		}
	}
#if defined (CONFIG_ARCH_MT7622)
	else if (entry_num > 15){
		mcast_h = GET_PPE_MCAST_H10(entry_num - 16);
		mcast_l = GET_PPE_MCAST_L10(entry_num - 16);
		mcast_h->mc_px_en &= ~mc_px_en;
		mcast_h->mc_px_qos_en &= ~mc_px_qos_en;
		if(mcast_h->mc_px_en == 0 && mcast_h->mc_px_qos_en == 0) {
			mcast_h->valid = 0;
			mcast_h->mc_vid = 0;
			mcast_h->mc_qos_qid = 0;
			mcast_h->mc_qos_qid54 = 0;
			memset(&mcast_l->mc_mac_addr, 0, 4);
		}
	}
#endif
#if !defined (CONFIG_ARCH_MT7622)
		UP(mtbl_lock);
#endif
	return 0;
    }else {
    	printk("foe_mcast_entry_del fail: entry_number = %d\n", entry_num);
#if !defined (CONFIG_ARCH_MT7622) 
			UP(mtbl_lock);
#endif
	return 1;
    }
}

void foe_mcast_entry_dump(void)
{
    int i;
    ppe_mcast_h *mcast_h;
    ppe_mcast_l *mcast_l;
#if !defined (CONFIG_ARCH_MT7622)
    DOWN(mtbl_lock);
#endif 
    printk("MAC | VID | PortMask | QosPortMask \n");
    for(i=0;i<MAX_MCAST_ENTRY;i++) {

	    mcast_h = GET_PPE_MCAST_H(i);
	    mcast_l = GET_PPE_MCAST_L(i);

	    printk("%x:%x:%x:%x  %d  %c%c%c%c %c%c%c%c (QID=%d, mc_mpre_sel=%d)\n", 
			    mcast_l->mc_mac_addr[3], 
			    mcast_l->mc_mac_addr[2], 
			    mcast_l->mc_mac_addr[1], 
			    mcast_l->mc_mac_addr[0], 
			    mcast_h->mc_vid, 
			    (mcast_h->mc_px_en & 0x08)?'1':'-',
			    (mcast_h->mc_px_en & 0x04)?'1':'-',
			    (mcast_h->mc_px_en & 0x02)?'1':'-',
			    (mcast_h->mc_px_en & 0x01)?'1':'-',
			    (mcast_h->mc_px_qos_en & 0x08)?'1':'-',
			    (mcast_h->mc_px_qos_en & 0x04)?'1':'-',
			    (mcast_h->mc_px_qos_en & 0x02)?'1':'-',
			    (mcast_h->mc_px_qos_en & 0x01)?'1':'-',
#if defined (CONFIG_ARCH_MT7622) 
			     mcast_h->mc_qos_qid + ((mcast_h->mc_qos_qid54) << 4),
#else
 			     mcast_h->mc_qos_qid,
#endif
			     mcast_h->mc_mpre_sel);
    }
#if defined (CONFIG_ARCH_MT7622)    
    for(i=0;i<MAX_MCAST_ENTRY16_63;i++) {

	    mcast_h = GET_PPE_MCAST_H10(i);
	    mcast_l = GET_PPE_MCAST_L10(i);

	    printk("%x:%x:%x:%x  %d  %c%c%c%c %c%c%c%c (QID=%d, mc_mpre_sel=%d)\n", 
			    mcast_l->mc_mac_addr[3], 
			    mcast_l->mc_mac_addr[2], 
			    mcast_l->mc_mac_addr[1], 
			    mcast_l->mc_mac_addr[0], 
			    mcast_h->mc_vid, 
			    (mcast_h->mc_px_en & 0x08)?'1':'-',
			    (mcast_h->mc_px_en & 0x04)?'1':'-',
			    (mcast_h->mc_px_en & 0x02)?'1':'-',
			    (mcast_h->mc_px_en & 0x01)?'1':'-',
			    (mcast_h->mc_px_qos_en & 0x08)?'1':'-',
			    (mcast_h->mc_px_qos_en & 0x04)?'1':'-',
			    (mcast_h->mc_px_qos_en & 0x02)?'1':'-',
			    (mcast_h->mc_px_qos_en & 0x01)?'1':'-',
			     mcast_h->mc_qos_qid + ((mcast_h->mc_qos_qid54) << 4),
			     mcast_h->mc_mpre_sel);
    }
#endif
#if !defined (CONFIG_ARCH_MT7622)
    UP(mtbl_lock); 
#endif
}	

void foe_mcast_entry_del_all(void)
{
    int i;
    ppe_mcast_h *mcast_h;
    ppe_mcast_l *mcast_l;
#if !defined (CONFIG_ARCH_MT7622)
    DOWN(mtbl_lock);
#endif
    for(i=0;i<MAX_MCAST_ENTRY;i++) {
	    	mcast_h = GET_PPE_MCAST_H(i);
	    	mcast_l = GET_PPE_MCAST_L(i);
		mcast_h->mc_px_en = 0;
	        mcast_h->mc_px_qos_en = 0;	          
		mcast_h->valid = 0;	
		mcast_h->mc_vid = 0;
		mcast_h->mc_qos_qid = 0;
		mcast_h->mc_mpre_sel = 0;
		memset(&mcast_l->mc_mac_addr, 0, 4);
    }
#if defined (CONFIG_ARCH_MT7622)
	for(i=0;i<MAX_MCAST_ENTRY16_63;i++) { // entry16 ~ entry63
		mcast_h = GET_PPE_MCAST_H10(i);
		mcast_l = GET_PPE_MCAST_L10(i);
		mcast_h->mc_px_en = 0;
	        mcast_h->mc_px_qos_en = 0;	          
		mcast_h->valid = 0;	
		mcast_h->mc_vid = 0;
		mcast_h->mc_qos_qid = 0;
		mcast_h->mc_mpre_sel = 0;
		memset(&mcast_l->mc_mac_addr, 0, 4);
	}
#endif      
#if !defined (CONFIG_ARCH_MT7622)
    UP(mtbl_lock);
#endif
}
