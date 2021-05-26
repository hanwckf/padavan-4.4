/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef __TRUSTZONE_EFUSE_INFO__
#define __TRUSTZONE_EFUSE_INFO__

struct efuse_entry {
	u32 id;
	size_t len;
};

#define EFUEE_INFO(_id, _len) {   \
	.id = _id,                  \
	.len = _len, /* byte length */ \
}

/* efuse index */
enum efuse_index {
	SBC_PUBK0_HASH = 0,
	SBC_PUBK1_HASH,
	SBC_PUBK2_HASH,
	SBC_PUBK3_HASH,
	SBC_PUBK0_HASH_LOCK,
	SBC_PUBK1_HASH_LOCK,
	SBC_PUBK2_HASH_LOCK,
	SBC_PUBK3_HASH_LOCK,
	JTAG_DIS,
	SBC_EN,
	DAA_EN, /* 10 */
	SEC_CTRL_LOCK,
	JTAG_CTL,
	PL_AR_EN,
	RSA_PSS_EN,
	CUS_DAT,
	SBC_PUBK0_DIS,
	SBC_PUBK1_DIS,
	SBC_PUBK2_DIS,
	SBC_PUBK3_DIS,
	PL_VER, /* 20 */
	RMA_EN,
	PL_VER_LOCK,
	CUS_DAT_LOCK,
	SEC_CTRL_LOCK1,
	SEC_CTRL_LOCK2,
	HRID,
	M_HW_RES4,
	M_HW2_RES2,
	SW_JTAG_CON,
	BROM_CMD_DIS, /* 30 */
	EFUSE_MAX
};

static const struct efuse_entry efuse_info[] = {
	EFUEE_INFO(SBC_PUBK0_HASH, 32),
	EFUEE_INFO(SBC_PUBK1_HASH, 32),
	EFUEE_INFO(SBC_PUBK2_HASH, 32),
	EFUEE_INFO(SBC_PUBK3_HASH, 32),
	EFUEE_INFO(SBC_PUBK0_HASH_LOCK, 1),
	EFUEE_INFO(SBC_PUBK1_HASH_LOCK, 1),
	EFUEE_INFO(SBC_PUBK2_HASH_LOCK, 1),
	EFUEE_INFO(SBC_PUBK3_HASH_LOCK, 1),
	EFUEE_INFO(JTAG_DIS, 1),
	EFUEE_INFO(SBC_EN, 1),
	EFUEE_INFO(DAA_EN, 1), /* 10 */
	EFUEE_INFO(SEC_CTRL_LOCK, 1),
	EFUEE_INFO(JTAG_CTL, 1),
	EFUEE_INFO(PL_AR_EN, 1),
	EFUEE_INFO(RSA_PSS_EN, 1),
	EFUEE_INFO(CUS_DAT, 16),
	EFUEE_INFO(SBC_PUBK0_DIS, 1),
	EFUEE_INFO(SBC_PUBK1_DIS, 1),
	EFUEE_INFO(SBC_PUBK2_DIS, 1),
	EFUEE_INFO(SBC_PUBK3_DIS, 1),
	EFUEE_INFO(PL_VER, 16), /* 20 */
	EFUEE_INFO(RMA_EN, 1),
	EFUEE_INFO(PL_VER_LOCK, 1),
	EFUEE_INFO(CUS_DAT_LOCK, 1),
	EFUEE_INFO(SEC_CTRL_LOCK1, 1),
	EFUEE_INFO(SEC_CTRL_LOCK2, 1),
	EFUEE_INFO(HRID, 8),
	EFUEE_INFO(M_HW_RES4, 4),
	EFUEE_INFO(M_HW2_RES2, 4),
	EFUEE_INFO(SW_JTAG_CON, 1),
	EFUEE_INFO(BROM_CMD_DIS, 1), /* 30 */
};

#endif /* __TRUSTZONE_EFUSE_INFO__ */
