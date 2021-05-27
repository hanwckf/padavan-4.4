/**
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
**/
#include <linux/jiffies.h>
#include "mtk_ir_common.h"
#include "mtk_ir_core.h"
#include "mtk_ir_regs.h" /* include ir registers */
#include "mtk_ir_cus_rc6.h"	/* include customer's key map */

#define MTK_RC6_BITCNT   0x1e
#define MTK_RC6_LEADER   0x8
#define MTK_RC6_TOGGLE0  0x1
#define MTK_RC6_TOGGLE1  0x2
/*#define MTK_IR_RC6_CUSTOMER_CODE   0x46*/

/* set deglitch with the min number. when glitch < (33*6 = 198us,ignore) */

#define RC6_INFO_TO_BITCNT(u4Info)      ((u4Info & IRRX_CH_BITCNT_MASK)    >> IRRX_CH_BITCNT_BITSFT)

#define MTK_RC6_GET_LEADER(bdata0) ((bdata0>>4))
#define MTK_RC6_GET_TOGGLE(bdata0) ((bdata0 & 0xc)>>2)
#define MTK_RC6_GET_CUSTOM(bdata0, bdata1) (((bdata0 & 0x3) << 6) | bdata1 >> 2)
#define MTK_RC6_GET_KEYCODE(bdata1, bdata2)  \
		(((bdata2>>2) | ((bdata1 & 0x3)<<6)) & 0xff)


/*----------------------------------------------------------------------------*/
#define IR_RC6_DEBUG	1
#define IR_RC6_TAG                  "[IRRX_RC6] "
#define IR_RC6_FUN(f) \
	do { \
		if (ir_log_debug_on) \
			pr_err(IR_RC6_TAG"%s++++\n", __func__); \
		else \
			pr_debug(IR_RC6_TAG"%s++++\n", __func__); \
	} while (0)

