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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/atomic.h>
#include <linux/string.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/regmap.h>
#include <uapi/asm/setup.h>

#include "mtk_spm_internal.h"

int __weak is_ext_buck_exist(void)
{
	return 0;
}

void __attribute__((weak)) __iomem *spm_get_i2c_base(void)
{
	return NULL;
}

/*
 * Config and Parameter
 */
#define LOG_BUF_SIZE		256

/*
 * Define and Declare
 */
DEFINE_SPINLOCK(__spm_lock);
atomic_t __spm_mainpll_req = ATOMIC_INIT(0);

static const char *wakesrc_str[32] = {
	[0] = "SPM_MERGE",
	[3] = "WDT",
	[4] = "GPT",
	[5] = "EINT",
	[6] = "CONN_WDT",
	[7] = "PCIE",
	[9] = "GMAC",
	[10] = "CONN2AP",
	[11] = "F26M_WAKE",
	[12] = "F26M_SLEE",
	[13] = "PCM_WDT",
	[14] = "USB_CD ",
	[15] = "USB_PDN",
	[18] = "SEJ",
	[19] = "UART0",
	[20] = "AFE",
	[21] = "THERM",
	[22] = "CIRQ",
	[24] = "SYSPWREQ",
	[27] = "RTC",
	[29] = "CPU_IRQ",
	[30] = "APSRC_WAKE",
	[31] = "APSRC_SLEEP",
};

unsigned int __attribute__((weak))
	pmic_read_interface_nolock(unsigned int RegNum,
					unsigned int *val,
					unsigned int MASK,
					unsigned int SHIFT)
{
	return 0;
}

unsigned int __attribute__((weak))
	pmic_config_interface_nolock(unsigned int RegNum,
					unsigned int val,
					unsigned int MASK,
					unsigned int SHIFT)
{
	return 0;
}

/*
 * Function and API
 */
void __spm_reset_and_init_pcm(const struct pcm_desc *pcmdesc)
{
	u32 con1;

#ifdef SPM_VCORE_EN
	if (spm_read(SPM_PCM_REG1_DATA) == 0x1) {
		/* SPM code swapping (force high voltage) */
		spm_write(SPM_SLEEP_CPU_WAKEUP_EVENT, 1);
		while (spm_read(SPM_PCM_REG11_DATA) != 0x55AA55AA)
			udelay(1);
		spm_write(SPM_SLEEP_CPU_WAKEUP_EVENT, 0);
	}
#endif

	/* reset PCM */
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY | CON0_PCM_SW_RESET);
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY);
	/* PCM reset failed */
	WARN_ON((spm_read(SPM_PCM_FSM_STA) & 0x3fffff) != PCM_FSM_STA_DEF);

	/* init PCM_CON0 (disable event vector) */
	spm_write(SPM_PCM_CON0, CON0_CFG_KEY | CON0_IM_SLEEP_DVS);

	/* init PCM_CON1 (disable PCM timer but keep PCM WDT setting) */
	con1 = spm_read(SPM_PCM_CON1) & (CON1_PCM_WDT_WAKE_MODE |
							CON1_PCM_WDT_EN);
	spm_write(SPM_PCM_CON1, con1 | CON1_CFG_KEY | CON1_EVENT_LOCK_EN |
		  CON1_SPM_SRAM_ISO_B | CON1_SPM_SRAM_SLP_B |
		  (pcmdesc->replace ? 0 : CON1_IM_NONRP_EN) |
		  CON1_MIF_APBEN);
}

void __spm_kick_im_to_fetch(const struct pcm_desc *pcmdesc)
{
	u32 ptr, len, con0;

	/* tell IM where is PCM code (use slave mode if code existed) */
	ptr = base_va_to_pa(pcmdesc->base);
	len = pcmdesc->size - 1;
	if (spm_read(SPM_PCM_IM_PTR) != ptr ||
		spm_read(SPM_PCM_IM_LEN) != len ||
		pcmdesc->sess > 2) {
		spm_write(SPM_PCM_IM_PTR, ptr);
		spm_write(SPM_PCM_IM_LEN, len);
	} else {
		spm_write(SPM_PCM_CON1, spm_read(SPM_PCM_CON1) | CON1_CFG_KEY |
								CON1_IM_SLAVE);
	}

	/* kick IM to fetch (only toggle IM_KICK) */
	con0 = spm_read(SPM_PCM_CON0) & ~(CON0_IM_KICK | CON0_PCM_KICK);
	spm_write(SPM_PCM_CON0, con0 | CON0_CFG_KEY | CON0_IM_KICK);
	spm_write(SPM_PCM_CON0, con0 | CON0_CFG_KEY);
}

