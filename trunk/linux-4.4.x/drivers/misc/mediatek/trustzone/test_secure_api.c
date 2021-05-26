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

#include "tz_mtk_crypto_api.h"
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>

#define HEXTD(x) (((x) <= '9')?((x)-'0'):(((x)&7) + 9))

static void convert_str_to_value(const char *str, u8 *value)
{
	int i;
	int high;
	int low;

	for (i = 0; i < strlen(str); i += 2) {
		high = HEXTD(str[i]);
		low = HEXTD(str[i+1]);
		value[i/2] = (high << 4) + low;
	}

}

static void dump_hex(const char *str, const void *vbuf, size_t len)
{
	/*a string and then
	  *16 bytes per line with
	  *1 space in middle and newline at end (and null)
	  */
	char line[8*2 + 1 + 8*2 + 1 + 1];
	const u8 *buf = (const u8 *)vbuf;
	size_t i;

	WARN_ON(len % 16);
	pr_warn("%s (%zu bytes):\n", str, len);
	for (i = 0; i < len; i += 16) {
		snprintf(line, sizeof(line),
			 "%02x%02x%02x%02x%02x%02x%02x%02x %02x%02x%02x%02x%02x%02x%02x%02x\n",
			 buf[i], buf[i + 1], buf[i + 2], buf[i + 3],
			 buf[i + 4], buf[i + 5], buf[i + 6], buf[i + 7],
			 buf[i + 8], buf[i + 9], buf[i + 10], buf[i + 11],
			 buf[i + 12], buf[i + 13], buf[i + 14], buf[i + 15]);
		pr_warn("%s", line);
	}
}

struct aes_test_vector {
	int mode;
	const char *testname;
	const char *key;
	const char *iv;
	const char *plaintext;
	const char *ciphertext;
};

