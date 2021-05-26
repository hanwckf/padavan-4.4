/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/reboot.h>
#include <linux/nvmem-consumer.h>
#include "mtk_thermal_typedefs.h"
/* #include <mach/mt_ptp.h> */
/* #include "../../base/power/mt2701/mt_spm.h" */
#include <mt_cpufreq.h>
#include <mt-plat/sync_write.h>
#include "mt-plat/mtk_thermal_monitor.h"
#include "mach/mtk_thermal.h"
#include <linux/time.h>
/* #include <inc/mt_gpufreq.h> */

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#define __MT_MTK_TS_CPU_C__

#ifdef CONFIG_OF
u32 thermal_irq_number;
void __iomem *thermal_base;
void __iomem *auxadc_ts_base;
void __iomem *apmixed_ts_base;
void __iomem *pericfg_base;

int thermal_phy_base;
int auxadc_ts_phy_base;
int apmixed_phy_base;
int pericfg_phy_base;

struct clk *clk_peri_therm;
struct clk *clk_auxadc;
#endif

#define THERMAL_DEVICE_NAME_2701	"mediatek,mt2701-thermal"

/* 1: turn on adaptive AP cooler; 0: turn off */
#define CPT_ADAPTIVE_AP_COOLER (0)

/* 1: turn on supports to MET logging; 0: turn off */
#define CONFIG_SUPPORT_MET_MTKTSCPU (0)

#define THERMAL_CONTROLLER_HW_FILTER (1) /* 1, 2, 4, 8, 16 */

/* 1: turn on thermal controller HW thermal protection; 0: turn off */
#define THERMAL_CONTROLLER_HW_TP     (1)

/* 1: turn on SW filtering in this sw module; 0: turn off */
#define MTK_TS_CPU_SW_FILTER         (1)

/* 1: thermal driver fast polling, use hrtimer; 0: turn off */
#define THERMAL_DRV_FAST_POLL_HRTIMER          (0)

/* 1: thermal driver update temp to MET directly, use hrtimer; 0: turn off */
#define THERMAL_DRV_UPDATE_TEMP_DIRECT_TO_MET  (1)


#define MIN(_a_, _b_) ((_a_) > (_b_) ? (_b_) : (_a_))
#define MAX(_a_, _b_) ((_a_) > (_b_) ? (_a_) : (_b_))

/*==============*/
/*Variables*/
/*==============*/
#if CPT_ADAPTIVE_AP_COOLER
static int g_curr_temp;
static int g_prev_temp;
#endif

#define thermal_readl(addr)		readl(addr)
#define thermal_writel(addr, val)	writel((val), ((void *)addr))
#define thermal_setl(addr, val)		writel(thermal_readl(addr) | (val), ((void *)addr))
#define thermal_clrl(addr, val)		writel(thermal_readl(addr) & ~(val), ((void *)addr))
#define THERMAL_WRAP_WR32(val, addr)	writel((val), ((void *)addr))

static unsigned int interval = 1000;	/* mseconds, 0 : no auto polling */
static int last_cpu_real_temp;
/* trip_temp[0] must be initialized to the thermal HW protection point. */
static int trip_temp[10] = {
	117000, 100000, 85000, 76000, 70000, 68000, 45000, 35000, 25000, 15000
};

/* atic int gtemp_hot=80000, gtemp_normal=70000, gtemp_low=50000,goffset=5000; */

static unsigned int *cl_dev_state;
static unsigned int cl_dev_sysrst_state;
#if CPT_ADAPTIVE_AP_COOLER
static unsigned int cl_dev_adp_cpu_state[MAX_CPT_ADAPTIVE_COOLERS] = { 0 };

static unsigned int cl_dev_adp_cpu_state_active;
static int active_adp_cooler;
#endif
static struct thermal_zone_device *thz_dev;

static struct thermal_cooling_device **cl_dev;
static struct thermal_cooling_device *cl_dev_sysrst;
#if CPT_ADAPTIVE_AP_COOLER
static struct thermal_cooling_device *cl_dev_adp_cpu[MAX_CPT_ADAPTIVE_COOLERS] = { NULL };
#endif

#if CPT_ADAPTIVE_AP_COOLER
static int TARGET_TJS[MAX_CPT_ADAPTIVE_COOLERS] = { 85000, 0 };
static int PACKAGE_THETA_JA_RISES[MAX_CPT_ADAPTIVE_COOLERS] = { 35, 0 };
static int PACKAGE_THETA_JA_FALLS[MAX_CPT_ADAPTIVE_COOLERS] = { 25, 0 };
static int MINIMUM_CPU_POWERS[MAX_CPT_ADAPTIVE_COOLERS] = { 1200, 0 };
static int MAXIMUM_CPU_POWERS[MAX_CPT_ADAPTIVE_COOLERS] = { 4400, 0 };
static int MINIMUM_GPU_POWERS[MAX_CPT_ADAPTIVE_COOLERS] = { 350, 0 };
static int MAXIMUM_GPU_POWERS[MAX_CPT_ADAPTIVE_COOLERS] = { 960, 0 };
static int FIRST_STEP_TOTAL_POWER_BUDGETS[MAX_CPT_ADAPTIVE_COOLERS] = { 3300, 0 };
static int MINIMUM_BUDGET_CHANGES[MAX_CPT_ADAPTIVE_COOLERS] = { 50, 0 };
#endif

static U32 calefuse1;
static U32 calefuse2;
static U32 calefuse3;

/* +ASC+ */
#if CPT_ADAPTIVE_AP_COOLER
/*0: default:  ATM v1 */
/*1:	 FTL ATM v2 */
/*2:	 CPU_GPU_Weight ATM v2 */
static int mtktscpu_atm = 1;
static int tt_ratio_high_rise = 1;
static int tt_ratio_high_fall = 1;
static int tt_ratio_low_rise = 1;
static int tt_ratio_low_fall = 1;
static int tp_ratio_high_rise = 1;
static int tp_ratio_high_fall;
static int tp_ratio_low_rise;
static int tp_ratio_low_fall;
/* static int cpu_loading = 0; */
#endif
/* -ASC- */

static int kernelmode;
static int g_THERMAL_TRIP[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* atic int RAW_TS2=0, RAW_TS3=0, RAW_TS4=0; */

static int num_trip = 5;
static int previous_step = -1;
int MA_len_temp;
static int proc_write_flag;
static char *cooler_name;
#define CPU_COOLER_NUM 19

static DEFINE_MUTEX(TS_lock);

#if CPT_ADAPTIVE_AP_COOLER
static const char adaptive_cooler_name[] = "cpu_adaptive_";
#endif

static char g_bind0[20] = "mtktscpu-sysrst";
static char g_bind1[20] = "mtk-cl-kshutdown00";
static char g_bind2[20] = "1200";
static char g_bind3[20] = "1400";
static char g_bind4[20] = "1600";
static char g_bind5[20] = "1800";
static char g_bind6[20] = "";
static char g_bind7[20] = "";
static char g_bind8[20] = "";
static char g_bind9[20] = "";

static int read_curr_temp;

#define MTKTSCPU_TEMP_CRIT 120000	/* 120.000 degree Celsius */

static int tc_mid_trip = -275000;

static S32 temperature_to_raw_abb(U32 ret);
/* static int last_cpu_t=0; */
int last_abb_t;
int last_CPU1_t;
int last_CPU2_t;

static int g_tc_resume;		/* default=0,read temp */

static S32 g_adc_ge_t;
static S32 g_adc_oe_t;
static S32 g_o_vtsmcu1;
static S32 g_o_vtsmcu2;
static S32 g_o_vtsmcu3;
static S32 g_o_vtsmcu4;
static S32 g_o_vtsabb;
static S32 g_degc_cali;
static S32 g_adc_cali_en_t;
static S32 g_o_slope;
static S32 g_o_slope_sign;
static S32 g_id;

static S32 g_ge;
static S32 g_oe;
static S32 g_gain;

static S32 g_x_roomt1;
static S32 g_x_roomt2;
static S32 g_x_roomtabb;

static int Num_of_OPP;

#if 0
static int Num_of_GPU_OPP = 1;	/* Set this value =1 for non-DVS GPU */
#else				/* DVFS GPU */
static int Num_of_GPU_OPP;
#endif


#define y_curr_repeat_times 1
#define THERMAL_NAME    "mtk-thermal"
/* #define GPU_Default_POWER     456 */

#define		FIX_ME_IOMAP

static struct mtk_cpu_power_info *mtk_cpu_power;
static int tscpu_num_opp;
static struct mt_gpufreq_power_table_info *mtk_gpu_power;

static int tscpu_cpu_dmips[CPU_COOLER_NUM] = { 0 };

int mtktscpu_limited_dmips;

static bool talking_flag;
static int temperature_switch;
static int thermal_fast_init(void);

#if THERMAL_DRV_UPDATE_TEMP_DIRECT_TO_MET
static int a_tscpu_all_temp[THERMAL_SENSOR_NUM] = { 0 };

static DEFINE_MUTEX(MET_GET_TEMP_LOCK);
static met_thermalsampler_funcMET g_pThermalSampler;
void mt_thermalsampler_registerCB(met_thermalsampler_funcMET pCB)
{
	g_pThermalSampler = pCB;
}
EXPORT_SYMBOL(mt_thermalsampler_registerCB);

static DEFINE_SPINLOCK(tscpu_met_spinlock);
void tscpu_met_lock(unsigned long *flags)
{
	spin_lock_irqsave(&tscpu_met_spinlock, *flags);
}
EXPORT_SYMBOL(tscpu_met_lock);

void tscpu_met_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&tscpu_met_spinlock, *flags);
}
EXPORT_SYMBOL(tscpu_met_unlock);

#endif

void __attribute__ ((weak)) mt_cpufreq_thermal_protect(unsigned int limited_power)
{
	pr_err("%s is not ready\n", __func__);
}

void set_taklking_flag(bool flag)
{
	talking_flag = flag;
	pr_debug("talking_flag=%d\n", talking_flag);
}

static unsigned int adaptive_cpu_power_limit = 0x7FFFFFFF, static_cpu_power_limit = 0x7FFFFFFF;

static unsigned int adaptive_gpu_power_limit = 0x7FFFFFFF, static_gpu_power_limit = 0x7FFFFFFF;

void tscpu_thermal_clock_on(void)
{
	pr_debug("tscpu_thermal_clock_on\n");
	clk_prepare(clk_auxadc);
	clk_enable(clk_auxadc);
	clk_prepare(clk_peri_therm);
	clk_enable(clk_peri_therm);
}

void tscpu_thermal_clock_off(void)
{
	pr_debug("tscpu_thermal_clock_off\n");
	clk_disable(clk_peri_therm);
	clk_unprepare(clk_peri_therm);
	clk_disable(clk_auxadc);
	clk_unprepare(clk_auxadc);
}

