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

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

#ifdef DFT_TAG
#undef DFT_TAG
#endif
#define DFT_TAG "[WMT-CONSYS-HW]"

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "osal_typedef.h"
#include "mtk_wcn_consys_hw.h"
#include <linux/of_reserved_mem.h>

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static INT32 mtk_wmt_probe(struct platform_device *pdev);
static INT32 mtk_wmt_remove(struct platform_device *pdev);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
UINT8 __iomem *pEmibaseaddr;
phys_addr_t gConEmiPhyBase;

P_WMT_CONSYS_IC_OPS wmt_consys_ic_ops;

struct platform_device *g_pdev;

#ifdef CONFIG_OF
const struct of_device_id apwmt_of_ids[] = {
	{.compatible = "mediatek,mt6763-consys",},
	{.compatible = "mediatek,mt6757-consys",},
	{.compatible = "mediatek,mt8167-consys",},
	{.compatible = "mediatek,mt6759-consys",},
	{}
};
struct CONSYS_BASE_ADDRESS conn_reg;
#endif

static struct platform_driver mtk_wmt_dev_drv = {
	.probe = mtk_wmt_probe,
	.remove = mtk_wmt_remove,
	.driver = {
		   .name = "mtk_wmt",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = apwmt_of_ids,
#endif
		   },
};

/* GPIO part */
#if !defined(CONFIG_MTK_GPIO_LEGACY)
struct pinctrl *consys_pinctrl;
#endif

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
INT32 __weak mtk_wcn_consys_jtag_set_for_mcu(VOID)
{
	WMT_PLAT_WARN_FUNC("Does not support on combo\n");
	return 0;
}

#if CONSYS_ENALBE_SET_JTAG
UINT32 __weak mtk_wcn_consys_jtag_flag_ctrl(UINT32 en)
{
	WMT_PLAT_WARN_FUNC("Does not support on combo\n");
	return 0;
}
#endif

#ifdef CONSYS_WMT_REG_SUSPEND_CB_ENABLE
UINT32 __weak mtk_wcn_consys_hw_osc_en_ctrl(UINT32 en)
{
	WMT_PLAT_WARN_FUNC("Does not support on combo\n");
	return 0;
}
#endif

P_WMT_CONSYS_IC_OPS __weak mtk_wcn_get_consys_ic_ops(VOID)
{
	WMT_PLAT_WARN_FUNC("Does not support on combo\n");
	return NULL;
}

static INT32 mtk_wmt_probe(struct platform_device *pdev)
{
	INT32 iRet = -1;

	if (pdev)
		g_pdev = pdev;

	if (wmt_consys_ic_ops->consys_ic_need_store_pdev) {
		if (wmt_consys_ic_ops->consys_ic_need_store_pdev() == MTK_WCN_BOOL_TRUE) {
			if (wmt_consys_ic_ops->consys_ic_store_pdev)
				wmt_consys_ic_ops->consys_ic_store_pdev(pdev);
			pm_runtime_enable(&pdev->dev);
		}
	}

	if (wmt_consys_ic_ops->consys_ic_read_reg_from_dts)
		iRet = wmt_consys_ic_ops->consys_ic_read_reg_from_dts(pdev);
	else
		iRet = -1;

	if (iRet)
		return iRet;
	if (wmt_consys_ic_ops->consys_ic_clk_get_from_dts)
		iRet = wmt_consys_ic_ops->consys_ic_clk_get_from_dts(pdev);
	else
		iRet = -1;
	if (iRet)
		return iRet;
	if (gConEmiPhyBase) {
		if (wmt_consys_ic_ops->consys_ic_emi_mpu_set_region_protection)
			wmt_consys_ic_ops->consys_ic_emi_mpu_set_region_protection();
		if (wmt_consys_ic_ops->consys_ic_emi_set_remapping_reg)
			wmt_consys_ic_ops->consys_ic_emi_set_remapping_reg();
#if 1
		pEmibaseaddr = ioremap_nocache(gConEmiPhyBase + SZ_1M / 2, CONSYS_EMI_MEM_SIZE);
#else
		pEmibaseaddr = ioremap_nocache(CONSYS_EMI_AP_PHY_BASE, CONSYS_EMI_MEM_SIZE);
#endif
		/* pEmibaseaddr = ioremap_nocache(0x80090400,270*KBYTE); */
		if (pEmibaseaddr) {
			WMT_PLAT_INFO_FUNC("EMI mapping OK(0x%p)\n", pEmibaseaddr);
			memset_io(pEmibaseaddr, 0, CONSYS_EMI_MEM_SIZE);
			iRet = 0;
		} else {
			WMT_PLAT_ERR_FUNC("EMI mapping fail\n");
		}
	} else {
		WMT_PLAT_ERR_FUNC("consys emi memory address gConEmiPhyBase invalid\n");
	}

#ifdef CONFIG_MTK_HIBERNATION
	WMT_PLAT_INFO_FUNC("register connsys restore cb for complying with IPOH function\n");
	register_swsusp_restore_noirq_func(ID_M_CONNSYS, mtk_wcn_consys_hw_restore, NULL);
#endif

	if (wmt_consys_ic_ops->ic_bt_wifi_share_v33_spin_lock_init)
		wmt_consys_ic_ops->ic_bt_wifi_share_v33_spin_lock_init();


	if (wmt_consys_ic_ops->consys_ic_pmic_get_from_dts)
		wmt_consys_ic_ops->consys_ic_pmic_get_from_dts(pdev);

#if !defined(CONFIG_MTK_GPIO_LEGACY)
	consys_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(consys_pinctrl)) {
		WMT_PLAT_ERR_FUNC("cannot find consys pinctrl.\n");
		return PTR_ERR(consys_pinctrl);
	}
