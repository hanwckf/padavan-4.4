#ifndef _NFT_ARP_H_
#define _NFT_ARP_H_

extern char *arp_opcodes[];
#define NUMOPCODES 9

/* define invflags which won't collide with IPT ones */
#define IPT_INV_SRCDEVADDR	0x0080
#define IPT_INV_TGTDEVADDR	0x0100
#define IPT_INV_ARPHLN		0x0200
#define IPT_INV_ARPOP		0x0400
#define IPT_INV_ARPHRD		0x0800

#endif