void get_thermal_all_register(void)
{
	pr_debug("get_thermal_all_register\n");
	pr_debug("TEMPMSR1            = 0x%8x\n", DRV_Reg32(TEMPMSR1));
	pr_debug("TEMPMSR2            = 0x%8x\n", DRV_Reg32(TEMPMSR2));
	pr_debug("TEMPMONCTL0         = 0x%8x\n", DRV_Reg32(TEMPMONCTL0));
	pr_debug("TEMPMONCTL1         = 0x%8x\n", DRV_Reg32(TEMPMONCTL1));
	pr_debug("TEMPMONCTL2         = 0x%8x\n", DRV_Reg32(TEMPMONCTL2));
	pr_debug("TEMPMONINT          = 0x%8x\n", DRV_Reg32(TEMPMONINT));
	pr_debug("TEMPMONINTSTS       = 0x%8x\n", DRV_Reg32(TEMPMONINTSTS));
	pr_debug("TEMPMONIDET0        = 0x%8x\n", DRV_Reg32(TEMPMONIDET0));

	pr_debug("TEMPMONIDET1        = 0x%8x\n", DRV_Reg32(TEMPMONIDET1));
	pr_debug("TEMPMONIDET2        = 0x%8x\n", DRV_Reg32(TEMPMONIDET2));
	pr_debug("TEMPH2NTHRE         = 0x%8x\n", DRV_Reg32(TEMPH2NTHRE));
	pr_debug("TEMPHTHRE           = 0x%8x\n", DRV_Reg32(TEMPHTHRE));
	pr_debug("TEMPCTHRE           = 0x%8x\n", DRV_Reg32(TEMPCTHRE));
	pr_debug("TEMPOFFSETH         = 0x%8x\n", DRV_Reg32(TEMPOFFSETH));

	pr_debug("TEMPOFFSETL         = 0x%8x\n", DRV_Reg32(TEMPOFFSETL));
	pr_debug("TEMPMSRCTL0         = 0x%8x\n", DRV_Reg32(TEMPMSRCTL0));
	pr_debug("TEMPMSRCTL1         = 0x%8x\n", DRV_Reg32(TEMPMSRCTL1));
	pr_debug("TEMPAHBPOLL         = 0x%8x\n", DRV_Reg32(TEMPAHBPOLL));
	pr_debug("TEMPAHBTO           = 0x%8x\n", DRV_Reg32(TEMPAHBTO));
	pr_debug("TEMPADCPNP0         = 0x%8x\n", DRV_Reg32(TEMPADCPNP0));

	pr_debug("TEMPADCPNP1         = 0x%8x\n", DRV_Reg32(TEMPADCPNP1));
	pr_debug("TEMPADCPNP2         = 0x%8x\n", DRV_Reg32(TEMPADCPNP2));
	pr_debug("TEMPADCMUX          = 0x%8x\n", DRV_Reg32(TEMPADCMUX));
	pr_debug("TEMPADCEXT          = 0x%8x\n", DRV_Reg32(TEMPADCEXT));
	pr_debug("TEMPADCEXT1         = 0x%8x\n", DRV_Reg32(TEMPADCEXT1));
	pr_debug("TEMPADCEN           = 0x%8x\n", DRV_Reg32(TEMPADCEN));

	pr_debug("TEMPPNPMUXADDR      = 0x%8x\n", DRV_Reg32(TEMPPNPMUXADDR));
	pr_debug("TEMPADCMUXADDR      = 0x%8x\n", DRV_Reg32(TEMPADCMUXADDR));
	pr_debug("TEMPADCEXTADDR      = 0x%8x\n", DRV_Reg32(TEMPADCEXTADDR));
	pr_debug("TEMPADCEXT1ADDR     = 0x%8x\n", DRV_Reg32(TEMPADCEXT1ADDR));
	pr_debug("TEMPADCENADDR       = 0x%8x\n", DRV_Reg32(TEMPADCENADDR));
	pr_debug("TEMPADCVALIDADDR    = 0x%8x\n", DRV_Reg32(TEMPADCVALIDADDR));

	pr_debug("TEMPADCVOLTADDR     = 0x%8x\n", DRV_Reg32(TEMPADCVOLTADDR));
	pr_debug("TEMPRDCTRL          = 0x%8x\n", DRV_Reg32(TEMPRDCTRL));
	pr_debug("TEMPADCVALIDMASK    = 0x%8x\n", DRV_Reg32(TEMPADCVALIDMASK));
	pr_debug("TEMPADCVOLTAGESHIFT = 0x%8x\n", DRV_Reg32(TEMPADCVOLTAGESHIFT));
	pr_debug("TEMPADCWRITECTRL    = 0x%8x\n", DRV_Reg32(TEMPADCWRITECTRL));
	pr_debug("TEMPMSR0            = 0x%8x\n", DRV_Reg32(TEMPMSR0));


	pr_debug("TEMPIMMD0           = 0x%8x\n", DRV_Reg32(TEMPIMMD0));
	pr_debug("TEMPIMMD1           = 0x%8x\n", DRV_Reg32(TEMPIMMD1));
	pr_debug("TEMPIMMD2           = 0x%8x\n", DRV_Reg32(TEMPIMMD2));
	pr_debug("TEMPPROTCTL         = 0x%8x\n", DRV_Reg32(TEMPPROTCTL));

	pr_debug("TEMPPROTTA          = 0x%8x\n", DRV_Reg32(TEMPPROTTA));
	pr_debug("TEMPPROTTB          = 0x%8x\n", DRV_Reg32(TEMPPROTTB));
	pr_debug("TEMPPROTTC          = 0x%8x\n", DRV_Reg32(TEMPPROTTC));
	pr_debug("TEMPSPARE0          = 0x%8x\n", DRV_Reg32(TEMPSPARE0));
	pr_debug("TEMPSPARE1          = 0x%8x\n", DRV_Reg32(TEMPSPARE1));
	pr_debug("TEMPSPARE2          = 0x%8x\n", DRV_Reg32(TEMPSPARE2));
	pr_debug("TEMPSPARE3          = 0x%8x\n", DRV_Reg32(TEMPSPARE3));
	/* pr_debug("0x11001040          = 0x%8x\n", DRV_Reg32(0xF1001040)); */
}

void get_thermal_slope_intercept(struct TS_PTPOD *ts_info)
{
	unsigned int temp0, temp1, temp2;
	struct TS_PTPOD ts_ptpod;

	pr_debug("get_thermal_slope_intercept\n");

	/* temp0 = (10000*100000/4096/g_gain)*15/18; */
	temp0 = (10000 * 100000 / g_gain) * 15 / 18;
	/* pr_debug("temp0=%d\n", temp0); */
	if (g_o_slope_sign == 0)
		temp1 = temp0 / (165 + g_o_slope);
	else
		temp1 = temp0 / (165 - g_o_slope);

	/* pr_debug("temp1=%d\n", temp1); */
	/* ts_ptpod.ts_MTS = temp1 - (2*temp1) + 2048; */
	ts_ptpod.ts_MTS = temp1;

	temp0 = (g_degc_cali * 10 / 2);
	temp1 = ((10000 * 100000 / 4096 / g_gain) * g_oe + g_x_roomtabb * 10) * 15 / 18;
	/* pr_debug("temp1=%d\n", temp1); */
	if (g_o_slope_sign == 0)
		temp2 = temp1 * 10 / (165 + g_o_slope);
	else
		temp2 = temp1 * 10 / (165 - g_o_slope);

	/* pr_debug("temp2=%d\n", temp2); */
	ts_ptpod.ts_BTS = (temp0 + temp2 - 250) * 4 / 10;

	/* ts_info = &ts_ptpod; */
	ts_info->ts_MTS = ts_ptpod.ts_MTS;
	ts_info->ts_BTS = ts_ptpod.ts_BTS;
	pr_debug("ts_MTS=%d, ts_BTS=%d\n", ts_ptpod.ts_MTS, ts_ptpod.ts_BTS);
}
EXPORT_SYMBOL(get_thermal_slope_intercept);

static irqreturn_t thermal_interrupt_handler(int irq, void *dev_id)
{
	U32 ret = 0;

	ret = DRV_Reg32(TEMPMONINTSTS);
	/* pr_debug("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"); */
	pr_debug("thermal_interrupt_handler,ret=0x%08x\n", ret);
	/* pr_debug("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"); */

	/* ret2 = DRV_Reg32(THERMINTST); */
	/* pr_debug("thermal_interrupt_handler : THERMINTST = 0x%x\n", ret2); */


	/* for SPM reset debug */
	/* dump_spm_reg(); */

	/* pr_debug("thermal_isr: [Interrupt trigger]: status = 0x%x\n", ret); */
	if (ret & THERMAL_MON_CINTSTS0)
		pr_emerg("thermal_isr: thermal sensor point 0 - cold interrupt trigger\n");

	if (ret & THERMAL_MON_HINTSTS0)
		pr_emerg("<<<thermal_isr>>>: thermal sensor point 0 - hot interrupt trigger\n");

	if (ret & THERMAL_MON_HINTSTS1)
		pr_emerg("<<<thermal_isr>>>: thermal sensor point 1 - hot interrupt trigger\n");

	if (ret & THERMAL_MON_HINTSTS2)
		pr_emerg("<<<thermal_isr>>>: thermal sensor point 2 - hot interrupt trigger\n");

	if (ret & THERMAL_tri_SPM_State0)
		pr_emerg("thermal_isr: Thermal state0 to trigger SPM state0\n");

	if (ret & THERMAL_tri_SPM_State1)
		pr_debug("thermal_isr: Thermal state1 to trigger SPM state1\n");

	if (ret & THERMAL_tri_SPM_State2)
		pr_emerg("thermal_isr: Thermal state2 to trigger SPM state2\n");

	return IRQ_HANDLED;
}

static void thermal_reset_and_initial(void)
{
	UINT32 temp;
	int cnt = 0;
	int temp2;

	pr_debug("[Reset and init thermal controller]\n");

	tscpu_thermal_clock_on();

	temp = DRV_Reg32(PERI_GLOBALCON_RST0);
	temp |= 0x00010000;
	THERMAL_WRAP_WR32(temp, PERI_GLOBALCON_RST0);

	temp = DRV_Reg32(PERI_GLOBALCON_RST0);
	temp &= 0xFFFEFFFF;
	THERMAL_WRAP_WR32(temp, PERI_GLOBALCON_RST0);

	thermal_fast_init();
	while (cnt < 30) {
		temp2 = DRV_Reg32(TEMPMSRCTL1);
		pr_debug("TEMPMSRCTL1 = 0x%x\n", temp2);

		if (((temp2 & 0x81) == 0x00) || ((temp2 & 0x81) == 0x81)) {

			DRV_WriteReg32(TEMPMSRCTL1, (temp2 | 0x10E));

			break;
		}
		pr_debug("temp=0x%x, cnt=%d\n", temp2, cnt);
		udelay(10);
		cnt++;
	}
	THERMAL_WRAP_WR32(0x00000000, TEMPMONCTL0);

	/* AuxADC Initialization,ref MT6582_AUXADC.doc */
	temp = DRV_Reg32(AUXADC_CON0_V); /*Auto set enable for CH11*/
	temp &= 0xFFFFF7FF; /*0: Not AUTOSET mode*/
	THERMAL_WRAP_WR32(temp, AUXADC_CON0_V);
	THERMAL_WRAP_WR32(0x800, AUXADC_CON1_CLR_V);
	THERMAL_WRAP_WR32(0x000003FF, TEMPMONCTL1);

#if THERMAL_CONTROLLER_HW_FILTER == 2
	THERMAL_WRAP_WR32(0x03FF03FF, TEMPMONCTL2);
	THERMAL_WRAP_WR32(0x006DEC78, TEMPAHBPOLL);
	THERMAL_WRAP_WR32(0x00000092, TEMPMSRCTL0);
#elif THERMAL_CONTROLLER_HW_FILTER == 4
	THERMAL_WRAP_WR32(0x03FF03FF, TEMPMONCTL2);
	THERMAL_WRAP_WR32(0x0043F459, TEMPAHBPOLL);
	THERMAL_WRAP_WR32(0x000000DB, TEMPMSRCTL0);
#elif THERMAL_CONTROLLER_HW_FILTER == 8
	THERMAL_WRAP_WR32(0x03390339, TEMPMONCTL2);
	THERMAL_WRAP_WR32(0x000C96FA, TEMPAHBPOLL);
	THERMAL_WRAP_WR32(0x00000124, TEMPMSRCTL0);
#elif THERMAL_CONTROLLER_HW_FILTER == 16
	THERMAL_WRAP_WR32(0x01C001C0, TEMPMONCTL2);
	THERMAL_WRAP_WR32(0x0006FE8B, TEMPAHBPOLL);
	THERMAL_WRAP_WR32(0x0000016D, TEMPMSRCTL0);
#else
	THERMAL_WRAP_WR32(0x03FF03FF, TEMPMONCTL2);
	THERMAL_WRAP_WR32(0x00FFFFFF, TEMPAHBPOLL);
	THERMAL_WRAP_WR32(0x00000000, TEMPMSRCTL0);
#endif

	THERMAL_WRAP_WR32(0xFFFFFFFF, TEMPAHBTO);
	THERMAL_WRAP_WR32(0x00000000, TEMPMONIDET0);
	THERMAL_WRAP_WR32(0x00000000, TEMPMONIDET1);

	THERMAL_WRAP_WR32(0x800, AUXADC_CON1_SET_V);
	THERMAL_WRAP_WR32(0x800, TEMPADCMUX);
	THERMAL_WRAP_WR32((UINT32) AUXADC_CON1_CLR_P, TEMPADCMUXADDR);
	THERMAL_WRAP_WR32(0x800, TEMPADCEN);
	THERMAL_WRAP_WR32((UINT32) AUXADC_CON1_SET_P, TEMPADCENADDR);
	THERMAL_WRAP_WR32((UINT32) AUXADC_DAT11_P, TEMPADCVALIDADDR);
	THERMAL_WRAP_WR32((UINT32) AUXADC_DAT11_P, TEMPADCVOLTADDR);
	THERMAL_WRAP_WR32(0x0, TEMPRDCTRL);
	THERMAL_WRAP_WR32(0x0000002C, TEMPADCVALIDMASK);
	THERMAL_WRAP_WR32(0x0, TEMPADCVOLTAGESHIFT);
	THERMAL_WRAP_WR32(0x2, TEMPADCWRITECTRL);
	temp = DRV_Reg32(TS_CON0);
	temp &= ~(0x000000C0);
	THERMAL_WRAP_WR32(temp, TS_CON0);

	/* Add delay time before sensor polling. */
	udelay(200);

	THERMAL_WRAP_WR32(0x0, TEMPADCPNP0);
	THERMAL_WRAP_WR32(0x1, TEMPADCPNP1);
	THERMAL_WRAP_WR32((UINT32) apmixed_phy_base + 0x604, TEMPPNPMUXADDR);
	THERMAL_WRAP_WR32(0x3, TEMPADCWRITECTRL);

	temp = DRV_Reg32(TEMPMSRCTL1);
	DRV_WriteReg32(TEMPMSRCTL1, ((temp & (~0x10E))));

	THERMAL_WRAP_WR32(0x00000003, TEMPMONCTL0);
}

