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

#ifndef __TZ_MTK_CRYPTO_API_H__
#define __TZ_MTK_CRYPTO_API_H__

#define SHA256_ALIGN_SIZE	256

#define SHA256_BLOCK_SIZE	64
#define SHA256_BLOCK_SIZE_MASK (64-1)
#define SHA256_BLOCK_SIZE_SHIFT 6

#define AES_ALIGN_SIZE		16
#define AES_IV_SIZE 16

/* 0 -> ecb, 1 -> cbc, 2 -> ctr, 3 -> GCM */
#define AES_ECB_MOD 0
#define AES_CBC_MOD 1
#define AES_CTR_MOD 2
#define AES_GCM_MOD 3

#define AES_OP_MODE_ENC  0         /* [in]  0 -> enc, 1 -> dec */
#define AES_OP_MODE_DEC  1

#define DEV_KDF_AES128_PARAM_SZ  16  /* size for crypto and hash by using AES128 */

#define ERROR_MTK_CRYPTO_OK							0
#define ERROR_MTK_CRYPTO_CTX_IS_NULL				1
#define ERROR_MTK_CRYPTO_SHA256_FAIL				2
#define ERROR_MTK_CRYPTO_INPUT_IS_NULL				3
#define ERROR_MTK_CRYPTO_OUTPUT_IS_NULL				4
#define ERROR_MTK_CRYPTO_INPUT_LENGTH_NOT_ALIGNED	5
#define ERROR_MTK_CRYPTO_CREATE_GCPU_HANDLE_FAIL	6
#define ERROR_MTK_CRYPTO_KEY_IS_NULL				7
#define ERROR_MTK_CRYPTO_IV_IS_NULL					8
#define ERROR_MTK_CRYPTO_GCPU_AES_ENC_FAIL			9
#define ERROR_MTK_CRYPTO_GCPU_SET_SLOT_DATA_FAIL	10
#define ERROR_MTK_CRYPTO_BLK_MODE_NOT_DEFINED       11
#define ERROR_MTK_CRYPTO_KEY_LENGTH_NOT_SUPPORTED   12
#define ERROR_MTK_CRYPTO_BUFFER_NOT_IN_DRAM         13
#define ERROR_MTK_CRYPTO_GCM_DECRYPT_FAILED         14
#define ERROR_MTK_CRYPTO_GCPU_EXCUTE_CMD_FAIL       15
#define ERROR_MTK_CRYPTO_GCM_ADD_NOT_SET            16
#define ERROR_MTK_CRYPTO_MTEE_OPERATION_FAIL        17
#define ERROR_MTK_CRYPTO_KEY_IDX_NOT_SUPPORT        18
#define ERROR_MTK_CRYPTO_CTX_BUSY                   19


typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef unsigned char GCPU_INST_HANDLE_T;

/* mtk crypto driver context */
struct mtk_crypto_ctx {
      /* TEE session information and intermediate data for SHA */
	  GCPU_INST_HANDLE_T gcpu_handle;
	  /* for sha256*/
	  u8 first_packet;
	  u8 init_hash[32];
	  u8 res_hash[32];
	  u64 bit_count;    /* Bit Count for previouse data, for first packet, it should be zero */
	  u8 buf[64];
	  u32 buf_idx;
	  void *mutex;
	  /*for aes-gcm */
	  u8 *paad;
	  u32 aad_len;
	  u8  tag[16];
};

/*
*    initialize mtk crypto driver
*    ret >=0 success
*		<0 fail
*/
extern int mtk_crypto_driver_init(void);

/*
*    un-initialize mtk crypto driver
*    ret >=0 success
*       <0 fail
*/
extern int mtk_crypto_driver_uninit(void);

/*
*   initialize mtk crypto driver context
*    ret >=0 success
*       <0 fail
*/
extern int mtk_crypto_ctx_init
(
struct mtk_crypto_ctx *pctx /* [in/out] memory prepared by caller */
);

/*
*   un-initialize mtk crypto driver context
*   ret >=0 success
*       <0 fail
*/
extern int mtk_crypto_ctx_uninit
(
struct mtk_crypto_ctx *pctx /* [in/out] memory prepared by caller */
);

