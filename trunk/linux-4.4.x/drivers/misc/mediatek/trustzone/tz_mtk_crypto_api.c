/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include "tz_cross/trustzone.h"
#include "tz_cross/ta_system.h"
#include "tz_cross/ta_mem.h"
#include "trustzone/kree/system.h"
#include "trustzone/kree/mem.h"
#include "tz_cross/ree_service.h"

#include "tz_cross/ta_secure_api.h"
#include "tz_mtk_crypto_api.h"

static KREE_SESSION_HANDLE secure_api_session;

static int isKernelLowmem(const void *buf)
{
	if (buf < (void *)PAGE_OFFSET ||
		buf >= high_memory) {
		return 0;
		}
	return 1;
}
/*
*    initialize mtk crypto driver
*    ret >=0 success
*       <0 fail
*/
int mtk_crypto_driver_init(void)
{
	TZ_RESULT ret;

	ret = KREE_CreateSession(TZ_TA_SECURE_API_UUID, &secure_api_session);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("mtk crypto driver init fail, error:%d\n", ret);
		return -ERROR_MTK_CRYPTO_MTEE_OPERATION_FAIL;
	}

	return 0;
}

/*
*    un-initialize mtk crypto driver
*    ret >=0 success
*       <0 fail
*/
int mtk_crypto_driver_uninit(void)
{
	TZ_RESULT ret;

	ret = KREE_CloseSession(secure_api_session);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("mtk crypto driver init fail %d\n", ret);
		return -ERROR_MTK_CRYPTO_MTEE_OPERATION_FAIL;
	}

	return 0;
}

/*
*    initialize mtk crypto driver context
*    ret >=0 success
*	<0 fail
*/
int mtk_crypto_ctx_init(
struct mtk_crypto_ctx *pctx /* [in/out] memory prepared by caller */
)
{
	TZ_RESULT ret;
	MTEEC_PARAM param[4];

	if (pctx == NULL)
		return -ERROR_MTK_CRYPTO_CTX_IS_NULL;

	param[0].mem.buffer = (void *)pctx;
	param[0].mem.size = sizeof(struct mtk_crypto_ctx);

	/* send command */
	ret = KREE_TeeServiceCall(secure_api_session, TZCMD_SECURE_API_CRYPTO_CTX_INIT,
								TZ_ParamTypes1(TZPT_MEM_OUTPUT), param);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("KREE_TeeServiceCall fail, error:%d\n", ret);
		return -ERROR_MTK_CRYPTO_MTEE_OPERATION_FAIL;
	}

	pctx->mutex = kmalloc(sizeof(struct mutex), GFP_KERNEL);
	mutex_init((struct mutex *)pctx->mutex);

	return 0;
}
EXPORT_SYMBOL(mtk_crypto_ctx_init);

/*
*    un-initialize mtk crypto driver context
*    ret >=0 success
*       <0 fail
*/
int mtk_crypto_ctx_uninit(
struct mtk_crypto_ctx *pctx /* [in/out] memory prepared by caller */
)
{
	MTEEC_PARAM param[4];
	TZ_RESULT ret;

	if (pctx == NULL)
		return -ERROR_MTK_CRYPTO_CTX_IS_NULL;

	kfree(pctx->mutex);

	param[0].mem.buffer = (void *)pctx;
	param[0].mem.size = sizeof(struct mtk_crypto_ctx);

	/* send command */
	ret = KREE_TeeServiceCall(secure_api_session, TZCMD_SECURE_API_CRYPTO_CTX_UNINIT,
								TZ_ParamTypes1(TZPT_MEM_INPUT), param);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("KREE_TeeServiceCall fail, error:%d\n", ret);
		return -ERROR_MTK_CRYPTO_MTEE_OPERATION_FAIL;
	}

	return 0;
}
EXPORT_SYMBOL(mtk_crypto_ctx_uninit);