int mtk_cpufreq_register(struct mtk_cpu_power_info *freqs, int num)
{
	int i = 0;
	int gpu_power = 0;
	int base;

	pr_debug("mtk_cpufreq_register\n");

	tscpu_num_opp = num;

	mtk_cpu_power = kzalloc((num) * sizeof(struct mtk_cpu_power_info), GFP_KERNEL);
	if (mtk_cpu_power == NULL)
		return -ENOMEM;

	if (0 != Num_of_GPU_OPP && NULL != mtk_gpu_power)
		gpu_power = mtk_gpu_power[Num_of_GPU_OPP - 1].gpufreq_power;
	else
		pr_debug("Num_of_GPU_OPP is 0!\n");

	for (i = 0; i < num; i++) {
		int dmips = freqs[i].cpufreq_khz * freqs[i].cpufreq_ncpu / 1000;

		int cl_id = (((freqs[i].cpufreq_power + gpu_power) + 99) / 100) - 7;

		mtk_cpu_power[i].cpufreq_khz = freqs[i].cpufreq_khz;
		mtk_cpu_power[i].cpufreq_ncpu = freqs[i].cpufreq_ncpu;
		mtk_cpu_power[i].cpufreq_power = freqs[i].cpufreq_power;

		if (cl_id < CPU_COOLER_NUM) {
			if (tscpu_cpu_dmips[cl_id] < dmips)
				tscpu_cpu_dmips[cl_id] = dmips;
		}

	}

	base = (mtk_cpu_power[num - 1].cpufreq_khz * mtk_cpu_power[num - 1].cpufreq_ncpu) / 1000;

	for (i = 0; i < CPU_COOLER_NUM; i++) {
		if (tscpu_cpu_dmips[i] == 0 || tscpu_cpu_dmips[i] < base)
			tscpu_cpu_dmips[i] = base;
		else
			base = tscpu_cpu_dmips[i];
	}

	mtktscpu_limited_dmips = base;

	return 0;
}
EXPORT_SYMBOL(mtk_cpufreq_register);

/* Init local structure for AP coolers */
static int init_cooler(void)
{
	int i;
	int num = CPU_COOLER_NUM;

	cl_dev_state = kzalloc((num) * sizeof(unsigned int), GFP_KERNEL);
	if (cl_dev_state == NULL)
		return -ENOMEM;

	cl_dev = kzalloc((num) * sizeof(struct thermal_cooling_device *), GFP_KERNEL);
	if (cl_dev == NULL)
		return -ENOMEM;

	cooler_name = kzalloc((num) * sizeof(char) * 20, GFP_KERNEL);
	if (cooler_name == NULL)
		return -ENOMEM;

	for (i = 0; i < num; i++)
		sprintf(cooler_name + (i * 20), "%d", (i * 100) + 700);	/* using index=>0=700,1=800 ~ 19=2600 */

	Num_of_OPP = num;	/* CPU COOLER COUNT, not CPU OPP count */
	return 0;
}


static void set_thermal_ctrl_trigger_SPM(int temperature)
{
#if THERMAL_CONTROLLER_HW_TP
	int temp = 0;
	int raw_high, raw_middle, raw_low;

	pr_err("[Set_thermal_ctrl_trigger_SPM]: temperature=%d\n", temperature);

	/*temperature to trigger SPM state2 */
	raw_high   = temperature_to_raw_abb(temperature);
	raw_middle = temperature_to_raw_abb(20000);
	raw_low    = temperature_to_raw_abb(5000);

	temp = DRV_Reg32(TEMPMONINT);
	/* THERMAL_WRAP_WR32(temp & 0x8FFFFFFF, TEMPMONINT);*/	/* enable trigger SPM interrupt  */
	THERMAL_WRAP_WR32(temp & 0x1FFFFFFF, TEMPMONINT);	/* disable trigger SPM interrupt */

	THERMAL_WRAP_WR32(0x20000, TEMPPROTCTL); /* set hot to wakeup event control */
	THERMAL_WRAP_WR32(raw_low, TEMPPROTTA);
	THERMAL_WRAP_WR32(raw_middle, TEMPPROTTB);
	THERMAL_WRAP_WR32(raw_high, TEMPPROTTC); /* set hot to HOT wakeup event */

	/*trigger cold ,normal and hot interrupt*/
	/*remove for temp	THERMAL_WRAP_WR32(temp | 0xE0000000, TEMPMONINT); */
	/* enable trigger SPM interrupt */
	/*Only trigger hot interrupt*/
	THERMAL_WRAP_WR32(temp | 0x80000000, TEMPMONINT);	/* enable trigger SPM interrupt */
#endif
}

int mtk_gpufreq_register(struct mt_gpufreq_power_table_info *freqs, int num)
{
	int i = 0;

	pr_debug("mtk_gpufreq_register\n");
	mtk_gpu_power = kzalloc((num) * sizeof(struct mt_gpufreq_power_table_info), GFP_KERNEL);
	if (mtk_gpu_power == NULL)
		return -ENOMEM;

	for (i = 0; i < num; i++) {
		mtk_gpu_power[i].gpufreq_khz = freqs[i].gpufreq_khz;
		mtk_gpu_power[i].gpufreq_power = freqs[i].gpufreq_power;

		pr_debug("[%d].gpufreq_khz=%u, .gpufreq_power=%u\n",
			 i, freqs[i].gpufreq_khz, freqs[i].gpufreq_power);
	}

	Num_of_GPU_OPP = num;	/* GPU OPP count */
	return 0;
}
EXPORT_SYMBOL(mtk_gpufreq_register);

void mtkts_dump_cali_info(void)
{
	pr_debug("[calibration] g_adc_ge_t      = 0x%x\n", g_adc_ge_t);
	pr_debug("[calibration] g_adc_oe_t      = 0x%x\n", g_adc_oe_t);
	pr_debug("[calibration] g_degc_cali     = 0x%x\n", g_degc_cali);
	pr_debug("[calibration] g_adc_cali_en_t = 0x%x\n", g_adc_cali_en_t);
	pr_debug("[calibration] g_o_slope       = 0x%x\n", g_o_slope);
	pr_debug("[calibration] g_o_slope_sign  = 0x%x\n", g_o_slope_sign);
	pr_debug("[calibration] g_id            = 0x%x\n", g_id);

	pr_debug("[calibration] g_o_vtsmcu2     = 0x%x\n", g_o_vtsmcu2);
	pr_debug("[calibration] g_o_vtsmcu3     = 0x%x\n", g_o_vtsmcu3);
	pr_debug("[calibration] g_o_vtsmcu4     = 0x%x\n", g_o_vtsmcu4);
}


static void thermal_cal_prepare(struct device *dev)
{
	U32 *buf;
	struct nvmem_cell *cell;
	size_t len;

	g_adc_ge_t = 512;
	g_adc_oe_t = 512;
	g_degc_cali = 40;
	g_o_slope = 0;
	g_o_slope_sign = 0;
	g_o_vtsmcu1 = 287;
	g_o_vtsmcu2 = 287;
	g_o_vtsabb = 287;

	cell = nvmem_cell_get(dev, "calibration-data");
	if (IS_ERR(cell))
		return;

	buf = (u32 *)nvmem_cell_read(cell, &len);

	nvmem_cell_put(cell);

	if (IS_ERR(buf))
		return;

	if (len < 3 * sizeof(u32)) {
		dev_err(dev, "invalid calibration data\n");
		goto out;
	}

	calefuse1 = buf[0];
	calefuse2 = buf[1];
	calefuse3 = buf[2];
	pr_err("[Thermal calibration] buf[0]=0x%x, buf[1]=0x%x, , buf[2]=0x%x\n", buf[0], buf[1], buf[2]);

	g_adc_cali_en_t  = (buf[2] & 0x01000000)>>24;    /* ADC_CALI_EN_T(1b) *(0x1020642c)[24] */

	if (g_adc_cali_en_t) {
		g_adc_ge_t		= (buf[0] & 0x003FF000)>>12;	/*ADC_GE_T [9:0] *(0x10206424)[21:12] */
		g_adc_oe_t		= (buf[0] & 0xFFC00000)>>22;	/*ADC_OE_T [9:0] *(0x10206424)[31:22] */
		g_o_vtsmcu1		= (buf[1] & 0x000001FF);	/* O_VTSMCU1    (9b) *(0x10206428)[8:0] */
		g_o_vtsmcu2		= (buf[1] & 0x0003FE00)>>9;	/* O_VTSMCU2    (9b) *(0x10206428)[17:9] */
		g_o_vtsabb		= (buf[2] & 0x0003FE00)>>9;	/* O_VTSABB     (9b) *(0x1020642c)[17:9] */
		g_degc_cali		= (buf[2] & 0x00FC0000)>>18;	/* DEGC_cali    (6b) *(0x1020642c)[23:18] */
		g_o_slope		= (buf[2] & 0xFC000000)>>26;	/* O_SLOPE      (6b) *(0x1020642c)[31:26] */
		g_o_slope_sign		= (buf[2] & 0x02000000)>>25;	/* O_SLOPE_SIGN (1b) *(0x1020642c)[25] */
		g_id			= (buf[1] & 0x08000000)>>27;
		if (g_id == 0)
			g_o_slope = 0;
	} else {
		dev_info(dev, "Device not calibrated, using default calibration values\n");
	}

	pr_err("[Power/CPU_Thermal] [Thermal calibration] g_adc_ge_t      = 0x%x\n", g_adc_ge_t);
	pr_err("[Power/CPU_Thermal] [Thermal calibration] g_adc_oe_t      = 0x%x\n", g_adc_oe_t);
	pr_err("[Power/CPU_Thermal] [Thermal calibration] g_degc_cali     = 0x%x\n", g_degc_cali);
	pr_err("[Power/CPU_Thermal] [Thermal calibration] g_adc_cali_en_t = 0x%x\n", g_adc_cali_en_t);
	pr_err("[Power/CPU_Thermal] [Thermal calibration] g_o_slope       = 0x%x\n", g_o_slope);
	pr_err("[Power/CPU_Thermal] [Thermal calibration] g_o_slope_sign  = 0x%x\n", g_o_slope_sign);
	pr_err("[Power/CPU_Thermal] [Thermal calibration] g_id            = 0x%x\n", g_id);
	pr_err("[Power/CPU_Thermal] [Thermal calibration] g_o_vtsmcu1     = 0x%x\n", g_o_vtsmcu1);
	pr_err("[Power/CPU_Thermal] [Thermal calibration] g_o_vtsmcu2     = 0x%x\n", g_o_vtsmcu2);
	pr_err("[Power/CPU_Thermal] [Thermal calibration] g_o_vtsabb      = 0x%x\n", g_o_vtsabb);

out:
	kfree(buf);

	return;
}

static void thermal_cal_prepare_2(U32 ret)
{
	S32 format_1, format_2, format_abb;

	pr_debug("thermal_cal_prepare_2\n");

	g_ge = ((g_adc_ge_t - 512) * 10000) / 4096;
	g_oe =  (g_adc_oe_t - 512);

	g_gain = (10000 + g_ge);

	format_1   = (g_o_vtsmcu1 + 3350 - g_oe);
	format_2   = (g_o_vtsmcu2 + 3350 - g_oe);
	format_abb = (g_o_vtsabb  + 3350 - g_oe);

	g_x_roomt1   = (((format_1   * 10000) / 4096) * 10000) / g_gain;
	g_x_roomt2   = (((format_2   * 10000) / 4096) * 10000) / g_gain;
	g_x_roomtabb = (((format_abb * 10000) / 4096) * 10000) / g_gain;

	pr_debug("[Power/CPU_Thermal] [Thermal calibration] g_ge       = 0x%x\n", g_ge);
	pr_debug("[Power/CPU_Thermal] [Thermal calibration] g_gain     = 0x%x\n", g_gain);
	pr_debug("[Power/CPU_Thermal] [Thermal calibration] g_gain     = 0x%x\n", g_gain);
	pr_debug("[Power/CPU_Thermal] [Thermal calibration] g_x_roomt1 = 0x%x\n", g_x_roomt1);
	pr_debug("[Power/CPU_Thermal] [Thermal calibration] g_x_roomt2 = 0x%x\n", g_x_roomt2);
}

static kal_int32 temperature_to_raw_abb(kal_uint32 ret)
{

	kal_int32 t_curr = ret;
	/* kal_int32 y_curr = 0; */
	kal_int32 format_1 = 0;
	kal_int32 format_2 = 0;
	kal_int32 format_3 = 0;
	kal_int32 format_4 = 0;

	pr_debug("[temperature_to_raw_abb]\n");

	if (g_o_slope_sign == 0) {
		format_1 = t_curr-(g_degc_cali*1000/2);
		format_2 = format_1 * (165+g_o_slope) * 18/15;
		format_2 = format_2 - 2*format_2;
		format_3 = format_2/1000 + g_x_roomt1*10;
		format_4 = (format_3*4096/10000*g_gain)/100000 + g_oe;
	} else {
		format_1 = t_curr-(g_degc_cali*1000/2);
		format_2 = format_1 * (165-g_o_slope) * 18/15;
		format_2 = format_2 - 2*format_2;
		format_3 = format_2/1000 + g_x_roomt1*10;
		format_4 = (format_3*4096/10000*g_gain)/100000 + g_oe;
	}

	pr_debug("[temperature_to_raw_abb] temperature=%d, raw=%d", ret, format_4);
	return format_4;
}

static kal_int32 raw_to_temperature_MCU1(kal_uint32 ret)
{
	S32 t_current = 0;
	S32 y_curr = ret;
	S32 format_1 = 0;
	S32 format_2 = 0;
	S32 format_3 = 0;
	S32 format_4 = 0;

	pr_debug("raw_to_temperature_MCU1\n");

	if (ret == 0)
		return 0;

	format_1 = (g_degc_cali*10 / 2);
	format_2 = (y_curr - g_oe);
	format_3 = (((((format_2) * 10000) / 4096) * 10000) / g_gain) - g_x_roomt1;
	format_3 = format_3 * 15/18;

	if (g_o_slope_sign == 0)
		format_4 = ((format_3 * 100) / (165+g_o_slope)); /* uint = 0.1 deg */
	else
		format_4 = ((format_3 * 100) / (165-g_o_slope)); /* uint = 0.1 deg */

	format_4 = format_4 - (2 * format_4);
	t_current = format_1 + format_4; /* uint = 0.1 deg */

	return t_current;
}

