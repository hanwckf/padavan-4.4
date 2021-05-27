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





/*
 * Base kernel property query APIs
 */

#include <mali_kbase.h>
#include <mali_midg_regmap.h>
#include <mali_kbase_gpuprops.h>
#include <mali_kbase_config_defaults.h>
#include <mali_kbase_hwaccess_gpuprops.h>
#include <linux/clk.h>

/**
 * KBASE_UBFX32 - Extracts bits from a 32-bit bitfield.
 * @value:  The value from which to extract bits.
 * @offset: The first bit to extract (0 being the LSB).
 * @size:   The number of bits to extract.
 *
 * Context: @offset + @size <= 32.
 *
 * Return: Bits [@offset, @offset + @size) from @value.
 */
/* from mali_cdsb.h */
#define KBASE_UBFX32(value, offset, size) \
	(((u32)(value) >> (u32)(offset)) & (u32)((1ULL << (u32)(size)) - 1))

int kbase_gpuprops_uk_get_props(struct kbase_context *kctx, struct kbase_uk_gpuprops * const kbase_props)
{
	kbase_gpu_clk_speed_func get_gpu_speed_mhz;
	u32 gpu_speed_mhz;
	int rc = 1;

	KBASE_DEBUG_ASSERT(NULL != kctx);
	KBASE_DEBUG_ASSERT(NULL != kbase_props);

	/* Current GPU speed is requested from the system integrator via the GPU_SPEED_FUNC function.
	 * If that function fails, or the function is not provided by the system integrator, we report the maximum
	 * GPU speed as specified by GPU_FREQ_KHZ_MAX.
	 */
	get_gpu_speed_mhz = (kbase_gpu_clk_speed_func) GPU_SPEED_FUNC;
	if (get_gpu_speed_mhz != NULL) {
		rc = get_gpu_speed_mhz(&gpu_speed_mhz);
#ifdef CONFIG_MALI_DEBUG
		/* Issue a warning message when the reported GPU speed falls outside the min/max range */
		if (rc == 0) {
			u32 gpu_speed_khz = gpu_speed_mhz * 1000;

			if (gpu_speed_khz < kctx->kbdev->gpu_props.props.core_props.gpu_freq_khz_min ||
					gpu_speed_khz > kctx->kbdev->gpu_props.props.core_props.gpu_freq_khz_max)
				dev_warn(kctx->kbdev->dev, "GPU Speed is outside of min/max range (got %lu Khz, min %lu Khz, max %lu Khz)\n",
						(unsigned long)gpu_speed_khz,
						(unsigned long)kctx->kbdev->gpu_props.props.core_props.gpu_freq_khz_min,
						(unsigned long)kctx->kbdev->gpu_props.props.core_props.gpu_freq_khz_max);
		}
#endif				/* CONFIG_MALI_DEBUG */
	}
	if (kctx->kbdev->clock) {
		gpu_speed_mhz = clk_get_rate(kctx->kbdev->clock) / 1000000;
		rc = 0;
	}
	if (rc != 0)
		gpu_speed_mhz = kctx->kbdev->gpu_props.props.core_props.gpu_freq_khz_max / 1000;

	kctx->kbdev->gpu_props.props.core_props.gpu_speed_mhz = gpu_speed_mhz;

	memcpy(&kbase_props->props, &kctx->kbdev->gpu_props.props, sizeof(kbase_props->props));

	/* Before API 8.2 they expect L3 cache info here, which was always 0 */
	if (kctx->api_version < KBASE_API_VERSION(8, 2))
		kbase_props->props.raw_props.suspend_size = 0;

	return 0;
}

static void kbase_gpuprops_construct_coherent_groups(base_gpu_props * const props)
{
	struct mali_base_gpu_coherent_group *current_group;
	u64 group_present;
	u64 group_mask;
	u64 first_set, first_set_prev;
	u32 num_groups = 0;

	KBASE_DEBUG_ASSERT(NULL != props);

	props->coherency_info.coherency = props->raw_props.mem_features;
	props->coherency_info.num_core_groups = hweight64(props->raw_props.l2_present);

	if (props->coherency_info.coherency & GROUPS_L2_COHERENT) {
		/* Group is l2 coherent */
		group_present = props->raw_props.l2_present;
	} else {
		/* Group is l1 coherent */
		group_present = props->raw_props.shader_present;
	}

	/*
	 * The coherent group mask can be computed from the l2 present
	 * register.
	 *
	 * For the coherent group n:
	 * group_mask[n] = (first_set[n] - 1) & ~(first_set[n-1] - 1)
	 * where first_set is group_present with only its nth set-bit kept
	 * (i.e. the position from where a new group starts).
	 *
	 * For instance if the groups are l2 coherent and l2_present=0x0..01111:
	 * The first mask is:
	 * group_mask[1] = (first_set[1] - 1) & ~(first_set[0] - 1)
	 *               = (0x0..010     - 1) & ~(0x0..01      - 1)
	 *               =  0x0..00f
	 * The second mask is:
	 * group_mask[2] = (first_set[2] - 1) & ~(first_set[1] - 1)
	 *               = (0x0..100     - 1) & ~(0x0..010     - 1)
	 *               =  0x0..0f0
	 * And so on until all the bits from group_present have been cleared
	 * (i.e. there is no group left).
	 */

	current_group = props->coherency_info.group;
	first_set = group_present & ~(group_present - 1);

	while (group_present != 0 && num_groups < BASE_MAX_COHERENT_GROUPS) {
		group_present -= first_set;	/* Clear the current group bit */
		first_set_prev = first_set;

		first_set = group_present & ~(group_present - 1);
		group_mask = (first_set - 1) & ~(first_set_prev - 1);

		/* Populate the coherent_group structure for each group */
		current_group->core_mask = group_mask & props->raw_props.shader_present;
		current_group->num_cores = hweight64(current_group->core_mask);

		num_groups++;
		current_group++;
	}

	if (group_present != 0)
		pr_warn("Too many coherent groups (keeping only %d groups).\n", BASE_MAX_COHERENT_GROUPS);

	props->coherency_info.num_groups = num_groups;
}

