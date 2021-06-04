#include <linux/init.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/string.h>
#include <linux/if_ether.h>
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <linux/ppp_defs.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/ip.h>
#include <net/checksum.h>
#include <net/protocol.h>
#include "fast_path.h"
#include "pptp_l2tp_fdb.h"
#include "ra_nat.h"
#include "util.h"

#if 0
#if defined(CONFIG_GE_RGMII_INTERNAL_P4_AN) || defined(CONFIG_GE_RGMII_INTERNAL_P0_AN)
char *ifname="eth1";
#else
char *ifname="eth0";
#endif
char *lanifname="eth0";
#else
#if defined(CONFIG_GE_RGMII_INTERNAL_P4_AN) || defined(CONFIG_GE_RGMII_INTERNAL_P0_AN)
char *ifname="eth3";
#else
char *ifname="eth2";
#endif
char *lanifname="eth2";
#endif

struct net_device *WanInf;
struct net_device *LanInf;

static struct hnat_l2tp l2tp_log;
static struct hnat_l2tp l2tp_tunnel;/*for wan->lan*/
static struct hnat_pptp pptp_log;
static struct hnat_pptp pptp_tunnel;
extern uint32_t sync_tx_sequence;
extern struct pkt_parse_result   ppe_parse_result;
extern int32_t    fast_bind;
extern uint32_t	  debug_level;
extern uint16_t   wan_vid;

uint8_t GetPppLength(uint8_t *data, uint8_t *protocol)
{
    uint8_t ppp_length = 0;

    /* parse PPP length and get protocol field*/
    if (*data == 0x0  && *(data+1)== PPP_IP){
	ppp_length = 2;
	*protocol = PPP_IP;
    }
    else if (*data == PPP_ALLSTATIONS && *(data+1) == PPP_UI){									
	ppp_length = 2;
	data += 2;
	if (*data == 0x0 && *(data+1) == PPP_IP){
	    ppp_length += 2;
	    *protocol = PPP_IP;
	}
    }											
    
    return ppp_length ;
}


int32_t AddPptpHeader(struct sk_buff *skb, uint8_t data_offset)
{
    struct iphdr	   *iph_new = NULL;
    struct pptp_gre_header *greh_new = NULL;	
    struct ppp_hdr	   *ppph = NULL;
    int32_t		   add_len = 0;

    /* add L3 */
    iph_new = (struct iphdr *)(skb->data + data_offset);    
    memset(iph_new, 0, sizeof(struct iphdr));
    iph_new->version	=	IPVERSION;
    iph_new->ihl	=	5;
    iph_new->ttl 	=	IPDEFTTL;   
    iph_new->protocol	=	IPPROTO_GRE;
    iph_new->saddr	=	pptp_log.saddr;
    iph_new->daddr	=	pptp_log.daddr;
    /* TODO: To enable checksum offload in the furure */		
    iph_new->tot_len	=	htons(skb->len - ETH_HLEN - VLAN_LEN);		
    skb->ip_summed	=	CHECKSUM_NONE;			
    iph_new->id		=	htons(++pptp_log.tx_ip_id);
    iph_new->check	=	ip_fast_csum((uint8_t *)iph_new, 5);	
   
    /* add gre header */
    add_len = data_offset + sizeof(struct iphdr) + sizeof(struct pptp_gre_header) -4;
    greh_new = (struct pptp_gre_header *)(skb->data + data_offset + sizeof(struct iphdr));
    greh_new->flags		= PPTP_GRE_FLAG_K | PPTP_GRE_FLAG_S;
    greh_new->ver		= PPTP_GRE_VER;
    greh_new->protocol		= htons(PPTP_GRE_PROTO); /*ppp type = 0x880b*/
    greh_new->payload		= htons(skb->len - add_len);    	
    greh_new->cid		= pptp_log.call_id;			
    greh_new->seq 		= htonl(++sync_tx_sequence);		
    /* do not ack, -4 bytes*/

    /* add PPP */
    ppph = (struct vlan_hdr *)(skb->data + data_offset + sizeof(struct iphdr) + sizeof(struct pptp_gre_header) - 4);
    ppph->addr_ctrl = htons(PPP_ADDRESS_CONTROL);
    ppph->protocol = htons(PPP_IP);

    return 0;
}

int32_t AddL2tpHeader(struct sk_buff *skb, uint8_t data_offset)
{
    struct iphdr	*iph_new = NULL;
    struct l2tp_add_hdr	*l2tph_new = NULL;
    struct ppp_hdr	*ppph = NULL;

    /* add L3 */
    iph_new = (struct iphdr *)(skb->data + data_offset);
    memset(iph_new, 0, sizeof(struct iphdr));
    iph_new->version	=	IPVERSION;
    iph_new->ihl	=	5;
    iph_new->ttl 	=	IPDEFTTL;
    iph_new->protocol	=	IPPROTO_UDP;
    iph_new->saddr	=	l2tp_log.saddr;
    iph_new->daddr	=	l2tp_log.daddr;
    iph_new->tot_len	=	htons(skb->len - data_offset);
    skb->ip_summed	=	CHECKSUM_NONE;
    iph_new->check	=	ip_fast_csum((uint8_t *)iph_new, 5);	

    /* add UDP + L2TP */
    l2tph_new = (struct l2tp_add_hdr *)(skb->data + data_offset + sizeof(struct iphdr));
    l2tph_new->source		=	l2tp_log.source;
    l2tph_new->dest		=	l2tp_log.dest;
    l2tph_new->checksum		=	0;
    l2tph_new->len		=	htons(skb->len - data_offset - sizeof(struct iphdr));/* UDP Length */
    l2tph_new->type		=	0x0200; /* V2 */
    l2tph_new->tid		=	l2tp_log.tid;
    l2tph_new->sid		=	l2tp_log.sid;

/* add PPP */
    ppph = (struct vlan_hdr *)(skb->data + data_offset + sizeof(struct iphdr) + sizeof(struct l2tp_add_hdr));
    ppph->addr_ctrl = htons(PPP_ADDRESS_CONTROL);
    ppph->protocol  = htons(PPP_IP);

    return 0;
}



