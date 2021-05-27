/*
    Module Name:
    util.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Name        Date            Modification logs
    Steven Liu  2012-10-19      Initial version
*/

#ifndef _MCAST_TBL_WANTED
#define _MCAST_TBL_WANTED

#if defined (CONFIG_ARCH_MT7622)
typedef struct {
    uint32_t	mc_vid:12;
    uint32_t	mc_qos_qid54:2;
    uint32_t	valid:1;
    uint32_t	rev1:1;
    uint32_t	mc_px_en:4;
    uint32_t	mc_mpre_sel:2; //0=01:00, 2=33:33
    uint32_t	mc_vid_cmp:1;
    uint32_t	rev2:1;
    uint32_t	mc_px_qos_en:4;
    uint32_t	mc_qos_qid:4;
} ppe_mcast_h;
#else
/*
 * TYPEDEFS AND STRUCTURES
 */
typedef struct {
    uint32_t	mc_vid:12;
    uint32_t	valid:1;
    uint32_t	rev1:3;
    uint32_t	mc_px_en:4;
    uint32_t	mc_mpre_sel:2; //0=01:00, 2=33:33
    uint32_t	mc_vid_cmp:1;
    uint32_t	rev2:1;
    uint32_t	mc_px_qos_en:4;
    uint32_t	mc_qos_qid:4;
} ppe_mcast_h;
#endif
typedef struct {
    uint8_t	mc_mac_addr[4]; //mc_mac_addr[31:0]
} ppe_mcast_l;

/*
 * DEFINITIONS AND MACROS
 */
#define MAX_MCAST_ENTRY	    16
#define MAX_MCAST_ENTRY16_63    48
#define MAX_MCAST_ENTRY_TOTOAL  64
//#define MCAST_DEBUG
#ifdef MCAST_DEBUG
#define MCAST_PRINT(fmt, args...) printk(KERN_INFO fmt, ## args)
#else
#define MCAST_PRINT(fmt, args...) { }
#endif

#define GET_PPE_MCAST_H(idx)		((ppe_mcast_h *)(PPE_MCAST_H_0 + ((idx) * 8)))
#define GET_PPE_MCAST_L(idx)		((ppe_mcast_l *)(PPE_MCAST_L_0 + ((idx) * 8)))

#define GET_PPE_MCAST_H10(idx)		((ppe_mcast_h *)(PPE_MCAST_H_10 + ((idx) * 8)))
#define GET_PPE_MCAST_L10(idx)		((ppe_mcast_l *)(PPE_MCAST_L_10 + ((idx) * 8)))

/*
 * EXPORT FUNCTION
 */
int foe_mcast_entry_ins(uint16_t vlan_id, uint8_t *dst_mac, uint8_t mc_px_en, uint8_t mc_px_qos_en, uint8_t mc_qos_qid);
int foe_mcast_entry_qid(uint16_t vlan_id, uint8_t *dst_mac, uint8_t mc_qos_qid);
int foe_mcast_entry_del(uint16_t vlan_id, uint8_t *dst_mac, uint8_t mc_px_en, uint8_t mc_px_qos_en, uint8_t mc_qos_qid);
void foe_mcast_entry_dump(void);
void foe_mcast_entry_del_all(void);

#endif
