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

#include "mach/mt_ppm_api.h"
#include "gl_typedef.h"
INT32 kalBoostCpu(UINT_32 core_num)
{
	pr_warn("enter kalBoostCpu, core_num:%d\n", core_num);
	mt_ppm_sysboost_core(BOOST_BY_WIFI, core_num);
	return 0;
}
