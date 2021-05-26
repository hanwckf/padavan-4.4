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

#ifndef __TRUSTZONE_TA_EFUSE__
#define __TRUSTZONE_TA_EFUSE__

/* efuse status code */
#define TZ_EFUSE_OK                0    /* eFuse is ok */
#define TZ_EFUSE_BUSY              1    /* eFuse is busy */
#define TZ_EFUSE_INVAL             2    /* input parameters is invalid */
#define TZ_EFUSE_LEN               3    /* incorrect length of eFuse value */
#define TZ_EFUSE_ZERO_VAL          4    /* can't write a zero efuse value */
#define TZ_EFUSE_WRITE_DENIED      5    /* the eFuse can't be written */
#define TZ_EFUSE_SENS_WRITE        6    /* "sensitive write" is not allowed */
#define TZ_EFUSE_ACCESS_TYPE       7    /* incorrect eFuse access type */
#define TZ_EFUSE_BLOWN             8    /* the eFuse has been blown */
#define TZ_EFUSE_NOT_IMPLEMENTED   9    /* a eFuse feature is not implemented */
#define TZ_EFUSE_READ_AFTER_WRITE  10   /*
					 * check eFuse read failed after
					 * eFuse is written
					 */
#define TZ_EFUSE_TOKEN_INVAL       11   /*
					 * token's eFuse value and length is
					 * not match with that defined in
					 * function "efuse_write"
					 */
#define TZ_EFUSE_TOKEN_MAGIC       12   /* incorrect magic number in token */
#define TZ_EFUSE_TOKEN_HRID        13   /*
					 * incorrect hardware random id
					 * in token
					 */
#define TZ_EFUSE_BUSY_LOCK         14   /* eFuse is locking */
#define TZ_EFUSE_INV_CMD           15   /* the eFuse command is invalid */
#define TZ_EFUSE_INV_SETTING       16   /* the eFuse setting is invalid */
#define TZ_EFUSE_RES               17   /*
					 * resource problem duing eFuse
					 * operation
					 */
#define TZ_EFUSE_VAL_OUT_SPEC      18   /*
					 * input eFuse value is out of
					 * sepcification
					 */
#define TZ_EFUSE_PUBK_FAILED       19   /* failed to verify public key */
#define TZ_EFUSE_SIG_FAILED        20   /* failed to verify signature */

/**
 * tee_fuse_read - read data from efuse hardware
 *
 * @fuse:   the efuse index
 * @data:   the data storing the returned efuse data
 * @len:    the data length in byte
 *
 * Return: Return 0 if the command is done,
 *	   otherwise it is failed (efuse status code).
 */
int tee_fuse_read(u32 fuse, u8 *data, size_t len);

#endif /* __TRUSTZONE_TA_EFUSE__ */