/*
*    random number generation
*    ret >=0 success
*       <0 fail
*/
int mtk_crypto_rng_gen(
struct mtk_crypto_ctx *pctx, /* [in]  context of mtk crypto driver */
u8 *pbuf,                    /* [out] destination data buffer, memory prepared by caller */
u32 buf_len                  /* [in]  buffer length (byte unit) */
)
{
	MTEEC_PARAM param[4];
	TZ_RESULT ret;
	int copied = 0;

	if (pctx == NULL)
		return -ERROR_MTK_CRYPTO_CTX_IS_NULL;

	if (pbuf == NULL)
		return -ERROR_MTK_CRYPTO_OUTPUT_IS_NULL;

	param[0].mem.buffer = (void *)pctx;
	param[0].mem.size = sizeof(struct mtk_crypto_ctx);
	if (isKernelLowmem(pbuf)) {
		param[1].mem.buffer = (void *)pbuf;
	} else {
		param[1].mem.buffer = kmalloc(buf_len, GFP_KERNEL);
		if (!param[1].mem.buffer)
			return -ENOMEM;
		copied = 1;
	}
	param[1].mem.size = buf_len;

	/* send command */
	ret = KREE_TeeServiceCall(secure_api_session, TZCMD_SECURE_API_CRYPTO_RGN_GEN,
								TZ_ParamTypes2(TZPT_MEM_INPUT, TZPT_MEM_OUTPUT), param);
	if (ret != TZ_RESULT_SUCCESS) {
		if (copied)
			kfree(param[copied].mem.buffer);
		pr_warn("KREE_TeeServiceCall fail, error:%d\n", ret);
		return -ERROR_MTK_CRYPTO_MTEE_OPERATION_FAIL;
	}

	if (copied) {
		memcpy(pbuf, param[1].mem.buffer, buf_len);
		kfree(param[copied].mem.buffer);
	}
	return 0;
}
EXPORT_SYMBOL(mtk_crypto_rng_gen);

/*
*    configure additional authenticated data aes gcm mode
*    ret >=0 success
*        <0 fail
*    note
*      It should be called before using aes gcm mode for the crypto context
*/
int mtk_crypto_cfg_aes_aad(
struct mtk_crypto_ctx *pctx, /* [in]  context of mtk crypto driver */
const u8 *paad,              /* [in]  aad buffer */
u32       aad_len,          /* [in]  aad length (byte unit) */
u8 *tag
) {
	MTEEC_PARAM param[4];
	TZ_RESULT ret;

	if (pctx == NULL)
		return -ERROR_MTK_CRYPTO_CTX_IS_NULL;

	if (paad == NULL && aad_len != 0)
		return -ERROR_MTK_CRYPTO_INPUT_IS_NULL;

	param[0].mem.buffer = (void *)pctx;
	param[0].mem.size = sizeof(struct mtk_crypto_ctx);
	param[1].mem.buffer = (void *)paad;
	param[1].mem.size = aad_len;

	if (tag != NULL) {
		param[2].mem.buffer = (void *)tag;
		param[2].mem.size = 16;
	} else {
		param[2].mem.buffer = NULL;
		param[2].mem.size = 0;
	}

	/* send command */
	ret = KREE_TeeServiceCall(secure_api_session, TZCMD_SECURE_API_CRYPTO_SET_AAD,
			TZ_ParamTypes3(TZPT_MEM_INOUT, TZPT_MEM_INPUT, TZPT_MEM_INOUT), param);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("KREE_TeeServiceCall fail, error:%d\n", ret);
		return -ERROR_MTK_CRYPTO_MTEE_OPERATION_FAIL;
	}
	return 0;
}
EXPORT_SYMBOL(mtk_crypto_cfg_aes_aad);

int mtk_crypto_get_aes_tag(
struct mtk_crypto_ctx *pctx, /* [in]  context of mtk crypto driver */
u8 *tag					 /* tag length = 16 */
) {
	memcpy(tag, pctx->tag, 16);
	return 0;
}
EXPORT_SYMBOL(mtk_crypto_get_aes_tag);