int32_t hnat_pptp_lan(struct sk_buff *skb)
{		
    struct	iphdr *iph_ppp0 = NULL;
    struct	iphdr *iph	= NULL;
#if defined (CONFIG_HNAT_V2) && defined (CONFIG_RALINK_MT7620)	    
    struct	ethhdr *eth	= NULL;
#endif
    struct	pptp_gre_header *greh;					
    struct	tcphdr *th	= NULL;
    struct	udphdr *uh	= NULL;	
    uint32_t	addr		= 0;
    uint32_t    offset		= 8;	// gre 8 bytes, wothout seq and ack
    uint32_t    ppp_length	= 0;
    uint8_t	*ppp_format;	
    uint8_t	ppp_protocol	= 0;
    int32_t	rev = 0;

    iph = (struct iphdr *)(skb->data + VLAN_LEN);
    if(iph->protocol != IPPROTO_GRE || skb->len < 60){
	return 1;
    }
    greh = (struct pptp_gre_header *)(skb->data + (iph->ihl*4) + VLAN_LEN);

    /* handle PPTP packet */
    if ((greh->ver&7) == PPTP_GRE_VER && ntohs(greh->protocol) == PPTP_GRE_PROTO){
	/* check optional length */
	if (PPTP_GRE_IS_S(greh->flags)){	
	    pptp_log.rx_seqno = ntohl(greh->seq);
	    offset += 4;
	}	

	if (PPTP_GRE_IS_A(greh->ver))
	    offset += 4;

	ppp_format = ((char *)greh) + offset;				

	if (greh->payload > 0){	
	    ppp_length = GetPppLength(ppp_format, &ppp_protocol);
	}

	if (ppp_length ==  0){
	    return 1;
	}

	/* not IP packet Inside,return */
	if (ppp_protocol != PPP_IP)		
	    return 1;				

	/*calculate remove length*/
	offset = iph->ihl*4 + offset + ppp_length;	/* add IP offset */	

	iph_ppp0 = (struct iphdr *)(skb->data + offset + VLAN_LEN);

	if(!fast_bind){
	    pptp_tunnel.saddr = iph_ppp0->saddr;
	    pptp_tunnel.daddr = iph_ppp0->daddr;
	}

	if(iph_ppp0->protocol == IPPROTO_TCP){
	    th = (struct tcphdr *)(skb->data + offset + VLAN_LEN + 20);
	    addr = ((th->source << 16)|th->dest);
	   // skb->rxhash = addr; /*change for rps*/
	   skb->hash = addr; /*harry change for rps*/
	    //printk("TCP src port=%d, dst port=%d", ntohs(th->source), ntohs(th->dest));
	}
	else if (iph_ppp0->protocol == IPPROTO_UDP){
	    uh = (struct udphdr *)(skb->data + offset + VLAN_LEN + 20);
	    addr = ((uh->source << 16)|uh->dest);
            //skb->rxhash = addr; /*change for rps*/
            skb->hash = addr; /*harry change for rps*/
	    //printk("UDP src port=%d, dst port=%d", ntohs(uh->source), ntohs(uh->dest));
	}
	else{
	    //printk("HnatPptpLan, Non TCP/UDP to lan, return\n");
	    return 1;

	}

	/* header removal section */

	rev = is_pptp_l2tp_bind(iph_ppp0->protocol, addr, iph_ppp0->saddr);
	if(rev){
	    /*memory remove from head*/    
	    memcpy(skb->data + VLAN_LEN, skb->data - offset + VLAN_LEN, offset);
	    //printk("afater memmove  GRE + PPTP header\n");
	    /* redirect to PPE */
	    FOE_AI(skb) = UN_HIT;
	    FOE_MAGIC_TAG(skb) = FOE_MAGIC_PPE;
	    skb_pull(skb, offset);
	    skb_push(skb, 14);
#if defined (CONFIG_HNAT_V2) && defined (CONFIG_RALINK_MT7620)	    
	    /*make mac table transparent*/
	    eth = (struct ethhdr *)skb->data;
	    eth->h_source[0] = 0x01;
#endif
	    skb->dev = LanInf;
	    dev_queue_xmit(skb);
	    return 0;
	}
	else{
	    /* not in bind state*/
	    FOE_MAGIC_TAG(skb) = FOE_MAGIC_FASTPATH;
	    if(iph_ppp0->protocol == IPPROTO_TCP){
		th = (struct tcphdr *)(skb->data + offset + VLAN_LEN + 20);
		FOE_SOURCE(skb) = ntohs(th->source);
		FOE_DEST(skb) = ntohs(th->dest);
	    }
	    else if (iph_ppp0->protocol == IPPROTO_UDP){
		uh = (struct udphdr *)(skb->data + offset + VLAN_LEN + 20);
		FOE_SOURCE(skb) = ntohs(uh->source);
		FOE_DEST(skb) = ntohs(uh->dest);
	    }
	    else{
		FOE_MAGIC_TAG(skb) = 0;
		return 1;
	    }
	    return 1;
	}
	//LAYER3_HEADER(skb) = skb->data;
	//LAYER4_HEADER(skb) = skb->data;
	//harry
	skb_set_network_header(skb, 0);
	skb_set_transport_header(skb, 0);
    }
    return 1;		
}

/* add PPTP header*/
int32_t hnat_pptp_wan(struct sk_buff *skb)
{
    struct   iphdr *iph = NULL;
    struct   vlan_ethhdr *veth = NULL;
    uint32_t add_len = 0;

    iph = (struct iphdr *)(skb->data + VLAN_LEN);	
    /* set tcp udp with different call ID */
    if(iph->protocol == IPPROTO_TCP){
	pptp_log.call_id = pptp_log.call_id_tcp;
    } else{
	pptp_log.call_id = pptp_log.call_id_udp;
    }

    /*  mac+ip+gre+ppp-4(ack) */
    add_len = ETH_HLEN + VLAN_LEN + sizeof(struct iphdr) + sizeof(struct pptp_gre_header) -4; 	
    if (skb_headroom(skb) < add_len || skb_shared(skb) ||
	    (skb_cloned(skb) && !skb_clone_writable(skb,0))){
		struct sk_buff *new_skb = skb_realloc_headroom(skb, add_len);
		if (!new_skb){
			printk("realloc headroom failed!\n");	
			return 1;
		}
		kfree_skb(skb);
		skb = new_skb;
    }
    /* add L2	*/					
    memcpy(skb_push(skb, add_len), pptp_log.eth_header, ETH_HLEN);

#if defined(CONFIG_GE_RGMII_INTERNAL_P4_AN) || defined(CONFIG_GE_RGMII_INTERNAL_P0_AN)
    veth = (struct vlan_ethhdr *)(skb->data);
    veth->h_vlan_proto = htons(ETH_P_IP);
#else
    /* add vid 2 */
    veth = (struct vlan_ethhdr *)(skb->data);
    veth->h_vlan_proto = htons(ETH_P_8021Q);
    veth->h_vlan_TCI = htons(wan_vid);
    veth->h_vlan_encapsulated_proto = htons(ETH_P_IP);
#endif

    AddPptpHeader(skb, ETH_HLEN + VLAN_LEN);
    skb->dev=WanInf;
    FOE_AI(skb) = UN_HIT;
    dev_queue_xmit(skb);
    return 0;	
}