int test_secure_api_thread(void *param)
{
	struct aes_test_vector aes_tests[] = {
		{
			AES_CBC_MOD,
			"AES_CBC_MOD",
			"2b7e151628aed2a6abf7158809cf4f3c",
			"7649ABAC8119B246CEE98E9B12E9197D",
			"ae2d8a571e03ac9c9eb76fac45af8e51",
			"5086cb9b507219ee95db113a917678b2"
		},
		{
			AES_ECB_MOD,
			"AES_ECB_MOD",
			"2b7e151628aed2a6abf7158809cf4f3c",
			NULL,
			"6bc1bee22e409f96e93d7e117393172a",
			"3ad77bb40d7a3660a89ecaf32466ef97"
		},
		{
			AES_CTR_MOD,
			"AES_CTR_MOD",
			"2b7e151628aed2a6abf7158809cf4f3c",
			"f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
			"6bc1bee22e409f96e93d7e117393172a",
			"874d6191b620e3261bef6864990db6ce"
		},

		{ 0, NULL },
	};

	u8  origina_str[256];
	u8	encrypt_str[256];
	u8  encrypt_ref_str[16];
	u8  decrypt_str[256];
	u8 *sha256_origina = NULL;
	u8 *sha256_buffer1 = NULL;
	u8 sha256_buffer2[32];

	u8  iv[16];
	u8  key_len[3] = {16, 24, 32};
	u8  key[32];
	u8  blk_mod;
	u8  tag[16];

	int i;
	int count = 0;
	int times;
	int rc;
	int result = -1;

	/* alloc mtk_crypto_ctx */
	struct mtk_crypto_ctx *pctx;

	mdelay(5000);
	pr_warn("delay to wait gcpu driver\n");

	pctx = kmalloc(sizeof(struct mtk_crypto_ctx), GFP_KERNEL);

	/* init pctx */
	mtk_crypto_ctx_init(pctx);

	times = 1;

	pr_warn("2. test mtk_crypto_aes\n");
	for (count = 0; count < times; count++) {
		/* for different key length */
		for (i = 0; i < 3; i++) {
			/* test different mode */
			for (blk_mod = 0; blk_mod <= AES_GCM_MOD; blk_mod++) {
				/* generate origina_str */
				rc = mtk_crypto_rng_gen(pctx, origina_str,
										sizeof(origina_str));
				if (rc != 0) {
					pr_warn("mtk_crypto_rng_gen failed: %d\n", rc);
					goto end;
				}

				/* generate iv */
				rc = mtk_crypto_rng_gen(pctx, iv, sizeof(iv));
				if (rc != 0) {
					pr_warn("mtk_crypto_rng_gen failed: %d\n", rc);
					goto end;
				}

				/* generate key */
				rc = mtk_crypto_rng_gen(pctx, key, key_len[i]);
				if (rc != 0) {
					pr_warn("mtk_crypto_rng_gen failed: %d\n", rc);
					goto end;
				}

				pr_warn("begin mtk_crypto_aes, mode:%d, key length: %d\n", blk_mod, key_len[i]);

				memset(encrypt_str, 0, 256);
				memset(decrypt_str, 0, 256);

				if (blk_mod == AES_GCM_MOD) {
					/* set iv as aad */
					mtk_crypto_cfg_aes_aad(pctx, iv, 16, NULL);
				}

				/* test encrypt */
				rc = mtk_crypto_aes(pctx, blk_mod,
						    AES_OP_MODE_ENC, iv,
						    key, key_len[i],
						    origina_str, encrypt_str,
						    256);
				if (rc != 0) {
					pr_warn("mtk_crypto_aes enc failed: %d\n", rc);
					goto end;
				}

				if (blk_mod == AES_GCM_MOD) {
					/* get tag */
					mtk_crypto_get_aes_tag(pctx, tag);

					/* set tag and aad before decrypt */
					mtk_crypto_cfg_aes_aad(pctx, iv, 16, tag);
				}

				/* test decrypt */
				rc = mtk_crypto_aes(pctx, blk_mod,
								AES_OP_MODE_DEC, iv,
								key, key_len[i],
								encrypt_str, decrypt_str,
								256);
				if (rc != 0) {
					pr_warn("mtk_crypto_aes dec failed: %d\n", rc);
					goto end;
				}

				/* compare decrypt_str to origina_str */
				if (memcmp(origina_str, decrypt_str, 256)) {
					pr_warn("AES memcmp fail, abort test\n");
					goto end;
				}

				pr_warn("success on test mtk_crypto_aes, mode\n");

				memset(encrypt_str, 0, 256);
				memset(decrypt_str, 0, 256);

				pr_warn("begin test mtk_crypto_aes_using_hw_key\n");
				if (blk_mod == AES_GCM_MOD) {
					/* set iv as aad */
					mtk_crypto_cfg_aes_aad(pctx, iv, 16, NULL);
				}

				/* test hwkey encrypt */
				rc = mtk_crypto_aes_using_hw_key(pctx, 0,
												blk_mod,
												AES_OP_MODE_ENC,
												iv,
												origina_str,
												encrypt_str,
												256);
				if (rc != 0) {
					pr_warn("mtk_crypto_aes_hw_key enc failed: %d\n", rc);
					goto end;
				}
				if (blk_mod == AES_GCM_MOD) {
					/* get tag */
					mtk_crypto_get_aes_tag(pctx, tag);

					/* set tag and aad before decrypt */
					mtk_crypto_cfg_aes_aad(pctx, iv, 16, tag);
				}
				/* test hwkey decrypt */
				rc = mtk_crypto_aes_using_hw_key(pctx, 0,
												blk_mod,
												AES_OP_MODE_DEC,
												iv,
												encrypt_str,
												decrypt_str,
												256);
				if (rc != 0) {
					pr_warn("mtk_crypto_aes_hw_key dec failed: %d\n", rc);
					goto end;
				}

				/* compare decrypt_str to origina_str */
				if (memcmp(origina_str, decrypt_str, 256)) {
					pr_warn("test fail, abort test\n");
					return -1;
				}

				pr_warn("success on test mtk_crypto_aes_using_hw_key\n");
			}

		}
	}

	#define SHA256_TEST_SIZE (4096*10)
	#define TEST_BLOCK_SIZE 35  /*for arbitrary size test*/
	sha256_origina = kmalloc(SHA256_TEST_SIZE, GFP_KERNEL);
	sha256_buffer1 = kmalloc(32, GFP_KERNEL);
	if (sha256_origina == NULL || sha256_buffer1 == NULL) {
		pr_warn("kmalloc failure\n");
		goto end;
	}

	/* test sha256 */
	rc = mtk_crypto_rng_gen(pctx, sha256_origina, SHA256_TEST_SIZE);
	if (rc != 0) {
		pr_warn("mtk_crypto_rng_gen failed: %d\n", rc);
		goto end;
	}
	memset(sha256_buffer1, 0, 32);
	memset(sha256_buffer2, 0, 32);

	pr_warn("begin mtk_crypto_sha256 test\n");

	/* calculate sha256 in one time */
	rc = mtk_crypto_sha256(pctx, sha256_origina, SHA256_TEST_SIZE, sha256_buffer2);
	if (rc != 0) {
		pr_warn("mtk_crypto_sha256 failed: %d\n", rc);
		goto end;
	}

	/* calculate in more than one time */
	pr_warn("begin mtk_crypto_sha256 multi-call test\n");
	rc = mtk_crypto_sha256_begin(pctx);
	if (rc != 0) {
		pr_warn("mtk_crypto_sha256_begin failed: %d\n", rc);
		goto end;
	}

	for (i = 0; i < SHA256_TEST_SIZE; i += TEST_BLOCK_SIZE) {
		int dat_len = (i + TEST_BLOCK_SIZE >= SHA256_TEST_SIZE ? SHA256_TEST_SIZE-i : TEST_BLOCK_SIZE);

		rc = mtk_crypto_sha256_process(pctx, sha256_origina + i, dat_len);
		if (rc != 0) {
			pr_warn("mtk_crypto_sha256_process failed: %d\n", rc);
			goto end;
		}
	}

	rc = mtk_crypto_sha256_done(pctx, sha256_buffer1);
	if (rc != 0) {
		pr_warn("mtk_crypto_sha256_done failed: %d\n", rc);
		goto end;
	}

	if (memcmp(sha256_buffer1, sha256_buffer2, 32)) {
		pr_warn("test fail, abort test\n");
		goto end;
	}

	pr_warn("sha256 test success\n");
	dump_hex("SHA256 input", sha256_origina, SHA256_TEST_SIZE);
	dump_hex("SHA256 output", sha256_buffer1, 32);

	for (i = 0; aes_tests[i].testname; i++) {
		memset(iv, 0, 16);
		memset(key, 0, 32);

		memset(origina_str, 0, 256);
		memset(encrypt_str, 0, 256);
		memset(decrypt_str, 0, 256);

		pr_warn("test %s:\n", aes_tests[i].testname);
		convert_str_to_value(aes_tests[i].plaintext, origina_str);
		convert_str_to_value(aes_tests[i].key, key);
		if (aes_tests[i].iv)
			convert_str_to_value(aes_tests[i].iv, iv);
		convert_str_to_value(aes_tests[i].ciphertext, encrypt_ref_str);

		/* test encrypt */
		rc = mtk_crypto_aes(pctx, aes_tests[i].mode, AES_OP_MODE_ENC,
				    iv, key, 16, origina_str, encrypt_str, 16);
		if (rc != 0) {
			pr_warn("mtk_crypto_aes enc failed: %d\n", rc);
			goto end;
		}

		if (memcmp(encrypt_str, encrypt_ref_str, 16) != 0) {
			pr_warn("enc memcmp failed\n");
			goto end;
		}

		/* test decrypt */
		rc = mtk_crypto_aes(pctx, aes_tests[i].mode, AES_OP_MODE_DEC,
				    iv, key, 16, encrypt_str, decrypt_str, 16);
		if (rc != 0) {
			pr_warn("mtk_crypto_aes dec failed: %d\n", rc);
			goto end;
		}

		if (memcmp(decrypt_str, origina_str, 16) != 0) {
			pr_warn("dec memcmp failed\n");
			goto end;
		}
	}

	/* uninit */
	rc = mtk_crypto_ctx_uninit(pctx);
	if (rc < 0) {
		pr_warn("mtk_crypto_ctx_uninit failed: %d\n", rc);
		goto end;
	}

	pr_warn("all test passed!\n");
	result = 0;

end:
	kfree(sha256_origina);
	kfree(sha256_buffer1);
	kfree(pctx);
	return result;
}
