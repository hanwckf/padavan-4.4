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

/*! \file
 * \brief  Declaration of library functions
 *
 * Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

#ifndef _MTK_WCN_CONSYS_HW_H_
#define _MTK_WCN_CONSYS_HW_H_

#include <sync_write.h>
/*#include <mt_reg_base.h>*/
#include "wmt_plat.h"

/*device tree mode*/
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/irqreturn.h>
#include <linux/of_address.h>
#endif
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#define ENABLE MTK_WCN_BOOL_TRUE
#define DISABLE MTK_WCN_BOOL_FALSE

#define KBYTE (1024*sizeof(char))
#define CONSYS_PAGED_DUMP_SIZE (32*KBYTE)
#define CONSYS_EMI_MEM_SIZE (343*KBYTE) /*coredump space , 343K is enough */

#define CONSYS_SET_BIT(REG, BITVAL) (*((volatile UINT32 *)(REG)) |= ((UINT32)(BITVAL)))
#define CONSYS_CLR_BIT(REG, BITVAL) ((*(volatile UINT32 *)(REG)) &= ~((UINT32)(BITVAL)))
#define CONSYS_CLR_BIT_WITH_KEY(REG, BITVAL, KEY) {\
	UINT32 val = (*(volatile UINT32 *)(REG)); \
	val &= ~((UINT32)(BITVAL)); \
	val |= ((UINT32)(KEY)); \
	(*(volatile UINT32 *)(REG)) = val;\
}
#define CONSYS_REG_READ(addr) (*((volatile UINT32 *)(addr)))
#define CONSYS_REG_WRITE(addr, data)  mt_reg_sync_writel(data, addr)
/*force fw assert pattern*/
#define EXP_APMEM_HOST_OUTBAND_ASSERT_MAGIC_W1   (0x19b30bb1)

#define DYNAMIC_DUMP_GROUP_NUM 5

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

struct CONSYS_BASE_ADDRESS {
	SIZE_T mcu_base;
	SIZE_T ap_rgu_base;
	SIZE_T topckgen_base;
	SIZE_T spm_base;
};

typedef enum _ENUM_EMI_CTRL_STATE_OFFSET_ {
	EXP_APMEM_CTRL_STATE = 0x0,
	EXP_APMEM_CTRL_HOST_SYNC_STATE = 0x4,
	EXP_APMEM_CTRL_HOST_SYNC_NUM = 0x8,
	EXP_APMEM_CTRL_CHIP_SYNC_STATE = 0xc,
	EXP_APMEM_CTRL_CHIP_SYNC_NUM = 0x10,
	EXP_APMEM_CTRL_CHIP_SYNC_ADDR = 0x14,
	EXP_APMEM_CTRL_CHIP_SYNC_LEN = 0x18,
	EXP_APMEM_CTRL_CHIP_PRINT_BUFF_START = 0x1c,
	EXP_APMEM_CTRL_CHIP_PRINT_BUFF_LEN = 0x20,
	EXP_APMEM_CTRL_CHIP_PRINT_BUFF_IDX = 0x24,
	EXP_APMEM_CTRL_CHIP_INT_STATUS = 0x28,
	EXP_APMEM_CTRL_CHIP_PAGED_DUMP_END = 0x2c,
	EXP_APMEM_CTRL_HOST_OUTBAND_ASSERT_W1 = 0x30,
	EXP_APMEM_CTRL_CHIP_PAGE_DUMP_NUM = 0x44,
	EXP_APMEM_CTRL_CHIP_FW_DBGLOG_MODE = 0x40,
	EXP_APMEM_CTRL_CHIP_DYNAMIC_DUMP = 0x48,
	EXP_APMEM_CTRL_MAX
} ENUM_EMI_CTRL_STATE_OFFSET, *P_ENUM_EMI_CTRL_STATE_OFFSET;

typedef enum _CONSYS_GPS_CO_CLOCK_TYPE_ {
	GPS_TCXO_TYPE = 0,
	GPS_CO_TSX_TYPE = 1,
	GPS_CO_DCXO_TYPE = 2,
	GPS_CO_VCTCXO_TYPE = 3,
	GPS_CO_CLOCK_TYPE_MAX
} CONSYS_GPS_CO_CLOCK_TYPE, *P_CONSYS_GPS_CO_CLOCK_TYPE;