#endif /* !defined(CONFIG_MTK_GPIO_LEGACY) */

	if (wmt_consys_ic_ops->consys_ic_store_reset_control)
		wmt_consys_ic_ops->consys_ic_store_reset_control(pdev);
	return 0;
}

static INT32 mtk_wmt_remove(struct platform_device *pdev)
{
	if (wmt_consys_ic_ops->consys_ic_need_store_pdev) {
		if (wmt_consys_ic_ops->consys_ic_need_store_pdev() == MTK_WCN_BOOL_TRUE)
			pm_runtime_disable(&pdev->dev);
	}
	if (g_pdev)
		g_pdev = NULL;

	return 0;
}

INT32 mtk_wcn_consys_hw_reg_ctrl(UINT32 on, UINT32 co_clock_type)
{
	INT32 iRet = 0;

	WMT_PLAT_WARN_FUNC("CONSYS-HW-REG-CTRL(0x%08x),start\n", on);

	if (on) {
		WMT_PLAT_DBG_FUNC("++\n");
		if (wmt_consys_ic_ops->consys_ic_set_if_pinmux)
			wmt_consys_ic_ops->consys_ic_set_if_pinmux(ENABLE);

		if (wmt_consys_ic_ops->consys_ic_hw_vcn18_ctrl)
			wmt_consys_ic_ops->consys_ic_hw_vcn18_ctrl(ENABLE);

		udelay(150);

		if (co_clock_type) {
			WMT_PLAT_INFO_FUNC("co clock type(%d),turn on clk buf\n", co_clock_type);
			if (wmt_consys_ic_ops->consys_ic_clock_buffer_ctrl)
				wmt_consys_ic_ops->consys_ic_clock_buffer_ctrl(ENABLE);
		}

		if (co_clock_type) {
			/*if co-clock mode: */
			/*2.set VCN28 to SW control mode (with PMIC_WRAP API) */
			/*turn on VCN28 LDO only when FMSYS is activated"  */
			if (wmt_consys_ic_ops->consys_ic_vcn28_hw_mode_ctrl)
				wmt_consys_ic_ops->consys_ic_vcn28_hw_mode_ctrl(DISABLE);
		} else {
			/*if NOT co-clock: */
			/*2.1.switch VCN28 to HW control mode (with PMIC_WRAP API) */
			/*2.2.turn on VCN28 LDO (with PMIC_WRAP API)" */
			/*fix vcn28 not balance warning */
			if (wmt_consys_ic_ops->consys_ic_vcn28_hw_mode_ctrl)
				wmt_consys_ic_ops->consys_ic_vcn28_hw_mode_ctrl(ENABLE);
			if (wmt_consys_ic_ops->consys_ic_hw_vcn28_ctrl)
				wmt_consys_ic_ops->consys_ic_hw_vcn28_ctrl(ENABLE);
		}

		if (wmt_consys_ic_ops->consys_ic_hw_reset_bit_set)
			wmt_consys_ic_ops->consys_ic_hw_reset_bit_set(ENABLE);
		if (wmt_consys_ic_ops->consys_ic_hw_spm_clk_gating_enable)
			wmt_consys_ic_ops->consys_ic_hw_spm_clk_gating_enable();
		if (wmt_consys_ic_ops->consys_ic_hw_power_ctrl)
			wmt_consys_ic_ops->consys_ic_hw_power_ctrl(ENABLE);

		udelay(10);

		if (wmt_consys_ic_ops->consys_ic_ahb_clock_ctrl)
			wmt_consys_ic_ops->consys_ic_ahb_clock_ctrl(ENABLE);
		if (wmt_consys_ic_ops->polling_consys_ic_chipid)
			wmt_consys_ic_ops->polling_consys_ic_chipid();
		if (wmt_consys_ic_ops->consys_ic_acr_reg_setting)
			wmt_consys_ic_ops->consys_ic_acr_reg_setting();
		if (wmt_consys_ic_ops->consys_ic_afe_reg_setting)
			wmt_consys_ic_ops->consys_ic_afe_reg_setting();
		if (wmt_consys_ic_ops->consys_ic_hw_reset_bit_set)
			wmt_consys_ic_ops->consys_ic_hw_reset_bit_set(DISABLE);

		msleep(20);

	} else {
		if (wmt_consys_ic_ops->consys_ic_ahb_clock_ctrl)
			wmt_consys_ic_ops->consys_ic_ahb_clock_ctrl(DISABLE);
		if (wmt_consys_ic_ops->consys_ic_hw_power_ctrl)
			wmt_consys_ic_ops->consys_ic_hw_power_ctrl(DISABLE);
		if (co_clock_type) {
			if (wmt_consys_ic_ops->consys_ic_clock_buffer_ctrl)
				wmt_consys_ic_ops->consys_ic_clock_buffer_ctrl(DISABLE);
		}

		if (co_clock_type == 0) {
			if (wmt_consys_ic_ops->consys_ic_vcn28_hw_mode_ctrl)
				wmt_consys_ic_ops->consys_ic_vcn28_hw_mode_ctrl(DISABLE);
			/*turn off VCN28 LDO (with PMIC_WRAP API)" */
			if (wmt_consys_ic_ops->consys_ic_hw_vcn28_ctrl)
				wmt_consys_ic_ops->consys_ic_hw_vcn28_ctrl(DISABLE);
		}
		if (wmt_consys_ic_ops->consys_ic_hw_vcn18_ctrl)
			wmt_consys_ic_ops->consys_ic_hw_vcn18_ctrl(DISABLE);
		if (wmt_consys_ic_ops->consys_ic_set_if_pinmux)
			wmt_consys_ic_ops->consys_ic_set_if_pinmux(DISABLE);
	}
	WMT_PLAT_WARN_FUNC("CONSYS-HW-REG-CTRL(0x%08x),finish\n", on);
	return iRet;
}
/*tag4 wujun api big difference end*/