/**
 * kbase_gpuprops_get_props - Get the GPU configuration
 * @gpu_props: The &base_gpu_props structure
 * @kbdev: The &struct kbase_device structure for the device
 *
 * Fill the &base_gpu_props structure with values from the GPU configuration
 * registers. Only the raw properties are filled in this function
 */
static void kbase_gpuprops_get_props(base_gpu_props * const gpu_props, struct kbase_device *kbdev)
{
	struct kbase_gpuprops_regdump regdump;
	int i;

	KBASE_DEBUG_ASSERT(NULL != kbdev);
	KBASE_DEBUG_ASSERT(NULL != gpu_props);

	/* Dump relevant registers */
	kbase_backend_gpuprops_get(kbdev, &regdump);

	gpu_props->raw_props.gpu_id = regdump.gpu_id;
	gpu_props->raw_props.tiler_features = regdump.tiler_features;
	gpu_props->raw_props.mem_features = regdump.mem_features;
	gpu_props->raw_props.mmu_features = regdump.mmu_features;
	gpu_props->raw_props.l2_features = regdump.l2_features;
	gpu_props->raw_props.suspend_size = regdump.suspend_size;

	gpu_props->raw_props.as_present = regdump.as_present;
	gpu_props->raw_props.js_present = regdump.js_present;
	gpu_props->raw_props.shader_present = ((u64) regdump.shader_present_hi << 32) + regdump.shader_present_lo;
	gpu_props->raw_props.tiler_present = ((u64) regdump.tiler_present_hi << 32) + regdump.tiler_present_lo;
	gpu_props->raw_props.l2_present = ((u64) regdump.l2_present_hi << 32) + regdump.l2_present_lo;

	for (i = 0; i < GPU_MAX_JOB_SLOTS; i++)
		gpu_props->raw_props.js_features[i] = regdump.js_features[i];

	for (i = 0; i < BASE_GPU_NUM_TEXTURE_FEATURES_REGISTERS; i++)
		gpu_props->raw_props.texture_features[i] = regdump.texture_features[i];

	gpu_props->raw_props.thread_max_barrier_size = regdump.thread_max_barrier_size;
	gpu_props->raw_props.thread_max_threads = regdump.thread_max_threads;
	gpu_props->raw_props.thread_max_workgroup_size = regdump.thread_max_workgroup_size;
	gpu_props->raw_props.thread_features = regdump.thread_features;
}

/**
 * kbase_gpuprops_calculate_props - Calculate the derived properties
 * @gpu_props: The &base_gpu_props structure
 * @kbdev:     The &struct kbase_device structure for the device
 *
 * Fill the &base_gpu_props structure with values derived from the GPU
 * configuration registers
 */
