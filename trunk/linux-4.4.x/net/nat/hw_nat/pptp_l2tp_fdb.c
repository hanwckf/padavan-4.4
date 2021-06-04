#include <linux/netdevice.h>
#include <linux/jhash.h>
#include "pptp_l2tp_fdb.h"
#include "foe_fdb.h"


spinlock_t                      pptp_l2tp_hash_lock;
struct hlist_head               pptp_l2tp_hash[PPTP_L2TP_HASH_SIZE];
static u32 fdb_salt __read_mostly;
extern uint32_t         debug_level;



int32_t pptp_l2tp_addr_hash(uint32_t key)
{
    return jhash_1word(key, fdb_salt) & (PPTP_L2TP_HASH_SIZE - 1);
}

void fdb_delete(struct pptp_l2tp_fdb_entry *f)
{
    //printk("fdb_delete %p\n\r", (void*)f);
    hlist_del_rcu(&f->hlist);
    kfree(f);
}


/* Flush  dynamic entries in forwarding database.*/
void pptp_l2tp_fdb_flush(u32 addr_hash)
{
    int32_t i;
    struct foe_entry *entry = NULL;

    spin_lock_bh(&pptp_l2tp_hash_lock);
    i = addr_hash;
    {
	struct pptp_l2tp_fdb_entry *f;
	struct hlist_node *h, *n;
	hlist_for_each_entry_safe(f, n, &pptp_l2tp_hash[i], hlist) {

	    entry = &ppe_foe_base[f->hash_index];
	    if (entry->bfib1.state != BIND){
		fdb_delete(f);
		//FoeDumpEntry(f->hash_index);
	    }
	}
    }
    spin_unlock_bh(&pptp_l2tp_hash_lock);
}



struct pptp_l2tp_fdb_entry *pptp_l2tp_fdb_find(struct hlist_head *head,
	uint32_t addr, uint32_t src_ip, uint8_t protocol)
{
    struct hlist_node *h;
    struct pptp_l2tp_fdb_entry *fdb;


   	hlist_for_each_entry_rcu(fdb, head, hlist) {

		if (fdb->addr == addr){
		    if(fdb->src_ip == src_ip  && fdb->protocol == protocol)
			return fdb;
		}
	}
    return NULL;
}

struct pptp_l2tp_fdb_entry *pptp_l2tp_fdb_create(struct hlist_head *head,
	uint32_t foe_hash,
	uint32_t addr,
	uint32_t src_ip,
	uint8_t protocol)
{
    struct pptp_l2tp_fdb_entry *fdb = NULL;

    fdb = kzalloc(sizeof(struct pptp_l2tp_fdb_entry), GFP_ATOMIC);
    if (debug_level >= 1) {
	printk("get a fdb space at %p\n", (void*)fdb);
    }
    if (fdb) {
	//memcpy(fdb->addr, addr, ETH_ALEN);
	hlist_add_head_rcu(&fdb->hlist, head);
	fdb->addr = addr;
	fdb->src_ip = src_ip;
	fdb->hash_index = foe_hash;
	fdb->protocol = protocol;
    }
    return fdb;
}


/*
 *
 *
 *  update port address, protocol  
 *  protocol: UDP (0x11) or TCP (0x6)
 *
 *
 */
void pptp_l2tp_fdb_update(uint8_t protocol, uint32_t addr, uint32_t src_ip, uint32_t foe_hash_index)
{
    struct hlist_head *head = &pptp_l2tp_hash[pptp_l2tp_addr_hash(addr)];
    struct pptp_l2tp_fdb_entry *fdb;

    fdb = pptp_l2tp_fdb_find(head, addr, src_ip, protocol);
    if (likely(fdb)) {

	if (debug_level >= 1) {
	    printk("pptp l2tp entry already exist\n");  
	}
    } else {
	spin_lock(&pptp_l2tp_hash_lock);
	if (debug_level >= 1) 
	{
	    printk("create an fdb entry #%d protocol=0x%2x addr=0x%4x\n",foe_hash_index, protocol, addr);
	    printk("pptp_l2tp_addr_hash(addr) is #%d \n", pptp_l2tp_addr_hash(addr));
	}
	pptp_l2tp_fdb_create(head, foe_hash_index, addr, src_ip, protocol);
	spin_unlock(&pptp_l2tp_hash_lock);
	/* flush time out FDB */
	pptp_l2tp_fdb_flush(pptp_l2tp_addr_hash(addr));
    }
}

int32_t is_pptp_l2tp_bind(uint8_t protocol, uint32_t addr, uint32_t src_ip)
{
    struct hlist_head *head = &pptp_l2tp_hash[pptp_l2tp_addr_hash(addr)];
    struct pptp_l2tp_fdb_entry *fdb = NULL;
    struct foe_entry *entry = NULL;

    fdb = pptp_l2tp_fdb_find(head, addr, src_ip, protocol);
    if (fdb) {
	    entry = &ppe_foe_base[fdb->hash_index];
	    //FoeDumpEntry(fdb->hash_index);
	    if (entry->bfib1.state == BIND){
		//printk("pptp_l2tp_bind protocol=0x%2x addr=0x%4x\n", protocol, addr);
		return 1;
	    }
    } 
    return 0;
}



/*
 * @brief: 
 *
 * @param: 
 *         
 * @return: 
 */
int32_t pptp_l2tp_fdb_dump(void)
{
    int32_t i = 0;
    struct hlist_node *h;
    struct pptp_l2tp_fdb_entry *f;
    struct foe_entry *entry = NULL;

   //rcu_read_lock();
    printk("pptp_l2tp_fdb_dump ~~~~~~~\n");
    for (i = 0; i < PPTP_L2TP_HASH_SIZE; i++) 
    {
	hlist_for_each_entry_rcu(f, &pptp_l2tp_hash[i], hlist) 

	{
	    entry = &ppe_foe_base[f->hash_index];
	    if (entry->bfib1.state != BIND)    
	    {
		//printk("!B:hash=%d,fdb bind address 0x%x\n",i, f->addr);
		continue;
	    }
	    else{
		printk("hash=%d,fdb bind address 0x%x\n",i, f->addr);
	    }

	}
    }
    //rcu_read_unlock();
    return 0;
}



int32_t pptp_l2tp_addr_delete(uint8_t protocol, uint32_t addr, uint32_t src_ip)
{
    struct hlist_head *head = &pptp_l2tp_hash[pptp_l2tp_addr_hash((uint32_t)addr)];
    struct pptp_l2tp_fdb_entry *fdb;

    fdb = pptp_l2tp_fdb_find(head, addr, src_ip, protocol);
    if (likely(fdb)) 
    {
	fdb_delete(fdb);
	return 1;
    } else 
    {
	printk("pptp_l2tp_addr_delete fdb no find \n\r");
    }
    return 0;
}