INT32 mtk_wcn_consys_hw_bt_paldo_ctrl(UINT32 enable)
{
	if (wmt_consys_ic_ops->consys_ic_hw_bt_vcn33_ctrl)
		wmt_consys_ic_ops->consys_ic_hw_bt_vcn33_ctrl(enable);
	return 0;
}

INT32 mtk_wcn_consys_hw_wifi_paldo_ctrl(UINT32 enable)
{
	if (wmt_consys_ic_ops->consys_ic_hw_wifi_vcn33_ctrl)
		wmt_consys_ic_ops->consys_ic_hw_wifi_vcn33_ctrl(enable);
	return 0;
}

INT32 mtk_wcn_consys_hw_vcn28_ctrl(UINT32 enable)
{
	if (wmt_consys_ic_ops->consys_ic_hw_vcn28_ctrl)
		wmt_consys_ic_ops->consys_ic_hw_vcn28_ctrl(enable);
	if (enable)
		WMT_PLAT_INFO_FUNC("turn on vcn28 for fm/gps usage in co-clock mode\n");
	else
		WMT_PLAT_INFO_FUNC("turn off vcn28 for fm/gps usage in co-clock mode\n");
	return 0;
}

UINT32 mtk_wcn_consys_soc_chipid(VOID)
{
	if (wmt_consys_ic_ops == NULL)
		wmt_consys_ic_ops = mtk_wcn_get_consys_ic_ops();

	if (wmt_consys_ic_ops->consys_ic_soc_chipid_get)
		return wmt_consys_ic_ops->consys_ic_soc_chipid_get();
	else
		return 0;
}

