#ifndef __PPTP_L2TP_FDB_H_
#define __PPTP_L2TP_FDB_H_

/* -------------------- Macro Definitions ------------------------------ */


#define PPTP_L2TP_HASH_BITS 10
#define PPTP_L2TP_HASH_SIZE (1 << PPTP_L2TP_HASH_BITS)

//typedef uint32_t            u32;

/* -------------------- Structure Definitions -------------------------- */




struct pptp_l2tp_fdb_entry
{
    struct hlist_node		hlist;
    struct rcu_head		rcu;
    uint32_t			hash_index;
    uint32_t			addr;/*src port + dst port*/
    uint32_t			src_ip;/*src IP*/
    uint8_t			protocol;
    uint8_t			entry_type;  /*0: Invalid, 1: from ethernet, 2: from WLan, 3: remote??*/
};



extern struct FoeEntry         *PpeFoeBase;


/* -------------------- Address Definitions ---------------------------- */

/* -------------------- Function Declaration ---------------------------- */
void pptp_l2tp_fdb_update(uint8_t protocol, uint32_t addr, uint32_t src_ip, uint32_t foe_hash_index);
int32_t is_pptp_l2tp_bind(uint8_t protocol, uint32_t addr, uint32_t src_ip);
int32_t pptp_l2tp_fdb_dump(void);    


#endif