static kal_int32 raw_to_temperature_MCU2(kal_uint32 ret)
{
	S32 t_current = 0;
	S32 y_curr = ret;
	S32 format_1 = 0;
	S32 format_2 = 0;
	S32 format_3 = 0;
	S32 format_4 = 0;

	pr_debug("raw_to_temperature_MCU2\n");
	if (ret == 0)
		return 0;

	format_1 = (g_degc_cali*10 / 2);
	format_2 = (y_curr - g_oe);
	format_3 = (((((format_2) * 10000) / 4096) * 10000) / g_gain) - g_x_roomt2;
	format_3 = format_3 * 15/18;

	if (g_o_slope_sign == 0)
		format_4 = ((format_3 * 100) / (165+g_o_slope)); /* uint = 0.1 deg */
	else
		format_4 = ((format_3 * 100) / (165-g_o_slope)); /* uint = 0.1 deg */
	format_4 = format_4 - (2 * format_4);

	t_current = format_1 + format_4; /* uint = 0.1 deg */

	return t_current;
}



static void thermal_calibration(void)
{
	if (g_adc_cali_en_t == 0)
		pr_debug("#####  Not Calibration  ######\n");

	pr_debug("thermal_calibration\n");
	thermal_cal_prepare_2(0);
}


static int get_immediate_temp1(void)
{
	int curr_raw1, curr_temp1;

	mutex_lock(&TS_lock);
	curr_raw1 = DRV_Reg32(TEMPMSR0);
	curr_raw1 = curr_raw1 & 0x0fff; /* bit0~bit11 */
	curr_temp1 = raw_to_temperature_MCU1(curr_raw1);
	curr_temp1 = curr_temp1*100;
	mutex_unlock(&TS_lock);
	/* pr_debug("[get_immediate_temp1] temp1=%d, raw1=%d\n", curr_temp1, curr_raw1); */
	return curr_temp1;
}

static int get_immediate_temp2(void)
{
	int curr_raw2, curr_temp2;

	mutex_lock(&TS_lock);
	curr_raw2 = DRV_Reg32(TEMPMSR1);
	curr_raw2 = curr_raw2 & 0x0fff; /* bit0~bit11 */
	curr_temp2 = raw_to_temperature_MCU2(curr_raw2);
	curr_temp2 = curr_temp2*100;
	mutex_unlock(&TS_lock);
	/* pr_debug("[get_immediate_temp2] temp2=%d, raw2=%d\n", curr_temp2, curr_raw2); */
	return curr_temp2;
}

int get_immediate_temp2_wrap(void)
{
	int curr_raw;

	curr_raw = get_immediate_temp2();
	pr_debug("[get_immediate_temp2_wrap] curr_raw=%d\n", curr_raw);
	return curr_raw;
}

int CPU_Temp(void)
{
	int curr_temp, curr_temp2;

	curr_temp = get_immediate_temp1();
	curr_temp2 = get_immediate_temp2();

#if CONFIG_SUPPORT_MET_MTKTSCPU
	if (met_mtktscpu_dbg)
		trace_printk("%d,%d\n", curr_temp, curr_temp2);
#endif
	return curr_temp;
}

static int tscpu_get_temp(struct thermal_zone_device *thermal, int *t)
{
#if MTK_TS_CPU_SW_FILTER == 1
	int ret = 0;
	int curr_temp;
	int temp_temp;

	curr_temp = CPU_Temp();

	pr_debug(" mtktscpu_get_temp  CPU T1=%d\n", curr_temp);

	if ((curr_temp > (trip_temp[0] - 15000)) || (curr_temp < -30000))
		pr_err("[Power/CPU_Thermal] CPU T=%d\n", curr_temp);

	temp_temp = curr_temp;
	/* not resumed from suspensio */
	if (curr_temp != 0) {
		/* invalid range */
		if ((curr_temp > 150000) || (curr_temp < -20000)) {
			pr_err("[Power/CPU_Thermal] CPU temp invalid=%d\n", curr_temp);
			temp_temp = 50000;
			ret = -1;
		} else if (last_cpu_real_temp != 0) {
			if ((curr_temp - last_cpu_real_temp > 20000) || ((last_cpu_real_temp - curr_temp) > 20000)) {
				pr_err("[Power/CPU_Thermal] CPU temp float hugely temp=%d, lasttemp=%d\n",
					curr_temp, last_cpu_real_temp);
				temp_temp = 50000;
				ret = -1;
			}
		}
	}

	last_cpu_real_temp = curr_temp;
	curr_temp = temp_temp;
#else
	int ret = 0;
	int curr_temp;

	curr_temp = CPU_Temp();

	pr_debug(" mtktscpu_get_temp CPU T1=%d\n", curr_temp);

	if ((curr_temp > (trip_temp[0] - 15000)) || (curr_temp < -30000))
		pr_err("[Power/CPU_Thermal] CPU T=%d\n", curr_temp);
#endif
	read_curr_temp = curr_temp;
	*t = (unsigned long) curr_temp;
#if CPT_ADAPTIVE_AP_COOLER
	g_prev_temp = g_curr_temp;
	g_curr_temp = curr_temp;
#endif
	return ret;
}

int tscpu_get_cpu_temp(void)
{
	int curr_temp;

	curr_temp = get_immediate_temp1();

	pr_debug(" mtktscpu_get_cpu_temp  CPU T1=%d\n", curr_temp);

	if ((curr_temp > (trip_temp[0] - 15000)) || (curr_temp < -30000))
		pr_err("[Power/CPU_Thermal] CPU T=%d\n", curr_temp);

	return (unsigned long) curr_temp;
}

