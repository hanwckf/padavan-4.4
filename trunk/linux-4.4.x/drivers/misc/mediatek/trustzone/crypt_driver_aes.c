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

#include <asm/hwcap.h>
#include <asm/neon.h>
#include <asm/hwcap.h>
#include <crypto/aes.h>
#include <crypto/algapi.h>
#include <linux/module.h>
#include <crypto/scatterwalk.h>
#include "tz_mtk_crypto_api.h"

#define MAX_IVLEN 32

struct mtk_aes_ctx {
	u8 key[AES_MAX_KEY_SIZE];
	u32 key_len;
	struct mtk_crypto_ctx pctx;
};

static int mtk_aes_setkey(struct crypto_tfm *tfm, const u8 *in_key,
			 unsigned int key_len)
{
	struct mtk_aes_ctx *ctx = crypto_tfm_ctx(tfm);

	if (key_len != AES_KEYSIZE_128 && key_len != AES_KEYSIZE_192 && key_len != AES_KEYSIZE_256) {
		tfm->crt_flags |= CRYPTO_TFM_RES_BAD_KEY_LEN;
		return -EINVAL;
	}
	memcpy(ctx->key, in_key, key_len);
	ctx->key_len = key_len;

	return 0;
}

static int mtk_ctr_encrypt(struct blkcipher_desc *desc,
			      struct scatterlist *dst, struct scatterlist *src,
			      unsigned int nbytes)
{
	struct scatter_walk walk_in, walk_out;
	struct crypto_blkcipher *tfm = desc->tfm;
	struct mtk_aes_ctx *ctx = crypto_blkcipher_ctx(tfm);
	unsigned int nblock_bytes = nbytes/AES_BLOCK_SIZE*AES_BLOCK_SIZE;
	unsigned int left_bytes = nbytes - nblock_bytes;
	unsigned char iv[AES_IV_SIZE];

	u8 t_src_buf[AES_BLOCK_SIZE] = {0};
	u8 t_dst_buf[AES_BLOCK_SIZE] = {0};
	int use_src_buf = 0;
	int use_dst_buf = 0;

	memcpy(iv, desc->info, AES_IV_SIZE);
	scatterwalk_start(&walk_in, src);
	scatterwalk_start(&walk_out, dst);

	while (nblock_bytes) {
		int src_n, dst_n, cal_n;
		u8 *data_src, *data_dst;

		src_n = scatterwalk_clamp(&walk_in, nblock_bytes);
		if (!src_n) {
			scatterwalk_start(&walk_in, sg_next(walk_in.sg));
			src_n = scatterwalk_clamp(&walk_in, nblock_bytes);
		}

		dst_n = scatterwalk_clamp(&walk_out, nblock_bytes);
		if (!dst_n) {
			scatterwalk_start(&walk_out, sg_next(walk_out.sg));
			dst_n = scatterwalk_clamp(&walk_out, nblock_bytes);
		}

		if (src_n >= AES_BLOCK_SIZE) {
			src_n = src_n/AES_BLOCK_SIZE*AES_BLOCK_SIZE;
			data_src = scatterwalk_map(&walk_in);
			use_src_buf = 0;
		} else {
			int t_src_buf_idx = 0;
			u8 *t_src_data;

			while (t_src_buf_idx != AES_BLOCK_SIZE) {
				int pagedone = 0;

				src_n = scatterwalk_clamp(&walk_in, AES_BLOCK_SIZE-t_src_buf_idx);
				if (!src_n) {
					scatterwalk_start(&walk_in, sg_next(walk_in.sg));
					src_n = scatterwalk_clamp(&walk_in, AES_BLOCK_SIZE-t_src_buf_idx);
				}
				if (src_n == scatterwalk_pagelen(&walk_in))
					pagedone = 1;
				t_src_data = scatterwalk_map(&walk_in);
				memcpy(t_src_buf+t_src_buf_idx, t_src_data, src_n);
				scatterwalk_advance(&walk_in, src_n);
				if (pagedone)
					scatterwalk_done(&walk_in, 0, 1);
				scatterwalk_unmap(t_src_data);
				t_src_buf_idx += src_n;
			}
			src_n = t_src_buf_idx;
			data_src = t_src_buf;
			use_src_buf = 1;
		}

		if (dst_n >= AES_BLOCK_SIZE) {
			dst_n = dst_n/AES_BLOCK_SIZE*AES_BLOCK_SIZE;
			data_dst = scatterwalk_map(&walk_out);
			use_dst_buf = 0;
		} else {
			dst_n = AES_BLOCK_SIZE;
			data_dst = t_dst_buf;
			use_dst_buf = 1;
		}

		cal_n = dst_n > src_n?src_n:dst_n;

		mtk_crypto_aes(&ctx->pctx, 2, 0, iv, ctx->key, ctx->key_len, data_src, data_dst, cal_n);
		memcpy(iv, ctx->pctx.tag, AES_IV_SIZE);
		nblock_bytes -= cal_n;

		if (!use_src_buf) {
			scatterwalk_unmap(data_src);
			scatterwalk_advance(&walk_in, cal_n);
			scatterwalk_done(&walk_in, 0, nblock_bytes);
		}

		if (!use_dst_buf) {
			scatterwalk_unmap(data_dst);
			scatterwalk_advance(&walk_out, cal_n);
			scatterwalk_done(&walk_out, 1, nblock_bytes);
		} else {
			int t_dst_buf_idx = 0;
			u8 *t_dst_data;

			while (t_dst_buf_idx != AES_BLOCK_SIZE) {
				int pagedone = 0;

				dst_n = scatterwalk_clamp(&walk_out, AES_BLOCK_SIZE-t_dst_buf_idx);
				if (!dst_n) {
					scatterwalk_start(&walk_out, sg_next(walk_out.sg));
					dst_n = scatterwalk_clamp(&walk_out, AES_BLOCK_SIZE-t_dst_buf_idx);
				}
				if (dst_n == scatterwalk_pagelen(&walk_out))
					pagedone = 1;
				t_dst_data = scatterwalk_map(&walk_out);
				memcpy(t_dst_data, t_dst_buf+t_dst_buf_idx, dst_n);
				scatterwalk_advance(&walk_out, dst_n);
				if (pagedone)
					scatterwalk_done(&walk_out, 1, 1);
				scatterwalk_unmap(t_dst_data);
				t_dst_buf_idx += dst_n;
			}
		}
	}

	if (left_bytes) {
		int t_src_buf_idx = 0;
		u8 *t_src_data;
		int t_dst_buf_idx = 0;
		u8 *t_dst_data;
		int src_n, dst_n;

		while (t_src_buf_idx != left_bytes) {
			src_n = scatterwalk_clamp(&walk_in, left_bytes - t_src_buf_idx);
			if (!src_n) {
				scatterwalk_start(&walk_in, sg_next(walk_in.sg));
				src_n = scatterwalk_clamp(&walk_in, left_bytes - t_src_buf_idx);
			}
			t_src_data = scatterwalk_map(&walk_in);
			memcpy(t_src_buf+t_src_buf_idx, t_src_data, src_n);
			t_src_buf_idx += src_n;
			scatterwalk_advance(&walk_in, src_n);
			scatterwalk_done(&walk_in, 0, left_bytes - t_src_buf_idx);
			scatterwalk_unmap(t_src_data);
		}

		mtk_crypto_aes(&ctx->pctx, 2, 0, iv, ctx->key, ctx->key_len, t_src_buf, t_dst_buf, AES_BLOCK_SIZE);

		while (t_dst_buf_idx != left_bytes) {
			dst_n = scatterwalk_clamp(&walk_out, left_bytes-t_dst_buf_idx);
			if (!dst_n) {
				scatterwalk_start(&walk_out, sg_next(walk_out.sg));
				dst_n = scatterwalk_clamp(&walk_out, left_bytes-t_dst_buf_idx);
			}
			t_dst_data = scatterwalk_map(&walk_out);
			memcpy(t_dst_data, t_dst_buf+t_dst_buf_idx, dst_n);
			t_dst_buf_idx += dst_n;
			scatterwalk_advance(&walk_out, dst_n);
			scatterwalk_done(&walk_out, 1, left_bytes - t_dst_buf_idx);
			scatterwalk_unmap(t_dst_data);
		}
	}
	return 0;
}