/*L2TP*/
int32_t hnat_l2tp_lan(struct sk_buff *skb)
{		
    struct iphdr *iph_ppp0 = NULL;
#if defined (CONFIG_HNAT_V2) && defined (CONFIG_RALINK_MT7620)	    
    struct ethhdr *eth = NULL;
#endif
    struct hnat_l2tp_parse  *l2tpremove;
    uint8_t ppp_protocol = 0;
    struct iphdr *iph = NULL;
    struct tcphdr *th = NULL;
    struct udphdr *uh = NULL;	
    uint32_t addr = 0;
    uint8_t *ppp_format;	
    int32_t offset = 6;	//l2tp header from 6 
    int32_t ppp_length = 0;
    int32_t rev = 0;

    iph = (struct iphdr *)(skb->data + VLAN_LEN);
    if (iph->protocol != IPPROTO_UDP || skb->len < 60){
	return 1;
    }

    uh = (struct udphdr *)(skb->data + (iph->ihl*4) + VLAN_LEN);
    l2tpremove = (struct hnat_l2tp_parse *)(skb->data + (iph->ihl*4) + VLAN_LEN + 8);

    if(ntohs(uh->source)!=1701) /*udp source port 1701*/
	return 1;
    
    if(l2tpremove->ver & 0x4000)	
	offset += 2;

    if(l2tpremove->ver & 0x0200)
	offset += 2;

    ppp_format = ((char *)l2tpremove) + offset;				

    /* parse PPP length and check inside IP protocol */
    ppp_length = GetPppLength(ppp_format, &ppp_protocol);
    if (ppp_length ==  0)
	return 1;

    offset = iph->ihl*4 + 8/*UDP*/ + offset + ppp_length;	// tunnel IP offset	
    iph_ppp0 = (struct iphdr *)(skb->data + offset + VLAN_LEN );/*inner IP, 8:UDP in offset*/

    if(!fast_bind){
	l2tp_tunnel.saddr = iph_ppp0->saddr;
	l2tp_tunnel.daddr = iph_ppp0->daddr;
    }

     /* not IP packet in PPP */
    if(ppp_protocol != PPP_IP)						
	return 1;				

   /* get source&dest port to check if binded */
    if(iph_ppp0->protocol == IPPROTO_TCP){
	th = (struct tcphdr *)(skb->data + offset + VLAN_LEN + 20);
	addr = ((th->source << 16)|th->dest);
	//skb->rxhash = addr; /*change for rps*/
	skb->hash = addr; /*change for rps*/
	//printk("TCP src port=%d, dst port=%d", ntohs(th->source), ntohs(th->dest));
    }
    else if (iph_ppp0->protocol == IPPROTO_UDP){
	uh = (struct udphdr *)(skb->data + offset + VLAN_LEN + 20);
	addr = ((uh->source << 16)|uh->dest);
	//skb->rxhash = addr; /*change for rps*/
	skb->hash = addr; /*change for rps*/
    }
    else{
	//printk("HnatL2tpLan, Non TCP/UDP to lan\n");
	return 1;
    }

    /* header removal section */
    rev = is_pptp_l2tp_bind(iph_ppp0->protocol, addr, iph_ppp0->saddr);
    if(rev){
	/* memory remove from head */    
	memcpy(skb->data + VLAN_LEN, skb->data - offset + VLAN_LEN, offset);
	//printk("Bind flow,  remmove L2TP header send to PPE\n");
	/* redirect to PPE */
	FOE_AI(skb) = UN_HIT;
	FOE_MAGIC_TAG(skb) = FOE_MAGIC_PPE;
	skb_pull(skb, offset);
	skb_push(skb, 14);
#if defined (CONFIG_HNAT_V2) && defined (CONFIG_RALINK_MT7620)	    
	/* make mac table transparent */
	eth = (struct ethhdr *)skb->data;
	eth->h_source[0] = 0x01;
#endif
	skb->dev = LanInf;
	dev_queue_xmit(skb);
	return 0;
    }
    else{
	FOE_MAGIC_TAG(skb) = FOE_MAGIC_FASTPATH;

	if(iph_ppp0->protocol == IPPROTO_TCP){
	    th = (struct tcphdr *)(skb->data + offset + VLAN_LEN + 20);
	    //printk("l2tp_to_lan: TCP src port=%d, dst port=%d\n", ntohs(th->source), ntohs(th->dest));
	    FOE_SOURCE(skb) = ntohs(th->source);
	    FOE_DEST(skb) = ntohs(th->dest);
	}
	else if (iph_ppp0->protocol == IPPROTO_UDP){
	    uh = (struct udphdr *)(skb->data + offset + VLAN_LEN + 20);
	    //printk("UDP src port=%d, dst port=%d", ntohs(uh->source), ntohs(uh->dest));
	    FOE_SOURCE(skb) = ntohs(uh->source);
	    FOE_DEST(skb) = ntohs(uh->dest);
	}
	else{
	    printk("HnatL2tpLan, return line %d!!\n", __LINE__);
	    return 1;

	}
	return 1;
    }
    //LAYER3_HEADER(skb) = skb->data;
    //LAYER4_HEADER(skb) = skb->data;
	skb_set_network_header(skb, 0);
	skb_set_transport_header(skb, 0);
    return 1;		
}


int32_t hnat_l2tp_wan(struct sk_buff *skb)
{	
    uint32_t		add_len = 0;
    struct vlan_ethhdr	*veth = NULL;

    add_len = ETH_HLEN + sizeof(struct iphdr) + 8 + 6 + 4;/* UDP(8)+L2TP(6)+PPP(4) */
    if (skb_headroom(skb) < add_len || skb_shared(skb) ||
	    (skb_cloned(skb) && !skb_clone_writable(skb,0))){
	struct sk_buff *new_skb = skb_realloc_headroom(skb, add_len);
	if (!new_skb){
	    printk("realloc headroom failed!\n");	
			return 1;
		}
		kfree_skb(skb);
		skb = new_skb;
    }
    /* add L2 */
    memcpy(skb_push(skb, add_len), l2tp_log.eth_header, ETH_HLEN);
#if defined(CONFIG_GE_RGMII_INTERNAL_P4_AN) || defined(CONFIG_GE_RGMII_INTERNAL_P0_AN)
    veth = (struct vlan_ethhdr *)(skb->data);
    veth->h_vlan_proto = htons(ETH_P_IP);
#else
   /* add wan vid*/
    veth = (struct vlan_ethhdr *)(skb->data);
    veth->h_vlan_proto = htons(ETH_P_8021Q);
    veth->h_vlan_TCI = htons(wan_vid);
    veth->h_vlan_encapsulated_proto = htons(ETH_P_IP);
#endif
   
    AddL2tpHeader(skb, ETH_HLEN + VLAN_LEN);
    skb->dev =	WanInf;
    FOE_AI(skb) = UN_HIT;

    dev_queue_xmit(skb);	
    return 0;
}




int32_t  hnat_pptp_l2tpinit(void)
{
    WanInf = ra_dev_get_by_name(ifname);
    LanInf = ra_dev_get_by_name(lanifname);
    //printk("WanInf name is %s!\n", WanInf->name);
    return 0;
}

int32_t  hnat_pptp_l2tp_clean(void)
{
    if(WanInf != NULL){
	dev_put(WanInf);
    }
    if(LanInf != NULL){
	dev_put(LanInf);
    }
    return 0;
}



