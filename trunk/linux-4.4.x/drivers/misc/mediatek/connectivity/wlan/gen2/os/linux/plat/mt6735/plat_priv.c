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
#include <linux/kernel.h>
#include "mt-plat/mt_hotplug_strategy.h"
#include "gl_typedef.h"
#include <mt_vcore_dvfs.h>
#include "config.h"

UINT_8 u1VcoreEnb;

INT32 kalBoostCpu(UINT_32 core_num)
{
#if CFG_SUPPORT_CPU_BOOST
	UINT_32 cpu_num;

	pr_warn("enter kalBoostCpu, core_num:%d\n", core_num);
	cpu_num = core_num;
	if (cpu_num != 0)
		cpu_num += 2; /* For denali only, additional 2 cores for 5G HT40 peak throughput*/
	if (cpu_num > 4)
		cpu_num = 4; /* There are only 4 cores for denali */
	if (cpu_num == 0)
		cpu_num = 1; /* denali default core is 1*/

	hps_set_cpu_num_base(BASE_WIFI, cpu_num, 0);
	if ((core_num >= 3) && (u1VcoreEnb == 0)) {
		vcorefs_request_dvfs_opp(KIR_WIFI, OPPI_PERF);
		u1VcoreEnb = 1;
	} else if ((core_num < 3) && (u1VcoreEnb == 1)) {
		vcorefs_request_dvfs_opp(KIR_WIFI, OPPI_UNREQ);
		u1VcoreEnb = 0;
	}
#else
	pr_warn("enter kalBoostCpu(Not SUPPORT CPU BOOST), core_num:%d\n", core_num);
#endif
	return 0;
}