static int tscpu_bind(struct thermal_zone_device *thermal, struct thermal_cooling_device *cdev)
{
	int table_val = 0;

	if (!strcmp(cdev->type, g_bind0)) {
		table_val = 0;
		set_thermal_ctrl_trigger_SPM(trip_temp[0]);

		pr_debug("tscpu_bind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind1)) {
		table_val = 1;
		tc_mid_trip = trip_temp[1];

		pr_debug("tscpu_bind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind2)) {
		table_val = 2;
		pr_debug("tscpu_bind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind3)) {
		table_val = 3;
		pr_debug("tscpu_bind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind4)) {
		table_val = 4;
		pr_debug("tscpu_bind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind5)) {
		table_val = 5;
		pr_debug("tscpu_bind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind6)) {
		table_val = 6;
		pr_debug("tscpu_bind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind7)) {
		table_val = 7;
		pr_debug("tscpu_bind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind8)) {
		table_val = 8;
		pr_debug("tscpu_bind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind9)) {
		table_val = 9;
		pr_debug("tscpu_bind %s\n", cdev->type);
	} else {
		return 0;
	}

	if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) {
		pr_debug("tscpu_bind error binding cooling dev\n");
		return -EINVAL;
	}
	pr_debug("tscpu_bind binding OK, %d\n", table_val);

	return 0;
}

static int tscpu_unbind(struct thermal_zone_device *thermal, struct thermal_cooling_device *cdev)
{
	int table_val = 0;

	if (!strcmp(cdev->type, g_bind0)) {
		table_val = 0;
		pr_debug("tscpu_unbind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind1)) {
		table_val = 1;
		tc_mid_trip = -275000;
		pr_debug("tscpu_unbind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind2)) {
		table_val = 2;
		pr_debug("tscpu_unbind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind3)) {
		table_val = 3;
		pr_debug("tscpu_unbind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind4)) {
		table_val = 4;
		pr_debug("tscpu_unbind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind5)) {
		table_val = 5;
		pr_debug("tscpu_unbind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind6)) {
		table_val = 6;
		pr_debug("tscpu_unbind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind7)) {
		table_val = 7;
		pr_debug("tscpu_unbind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind8)) {
		table_val = 8;
		pr_debug("tscpu_unbind %s\n", cdev->type);
	} else if (!strcmp(cdev->type, g_bind9)) {
		table_val = 9;
		pr_debug("tscpu_unbind %s\n", cdev->type);
	} else
		return 0;

	if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) {
		pr_debug("tscpu_unbind error unbinding cooling dev\n");
		return -EINVAL;
	}
	pr_debug("tscpu_unbind unbinding OK\n");

	return 0;
}

static int tscpu_get_mode(struct thermal_zone_device *thermal, enum thermal_device_mode *mode)
{
	*mode = (kernelmode) ? THERMAL_DEVICE_ENABLED : THERMAL_DEVICE_DISABLED;
	return 0;
}

static int tscpu_set_mode(struct thermal_zone_device *thermal, enum thermal_device_mode mode)
{
	kernelmode = mode;
	return 0;
}

static int tscpu_get_trip_type(struct thermal_zone_device *thermal, int trip,
			       enum thermal_trip_type *type)
{
	*type = g_THERMAL_TRIP[trip];
	return 0;
}

static int tscpu_get_trip_temp(struct thermal_zone_device *thermal, int trip, int *temp)
{
	*temp = trip_temp[trip];
	return 0;
}

static int tscpu_get_crit_temp(struct thermal_zone_device *thermal, int *temperature)
{
	*temperature = MTKTSCPU_TEMP_CRIT;
	return 0;
}

/* bind callback functions to thermalzone */
static struct thermal_zone_device_ops mtktscpu_dev_ops = {
	.bind = tscpu_bind,
	.unbind = tscpu_unbind,
	.get_temp = tscpu_get_temp,
	.get_mode = tscpu_get_mode,
	.set_mode = tscpu_set_mode,
	.get_trip_type = tscpu_get_trip_type,
	.get_trip_temp = tscpu_get_trip_temp,
	.get_crit_temp = tscpu_get_crit_temp,
};

#if CPT_ADAPTIVE_AP_COOLER
static void set_adaptive_cpu_power_limit(unsigned int limit)
{
	unsigned int final_limit;

	adaptive_cpu_power_limit = (limit != 0) ? limit : 0x7FFFFFFF;
	final_limit = MIN(adaptive_cpu_power_limit, static_cpu_power_limit);
	mt_cpufreq_thermal_protect((final_limit != 0x7FFFFFFF) ? final_limit : 0);
}
#endif

static void set_static_cpu_power_limit(unsigned int limit)
{
	unsigned int final_limit;

	adaptive_cpu_power_limit = (limit != 0) ? limit : 0x7FFFFFFF;
	final_limit = MIN(adaptive_cpu_power_limit, static_cpu_power_limit);
	mt_cpufreq_thermal_protect((final_limit != 0x7FFFFFFF) ? final_limit : 0);
}

static int tscpu_set_power_consumption_state(void)
{
	int i = 0;
	int power = 0;

	pr_debug("tscpu_set_power_consumption_state Num_of_OPP=%d\n", Num_of_OPP);

	/* in 92, Num_of_OPP=34 */
	for (i = 0; i < Num_of_OPP; i++) {
		if (cl_dev_state[i] == 1) {
			if (i != previous_step) {
				pr_debug("previous_opp=%d, now_opp=%d\n", previous_step, i);
				previous_step = i;
				mtktscpu_limited_dmips = tscpu_cpu_dmips[previous_step];

				/* Add error-checking */
				if (!mtk_gpu_power) {
					pr_err("%s GPU POWER NOT READY!!", __func__);
					/* GPU freq = 396500, power = 568 */
					power = (i * 100) + 700 - 568;
					pr_err("%s cpu_power=%d\n", __func__, power);
					set_static_cpu_power_limit(power);
					return -ENOMEM;
				}

				if (Num_of_GPU_OPP == 3) {
					power =
					    (i * 100 + 700) - mtk_gpu_power[Num_of_GPU_OPP -
									    1].gpufreq_power;
					set_static_cpu_power_limit(power);
					pr_debug
					    ("Num_of_GPU_OPP=%d, gpufreq_power=%d, power=%d\n",
					     Num_of_GPU_OPP,
					     mtk_gpu_power[Num_of_GPU_OPP - 1].gpufreq_power,
					     power);
				} else if (Num_of_GPU_OPP == 2) {
					power = (i * 100 + 700) - mtk_gpu_power[1].gpufreq_power;
					set_static_cpu_power_limit(power);
					pr_debug
					    ("Num_of_GPU_OPP=%d, gpufreq_power=%d, power=%d\n",
					     Num_of_GPU_OPP, mtk_gpu_power[1].gpufreq_power, power);
				} else if (Num_of_GPU_OPP == 1) {
#if 0
					/* 653mW,GPU 500Mhz,1V(preloader default) */
					/* 1016mW,GPU 700Mhz,1.1V */
					power = (i * 100 + 700) - 653;
#else
					power = (i * 100 + 700) - mtk_gpu_power[0].gpufreq_power;
#endif
					set_static_cpu_power_limit(power);
					pr_debug
					    ("Num_of_GPU_OPP=%d, gpufreq_power=%d, power=%d\n",
					     Num_of_GPU_OPP, mtk_gpu_power[0].gpufreq_power, power);
				} else {	/* TODO: fix this, temp solution, ROME has over 5 GPU OPP... */

					power = (i * 100 + 700);
					set_static_cpu_power_limit(power);
					pr_debug
					    ("Num_of_GPU_OPP=%d, gpufreq_power=%d, power=%d\n",
					     Num_of_GPU_OPP, mtk_gpu_power[0].gpufreq_power, power);
				}
			}
			break;
		}
	}

	/* If temp drop to our expect value, we need to restore initial cpu freq setting */
	if (i == Num_of_OPP) {
		if (previous_step != -1) {
			previous_step = -1;
			mtktscpu_limited_dmips = tscpu_cpu_dmips[CPU_COOLER_NUM - 1];	/* highest dmips */
			pr_debug("Free all static thermal limit, previous_opp=%d\n", previous_step);
			set_static_cpu_power_limit(0);
		}
	}
	return 0;
}

#if CPT_ADAPTIVE_AP_COOLER

static int GPU_L_H_TRIP = 80, GPU_L_L_TRIP = 40;

static int TARGET_TJ = 65000;
static int TARGET_TJ_HIGH = 66000;
static int TARGET_TJ_LOW = 64000;
static int PACKAGE_THETA_JA_RISE = 10;
static int PACKAGE_THETA_JA_FALL = 10;
static int MINIMUM_CPU_POWER = 500;
static int MAXIMUM_CPU_POWER = 1240;
static int MINIMUM_GPU_POWER = 676;
static int MAXIMUM_GPU_POWER = 676;
static int MINIMUM_TOTAL_POWER = 500 + 676;
static int MAXIMUM_TOTAL_POWER = 1240 + 676;
static int FIRST_STEP_TOTAL_POWER_BUDGET = 1750;

/* 1. MINIMUM_BUDGET_CHANGE = 0 ==> thermal equilibrium maybe at higher than TARGET_TJ_HIGH */
/* 2. Set MINIMUM_BUDGET_CHANGE > 0 if to keep Tj at TARGET_TJ */
static int MINIMUM_BUDGET_CHANGE = 50;

static int P_adaptive(int total_power, unsigned int gpu_loading)
{
	/* But the ground rule is real gpu power should always under gpu_power for the same time interval */
	static int cpu_power = 0, gpu_power;
	static int last_cpu_power = 0, last_gpu_power;

	last_cpu_power = cpu_power;
	last_gpu_power = gpu_power;

	if (total_power == 0) {
		cpu_power = gpu_power = 0;
		set_adaptive_gpu_power_limit(0);
		return 0;
	}

	if (total_power <= MINIMUM_TOTAL_POWER) {
		cpu_power = MINIMUM_CPU_POWER;
		gpu_power = MINIMUM_GPU_POWER;
	} else if (total_power >= MAXIMUM_TOTAL_POWER) {
		cpu_power = MAXIMUM_CPU_POWER;
		gpu_power = MAXIMUM_GPU_POWER;
	} else {
		int max_allowed_gpu_power =
		    MIN((total_power - MINIMUM_CPU_POWER), MAXIMUM_GPU_POWER);
		int highest_possible_gpu_power = -1;
		/* int highest_possible_gpu_power_idx = 0; */
		int i = 0;
		unsigned int cur_gpu_freq = mt_gpufreq_get_cur_freq();
		/* int cur_idx = 0; */
		unsigned int cur_gpu_power = 0;
		unsigned int next_lower_gpu_power = 0;

		/* get GPU highest possible power and index and current power and index and next lower power */
		for (; i < Num_of_GPU_OPP; i++) {
			if ((mtk_gpu_power[i].gpufreq_power <= max_allowed_gpu_power) &&
			    (-1 == highest_possible_gpu_power)) {
				highest_possible_gpu_power = mtk_gpu_power[i].gpufreq_power;
				/* highest_possible_gpu_power_idx = i; */
			}

			if (mtk_gpu_power[i].gpufreq_khz == cur_gpu_freq) {
				next_lower_gpu_power = cur_gpu_power =
				    mtk_gpu_power[i].gpufreq_power;
				/* cur_idx = i; */

				if ((i != Num_of_GPU_OPP - 1)
				    && (mtk_gpu_power[i + 1].gpufreq_power >= MINIMUM_GPU_POWER)) {
					next_lower_gpu_power = mtk_gpu_power[i + 1].gpufreq_power;
				}
			}
		}

		/* decide GPU power limit by loading */
		if (gpu_loading > GPU_L_H_TRIP) {
			gpu_power = highest_possible_gpu_power;
		} else if (gpu_loading <= GPU_L_L_TRIP) {
			gpu_power = MIN(next_lower_gpu_power, highest_possible_gpu_power);
			gpu_power = MAX(gpu_power, MINIMUM_GPU_POWER);
		} else {
			gpu_power = MIN(highest_possible_gpu_power, cur_gpu_power);
		}

		cpu_power = MIN((total_power - gpu_power), MAXIMUM_CPU_POWER);
	}

	if (cpu_power != last_cpu_power)
		set_adaptive_cpu_power_limit(cpu_power);

	if (gpu_power != last_gpu_power)
		set_adaptive_gpu_power_limit(gpu_power);

	pr_debug("%s cpu %d, gpu %d\n", __func__, cpu_power, gpu_power);

	return 0;
}

static int _adaptive_power(long prev_temp, long curr_temp, unsigned int gpu_loading)
{
	static int triggered = 0, total_power;
	int delta_power = 0;

	if (cl_dev_adp_cpu_state_active == 1) {

		/* Check if it is triggered */
		if (!triggered) {
			if (curr_temp < TARGET_TJ)
				return 0;
			if (curr_temp >= TARGET_TJ) {
				triggered = 1;
				switch (mtktscpu_atm) {
				case 1:	/* FTL ATM v2 */
				case 2:	/* CPU_GPU_Weight ATM v2 */
					total_power =
					    FIRST_STEP_TOTAL_POWER_BUDGET -
					    ((curr_temp - TARGET_TJ) * tt_ratio_high_rise +
					     (curr_temp -
					      prev_temp) * tp_ratio_high_rise) /
					    PACKAGE_THETA_JA_RISE;
					break;
				case 0:
				default:	/* ATM v1 */
					total_power = FIRST_STEP_TOTAL_POWER_BUDGET;
				}
				return P_adaptive(total_power, gpu_loading);
			}
		}

		/* Adjust total power budget if necessary */
		switch (mtktscpu_atm) {
		case 1:	/* FTL ATM v2 */
		case 2:	/* CPU_GPU_Weight ATM v2 */
			if ((curr_temp >= TARGET_TJ_HIGH) && (curr_temp > prev_temp)) {
				total_power -=
				    MAX(((curr_temp - TARGET_TJ) * tt_ratio_high_rise +
					 (curr_temp -
					  prev_temp) * tp_ratio_high_rise) / PACKAGE_THETA_JA_RISE,
					MINIMUM_BUDGET_CHANGE);
			} else if ((curr_temp >= TARGET_TJ_HIGH) && (curr_temp <= prev_temp)) {
				total_power -=
				    MAX(((curr_temp - TARGET_TJ) * tt_ratio_high_fall -
					 (prev_temp -
					  curr_temp) * tp_ratio_high_fall) / PACKAGE_THETA_JA_FALL,
					MINIMUM_BUDGET_CHANGE);
			} else if ((curr_temp <= TARGET_TJ_LOW) && (curr_temp > prev_temp)) {
				total_power +=
				    MAX(((TARGET_TJ - curr_temp) * tt_ratio_low_rise -
					 (curr_temp -
					  prev_temp) * tp_ratio_low_rise) / PACKAGE_THETA_JA_RISE,
					MINIMUM_BUDGET_CHANGE);
			} else if ((curr_temp <= TARGET_TJ_LOW) && (curr_temp <= prev_temp)) {
				total_power +=
				    MAX(((TARGET_TJ - curr_temp) * tt_ratio_low_fall +
					 (prev_temp -
					  curr_temp) * tp_ratio_low_fall) / PACKAGE_THETA_JA_FALL,
					MINIMUM_BUDGET_CHANGE);
			}

			total_power =
			    (total_power > MINIMUM_TOTAL_POWER) ? total_power : MINIMUM_TOTAL_POWER;
			total_power =
			    (total_power < MAXIMUM_TOTAL_POWER) ? total_power : MAXIMUM_TOTAL_POWER;
			break;

		case 0:
		default:	/* ATM v1 */
			if ((curr_temp > TARGET_TJ_HIGH) && (curr_temp >= prev_temp)) {
				delta_power = (curr_temp - prev_temp) / PACKAGE_THETA_JA_RISE;
				if (prev_temp > TARGET_TJ_HIGH) {
					delta_power =
					    (delta_power >
					     MINIMUM_BUDGET_CHANGE) ? delta_power :
					    MINIMUM_BUDGET_CHANGE;
				}
				total_power -= delta_power;
				total_power =
				    (total_power >
				     MINIMUM_TOTAL_POWER) ? total_power : MINIMUM_TOTAL_POWER;
			}

			if ((curr_temp < TARGET_TJ_LOW) && (curr_temp <= prev_temp)) {
				delta_power = (prev_temp - curr_temp) / PACKAGE_THETA_JA_FALL;
				if (prev_temp < TARGET_TJ_LOW) {
					delta_power =
					    (delta_power >
					     MINIMUM_BUDGET_CHANGE) ? delta_power :
					    MINIMUM_BUDGET_CHANGE;
				}
				total_power += delta_power;
				total_power =
				    (total_power <
				     MAXIMUM_TOTAL_POWER) ? total_power : MAXIMUM_TOTAL_POWER;
			}
			break;
		}
		pr_debug("%s Tp %ld, Tc %ld, Pt %d\n", __func__, prev_temp,
						curr_temp, total_power);
		return P_adaptive(total_power, gpu_loading);
	}
	if (cl_dev_adp_cpu_state_active != 1) {
		if (triggered) {
			triggered = 0;
			pr_debug("%s Tp %ld, Tc %ld, Pt %d\n", __func__,
				prev_temp, curr_temp, total_power);
			return P_adaptive(0, 0);
		}
	}

	return 0;
}

static int decide_ttj(void)
{
	int i = 0;
	int active_cooler_id = -1;
	int ret = 117000;	/* highest allowable TJ */
	int temp_cl_dev_adp_cpu_state_active = 0;

	for (; i < MAX_CPT_ADAPTIVE_COOLERS; i++) {
		if (cl_dev_adp_cpu_state[i]) {
			ret = MIN(ret, TARGET_TJS[i]);
			temp_cl_dev_adp_cpu_state_active = 1;

			if (ret == TARGET_TJS[i])
				active_cooler_id = i;
		}
	}
	cl_dev_adp_cpu_state_active = temp_cl_dev_adp_cpu_state_active;
	TARGET_TJ = ret;
	TARGET_TJ_HIGH = TARGET_TJ + 1000;
	TARGET_TJ_LOW = TARGET_TJ - 1000;

	if (0 <= active_cooler_id && MAX_CPT_ADAPTIVE_COOLERS > active_cooler_id) {
		PACKAGE_THETA_JA_RISE = PACKAGE_THETA_JA_RISES[active_cooler_id];
		PACKAGE_THETA_JA_FALL = PACKAGE_THETA_JA_FALLS[active_cooler_id];
		MINIMUM_CPU_POWER = MINIMUM_CPU_POWERS[active_cooler_id];
		MAXIMUM_CPU_POWER = MAXIMUM_CPU_POWERS[active_cooler_id];
		MINIMUM_GPU_POWER = MINIMUM_GPU_POWERS[active_cooler_id];
		MAXIMUM_GPU_POWER = MAXIMUM_GPU_POWERS[active_cooler_id];
		MINIMUM_TOTAL_POWER = MINIMUM_CPU_POWER + MINIMUM_GPU_POWER;
		MAXIMUM_TOTAL_POWER = MAXIMUM_CPU_POWER + MAXIMUM_GPU_POWER;
		FIRST_STEP_TOTAL_POWER_BUDGET = FIRST_STEP_TOTAL_POWER_BUDGETS[active_cooler_id];
		MINIMUM_BUDGET_CHANGE = MINIMUM_BUDGET_CHANGES[active_cooler_id];
	} else {
		MINIMUM_CPU_POWER = MINIMUM_CPU_POWERS[0];
		MAXIMUM_CPU_POWER = MAXIMUM_CPU_POWERS[0];
	}
	return ret;
}
#endif

static int dtm_cpu_get_max_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	/* pr_debug("dtm_cpu_get_max_state\n"); */
	*state = 1;
	return 0;
}

static int dtm_cpu_get_cur_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	int i = 0;
	/* pr_debug("dtm_cpu_get_cur_state %s\n", cdev->type); */

	for (i = 0; i < Num_of_OPP; i++) {
		if (!strcmp(cdev->type, &cooler_name[i * 20]))
			*state = cl_dev_state[i];
	}
	return 0;
}

static int dtm_cpu_set_cur_state(struct thermal_cooling_device *cdev, unsigned long state)
{
	int i = 0;
	/* pr_debug("dtm_cpu_set_cur_state %s\n", cdev->type); */

	for (i = 0; i < Num_of_OPP; i++) {
		if (!strcmp(cdev->type, &cooler_name[i * 20])) {
			cl_dev_state[i] = state;
			tscpu_set_power_consumption_state();
			break;
		}
	}
	return 0;
}

/*
 * cooling device callback functions (tscpu_cooling_sysrst_ops)
 * 1 : ON and 0 : OFF
 */
static int sysrst_cpu_get_max_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	/* pr_debug("sysrst_cpu_get_max_state\n"); */
	*state = 1;
	return 0;
}

static int sysrst_cpu_get_cur_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	/* pr_debug("sysrst_cpu_get_cur_state\n"); */
	*state = cl_dev_sysrst_state;
	return 0;
}

static int sysrst_cpu_set_cur_state(struct thermal_cooling_device *cdev, unsigned long state)
{
	cl_dev_sysrst_state = state;

	if (cl_dev_sysrst_state == 1) {
		mtkts_dump_cali_info();
		pr_err("sysrst_cpu_set_cur_state = 1\n");
		pr_err("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
		pr_err("*****************************************\n");
		pr_err("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

		*(unsigned int *)0x0 = 0xdead;	/* To trigger data abort to reset the system for thermal protection. */
	}
	return 0;
}

#if CPT_ADAPTIVE_AP_COOLER
static int adp_cpu_get_max_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	/* pr_debug("adp_cpu_get_max_state\n"); */
	*state = 1;
	return 0;
}

static int adp_cpu_get_cur_state(struct thermal_cooling_device *cdev, unsigned long *state)
{
	/* pr_debug("adp_cpu_get_cur_state\n"); */
	*state = cl_dev_adp_cpu_state[(cdev->type[13] - '0')];
	/* *state = cl_dev_adp_cpu_state; */
	return 0;
}

static int adp_cpu_set_cur_state(struct thermal_cooling_device *cdev, unsigned long state)
{
	int ttj = 117000;

	cl_dev_adp_cpu_state[(cdev->type[13] - '0')] = state;

	ttj = decide_ttj();	/* TODO: no exit point can be obtained in mtk_ts_cpu.c */

	/* pr_debug("adp_cpu_set_cur_state[%d] =%d, ttj=%d\n", (cdev->type[13] - '0'), state, ttj); */

	if (active_adp_cooler == (int)(cdev->type[13] - '0')) {
		unsigned int gpu_loading;
		/* if (!mtk_get_gpu_loading(&gpu_loading)) */
		gpu_loading = 0;
		_adaptive_power(g_prev_temp, g_curr_temp, (unsigned int)gpu_loading);
		/* _adaptive_power(g_prev_temp, g_curr_temp, (unsigned int) 0); */
	}
	return 0;
}
#endif

/* bind fan callbacks to fan device */

static struct thermal_cooling_device_ops mtktscpu_cooling_F0x2_ops = {
	.get_max_state = dtm_cpu_get_max_state,
	.get_cur_state = dtm_cpu_get_cur_state,
	.set_cur_state = dtm_cpu_set_cur_state,
};

#if CPT_ADAPTIVE_AP_COOLER
static struct thermal_cooling_device_ops mtktscpu_cooler_adp_cpu_ops = {
	.get_max_state = adp_cpu_get_max_state,
	.get_cur_state = adp_cpu_get_cur_state,
	.set_cur_state = adp_cpu_set_cur_state,
};
#endif

static struct thermal_cooling_device_ops mtktscpu_cooling_sysrst_ops = {
	.get_max_state = sysrst_cpu_get_max_state,
	.get_cur_state = sysrst_cpu_get_cur_state,
	.set_cur_state = sysrst_cpu_set_cur_state,
};

static int tscpu_read_Tj_out(struct seq_file *m, void *v)
{

	int ts_con0 = 0;

	/*      TS_CON0[19:16] = 0x8: Tj sensor Analog signal output via HW pin */
	ts_con0 = DRV_Reg32(TS_CON0);

	seq_printf(m, "TS_CON0:0x%x\n", ts_con0);


	return 0;
}

static ssize_t tscpu_write_Tj_out(struct file *file, const char __user *buffer, size_t count,
				  loff_t *data)
{
	char desc[32];
	int lv_Tj_out_flag;
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (kstrtouint(desc, 16, &lv_Tj_out_flag) == 1) {
		if (lv_Tj_out_flag == 1) {
			/*      TS_CON0[19:16] = 0x8: Tj sensor Analog signal output via HW pin */
			THERMAL_WRAP_WR32(DRV_Reg32(TS_CON0) | 0x00010000, TS_CON0);
		} else {
			/*      TS_CON0[19:16] = 0x8: Tj sensor Analog signal output via HW pin */
			THERMAL_WRAP_WR32(DRV_Reg32(TS_CON0) & 0xfffeffff, TS_CON0);
		}

		pr_debug("tscpu_write_Tj_out lv_Tj_out_flag=%d\n", lv_Tj_out_flag);
		return count;
	}

	return -EINVAL;
}

static int tscpu_read_opp(struct seq_file *m, void *v)
{

	unsigned int cpu_power, gpu_power;

	cpu_power = MIN(adaptive_cpu_power_limit, static_cpu_power_limit);

	gpu_power = MIN(adaptive_gpu_power_limit, static_gpu_power_limit);

#if CPT_ADAPTIVE_AP_COOLER
	/* if (!mtk_get_gpu_loading(&gpu_loading)) */
	gpu_loading = 0;

	seq_printf(m, "%d,%d,%d,%d\n",
		   (int)((cpu_power != 0x7FFFFFFF) ? cpu_power : 0),
		   (int)((gpu_power != 0x7FFFFFFF) ? gpu_power : 0),
		   /* ((NULL == mtk_thermal_get_gpu_loading_fp) ? 0 : mtk_thermal_get_gpu_loading_fp()), */
		   (int)gpu_loading, (int) mt_gpufreq_get_cur_freq());

#else
	seq_printf(m, "%d,%d,0,%d\n", (int)((cpu_power != 0x7FFFFFFF) ? cpu_power : 0),
			(int)((gpu_power != 0x7FFFFFFF) ? gpu_power : 0),
			(int) mt_gpufreq_get_cur_freq());
#endif

	return 0;
}

static int tscpu_read_temperature_info(struct seq_file *m, void *v)
{
	seq_printf(m, "current temp:%d\n", read_curr_temp);
	seq_printf(m, "calefuse1:0x%x\n", calefuse1);
	seq_printf(m, "calefuse2:0x%x\n", calefuse2);
	seq_printf(m, "calefuse3:0x%x\n", calefuse3);
	seq_printf(m, "g_adc_ge_t:%d\n", g_adc_ge_t);
	seq_printf(m, "g_adc_oe_t:%d\n", g_adc_oe_t);
	seq_printf(m, "g_degc_cali:%d\n", g_degc_cali);
	seq_printf(m, "g_adc_cali_en_t:%d\n", g_adc_cali_en_t);
	seq_printf(m, "g_o_slope:%d\n", g_o_slope);
	seq_printf(m, "g_o_slope_sign:%d\n", g_o_slope_sign);
	seq_printf(m, "g_id:%d\n", g_id);
	seq_printf(m, "g_o_vtsmcu1:%d\n", g_o_vtsmcu1);
	seq_printf(m, "g_o_vtsmcu2:%d\n", g_o_vtsmcu2);
	seq_printf(m, "g_o_vtsmcu3:%d\n", g_o_vtsmcu3);
	seq_printf(m, "g_o_vtsmcu4:%d\n", g_o_vtsmcu4);
	seq_printf(m, "g_o_vtsabb :%d\n", g_o_vtsabb);

	return 0;
}

static int tscpu_set_temperature_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", temperature_switch);

	return 0;
}


static ssize_t tscpu_set_temperature_write(struct file *file, const char __user *buffer,
					   size_t count, loff_t *data)
{
	char desc[32];
	int lv_tempe_switch;
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	pr_debug("tscpu_set_temperature_write\n");

	if (kstrtouint(desc, 16, &lv_tempe_switch) == 1) {
		temperature_switch = lv_tempe_switch;

		pr_debug("tscpu_set_temperature_write temperature_switch=%d\n", temperature_switch);
		return count;
	}

	return -EINVAL;
}

static int tscpu_read_cal(struct seq_file *m, void *v)
{
	seq_printf(m, "mtktscpu cal:\n devinfo index(7)=0x%x, devinfo index(8)=0x%x, devinfo index(8)=0x%x\n",
		get_devinfo_with_index(7), get_devinfo_with_index(8), get_devinfo_with_index(9));
	return 0;
}

static int tscpu_read(struct seq_file *m, void *v)
{
	int i;

	seq_printf(m,
		   "[tscpu_read]%d\ntrip_0=%d %d %s\ntrip_1=%d %d %s\ntrip_2=%d %d %s\ntrip_3=%d %d %s\ntrip_4=%d %d %s\ntrip_5=%d %d %s\ntrip_6=%d %d %s\ntrip_7=%d %d %s\ntrip_8=%d %d %s\ntrip_9=%d %d %s\ninterval=%d\n",
		   num_trip,
		   trip_temp[0], g_THERMAL_TRIP[0], g_bind0,
		   trip_temp[1], g_THERMAL_TRIP[1], g_bind1,
		   trip_temp[2], g_THERMAL_TRIP[2], g_bind2,
		   trip_temp[3], g_THERMAL_TRIP[3], g_bind3,
		   trip_temp[4], g_THERMAL_TRIP[4], g_bind4,
		   trip_temp[5], g_THERMAL_TRIP[5], g_bind5,
		   trip_temp[6], g_THERMAL_TRIP[6], g_bind6,
		   trip_temp[7], g_THERMAL_TRIP[7], g_bind7,
		   trip_temp[8], g_THERMAL_TRIP[8], g_bind8,
		   trip_temp[9], g_THERMAL_TRIP[9], g_bind9, interval);

	for (i = 0; i < Num_of_GPU_OPP; i++)
		seq_printf(m, "g %d %d %d\n", i, mtk_gpu_power[i].gpufreq_khz,
			   mtk_gpu_power[i].gpufreq_power);

	for (i = 0; i < tscpu_num_opp; i++)
		seq_printf(m, "c %d %d %d %d\n", i, mtk_cpu_power[i].cpufreq_khz,
			   mtk_cpu_power[i].cpufreq_ncpu, mtk_cpu_power[i].cpufreq_power);

	for (i = 0; i < CPU_COOLER_NUM; i++)
		seq_printf(m, "d %d %d\n", i, tscpu_cpu_dmips[i]);

	return 0;
}

#if CPT_ADAPTIVE_AP_COOLER

static int tscpu_read_atm(struct seq_file *m, void *v)
{

	seq_printf(m, "[tscpu_read_atm] ver = %d\n", mtktscpu_atm);
	seq_printf(m, "tt_ratio_high_rise = %d\n", tt_ratio_high_rise);
	seq_printf(m, "tt_ratio_high_fall = %d\n", tt_ratio_high_fall);
	seq_printf(m, "tt_ratio_low_rise = %d\n", tt_ratio_low_rise);
	seq_printf(m, "tt_ratio_low_fall = %d\n", tt_ratio_low_fall);
	seq_printf(m, "tp_ratio_high_rise = %d\n", tp_ratio_high_rise);
	seq_printf(m, "tp_ratio_high_fall = %d\n", tp_ratio_high_fall);
	seq_printf(m, "tp_ratio_low_rise = %d\n", tp_ratio_low_rise);
	seq_printf(m, "tp_ratio_low_fall = %d\n", tp_ratio_low_fall);

	return 0;
}

static ssize_t tscpu_write_atm(struct file *file, const char __user *buffer, size_t count,
			       loff_t *data)
{
	char desc[128];
	int atm_ver;
	int tmp_tt_ratio_high_rise;
	int tmp_tt_ratio_high_fall;
	int tmp_tt_ratio_low_rise;
	int tmp_tt_ratio_low_fall;
	int tmp_tp_ratio_high_rise;
	int tmp_tp_ratio_high_fall;
	int tmp_tp_ratio_low_rise;
	int tmp_tp_ratio_low_fall;
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%d %d %d %d %d %d %d %d %d ",
		   &atm_ver, &tmp_tt_ratio_high_rise, &tmp_tt_ratio_high_fall,
		   &tmp_tt_ratio_low_rise, &tmp_tt_ratio_low_fall, &tmp_tp_ratio_high_rise,
		   &tmp_tp_ratio_high_fall, &tmp_tp_ratio_low_rise, &tmp_tp_ratio_low_fall) == 9) {
		mtktscpu_atm = atm_ver;
		tt_ratio_high_rise = tmp_tt_ratio_high_rise;
		tt_ratio_high_fall = tmp_tt_ratio_high_fall;
		tt_ratio_low_rise = tmp_tt_ratio_low_rise;
		tt_ratio_low_fall = tmp_tt_ratio_low_fall;
		tp_ratio_high_rise = tmp_tp_ratio_high_rise;
		tp_ratio_high_fall = tmp_tp_ratio_high_fall;
		tp_ratio_low_rise = tmp_tp_ratio_low_rise;
		tp_ratio_low_fall = tmp_tp_ratio_low_fall;
		return count;
	}

	return -EINVAL;
}

static int tscpu_read_dtm_setting(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < MAX_CPT_ADAPTIVE_COOLERS; i++) {
		seq_printf(m, "%s%02d\n", adaptive_cooler_name, i);
		seq_printf(m, " first_step = %d\n", FIRST_STEP_TOTAL_POWER_BUDGETS[i]);
		seq_printf(m, " theta rise = %d\n", PACKAGE_THETA_JA_RISES[i]);
		seq_printf(m, " theta fall = %d\n", PACKAGE_THETA_JA_FALLS[i]);
		seq_printf(m, " min_budget_change = %d\n", MINIMUM_BUDGET_CHANGES[i]);
		seq_printf(m, " m cpu = %d\n", MINIMUM_CPU_POWERS[i]);
		seq_printf(m, " M cpu = %d\n", MAXIMUM_CPU_POWERS[i]);
		seq_printf(m, " m gpu = %d\n", MINIMUM_GPU_POWERS[i]);
		seq_printf(m, " M gpu = %d\n", MAXIMUM_GPU_POWERS[i]);
	}

	return 0;
}

static ssize_t tscpu_write_dtm_setting(struct file *file, const char __user *buffer, size_t count,
				       loff_t *data)
{
	char desc[128];
	/* char arg_name[32] = {0}; */
	/* int arg_val = 0; */
	int len = 0;

	int i_id = -1, i_first_step = -1, i_theta_r = -1, i_theta_f = -1, i_budget_change =
	    -1, i_min_cpu_pwr = -1, i_max_cpu_pwr = -1, i_min_gpu_pwr = -1, i_max_gpu_pwr = -1;


	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%d %d %d %d %d %d %d %d %d", &i_id, &i_first_step, &i_theta_r, &i_theta_f,
		   &i_budget_change, &i_min_cpu_pwr, &i_max_cpu_pwr, &i_min_gpu_pwr,
		   &i_max_gpu_pwr) >= 9) {
		pr_debug("tscpu_write_dtm_setting input %d %d %d %d %d %d %d %d %d\n", i_id,
			 i_first_step, i_theta_r, i_theta_f, i_budget_change, i_min_cpu_pwr,
			 i_max_cpu_pwr, i_min_gpu_pwr, i_max_gpu_pwr);

		if (i_id >= 0 && i_id < MAX_CPT_ADAPTIVE_COOLERS) {
			if (i_first_step > 0)
				FIRST_STEP_TOTAL_POWER_BUDGETS[i_id] = i_first_step;
			if (i_theta_r > 0)
				PACKAGE_THETA_JA_RISES[i_id] = i_theta_r;
			if (i_theta_f > 0)
				PACKAGE_THETA_JA_FALLS[i_id] = i_theta_f;
			if (i_budget_change > 0)
				MINIMUM_BUDGET_CHANGES[i_id] = i_budget_change;
			if (i_min_cpu_pwr > 0)
				MINIMUM_CPU_POWERS[i_id] = i_min_cpu_pwr;
			if (i_max_cpu_pwr > 0)
				MAXIMUM_CPU_POWERS[i_id] = i_max_cpu_pwr;
			if (i_min_gpu_pwr > 0)
				MINIMUM_GPU_POWERS[i_id] = i_min_gpu_pwr;
			if (i_max_gpu_pwr > 0)
				MAXIMUM_GPU_POWERS[i_id] = i_max_gpu_pwr;

			active_adp_cooler = i_id;

			pr_debug("tscpu_write_dtm_setting applied %d %d %d %d %d %d %d %d %d\n",
				 i_id, FIRST_STEP_TOTAL_POWER_BUDGETS[i_id],
				 PACKAGE_THETA_JA_RISES[i_id], PACKAGE_THETA_JA_FALLS[i_id],
				 MINIMUM_BUDGET_CHANGES[i_id], MINIMUM_CPU_POWERS[i_id],
				 MAXIMUM_CPU_POWERS[i_id], MINIMUM_GPU_POWERS[i_id],
				 MAXIMUM_GPU_POWERS[i_id]);
		} else
			pr_debug("tscpu_write_dtm_setting out of range\n");

		/* MINIMUM_TOTAL_POWER = MINIMUM_CPU_POWER + MINIMUM_GPU_POWER; */
		/* MAXIMUM_TOTAL_POWER = MAXIMUM_CPU_POWER + MAXIMUM_GPU_POWER; */
		return count;
	}

	return -EINVAL;
}

static int tscpu_read_gpu_threshold(struct seq_file *m, void *v)
{
	seq_printf(m, "H %d L %d\n", GPU_L_H_TRIP, GPU_L_L_TRIP);

	return 0;
}


static ssize_t tscpu_write_gpu_threshold(struct file *file, const char __user *buffer,
					 size_t count, loff_t *data)
{
	char desc[128];
	int len = 0;
	int gpu_h = -1, gpu_l = -1;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%d %d", &gpu_h, &gpu_l) >= 2) {
		pr_debug("tscpu_write_gpu_threshold input %d %d\n", gpu_h, gpu_l);

		if ((gpu_h > 0) && (gpu_l > 0) && (gpu_h > gpu_l)) {
			GPU_L_H_TRIP = gpu_h;
			GPU_L_L_TRIP = gpu_l;

			pr_debug("tscpu_write_gpu_threshold applied %d %d\n", GPU_L_H_TRIP,
				 GPU_L_L_TRIP);
		} else {
			pr_debug("tscpu_write_gpu_threshold out of range\n");
		}

		return count;
	}
	return -EINVAL;
}
#endif

int tscpu_register_thermal(void)
{

	pr_debug("tscpu_register_thermal\n");

	/* trips : trip 0~3 */
	thz_dev = mtk_thermal_zone_device_register("mtktscpu", num_trip, NULL,
						   &mtktscpu_dev_ops, 0, 0, 0, interval);

	return 0;
}

void tscpu_unregister_thermal(void)
{

	pr_debug("tscpu_unregister_thermal\n");
	if (thz_dev) {
		mtk_thermal_zone_device_unregister(thz_dev);
		thz_dev = NULL;
	}
}

static ssize_t tscpu_write(struct file *file, const char __user *buffer, size_t count,
		loff_t *data)
{
	int len = 0, time_msec = 0;
	int trip[10] = { 0 };
	int t_type[10] = { 0 };
	int i;
	char bind0[20], bind1[20], bind2[20], bind3[20], bind4[20];
	char bind5[20], bind6[20], bind7[20], bind8[20], bind9[20];
	char desc[512];

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (sscanf
			(desc,
			 "%d %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d",
			 &num_trip, &trip[0], &t_type[0], bind0, &trip[1], &t_type[1], bind1, &trip[2],
			 &t_type[2], bind2, &trip[3], &t_type[3], bind3, &trip[4], &t_type[4], bind4, &trip[5],
			 &t_type[5], bind5, &trip[6], &t_type[6], bind6, &trip[7], &t_type[7], bind7, &trip[8],
			 &t_type[8], bind8, &trip[9], &t_type[9], bind9, &time_msec, &MA_len_temp) == 33) {

		pr_debug("tscpu_write tscpu_unregister_thermal MA_len_temp=%d\n", MA_len_temp);

		tscpu_unregister_thermal();


		for (i = 0; i < num_trip; i++)
			g_THERMAL_TRIP[i] = t_type[i];

		g_bind0[0] = g_bind1[0] = g_bind2[0] = g_bind3[0] = g_bind4[0] = g_bind5[0] =
			g_bind6[0] = g_bind7[0] = g_bind8[0] = g_bind9[0] = '\0';

		for (i = 0; i < 20; i++) {
			g_bind0[i] = bind0[i];
			g_bind1[i] = bind1[i];
			g_bind2[i] = bind2[i];
			g_bind3[i] = bind3[i];
			g_bind4[i] = bind4[i];
			g_bind5[i] = bind5[i];
			g_bind6[i] = bind6[i];
			g_bind7[i] = bind7[i];
			g_bind8[i] = bind8[i];
			g_bind9[i] = bind9[i];
		}

#if CPT_ADAPTIVE_AP_COOLER
		/* initialize... */
		for (i = 0; i < MAX_CPT_ADAPTIVE_COOLERS; i++)
			TARGET_TJS[i] = 117000;

		if (!strncmp(bind0, adaptive_cooler_name, 13))
			TARGET_TJS[(bind0[13] - '0')] = trip[0];

		if (!strncmp(bind1, adaptive_cooler_name, 13))
			TARGET_TJS[(bind1[13] - '0')] = trip[1];

		if (!strncmp(bind2, adaptive_cooler_name, 13))
			TARGET_TJS[(bind2[13] - '0')] = trip[2];

		if (!strncmp(bind3, adaptive_cooler_name, 13))
			TARGET_TJS[(bind3[13] - '0')] = trip[3];

		if (!strncmp(bind4, adaptive_cooler_name, 13))
			TARGET_TJS[(bind4[13] - '0')] = trip[4];

		if (!strncmp(bind5, adaptive_cooler_name, 13))
			TARGET_TJS[(bind5[13] - '0')] = trip[5];

		if (!strncmp(bind6, adaptive_cooler_name, 13))
			TARGET_TJS[(bind6[13] - '0')] = trip[6];

		if (!strncmp(bind7, adaptive_cooler_name, 13))
			TARGET_TJS[(bind7[13] - '0')] = trip[7];

		if (!strncmp(bind8, adaptive_cooler_name, 13))
			TARGET_TJS[(bind8[13] - '0')] = trip[8];

		if (!strncmp(bind9, adaptive_cooler_name, 13))
			TARGET_TJS[(bind9[13] - '0')] = trip[9];

		pr_debug("tscpu_write TTJ0=%d, TTJ1=%d, TTJ2=%d\n", TARGET_TJS[0],
				TARGET_TJS[1], TARGET_TJS[2]);
#endif

		pr_debug("tscpu_write g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,",
				g_THERMAL_TRIP[0], g_THERMAL_TRIP[1], g_THERMAL_TRIP[2]);
		pr_debug("g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,g_THERMAL_TRIP_5=%d,",
				g_THERMAL_TRIP[3], g_THERMAL_TRIP[4], g_THERMAL_TRIP[5]);
		pr_debug("g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
				g_THERMAL_TRIP[6], g_THERMAL_TRIP[7], g_THERMAL_TRIP[8],	g_THERMAL_TRIP[9]);
		pr_debug("tscpu_write cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,",
				g_bind0, g_bind1, g_bind2, g_bind3, g_bind4);
		pr_debug("cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
				g_bind5, g_bind6, g_bind7, g_bind8,	g_bind9);

		for (i = 0; i < num_trip; i++)
			trip_temp[i] = trip[i];

		interval = time_msec;

		pr_debug("tscpu_write trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,",
									trip_temp[0], trip_temp[1], trip_temp[2]);
		pr_debug("trip_3_temp=%d,trip_4_temp=%d,trip_5_temp=%d,",
									trip_temp[3], trip_temp[4], trip_temp[5]);
		pr_debug("trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,",
									trip_temp[6],	trip_temp[7], trip_temp[8]);
		pr_debug("trip_9_temp=%d,time_ms=%d, num_trip=%d\n",
									trip_temp[9], interval, num_trip);

		/* get temp, set high low threshold */

		pr_debug("tscpu_write tscpu_register_thermal\n");
		tscpu_register_thermal();

		proc_write_flag = 1;

		return count;
	}
	pr_debug("tscpu_write bad argument\n");

	return -EINVAL;
}

int tscpu_register_DVFS_hotplug_cooler(void)
{

	int i;

	pr_debug("tscpu_register_DVFS_hotplug_cooler\n");
	for (i = 0; i < Num_of_OPP; i++) {
		cl_dev[i] = mtk_thermal_cooling_device_register(&cooler_name[i * 20], NULL,
								&mtktscpu_cooling_F0x2_ops);
	}
	cl_dev_sysrst = mtk_thermal_cooling_device_register("mtktscpu-sysrst", NULL,
							    &mtktscpu_cooling_sysrst_ops);
#if CPT_ADAPTIVE_AP_COOLER
	cl_dev_adp_cpu[0] = mtk_thermal_cooling_device_register("cpu_adaptive_0", NULL,
								&mtktscpu_cooler_adp_cpu_ops);

	cl_dev_adp_cpu[1] = mtk_thermal_cooling_device_register("cpu_adaptive_1", NULL,
								&mtktscpu_cooler_adp_cpu_ops);

	cl_dev_adp_cpu[2] = mtk_thermal_cooling_device_register("cpu_adaptive_2", NULL,
								&mtktscpu_cooler_adp_cpu_ops);
#endif

	return 0;
}

void tscpu_unregister_DVFS_hotplug_cooler(void)
{

	int i;

	for (i = 0; i < Num_of_OPP; i++) {
		if (cl_dev[i]) {
			mtk_thermal_cooling_device_unregister(cl_dev[i]);
			cl_dev[i] = NULL;
		}
	}
	if (cl_dev_sysrst) {
		mtk_thermal_cooling_device_unregister(cl_dev_sysrst);
		cl_dev_sysrst = NULL;
	}
#if CPT_ADAPTIVE_AP_COOLER
	if (cl_dev_adp_cpu[0]) {
		mtk_thermal_cooling_device_unregister(cl_dev_adp_cpu[0]);
		cl_dev_adp_cpu[0] = NULL;
	}

	if (cl_dev_adp_cpu[1]) {
		mtk_thermal_cooling_device_unregister(cl_dev_adp_cpu[1]);
		cl_dev_adp_cpu[1] = NULL;
	}

	if (cl_dev_adp_cpu[2]) {
		mtk_thermal_cooling_device_unregister(cl_dev_adp_cpu[2]);
		cl_dev_adp_cpu[2] = NULL;
	}
#endif

}

/*tscpu_thermal_suspend spend 1000us~1310us*/
static int tscpu_thermal_suspend(struct platform_device *dev, pm_message_t state)
{
	int cnt = 0;
	int temp = 0;

	pr_debug("tscpu_thermal_suspend\n");

	g_tc_resume = 1;	/* set "1", don't read temp during suspend */

	if (talking_flag == false) {
		pr_debug("tscpu_thermal_suspend no talking\n");

		while (cnt < 30) {
			temp = DRV_Reg32(TEMPMSRCTL1);
			if (((temp & 0x81) == 0x00) || ((temp & 0x81) == 0x81)) {
				DRV_WriteReg32(TEMPMSRCTL1, (temp | 0x0E));
				break;
			}
			udelay(10);
			cnt++;
		}
		THERMAL_WRAP_WR32(0x00000000, TEMPMONCTL0);
		/*TSCON1[5:4]=2'b11, Buffer off */
		THERMAL_WRAP_WR32(DRV_Reg32(TS_CON1) | 0x00000030, TS_CON1);
		tscpu_thermal_clock_off();
	}
	return 0;
}

/*tscpu_thermal_suspend spend 3000us~4000us*/
static int tscpu_thermal_resume(struct platform_device *dev)
{

	pr_debug("tscpu_thermal_resume\n");
	g_tc_resume = 1;	/* set "1", don't read temp during start resume */

	if (talking_flag == false) {
		THERMAL_WRAP_WR32(DRV_Reg32(TS_CON0) | 0x000000C0, TS_CON0);
		thermal_reset_and_initial();
		set_thermal_ctrl_trigger_SPM(trip_temp[0]);
	}

	g_tc_resume = 2;	/* set "2", resume finish,can read temp */

	return 0;
}

static int tscpu_Tj_out(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_Tj_out, NULL);
}

static const struct file_operations mtktscpu_Tj_out_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_Tj_out,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write_Tj_out,
	.release = single_release,
};

static int tscpu_open_opp(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_opp, NULL);
}

static const struct file_operations mtktscpu_opp_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_open_opp,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int tscpu_open(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read, NULL);
}

static const struct file_operations mtktscpu_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write,
	.release = single_release,
};

