/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000  Ani Joshi <ajoshi@unixbox.com>
 * Copyright (C) 2000, 2001, 06	 Ralf Baechle <ralf@linux-mips.org>
 * swiped from i386, and cloned for MIPS by Geert, polished by Ralf.
 */

#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/string.h>
#include <linux/gfp.h>
#include <linux/highmem.h>
#include <linux/dma-contiguous.h>

#include <asm/cache.h>
#include <asm/cpu-type.h>
#include <asm/io.h>

#include <dma-coherence.h>

#if defined(CONFIG_DMA_MAYBE_COHERENT) && !defined(CONFIG_DMA_PERDEV_COHERENT)
/* User defined DMA coherency from command line. */
enum coherent_io_user_state coherentio = IO_COHERENCE_DEFAULT;
EXPORT_SYMBOL_GPL(coherentio);
int hw_coherentio = 0;	/* Actual hardware supported DMA coherency setting. */

static int __init setcoherentio(char *str)
{
	coherentio = IO_COHERENCE_ENABLED;
	pr_info("Hardware DMA cache coherency (command line)\n");
	return 0;
}
early_param("coherentio", setcoherentio);

static int __init setnocoherentio(char *str)
{
	coherentio = IO_COHERENCE_DISABLED;
	pr_info("Software DMA cache coherency (command line)\n");
	return 0;
}
early_param("nocoherentio", setnocoherentio);
#endif

static gfp_t massage_gfp_flags(const struct device *dev, gfp_t gfp)
{
	gfp_t dma_flag;

	/* ignore region specifiers */
	gfp &= ~(__GFP_DMA | __GFP_DMA32 | __GFP_HIGHMEM);

#ifdef CONFIG_ISA
	if (dev == NULL)
		dma_flag = __GFP_DMA;
	else
#endif
#if defined(CONFIG_ZONE_DMA32) && defined(CONFIG_ZONE_DMA)
	     if (dev->coherent_dma_mask < DMA_BIT_MASK(32))
			dma_flag = __GFP_DMA;
	else if (dev->coherent_dma_mask < DMA_BIT_MASK(64))
			dma_flag = __GFP_DMA32;
	else
#endif
#if defined(CONFIG_ZONE_DMA32) && !defined(CONFIG_ZONE_DMA)
	     if (dev->coherent_dma_mask < DMA_BIT_MASK(64))
		dma_flag = __GFP_DMA32;
	else
#endif
#if defined(CONFIG_ZONE_DMA) && !defined(CONFIG_ZONE_DMA32)
#if defined(CONFIG_MIPS_L2_CACHE_ER35)
	     if (!dev || dev->coherent_dma_mask < DMA_BIT_MASK(sizeof(phys_addr_t) * 8))
#else
	     if (dev->coherent_dma_mask < DMA_BIT_MASK(sizeof(phys_addr_t) * 8))
#endif
		dma_flag = __GFP_DMA;
	else
#endif
#if defined(CONFIG_MIPS_L2_CACHE_ER35) && defined(CONFIG_ZONE_DMA)
		dma_flag = __GFP_DMA;
#else
		dma_flag = 0;
#endif

	/* Don't invoke OOM killer */
	gfp |= __GFP_NORETRY;

	return gfp | dma_flag;
}

static void *mips_dma_alloc_noncoherent(struct device *dev, size_t size,
	dma_addr_t * dma_handle, gfp_t gfp)
{
	void *ret;

	gfp = massage_gfp_flags(dev, gfp);

	ret = (void *) __get_free_pages(gfp, get_order(size));

	if (ret != NULL) {
		memset(ret, 0, size);
		*dma_handle = plat_map_dma_mem(dev, ret, size);
	}

	return ret;
}

