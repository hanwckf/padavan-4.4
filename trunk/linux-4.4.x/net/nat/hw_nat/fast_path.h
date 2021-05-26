#ifndef _FASTPATH_WANTED
#define _FASTPATH_WANTED

#define PPTP_TCP_PORT           1723
#define PPP_ADDRESS_CONTROL     0xff03
/* gre header structure: -------------------------------------------- */

#define PPTP_GRE_PROTO  0x880B
#define PPTP_GRE_VER    0x1

#define PPTP_GRE_FLAG_C 0x80
#define PPTP_GRE_FLAG_R 0x40
#define PPTP_GRE_FLAG_K 0x20
#define PPTP_GRE_FLAG_S 0x10
#define PPTP_GRE_FLAG_A 0x80

#define PPTP_GRE_IS_C(f) ((f)&PPTP_GRE_FLAG_C)
#define PPTP_GRE_IS_R(f) ((f)&PPTP_GRE_FLAG_R)
#define PPTP_GRE_IS_K(f) ((f)&PPTP_GRE_FLAG_K)
#define PPTP_GRE_IS_S(f) ((f)&PPTP_GRE_FLAG_S)
#define PPTP_GRE_IS_A(f) ((f)&PPTP_GRE_FLAG_A)

/* 16 bytes GRE header */
struct pptp_gre_header {
    u8 flags;             /* bitfield */
    u8 ver;                       /* should be PPTP_GRE_VER (enhanced GRE) */
    u16 protocol;         /* should be PPTP_GRE_PROTO (ppp-encaps) */
    u16 payload;      /* size of ppp payload, not inc. gre header */
    u16 cid;          /* peer's call_id for this session */
    u32 seq;              /* sequence number.  Present if S==1 */
    u32 ack;              /* seq number of highest packet recieved by */
    /*  sender in this session */
} __packed;


struct hnat_pptp 
{
    uint32_t tx_seqno;
    uint32_t rx_seqno;
    uint32_t saddr;
    uint32_t daddr;
    uint16_t call_id;
    uint16_t call_id_udp;/*tcp udp with different ID*/
    uint16_t call_id_tcp;
    uint16_t tx_ip_id;
    uint8_t eth_header[ETH_HLEN];
    uint32_t key;/*MT7620:add key*/
};

/*L2TP*/
struct hnat_l2tp
{
    uint32_t daddr;                     /* DIP */
    uint32_t saddr;			    /* SIP */
    uint16_t tid;                     /* Tunnel ID */
    uint16_t sid;                     /* Session ID */
    uint16_t source;                  /* UDP source port */
    uint16_t dest;                    /* UDP dest port */
    uint8_t eth_header[ETH_HLEN];
};

struct l2tp_add_hdr
{
    uint16_t source; /* UDP */
    uint16_t dest;
    uint16_t len;
    uint16_t checksum;
    uint16_t type; /* L2TP */
    uint16_t tid;
    uint16_t sid;
};

struct hnat_l2tp_parse
{
    uint16_t ver;                   /* Packets Type */
    uint16_t length;                /* Length (Optional)*/
    uint16_t tid;                   /* Tunnel ID */
    uint16_t sid;                   /* Session ID */
};

struct ppp_hdr {
	uint16_t addr_ctrl;
	uint16_t protocol;
};




/* MT7621 2 GMAC*/
#ifdef CONFIG_GE_RGMII_INTERNAL_P4_AN
#define VLAN_LEN       0
#else
#define VLAN_LEN       4
#endif

int32_t	hnat_pptp_lan(struct sk_buff *skb);
int32_t hnat_pptp_wan(struct sk_buff *skb);
int32_t	hnat_l2tp_lan(struct sk_buff *skb);
int32_t	hnat_l2tp_wan(struct sk_buff *skb);
int32_t PktGenInitMod(void);
int32_t pptp_tolan_parse_layer_info(struct sk_buff * skb);
int32_t pptp_towan_parse_layerinfo(struct sk_buff * skb);
int32_t l2tp_towan_parse_layerinfo(struct sk_buff * skb);
void	pptp_l2tp_fdb_update(uint8_t protocol, uint32_t addr,uint32_t src_ip, uint32_t foe_hash_index);
int32_t send_hash_pkt(struct sk_buff *pskb);
int32_t send_L2TP_hash_pkt(struct sk_buff *pskb);
int32_t	hnat_pptp_l2tpinit(void);
int32_t	hnat_pptp_l2tp_clean(void);
#endif