int32_t send_L2TP_hash_pkt(struct sk_buff *pskb)
{
    struct sk_buff *skb = NULL;
    struct iphdr *iph = NULL;
    struct iphdr *iph_new = NULL;
    struct tcphdr *th = NULL;
    struct udphdr *uh = NULL;

    uint8_t pkt[]={
	0x00, 0x30, 0xda, 0x01, 0x02, 0x0f, // dest macA
	0x00, 0x88, 0x99, 0x00, 0xaa, 0xbb, // src mac
	0x81, 0x00, // vlan tag
	0x01, 0x23, // pri=0, vlan=1
	0x08, 0x00, // eth type=ip
	0x45, 0x00, 0x00, 0x30, 0x12, 0x34, 0x40, 0x00, 0xff, 0x06, //TCP
	0x40, 0x74, 0x0a, 0x0a, 0x1e, 0x0a, 0x0a, 0x0a, 0x1e, 0x0b,
	0x00, 0x1e, 0x00, 0x28, 0x00, 0x1c, 0x81, 0x06, 0x00, 0x00,
	0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    skb = alloc_skb(256, GFP_ATOMIC);
    if( skb == NULL){
	return 1;
    }
    if(debug_level >= 1){
	printk("Ori source port = %d\n",FOE_SOURCE(pskb));
    }
    /* TODO:consider HW vlan in the future */
    /* slow path information */
    iph = (struct iphdr *)(pskb->data+14 + VLAN_LEN);

    if(1){
	skb->dev=LanInf;
	/* we use GMAC1 to send the packet to PPE */
	FOE_AI(skb) = UN_HIT;
	FOE_MAGIC_TAG(skb) = FOE_MAGIC_PPE;

	skb_reserve(skb, 32);
	skb_put(skb,sizeof(pkt));
	memcpy(skb->data, pkt, sizeof(pkt));

	iph_new = (struct iphdr *)(skb->data+14 + 4);

	/* from wan -> lan */
	//iph_new->saddr = l2tp_tunnel.saddr;
	iph_new->saddr = iph->saddr;
	iph_new->daddr = l2tp_tunnel.daddr;

	if(iph->protocol == IPPROTO_TCP){
	    skb_put(skb, (14+4+sizeof(struct iphdr)+sizeof(struct tcphdr)));
	    memcpy(skb->data+14+4+40, pskb->data, (14+VLAN_LEN+sizeof(struct iphdr)+sizeof(struct tcphdr)));

	    th = (struct tcphdr *)(skb->data +20 +14 + 4);
	    th->source = htons(FOE_SOURCE(pskb));
	    th->dest = htons(FOE_DEST(pskb));
	    if(debug_level >= 1){
		printk("send pingpong TCP  pkt:\n");
	    }
	}
	else if(iph->protocol == IPPROTO_UDP){
	    iph_new->protocol = IPPROTO_UDP;
	    skb_put(skb, (14+4+sizeof(struct iphdr)+sizeof(struct udphdr)));
	    memcpy(skb->data+14+4+ sizeof(struct iphdr) + sizeof(struct udphdr), pskb->data, (14+4+sizeof(struct iphdr)+sizeof(struct udphdr)));

	    uh = (struct udphdr *)(skb->data +20 +14 + 4);
	    uh->source = htons(FOE_SOURCE(pskb));
	    uh->dest = htons(FOE_DEST(pskb));

	    if (debug_level >= 1){
		printk("send pingpong UDP pkt\n");
	    }
	}
	
	if (debug_level >= 1){
	    printk("send L2TP Hash pkt(len=%d) dport=%d to %s\n", skb->len,FOE_DEST(pskb), skb->dev->name);
	}
	//dev_queue_xmit(skb);
	skb->dev->netdev_ops->ndo_start_xmit(skb, skb->dev);
    }else{
	printk("interface %s not found\n",ifname);
	return 1;
    }
    return 0;
}


int32_t send_hash_pkt(struct sk_buff *pskb)
{
    struct sk_buff *skb = NULL;
    struct iphdr *iph = NULL;
    struct iphdr *iph_new = NULL;
    struct tcphdr *th = NULL;
    struct udphdr *uh = NULL;

    uint8_t pkt[]={
	0x00, 0x30, 0xda, 0x01, 0x02, 0x0f,	// dest macA
	0x00, 0x88, 0x99, 0x00, 0xaa, 0xbb,	// src mac
	0x81, 0x00,				// vlan tag
	0x01, 0x23,				// pri=0, vlan=0x123
	0x08, 0x00,				// eth type=ip
	0x45, 0x00, 0x00, 0x30, 0x12, 0x34, 0x40, 0x00, 0xff, 0x06, //TCP
	0x40, 0x74, 0x0a, 0x0a, 0x1e, 0x0a, 0x0a, 0x0a, 0x1e, 0x0b,
	0x00, 0x1e, 0x00, 0x28, 0x00, 0x1c, 0x81, 0x06, 0x00, 0x00,
	0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    skb = alloc_skb(256, GFP_ATOMIC);
    if( skb == NULL){
	return 1;
    }
    if(debug_level >= 1){
	printk("Ori source port = %d\n",FOE_SOURCE(pskb));
    }
    //printk("Ori dest port = %d\n",FOE_DEST(pskb));
    /*no  HW vlan*/
    iph = (struct iphdr *)(pskb->data+14 + 4);

    if(1){
	skb->dev=LanInf;
	//use GMAC1 to send the packet to PPE
	//redirect to PPE
	FOE_AI(skb) = UN_HIT;
	FOE_MAGIC_TAG(skb) = FOE_MAGIC_PPE;

	skb_reserve(skb, 32);
	skb_put(skb,sizeof(pkt));
	memcpy(skb->data, pkt, sizeof(pkt));

	iph_new = (struct iphdr *)(skb->data+14 + 4);

	/* from wan -> lan */
	//iph_new->saddr = pptp_tunnel.saddr;
	iph_new->saddr = iph->saddr;
	iph_new->daddr = pptp_tunnel.daddr;

	if(iph->protocol == IPPROTO_TCP){
	    skb_put(skb, (14+4+sizeof(struct iphdr)+sizeof(struct tcphdr)));
	    memcpy(skb->data+14+4+40, pskb->data, (14+4+sizeof(struct iphdr)+sizeof(struct tcphdr)));

	    th = (struct tcphdr *)(skb->data +20 +14 + 4);
	    th->source = htons(FOE_SOURCE(pskb));
	    th->dest = htons(FOE_DEST(pskb));
	    //printk("original pkt is TCP \n");
	    //printk("original pkt:\n");

	    if (debug_level >= 1){
		printk("send pingpong TCP  pkt:\n");
	    }
	}
	else if(iph->protocol == IPPROTO_UDP){
	
	    iph_new->protocol = IPPROTO_UDP;
	    skb_put(skb, (14+4+sizeof(struct iphdr)+sizeof(struct udphdr)));
	    memcpy(skb->data+14+4+ sizeof(struct iphdr) + sizeof(struct udphdr), pskb->data, (14+4+sizeof(struct iphdr)+sizeof(struct udphdr)));

	    uh = (struct udphdr *)(skb->data +20 +14 + 4);
	    uh->source = htons(FOE_SOURCE(pskb));
	    uh->dest = htons(FOE_DEST(pskb));

	    if (debug_level >= 1){
		printk("send pingpong UDP pkt\n");
	    }
	}

	if (debug_level >= 1){
		printk("send Hash pkt(len=%d) dport=%d to %s\n", skb->len,FOE_DEST(pskb), skb->dev->name);
	}

	//dev_queue_xmit(skb);
	skb->dev->netdev_ops->ndo_start_xmit(skb, skb->dev);
    }else{
	printk("interface %s not found\n",ifname);
        kfree_skb(skb);
	return 1;
    }

    return 0;
}


int32_t pptp_tolan_parse_layer_info(struct sk_buff * skb)
{
	struct vlan_hdr *vh = NULL;
	struct ethhdr *eth = NULL;
	struct iphdr *iph = NULL;
	struct tcphdr *th = NULL;
	struct udphdr *uh = NULL;

	struct iphdr *iph_ori = NULL;
	int32_t offset = 0;
#ifdef CONFIG_RAETH_HW_VLAN_TX
	struct vlan_hdr pseudo_vhdr;
#endif
	memset(&ppe_parse_result, 0, sizeof(ppe_parse_result));

	vh = (struct vlan_hdr *)(skb->data);
	if(ntohs(vh->h_vlan_TCI) != 0x123){
	    printk("drop pingpong non vid=0x123 vh->h_vlan_TCI is 0x%4x\n", vh->h_vlan_TCI);
    	    return 1;
	}

	iph_ori = (struct iphdr *)(skb->data + 4);
	if(iph_ori->protocol == IPPROTO_TCP){
	    offset = VLAN_HLEN + (iph_ori->ihl * 4) + sizeof(struct tcphdr);
	}
	else if(iph_ori->protocol == IPPROTO_UDP){
	    offset = VLAN_HLEN + (iph_ori->ihl * 4) + sizeof(struct udphdr);
	}
	else{
	    printk("FastPathParseLayerInfo error type!!\n");
	    return 1;
	}


	eth = (struct ethhdr *)(skb->data + offset);
	memcpy(ppe_parse_result.dmac, eth->h_dest, ETH_ALEN);
	memcpy(ppe_parse_result.smac, eth->h_source, ETH_ALEN);
	ppe_parse_result.eth_type = eth->h_proto;

	// we cannot speed up multicase packets because both wire and wireless PCs might join same multicast group.
#if defined(CONFIG_RALINK_MT7620)
	if(is_multicast_ether_addr(&eth->h_dest[0])){
		ppe_parse_result.is_mcast = 1;
	}else{
		ppe_parse_result.is_mcast = 0;
	}
#else
	if(is_multicast_ether_addr(&eth->h_dest[0])){
		return 1;
	}
#endif
	if (is8021Q(ppe_parse_result.eth_type) || is_special_tag(ppe_parse_result.eth_type) || is_hw_vlan_tx(skb)) {
#ifdef CONFIG_RAETH_HW_VLAN_TX
		ppe_parse_result.vlan1_gap = 0;
		ppe_parse_result.vlan_layer++;
		//pseudo_vhdr.h_vlan_TCI = htons(vlan_tx_tag_get(skb));
		pseudo_vhdr.h_vlan_TCI = htons(hwnat_vlan_tag_get(skb));
		pseudo_vhdr.h_vlan_encapsulated_proto = eth->h_proto;
		vh = (struct vlan_hdr *)&pseudo_vhdr;
#else
		ppe_parse_result.vlan1_gap = VLAN_HLEN;
		ppe_parse_result.vlan_layer++;
		vh = (struct vlan_hdr *)(skb->data + offset + ETH_HLEN);
#endif
		ppe_parse_result.vlan1 = vh->h_vlan_TCI;
		ppe_parse_result.eth_type = vh->h_vlan_encapsulated_proto;
	}
	
	//printk("offset is %d!!!!!!!!!!!!!\n",offset);
	//printk("ETH_HLEN is %d!!!!!!!!!!!!!\n",ETH_HLEN);
	//printk("ppe_parse_result.vlan1_gap is %d!!!!!!!!!!!!!\n",ppe_parse_result.vlan1_gap);
	//printk("ppe_parse_result.vlan2_gap is %d!!!!!!!!!!!!!\n",ppe_parse_result.vlan2_gap);
	    
	/* set layer2 start addr */
	//LAYER2_HEADER(skb) = skb->data + offset;

	/* set layer3 start addr */
	//LAYER3_HEADER(skb) =
	    //(skb->data + offset +ETH_HLEN + ppe_parse_result.vlan1_gap +
	    // ppe_parse_result.vlan2_gap + ppe_parse_result.pppoe_gap);

	/* set layer2 start addr */
	skb_set_mac_header(skb, offset);
	/* set layer3 start addr */
	skb_set_network_header(skb, offset + ETH_HLEN + ppe_parse_result.vlan1_gap +
			       ppe_parse_result.vlan2_gap + ppe_parse_result.pppoe_gap);
	/* set layer4 start addr */
	if((ppe_parse_result.eth_type == htons(ETH_P_IP))){
		//iph = (struct iphdr *)LAYER3_HEADER(skb);
		iph = (struct iphdr *)skb_network_header(skb);
		//prepare layer3/layer4 info
		memcpy(&ppe_parse_result.iph, iph, sizeof(struct iphdr));
		if(iph->protocol == IPPROTO_TCP){
			//LAYER4_HEADER(skb) = ((uint8_t *) iph + (iph->ihl * 4));
			skb_set_transport_header(skb, offset + ETH_HLEN + ppe_parse_result.vlan1_gap +
						 ppe_parse_result.vlan2_gap +
						 ppe_parse_result.pppoe_gap + (iph->ihl * 4));
			//th = (struct tcphdr *)LAYER4_HEADER(skb);
			th = (struct tcphdr *)skb_transport_header(skb);
			memcpy(&ppe_parse_result.th, th, sizeof(struct tcphdr));
			ppe_parse_result.pkt_type = IPV4_HNAPT;

			if(iph->frag_off & htons(IP_MF|IP_OFFSET)){
				return 1;
			}
		}else if(iph->protocol == IPPROTO_UDP){
			//LAYER4_HEADER(skb) = ((uint8_t *) iph + iph->ihl * 4);
			skb_set_transport_header(skb, offset + ETH_HLEN + ppe_parse_result.vlan1_gap +
						 ppe_parse_result.vlan2_gap +
						 ppe_parse_result.pppoe_gap + (iph->ihl * 4));
			//uh = (struct udphdr *)LAYER4_HEADER(skb);
			uh = (struct udphdr *)skb_transport_header(skb);
			memcpy(&ppe_parse_result.uh, uh, sizeof(struct udphdr));
			ppe_parse_result.pkt_type = IPV4_HNAPT;
			
			if(iph->frag_off & htons(IP_MF|IP_OFFSET)){
				return 1;
			}
		}
#if defined (CONFIG_HNAT_V2)
		else if(iph->protocol == IPPROTO_GRE){
			/* do nothing */
		}
#endif
		else{
			/* Packet format is not supported */
			return 1;
		}

	} else {
		return 1;
	}

	if (debug_level >= 6) {
		printk("--------------\n");
		printk("DMAC:%02X:%02X:%02X:%02X:%02X:%02X\n",
		       ppe_parse_result.dmac[0], ppe_parse_result.dmac[1],
		       ppe_parse_result.dmac[2], ppe_parse_result.dmac[3],
		       ppe_parse_result.dmac[4], ppe_parse_result.dmac[5]);
		printk("SMAC:%02X:%02X:%02X:%02X:%02X:%02X\n",
		       ppe_parse_result.smac[0], ppe_parse_result.smac[1],
		       ppe_parse_result.smac[2], ppe_parse_result.smac[3],
		       ppe_parse_result.smac[4], ppe_parse_result.smac[5]);
		printk("Eth_Type=%x\n", ppe_parse_result.eth_type);
		if (ppe_parse_result.vlan1_gap > 0) {
			printk("VLAN1 ID=%x\n", ntohs(ppe_parse_result.vlan1));
		}

		if (ppe_parse_result.vlan2_gap > 0) {
			printk("VLAN2 ID=%x\n", ntohs(ppe_parse_result.vlan2));
		}

		if (ppe_parse_result.pppoe_gap > 0) {
			printk("PPPOE Session ID=%x\n",
			       ppe_parse_result.pppoe_sid);
			printk("PPP Tag=%x\n", ntohs(ppe_parse_result.ppp_tag));
		}
#if defined (CONFIG_HNAT_V2)
		printk("PKT_TYPE=%s\n",
		       ppe_parse_result.pkt_type ==
		       0 ? "IPV4_HNAT" : ppe_parse_result.pkt_type ==
		       1 ? "IPV4_HNAPT" : ppe_parse_result.pkt_type ==
		       3 ? "IPV4_DSLITE" : ppe_parse_result.pkt_type ==
		       4 ? "IPV6_ROUTE" : ppe_parse_result.pkt_type ==
		       5 ? "IPV6_6RD" : "Unknown");
#else
		printk("PKT_TYPE=%s\n",
		       ppe_parse_result.pkt_type ==
		       0 ? "IPV4_HNAPT" : ppe_parse_result.pkt_type ==
		       1 ? "IPV4_HNAT" : ppe_parse_result.pkt_type ==
		       2 ? "IPV6_ROUTE" : "Unknown");
#endif

		if (ppe_parse_result.pkt_type == IPV4_HNAT) {
			printk("SIP=%s\n",
			       ip_to_str(ntohl(ppe_parse_result.iph.saddr)));
			printk("DIP=%s\n",
			       ip_to_str(ntohl(ppe_parse_result.iph.daddr)));
			printk("TOS=%x\n", ntohs(ppe_parse_result.iph.tos));
		} else if (ppe_parse_result.pkt_type == IPV4_HNAPT) {
			printk("SIP=%s\n",
			       ip_to_str(ntohl(ppe_parse_result.iph.saddr)));
			printk("DIP=%s\n",
			       ip_to_str(ntohl(ppe_parse_result.iph.daddr)));
			printk("TOS=%x\n", ntohs(ppe_parse_result.iph.tos));
			
			if (ppe_parse_result.iph.protocol == IPPROTO_TCP) {
			    printk("TCP SPORT=%d\n", ntohs(ppe_parse_result.th.source));
			    printk("TCP DPORT=%d\n", ntohs(ppe_parse_result.th.dest));
			}else if(ppe_parse_result.iph.protocol == IPPROTO_UDP) {
			    printk("UDP SPORT=%d\n", ntohs(ppe_parse_result.uh.source));
			    printk("UDP DPORT=%d\n", ntohs(ppe_parse_result.uh.dest));
			}
		}
	}

	return 0;/* OK */
}



int32_t pptp_towan_parse_layerinfo(struct sk_buff * skb)
{
    struct vlan_hdr *vh = NULL;
    struct ethhdr *eth = NULL;
    struct iphdr *iph = NULL;
    struct iphdr *iph_ppp0 = NULL;
    struct tcphdr *th = NULL;
    struct udphdr *uh = NULL;

    struct		pptp_gre_header *greh = NULL;			
    uint8_t	*ppp_format = NULL;	
    int32_t		offset = sizeof(*greh) - 8; // delete seq and ack no
    int32_t		ppp_length = 0;
#ifdef CONFIG_RAETH_HW_VLAN_TX
    struct vlan_hdr pseudo_vhdr;
#endif
    memset(&ppe_parse_result, 0, sizeof(ppe_parse_result));

    eth = (struct ethhdr *)(skb->data);
    iph_ppp0 = (struct iphdr *)(skb->data + 14 + 4);

    memcpy(ppe_parse_result.smac, eth->h_dest, ETH_ALEN);
    memcpy(ppe_parse_result.dmac, eth->h_source, ETH_ALEN);
    ppe_parse_result.smac[0] = 0x01;

    greh = (struct pptp_gre_header *)(skb->data + 14 + (iph_ppp0->ihl*4) + 4);

    /*log pptp info*/	
    pptp_log.call_id = greh->cid;
    pptp_log.saddr = iph_ppp0->saddr;
    pptp_log.daddr = iph_ppp0->daddr;
    memcpy(pptp_log.eth_header, eth->h_dest, ETH_ALEN);
    memcpy(&pptp_log.eth_header[6], eth->h_source, ETH_ALEN);

    if (debug_level >= 1){
	printk("greh flags is 0x%1x\n", greh->flags);
	printk("greh version is 0x%1x\n", greh->ver);
    }
    ppe_parse_result.eth_type = eth->h_proto;

    if (PPTP_GRE_IS_S(greh->flags)){	
	if (debug_level >= 1) {
	    printk("greh->seq is %d\n", ntohl(greh->seq));    
	    printk("log pptp_log.tx_seqno!!!!!!!!!!\n");
	}
	pptp_log.tx_seqno = ntohl(greh->seq);						
	if (debug_level >= 1) {
	    printk("log pptp_log. IP ID!!!!!!!!!!\n");
	}
	pptp_log.tx_ip_id = ntohs(iph_ppp0->id);
	offset += 4;
    }	

    if (PPTP_GRE_IS_A(greh->ver)){
	if (debug_level >= 1){
	    printk("log pptp_log.rx_seqno ACK!!!!!!!!!!\n");
	}
	pptp_log.rx_seqno =  ntohl(greh->ack);
	offset += 4;
    }			
    ppp_format = ((char *)greh) + offset;				

    if (greh->payload > 0){	
	uint8_t    ppp_protocol	= 0;
	/* parse PPP length */
	ppp_length = GetPppLength(ppp_format, &ppp_protocol);
    }	

    if (ppp_length ==  0) 
	return 1;
    /* set IP offset*/
    offset = iph_ppp0->ihl*4 + offset + ppp_length;	
    //offset = iph->ihl*4 + offset + ppp_length + 4;	/* + vlan */	

    if (debug_level >= 1){
	printk("pptp offset is 0x%d\n", offset);
    }

    if (is8021Q(ppe_parse_result.eth_type) || is_special_tag(ppe_parse_result.eth_type) || is_hw_vlan_tx(skb)) 
    {
#ifdef CONFIG_RAETH_HW_VLAN_TX
	ppe_parse_result.vlan1_gap = 0;
	ppe_parse_result.vlan_layer++;
	//pseudo_vhdr.h_vlan_TCI = htons(vlan_tx_tag_get(skb));
	pseudo_vhdr.h_vlan_TCI = htons(hwnat_vlan_tag_get(skb));
	pseudo_vhdr.h_vlan_encapsulated_proto = eth->h_proto;
	vh = (struct vlan_hdr *)&pseudo_vhdr;
#else
	ppe_parse_result.vlan1_gap = VLAN_HLEN;
	ppe_parse_result.vlan_layer++;
	vh = (struct vlan_hdr *)(skb->data + ETH_HLEN);
#endif
	ppe_parse_result.vlan1 = vh->h_vlan_TCI;

	ppe_parse_result.eth_type = vh->h_vlan_encapsulated_proto;
    }

    /* set layer2 start addr, original L2 MAC */
   // LAYER2_HEADER(skb) = skb->data;

    /* set layer3 start addr, inner IP */
   // LAYER3_HEADER(skb) =
	//(skb->data + offset +ETH_HLEN + ppe_parse_result.vlan1_gap +
	// ppe_parse_result.vlan2_gap);
    /* set layer2 start addr, original L2 MAC */
   	skb_set_mac_header(skb, 0);
    /* set layer3 start addr, inner IP */
	skb_set_network_header(skb, offset +ETH_HLEN + ppe_parse_result.vlan1_gap +
				    ppe_parse_result.vlan2_gap);

    if (debug_level >= 1) {
	printk("LAN -> WAN set layer4 start addr\n");
    }
    /* set layer4 start addr */
    if ((ppe_parse_result.eth_type == htons(ETH_P_IP))){
	//iph = (struct iphdr *)LAYER3_HEADER(skb);
	iph = (struct iphdr *)skb_network_header(skb);
	if(iph->protocol == IPPROTO_TCP){
	    th = (struct tcphdr *)((uint8_t *) iph + 20);
	    pptp_log.call_id_tcp = greh->cid;
	    //printk("LAN -> WAN TCP src port=%d, dst port=%d \n", ntohs(th->source), ntohs(th->dest));
	}
	else if (iph->protocol == IPPROTO_UDP){
	    uh = (struct udphdr *)((uint8_t *)iph + 20);
	    pptp_log.call_id_udp = greh->cid;
	    //printk("UDP src port=%d, dst port=%d", ntohs(uh->source), ntohs(uh->dest));
	}
	//prepare layer3/layer4 info
	memcpy(&ppe_parse_result.iph, iph, sizeof(struct iphdr));
	if (iph->protocol == IPPROTO_TCP){
	   // LAYER4_HEADER(skb) = ((uint8_t *) iph + (iph->ihl * 4));
	    //th = (struct tcphdr *)LAYER4_HEADER(skb);
		skb_set_transport_header(skb, offset + ETH_HLEN + ppe_parse_result.vlan1_gap +
						 ppe_parse_result.vlan2_gap +
						 ppe_parse_result.pppoe_gap + (iph->ihl * 4));
		th = (struct tcphdr *)skb_transport_header(skb);	    
	    memcpy(&ppe_parse_result.th, th, sizeof(struct tcphdr));
	    ppe_parse_result.pkt_type = IPV4_HNAPT;
	    if (debug_level >= 1){
		printk("LAN -> WAN TCP src port=%d, dst port=%d \n", ntohs(th->source), ntohs(th->dest));
	    }
	    if(iph->frag_off & htons(IP_MF|IP_OFFSET)){
		printk("iph->frag_off  return\n");
		return 1;
	    }
	}else if (iph->protocol == IPPROTO_UDP){
	    //LAYER4_HEADER(skb) = ((uint8_t *) iph + iph->ihl * 4);
	   // uh = (struct udphdr *)LAYER4_HEADER(skb);
		skb_set_transport_header(skb, offset + ETH_HLEN + ppe_parse_result.vlan1_gap +
						 ppe_parse_result.vlan2_gap +
						 ppe_parse_result.pppoe_gap + (iph->ihl * 4));
		uh = (struct udphdr *)skb_transport_header(skb);
	    memcpy(&ppe_parse_result.uh, uh, sizeof(struct udphdr));
	    ppe_parse_result.pkt_type = IPV4_HNAPT;

	    if(iph->frag_off & htons(IP_MF|IP_OFFSET)){
		return 1;
	    }
	}
#if defined (CONFIG_HNAT_V2)
	else if (iph->protocol == IPPROTO_GRE){
	    /* do nothing */
	}
#endif
	else{
	    /* Packet format is not supported */
	    return 1;
	}

    } else{
	return 1;
    }

    if (debug_level >= 6){
	printk("--------------\n");
	printk("DMAC:%02X:%02X:%02X:%02X:%02X:%02X\n",
		ppe_parse_result.dmac[0], ppe_parse_result.dmac[1],
		ppe_parse_result.dmac[2], ppe_parse_result.dmac[3],
		ppe_parse_result.dmac[4], ppe_parse_result.dmac[5]);
	printk("SMAC:%02X:%02X:%02X:%02X:%02X:%02X\n",
		ppe_parse_result.smac[0], ppe_parse_result.smac[1],
		ppe_parse_result.smac[2], ppe_parse_result.smac[3],
		ppe_parse_result.smac[4], ppe_parse_result.smac[5]);
	printk("Eth_Type=%x\n", ppe_parse_result.eth_type);
	if (ppe_parse_result.vlan1_gap > 0) {
	    printk("VLAN1 ID=%x\n", ntohs(ppe_parse_result.vlan1));
	}

	if (ppe_parse_result.vlan2_gap > 0) {
	    printk("VLAN2 ID=%x\n", ntohs(ppe_parse_result.vlan2));
	}

	if (ppe_parse_result.pppoe_gap > 0) {
	    printk("PPPOE Session ID=%x\n",
		    ppe_parse_result.pppoe_sid);
	    printk("PPP Tag=%x\n", ntohs(ppe_parse_result.ppp_tag));
	}
#if defined (CONFIG_HNAT_V2)
	printk("PKT_TYPE=%s\n",
		ppe_parse_result.pkt_type ==
		0 ? "IPV4_HNAT" : ppe_parse_result.pkt_type ==
		1 ? "IPV4_HNAPT" : ppe_parse_result.pkt_type ==
		3 ? "IPV4_DSLITE" : ppe_parse_result.pkt_type ==
		4 ? "IPV6_ROUTE" : ppe_parse_result.pkt_type ==
		5 ? "IPV6_6RD" : "Unknown");
#else
	printk("PKT_TYPE=%s\n",
		ppe_parse_result.pkt_type ==
		0 ? "IPV4_HNAPT" : ppe_parse_result.pkt_type ==
		1 ? "IPV4_HNAT" : ppe_parse_result.pkt_type ==
		2 ? "IPV6_ROUTE" : "Unknown");
#endif

	if (ppe_parse_result.pkt_type == IPV4_HNAT) {
	    printk("SIP=%s\n",
		    ip_to_str(ntohl(ppe_parse_result.iph.saddr)));
	    printk("DIP=%s\n",
		    ip_to_str(ntohl(ppe_parse_result.iph.daddr)));
	    printk("TOS=%x\n", ntohs(ppe_parse_result.iph.tos));
	} else if (ppe_parse_result.pkt_type == IPV4_HNAPT) {
	    printk("SIP=%s\n",
		    ip_to_str(ntohl(ppe_parse_result.iph.saddr)));
	    printk("DIP=%s\n",
		    ip_to_str(ntohl(ppe_parse_result.iph.daddr)));
	    printk("TOS=%x\n", ntohs(ppe_parse_result.iph.tos));

	    if (ppe_parse_result.iph.protocol == IPPROTO_TCP) {
		printk("TCP SPORT=%d\n", ntohs(ppe_parse_result.th.source));
		printk("TCP DPORT=%d\n", ntohs(ppe_parse_result.th.dest));
	    }else if(ppe_parse_result.iph.protocol == IPPROTO_UDP) {
		printk("UDP SPORT=%d\n", ntohs(ppe_parse_result.uh.source));
		printk("UDP DPORT=%d\n", ntohs(ppe_parse_result.uh.dest));
	    }
	}
	}

	return 0;/*0 means OK here*/
}

int32_t l2tp_towan_parse_layerinfo(struct sk_buff * skb)
{

    struct vlan_hdr *vh = NULL;
    struct ethhdr *eth = NULL;
    struct iphdr *iph = NULL;
    struct iphdr *iph_ppp0 = NULL;
    struct tcphdr *th = NULL;
    struct udphdr *uh = NULL;

    struct hnat_l2tp_parse  *l2tph = NULL;
    uint16_t *tunnel_id = NULL;	
    uint16_t *session_id = NULL;
    uint8_t *ppp_format;	

    int32_t offset		   = 6;	//l2tp header from 6 
    int32_t ppp_length	   = 0;
    uint8_t ppp_protocol = 0;
#ifdef CONFIG_RAETH_HW_VLAN_TX
    struct vlan_hdr pseudo_vhdr;
#endif

    memset(&ppe_parse_result, 0, sizeof(ppe_parse_result));

    /*let dst to cpu mac*/
    eth = (struct ethhdr *)(skb->data);
    iph_ppp0 = (struct iphdr *)(skb->data + 14 + VLAN_LEN);
    memcpy(ppe_parse_result.smac, eth->h_dest, ETH_ALEN);
    memcpy(ppe_parse_result.dmac, eth->h_source, ETH_ALEN);
    ppe_parse_result.smac[0] = 0x01;

    uh = (struct udphdr *)(skb->data + 14 + (iph_ppp0->ihl*4) + VLAN_LEN);
    l2tph = (struct hnat_l2tp_parse *)(skb->data + 14 + (iph_ppp0->ihl*4) + VLAN_LEN + 8);

    if(ntohs(uh->dest)!=1701) /*l2f port:1701*/
	return 1;


    if(ntohs(l2tph->ver) & 0x4000)
	offset += 2;

    if(ntohs(l2tph->ver) & 0x0200)
	offset += 2;

    tunnel_id = (uint16_t *)(skb->data + 14 + (iph_ppp0->ihl*4) + VLAN_LEN + 8 + offset - 4);
    session_id = (uint16_t *)(skb->data + 14 + (iph_ppp0->ihl*4) + VLAN_LEN + 8 + offset - 2);

    if (debug_level >= 1){
	printk("tunnel_id is 0x%x\n", *tunnel_id);
	printk("h2tp header offset is 0x%d\n", offset);
	printk("VLAN_LEN is 0x%d\n", VLAN_LEN);
    }
    /* log l2tp info */	
    l2tp_log.tid = *tunnel_id;
    l2tp_log.sid = *session_id;
    l2tp_log.saddr = iph_ppp0->saddr;
    l2tp_log.daddr = iph_ppp0->daddr;
    l2tp_log.source = uh->source;
    l2tp_log.dest = uh->dest;
    memcpy(l2tp_log.eth_header, eth->h_dest, ETH_ALEN);
    memcpy(&l2tp_log.eth_header[6], eth->h_source, ETH_ALEN);

    if (debug_level >= 1){
	printk("l2tp_log.sid is 0x%4x\n", l2tp_log.sid);
	printk("l2tp_log.tid is 0x%4x\n", l2tp_log.tid);
    }
    ppe_parse_result.eth_type = eth->h_proto;
    ppp_format = ((char *)l2tph) + offset;				

    /* parse PPP length and check inside IP protocol */
    ppp_length = GetPppLength(ppp_format, &ppp_protocol);

    if (ppp_length ==  0) 
	return 1;

    offset = iph_ppp0->ihl*4+ 8 + offset + ppp_length;	// tunnel IP offset + udp	

    if (ppp_protocol != PPP_IP){						
	//printk("3.1 ppp_protocol != IP, return, line %d!!\n", __LINE__);
	return 1;				
    }

    if (debug_level >= 1) 
	printk("l2tp offset is 0x%d\n", offset);

    if (is8021Q(ppe_parse_result.eth_type) || is_special_tag(ppe_parse_result.eth_type) || is_hw_vlan_tx(skb)){

#ifdef CONFIG_RAETH_HW_VLAN_TX
	ppe_parse_result.vlan1_gap = 0;
	ppe_parse_result.vlan_layer++;
	//pseudo_vhdr.h_vlan_TCI = htons(vlan_tx_tag_get(skb));
	pseudo_vhdr.h_vlan_TCI = htons(hwnat_vlan_tag_get(skb));
	pseudo_vhdr.h_vlan_encapsulated_proto = eth->h_proto;
	vh = (struct vlan_hdr *)&pseudo_vhdr;
#else
	/* For MT7621 GE1 only case */
	ppe_parse_result.vlan1_gap = VLAN_LEN;
	ppe_parse_result.vlan_layer++;
	vh = (struct vlan_hdr *)(skb->data + ETH_HLEN);
#endif
	ppe_parse_result.vlan1 = vh->h_vlan_TCI;
	ppe_parse_result.eth_type = vh->h_vlan_encapsulated_proto;
    }

    /* set layer2 start addr, original L2 MAC */
   // LAYER2_HEADER(skb) = skb->data;
   	skb_set_mac_header(skb, 0);

    /* set layer3 start addr, inner IP */
    //LAYER3_HEADER(skb) =
	//(skb->data + offset +ETH_HLEN + ppe_parse_result.vlan1_gap +
	// ppe_parse_result.vlan2_gap);
	skb_set_network_header(skb, offset +ETH_HLEN + ppe_parse_result.vlan1_gap +
				    ppe_parse_result.vlan2_gap);

    if (debug_level >= 1) 
	printk("LAN -> WAN set layer4 start addr\n");

    /* set layer4 start addr */
    if ((ppe_parse_result.eth_type == htons(ETH_P_IP))){
	//iph = (struct iphdr *)LAYER3_HEADER(skb);
	iph = (struct iphdr *)skb_network_header(skb);
	/* prepare layer3/layer4 info */
	memcpy(&ppe_parse_result.iph, iph, sizeof(struct iphdr));
	if (iph->protocol == IPPROTO_TCP){
	    //LAYER4_HEADER(skb) = ((uint8_t *) iph + (iph->ihl * 4));
	    //th = (struct tcphdr *)LAYER4_HEADER(skb);
		skb_set_transport_header(skb, offset + ETH_HLEN + ppe_parse_result.vlan1_gap +
						 ppe_parse_result.vlan2_gap +
						 ppe_parse_result.pppoe_gap + (iph->ihl * 4));
		th = (struct tcphdr *)skb_transport_header(skb);
	    memcpy(&ppe_parse_result.th, th, sizeof(struct tcphdr));
	    ppe_parse_result.pkt_type = IPV4_HNAPT;
	    if (debug_level >= 1){
		printk("LAN -> WAN TCP src port=%d, dst port=%d \n", ntohs(th->source), ntohs(th->dest));
	    }
	    if(iph->frag_off & htons(IP_MF|IP_OFFSET)){
		printk("iph->frag_off  return\n");
		return 1;
	    }
	} else if (iph->protocol == IPPROTO_UDP){
	    //LAYER4_HEADER(skb) = ((uint8_t *) iph + iph->ihl * 4);
	    //uh = (struct udphdr *)LAYER4_HEADER(skb);
		skb_set_transport_header(skb, offset + ETH_HLEN + ppe_parse_result.vlan1_gap +
						 ppe_parse_result.vlan2_gap +
						 ppe_parse_result.pppoe_gap + (iph->ihl * 4));
		uh = (struct udphdr *)skb_transport_header(skb);
	    memcpy(&ppe_parse_result.uh, uh, sizeof(struct udphdr));
	    ppe_parse_result.pkt_type = IPV4_HNAPT;

	    if(iph->frag_off & htons(IP_MF|IP_OFFSET)){
		printk("iph->frag_off  return\n");
		return 1;
	    }
	}
#if defined (CONFIG_HNAT_V2)
	else if (iph->protocol == IPPROTO_GRE){
	    /* do nothing */
	}
#endif
	else{
	    /* Packet format is not supported */
	    return 1;
	}

    } 
    else{
	return 1;
    }

    if (debug_level >= 6) 
    {
	printk("--------------\n");
	printk("DMAC:%02X:%02X:%02X:%02X:%02X:%02X\n",
		ppe_parse_result.dmac[0], ppe_parse_result.dmac[1],
		ppe_parse_result.dmac[2], ppe_parse_result.dmac[3],
		ppe_parse_result.dmac[4], ppe_parse_result.dmac[5]);
	printk("SMAC:%02X:%02X:%02X:%02X:%02X:%02X\n",
		ppe_parse_result.smac[0], ppe_parse_result.smac[1],
		ppe_parse_result.smac[2], ppe_parse_result.smac[3],
		ppe_parse_result.smac[4], ppe_parse_result.smac[5]);
	printk("Eth_Type=%x\n", ppe_parse_result.eth_type);
	if (ppe_parse_result.vlan1_gap > 0) {
	    printk("VLAN1 ID=%x\n", ntohs(ppe_parse_result.vlan1));
	}

	if (ppe_parse_result.vlan2_gap > 0) {
	    printk("VLAN2 ID=%x\n", ntohs(ppe_parse_result.vlan2));
	}

	if (ppe_parse_result.pppoe_gap > 0) {
	    printk("PPPOE Session ID=%x\n",
		    ppe_parse_result.pppoe_sid);
	    printk("PPP Tag=%x\n", ntohs(ppe_parse_result.ppp_tag));
	}
#if defined (CONFIG_HNAT_V2)
	printk("PKT_TYPE=%s\n",
		ppe_parse_result.pkt_type ==
		0 ? "IPV4_HNAT" : ppe_parse_result.pkt_type ==
		1 ? "IPV4_HNAPT" : ppe_parse_result.pkt_type ==
		3 ? "IPV4_DSLITE" : ppe_parse_result.pkt_type ==
		4 ? "IPV6_ROUTE" : ppe_parse_result.pkt_type ==
		5 ? "IPV6_6RD" : "Unknown");
#else
	printk("PKT_TYPE=%s\n",
		ppe_parse_result.pkt_type ==
		0 ? "IPV4_HNAPT" : ppe_parse_result.pkt_type ==
		1 ? "IPV4_HNAT" : ppe_parse_result.pkt_type ==
		2 ? "IPV6_ROUTE" : "Unknown");
#endif

	if (ppe_parse_result.pkt_type == IPV4_HNAT) {
	    printk("SIP=%s\n",
		    ip_to_str(ntohl(ppe_parse_result.iph.saddr)));
	    printk("DIP=%s\n",
		    ip_to_str(ntohl(ppe_parse_result.iph.daddr)));
	    printk("TOS=%x\n", ntohs(ppe_parse_result.iph.tos));
	} else if (ppe_parse_result.pkt_type == IPV4_HNAPT) {
	    printk("SIP=%s\n",
			       ip_to_str(ntohl(ppe_parse_result.iph.saddr)));
			printk("DIP=%s\n",
			       ip_to_str(ntohl(ppe_parse_result.iph.daddr)));
			printk("TOS=%x\n", ntohs(ppe_parse_result.iph.tos));
			
			if (ppe_parse_result.iph.protocol == IPPROTO_TCP) {
			    printk("TCP SPORT=%d\n", ntohs(ppe_parse_result.th.source));
			    printk("TCP DPORT=%d\n", ntohs(ppe_parse_result.th.dest));
			}else if(ppe_parse_result.iph.protocol == IPPROTO_UDP) {
			    printk("UDP SPORT=%d\n", ntohs(ppe_parse_result.uh.source));
			    printk("UDP DPORT=%d\n", ntohs(ppe_parse_result.uh.dest));
			}
		}
	}
	return 0;/*0 means OK here*/
}