#if !defined(CONFIG_MTK_GPIO_LEGACY)
struct pinctrl *mtk_wcn_consys_get_pinctrl()
{
	return consys_pinctrl;
}
#endif

INT32 mtk_wcn_consys_hw_gpio_ctrl(UINT32 on)
{
	INT32 iRet = 0;

	WMT_PLAT_DBG_FUNC("CONSYS-HW-GPIO-CTRL(0x%08x), start\n", on);

	if (on) {

		if (wmt_consys_ic_ops->consys_ic_need_gps) {
			if (wmt_consys_ic_ops->consys_ic_need_gps() == MTK_WCN_BOOL_TRUE) {
				/*if external modem used,GPS_SYNC still needed to control */
				iRet += wmt_plat_gpio_ctrl(PIN_GPS_SYNC, PIN_STA_INIT);
				iRet += wmt_plat_gpio_ctrl(PIN_GPS_LNA, PIN_STA_INIT);

				iRet += wmt_plat_gpio_ctrl(PIN_I2S_GRP, PIN_STA_INIT);
			}
		} else {
			iRet += wmt_plat_gpio_ctrl(PIN_GPS_SYNC, PIN_STA_INIT);
			iRet += wmt_plat_gpio_ctrl(PIN_GPS_LNA, PIN_STA_INIT);

			iRet += wmt_plat_gpio_ctrl(PIN_I2S_GRP, PIN_STA_INIT);
		}
		/* TODO: [FixMe][GeorgeKuo] double check if BGF_INT is implemented ok */
		/* iRet += wmt_plat_gpio_ctrl(PIN_BGF_EINT, PIN_STA_MUX); */
		iRet += wmt_plat_eirq_ctrl(PIN_BGF_EINT, PIN_STA_INIT);
		iRet += wmt_plat_eirq_ctrl(PIN_BGF_EINT, PIN_STA_EINT_DIS);
		WMT_PLAT_DBG_FUNC("CONSYS-HW, BGF IRQ registered and disabled\n");

	} else {

		/* set bgf eint/all eint to deinit state, namely input low state */
		iRet += wmt_plat_eirq_ctrl(PIN_BGF_EINT, PIN_STA_EINT_DIS);
		iRet += wmt_plat_eirq_ctrl(PIN_BGF_EINT, PIN_STA_DEINIT);
		WMT_PLAT_DBG_FUNC("CONSYS-HW, BGF IRQ unregistered and disabled\n");
		/* iRet += wmt_plat_gpio_ctrl(PIN_BGF_EINT, PIN_STA_DEINIT); */
		if (wmt_consys_ic_ops->consys_ic_need_gps) {
			if (wmt_consys_ic_ops->consys_ic_need_gps() == MTK_WCN_BOOL_TRUE) {
				/*if external modem used,GPS_SYNC still needed to control */
				iRet += wmt_plat_gpio_ctrl(PIN_GPS_SYNC, PIN_STA_DEINIT);
				iRet += wmt_plat_gpio_ctrl(PIN_I2S_GRP, PIN_STA_DEINIT);
				/* deinit gps_lna */
				iRet += wmt_plat_gpio_ctrl(PIN_GPS_LNA, PIN_STA_DEINIT);
			}
		} else {
			iRet += wmt_plat_gpio_ctrl(PIN_GPS_SYNC, PIN_STA_DEINIT);
			iRet += wmt_plat_gpio_ctrl(PIN_I2S_GRP, PIN_STA_DEINIT);
			iRet += wmt_plat_gpio_ctrl(PIN_GPS_LNA, PIN_STA_DEINIT);
		}
	}
	WMT_PLAT_DBG_FUNC("CONSYS-HW-GPIO-CTRL(0x%08x), finish\n", on);
	return iRet;

}

