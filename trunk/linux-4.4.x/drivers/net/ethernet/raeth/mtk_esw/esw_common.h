#ifndef __MTK_ESW_COMMON__
#define __MTK_ESW_COMMON__

#define CONFIG_MT7530_GSW 				1
#define CONFIG_GE2_INTERNAL_GMAC_P5		1
#define CONFIG_RAETH_BOTH_GMAC			1

//#define CONFIG_RAETH_ESW_IGMP_SNOOP_OFF	1
#define CONFIG_RAETH_ESW_IGMP_SNOOP_HW	1

#define ESW_PORT_ID_MAX			6

#define REG_ESW_PORT_PCR_P0		0x2004
#define REG_ESW_PORT_PIC_P0		0x2008
#define REG_ESW_PORT_PVC_P0		0x2010
#define REG_ESW_PORT_PPBV1_P0		0x2014
#define REG_ESW_PORT_BSR_P0		0x201C
#define REG_ESW_MAC_PMCR_P0		0x3000
#define REG_ESW_MAC_PMSR_P0		0x3008
#define REG_ESW_MAC_GMACCR		0x30E0
#define REG_ESW_IMC			0x1C
#undef ESW_PORT_PPE

#ifdef __KERNEL__
typedef union _ULARGE_INTEGER {
	struct {
		uint32_t LowPart;
		uint32_t HighPart;
	} u;
	uint64_t QuadPart;
} ULARGE_INTEGER;
#endif

#endif
