/*
    Module Name:
    util.h

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Name        Date            Modification logs
    Steven Liu  2007-01-25      Initial version
*/

#ifndef _UTIL_WANTED
#define _UTIL_WANTED

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include "foe_fdb.h"
#include "frame_engine.h"
/*
 * DEFINITIONS AND MACROS
 */
#if defined (CONFIG_MACH_MT7623)
//#include <mach/sync_write.h>
#define reg_read(phys)		 (*(volatile unsigned int*)(phys))
//#define RegWrite(phys, val)	 mt65xx_reg_sync_writel(val, phys)
#define reg_write(phys, val) (__raw_writel(val, (void __iomem *)phys))
#define sysRegWrite(phys, val)      ((*(volatile unsigned int *)(phys)) = (val))
#define sysRegRead(phys)            (*(volatile unsigned int *)((phys)))
#elif defined (CONFIG_ARCH_MT7622)
//#include <../../../drivers/misc/mediatek/mach/mt6735/include/mach/sync_write.h>
#define sysRegRead(phys)            (*(volatile unsigned int *)((phys)))
#define sysRegWrite(phys, val)      ((*(volatile unsigned int *)(phys)) = (val))
#define reg_read(phys)		 (*(volatile unsigned int *)((phys)))
#define reg_write(phys, val)	 ((*(volatile unsigned int *)(phys)) = (val))
#elif defined(CONFIG_ARCH_MT7623)
#include <mach/sync_write.h>
#define reg_read(phys)		 (*(volatile unsigned int*)(phys))
#define reg_write(phys, val)	 mt65xx_reg_sync_writel(val, phys)
#else
#define PHYS_TO_K1(physaddr) KSEG1ADDR(physaddr)
#define reg_read(phys) (*(volatile uint32_t *)PHYS_TO_K1(phys))
#define reg_write(phys, val)  ((*(volatile uint32_t *)PHYS_TO_K1(phys)) = (val))

#endif

/*
 * TYPEDEFS AND STRUCTURES
 */


/*
 * EXPORT FUNCTION
 */
uint8_t *ip_to_str(uint32_t ip);
void MacReverse(uint8_t * Mac);

#if defined(CONFIG_ARCH_MT7622) || defined(CONFIG_MACH_MT7623) 
void reg_modify_bits(unsigned int* Addr, uint32_t Data, uint32_t Offset, uint32_t Len);
#else
void reg_modify_bits(uint32_t Addr, uint32_t Data, uint32_t Offset, uint32_t Len);
#endif


void CalIpRange(uint32_t StartIp, uint32_t EndIp, uint8_t * M, uint8_t * E);
void foe_to_org_tcphdr(IN struct foe_entry *entry, IN struct iphdr *iph,
		    OUT struct tcphdr *th);
void foe_to_org_udphdr(IN struct foe_entry *entry, IN struct iphdr *iph,
		    OUT struct udphdr *uh);
void foe_to_org_iphdr(IN struct foe_entry *entry, OUT struct iphdr *iph);
void PpeIpv6PktRebuild(struct sk_buff *skb, struct foe_entry *foe_entry);
void PpeIpv4PktRebuild(struct sk_buff *skb, struct iphdr *iph,
		       struct foe_entry *foe_entry);
unsigned int Str2Ip(IN char *str);
void hwnat_memcpy(void *dest, void *src, u32 n);
#endif
