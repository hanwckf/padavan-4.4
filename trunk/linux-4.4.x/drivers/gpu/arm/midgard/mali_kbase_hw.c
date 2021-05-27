/*
 *
 * (C) COPYRIGHT 2012-2016 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */





/*
 * Run-time work-arounds helpers
 */

#include <mali_base_hwconfig_features.h>
#include <mali_base_hwconfig_issues.h>
#include <mali_midg_regmap.h>
#include "mali_kbase.h"
#include "mali_kbase_hw.h"

void kbase_hw_set_features_mask(struct kbase_device *kbdev)
{
	const enum base_hw_feature *features;
	u32 gpu_id;
	u32 product_id;

	gpu_id = kbdev->gpu_props.props.raw_props.gpu_id;
	product_id = gpu_id & GPU_ID_VERSION_PRODUCT_ID;
	product_id >>= GPU_ID_VERSION_PRODUCT_ID_SHIFT;

	if (GPU_ID_IS_NEW_FORMAT(product_id)) {
		switch (gpu_id & GPU_ID2_PRODUCT_MODEL) {
		case GPU_ID2_PRODUCT_TMIX:
			features = base_hw_features_tMIx;
			break;
		default:
			features = base_hw_features_generic;
			break;
		}
	} else {
		switch (product_id) {
		case GPU_ID_PI_TFRX:
			/* FALLTHROUGH */
		case GPU_ID_PI_T86X:
			features = base_hw_features_tFxx;
			break;
		case GPU_ID_PI_T83X:
			features = base_hw_features_t83x;
			break;
		case GPU_ID_PI_T82X:
			features = base_hw_features_t82x;
			break;
		case GPU_ID_PI_T76X:
			features = base_hw_features_t76x;
			break;
		case GPU_ID_PI_T72X:
			features = base_hw_features_t72x;
			break;
		case GPU_ID_PI_T62X:
			features = base_hw_features_t62x;
			break;
		case GPU_ID_PI_T60X:
			features = base_hw_features_t60x;
			break;
		default:
			features = base_hw_features_generic;
			break;
		}
	}

	for (; *features != BASE_HW_FEATURE_END; features++)
		set_bit(*features, &kbdev->hw_features_mask[0]);
}

