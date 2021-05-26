/*
 * Copyright (C) 2015 MediaTek Inc.
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

#include <linux/hw_random.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "tz_mtk_crypto_api.h"
#include <linux/err.h>

static struct mtk_crypto_ctx unused;
static int mediatek_rng_read(struct hwrng *rng, void *buf,
					size_t max, bool wait)
{
	if (mtk_crypto_rng_gen(&unused, buf, max) != 0)
		return 0;
	return max;
}

static struct hwrng mediatek_rng = {
	.name = "mediatek",
	.read = mediatek_rng_read,
};

static int __init rng_init(void)
{
	return hwrng_register(&mediatek_rng);
}

static void __exit rng_exit(void)
{
	hwrng_unregister(&mediatek_rng);
}

module_init(rng_init);
module_exit(rng_exit);