static void kbase_gpuprops_calculate_props(base_gpu_props * const gpu_props, struct kbase_device *kbdev)
{
	int i;

	/* Populate the base_gpu_props structure */
	gpu_props->core_props.version_status = KBASE_UBFX32(gpu_props->raw_props.gpu_id, 0U, 4);
	gpu_props->core_props.minor_revision = KBASE_UBFX32(gpu_props->raw_props.gpu_id, 4U, 8);
	gpu_props->core_props.major_revision = KBASE_UBFX32(gpu_props->raw_props.gpu_id, 12U, 4);
	gpu_props->core_props.product_id = KBASE_UBFX32(gpu_props->raw_props.gpu_id, 16U, 16);
	gpu_props->core_props.log2_program_counter_size = KBASE_GPU_PC_SIZE_LOG2;
	gpu_props->core_props.gpu_available_memory_size = totalram_pages << PAGE_SHIFT;

	for (i = 0; i < BASE_GPU_NUM_TEXTURE_FEATURES_REGISTERS; i++)
		gpu_props->core_props.texture_features[i] = gpu_props->raw_props.texture_features[i];

	gpu_props->l2_props.log2_line_size = KBASE_UBFX32(gpu_props->raw_props.l2_features, 0U, 8);
	gpu_props->l2_props.log2_cache_size = KBASE_UBFX32(gpu_props->raw_props.l2_features, 16U, 8);

	/* Field with number of l2 slices is added to MEM_FEATURES register
	 * since t76x. Below code assumes that for older GPU reserved bits will
	 * be read as zero. */
	gpu_props->l2_props.num_l2_slices =
		KBASE_UBFX32(gpu_props->raw_props.mem_features, 8U, 4) + 1;

	gpu_props->tiler_props.bin_size_bytes = 1 << KBASE_UBFX32(gpu_props->raw_props.tiler_features, 0U, 6);
	gpu_props->tiler_props.max_active_levels = KBASE_UBFX32(gpu_props->raw_props.tiler_features, 8U, 4);

	if (gpu_props->raw_props.thread_max_threads == 0)
		gpu_props->thread_props.max_threads = THREAD_MT_DEFAULT;
	else
		gpu_props->thread_props.max_threads = gpu_props->raw_props.thread_max_threads;

	if (gpu_props->raw_props.thread_max_workgroup_size == 0)
		gpu_props->thread_props.max_workgroup_size = THREAD_MWS_DEFAULT;
	else
		gpu_props->thread_props.max_workgroup_size = gpu_props->raw_props.thread_max_workgroup_size;

	if (gpu_props->raw_props.thread_max_barrier_size == 0)
		gpu_props->thread_props.max_barrier_size = THREAD_MBS_DEFAULT;
	else
		gpu_props->thread_props.max_barrier_size = gpu_props->raw_props.thread_max_barrier_size;

	gpu_props->thread_props.max_registers = KBASE_UBFX32(gpu_props->raw_props.thread_features, 0U, 16);
	gpu_props->thread_props.max_task_queue = KBASE_UBFX32(gpu_props->raw_props.thread_features, 16U, 8);
	gpu_props->thread_props.max_thread_group_split = KBASE_UBFX32(gpu_props->raw_props.thread_features, 24U, 6);
	gpu_props->thread_props.impl_tech = KBASE_UBFX32(gpu_props->raw_props.thread_features, 30U, 2);

	/* If values are not specified, then use defaults */
	if (gpu_props->thread_props.max_registers == 0) {
		gpu_props->thread_props.max_registers = THREAD_MR_DEFAULT;
		gpu_props->thread_props.max_task_queue = THREAD_MTQ_DEFAULT;
		gpu_props->thread_props.max_thread_group_split = THREAD_MTGS_DEFAULT;
	}
	/* Initialize the coherent_group structure for each group */
	kbase_gpuprops_construct_coherent_groups(gpu_props);
}

void kbase_gpuprops_set(struct kbase_device *kbdev)
{
	struct kbase_gpu_props *gpu_props;
	struct gpu_raw_gpu_props *raw;

	KBASE_DEBUG_ASSERT(NULL != kbdev);
	gpu_props = &kbdev->gpu_props;
	raw = &gpu_props->props.raw_props;

	/* Initialize the base_gpu_props structure from the hardware */
	kbase_gpuprops_get_props(&gpu_props->props, kbdev);

	/* Populate the derived properties */
	kbase_gpuprops_calculate_props(&gpu_props->props, kbdev);

	/* Populate kbase-only fields */
	gpu_props->l2_props.associativity = KBASE_UBFX32(raw->l2_features, 8U, 8);
	gpu_props->l2_props.external_bus_width = KBASE_UBFX32(raw->l2_features, 24U, 8);

	gpu_props->mem.core_group = KBASE_UBFX32(raw->mem_features, 0U, 1);

	gpu_props->mmu.va_bits = KBASE_UBFX32(raw->mmu_features, 0U, 8);
	gpu_props->mmu.pa_bits = KBASE_UBFX32(raw->mmu_features, 8U, 8);

	gpu_props->num_cores = hweight64(raw->shader_present);
	gpu_props->num_core_groups = hweight64(raw->l2_present);
	gpu_props->num_address_spaces = hweight32(raw->as_present);
	gpu_props->num_job_slots = hweight32(raw->js_present);
}

void kbase_gpuprops_set_features(struct kbase_device *kbdev)
{
	base_gpu_props *gpu_props;
	struct kbase_gpuprops_regdump regdump;

	gpu_props = &kbdev->gpu_props.props;

	/* Dump relevant registers */
	kbase_backend_gpuprops_get_features(kbdev, &regdump);

	/*
	 * Copy the raw value from the register, later this will get turned
	 * into the selected coherency mode.
	 */
	gpu_props->raw_props.coherency_mode = regdump.coherency_features;
}