#define IR_RC6_LOG(fmt, args...) \
	do { \
		if (ir_log_debug_on) \
			pr_err(IR_RC6_TAG fmt, ##args); \
		else if (IR_RC6_DEBUG) \
			pr_debug(IR_RC6_TAG fmt, ##args); \
	} while (0)

#define IR_RC6_ERR(fmt, args...)    pr_err(MTK_IR_TAG"%s %d : "fmt, __func__, __LINE__, ##args)

static int mtk_ir_rc6_enable_hwirq(int enable);
static int mtk_ir_rc6_init_hw(void);
static int mtk_ir_rc6_uninit_hw(void);
static u32 mtk_ir_rc6_decode(void *preserve);

/* Maintain  cust info here */
struct mtk_ir_hw ir_rc6_cust;
static struct mtk_ir_hw *hw = &ir_rc6_cust;

static u32 detect_hung_timers;
static u32 _u4PrevKey = BTN_NONE;	/* pre key */
static u32 _u4Rc6_customer_code = MTK_IR_RC6_CUSTOMER_CODE;
#define RC6_REPEAT_MS 200
#define RC6_REPEAT_MS_MIN 60

static struct rc_map_list mtk_rc6_map = {
	.map = {
		.scan = mtk_rc6_table,	/* here for custom to modify */
		.size = ARRAY_SIZE(mtk_rc6_table),
		.rc_type = RC_TYPE_RC6_0,
		.name = RC_MAP_MTK_RC6,
		}
};

static u32 mtk_ir_rc6_get_customer_code(void)
{
	return _u4Rc6_customer_code;
}

static u32 mtk_ir_rc6_set_customer_code(u32 data)
{
	_u4Rc6_customer_code = data;
	IR_RC6_LOG("_u4Rc6_customer_code = 0x%x\n", _u4Rc6_customer_code);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mtk_ir_rc6_early_suspend(void *preserve)
{
	IR_RC6_FUN();
}

static void mtk_ir_rc6_late_resume(void *preserve)
{
	IR_RC6_FUN();
}

#else

#define mtk_ir_rc6_early_suspend NULL
#define mtk_ir_rc6_late_resume NULL

#endif


#ifdef CONFIG_PM_SLEEP
static int mtk_ir_rc6_suspend(void *preserve)
{
	IR_RC6_FUN();
	return 0;
}

static int mtk_ir_rc6_resume(void *preserve)
{
	IR_RC6_FUN();
	return 0;
}

#else

#define mtk_ir_rc6_suspend NULL
#define mtk_ir_rc6_resume NULL

#endif

static int mtk_ir_rc6_enable_hwirq(int enable)
{
	u32 info;

	IR_RC6_LOG("IRRX enable hwirq: %d\n", enable);
	if (enable) {
		info = IR_READ32(IRRX_COUNT_HIGH_REG);
		IR_WRITE_MASK(IRRX_IRCLR, IRRX_IRCLR_MASK,
						IRRX_IRCLR_OFFSET, 0x1);	/* clear irrx state machine */
		dsb(sy);
		IR_WRITE_MASK(IRRX_IRINT_CLR, IRRX_INTCLR_MASK,
						IRRX_INTCLR_OFFSET, 0x1);	/* clear irrx eint stat */
		dsb(sy);

		do {
			info = IR_READ32(IRRX_COUNT_HIGH_REG);
		} while (info != 0);

		/* enable ir hw interrupt receiver function */
		IR_WRITE_MASK(IRRX_IRINT_EN, IRRX_INTEN_MASK, IRRX_INTCLR_OFFSET, 0x1);
		dsb(sy);
	} else {
		/* disable ir hw interrupt receiver function */
		IR_WRITE_MASK(IRRX_IRINT_EN, IRRX_INTEN_MASK, IRRX_INTCLR_OFFSET, 0x0);
		dsb(sy);
		IR_WRITE_MASK(IRRX_IRINT_CLR, IRRX_INTCLR_MASK,
						IRRX_INTCLR_OFFSET, 0x1);	/* clear irrx eint stat */
		dsb(sy);
	}
	return 0;
}

/**
* This timer function is for RC6 routine work.
* You can add stuff you want to do in this function.
**/
static int mtk_ir_rc6_timer_func(const char *data)
{
	u32 CHK_CNT_PULSE = 0;

	CHK_CNT_PULSE = (IR_READ32(IRRX_EXPBCNT) >> IRRX_IRCHK_CNT_OFFSET) & IRRX_IRCHK_CNT;
	if (CHK_CNT_PULSE == IRRX_IRCHK_CNT) {
		detect_hung_timers++;
		if (data[0])
			IR_RC6_ERR("CHK_CNT_PULSE = 0x%x,detect_hung_timers = %d\n",
			     CHK_CNT_PULSE, detect_hung_timers);
		return mtk_ir_rc6_enable_hwirq(1);
	}
	return 0;
}

static struct mtk_ir_core_platform_data mtk_ir_pdata_rc6 = {
	.input_name = MTK_INPUT_DEVICE_NAME,
	.p_map_list = &mtk_rc6_map,
	.i4_keypress_timeout = MTK_IR_RC6_KEYPRESS_TIMEOUT,
	.enable_hwirq = mtk_ir_rc6_enable_hwirq,
	.init_hw = mtk_ir_rc6_init_hw,
	.uninit_hw = mtk_ir_rc6_uninit_hw,
	.ir_hw_decode = mtk_ir_rc6_decode,
	.get_customer_code = mtk_ir_rc6_get_customer_code,
	.set_customer_code = mtk_ir_rc6_set_customer_code,
	.timer_func = mtk_ir_rc6_timer_func,
	.mouse_step = {
		MOUSE_SMALL_X_STEP,
		MOUSE_SMALL_Y_STEP,
		MOUSE_LARGE_X_STEP,
		MOUSE_LARGE_Y_STEP
	},
#ifdef CONFIG_HAS_EARLYSUSPEND
	.early_suspend = mtk_ir_rc6_early_suspend,
	.late_resume = mtk_ir_rc6_late_resume,
#endif

#ifdef CONFIG_PM_SLEEP
	.suspend = mtk_ir_rc6_suspend,
	.resume = mtk_ir_rc6_resume,
#endif
};

static int mtk_ir_rc6_uninit_hw(void)
{
	/* disable ir interrupt */
	IR_WRITE_MASK(IRRX_IRINT_EN, IRRX_INTEN_MASK, IRRX_INTCLR_OFFSET, 0x0);
	mtk_ir_rc6_enable_hwirq(0);
	rc_map_unregister(&mtk_rc6_map);
	return 0;
}


static int mtk_ir_rc6_init_hw(void)
{
	MTK_IR_MODE ir_mode = mtk_ir_core_getmode();
	struct rc_map_table *p_table = NULL;
	int size = 0;
	int i = 0;

	IR_RC6_FUN();
	IR_RC6_LOG(" ir_mode = %d\n", ir_mode);

	if (ir_mode == MTK_IR_FACTORY) {
		mtk_rc6_map.map.scan = mtk_rc6_factory_table;
		mtk_rc6_map.map.size = ARRAY_SIZE(mtk_rc6_factory_table);
	} else {
		mtk_rc6_map.map.scan = mtk_rc6_table;
		mtk_rc6_map.map.size = ARRAY_SIZE(mtk_rc6_table);
		p_table = mtk_rc6_map.map.scan;
		size = mtk_rc6_map.map.size;

		memset(&(mtk_ir_pdata_rc6.mouse_code), 0xff, sizeof(mtk_ir_pdata_rc6.mouse_code));
		for (; i < size; i++) {
			if (p_table[i].keycode == KEY_LEFT)
				mtk_ir_pdata_rc6.mouse_code.scanleft = p_table[i].scancode;
			else if (p_table[i].keycode == KEY_RIGHT)
				mtk_ir_pdata_rc6.mouse_code.scanright = p_table[i].scancode;
			else if (p_table[i].keycode == KEY_UP)
				mtk_ir_pdata_rc6.mouse_code.scanup = p_table[i].scancode;
			else if (p_table[i].keycode == KEY_DOWN)
				mtk_ir_pdata_rc6.mouse_code.scandown = p_table[i].scancode;
			else if (p_table[i].keycode == KEY_ENTER)
				mtk_ir_pdata_rc6.mouse_code.scanenter = p_table[i].scancode;
		}
		mtk_ir_pdata_rc6.mouse_support = MTK_IR_SUPPORT_MOUSE_INPUT;
		mtk_ir_pdata_rc6.mouse_code.scanswitch = MTK_IR_MOUSE_RC6_SWITCH_CODE;
		mtk_ir_pdata_rc6.mousename[0] = '\0';
		strcat(mtk_ir_pdata_rc6.mousename, mtk_ir_pdata_rc6.input_name);
		strcat(mtk_ir_pdata_rc6.mousename, "_Mouse");
		mtk_ir_mouse_set_device_mode(MTK_IR_MOUSE_MODE_DEFAULT);
	}

	rc_map_register(&mtk_rc6_map);
	IR_RC6_LOG(" rc_map_register ok.\n");

	/* first setting power key// */
	IR_WRITE32(IRRX_EXP_IRM1, MTK_RC6_EXP_POWE_KEY1);
	IR_WRITE32(IRRX_EXP_IRL1, MTK_RC6_EXP_POWE_KEY2);

	/* disable interrupt */
	mtk_ir_rc6_enable_hwirq(0);
  /*IR_WRITE32(IRRX_IRCKSEL, IRRX_IRCK_DIV8);*/
	IR_WRITE32(IRRX_CONFIG_HIGH_REG, MTK_RC6_CONFIG);
	IR_WRITE32(IRRX_CONFIG_LOW_REG, MTK_RC6_SAPERIOD);
	IR_WRITE32(IRRX_THRESHOLD_REG, MTK_RC6_THRESHOLD);
	IR_RC6_LOG(" config:0x%08x %08x, threshold:0x%08x\n", IR_READ32(IRRX_CONFIG_HIGH_REG),
		   IR_READ32(IRRX_CONFIG_LOW_REG), IR_READ32(IRRX_THRESHOLD_REG));
	/* mtk_ir_rc6_enable_hwirq(1); */

	IR_RC6_LOG("mtk_ir_rc6_init_hw----\n");
	return 0;
}

u32 mtk_ir_rc6_decode(void *preserve)
{
	u32 _au4IrRxData[2];
	u32 u4BitCnt, u2RC6Leader, u2RC6Custom, u2RC6Toggle, u4RC6key;
	static unsigned long last_jiffers;	/* this is only the patch for RC6 disturbed repeat key */
	unsigned long current_jiffers = jiffies;
	char *pu1Data = (char *)_au4IrRxData;
	u32 _u4Info = IR_READ32(IRRX_COUNT_HIGH_REG);

	_au4IrRxData[0] = IR_READ32(IRRX_COUNT_MID_REG);	/* RC6 's code data is in this register */
	_au4IrRxData[1] = IR_READ32(IRRX_COUNT_LOW_REG);


	u4BitCnt = RC6_INFO_TO_BITCNT(_u4Info);
	u2RC6Leader = MTK_RC6_GET_LEADER(pu1Data[0]);
	u2RC6Custom = MTK_RC6_GET_CUSTOM(pu1Data[0], pu1Data[1]);
	u2RC6Toggle = MTK_RC6_GET_TOGGLE(pu1Data[0]);
	u4RC6key = MTK_RC6_GET_KEYCODE(pu1Data[1], pu1Data[2]);

	if ((_au4IrRxData[0] != 0) || (_au4IrRxData[1] != 0) || (_u4Info != 0)) {
		IR_RC6_LOG("IRRX Info:0x%08x data:0x%08x %08x\n", _u4Info, _au4IrRxData[1], _au4IrRxData[0]);
	} else {
		IR_RC6_ERR("invalid key!!!\n");
		return BTN_INVALID_KEY; }

	IR_RC6_LOG("Bitcnt: 0x%02x, Leader: 0x%02x, Custom: 0x%02x, Toggle: 0x%02x, Keycode; 0x%02x\n",
			 u4BitCnt, u2RC6Leader, u2RC6Custom, u2RC6Toggle, u4RC6key);


	/* Check invalid pulse. */
	if ((u4BitCnt != MTK_RC6_BITCNT) || (u2RC6Leader != MTK_RC6_LEADER)) {
		IR_RC6_ERR("u4BitCnt(%d), should be(%d)!!!\n", u4BitCnt, MTK_RC6_BITCNT);
		IR_RC6_ERR("u2RC6Leader(%d), should be(%d)!!!\n", u2RC6Leader, MTK_RC6_LEADER);
		_u4PrevKey = BTN_NONE;
		goto end;
	}

	if (u2RC6Custom != _u4Rc6_customer_code) {
		IR_RC6_LOG("invalid customer code 0x%x!!!", u2RC6Custom);
		_u4PrevKey = BTN_NONE;
		goto end;
	}

	_u4PrevKey = u4RC6key;

end:

	IR_RC6_LOG("repeat_out=%dms\n", jiffies_to_msecs(current_jiffers - last_jiffers));
	last_jiffers = current_jiffers;

	IR_RC6_LOG("Customer ID:0x%08x, Key:0x%08x\n", u2RC6Custom, _u4PrevKey);
	return _u4PrevKey;

}

static int ir_rc6_local_init(void)
{
	int err = 0;

	IR_RC6_FUN();
	/* err += mtk_ir_rc6_init_hw(); //do not init HW herer, it'll run in mtk_ir_core_probe */
	err += mtk_ir_register_ctl_data_path(&mtk_ir_pdata_rc6, hw, RC_TYPE_RC6_0);
	IR_RC6_LOG("ir_rc6_local_init(%d)----\n", err);
	return err;
}

static int ir_rc6_remove(void)
{
	int err = 0;
	/* err += mtk_ir_rc6_uninit_hw(); */
	return err;
}

static struct mtk_ir_init_info ir_rc6_init_info = {
	.name = "ir_rc6",
	.init = ir_rc6_local_init,
	.uninit = ir_rc6_remove,
};


/*----------------------------------------------------------------------------*/
static int __init ir_rc6_init(void)
{
	IR_RC6_FUN();
	hw = get_mtk_ir_cus_dts(hw);
	if (hw == NULL)
		IR_RC6_ERR("get dts info fail\n");

	mtk_ir_driver_add(&ir_rc6_init_info);
	IR_RC6_LOG("ir_rc6_init----\n");
	return 0;
}

/*----------------------------------------------------------------------------*/
static void __exit ir_rc6_exit(void)
{
	IR_RC6_FUN();
}

/*----------------------------------------------------------------------------*/
module_init(ir_rc6_init);
module_exit(ir_rc6_exit);
/*----------------------------------------------------------------------------*/
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("mtk ir_rc6 driver");
MODULE_AUTHOR("Zhimin.Tang@mediatek.com");