void __spm_init_pcm_register(void)
{
	/* init r0 with POWER_ON_VAL0 */
	spm_write(SPM_PCM_REG_DATA_INI, spm_read(SPM_POWER_ON_VAL0));
	spm_write(SPM_PCM_PWR_IO_EN, PCM_RF_SYNC_R0);
	spm_write(SPM_PCM_PWR_IO_EN, 0);

	/* init r7 with POWER_ON_VAL1 */
	spm_write(SPM_PCM_REG_DATA_INI, spm_read(SPM_POWER_ON_VAL1));
	spm_write(SPM_PCM_PWR_IO_EN, PCM_RF_SYNC_R7);
	spm_write(SPM_PCM_PWR_IO_EN, 0);
}

void __spm_init_event_vector(const struct pcm_desc *pcmdesc)
{
	/* init event vector register */
	spm_write(SPM_PCM_EVENT_VECTOR0, pcmdesc->vec0);
	spm_write(SPM_PCM_EVENT_VECTOR1, pcmdesc->vec1);
	spm_write(SPM_PCM_EVENT_VECTOR2, pcmdesc->vec2);
	spm_write(SPM_PCM_EVENT_VECTOR3, pcmdesc->vec3);
	spm_write(SPM_PCM_EVENT_VECTOR4, pcmdesc->vec4);
	spm_write(SPM_PCM_EVENT_VECTOR5, pcmdesc->vec5);
	spm_write(SPM_PCM_EVENT_VECTOR6, pcmdesc->vec6);
	spm_write(SPM_PCM_EVENT_VECTOR7, pcmdesc->vec7);
	/* event vector will be enabled by PCM itself */
}

void __spm_set_power_control(const struct pwr_ctrl *pwrctrl)
{
	/* set other SYS request mask */
	spm_write(SPM_AP_STANBY_CON, (!pwrctrl->srclkenai_mask << 25) |
		  (!pwrctrl->conn_mask << 23) |
		  (!pwrctrl->mm_ddr_req_mask << 18) |
		  (!pwrctrl->vdec_req_mask << 17) |
		  (!pwrctrl->mfg_req_mask << 16) |
		  (!!pwrctrl->mcusys_idle_mask << 7) |
		  (!!pwrctrl->ca7top_idle_mask << 5) |
		  (!!pwrctrl->wfi_op << 4));
	spm_write(SPM_PCM_SRC_REQ, (!!pwrctrl->pcm_f26m_req << 1) |
		  (!!pwrctrl->pcm_apsrc_req << 0));
	/* set CPU WFI mask */
	spm_write(SPM_SLEEP_CA7_WFI0_EN, !!pwrctrl->ca7_wfi0_en);
	spm_write(SPM_SLEEP_CA7_WFI1_EN, !!pwrctrl->ca7_wfi1_en);
	spm_write(SPM_SLEEP_CA7_WFI2_EN, !!pwrctrl->ca7_wfi2_en);
	spm_write(SPM_SLEEP_CA7_WFI3_EN, !!pwrctrl->ca7_wfi3_en);
}

