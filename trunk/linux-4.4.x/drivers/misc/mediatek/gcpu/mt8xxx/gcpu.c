/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>

#if (defined(CONFIG_MTK_IN_HOUSE_TEE_SUPPORT) || \
	defined(CONFIG_TRUSTY))
#include "trustzone/kree/system.h"
#include "trustzone/tz_cross/ta_gcpu.h"
#define GCPU_TEE_ENABLE 1
#else
#define GCPU_TEE_ENABLE 0
#endif

#define GCPU_DEV_NAME "MTK_GCPU"
#define GCPU_TAG "GCPU Kernel"

#define GCPU_LOG_ERR(log, args...) \
	pr_err("[%s] [%d] *** ERROR: "log, __func__, __LINE__, ##args)
#define GCPU_LOG_INFO(log, args...) \
	pr_debug("[%s] [%d] "log, __func__, __LINE__, ##args)

#if GCPU_TEE_ENABLE
static int gcpu_tee_call(uint32_t cmd)
{
	TZ_RESULT l_ret = TZ_RESULT_SUCCESS;
	int ret = 0;
	KREE_SESSION_HANDLE test_session;
	/* MTEEC_PARAM param[4]; */
	struct timespec start, end;
	long long ns;

	l_ret = KREE_CreateSession(TZ_TA_GCPU_UUID, &test_session);
	if (l_ret != TZ_RESULT_SUCCESS) {
		GCPU_LOG_ERR("KREE_CreateSession error, ret = %x\n", l_ret);
		return 1;
	}

	getnstimeofday(&start);
	l_ret = KREE_TeeServiceCall(test_session, cmd, 0, NULL);
	if (l_ret != TZ_RESULT_SUCCESS) {
		GCPU_LOG_ERR("KREE_TeeServiceCall error, ret = %x\n", l_ret);
		ret = 1;
	}
	getnstimeofday(&end);
	ns = ((long long)end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
	GCPU_LOG_INFO("gcpu_tee_call, cmd: %d, time: %lld ns\n", cmd, ns);

	l_ret = KREE_CloseSession(test_session);
	if (l_ret != TZ_RESULT_SUCCESS) {
		GCPU_LOG_ERR("KREE_CloseSession error, ret = %x\n", l_ret);
		ret = 1;
	}

	return ret;
}

static int gcpu_probe(struct platform_device *pdev)
{
	int ret = 0;
	GCPU_LOG_INFO("gcpu_probe\n");
	/* register for GCPU */

	if (pdev->dev.of_node) {
		ret = of_property_match_string(pdev->dev.of_node, "power-names", "GCPU");
		if (ret == 0) {
			GCPU_LOG_INFO("[GCPU]run time get sync!!\n");
			pm_runtime_enable(&pdev->dev);
			pm_runtime_get_sync(&pdev->dev);
		}
	}
	return 0;
}

static int gcpu_remove(struct platform_device *pdev)
{
	int ret = 0;
	GCPU_LOG_INFO("gcpu_remove\n");
	if (pdev->dev.of_node) {
		ret = of_property_match_string(pdev->dev.of_node, "power-names", "GCPU");
		if (ret == 0) {
			GCPU_LOG_INFO("[GCPU]runtime put sync !!\n");
			pm_runtime_put_sync(&pdev->dev);
		}
	}
	return 0;
}

static int gcpu_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	int ret = 0;

	GCPU_LOG_INFO("gcpu_suspend\n");
	if (gcpu_tee_call(TZCMD_GCPU_SUSPEND)) {
		GCPU_LOG_ERR("Suspend fail\n");
		ret = 1;
	} else {
		GCPU_LOG_INFO("Suspend ok\n");
		ret = 0;
	}
	return ret;
}

static int gcpu_resume(struct platform_device *pdev)
{
	int ret = 0;

	GCPU_LOG_INFO("gcpu_resume\n");
	if (gcpu_tee_call(TZCMD_GCPU_RESUME)) {
		GCPU_LOG_ERR("gcpu_resume fail\n");
		ret = 1;
	} else {
		GCPU_LOG_INFO("gcpu_resume ok\n");
		ret = 0;
	}
	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id mtk_gcpu_ids[] = {
	{ .compatible = "mediatek,mt8127-gcpu" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mtk_gcpu_ids);
#endif

static struct platform_driver gcpu_driver = {
	.probe = gcpu_probe,
	.remove = gcpu_remove,
	.suspend = gcpu_suspend,
	.resume = gcpu_resume,
	.driver = {
		.name = GCPU_DEV_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mtk_gcpu_ids,
#endif
	}
};
#endif

static int __init gcpu_init(void)
{
#if GCPU_TEE_ENABLE
	int ret = 0;

	GCPU_LOG_INFO("module init\n");

	ret = platform_driver_register(&gcpu_driver);
	if (ret) {
		GCPU_LOG_ERR("Unable to register driver, ret = %d\n", ret);
		return ret;
	}
	gcpu_tee_call(TZCMD_GCPU_KERNEL_INIT_DONE);
#endif
	return 0;
}

static void __exit gcpu_exit(void)
{
#if GCPU_TEE_ENABLE
	GCPU_LOG_INFO("module exit\n");
#endif
}
module_init(gcpu_init);
module_exit(gcpu_exit);

MODULE_DESCRIPTION("MTK GCPU driver");
MODULE_AUTHOR("Yi Zheng <yi.zheng@mediatek.com>");
MODULE_LICENSE("GPL");
