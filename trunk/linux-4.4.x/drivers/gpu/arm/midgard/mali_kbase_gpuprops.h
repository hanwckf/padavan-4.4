/*
 *
 * (C) COPYRIGHT 2011-2015 ARM Limited. All rights reserved.
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





/**
 * @file mali_kbase_gpuprops.h
 * Base kernel property query APIs
 */

#ifndef _KBASE_GPUPROPS_H_
#define _KBASE_GPUPROPS_H_

#include "mali_kbase_gpuprops_types.h"

/* Forward definition - see mali_kbase.h */
struct kbase_device;

/**
 * @brief Set up Kbase GPU properties.
 *
 * Set up Kbase GPU properties with information from the GPU registers
 *
 * @param kbdev		The struct kbase_device structure for the device
 */
void kbase_gpuprops_set(struct kbase_device *kbdev);

/**
 * kbase_gpuprops_set_features - Set up Kbase GPU properties
 * @kbdev:   Device pointer
 *
 * This function sets up GPU properties that are dependent on the hardware
 * features bitmask. This function must be preceeded by a call to
 * kbase_hw_set_features_mask().
 */
void kbase_gpuprops_set_features(struct kbase_device *kbdev);

/**
 * @brief Provide GPU properties to userside through UKU call.
 *
 * Fill the struct kbase_uk_gpuprops with values from GPU configuration registers.
 *
 * @param kctx		The struct kbase_context structure
 * @param kbase_props	A copy of the struct kbase_uk_gpuprops structure from userspace
 *
 * @return 0 on success. Any other value indicates failure.
 */
int kbase_gpuprops_uk_get_props(struct kbase_context *kctx, struct kbase_uk_gpuprops * const kbase_props);

#endif				/* _KBASE_GPUPROPS_H_ */