/*
*    aes enc/dec ecb mode
*    ret >=0 success
*         <0 fail
*/
static int mtk_crypto_aes_helper(
struct mtk_crypto_ctx *pctx,	/* [in]  context of mtk crypto driver */
u8        blk_mode,		/* [in]  0 -> ecb, 1 -> cbc, 2 -> ctr, 3 -> GCM */
u8        op_mode,		/* [in]  0 -> enc, 1 -> dec */
const u8 *piv,			/* [in]  initial vector or initial counter value buffer, 16 bytes */
				/*	== NULL for ecb mode */
				/*	!= NULL for cbc/ctr mode */
const u8 *pkey,			/* [in]  key buffer */
u32       key_len,		/* [in]  key length (byte unit), 16 (128 bits), 24(192 bits), 32(256 bits) */
const u8 *psrc,			/* [in]  source data buffer */
u8 *pdes,			/* [out] destination data buffer, memory prepared by caller */
u32       dat_len		/* [in]  data buffer length for source/destination (byte unit), multiple of 16 */
)
{
	MTEEC_PARAM param[4];
	int param_index = 0;
	const int using_hardware = pkey == NULL;

	u8 *packed_buffer = NULL;

	TZ_RESULT ret;
	int result = -1;
	int src_copied = 0;
	int dst_copied = 0;

	if (pctx == NULL)
		return -ERROR_MTK_CRYPTO_CTX_IS_NULL;

	if (psrc == NULL)
		return -ERROR_MTK_CRYPTO_INPUT_IS_NULL;

	if (pdes == NULL)
		return -ERROR_MTK_CRYPTO_OUTPUT_IS_NULL;

	if ((AES_CBC_MOD == blk_mode || AES_CTR_MOD == blk_mode) && piv == NULL)
		return -ERROR_MTK_CRYPTO_IV_IS_NULL;

	if (dat_len % AES_ALIGN_SIZE != 0)
		return -ERROR_MTK_CRYPTO_INPUT_LENGTH_NOT_ALIGNED;

	/* arrange params */
	packed_buffer = kmalloc(AES_IV_SIZE + sizeof(u8) * 2 + sizeof(struct mtk_crypto_ctx), GFP_KERNEL);
	if (packed_buffer == NULL)
		goto end;

	memset(packed_buffer, 0, AES_IV_SIZE + sizeof(u8) * 2 + sizeof(struct mtk_crypto_ctx));
	if (blk_mode != AES_ECB_MOD)
		memcpy(packed_buffer, piv, AES_IV_SIZE);
	*(packed_buffer + AES_IV_SIZE) = op_mode;
	*(packed_buffer + AES_IV_SIZE + sizeof(u8)) = blk_mode;
	memcpy(packed_buffer + AES_IV_SIZE + sizeof(u8) * 2, pctx, sizeof(struct mtk_crypto_ctx));

	param[param_index].mem.buffer = (void *)packed_buffer;
	param[param_index].mem.size = AES_IV_SIZE + sizeof(u8) * 2 + sizeof(struct mtk_crypto_ctx);
	param_index++;

	if (!using_hardware) {
		param[param_index].mem.buffer = (void *)pkey;
		param[param_index].mem.size = key_len;
		param_index++;
	}

	if (isKernelLowmem(psrc)) {
		param[param_index].mem.buffer = (void *)psrc;
	} else if (dat_len > 0) {
		param[param_index].mem.buffer = kmalloc(dat_len, GFP_KERNEL);
		if (!param[param_index].mem.buffer) {
			result = -ENOMEM;
			goto end;
		}
		memcpy(param[param_index].mem.buffer, psrc, dat_len);
		src_copied = param_index;
	} else {
		param[param_index].mem.buffer = NULL;
	}
	param[param_index].mem.size = dat_len;
	param_index++;

	if (isKernelLowmem(pdes)) {
		param[param_index].mem.buffer = pdes;
	} else if (dat_len > 0) {
		param[param_index].mem.buffer = kmalloc(dat_len, GFP_KERNEL);
		if (!param[param_index].mem.buffer) {
			result = -ENOMEM;
			goto end;
		}
		dst_copied = param_index;
	} else {
		param[param_index].mem.buffer = NULL;
	}
	param[param_index].mem.size = dat_len;

	/* send command */
	if (using_hardware) {
		ret = KREE_TeeServiceCall(secure_api_session,
								TZCMD_SECURE_API_CRYPTO_AES_USING_HW_KEY,
								TZ_ParamTypes3(TZPT_MEM_INOUT,
								TZPT_MEM_INPUT,
								TZPT_MEM_OUTPUT),
								param);
	} else {
		ret = KREE_TeeServiceCall(secure_api_session,
								TZCMD_SECURE_API_CRYPTO_AES,
								TZ_ParamTypes4(TZPT_MEM_INOUT,
								TZPT_MEM_INPUT,
								TZPT_MEM_INPUT,
								TZPT_MEM_OUTPUT),
								param);
	}
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("KREE_TeeServiceCall fail, error:%d\n", ret);
		goto end;
	}

	if (dst_copied)
		memcpy(pdes, param[dst_copied].mem.buffer, dat_len);

	/* copy pctx back */
	memcpy(pctx, packed_buffer + AES_IV_SIZE + sizeof(u8) * 2, sizeof(struct mtk_crypto_ctx));
	result = 0;