static int mtk_aes_init(struct crypto_tfm *tfm)
{
	struct mtk_aes_ctx *ctx = crypto_tfm_ctx(tfm);

	return mtk_crypto_ctx_init(&ctx->pctx);
}

static void mtk_aes_exit(struct crypto_tfm *tfm)
{
	struct mtk_aes_ctx *ctx = crypto_tfm_ctx(tfm);

	mtk_crypto_ctx_uninit(&ctx->pctx);
}

static struct crypto_alg aes_algs[] = {
	{
		.cra_name		= "ctr(aes)",
		.cra_driver_name	= "mtk-ctr-aes",
		.cra_priority		= 500,
		.cra_flags		= CRYPTO_ALG_TYPE_BLKCIPHER,
		.cra_blocksize		= AES_BLOCK_SIZE,
		.cra_ctxsize		= sizeof(struct mtk_aes_ctx),
		.cra_alignmask		= 7,
		.cra_type		= &crypto_blkcipher_type,
		.cra_module		= THIS_MODULE,
		.cra_init		= mtk_aes_init,
		.cra_exit		= mtk_aes_exit,
		.cra_blkcipher = {
			.min_keysize	= AES_MIN_KEY_SIZE,
			.max_keysize	= AES_MAX_KEY_SIZE,
			.ivsize		= AES_BLOCK_SIZE,
			.setkey		= mtk_aes_setkey,
			.encrypt	= mtk_ctr_encrypt,
			.decrypt	= mtk_ctr_encrypt,
			}
	}
};

static int __init mtk_aes_mod_init(void)
{
	return crypto_register_algs(aes_algs, ARRAY_SIZE(aes_algs));
}

static void __exit mtk_aes_mod_fini(void)
{
	crypto_unregister_algs(aes_algs, ARRAY_SIZE(aes_algs));
}

late_initcall(mtk_aes_mod_init);
module_exit(mtk_aes_mod_fini);
MODULE_DESCRIPTION("AES-CTR using MTK Crypto Hardware");
