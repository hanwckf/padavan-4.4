/*
    Module Name:
    foe_fdb.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Name        Date            Modification logs
    Steven Liu  2006-10-06      Initial version
*/

#ifndef _FOE_DEFINE_WANTED
#define _FOE_DEFINE_WANTED

#include "frame_engine.h"
#include "raeth_config.h"

extern int (*ra_sw_nat_hook_rx)(struct sk_buff *skb);
extern int (*ra_sw_nat_hook_tx)(struct sk_buff *skb, int gmac_no);
#if defined(CONFIG_RA_HW_NAT_WIFI_NEW_ARCH)
extern void (*ppe_dev_register_hook)(struct net_device *dev);
extern void (*ppe_dev_unregister_hook)(struct net_device *dev);
#endif
extern u8 bind_dir;
extern u16 wan_vid;
extern u16 lan_vid;
extern struct foe_entry * ppe_virt_foe_base_tmp;
#if defined(CONFIG_RAETH_QDMA)
extern unsigned int M2Q_table[64];
extern unsigned int lan_wan_separate;
#endif
#if defined(CONFIG_HW_SFQ)
extern unsigned int web_sfq_enable;
#define HWSFQQUP 3
#define HWSfQQDL 1
#endif
#if defined(CONFIG_RA_HW_NAT_PPTP_L2TP)
extern u32 pptp_fast_path;
extern u32 l2tp_fast_path;
#endif

extern u32 debug_level;
extern struct net_device *dst_port[MAX_IF_NUM];

extern void hwnat_setup_dma_ops(struct device *dev, bool coherent);

#endif
