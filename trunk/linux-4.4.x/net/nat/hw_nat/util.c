/*
    Module Name:
    util.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Name        Date            Modification logs
    Steven Liu  2006-10-06      Initial version
*/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/in.h>

#include "util.h"

void MacReverse(uint8_t * Mac)
{
	uint8_t tmp;
	uint8_t i;

	for (i = 5; i > 2; i--) {
		tmp = Mac[i];
		Mac[i] = Mac[5 - i];
		Mac[5 - i] = tmp;
	}
}

int GetNext(char *src, int separator, char *dest)
{
	char *c;
	int len = 0;

	if ((src == NULL) || (dest == NULL)) {
		return -1;
	}

	c = strchr(src, separator);
	if (c == NULL) {
		strncpy(dest, src, len);
		return -1;
	}
	len = c - src;
	strncpy(dest, src, len);
	dest[len] = '\0';
	return len + 1;
}

static inline int atoi(char *s)
{
	int i = 0;
	while (isdigit(*s)) {
		i = i * 10 + *(s++) - '0';
	}
	return i;
}

/* Convert IP address from Hex to string */
uint8_t *ip_to_str(IN uint32_t Ip)
{
	static uint8_t Buf[32];
	uint8_t *ptr = (char *)&Ip;
	uint8_t c[4];

	c[0] = *(ptr);
	c[1] = *(ptr + 1);
	c[2] = *(ptr + 2);
	c[3] = *(ptr + 3);
	sprintf(Buf, "%d.%d.%d.%d", c[3], c[2], c[1], c[0]);
	return Buf;
}

unsigned int Str2Ip(IN char *str)
{
	int len;
	char *ptr = str;
	char buf[128];
	unsigned char c[4];
	int i;
	for (i = 0; i < 3; ++i) {
		if ((len = GetNext(ptr, '.', buf)) == -1) {
			return 1;	/* parsing error */
		}
		c[i] = atoi(buf);
		ptr += len;
	}
	c[3] = atoi(ptr);
	return ((c[0] << 24) + (c[1] << 16) + (c[2] << 8) + c[3]);
}

/* calculate ip address range */
/* start_ip <= x < end_ip */
void CalIpRange(uint32_t StartIp, uint32_t EndIp, uint8_t * M, uint8_t * E)
{
	uint32_t Range = (EndIp + 1) - StartIp;
	uint32_t i;

	for (i = 0; i < 32; i++) {
		if ((Range >> i) & 0x01) {
			break;
		}
	}

	if (i != 32) {
		*M = Range >> i;
		*E = i;
	} else {
		*M = 0;
		*E = 0;
	}

}

#if defined (CONFIG_ARCH_MT7622) || defined (CONFIG_MACH_MT7623)
void reg_modify_bits(unsigned int *Addr, uint32_t Data, uint32_t Offset, uint32_t Len)
{
	unsigned int Mask = 0;
	unsigned int Value;
	unsigned int i;

	for (i = 0; i < Len; i++) {
		Mask |= 1 << (Offset + i);
	}

	Value = sysRegRead(Addr);
	Value &= ~Mask;
	Value |= (Data << Offset) & Mask;;
	sysRegWrite(Addr, Value); 
}
#else
void reg_modify_bits(uint32_t Addr, uint32_t Data, uint32_t Offset, uint32_t Len)
{
	uint32_t Mask = 0;
	uint32_t Value;
	uint32_t i;

	for (i = 0; i < Len; i++) {
		Mask |= 1 << (Offset + i);
	}

	Value = reg_read(Addr);
	Value &= ~Mask;
	Value |= (Data << Offset) & Mask;;

	reg_write(Addr, Value);
}

#endif
static inline uint16_t CsumPart(uint32_t o, uint32_t n, uint16_t old)
{
	uint32_t d[] = { o, n };
	return csum_fold(csum_partial((char *)d, sizeof(d), old ^ 0xFFFF));
}

/*
 * KeepAlive with new header mode will pass the modified packet to cpu.
 * We must change to original packet to refresh NAT table.
 */

/*
 * Recover TCP Src/Dst Port and recalculate tcp checksum
 */
void
foe_to_org_tcphdr(IN struct foe_entry *entry, IN struct iphdr *iph,
	       OUT struct tcphdr *th)
{
	/* TODO: how to recovery 6rd/dslite packet */
	th->check =
	    CsumPart((th->source) ^ 0xffff,
		     htons(entry->ipv4_hnapt.sport), th->check);
	th->check =
	    CsumPart((th->dest) ^ 0xffff,
		     htons(entry->ipv4_hnapt.dport), th->check);
	th->check =
	    CsumPart(~(iph->saddr), htonl(entry->ipv4_hnapt.sip),
		     th->check);
	th->check =
	    CsumPart(~(iph->daddr), htonl(entry->ipv4_hnapt.dip),
		     th->check);
	th->source = htons(entry->ipv4_hnapt.sport);
	th->dest = htons(entry->ipv4_hnapt.dport);
}

/*
 * Recover UDP Src/Dst Port and recalculate udp checksum
 */
void
foe_to_org_udphdr(IN struct foe_entry *entry, IN struct iphdr *iph,
	       OUT struct udphdr *uh)
{
	/* TODO: how to recovery 6rd/dslite packet */

	uh->check =
	    CsumPart((uh->source) ^ 0xffff,
		     htons(entry->ipv4_hnapt.sport), uh->check);
	uh->check =
	    CsumPart((uh->dest) ^ 0xffff,
		     htons(entry->ipv4_hnapt.dport), uh->check);
	uh->check =
	    CsumPart(~(iph->saddr), htonl(entry->ipv4_hnapt.sip),
		     uh->check);
	uh->check =
	    CsumPart(~(iph->daddr), htonl(entry->ipv4_hnapt.dip),
		     uh->check);
	uh->source = htons(entry->ipv4_hnapt.sport);
	uh->dest = htons(entry->ipv4_hnapt.dport);
}

/*
 * Recover Src/Dst IP and recalculate ip checksum
 */
void foe_to_org_iphdr(IN struct foe_entry *entry, OUT struct iphdr *iph)
{
	/* TODO: how to recovery 6rd/dslite packet */
	iph->saddr = htonl(entry->ipv4_hnapt.sip);
	iph->daddr = htonl(entry->ipv4_hnapt.dip);
	iph->check = 0;
	iph->check = ip_fast_csum((unsigned char *)(iph), iph->ihl);
}

void hwnat_memcpy(void *dest, void *src, u32 n) {
#if defined(CONFIG_ARCH_MT7622) || defined(CONFIG_MACH_MT7622)
	ether_addr_copy(dest, src);
#else
	memcpy(dest, src, n);
#endif
}