static int tscpu_cal_open(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_cal, NULL);
}

static const struct file_operations mtktscpu_cal_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_cal_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


static int tscpu_read_temperature_open(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_temperature_info, NULL);
}

static const struct file_operations mtktscpu_read_temperature_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_read_temperature_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write,
	.release = single_release,
};


static int tscpu_set_temperature_open(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_set_temperature_read, NULL);
}

static const struct file_operations mtktscpu_set_temperature_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_set_temperature_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_set_temperature_write,
	.release = single_release,
};

#if CPT_ADAPTIVE_AP_COOLER

/* +ASC+ */
static int tscpu_open_atm(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_atm, NULL);
}

/* -ASC- */

static int tscpu_dtm_setting_open(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_dtm_setting, NULL);
}

static const struct file_operations mtktscpu_dtm_setting_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_dtm_setting_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write_dtm_setting,
	.release = single_release,
};


static int tscpu_gpu_threshold_open(struct inode *inode, struct file *file)
{
	return single_open(file, tscpu_read_gpu_threshold, NULL);
}

static const struct file_operations mtktscpu_gpu_threshold_fops = {
	.owner = THIS_MODULE,
	.open = tscpu_gpu_threshold_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = tscpu_write_gpu_threshold,
	.release = single_release,
};

#endif

