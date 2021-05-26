#include <linux/netdevice.h>

#define HNAT_PATH_ETHERNET      BIT(0)
#define HNAT_PATH_VLAN          BIT(1)
#define HNAT_PATH_PPPOE         BIT(2)
#define HNAT_PATH_DSLITE        BIT(3)

struct hnat_hw_path {
        const struct net_device *dev;
        u32 flags;

        u8 eth_src[ETH_ALEN];
        u8 eth_dest[ETH_ALEN];
        u16 vlan_proto;
        u16 vlan_id;
        u16 pppoe_sid;
};
