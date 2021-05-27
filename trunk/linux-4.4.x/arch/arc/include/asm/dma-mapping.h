/*
 * DMA Mapping glue for ARC
 *
 * Copyright (C) 2004, 2007-2010, 2011-2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ASM_ARC_DMA_MAPPING_H
#define ASM_ARC_DMA_MAPPING_H

#include <asm-generic/dma-coherent.h>
#include <asm/cacheflush.h>

void *dma_alloc_noncoherent(struct device *dev, size_t size,
			    dma_addr_t *dma_handle, gfp_t gfp);

void dma_free_noncoherent(struct device *dev, size_t size, void *vaddr,
			  dma_addr_t dma_handle);

void *dma_alloc_coherent(struct device *dev, size_t size,
			 dma_addr_t *dma_handle, gfp_t gfp);

void dma_free_coherent(struct device *dev, size_t size, void *kvaddr,
		       dma_addr_t dma_handle);

/* drivers/base/dma-mapping.c */
extern int dma_common_mmap(struct device *dev, struct vm_area_struct *vma,
			   void *cpu_addr, dma_addr_t dma_addr, size_t size);
extern int dma_common_get_sgtable(struct device *dev, struct sg_table *sgt,
				  void *cpu_addr, dma_addr_t dma_addr,
				  size_t size);

#define dma_mmap_coherent(d, v, c, h, s) dma_common_mmap(d, v, c, h, s)
#define dma_get_sgtable(d, t, v, h, s) dma_common_get_sgtable(d, t, v, h, s)

/*
 * streaming DMA Mapping API...
 * CPU accesses page via normal paddr, thus needs to explicitly made
 * consistent before each use
 */

static inline void __inline_dma_cache_sync(unsigned long paddr, size_t size,
					   enum dma_data_direction dir)
{
	switch (dir) {
	case DMA_FROM_DEVICE:
		dma_cache_inv(paddr, size);
		break;
	case DMA_TO_DEVICE:
		dma_cache_wback(paddr, size);
		break;
	case DMA_BIDIRECTIONAL:
		dma_cache_wback_inv(paddr, size);
		break;
	default:
		pr_err("Invalid DMA dir [%d] for OP @ %lx\n", dir, paddr);
	}
}

void __arc_dma_cache_sync(unsigned long paddr, size_t size,
			  enum dma_data_direction dir);

#define _dma_cache_sync(addr, sz, dir)			\
do {							\
	if (__builtin_constant_p(dir))			\
		__inline_dma_cache_sync(addr, sz, dir);	\
	else						\
		__arc_dma_cache_sync(addr, sz, dir);	\
}							\
while (0);

static inline dma_addr_t
dma_map_single(struct device *dev, void *cpu_addr, size_t size,
	       enum dma_data_direction dir)
{
	_dma_cache_sync((unsigned long)cpu_addr, size, dir);
	return (dma_addr_t)cpu_addr;
}

static inline void
dma_unmap_single(struct device *dev, dma_addr_t dma_addr,
		 size_t size, enum dma_data_direction dir)
{
}

static inline dma_addr_t
dma_map_page(struct device *dev, struct page *page,
	     unsigned long offset, size_t size,
	     enum dma_data_direction dir)
{
	unsigned long paddr = page_to_phys(page) + offset;
	return dma_map_single(dev, (void *)paddr, size, dir);
}

static inline void
dma_unmap_page(struct device *dev, dma_addr_t dma_handle,
	       size_t size, enum dma_data_direction dir)
{
}

static inline int
dma_map_sg(struct device *dev, struct scatterlist *sg,
	   int nents, enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nents, i)
		s->dma_address = dma_map_page(dev, sg_page(s), s->offset,
					       s->length, dir);

	return nents;
}

static inline void
dma_unmap_sg(struct device *dev, struct scatterlist *sg,
	     int nents, enum dma_data_direction dir)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nents, i)
		dma_unmap_page(dev, sg_dma_address(s), sg_dma_len(s), dir);
}

static inline void
dma_sync_single_for_cpu(struct device *dev, dma_addr_t dma_handle,
			size_t size, enum dma_data_direction dir)
{
	_dma_cache_sync(dma_handle, size, DMA_FROM_DEVICE);
}

static inline void
dma_sync_single_for_device(struct device *dev, dma_addr_t dma_handle,
			   size_t size, enum dma_data_direction dir)
{
	_dma_cache_sync(dma_handle, size, DMA_TO_DEVICE);
}

static inline void
dma_sync_single_range_for_cpu(struct device *dev, dma_addr_t dma_handle,
			      unsigned long offset, size_t size,
			      enum dma_data_direction direction)
{
	_dma_cache_sync(dma_handle + offset, size, DMA_FROM_DEVICE);
}

static inline void
dma_sync_single_range_for_device(struct device *dev, dma_addr_t dma_handle,
				 unsigned long offset, size_t size,
				 enum dma_data_direction direction)
{
	_dma_cache_sync(dma_handle + offset, size, DMA_TO_DEVICE);
}

static inline void
dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sglist, int nelems,
		    enum dma_data_direction dir)
{
	int i;
	struct scatterlist *sg;

	for_each_sg(sglist, sg, nelems, i)
		_dma_cache_sync((unsigned int)sg_virt(sg), sg->length, dir);
}

static inline void
dma_sync_sg_for_device(struct device *dev, struct scatterlist *sglist,
		       int nelems, enum dma_data_direction dir)
{
	int i;
	struct scatterlist *sg;

	for_each_sg(sglist, sg, nelems, i)
		_dma_cache_sync((unsigned int)sg_virt(sg), sg->length, dir);
}

static inline int dma_supported(struct device *dev, u64 dma_mask)
{
	/* Support 32 bit DMA mask exclusively */
	return dma_mask == DMA_BIT_MASK(32);
}

static inline int dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return 0;
}

static inline int dma_set_mask(struct device *dev, u64 dma_mask)
{
	if (!dev->dma_mask || !dma_supported(dev, dma_mask))
		return -EIO;

	*dev->dma_mask = dma_mask;

	return 0;
}

#endif