typedef INT32(*CONSYS_IC_CLOCK_BUFFER_CTRL) (MTK_WCN_BOOL enable);
typedef VOID(*CONSYS_IC_HW_RESET_BIT_SET) (MTK_WCN_BOOL enable);
typedef VOID(*CONSYS_IC_HW_SPM_CLK_GATING_ENABLE) (VOID);
typedef INT32(*CONSYS_IC_HW_POWER_CTRL) (MTK_WCN_BOOL enable);
typedef INT32(*CONSYS_IC_AHB_CLOCK_CTRL) (MTK_WCN_BOOL enable);
typedef INT32(*POLLING_CONSYS_IC_CHIPID) (VOID);
typedef VOID(*CONSYS_IC_ARC_REG_SETTING) (VOID);
typedef VOID(*CONSYS_IC_AFE_REG_SETTING) (VOID);
typedef INT32(*CONSYS_IC_HW_VCN18_CTRL) (MTK_WCN_BOOL enable);
typedef VOID(*CONSYS_IC_VCN28_HW_MODE_CTRL) (UINT32 enable);
typedef INT32(*CONSYS_IC_HW_VCN28_CTRL) (UINT32 enable);
typedef INT32(*CONSYS_IC_HW_WIFI_VCN33_CTRL) (UINT32 enable);
typedef INT32(*CONSYS_IC_HW_BT_VCN33_CTRL) (UINT32 enable);
typedef UINT32(*CONSYS_IC_SOC_CHIPID_GET) (VOID);
typedef INT32(*CONSYS_IC_EMI_MPU_SET_REGION_PROTECTION) (VOID);
typedef UINT32(*CONSYS_IC_EMI_SET_REMAPPING_REG) (VOID);
typedef INT32(*IC_BT_WIFI_SHARE_V33_SPIN_LOCK_INIT) (VOID);
typedef INT32(*CONSYS_IC_CLK_GET_FROM_DTS) (struct platform_device *pdev);
typedef INT32(*CONSYS_IC_PMIC_GET_FROM_DTS) (struct platform_device *pdev);
typedef INT32(*CONSYS_IC_READ_IRQ_INFO_FROM_DTS) (struct platform_device *pdev, PINT32 irq_num, PUINT32 irq_flag);
typedef INT32(*CONSYS_IC_READ_REG_FROM_DTS) (struct platform_device *pdev);
typedef UINT32(*CONSYS_IC_READ_CPUPCR) (VOID);
typedef VOID(*IC_FORCE_TRIGGER_ASSERT_DEBUG_PIN) (VOID);
typedef INT32(*CONSYS_IC_CO_CLOCK_TYPE) (VOID);
typedef P_CONSYS_EMI_ADDR_INFO(*CONSYS_IC_SOC_GET_EMI_PHY_ADD) (VOID);
typedef MTK_WCN_BOOL(*CONSYS_IC_NEED_STORE_PDEV) (VOID);
typedef UINT32(*CONSYS_IC_STORE_PDEV) (struct platform_device *pdev);
typedef UINT32(*CONSYS_IC_STORE_RESET_CONTROL) (struct platform_device *pdev);
typedef MTK_WCN_BOOL(*CONSYS_IC_NEED_GPS) (VOID);
typedef VOID(*CONSYS_IC_SET_IF_PINMUX) (MTK_WCN_BOOL enable);

