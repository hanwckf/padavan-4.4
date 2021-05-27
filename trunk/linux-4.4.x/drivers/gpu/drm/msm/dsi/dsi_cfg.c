/*
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "dsi_cfg.h"

/* DSI v2 has not been supported by now */
static const struct msm_dsi_config dsi_v2_cfg = {
	.io_offset = 0,
};

static const struct msm_dsi_config msm8974_apq8084_dsi_cfg = {
	.io_offset = DSI_6G_REG_SHIFT,
	.reg_cfg = {
		.num = 4,
		.regs = {
			{"gdsc", -1, -1, -1, -1},
			{"vdd", 3000000, 3000000, 150000, 100},
			{"vdda", 1200000, 1200000, 100000, 100},
			{"vddio", 1800000, 1800000, 100000, 100},
		},
	},
};

static const struct msm_dsi_config msm8916_dsi_cfg = {
	.io_offset = DSI_6G_REG_SHIFT,
	.reg_cfg = {
		.num = 4,
		.regs = {
			{"gdsc", -1, -1, -1, -1},
			{"vdd", 2850000, 2850000, 100000, 100},
			{"vdda", 1200000, 1200000, 100000, 100},
			{"vddio", 1800000, 1800000, 100000, 100},
		},
	},
};

static const struct msm_dsi_config msm8994_dsi_cfg = {
	.io_offset = DSI_6G_REG_SHIFT,
	.reg_cfg = {
		.num = 7,
		.regs = {
			{"gdsc", -1, -1, -1, -1},
			{"vdda", 1250000, 1250000, 100000, 100},
			{"vddio", 1800000, 1800000, 100000, 100},
			{"vcca", 1000000, 1000000, 10000, 100},
			{"vdd", 1800000, 1800000, 100000, 100},
			{"lab_reg", -1, -1, -1, -1},
			{"ibb_reg", -1, -1, -1, -1},
		},
	}
};

static const struct msm_dsi_cfg_handler dsi_cfg_handlers[] = {
	{MSM_DSI_VER_MAJOR_V2, U32_MAX, &dsi_v2_cfg},
	{MSM_DSI_VER_MAJOR_6G, MSM_DSI_6G_VER_MINOR_V1_0,
						&msm8974_apq8084_dsi_cfg},
	{MSM_DSI_VER_MAJOR_6G, MSM_DSI_6G_VER_MINOR_V1_1,
						&msm8974_apq8084_dsi_cfg},
	{MSM_DSI_VER_MAJOR_6G, MSM_DSI_6G_VER_MINOR_V1_1_1,
						&msm8974_apq8084_dsi_cfg},
	{MSM_DSI_VER_MAJOR_6G, MSM_DSI_6G_VER_MINOR_V1_2,
						&msm8974_apq8084_dsi_cfg},
	{MSM_DSI_VER_MAJOR_6G, MSM_DSI_6G_VER_MINOR_V1_3, &msm8994_dsi_cfg},
	{MSM_DSI_VER_MAJOR_6G, MSM_DSI_6G_VER_MINOR_V1_3_1, &msm8916_dsi_cfg},
};

const struct msm_dsi_cfg_handler *msm_dsi_cfg_get(u32 major, u32 minor)
{
	const struct msm_dsi_cfg_handler *cfg_hnd = NULL;
	int i;

	for (i = ARRAY_SIZE(dsi_cfg_handlers) - 1; i >= 0; i--) {
		if ((dsi_cfg_handlers[i].major == major) &&
			(dsi_cfg_handlers[i].minor == minor)) {
			cfg_hnd = &dsi_cfg_handlers[i];
			break;
		}
	}

	return cfg_hnd;
}