#if THERMAL_DRV_UPDATE_TEMP_DIRECT_TO_MET
int tscpu_get_cpu_temp_met(MTK_THERMAL_SENSOR_CPU_ID_MET id)
{
	unsigned long flags;
	int ret;

	if (id < 0 || id >= MTK_THERMAL_SENSOR_CPU_COUNT)
		return -127000;
	else if (id == ATM_CPU_LIMIT)
		return (adaptive_cpu_power_limit != 0x7FFFFFFF) ? adaptive_cpu_power_limit : 0;
	else if (id == ATM_GPU_LIMIT)
		return (adaptive_gpu_power_limit != 0x7FFFFFFF) ? adaptive_gpu_power_limit : 0;

	tscpu_met_lock(&flags);
	if (a_tscpu_all_temp[id] == 0) {
		tscpu_met_unlock(&flags);
		return -127000;
	}
	ret = a_tscpu_all_temp[id];

	tscpu_met_unlock(&flags);
	return ret;
}
EXPORT_SYMBOL(tscpu_get_cpu_temp_met);
#endif

int thermal_fast_init(void)
{
	UINT32 temp;
	UINT32 cunt = 0;

	pr_debug("thermal_fast_init\n");

	temp = 0xDA1;
	DRV_WriteReg32(TEMPSPARE2, (0x00001000 + temp));
	DRV_WriteReg32(TEMPMONCTL1, 1);
	DRV_WriteReg32(TEMPMONCTL2, 1);
	DRV_WriteReg32(TEMPAHBPOLL, 1);

	DRV_WriteReg32(TEMPAHBTO,    0x000000FF);
	DRV_WriteReg32(TEMPMONIDET0, 0x00000000);
	DRV_WriteReg32(TEMPMONIDET1, 0x00000000);

	DRV_WriteReg32(TEMPMSRCTL0, 0x0000000);

	DRV_WriteReg32(TEMPADCPNP0, 0x1);
	DRV_WriteReg32(TEMPADCPNP1, 0x2);
	DRV_WriteReg32(TEMPADCPNP2, 0x3);

	DRV_WriteReg32(TEMPPNPMUXADDR, thermal_phy_base + 0x0F0);
	DRV_WriteReg32(TEMPADCMUXADDR, thermal_phy_base + 0x0F0);
	DRV_WriteReg32(TEMPADCENADDR,  thermal_phy_base + 0X0F4);
	DRV_WriteReg32(TEMPADCVALIDADDR, thermal_phy_base + 0X0F8);
	DRV_WriteReg32(TEMPADCVOLTADDR, thermal_phy_base + 0X0F8);

	DRV_WriteReg32(TEMPRDCTRL, 0x0);
	DRV_WriteReg32(TEMPADCVALIDMASK, 0x0000002C);
	DRV_WriteReg32(TEMPADCVOLTAGESHIFT, 0x0);
	DRV_WriteReg32(TEMPADCWRITECTRL, 0x3);

	DRV_WriteReg32(TEMPMONINT, 0x00000000);

	DRV_WriteReg32(TEMPMONCTL0, 0x0000000F);

	temp = DRV_Reg32(TEMPMSR0) & 0x0fff;
	while (temp != 0xDA1 && cunt < 20) {
		cunt++;
		pr_debug("[Power/CPU_Thermal]0 temp=%d,cunt=%d\n", temp, cunt);
		temp = DRV_Reg32(TEMPMSR0) & 0x0fff;
	}

	cunt = 0;
	temp = DRV_Reg32(TEMPMSR1) & 0x0fff;
	while (temp != 0xDA1 &&  cunt < 20) {
		cunt++;
		pr_debug("[Power/CPU_Thermal]1 temp=%d,cunt=%d\n", temp, cunt);
		temp = DRV_Reg32(TEMPMSR1) & 0x0fff;
	}

	cunt = 0;
	temp = DRV_Reg32(TEMPMSR2) & 0x0fff;
	while (temp != 0xDA1 &&  cunt < 20) {
		cunt++;
		pr_debug("[Power/CPU_Thermal]2 temp=%d,cunt=%d\n", temp, cunt);
		temp = DRV_Reg32(TEMPMSR2) & 0x0fff;
	}
	return 0;
}