end:
	if (src_copied)
		kfree(param[src_copied].mem.buffer);
	if (dst_copied)
		kfree(param[dst_copied].mem.buffer);
	kfree(packed_buffer);
	return result;
}

/*
*    aes enc/dec ecb mode
*    ret >=0 success
*         <0 fail
*/
int mtk_crypto_aes(
struct mtk_crypto_ctx *pctx,	/* [in]  context of mtk crypto driver */
u8        blk_mode,		/* [in]  0 -> ecb, 1 -> cbc, 2 -> ctr, 3 -> GCM */
u8        op_mode,		/* [in]  0 -> enc, 1 -> dec */
const u8 *piv,			/* [in]  initial vector or initial counter value buffer, 16 bytes */
				/*	== NULL for ecb mode */
				/*	!= NULL for cbc/ctr mode */
const u8 *pkey,			/* [in]  key buffer */
u32       key_len,		/* [in]  key length (byte unit), 16 (128 bits), 24(192 bits), 32(256 bits) */
const u8 *psrc,			/* [in]  source data buffer */
u8 *pdes,			/* [out] destination data buffer, memory prepared by caller */
u32       dat_len		/* [in]  data buffer length for source/destination (byte unit), multiple of 16 */
)
{
	if (pkey == NULL)
		return -ERROR_MTK_CRYPTO_KEY_IS_NULL;
	return mtk_crypto_aes_helper(pctx, blk_mode, op_mode, piv,
				     pkey, key_len, psrc, pdes, dat_len);
}
EXPORT_SYMBOL(mtk_crypto_aes);

/*
*    aes enc/dec using hw key in crypto hw
*    ret >=0 success
*         <0 fail
*/
int mtk_crypto_aes_using_hw_key(
struct mtk_crypto_ctx *pctx,	/* [in]  context of mtk crypto driver */
u8        key_idx,		/* [in]  0 -> per device key (128bits) */
u8        blk_mode,		/* [in]  0 -> ecb, 1 -> cbc, 2 -> ctr */
u8        op_mode,		/* [in]  0 -> enc, 1 -> dec */
const u8 *piv,			/* [in]  initial vector or initial counter value buffer, 16 bytes */
				/*         == NULL for ecb mode */
				/*         != NULL for cbc/ctr mode */
const u8 *psrc,			/* [in]  source data buffer */
u8 *pdes,			/* [out] destination data buffer, memory prepared by caller */
u32       dat_len		/* [in]  data buffer length for source/destination (byte unit), multiple of 16 */
)
{
	/* check key_idx, only support 0 */
	if (key_idx != 0) {
		pr_warn("key_idx only support 0 currently!\n");
		return -1;
	}
	return mtk_crypto_aes_helper(pctx, blk_mode, op_mode, piv,
				     NULL, 0, psrc, pdes, dat_len);
}
EXPORT_SYMBOL(mtk_crypto_aes_using_hw_key);

