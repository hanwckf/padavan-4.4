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

/*
 * These are dummy functions for the case that any aee config is disabled
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <mt-plat/aee.h>
#include "aee-common.h"

struct proc_dir_entry;

#ifndef CONFIG_MTK_AEE_FEATURE
void *aee_excp_regs;
__weak void aee_sram_printk(const char *fmt, ...)
{
}
EXPORT_SYMBOL(aee_sram_printk);

__weak void aee_wdt_irq_info(void)
{
}

__weak void aee_wdt_fiq_info(void *arg, void *regs, void *svc_sp)
{
}

__weak void aee_trigger_kdb(void)
{
}

__weak struct aee_oops *aee_oops_create(AE_DEFECT_ATTR attr, AE_EXP_CLASS clazz, const char *module)
{
	return NULL;
}

__weak void aee_oops_set_backtrace(struct aee_oops *oops, const char *backtrace)
{
}

__weak void aee_oops_set_process_path(struct aee_oops *oops, const char *process_path)
{
}

__weak void aee_oops_free(struct aee_oops *oops)
{
}

__weak void aee_kernel_exception_api(const char *file, const int line, const int db_opt,
				     const char *module, const char *msg, ...)
{
}
EXPORT_SYMBOL(aee_kernel_exception_api);

__weak void aee_kernel_warning_api(const char *file, const int line, const int db_opt,
				   const char *module, const char *msg, ...)
{
}
EXPORT_SYMBOL(aee_kernel_warning_api);

__weak void aee_kernel_reminding_api(const char *file, const int line, const int db_opt,
				     const char *module, const char *msg, ...)
{
}
EXPORT_SYMBOL(aee_kernel_reminding_api);

__weak void aee_kernel_dal_api(const char *file, const int line, const char *msg)
{
}
EXPORT_SYMBOL(aee_kernel_dal_api);

__weak void aed_md_exception_api(const int *log, int log_size, const int *phy, int phy_size,
				 const char *detail, const int db_opt)
{
}
EXPORT_SYMBOL(aed_md_exception_api);

__weak void aed_md32_exception_api(const int *log, int log_size, const int *phy, int phy_size,
				   const char *detail, const int db_opt)
{
}
EXPORT_SYMBOL(aed_md32_exception_api);

__weak void aed_scp_exception_api(const int *log, int log_size, const int *phy, int phy_size,
				   const char *detail, const int db_opt)
{
}
EXPORT_SYMBOL(aed_scp_exception_api);

__weak void aed_combo_exception_api(const int *log, int log_size, const int *phy, int phy_size,
				    const char *detail, const int db_opt)
{
}
EXPORT_SYMBOL(aed_combo_exception_api);

__weak void mt_fiq_printf(const char *fmt, ...)
{
}

__weak void aee_register_api(struct aee_kernel_api *aee_api)
{
}

__weak void aee_stop_nested_panic(struct pt_regs *regs)
{
}

__weak int aee_in_nested_panic(void)
{
	return 0;
}

__weak void aee_wdt_dump_info(void)
{
}

__weak void aee_wdt_printf(const char *fmt, ...)
{
}

__weak void aee_kdump_reboot(AEE_REBOOT_MODE reboot_mode, const char *msg, ...)
{
	char str[80];
	va_list ap;

	va_start(ap, msg);
	vsnprintf(str, 80, msg, ap);
	LOGE("%s", str);
	va_end(ap);
}

__weak int aed_proc_debug_init(struct proc_dir_entry *aed_proc_dir)
{
	return 0;
}

__weak int aed_proc_debug_done(struct proc_dir_entry *aed_proc_dir)
{
	return 0;
}

__weak void aee_rr_proc_init(struct proc_dir_entry *aed_proc_dir)
{
}

__weak void aee_rr_proc_done(struct proc_dir_entry *aed_proc_dir)
{
}


__weak void aee_kernel_wdt_kick_Powkey_api(const char *module, int msg)
{
}

__weak int aee_kernel_wdt_kick_api(int kinterval)
{
	return 0;
}

__weak void aee_powerkey_notify_press(unsigned long pressed)
{
}

__weak int aee_kernel_Powerkey_is_press(void)
{
	return 0;
}



#endif

#ifndef CONFIG_MTK_AEE_DRAM_CONSOLE
__weak void dram_console_init(struct proc_dir_entry *aed_proc_dir)
{
}

__weak void dram_console_done(struct proc_dir_entry *aed_proc_dir)
{
}
#endif

#ifndef CONFIG_MTK_AEE_IPANIC
__weak void ipanic_recursive_ke(struct pt_regs *regs, struct pt_regs *excp_regs, int cpu)
{
}

__weak struct aee_oops *ipanic_oops_copy(void)
{
	return NULL;
}

__weak void ipanic_oops_free(struct aee_oops *oops, int erase)
{
}

__weak void mrdump_mini_per_cpu_regs(int cpu, struct pt_regs *regs)
{
}
#endif
