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

#ifndef __MT_DCM_H__
#define __MT_DCM_H__

int mt_dcm_disable(void);
int mt_dcm_restore(void);
int mt_dcm_topckg_disable(void);
int mt_dcm_topckg_enable(void);
int mt_dcm_topck_off(void);
int mt_dcm_topck_on(void);
int mt_dcm_peri_off(void);
int mt_dcm_peri_on(void);

#endif /* #ifndef __MT_DCM_H__ */