/*
*    begin to do sha256
*    ret >=0 success
*         <0 fail
*/
int mtk_crypto_sha256_begin(
struct mtk_crypto_ctx *pctx /* context of mtk crypto driver */
)
{
	MTEEC_PARAM param[4];
	TZ_RESULT ret;

	if (pctx == NULL)
		return -ERROR_MTK_CRYPTO_CTX_IS_NULL;

	if (!mutex_trylock((struct mutex *)pctx->mutex))
		return -ERROR_MTK_CRYPTO_CTX_BUSY;

	param[0].mem.buffer = (void *)pctx;
	param[0].mem.size = sizeof(struct mtk_crypto_ctx);

	/* send command */
	ret = KREE_TeeServiceCall(secure_api_session, TZCMD_SECURE_API_CRYPTO_SHA256_BEGIN,
								TZ_ParamTypes1(TZPT_MEM_INOUT), param);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("KREE_TeeServiceCall fail, ret=%d\n", ret);
		return -ERROR_MTK_CRYPTO_MTEE_OPERATION_FAIL;
	}
	return 0;
}
EXPORT_SYMBOL(mtk_crypto_sha256_begin);

/*
*    process data for sha256
*    ret >=0 success
*         <0 fail
*/
int mtk_crypto_sha256_process(
struct mtk_crypto_ctx *pctx, /* [in] context of mtk crypto driver */
const u8 *pdat,              /* [in] data buffer */
u32       dat_len            /* [in] data buffer length (byte unit) */
)
{
	TZ_RESULT ret;

	MTEEC_PARAM param[4];
	int result = -1;

	if (pctx == NULL)
		return -ERROR_MTK_CRYPTO_CTX_IS_NULL;

	if (pdat == NULL)
		return -ERROR_MTK_CRYPTO_INPUT_IS_NULL;

	if (pctx->buf_idx > 0 || (dat_len & SHA256_BLOCK_SIZE_MASK) || !isKernelLowmem(pdat)) {
		u32 wk_buf_len = pctx->buf_idx + dat_len;
		u32 wk_buf_block_count = wk_buf_len >> SHA256_BLOCK_SIZE_SHIFT;
		u8 *wk_buf = kmalloc(wk_buf_len, GFP_KERNEL);

		if (wk_buf == NULL)
			return -ENOMEM;
		memcpy(wk_buf, pctx->buf, pctx->buf_idx);
		memcpy(wk_buf + pctx->buf_idx, pdat, dat_len);

		if (wk_buf_block_count > 0) {
			param[0].mem.buffer = (void *)pctx;
			param[0].mem.size = sizeof(struct mtk_crypto_ctx);

			param[1].mem.buffer = (void *)wk_buf;
			param[1].mem.size = wk_buf_block_count << SHA256_BLOCK_SIZE_SHIFT;

			/* send command */
			ret = KREE_TeeServiceCall(secure_api_session, TZCMD_SECURE_API_CRYPTO_SHA256_PROCESS,
									TZ_ParamTypes2(TZPT_MEM_INOUT, TZPT_MEM_INPUT),
									param);
			if (ret != TZ_RESULT_SUCCESS) {
				pr_warn("KREE_TeeServiceCall fail, ret=%d\n", ret);
				kfree(wk_buf);
				goto end;
			}
		}

		if (wk_buf_len & SHA256_BLOCK_SIZE_MASK) {
			u32 left_bytes = wk_buf_len-(wk_buf_len&~SHA256_BLOCK_SIZE_MASK);

			memcpy(pctx->buf, wk_buf+wk_buf_len-left_bytes, left_bytes);
			pctx->buf_idx = left_bytes;
		} else {
			pctx->buf_idx = 0;
		}
		kfree(wk_buf);
		result = 0;
	} else {
		param[0].mem.buffer = (void *)pctx;
		param[0].mem.size = sizeof(struct mtk_crypto_ctx);

		param[1].mem.buffer = (void *)pdat;
		param[1].mem.size = dat_len;

		/* send command */
		ret = KREE_TeeServiceCall(secure_api_session, TZCMD_SECURE_API_CRYPTO_SHA256_PROCESS,
									TZ_ParamTypes2(TZPT_MEM_INOUT, TZPT_MEM_INPUT),
									param);
		if (ret != TZ_RESULT_SUCCESS) {
			pr_warn("KREE_TeeServiceCall fail, ret=%d\n", ret);
			goto end;
		}

		result = 0;
	}

end:
	return result;
}
EXPORT_SYMBOL(mtk_crypto_sha256_process);