#ifdef CONFIG_OF

static u64 of_get_phys_base(struct device_node *np)
{
	u64 size64;
	const __be32 *regaddr_p;

	regaddr_p = of_get_address(np, 0, &size64, NULL);
	if (!regaddr_p)
		return OF_BAD_ADDR;

	return of_translate_address(np, regaddr_p);
}

static int get_io_reg_base(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct device_node *np;

	if (node) {
		/* Setup IO addresses */
		thermal_base = of_iomap(node, 0);
		pr_debug("[THERM_CTRL] thermal_base=0x%lx\n", (unsigned long)thermal_base);

		/* get thermal phy base */
		thermal_phy_base = of_get_phys_base(node);
		pr_debug("[THERM_CTRL] thermal_phy_base=0x%lx\n", (unsigned long)thermal_phy_base);

		/* get thermal irq num */
		thermal_irq_number = irq_of_parse_and_map(node, 0);
		pr_debug("[THERM_CTRL] thermal_irq_number=0x%lx\n",
			(unsigned long)thermal_irq_number);

		np = of_parse_phandle(node, "auxadc", 0);
		auxadc_ts_base = of_iomap(np, 0);
		pr_debug("[THERM_CTRL] auxadc_ts_base=0x%lx\n", (unsigned long)auxadc_ts_base);

		auxadc_ts_phy_base = of_get_phys_base(np);
		pr_debug("[THERM_CTRL] auxadc_ts_phy_base=0x%lx\n",
			(unsigned long)auxadc_ts_phy_base);

		np = of_parse_phandle(node, "apmixedsys", 0);
		apmixed_ts_base = of_iomap(np, 0);
		pr_debug("[THERM_CTRL] apmixed_ts_base=0x%lx\n", (unsigned long)apmixed_ts_base);

		apmixed_phy_base = of_get_phys_base(np);
		pr_debug("[THERM_CTRL] apmixed_phy_base=0x%lx\n", (unsigned long)apmixed_phy_base);

		np = of_parse_phandle(node, "pericfg", 0);
		pericfg_base = of_iomap(np, 0);
		pr_debug("[THERM_CTRL] pericfg_base=0x%lx\n", (unsigned long)pericfg_base);
	}

	return 1;
}
#endif

static kuid_t uid = KUIDT_INIT(0);
static kgid_t gid = KGIDT_INIT(1000);

static int tscpu_thermal_probe(struct platform_device *pdev)
{
	int err = 0;
	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *mtktscpu_dir = NULL;

	pr_debug("tscpu_thermal_probe\n");

	clk_peri_therm = devm_clk_get(&pdev->dev, "therm");
	WARN_ON(IS_ERR(clk_peri_therm));

	clk_auxadc = devm_clk_get(&pdev->dev, "auxadc");
	WARN_ON(IS_ERR(clk_auxadc));

#ifdef CONFIG_OF
	if (get_io_reg_base(pdev) == 0)
		return 0;
#endif

	thermal_cal_prepare(&pdev->dev);

	thermal_calibration();

	THERMAL_WRAP_WR32(DRV_Reg32(TS_CON0) | 0x000000C0, TS_CON0);

	thermal_reset_and_initial();

	set_thermal_ctrl_trigger_SPM(trip_temp[0]);

	get_thermal_all_register();

#ifdef CONFIG_OF
	err =
	    request_irq(thermal_irq_number, thermal_interrupt_handler, IRQF_TRIGGER_LOW,
			THERMAL_NAME, NULL);
	if (err)
		pr_emerg("tscpu_thermal_probe IRQ register fail\n");
#else
	err =
	    request_irq(THERM_CTRL_IRQ_BIT_ID, thermal_interrupt_handler, IRQF_TRIGGER_LOW,
			THERMAL_NAME, NULL);
	if (err)
		pr_emerg("tscpu_thermal_probe IRQ register fail\n");
#endif

	err = init_cooler();
	if (err)
		return err;

	err = tscpu_register_DVFS_hotplug_cooler();
	if (err) {
		pr_debug("tscpu_register_DVFS_hotplug_cooler fail\n");
		return err;
	}

	err = tscpu_register_thermal();
	if (err) {
		pr_debug("tscpu_register_thermal fail\n");
		goto err_unreg;
	}

	mtktscpu_dir = mtk_thermal_get_proc_drv_therm_dir_entry();
	if (!mtktscpu_dir)
		pr_emerg("tscpu_thermal_probe mkdir /proc/driver/thermal failed\n");
	else {
		entry =
		    proc_create("tzcpu", S_IRUGO | S_IWUSR | S_IWGRP, mtktscpu_dir, &mtktscpu_fops);
		if (entry)
			proc_set_user(entry, uid, gid);

		entry = proc_create("thermlmt", S_IRUGO, NULL, &mtktscpu_opp_fops);

		entry = proc_create("tzcpu_cal", S_IRUSR, mtktscpu_dir, &mtktscpu_cal_fops);

		entry =
		    proc_create("tzcpu_read_temperature", S_IRUGO, mtktscpu_dir,
				&mtktscpu_read_temperature_fops);

		entry =
		    proc_create("tzcpu_set_temperature", S_IRUGO | S_IWUSR, mtktscpu_dir,
				&mtktscpu_set_temperature_fops);

#if CPT_ADAPTIVE_AP_COOLER
		entry =
		    proc_create("clatm_setting", S_IRUGO | S_IWUSR | S_IWGRP, mtktscpu_dir,
				&mtktscpu_dtm_setting_fops);
		if (entry)
			proc_set_user(entry, uid, gid);


		entry =
		    proc_create("clatm_gpu_threshold", S_IRUGO | S_IWUSR | S_IWGRP, mtktscpu_dir,
				&mtktscpu_gpu_threshold_fops);
		if (entry)
			proc_set_user(entry, uid, gid);

#endif

		entry =
		    proc_create("tzcpu_Tj_out_via_HW_pin", S_IRUGO | S_IWUSR, mtktscpu_dir,
				&mtktscpu_Tj_out_fops);
		if (entry)
			proc_set_user(entry, uid, gid);

	}
	return 0;

err_unreg:
	tscpu_unregister_DVFS_hotplug_cooler();

	return err;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_thermal_of_match[] = {
	{.compatible = THERMAL_DEVICE_NAME_2701,},
	{},
};
#endif

static struct platform_driver mtk_thermal_driver = {
	.remove = NULL,
	.shutdown = NULL,
	.probe = tscpu_thermal_probe,
	.suspend = tscpu_thermal_suspend,
	.resume = tscpu_thermal_resume,
	.driver = {
		   .name = THERMAL_NAME,
#ifdef CONFIG_OF
		   .of_match_table = mt_thermal_of_match,
#endif
		   },
};

static int __init tscpu_init(void)
{
	return platform_driver_register(&mtk_thermal_driver);
}

static void __exit tscpu_exit(void)
{

	pr_debug("tscpu_exit\n");

	tscpu_unregister_thermal();
	tscpu_unregister_DVFS_hotplug_cooler();

#if THERMAL_DRV_UPDATE_TEMP_DIRECT_TO_MET
	mt_thermalsampler_registerCB(NULL);
#endif

	tscpu_thermal_clock_off();

	return platform_driver_unregister(&mtk_thermal_driver);
}
module_init(tscpu_init);
module_exit(tscpu_exit);
