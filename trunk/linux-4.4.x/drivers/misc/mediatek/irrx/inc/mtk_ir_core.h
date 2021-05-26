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
#ifndef __MTK_IR_RECV_H__
#define __MTK_IR_RECV_H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <generated/autoconf.h>
#include <linux/kobject.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/fb.h>

#include <media/rc-core.h>
#include <media/lirc_dev.h>
/* #include <linux/rtpm_prio.h>//  scher_rr */
#include <linux/kthread.h>
#include <linux/delay.h>

#include <linux/clk.h>
/* #include <mt-plat/sync_write.h> */

#ifdef FINAL_SOLUTION_ /*zhimin...*/
#include <mt-plat/mt_boot_common.h>
#endif
#include "mtk_ir_common.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define IR_BUS		BUS_HOST
#define IR_VERSION	11
#define IR_PRODUCT	11
#define IR_VENDOR	11


#define RC_MAP_MTK_NEC			"rc_map_mtk_nec"
#define RC_MAP_MTK_RC6			"rc_map_mtk_rc6"
#define RC_MAP_MTK_RC5			"rc_map_mtk_rc5"
#define RC_MAP_MTK_SIRC			"rc_map_mtk_sirc"
#define RC_MAP_MTK_RSTEP		"rc_map_mtk_rstep"
#define RC_MAP_MTK_RCMM			"rc_map_mtk_rcmm"

#define MTK_IR_DRIVER_NAME		"mtk_ir"
#define MTK_IR_MISC_DEV_NAME	"m_mtk_ir_misc"
#define MTK_IR_LIRC_DEV_NAME	"mtk_ir_core_lirc"

#define MTK_INPUT_DEVICE_NAME	"MTK_Remote_Controller"	/* here is for input device name */
/**
*only one input device: "MTK_Remote_Controller"
*#define MTK_INPUT_NEC_DEVICE_NAME	"NEC_Remote_Controller"
*#define MTK_INPUT_RC6_DEVICE_NAME	"RC6_Remote_Controller"
*#define MTK_INPUT_RC5_DEVICE_NAME	"RC5_Remote_Controller"
*#define MTK_INPUT_SIRC_DEVICE_NAME	"SIRC_Remote_Controller"
*#define MTK_INPUT_RSTEP_DEVICE_NAME	"RSTEP_Remote_Controller"
**/

#define SEMA_STATE_LOCK   (0)
#define SEMA_STATE_UNLOCK (1)

#define SEMA_LOCK_OK                    ((int)   0)
#define SEMA_LOCK_TIMEOUT               ((int)  -1)
#define SEMA_LOCK_FAIL                  ((int)  -2)

#define MAX_CHOOSE_IR_NUM	3
#define MAX_IR_KEY_NUM	64

struct mtk_ir_hw {
	u32 irrx_chip_id;
	void __iomem *irrx_base_addr;	/* irrx base addr */
	u32 irrx_irq;
	u32 irrx_irq_type;
	struct clk *irrx_clk;
};

struct mtk_ir_init_info {
	char *name;
	int (*init)(void);
	int (*uninit)(void);
	struct platform_driver *platform_diver_addr;
};

