/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/threads.h>
#include "mach/mt_ppm_api.h"
#include "gl_typedef.h"

#define FREQUENCY_G(x) (x*1000*1000) /*parameter for mt_ppm_sysboost_freq in kHZ*/

INT_32 kalBoostCpu(UINT_32 core_num)
{
	INT_32 freq = core_num != 0 ? FREQUENCY_G(2) : 0;
	INT_32 max_cpu_num = num_possible_cpus();

	pr_warn("kalBoostCpu, core_num:%d, max_cpu:%d\n", core_num, max_cpu_num);
	if (core_num > max_cpu_num)
		core_num = max_cpu_num;

	mt_ppm_sysboost_core(BOOST_BY_WIFI, core_num);
	mt_ppm_sysboost_freq(BOOST_BY_WIFI, freq);

	return 0;
}