void __spm_set_wakeup_event(const struct pwr_ctrl *pwrctrl)
{
	u32 val, mask, isr;

	/* set PCM timer (set to max when disable) */
	if (pwrctrl->timer_val_cust == 0)
		val = pwrctrl->timer_val ? : PCM_TIMER_MAX;
	else
		val = pwrctrl->timer_val_cust;

	spm_write(SPM_PCM_TIMER_VAL, val);
	spm_write(SPM_PCM_CON1, spm_read(SPM_PCM_CON1) | CON1_CFG_KEY |
							CON1_PCM_TIMER_EN);

	/* unmask AP wakeup source */
	if (pwrctrl->wake_src_cust == 0)
		mask = pwrctrl->wake_src;
	else
		mask = pwrctrl->wake_src_cust;

	if (pwrctrl->syspwreq_mask)
		mask &= ~WAKE_SRC_SYSPWREQ;
	spm_write(SPM_SLEEP_WAKEUP_EVENT_MASK, ~mask);

	/* unmask SPM ISR (keep TWAM setting) */
	isr = spm_read(SPM_SLEEP_ISR_MASK) & ISRM_TWAM;
	spm_write(SPM_SLEEP_ISR_MASK, isr | ISRM_RET_IRQ_AUX);
}

#if !CONFIG_SUPPORT_PCM_ALLINONE
void __spm_kick_pcm_to_run(const struct pwr_ctrl *pwrctrl)
{
	u32 con0;

	/* init register to match PCM expectation */
	spm_write(SPM_PCM_MAS_PAUSE_MASK, 0xffffffff);
	spm_write(SPM_PCM_REG_DATA_INI, 0);

	/* set PCM flags and data */
	spm_write(SPM_PCM_FLAGS, pwrctrl->pcm_flags);
	spm_write(SPM_PCM_RESERVE, pwrctrl->pcm_reserve);

	/* lock Infra DCM when PCM runs */
	spm_write(SPM_CLK_CON, (spm_read(SPM_CLK_CON) & ~CC_LOCK_INFRA_DCM) |
		  (pwrctrl->infra_dcm_lock ? CC_LOCK_INFRA_DCM : 0));

	/* enable r0 and r7 to control power */
	spm_write(SPM_PCM_PWR_IO_EN,
		(pwrctrl->r0_ctrl_en ? PCM_PWRIO_EN_R0 : 0) |
		  (pwrctrl->r7_ctrl_en ? PCM_PWRIO_EN_R7 : 0));

	/* kick PCM to run (only toggle PCM_KICK) */
	con0 = spm_read(SPM_PCM_CON0) & ~(CON0_IM_KICK | CON0_PCM_KICK);
	spm_write(SPM_PCM_CON0, con0 | CON0_CFG_KEY | CON0_PCM_KICK);
	spm_write(SPM_PCM_CON0, con0 | CON0_CFG_KEY);
}
#else
void __spm_kick_pcm_to_run(const struct pwr_ctrl *pwrctrl)
{
	/* set PCM flags and data */
	spm_write(SPM_PCM_FLAGS, pwrctrl->pcm_flags);
	spm_write(SPM_PCM_RESERVE, pwrctrl->pcm_reserve);

	/* lock Infra DCM when PCM runs */
	spm_write(SPM_CLK_CON, (spm_read(SPM_CLK_CON) & ~CC_LOCK_INFRA_DCM) |
		  (pwrctrl->infra_dcm_lock ? CC_LOCK_INFRA_DCM : 0));
}
#endif

void __spm_get_wakeup_status(struct wake_status *wakesta)
{
	/* get PC value if PCM assert (pause abort) */
	wakesta->assert_pc = spm_read(SPM_PCM_REG_DATA_INI);

	/* get wakeup event */
	wakesta->r12 = spm_read(SPM_PCM_REG12_DATA);

	wakesta->raw_sta = spm_read(SPM_SLEEP_ISR_RAW_STA);
	wakesta->wake_misc = spm_read(SPM_SLEEP_WAKEUP_MISC);

	/* get sleep time */
	wakesta->timer_out = spm_read(SPM_PCM_TIMER_OUT);

	/* get other SYS and co-clock status */
	wakesta->r13 = spm_read(SPM_PCM_REG13_DATA);
	wakesta->idle_sta = spm_read(SPM_SLEEP_SUBSYS_IDLE_STA);

	/* get debug flag (r5) for PCM execution check */
	wakesta->debug_flag = spm_read(SPM_PCM_RESERVE4);

	/* get special pattern (0xf0000 or 0x10000) if sleep abort */
	wakesta->event_reg = spm_read(SPM_PCM_EVENT_REG_STA);

	/* get ISR status */
	wakesta->isr = spm_read(SPM_SLEEP_ISR_STATUS);
}