INT32 mtk_wcn_consys_hw_pwr_on(UINT32 co_clock_type)
{
	INT32 iRet = 0;

	WMT_PLAT_INFO_FUNC("CONSYS-HW-PWR-ON, start\n");
	if (!gConEmiPhyBase) {
		WMT_PLAT_ERR_FUNC("EMI base address is invalid, CONNSYS can not be powered on!");
		WMT_PLAT_ERR_FUNC("To avoid the occurrence of KE!\n");
		return -1;
	}
	iRet += mtk_wcn_consys_hw_reg_ctrl(1, co_clock_type);
	iRet += mtk_wcn_consys_hw_gpio_ctrl(1);
	mtk_wcn_consys_jtag_set_for_mcu();

	WMT_PLAT_INFO_FUNC("CONSYS-HW-PWR-ON, finish(%d)\n", iRet);
	return iRet;
}

INT32 mtk_wcn_consys_hw_pwr_off(UINT32 co_clock_type)
{
	INT32 iRet = 0;

	WMT_PLAT_INFO_FUNC("CONSYS-HW-PWR-OFF, start\n");

	iRet += mtk_wcn_consys_hw_reg_ctrl(0, co_clock_type);
	iRet += mtk_wcn_consys_hw_gpio_ctrl(0);

	WMT_PLAT_INFO_FUNC("CONSYS-HW-PWR-OFF, finish(%d)\n", iRet);
	return iRet;
}

INT32 mtk_wcn_consys_hw_rst(UINT32 co_clock_type)
{
	INT32 iRet = 0;

	WMT_PLAT_INFO_FUNC("CONSYS-HW, hw_rst start, eirq should be disabled before this step\n");

	/*1. do whole hw power off flow */
	iRet += mtk_wcn_consys_hw_reg_ctrl(0, co_clock_type);

	/*2. do whole hw power on flow */
	iRet += mtk_wcn_consys_hw_reg_ctrl(1, co_clock_type);

	WMT_PLAT_INFO_FUNC("CONSYS-HW, hw_rst finish, eirq should be enabled after this step\n");
	return iRet;
}

INT32 mtk_wcn_consys_hw_state_show(VOID)
{
	return 0;
}

INT32 mtk_wcn_consys_hw_restore(struct device *device)
{
	if (gConEmiPhyBase) {
		if (wmt_consys_ic_ops->consys_ic_emi_mpu_set_region_protection)
			wmt_consys_ic_ops->consys_ic_emi_mpu_set_region_protection();
		if (wmt_consys_ic_ops->consys_ic_emi_set_remapping_reg)
			wmt_consys_ic_ops->consys_ic_emi_set_remapping_reg();
#if 1
		pEmibaseaddr = ioremap_nocache(gConEmiPhyBase + SZ_1M / 2, CONSYS_EMI_MEM_SIZE);
#else
		pEmibaseaddr = ioremap_nocache(CONSYS_EMI_AP_PHY_BASE, CONSYS_EMI_MEM_SIZE);
#endif
		if (pEmibaseaddr) {
			WMT_PLAT_WARN_FUNC("EMI mapping OK(0x%p)\n", pEmibaseaddr);
			memset_io(pEmibaseaddr, 0, CONSYS_EMI_MEM_SIZE);
		} else {
			WMT_PLAT_ERR_FUNC("EMI mapping fail\n");
		}
	} else {
		WMT_PLAT_ERR_FUNC("consys emi memory address gConEmiPhyBase invalid\n");
	}

	return 0;
}

/*Reserved memory by device tree!*/
int reserve_memory_consys_fn(struct reserved_mem *rmem)
{
	WMT_PLAT_WARN_FUNC(" name: %s, base: 0x%llx, size: 0x%llx\n", rmem->name,
			   (unsigned long long)rmem->base, (unsigned long long)rmem->size);
	gConEmiPhyBase = rmem->base;
	return 0;
}

RESERVEDMEM_OF_DECLARE(reserve_memory_test, "mediatek,consys-reserve-memory", reserve_memory_consys_fn);


INT32 mtk_wcn_consys_hw_init(VOID)
{
	INT32 iRet = -1;

	if (wmt_consys_ic_ops == NULL)
		wmt_consys_ic_ops = mtk_wcn_get_consys_ic_ops();


	iRet = platform_driver_register(&mtk_wmt_dev_drv);
	if (iRet)
		WMT_PLAT_ERR_FUNC("WMT platform driver registered failed(%d)\n", iRet);


	return iRet;

}