int kbase_hw_set_issues_mask(struct kbase_device *kbdev)
{
	const enum base_hw_issue *issues;
	u32 gpu_id;
	u32 product_id;
	u32 impl_tech;

	gpu_id = kbdev->gpu_props.props.raw_props.gpu_id;
	product_id = gpu_id & GPU_ID_VERSION_PRODUCT_ID;
	product_id >>= GPU_ID_VERSION_PRODUCT_ID_SHIFT;
	impl_tech = kbdev->gpu_props.props.thread_props.impl_tech;

	if (impl_tech != IMPLEMENTATION_MODEL) {
		if (GPU_ID_IS_NEW_FORMAT(product_id)) {
			switch (gpu_id) {
			case GPU_ID2_MAKE(6, 0, 10, 0, 0, 0, 1):
				issues = base_hw_issues_tMIx_r0p0_05dev0;
				break;
			case GPU_ID2_MAKE(6, 0, 10, 0, 0, 0, 2):
				issues = base_hw_issues_tMIx_r0p0;
				break;
			default:
				if ((gpu_id & GPU_ID2_PRODUCT_MODEL) ==
							GPU_ID2_PRODUCT_TMIX) {
					issues = base_hw_issues_tMIx_r0p0;
				} else {
					dev_err(kbdev->dev,
						"Unknown GPU ID %x", gpu_id);
					return -EINVAL;
				}
			}
		} else {
			switch (gpu_id) {
			case GPU_ID_MAKE(GPU_ID_PI_T60X, 0, 0, GPU_ID_S_15DEV0):
				issues = base_hw_issues_t60x_r0p0_15dev0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T60X, 0, 0, GPU_ID_S_EAC):
				issues = base_hw_issues_t60x_r0p0_eac;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T60X, 0, 1, 0):
				issues = base_hw_issues_t60x_r0p1;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T62X, 0, 1, 0):
				issues = base_hw_issues_t62x_r0p1;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T62X, 1, 0, 0):
			case GPU_ID_MAKE(GPU_ID_PI_T62X, 1, 0, 1):
				issues = base_hw_issues_t62x_r1p0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T62X, 1, 1, 0):
				issues = base_hw_issues_t62x_r1p1;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T76X, 0, 0, 1):
				issues = base_hw_issues_t76x_r0p0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T76X, 0, 1, 1):
				issues = base_hw_issues_t76x_r0p1;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T76X, 0, 1, 9):
				issues = base_hw_issues_t76x_r0p1_50rel0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T76X, 0, 2, 1):
				issues = base_hw_issues_t76x_r0p2;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T76X, 0, 3, 1):
				issues = base_hw_issues_t76x_r0p3;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T76X, 1, 0, 0):
				issues = base_hw_issues_t76x_r1p0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T72X, 0, 0, 0):
			case GPU_ID_MAKE(GPU_ID_PI_T72X, 0, 0, 1):
			case GPU_ID_MAKE(GPU_ID_PI_T72X, 0, 0, 2):
				issues = base_hw_issues_t72x_r0p0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T72X, 1, 0, 0):
				issues = base_hw_issues_t72x_r1p0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T72X, 1, 1, 0):
				issues = base_hw_issues_t72x_r1p1;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_TFRX, 0, 1, 2):
				issues = base_hw_issues_tFRx_r0p1;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_TFRX, 0, 2, 0):
				issues = base_hw_issues_tFRx_r0p2;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_TFRX, 1, 0, 0):
			case GPU_ID_MAKE(GPU_ID_PI_TFRX, 1, 0, 8):
				issues = base_hw_issues_tFRx_r1p0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_TFRX, 2, 0, 0):
				issues = base_hw_issues_tFRx_r2p0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T86X, 0, 2, 0):
				issues = base_hw_issues_t86x_r0p2;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T86X, 1, 0, 0):
			case GPU_ID_MAKE(GPU_ID_PI_T86X, 1, 0, 8):
				issues = base_hw_issues_t86x_r1p0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T86X, 2, 0, 0):
				issues = base_hw_issues_t86x_r2p0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T83X, 0, 1, 0):
				issues = base_hw_issues_t83x_r0p1;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T83X, 1, 0, 0):
			case GPU_ID_MAKE(GPU_ID_PI_T83X, 1, 0, 8):
				issues = base_hw_issues_t83x_r1p0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T82X, 0, 0, 0):
				issues = base_hw_issues_t82x_r0p0;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T82X, 0, 1, 0):
				issues = base_hw_issues_t82x_r0p1;
				break;
			case GPU_ID_MAKE(GPU_ID_PI_T82X, 1, 0, 0):
			case GPU_ID_MAKE(GPU_ID_PI_T82X, 1, 0, 8):
				issues = base_hw_issues_t82x_r1p0;
				break;
			default:
				dev_err(kbdev->dev,
					"Unknown GPU ID %x", gpu_id);
				return -EINVAL;
			}
		}
	} else {
		/* Software model */
		if (GPU_ID_IS_NEW_FORMAT(product_id)) {
			switch (gpu_id & GPU_ID2_PRODUCT_MODEL) {
			case GPU_ID2_PRODUCT_TMIX:
				issues = base_hw_issues_model_tMIx;
				break;
			default:
				dev_err(kbdev->dev,
					"Unknown GPU ID %x", gpu_id);
				return -EINVAL;
			}
		} else {
			switch (product_id) {
			case GPU_ID_PI_T60X:
				issues = base_hw_issues_model_t60x;
				break;
			case GPU_ID_PI_T62X:
				issues = base_hw_issues_model_t62x;
				break;
			case GPU_ID_PI_T72X:
				issues = base_hw_issues_model_t72x;
				break;
			case GPU_ID_PI_T76X:
				issues = base_hw_issues_model_t76x;
				break;
			case GPU_ID_PI_TFRX:
				issues = base_hw_issues_model_tFRx;
				break;
			case GPU_ID_PI_T86X:
				issues = base_hw_issues_model_t86x;
				break;
			case GPU_ID_PI_T83X:
				issues = base_hw_issues_model_t83x;
				break;
			case GPU_ID_PI_T82X:
				issues = base_hw_issues_model_t82x;
				break;
			default:
				dev_err(kbdev->dev, "Unknown GPU ID %x",
					gpu_id);
				return -EINVAL;
			}
		}
	}

	dev_info(kbdev->dev, "GPU identified as 0x%04x r%dp%d status %d", (gpu_id & GPU_ID_VERSION_PRODUCT_ID) >> GPU_ID_VERSION_PRODUCT_ID_SHIFT, (gpu_id & GPU_ID_VERSION_MAJOR) >> GPU_ID_VERSION_MAJOR_SHIFT, (gpu_id & GPU_ID_VERSION_MINOR) >> GPU_ID_VERSION_MINOR_SHIFT, (gpu_id & GPU_ID_VERSION_STATUS) >> GPU_ID_VERSION_STATUS_SHIFT);

	for (; *issues != BASE_HW_ISSUE_END; issues++)
		set_bit(*issues, &kbdev->hw_issues_mask[0]);

	return 0;
}
