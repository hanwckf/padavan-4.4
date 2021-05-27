/*
 *
 * (C) COPYRIGHT 2015-2016 ARM Limited. All rights reserved.
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




#ifndef _KBASE_CACHE_POLICY_BACKEND_H_
#define _KBASE_CACHE_POLICY_BACKEND_H_

#include "mali_kbase.h"
#include "mali_base_kernel.h"

/**
  * kbase_cache_set_coherency_mode() - Sets the system coherency mode
  *			in the GPU.
  * @kbdev:	Device pointer
  * @mode:	Coherency mode. COHERENCY_ACE/ACE_LITE
  */
void kbase_cache_set_coherency_mode(struct kbase_device *kbdev,
		u32 mode);

#endif				/* _KBASE_CACHE_POLICY_H_ */