INT32 mtk_wcn_consys_hw_deinit(VOID)
{
	if (pEmibaseaddr) {
		iounmap(pEmibaseaddr);
		pEmibaseaddr = NULL;
	}
#ifdef CONFIG_MTK_HIBERNATION
	unregister_swsusp_restore_noirq_func(ID_M_CONNSYS);
#endif

	platform_driver_unregister(&mtk_wmt_dev_drv);

	if (wmt_consys_ic_ops)
		wmt_consys_ic_ops = NULL;

	return 0;
}

PUINT8 mtk_wcn_consys_emi_virt_addr_get(UINT32 ctrl_state_offset)
{
	UINT8 *p_virtual_addr = NULL;

	if (!pEmibaseaddr) {
		WMT_PLAT_ERR_FUNC("EMI base address is NULL\n");
		return NULL;
	}
	WMT_PLAT_DBG_FUNC("ctrl_state_offset(%08x)\n", ctrl_state_offset);
	p_virtual_addr = pEmibaseaddr + ctrl_state_offset;

	return p_virtual_addr;
}

INT32 mtk_wcn_consys_set_dbg_mode(UINT32 flag)
{
	INT32 ret = -1;
	PUINT8 vir_addr = NULL;

	vir_addr = mtk_wcn_consys_emi_virt_addr_get(EXP_APMEM_CTRL_CHIP_FW_DBGLOG_MODE);
	if (!vir_addr) {
		WMT_PLAT_ERR_FUNC("get vir address fail\n");
		return -2;
	}
	if (flag) {
		ret = 0;
		CONSYS_REG_WRITE(vir_addr, 0x1);
	} else {
		CONSYS_REG_WRITE(vir_addr, 0x0);
	}
	WMT_PLAT_ERR_FUNC("fw dbg mode register value(0x%08x)\n", CONSYS_REG_READ(vir_addr));
	return ret;
}

INT32 mtk_wcn_consys_set_dynamic_dump(PUINT32 str_buf)
{
	PUINT8 vir_addr = NULL;

	vir_addr = mtk_wcn_consys_emi_virt_addr_get(EXP_APMEM_CTRL_CHIP_DYNAMIC_DUMP);
	if (!vir_addr) {
		WMT_PLAT_ERR_FUNC("get vir address fail\n");
		return -2;
	}
	memcpy(vir_addr, str_buf, DYNAMIC_DUMP_GROUP_NUM*8);
	WMT_PLAT_INFO_FUNC("dynamic dump register value(0x%08x)\n", CONSYS_REG_READ(vir_addr));
	return 0;
}

INT32 mtk_wcn_consys_co_clock_type(VOID)
{
	if (wmt_consys_ic_ops == NULL)
		wmt_consys_ic_ops = mtk_wcn_get_consys_ic_ops();

	if (wmt_consys_ic_ops->consys_ic_co_clock_type)
		return wmt_consys_ic_ops->consys_ic_co_clock_type();
	else
		return -1;
}

P_CONSYS_EMI_ADDR_INFO mtk_wcn_consys_soc_get_emi_phy_add(VOID)
{
	if (wmt_consys_ic_ops->consys_ic_soc_get_emi_phy_add)
		return wmt_consys_ic_ops->consys_ic_soc_get_emi_phy_add();
	else
		return NULL;
}

UINT32 mtk_wcn_consys_read_cpupcr(VOID)
{
	if (wmt_consys_ic_ops->consys_ic_read_cpupcr)
		return wmt_consys_ic_ops->consys_ic_read_cpupcr();
	else
		return 0;
}

VOID mtk_wcn_force_trigger_assert_debug_pin(VOID)
{
	if (wmt_consys_ic_ops->ic_force_trigger_assert_debug_pin)
		wmt_consys_ic_ops->ic_force_trigger_assert_debug_pin();
}

INT32 mtk_wcn_consys_read_irq_info_from_dts(PINT32 irq_num, PUINT32 irq_flag)
{
	if (wmt_consys_ic_ops->consys_ic_read_irq_info_from_dts)
		return wmt_consys_ic_ops->consys_ic_read_irq_info_from_dts(g_pdev, irq_num, irq_flag);
	else
		return 0;
}