/*
*    generate sha256 hash value
*    ret >=0 success
*         <0 fail
*/
int mtk_crypto_sha256_done(
struct mtk_crypto_ctx *pctx, /* [in]  context of mtk crypto driver */
u8 *pres                     /* [out] hash result buffer, 32 bytes, buffer prepared by caller */
)
{
	MTEEC_PARAM param[4];
	TZ_RESULT ret;

	if (pctx == NULL)
		return -ERROR_MTK_CRYPTO_CTX_IS_NULL;

	if (pres == NULL)
		return ERROR_MTK_CRYPTO_OUTPUT_IS_NULL;

	param[0].mem.buffer = (void *)pctx;
	param[0].mem.size = sizeof(struct mtk_crypto_ctx);

	param[1].mem.buffer = (void *)pres;
	param[1].mem.size = 32;

	/* send command */
	ret = KREE_TeeServiceCall(secure_api_session, TZCMD_SECURE_API_CRYPTO_SHA256_DONE,
								TZ_ParamTypes2(TZPT_MEM_INPUT, TZPT_MEM_OUTPUT), param);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("KREE_TeeServiceCall fail, ret:%d\n", ret);
		return -ERROR_MTK_CRYPTO_MTEE_OPERATION_FAIL;
	}

	mutex_unlock((struct mutex *)pctx->mutex);
	return 0;
}
EXPORT_SYMBOL(mtk_crypto_sha256_done);

/*
*    generate sha256 hash value on data
*    ret >=0 success
*         <0 fail
*/
int mtk_crypto_sha256(
struct mtk_crypto_ctx *pctx, /* [in]  context of mtk crypto driver */
const u8 *pdat,              /* [in]  data buffer */
u32       dat_len,           /* [in]  data buffer length (byte unit) */
u8 *pres                     /* [out] hash result buffer, 32 bytes */
)
{
	TZ_RESULT ret;

	MTEEC_PARAM param[4];
	int result = -1;
	int copied = 0;

	if (pctx == NULL)
		return -ERROR_MTK_CRYPTO_CTX_IS_NULL;

	if (pdat == NULL)
		return -ERROR_MTK_CRYPTO_INPUT_IS_NULL;

	if (pres == NULL)
		return -ERROR_MTK_CRYPTO_OUTPUT_IS_NULL;

	param[0].mem.buffer = (void *)pctx;
	param[0].mem.size = sizeof(struct mtk_crypto_ctx);

	if (isKernelLowmem(pdat)) {
		param[1].mem.buffer = (void *)pdat;
	} else {
		param[1].mem.buffer = kmalloc(dat_len, GFP_KERNEL);
		if (!param[1].mem.buffer)
			return -ENOMEM;
		memcpy(param[1].mem.buffer, pdat, dat_len);
		copied = 1;
	}
	param[1].mem.size = dat_len;

	param[2].mem.buffer = (void *)pres;
	param[2].mem.size = 32;

	/* send command */
	ret = KREE_TeeServiceCall(secure_api_session, TZCMD_SECURE_API_CRYPTO_SHA256,
					TZ_ParamTypes3(TZPT_MEM_INPUT, TZPT_MEM_INPUT, TZPT_MEM_OUTPUT),
					param);
	if (ret != TZ_RESULT_SUCCESS) {
		pr_warn("KREE_TeeServiceCall fail, ret=%d\n", ret);
		goto end;
	}

	result = 0;

end:
	if (copied)
		kfree(param[copied].mem.buffer);
	return result;
}
EXPORT_SYMBOL(mtk_crypto_sha256);

