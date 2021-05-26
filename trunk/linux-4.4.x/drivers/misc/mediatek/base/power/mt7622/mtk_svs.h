/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _MT_SVS_
#define _MT_SVS_

#include <linux/kernel.h>
#include <sync_write.h>

#define EN_SVS_OD (1)

extern void __iomem *svs_base;
#define SVS_BASEADDR svs_base

#ifdef CONFIG_OF
struct devinfo_svs_tag {
	u32 size;
	u32 tag;
	u32 volt0;
	u32 volt1;
	u32 volt2;
	u32 have_550;
};
#endif

/* SVS Register Definition */
/* #define SVS_BASEADDR        (0xF100B000) */
/* #define SVS_BASEADDR        (THERM_CTRL_BASE) */

#define SVS_REVISIONID      (SVS_BASEADDR + 0x0FC)
#define SVS_DESCHAR         (SVS_BASEADDR + 0x200)
#define SVS_TEMPCHAR        (SVS_BASEADDR + 0x204)
#define SVS_DETCHAR         (SVS_BASEADDR + 0x208)
#define SVS_AGECHAR         (SVS_BASEADDR + 0x20C)
#define SVS_DCCONFIG        (SVS_BASEADDR + 0x210)
#define SVS_AGECONFIG       (SVS_BASEADDR + 0x214)
#define SVS_FREQPCT30       (SVS_BASEADDR + 0x218)
#define SVS_FREQPCT74       (SVS_BASEADDR + 0x21C)
#define SVS_LIMITVALS       (SVS_BASEADDR + 0x220)
#define SVS_VBOOT           (SVS_BASEADDR + 0x224)
#define SVS_DETWINDOW       (SVS_BASEADDR + 0x228)
#define SVS_SVSCONFIG       (SVS_BASEADDR + 0x22C)
#define SVS_TSCALCS         (SVS_BASEADDR + 0x230)
#define SVS_RUNCONFIG       (SVS_BASEADDR + 0x234)
#define SVS_SVSEN           (SVS_BASEADDR + 0x238)
#define SVS_INIT2VALS       (SVS_BASEADDR + 0x23C)
#define SVS_DCVALUES        (SVS_BASEADDR + 0x240)
#define SVS_AGEVALUES       (SVS_BASEADDR + 0x244)
#define SVS_VOP30           (SVS_BASEADDR + 0x248)
#define SVS_VOP74           (SVS_BASEADDR + 0x24C)
#define SVS_TEMP            (SVS_BASEADDR + 0x250)
#define SVS_SVSINTSTS       (SVS_BASEADDR + 0x254)
#define SVS_SVSINTSTSRAW    (SVS_BASEADDR + 0x258)
#define SVS_SVSINTEN        (SVS_BASEADDR + 0x25C)
#define SVS_AGECOUNT	    (SVS_BASEADDR + 0x27C)
#define SVS_SMSTATE0        (SVS_BASEADDR + 0x280)
#define SVS_SMSTATE1        (SVS_BASEADDR + 0x284)
#define VDESIGN30           (SVS_BASEADDR + 0x26C)
#define VDESIGN74           (SVS_BASEADDR + 0x270)
/* #define TEMPADCENADDR       (SVS_BASEADDR + 0x074) */

#define SVS_SVSCORESEL      (SVS_BASEADDR + 0x400)
/* #define APBSEL              3:0 */

#define SVS_THERMINTST      (SVS_BASEADDR + 0x404)
#define SVS_SVSINTST        (SVS_BASEADDR + 0x408)

#define SVS_THSTAGE0ST      (SVS_BASEADDR + 0x40C)
#define SVS_THSTAGE1ST      (SVS_BASEADDR + 0x410)
#define SVS_THSTAGE2ST      (SVS_BASEADDR + 0x414)
#define SVS_THAHBST0        (SVS_BASEADDR + 0x418)
#define SVS_THAHBST1        (SVS_BASEADDR + 0x41C)
#define SVS_SVSSPARE0       (SVS_BASEADDR + 0x420)
#define SVS_SVSSPARE1       (SVS_BASEADDR + 0x424)
#define SVS_SVSSPARE2       (SVS_BASEADDR + 0x428)
#define SVS_SVSSPARE3       (SVS_BASEADDR + 0x42C)
#define SVS_THSLPEVEB       (SVS_BASEADDR + 0x430)

/* Thermal Register Definition */
#define THERMAL_BASE            svs_base