typedef struct _WMT_CONSYS_IC_OPS_ {
	CONSYS_IC_CLOCK_BUFFER_CTRL consys_ic_clock_buffer_ctrl;
	CONSYS_IC_HW_RESET_BIT_SET consys_ic_hw_reset_bit_set;
	CONSYS_IC_HW_SPM_CLK_GATING_ENABLE consys_ic_hw_spm_clk_gating_enable;
	CONSYS_IC_HW_POWER_CTRL consys_ic_hw_power_ctrl;
	CONSYS_IC_AHB_CLOCK_CTRL consys_ic_ahb_clock_ctrl;
	POLLING_CONSYS_IC_CHIPID polling_consys_ic_chipid;
	CONSYS_IC_ARC_REG_SETTING consys_ic_acr_reg_setting;
	CONSYS_IC_AFE_REG_SETTING consys_ic_afe_reg_setting;
	CONSYS_IC_HW_VCN18_CTRL consys_ic_hw_vcn18_ctrl;
	CONSYS_IC_VCN28_HW_MODE_CTRL consys_ic_vcn28_hw_mode_ctrl;
	CONSYS_IC_HW_VCN28_CTRL consys_ic_hw_vcn28_ctrl;
	CONSYS_IC_HW_WIFI_VCN33_CTRL consys_ic_hw_wifi_vcn33_ctrl;
	CONSYS_IC_HW_BT_VCN33_CTRL consys_ic_hw_bt_vcn33_ctrl;
	CONSYS_IC_SOC_CHIPID_GET consys_ic_soc_chipid_get;
	CONSYS_IC_EMI_MPU_SET_REGION_PROTECTION consys_ic_emi_mpu_set_region_protection;
	CONSYS_IC_EMI_SET_REMAPPING_REG consys_ic_emi_set_remapping_reg;
	IC_BT_WIFI_SHARE_V33_SPIN_LOCK_INIT ic_bt_wifi_share_v33_spin_lock_init;
	CONSYS_IC_CLK_GET_FROM_DTS consys_ic_clk_get_from_dts;
	CONSYS_IC_PMIC_GET_FROM_DTS consys_ic_pmic_get_from_dts;
	CONSYS_IC_READ_IRQ_INFO_FROM_DTS consys_ic_read_irq_info_from_dts;
	CONSYS_IC_READ_REG_FROM_DTS consys_ic_read_reg_from_dts;
	CONSYS_IC_READ_CPUPCR consys_ic_read_cpupcr;
	IC_FORCE_TRIGGER_ASSERT_DEBUG_PIN ic_force_trigger_assert_debug_pin;
	CONSYS_IC_CO_CLOCK_TYPE consys_ic_co_clock_type;
	CONSYS_IC_SOC_GET_EMI_PHY_ADD consys_ic_soc_get_emi_phy_add;
	CONSYS_IC_NEED_STORE_PDEV consys_ic_need_store_pdev;
	CONSYS_IC_STORE_PDEV consys_ic_store_pdev;
	CONSYS_IC_STORE_RESET_CONTROL consys_ic_store_reset_control;
	CONSYS_IC_NEED_GPS consys_ic_need_gps;
	CONSYS_IC_SET_IF_PINMUX consys_ic_set_if_pinmux;
} WMT_CONSYS_IC_OPS, *P_WMT_CONSYS_IC_OPS;
/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
#if CFG_WMT_DUMP_INT_STATUS
extern void mt_irq_dump_status(int irq);
#endif
extern struct CONSYS_BASE_ADDRESS conn_reg;
extern UINT32 gCoClockFlag;
extern EMI_CTRL_STATE_OFFSET mtk_wcn_emi_state_off;
extern CONSYS_EMI_ADDR_INFO mtk_wcn_emi_addr_info;

extern UINT8 __iomem *pEmibaseaddr;
extern phys_addr_t gConEmiPhyBase;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
INT32 mtk_wcn_consys_hw_init(VOID);
INT32 mtk_wcn_consys_hw_deinit(VOID);
INT32 mtk_wcn_consys_hw_pwr_off(UINT32 co_clock_type);
INT32 mtk_wcn_consys_hw_pwr_on(UINT32 co_clock_type);
INT32 mtk_wcn_consys_hw_rst(UINT32 co_clock_type);
INT32 mtk_wcn_consys_hw_bt_paldo_ctrl(UINT32 enable);
INT32 mtk_wcn_consys_hw_wifi_paldo_ctrl(UINT32 enable);
INT32 mtk_wcn_consys_hw_vcn28_ctrl(UINT32 enable);
INT32 mtk_wcn_consys_hw_state_show(VOID);
PUINT8 mtk_wcn_consys_emi_virt_addr_get(UINT32 ctrl_state_offset);
P_CONSYS_EMI_ADDR_INFO mtk_wcn_consys_soc_get_emi_phy_add(VOID);
UINT32 mtk_wcn_consys_read_cpupcr(VOID);
VOID mtk_wcn_force_trigger_assert_debug_pin(VOID);
INT32 mtk_wcn_consys_read_irq_info_from_dts(PINT32 irq_num, PUINT32 irq_flag);

P_WMT_CONSYS_IC_OPS mtk_wcn_get_consys_ic_ops(VOID);
INT32 mtk_wcn_consys_jtag_set_for_mcu(VOID);
#if CONSYS_ENALBE_SET_JTAG
UINT32 mtk_wcn_consys_jtag_flag_ctrl(UINT32 en);
#endif
#ifdef CONSYS_WMT_REG_SUSPEND_CB_ENABLE
UINT32 mtk_wcn_consys_hw_osc_en_ctrl(UINT32 en);
#endif
UINT32 mtk_wcn_consys_soc_chipid(VOID);
#if !defined(CONFIG_MTK_GPIO_LEGACY)
struct pinctrl *mtk_wcn_consys_get_pinctrl(VOID);
#endif
INT32 mtk_wcn_consys_co_clock_type(VOID);
INT32 mtk_wcn_consys_set_dbg_mode(UINT32 flag);
INT32 mtk_wcn_consys_set_dynamic_dump(PUINT32 buf);
INT32 mtk_wdt_swsysret_config(INT32 bit, INT32 set_value);
#endif /* _MTK_WCN_CONSYS_HW_H_ */