/*
*    random number generation
*    ret >=0 success
*       <0 fail
*/
extern int mtk_crypto_rng_gen(
struct mtk_crypto_ctx *pctx, /* [in]  context of mtk crypto driver */
u8 *pbuf,                    /* [out] destination data buffer, memory prepared by caller */
u32 buf_len                  /* [in]  buffer length (byte unit) */
);

/*
*    configure additional authenticated data aes gcm mode
*    ret >=0 success
*        <0 fail
*    note
*      It should be called before using aes gcm mode for the crypto context
*/
extern int mtk_crypto_cfg_aes_aad
(
struct mtk_crypto_ctx *pctx,	/* [in]  context of mtk crypto driver */
const u8 *paad,			/* [in]  aad buffer */
u32       aad_len,		/* [in]  aad length (byte unit) */
u8 *tag
);

extern int mtk_crypto_get_aes_tag
(
struct mtk_crypto_ctx *pctx,	/* [in]  context of mtk crypto driver */
u8 *tag				/* tag length = 16 */
);

/*
*    aes enc/dec ecb mode
*    ret >=0 success
*		<0 fail
*/
extern int mtk_crypto_aes
(
struct mtk_crypto_ctx *pctx,	/* [in]  context of mtk crypto driver */
u8        blk_mode,		/* [in]  0 -> ecb, 1 -> cbc, 2 -> ctr, 3 -> GCM */
u8        op_mode,		/* [in]  0 -> enc, 1 -> dec */
const u8 *piv,			/* [in]  initial vector or initial counter value buffer, 16 bytes */
							/*        == NULL for ecb mode */
							/*        != NULL for cbc/ctr mode */
const u8 *pkey,              /* [in]  key buffer */
u32       key_len,           /* [in]  key length (byte unit), 16 (128 bits), 24(192 bits), 32(256 bits) */
const u8 *psrc,              /* [in]  source data buffer */
u8 *pdes,                    /* [out] destination data buffer, memory prepared by caller */
u32       dat_len            /* [in]  data buffer length for source/destination (byte unit), multiple of 16 */
);

/*
*    aes enc/dec using hw key in crypto hw
*    ret >=0 success
*		<0 fail
*/
extern int mtk_crypto_aes_using_hw_key
(
struct mtk_crypto_ctx *pctx, /* [in]  context of mtk crypto driver */
u8        key_idx,           /* [in]  0 -> per device key (128bits) */
u8        blk_mode,          /* [in]  0 -> ecb, 1 -> cbc, 2 -> ctr */
u8        op_mode,           /* [in]  0 -> enc, 1 -> dec */
const u8 *piv,               /* [in]  initial vector or initial counter value buffer, 16 bytes */
							/*        == NULL for ecb mode */
							/*        != NULL for cbc/ctr mode */
const u8 *psrc,              /* [in]  source data buffer */
u8 *pdes,              /* [out] destination data buffer, memory prepared by caller */
u32       dat_len            /* [in]  data buffer length for source/destination (byte unit), multiple of 16 */
);

/*
*    begin to do sha256
*    ret >=0 success
*		<0 fail
*/
extern int mtk_crypto_sha256_begin
(
struct mtk_crypto_ctx *pctx /* context of mtk crypto driver */
);

/*
*    process data for sha256
*    ret >=0 success
*		<0 fail
*/
extern int mtk_crypto_sha256_process
(
struct mtk_crypto_ctx *pctx, /* [in] context of mtk crypto driver */
const u8 *pdat,              /* [in] data buffer */
u32       dat_len            /* [in] data buffer length (byte unit) */
);

/*
*    generate sha256 hash value
*    ret >=0 success
*		<0 fail
*/
extern int mtk_crypto_sha256_done
(
struct mtk_crypto_ctx *pctx, /* [in]  context of mtk crypto driver */
u8 *pres                     /* [out] hash result buffer, 32 bytes, buffer prepared by caller */
);

/*
*    generate sha256 hash value on data
*    ret >=0 success
*		<0 fail
*/
extern int mtk_crypto_sha256
(
struct mtk_crypto_ctx *pctx, /* [in]  context of mtk crypto driver */
const u8 *pdat,              /* [in]  data buffer */
u32       dat_len,           /* [in]  data buffer length (byte unit) */
u8 *pres               /* [out] hash result buffer, 32 bytes */
);

#endif
/* __TZ_MTK_CRYPTO_API_H__ */