void __spm_clean_after_wakeup(void)
{
	/* disable r0 and r7 to control power */
#if !CONFIG_SUPPORT_PCM_ALLINONE
	spm_write(SPM_PCM_PWR_IO_EN, 0);
#endif

	/* clean CPU wakeup event */
	spm_write(SPM_SLEEP_CPU_WAKEUP_EVENT, 0);

	/* clean PCM timer event */
	spm_write(SPM_PCM_CON1, CON1_CFG_KEY | (spm_read(SPM_PCM_CON1) &
							~CON1_PCM_TIMER_EN));

	/* clean wakeup event raw status (for edge trigger event) */
	spm_write(SPM_SLEEP_WAKEUP_EVENT_MASK, ~0);

	/* clean ISR status (except TWAM) */
	spm_write(SPM_SLEEP_ISR_MASK, spm_read(SPM_SLEEP_ISR_MASK) |
						ISRM_ALL_EXC_TWAM);
	spm_write(SPM_SLEEP_ISR_STATUS, ISRC_ALL_EXC_TWAM);
	spm_write(SPM_PCM_SW_INT_CLEAR, PCM_SW_INT_ALL);
}

#define spm_print(suspend, fmt, args...)	\
do {						\
	if (!suspend)				\
		spm_debug(fmt, ##args);		\
	else					\
		spm_crit2(fmt, ##args);		\
} while (0)

wake_reason_t __spm_output_wake_reason(const struct wake_status *wakesta,
				       const struct pcm_desc *pcmdesc,
							bool suspend)
{
	int i;
	char buf[LOG_BUF_SIZE] = { 0 };
	wake_reason_t wr = WR_UNKNOWN;

	if (wakesta->assert_pc != 0) {
		spm_print(suspend, "PCM ASSERT AT %u (%s), r13 = 0x%x, debug_flag = 0x%x\n",
			  wakesta->assert_pc, pcmdesc->version, wakesta->r13,
							wakesta->debug_flag);
		return WR_PCM_ASSERT;
	}

	if (wakesta->r12 & WAKE_SRC_SPM_MERGE) {
		if (wakesta->wake_misc & WAKE_MISC_PCM_TIMER) {
			strcat(buf, " PCM_TIMER");
			wr = WR_PCM_TIMER;
		}
		if (wakesta->wake_misc & WAKE_MISC_TWAM) {
			strcat(buf, " TWAM");
			wr = WR_WAKE_SRC;
		}
		if (wakesta->wake_misc & WAKE_MISC_CPU_WAKE) {
			strcat(buf, " CPU");
			wr = WR_WAKE_SRC;
		}
	}
	for (i = 1; i < 32; i++) {
		if (wakesta->r12 & (1U << i)) {
			if ((strlen(buf) + strlen(wakesrc_str[i])) <
						LOG_BUF_SIZE)
			strncat(buf, wakesrc_str[i], strlen(wakesrc_str[i]));

			wr = WR_WAKE_SRC;
		}
	}

	spm_warn("wake up by%s, timer_out = %u, r13 = 0x%x, debug_flag = 0x%x, apsrc_cnt = %d\n",
		  buf, wakesta->timer_out, wakesta->r13, wakesta->debug_flag,
							wakesta->apsrc_cnt);

	spm_warn(
		  "r12 = 0x%x, raw_sta = 0x%x, idle_sta = 0x%x, event_reg = 0x%x, isr = 0x%x\n",
		  wakesta->r12, wakesta->raw_sta, wakesta->idle_sta,
		  wakesta->event_reg, wakesta->isr);

	return wr;
}

#if CONFIG_SUPPORT_PCM_ALLINONE
static unsigned int spm_is_pcm_loaded;
static void __spm_set_pcm_loaded(void)
{
	spm_is_pcm_loaded = 1;
}

unsigned int __spm_is_pcm_loaded(void)
{
	return spm_is_pcm_loaded;
}

void __spm_init_pcm_AllInOne(const struct pcm_desc *pcmdesc)
{
	u32 con0;

	__spm_reset_and_init_pcm(pcmdesc);

	__spm_kick_im_to_fetch(pcmdesc);

	__spm_init_pcm_register();

	__spm_init_event_vector(pcmdesc);

	/* init register to match PCM expectation */
	spm_write(SPM_PCM_MAS_PAUSE_MASK, 0xffffffff);
	spm_write(SPM_PCM_REG_DATA_INI, 0);

	/* enable r0 and r7 to control power */
	spm_write(SPM_PCM_PWR_IO_EN, PCM_PWRIO_EN_R0 | PCM_PWRIO_EN_R7);

	/* kick PCM to run (only toggle PCM_KICK) */
	con0 = spm_read(SPM_PCM_CON0) & ~(CON0_IM_KICK | CON0_PCM_KICK);
	spm_write(SPM_PCM_CON0, con0 | CON0_CFG_KEY | CON0_PCM_KICK);
	spm_write(SPM_PCM_CON0, con0 | CON0_CFG_KEY);

	__spm_set_pcm_loaded();
}

void __spm_set_pcm_cmd(unsigned int cmd)
{
	spm_write(SPM_PCM_RESERVE, (spm_read(SPM_PCM_RESERVE) & ~PCM_CMD_MASK)
									| cmd);
}
#endif

/* Pwrap API */
enum {
	IDX_DI_VPROC_CA7_NORMAL,	/* 0 *//* PMIC_WRAP_PHASE_DEEPIDLE */
	IDX_DI_VPROC_CA7_SLEEP,		/* 1 */
	IDX_DI_VSRAM_CA7_FAST_TRSN_EN,	/* 2 */
	IDX_DI_VSRAM_CA7_FAST_TRSN_DIS,	/* 3 */
	IDX_DI_VCORE_NORMAL,		/* 4 */
	IDX_DI_VCORE_SLEEP,		/* 5 */
	IDX_DI_VCORE_PDN_NORMAL,	/* 6 */
	IDX_DI_VCORE_PDN_SLEEP,		/* 7 */
	NR_IDX_DI,
};
enum {
	IDX_SO_VSRAM_CA15L_NORMAL,	/* 0 *//* PMIC_WRAP_PHASE_SODI */
	IDX_SO_VSRAM_CA15L_SLEEP,	/* 1 */
	IDX_SO_VPROC_CA7_NORMAL,	/* 2 */
	IDX_SO_VPROC_CA7_SLEEP,		/* 3 */
	IDX_SO_VCORE_NORMAL,		/* 4 */
	IDX_SO_VCORE_SLEEP,		/* 5 */
	IDX_SO_VSRAM_CA7_FAST_TRSN_EN,	/* 6 */
	IDX_SO_VSRAM_CA7_FAST_TRSN_DIS,	/* 7 */
	NR_IDX_SO,
};

static void __iomem *pwrap_base;
#define PMIC_WRAP_DVFS_ADR0     (unsigned long)(pwrap_base + 0x0E8)
#define PMIC_WRAP_DVFS_WDATA0   (unsigned long)(pwrap_base + 0x0EC)
#define PMIC_WRAP_DVFS_ADR1     (unsigned long)(pwrap_base + 0x0F0)
#define PMIC_WRAP_DVFS_WDATA1   (unsigned long)(pwrap_base + 0x0F4)
#define PMIC_WRAP_DVFS_ADR2     (unsigned long)(pwrap_base + 0x0F8)
#define PMIC_WRAP_DVFS_WDATA2   (unsigned long)(pwrap_base + 0x0FC)
#define PMIC_WRAP_DVFS_ADR3     (unsigned long)(pwrap_base + 0x100)
#define PMIC_WRAP_DVFS_WDATA3   (unsigned long)(pwrap_base + 0x104)
#define PMIC_WRAP_DVFS_ADR4     (unsigned long)(pwrap_base + 0x108)
#define PMIC_WRAP_DVFS_WDATA4   (unsigned long)(pwrap_base + 0x10C)
#define PMIC_WRAP_DVFS_ADR5     (unsigned long)(pwrap_base + 0x110)
#define PMIC_WRAP_DVFS_WDATA5   (unsigned long)(pwrap_base + 0x114)
#define PMIC_WRAP_DVFS_ADR6     (unsigned long)(pwrap_base + 0x118)
#define PMIC_WRAP_DVFS_WDATA6   (unsigned long)(pwrap_base + 0x11C)
#define PMIC_WRAP_DVFS_ADR7     (unsigned long)(pwrap_base + 0x120)
#define PMIC_WRAP_DVFS_WDATA7   (unsigned long)(pwrap_base + 0x124)

#define NR_PMIC_WRAP_CMD 8

#define PMIC_ADDR_VPROC_VOSEL_ON		0x0220	/* [6:0] */
#define PMIC_ADDR_VCORE_VOSEL_ON		0x0314	/* [6:0] */

struct pmic_wrap_cmd {
	unsigned long cmd_addr;
	unsigned long cmd_wdata;
};

struct pmic_wrap_setting {
	enum pmic_wrap_phase_id phase;
	struct pmic_wrap_cmd addr[NR_PMIC_WRAP_CMD];
	struct {
		struct {
			unsigned long cmd_addr;
			unsigned long cmd_wdata;
		} _[NR_PMIC_WRAP_CMD];
		const int nr_idx;
	} set[NR_PMIC_WRAP_PHASE];
};

#define VOLT_TO_PMIC_VAL(volt)  ((((volt) - 700) * 100 + 625 - 1) / 625)

static struct pmic_wrap_setting pw = {
	.phase = NR_PMIC_WRAP_PHASE,	/* invalid setting for init */

	.addr = { {0, 0} },

	/* Vproc only, power off: index 1, power on: 0 */
	.set[PMIC_WRAP_PHASE_DEEPIDLE] = {
		._[IDX_DI_VPROC_CA7_NORMAL] = {PMIC_ADDR_VPROC_VOSEL_ON,
			VOLT_TO_PMIC_VAL(1150),},
		._[IDX_DI_VPROC_CA7_SLEEP] = {PMIC_ADDR_VPROC_VOSEL_ON,
			VOLT_TO_PMIC_VAL(850),},
		._[IDX_DI_VSRAM_CA7_FAST_TRSN_EN] = {PMIC_ADDR_VCORE_VOSEL_ON,
			VOLT_TO_PMIC_VAL(1125),},
		._[IDX_DI_VSRAM_CA7_FAST_TRSN_DIS] = {PMIC_ADDR_VCORE_VOSEL_ON,
			VOLT_TO_PMIC_VAL(900),},
		._[IDX_DI_VCORE_NORMAL] = {PMIC_ADDR_VPROC_VOSEL_ON,
			VOLT_TO_PMIC_VAL(1000),},
		._[IDX_DI_VCORE_SLEEP] = {PMIC_ADDR_VPROC_VOSEL_ON,
			VOLT_TO_PMIC_VAL(800),},
		._[IDX_DI_VCORE_PDN_NORMAL] = {PMIC_ADDR_VCORE_VOSEL_ON,
			VOLT_TO_PMIC_VAL(1125),},
		._[IDX_DI_VCORE_PDN_SLEEP] = {PMIC_ADDR_VCORE_VOSEL_ON,
			VOLT_TO_PMIC_VAL(900),},
		.nr_idx = NR_IDX_DI,
	},

	/* Vproc only, power off: 1, power on: 0 */
	.set[PMIC_WRAP_PHASE_SODI] = {
		._[IDX_SO_VSRAM_CA15L_NORMAL] = {PMIC_ADDR_VPROC_VOSEL_ON,
			VOLT_TO_PMIC_VAL(1150),},
		._[IDX_SO_VSRAM_CA15L_SLEEP] = {PMIC_ADDR_VPROC_VOSEL_ON,
			VOLT_TO_PMIC_VAL(850),},
		._[IDX_SO_VPROC_CA7_NORMAL] = {PMIC_ADDR_VPROC_VOSEL_ON,
			VOLT_TO_PMIC_VAL(1125),},
		._[IDX_SO_VPROC_CA7_SLEEP] = {PMIC_ADDR_VPROC_VOSEL_ON,
			VOLT_TO_PMIC_VAL(700),},
		._[IDX_SO_VCORE_NORMAL] = {PMIC_ADDR_VPROC_VOSEL_ON,
			VOLT_TO_PMIC_VAL(1000),},
		._[IDX_SO_VCORE_SLEEP] = {PMIC_ADDR_VPROC_VOSEL_ON,
			VOLT_TO_PMIC_VAL(800),},
		._[IDX_SO_VSRAM_CA7_FAST_TRSN_EN] = {PMIC_ADDR_VCORE_VOSEL_ON,
			VOLT_TO_PMIC_VAL(1125),},
		._[IDX_SO_VSRAM_CA7_FAST_TRSN_DIS] = {PMIC_ADDR_VCORE_VOSEL_ON,
			VOLT_TO_PMIC_VAL(1125),},
		.nr_idx = NR_IDX_SO,
	},
};

static struct regmap *pmic_regmap;
static int __init spm_get_pwrap_base(void)
{
	struct device_node *node, *pwrap_node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt7622-pwrap");
	if (!node)
		spm_warn("find pwrap node failed\n");
	pwrap_base = of_iomap(node, 0);
	if (!pwrap_base)
		spm_warn("get pwrap_base failed\n");

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt7622-scpsys");
	if (!node)
		spm_err("find mt7622-scpsys node failed\n");
	WARN_ON(!node);
	pwrap_node = of_parse_phandle(node, "mediatek,pwrap-regmap", 0);
	if (pwrap_node) {
#if 0
		pmic_regmap = pwrap_node_to_regmap(pwrap_node);
		if (IS_ERR(pmic_regmap))
			spm_err("cannot get pmic_regmap\n");
#endif
	} else {
		spm_err("pwrap node has not register regmap\n");
	}

	return 0;
}
late_initcall(spm_get_pwrap_base);

static void spm_pmic_table_init(void)
{
	struct pmic_wrap_cmd pwrap_cmd_default[NR_PMIC_WRAP_CMD] = {
		{PMIC_WRAP_DVFS_ADR0, PMIC_WRAP_DVFS_WDATA0,},
		{PMIC_WRAP_DVFS_ADR1, PMIC_WRAP_DVFS_WDATA1,},
		{PMIC_WRAP_DVFS_ADR2, PMIC_WRAP_DVFS_WDATA2,},
		{PMIC_WRAP_DVFS_ADR3, PMIC_WRAP_DVFS_WDATA3,},
		{PMIC_WRAP_DVFS_ADR4, PMIC_WRAP_DVFS_WDATA4,},
		{PMIC_WRAP_DVFS_ADR5, PMIC_WRAP_DVFS_WDATA5,},
		{PMIC_WRAP_DVFS_ADR6, PMIC_WRAP_DVFS_WDATA6,},
		{PMIC_WRAP_DVFS_ADR7, PMIC_WRAP_DVFS_WDATA7,},
	};

	memcpy(pw.addr, pwrap_cmd_default, sizeof(pwrap_cmd_default));
}

void __spm_set_pmic_phase(enum pmic_wrap_phase_id phase)
{
	int i, ret;
	u32 rdata = 0;

	WARN_ON(phase >= NR_PMIC_WRAP_PHASE);

	if (pw.addr[0].cmd_addr == 0) {
		spm_warn("pmic table not initialized\n");
		spm_pmic_table_init();
	}

	pw.phase = phase;

	/* backup the voltage setting of Vproc */
	ret = regmap_read(pmic_regmap, PMIC_ADDR_VPROC_VOSEL_ON, &rdata);
	WARN_ON(ret);

	if (phase == PMIC_WRAP_PHASE_SODI)
		pw.set[phase]._[IDX_SO_VSRAM_CA15L_NORMAL].cmd_wdata = rdata;
	else if (phase == PMIC_WRAP_PHASE_DEEPIDLE)
		pw.set[phase]._[IDX_DI_VPROC_CA7_NORMAL].cmd_wdata = rdata;

	for (i = 0; i < pw.set[phase].nr_idx; i++) {
		spm_write(pw.addr[i].cmd_addr, pw.set[phase]._[i].cmd_addr);
		spm_write(pw.addr[i].cmd_wdata, pw.set[phase]._[i].cmd_wdata);
	}
}

MODULE_DESCRIPTION("SPM-Internal Driver v0.1");
