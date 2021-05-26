/*
 * Copyright (c) 2016 MediaTek Inc.
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

#ifndef __MTK_MDP_VPU_H__
#define __MTK_MDP_VPU_H__

#include "mtk_mdp_ipi.h"
#ifdef CONFIG_VIDEO_MEDIATEK_VCU
#include "mtk_vcu.h"
#else
#include "mtk_vpu.h"
#endif

#ifdef CONFIG_VIDEO_MEDIATEK_VCU
#define vpu_get_plat_device		vcu_get_plat_device
#define vpu_load_firmware		vcu_load_firmware
#define vpu_mapping_dm_addr		vcu_mapping_dm_addr
#define vpu_ipi_register		vcu_ipi_register
#define vpu_ipi_send			vcu_ipi_send
#endif

/**
 * struct mtk_mdp_vpu - VPU instance for MDP
 * @pdev	: pointer to the VPU platform device
 * @inst_addr	: VPU MDP instance address
 * @failure	: VPU execution result status
 * @vsi		: VPU shared information
 */
struct mtk_mdp_vpu {
	struct platform_device	*pdev;
	uint64_t		inst_addr;
	int32_t			failure;
	struct mdp_process_vsi	*vsi;
#ifdef CONFIG_VIDEO_MEDIATEK_MDP_FRVC
	struct mdp_process_vsi	vsi_frvc;
	int32_t			initialized;
#endif
};

int mtk_mdp_vpu_register(struct platform_device *pdev);
int mtk_mdp_vpu_init(struct mtk_mdp_vpu *vpu);
int mtk_mdp_vpu_deinit(struct mtk_mdp_vpu *vpu);
int mtk_mdp_vpu_process(struct mtk_mdp_vpu *vpu);

#endif /* __MTK_MDP_VPU_H__ */