void *mips_dma_alloc_coherent(struct device *dev, size_t size,
	dma_addr_t * dma_handle, gfp_t gfp, struct dma_attrs *attrs)
{
	void *ret;
	struct page *page = NULL;
	unsigned int count = PAGE_ALIGN(size) >> PAGE_SHIFT;

	/*
	 * XXX: seems like the coherent and non-coherent implementations could
	 * be consolidated.
	 */
	if (dma_get_attr(DMA_ATTR_NON_CONSISTENT, attrs))
		return mips_dma_alloc_noncoherent(dev, size, dma_handle, gfp);

	gfp = massage_gfp_flags(dev, gfp);

	if (IS_ENABLED(CONFIG_DMA_CMA) && gfpflags_allow_blocking(gfp))
		page = dma_alloc_from_contiguous(dev,
					count, get_order(size));
	if (!page)
		page = alloc_pages(gfp, get_order(size));

	if (!page)
		return NULL;

	ret = page_address(page);
	memset(ret, 0, size);
	*dma_handle = plat_map_dma_mem(dev, ret, size);
	if (!plat_device_is_coherent(dev)) {
		dma_cache_wback_inv((unsigned long) ret, size);
		ret = UNCAC_ADDR(ret);
	}

	return ret;
}
EXPORT_SYMBOL(mips_dma_alloc_coherent);


static void mips_dma_free_noncoherent(struct device *dev, size_t size,
		void *vaddr, dma_addr_t dma_handle)
{
	plat_unmap_dma_mem(dev, dma_handle, size, DMA_BIDIRECTIONAL);
	free_pages((unsigned long) vaddr, get_order(size));
}

void mips_dma_free_coherent(struct device *dev, size_t size, void *vaddr,
	dma_addr_t dma_handle, struct dma_attrs *attrs)
{
	unsigned long addr = (unsigned long) vaddr;
	unsigned int count = PAGE_ALIGN(size) >> PAGE_SHIFT;
	struct page *page = NULL;

	if (dma_get_attr(DMA_ATTR_NON_CONSISTENT, attrs)) {
		mips_dma_free_noncoherent(dev, size, vaddr, dma_handle);
		return;
	}

	plat_unmap_dma_mem(dev, dma_handle, size, DMA_BIDIRECTIONAL);

	if (!plat_device_is_coherent(dev))
		addr = CAC_ADDR(addr);

	page = virt_to_page((void *) addr);

	if (!dma_release_from_contiguous(dev, page, count))
		__free_pages(page, get_order(size));
}
EXPORT_SYMBOL(mips_dma_free_coherent);

static inline void __dma_sync_virtual(void *addr, size_t size,
	enum dma_data_direction direction)
{
	switch (direction) {
	case DMA_TO_DEVICE:
		dma_cache_wback((unsigned long)addr, size);
		break;

	case DMA_FROM_DEVICE:
		dma_cache_inv((unsigned long)addr, size);
		break;

	case DMA_BIDIRECTIONAL:
		dma_cache_wback_inv((unsigned long)addr, size);
		break;

	default:
		BUG();
	}
}

/*
 * A single sg entry may refer to multiple physically contiguous
 * pages. But we still need to process highmem pages individually.
 * If highmem is not configured then the bulk of this loop gets
 * optimized out.
 */
void __dma_sync(struct page *page,
	unsigned long offset, size_t size, enum dma_data_direction direction)
{
	size_t left = size;

	do {
		size_t len = left;

		if (PageHighMem(page)) {
			void *addr;

			if (offset + len > PAGE_SIZE) {
				if (offset >= PAGE_SIZE) {
					page += offset >> PAGE_SHIFT;
					offset &= ~PAGE_MASK;
				}
				len = PAGE_SIZE - offset;
			}

			addr = kmap_atomic(page);
			__dma_sync_virtual(addr + offset, len, direction);
			kunmap_atomic(addr);
		} else
			__dma_sync_virtual(page_address(page) + offset,
					   size, direction);
		offset = 0;
		page++;
		left -= len;
	} while (left);
}
EXPORT_SYMBOL(__dma_sync);

void dma_cache_sync(struct device *dev, void *vaddr, size_t size,
			 enum dma_data_direction direction)
{
	BUG_ON(direction == DMA_NONE);

	if (!plat_device_is_coherent(dev))
		__dma_sync_virtual(vaddr, size, direction);
}

EXPORT_SYMBOL(dma_cache_sync);

#ifdef CONFIG_SYS_HAS_DMA_OPS
struct dma_map_ops *mips_dma_map_ops = NULL;
EXPORT_SYMBOL(mips_dma_map_ops);
#endif

#define PREALLOC_DMA_DEBUG_ENTRIES (1 << 16)

static int __init mips_dma_init(void)
{
	dma_debug_init(PREALLOC_DMA_DEBUG_ENTRIES);

	return 0;
}
fs_initcall(mips_dma_init);