#define SVS_TEMPMONCTL0         (THERMAL_BASE + 0x000)
#define SVS_TEMPMONCTL1         (THERMAL_BASE + 0x004)
#define SVS_TEMPMONCTL2         (THERMAL_BASE + 0x008)
#define SVS_TEMPMONINT          (THERMAL_BASE + 0x00C)
#define SVS_TEMPMONINTSTS       (THERMAL_BASE + 0x010)
#define SVS_TEMPMONIDET0        (THERMAL_BASE + 0x014)
#define SVS_TEMPMONIDET1        (THERMAL_BASE + 0x018)
#define SVS_TEMPMONIDET2        (THERMAL_BASE + 0x01C)
#define SVS_TEMPH2NTHRE         (THERMAL_BASE + 0x024)
#define SVS_TEMPHTHRE           (THERMAL_BASE + 0x028)
#define SVS_TEMPCTHRE           (THERMAL_BASE + 0x02C)
#define SVS_TEMPOFFSETH         (THERMAL_BASE + 0x030)
#define SVS_TEMPOFFSETL         (THERMAL_BASE + 0x034)
#define SVS_TEMPMSRCTL0         (THERMAL_BASE + 0x038)
#define SVS_TEMPMSRCTL1         (THERMAL_BASE + 0x03C)
#define SVS_TEMPAHBPOLL         (THERMAL_BASE + 0x040)
#define SVS_TEMPAHBTO           (THERMAL_BASE + 0x044)
#define SVS_TEMPADCPNP0         (THERMAL_BASE + 0x048)
#define SVS_TEMPADCPNP1         (THERMAL_BASE + 0x04C)
#define SVS_TEMPADCPNP2         (THERMAL_BASE + 0x050)
#define SVS_TEMPADCMUX          (THERMAL_BASE + 0x054)
#define SVS_TEMPADCEXT          (THERMAL_BASE + 0x058)
#define SVS_TEMPADCEXT1         (THERMAL_BASE + 0x05C)
#define SVS_TEMPADCEN           (THERMAL_BASE + 0x060)
#define SVS_TEMPPNPMUXADDR      (THERMAL_BASE + 0x064)
#define SVS_TEMPADCMUXADDR      (THERMAL_BASE + 0x068)
#define SVS_TEMPADCEXTADDR      (THERMAL_BASE + 0x06C)
#define SVS_TEMPADCEXT1ADDR     (THERMAL_BASE + 0x070)
#define SVS_TEMPADCENADDR       (THERMAL_BASE + 0x074)
#define SVS_TEMPADCVALIDADDR    (THERMAL_BASE + 0x078)
#define SVS_TEMPADCVOLTADDR     (THERMAL_BASE + 0x07C)
#define SVS_TEMPRDCTRL          (THERMAL_BASE + 0x080)
#define SVS_TEMPADCVALIDMASK    (THERMAL_BASE + 0x084)
#define SVS_TEMPADCVOLTAGESHIFT (THERMAL_BASE + 0x088)
#define SVS_TEMPADCWRITECTRL    (THERMAL_BASE + 0x08C)
#define SVS_TEMPMSR0            (THERMAL_BASE + 0x090)
#define SVS_TEMPMSR1            (THERMAL_BASE + 0x094)
#define SVS_TEMPMSR2            (THERMAL_BASE + 0x098)
#define SVS_TEMPIMMD0           (THERMAL_BASE + 0x0A0)
#define SVS_TEMPIMMD1           (THERMAL_BASE + 0x0A4)
#define SVS_TEMPIMMD2           (THERMAL_BASE + 0x0A8)
#define SVS_TEMPMONIDET3        (THERMAL_BASE + 0x0B0)
#define SVS_TEMPADCPNP3         (THERMAL_BASE + 0x0B4)
#define SVS_TEMPMSR3            (THERMAL_BASE + 0x0B8)
#define SVS_TEMPIMMD3           (THERMAL_BASE + 0x0BC)
#define SVS_TEMPPROTCTL         (THERMAL_BASE + 0x0C0)
#define SVS_TEMPPROTTA          (THERMAL_BASE + 0x0C4)
#define SVS_TEMPPROTTB          (THERMAL_BASE + 0x0C8)
#define SVS_TEMPPROTTC          (THERMAL_BASE + 0x0CC)
#define SVS_TEMPSPARE0          (THERMAL_BASE + 0x0F0)
#define SVS_TEMPSPARE1          (THERMAL_BASE + 0x0F4)
#define SVS_TEMPSPARE2          (THERMAL_BASE + 0x0F8)
#define SVS_TEMPSPARE3          (THERMAL_BASE + 0x0FC)

/* SVS Structure */
struct SVS_INIT_T {
	unsigned int ADC_CALI_EN;
	unsigned int SVSINITEN;
	unsigned int SVSMONEN;
	unsigned int MDES;
	unsigned int BDES;
	unsigned int DCCONFIG;
	unsigned int DCMDET;
	unsigned int DCBDET;
	unsigned int AGECONFIG;
	unsigned int AGEM;
	unsigned int AGEDELTA;
	unsigned int DVTFIXED;
	unsigned int VCO;
	unsigned int MTDES;
	unsigned int MTS;
	unsigned int BTS;
	unsigned char FREQPCT0;
	unsigned char FREQPCT1;
	unsigned char FREQPCT2;
	unsigned char FREQPCT3;
	unsigned char FREQPCT4;
	unsigned char FREQPCT5;
	unsigned char FREQPCT6;
	unsigned char FREQPCT7;
	unsigned int DETWINDOW;
	unsigned int VMAX;
	unsigned int VMIN;
	unsigned int DTHI;
	unsigned int DTLO;
	unsigned int VBOOT;
	unsigned int DETMAX;
	unsigned int DCVOFFSETIN;
	unsigned int AGEVOFFSETIN;
};

typedef enum {
	SVS_CTRL_CPU = 0,
	NR_SVS_CTRL,
} svs_ctrl_id;

typedef enum {
	SVS_DET_CPU = SVS_CTRL_CPU,
	NR_SVS_DET,
} svs_det_id;


/* SVS Extern Function */
extern u32 get_devinfo_with_index(u32 index);
extern void mt_svs_lock(unsigned long *flags);
extern void mt_svs_unlock(unsigned long *flags);
extern int mt_svs_idle_can_enter(void);
extern int mt_svs_status(svs_det_id id);
extern int get_svs_status(void);
extern unsigned int get_vcore_svs_volt(int uv);
extern unsigned int is_have_550(void);
extern int is_svs_initialized_done(void);

#endif