struct mtk_ir_context {
	bool irq_register;	/* whether ir irq has been registered */
	struct rc_dev *rcdev;	/* current rcdev */
	struct lirc_driver *drv;	/* lirc_driver */
	/* struct platform_device *dev_current; // current activing device */
	struct task_struct *k_thread;	/* input thread */
	struct input_dev *p_devmouse;	/* IR as mouse */
	/* ///add */
	const char *name;
	enum rc_type protocol;
	atomic_t delay;		/*polling period for reporting input event */
	struct hrtimer hrTimer;
	ktime_t target_ktime;
	struct work_struct report;
	struct work_struct report_irq; /*For ir irq*/
	struct workqueue_struct *mtk_ir_workqueue;
	/* struct miscdevice   mdev; */
	struct mtk_ir_hw *hw;
	struct mtk_ir_core_platform_data *mtk_ir_ctl_data;
};
/**
*struct mtk_ir_device {
*	struct list_head list;
*	struct platform_device dev_platform;
*};
**/
struct mtk_ir_core_platform_data {
	const char *input_name;	/* /proc/bus/devices/input/input_name */
	struct rc_map_list *p_map_list;	/* rc map list */
	int i4_keypress_timeout;
	int (*enable_hwirq)(int enable);
	int (*init_hw)(void);	/* init ir_hw */
	int (*uninit_hw)(void);	/* uint ir_hw */
	 u32 (*ir_hw_decode)(void *preserve);	/* decode function. preserve for future use */
	 u32 (*get_customer_code)(void);	/* get customer code function */
	 u32 (*set_customer_code)(u32 data);	/* set customer code function */
	int (*timer_func)(const char *data);

	void (*early_suspend)(void *preserve);
	void (*late_resume)(void *preserve);

	int (*suspend)(void *preserve);
	int (*resume)(void *preserve);

	int mouse_support;
	struct mtk_ir_mouse_code mouse_code;	/* IRRX as mouse code */
	struct mtk_ir_mouse_step mouse_step;
	char mousename[32];
};

extern struct mtk_ir_context *mtk_ir_context_obj;
extern struct mtk_ir_core_platform_data *get_mtk_ir_ctl_data(void);
extern int mtk_ir_register_ctl_data_path(struct mtk_ir_core_platform_data *ctl_data,
					 struct mtk_ir_hw *hw, enum rc_type protocol);
extern struct mtk_ir_hw *get_mtk_ir_cus_dts(struct mtk_ir_hw *hw);
extern int mtk_ir_driver_add(struct mtk_ir_init_info *obj);

extern u32 IR_READ32(u32 u4Addr);
extern void IR_WRITE32(u32 u4Addr, u32 u4Val);
extern void IR_WRITE_MASK(u32 u4Addr, u32 u4Mask, u32 u4Offset, u32 u4Val);
extern u32 IR_READ_MASK(u32 u4Addr, u32 u4Mask, u32 u4Offset);

extern void release(struct device *dev);
extern int mtk_ir_core_create_thread(int (*threadfn) (void *data),
				     void *data,
				     const char *ps_name,
				     struct task_struct **p_struct_out, unsigned int ui4_priority);

extern void mtk_ir_core_send_scancode(u32 scancode);
extern void mtk_ir_core_send_mapcode(u32 mapcode, int stat);

extern void mtk_ir_core_send_mapcode_auto_up(u32 mapcode, u32 ms);

extern void mtk_ir_core_get_msg_by_scancode(u32 scancode, struct mtk_ir_msg *msg);
extern MTK_IR_MODE mtk_ir_core_getmode(void);
/*extern void rc_set_keypress_timeout(int i4value);*/

extern MTK_IR_DEVICE_MODE mtk_ir_mouse_get_device_mode(void);
extern void mtk_ir_mouse_set_device_mode(MTK_IR_DEVICE_MODE devmode);
extern struct input_dev *mtk_ir_mouse_register_input(struct platform_device *pdev);
extern int mtk_ir_mouse_proc_key(u32 scancode, struct mtk_ir_context *p_mtk_rc_core);
extern void mtk_ir_mouse_unregister_input(struct input_dev *dev);

extern int mtk_ir_mouse_get_x_smallstep(void);
extern int mtk_ir_mouse_get_y_smallstep(void);
extern int mtk_ir_mouse_get_x_largestep(void);
extern int mtk_ir_mouse_get_y_largestep(void);

extern void mtk_ir_mouse_set_x_smallstep(int xs);
extern void mtk_ir_mouse_set_y_smallstep(int ys);
extern void mtk_ir_mouse_set_x_largestep(int xl);
extern void mtk_ir_mouse_set_y_largestep(int yl);

#endif
