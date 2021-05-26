/*
 * Copyright (C) 2016 MediaTek Inc.
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

#include <linux/kernel.h>
#include <linux/types.h>
#include <trustzone/kree/mem.h>
#include <trustzone/kree/system.h>
#include <trustzone/tz_cross/ta_efuse.h>

#define TZ_TA_EFUSE_UUID   "5deec602-b2fe-4bb6-8c7b-be99c6ccabb0"

int tee_fuse_read(u32 fuse, u8 *data, size_t len)
{
	MTEEC_PARAM param[4];
	u32 paramTypes;
	TZ_RESULT tz_ret = TZ_RESULT_SUCCESS;
	KREE_SESSION_HANDLE efuse_session;

	tz_ret = KREE_CreateSession(TZ_TA_EFUSE_UUID, &efuse_session);
	if (tz_ret) {
		pr_err("[efuse] failed to create session: %d\n", tz_ret);
		return tz_ret;
	}

	param[0].value.a = fuse;
	param[1].mem.buffer = data;
	param[1].mem.size = len;
	paramTypes = TZ_ParamTypes2(TZPT_VALUE_INPUT, TZPT_MEM_OUTPUT);
	tz_ret = KREE_TeeServiceCall(efuse_session, 0, paramTypes, param);
	if (tz_ret)
		pr_err("[efuse] failed to do service call: %x\n", tz_ret);

	tz_ret = KREE_CloseSession(efuse_session);
	if (tz_ret)
		pr_err("[efuse] failed to close seesion: %x\n", tz_ret);

	return tz_ret;
}
