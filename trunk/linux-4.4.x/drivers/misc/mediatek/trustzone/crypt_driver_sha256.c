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

#include <crypto/internal/hash.h>
#include <crypto/sha.h>
#include <linux/module.h>
#include "tz_mtk_crypto_api.h"

struct mtk_sha256_state {
	struct mtk_crypto_ctx pctx;
};

static inline int mtk_sha256_init(struct shash_desc *desc)
{
	int err;
	struct mtk_sha256_state *sctx = shash_desc_ctx(desc);

	err = mtk_crypto_ctx_init(&sctx->pctx);

	if (err)
		return -EINVAL;

	err = mtk_crypto_sha256_begin(&sctx->pctx);

	return err;
}

static int mtk_sha256_update(struct shash_desc *desc, const u8 *data,
			  unsigned int len)
{
	struct mtk_sha256_state *sctx = shash_desc_ctx(desc);

	return mtk_crypto_sha256_process(&sctx->pctx, data, len);
}

static int mtk_sha256_final(struct shash_desc *desc, u8 *out)
{
	struct mtk_sha256_state *sctx = shash_desc_ctx(desc);
	int err;

	err = mtk_crypto_sha256_done(&sctx->pctx, out);

	mtk_crypto_ctx_uninit(&sctx->pctx);

	return err;
}

static int mtk_sha256_finup(struct shash_desc *desc, const u8 *data,
			 unsigned int len, u8 *out)
{
	int err = 0;

	err = mtk_sha256_update(desc, data, len);
	if (err)
		return -EINVAL;

	err = mtk_sha256_final(desc, out);

	return err;
}


static struct shash_alg algs[] = { {
	.init			= mtk_sha256_init,
	.update			= mtk_sha256_update,
	.final			= mtk_sha256_final,
	.finup			= mtk_sha256_finup,
	.descsize		= sizeof(struct mtk_sha256_state),
	.digestsize		= SHA256_DIGEST_SIZE,
	.base			= {
		.cra_name		= "sha256",
		.cra_driver_name	= "sha256-mtk",
		.cra_priority		= 500,
		.cra_flags		= CRYPTO_ALG_TYPE_SHASH,
		.cra_blocksize		= SHA256_BLOCK_SIZE,
		.cra_module		= THIS_MODULE,
	}
} };

static int __init mtk_sha256_mod_init(void)
{
	return crypto_register_shashes(algs, ARRAY_SIZE(algs));
}

static void __exit mtk_sha256_mod_fini(void)
{
	crypto_unregister_shashes(algs, ARRAY_SIZE(algs));
}

late_initcall(mtk_sha256_mod_init);
module_exit(mtk_sha256_mod_fini);
MODULE_DESCRIPTION("SHA-256 secure hash using MTK Crypto Hardware");
