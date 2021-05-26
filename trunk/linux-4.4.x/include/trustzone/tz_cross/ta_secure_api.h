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

#ifndef __TRUSTZONE_TA_SECURE_API__
#define __TRUSTZONE_TA_SECURE_API__

#define TZ_TA_SECURE_API_UUID   "42a10730-f349-e1f2-a99a-d4856458b22d"

/* Command for Debug */
#define TZCMD_SECURE_API_CRYPTO_CTX_INIT			0
#define TZCMD_SECURE_API_CRYPTO_CTX_UNINIT			1
#define TZCMD_SECURE_API_CRYPTO_RGN_GEN				2
#define TZCMD_SECURE_API_CRYPTO_AES					3
#define TZCMD_SECURE_API_CRYPTO_AES_USING_HW_KEY	4
#define TZCMD_SECURE_API_CRYPTO_SHA256_BEGIN		5
#define TZCMD_SECURE_API_CRYPTO_SHA256_PROCESS		6
#define TZCMD_SECURE_API_CRYPTO_SHA256_DONE			7
#define TZCMD_SECURE_API_CRYPTO_SHA256				8
#define TZCMD_SECURE_API_CRYPTO_SET_AAD                 9

#endif /* __TRUSTZONE_TA_SECURE_API__ */
