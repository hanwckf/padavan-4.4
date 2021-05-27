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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>

#include "mtk_spm_internal.h"
#include "mtk_sleep.h"

/*
 * Macro and Inline
 */
#define DEFINE_ATTR_RO(_name)			\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = #_name,			\
		.mode = 0444,			\
	},					\
	.show	= _name##_show,			\
}

#define DEFINE_ATTR_RW(_name)			\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = #_name,			\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

#define __ATTR_OF(_name)	(&_name##_attr.attr)


/*
 * xxx_pcm_show Function
 */
static ssize_t show_pcm_desc(const struct pcm_desc *pcmdesc, char *buf)
{
	char *p = buf;

	p += sprintf(p, "version = %s\n", pcmdesc->version);
	p += sprintf(p, "base = 0x%p\n", pcmdesc->base);
	p += sprintf(p, "size = %u\n", pcmdesc->size);
	p += sprintf(p, "sess = %u\n", pcmdesc->sess);
	p += sprintf(p, "replace = %u\n", pcmdesc->replace);

	p += sprintf(p, "vec0 = 0x%x\n", pcmdesc->vec0);
	p += sprintf(p, "vec1 = 0x%x\n", pcmdesc->vec1);
	p += sprintf(p, "vec2 = 0x%x\n", pcmdesc->vec2);
	p += sprintf(p, "vec3 = 0x%x\n", pcmdesc->vec3);
	p += sprintf(p, "vec4 = 0x%x\n", pcmdesc->vec4);
	p += sprintf(p, "vec5 = 0x%x\n", pcmdesc->vec5);
	p += sprintf(p, "vec6 = 0x%x\n", pcmdesc->vec6);
	p += sprintf(p, "vec7 = 0x%x\n", pcmdesc->vec7);

	WARN_ON(p - buf >= PAGE_SIZE);
	return p - buf;
}

static ssize_t suspend_pcm_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return show_pcm_desc(__spm_suspend.pcmdesc, buf);
}

static ssize_t dpidle_pcm_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return show_pcm_desc(__spm_dpidle.pcmdesc, buf);
}

static ssize_t sodi_pcm_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t ddrdfs_pcm_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
/* For bring up */
#if 0
	return show_pcm_desc(__spm_ddrdfs.pcmdesc, buf);
#else
	return 0;
#endif
}

#if defined(SPM_VCORE_EN)
static ssize_t vcorefs_pcm_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return show_pcm_desc(__spm_vcore_dvfs.pcmdesc, buf);
}
#endif

/*
 * xxx_ctrl_show Function
 */
static ssize_t show_pwr_ctrl(const struct pwr_ctrl *pwrctrl, char *buf)
{
	char *p = buf;

	p += sprintf(p, "pcm_flags = 0x%x\n", pwrctrl->pcm_flags);
	p += sprintf(p, "pcm_flags_cust = 0x%x\n", pwrctrl->pcm_flags_cust);
	p += sprintf(p, "pcm_reserve = 0x%x\n", pwrctrl->pcm_reserve);
	p += sprintf(p, "timer_val = 0x%x\n", pwrctrl->timer_val);
	p += sprintf(p, "timer_val_cust = 0x%x\n", pwrctrl->timer_val_cust);
	p += sprintf(p, "wake_src = 0x%x\n", pwrctrl->wake_src);
	p += sprintf(p, "wake_src_cust = 0x%x\n", pwrctrl->wake_src_cust);
	p += sprintf(p, "r0_ctrl_en = %u\n", pwrctrl->r0_ctrl_en);
	p += sprintf(p, "r7_ctrl_en = %u\n", pwrctrl->r7_ctrl_en);
	p += sprintf(p, "infra_dcm_lock = %u\n", pwrctrl->infra_dcm_lock);
	p += sprintf(p, "pcm_apsrc_req = %u\n", pwrctrl->pcm_apsrc_req);
	p += sprintf(p, "pcm_f26m_req = %u\n", pwrctrl->pcm_f26m_req);

	p += sprintf(p, "mcusys_idle_mask = %u\n", pwrctrl->mcusys_idle_mask);
	p += sprintf(p, "ca15top_idle_mask = %u\n", pwrctrl->ca15top_idle_mask);
	p += sprintf(p, "ca7top_idle_mask = %u\n", pwrctrl->ca7top_idle_mask);
	p += sprintf(p, "wfi_op = %u\n", pwrctrl->wfi_op);
	p += sprintf(p, "ca15_wfi0_en = %u\n", pwrctrl->ca15_wfi0_en);
	p += sprintf(p, "ca15_wfi1_en = %u\n", pwrctrl->ca15_wfi1_en);
	p += sprintf(p, "ca15_wfi2_en = %u\n", pwrctrl->ca15_wfi2_en);
	p += sprintf(p, "ca15_wfi3_en = %u\n", pwrctrl->ca15_wfi3_en);
	p += sprintf(p, "ca7_wfi0_en = %u\n", pwrctrl->ca7_wfi0_en);
	p += sprintf(p, "ca7_wfi1_en = %u\n", pwrctrl->ca7_wfi1_en);
	p += sprintf(p, "ca7_wfi2_en = %u\n", pwrctrl->ca7_wfi2_en);
	p += sprintf(p, "ca7_wfi3_en = %u\n", pwrctrl->ca7_wfi3_en);

	p += sprintf(p, "gce_req_mask = %u\n", pwrctrl->gce_req_mask);

	p += sprintf(p, "conn_mask = %u\n", pwrctrl->conn_mask);

	p += sprintf(p, "disp0_req_mask = %u\n", pwrctrl->disp0_req_mask);
	p += sprintf(p, "disp1_req_mask = %u\n", pwrctrl->disp1_req_mask);
	p += sprintf(p, "mfg_req_mask = %u\n", pwrctrl->mfg_req_mask);
	p += sprintf(p, "vdec_req_mask = %u\n", pwrctrl->vdec_req_mask);
	p += sprintf(p, "mm_ddr_req_mask = %u\n", pwrctrl->mm_ddr_req_mask);

	p += sprintf(p, "syspwreq_mask = %u\n", pwrctrl->syspwreq_mask);
	p += sprintf(p, "srclkenai_mask = %u\n", pwrctrl->srclkenai_mask);

	p += sprintf(p, "param1 = 0x%x\n", pwrctrl->param1);
	p += sprintf(p, "param2 = 0x%x\n", pwrctrl->param2);
	p += sprintf(p, "param3 = 0x%x\n", pwrctrl->param3);

	WARN_ON(p - buf >= PAGE_SIZE);
	return p - buf;
}

static ssize_t suspend_ctrl_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return show_pwr_ctrl(__spm_suspend.pwrctrl, buf);
}

static ssize_t dpidle_ctrl_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return show_pwr_ctrl(__spm_dpidle.pwrctrl, buf);
}

static ssize_t sodi_ctrl_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t ddrdfs_ctrl_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return 0;
}

#if defined(SPM_VCORE_EN)
static ssize_t vcorefs_ctrl_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return show_pwr_ctrl(__spm_vcore_dvfs.pwrctrl, buf);
}
#endif

/*
 * xxx_ctrl_store Function
 */
static ssize_t store_pwr_ctrl(struct pwr_ctrl *pwrctrl, const char *buf,
								size_t count)
{
	u32 val;
	char cmd[32];

	if (sscanf(buf, "%31s %x", cmd, &val) != 2)
		return -EPERM;

	spm_debug("pwr_ctrl: cmd = %s, val = 0x%x\n", cmd, val);

	if (!strcmp(cmd, "pcm_flags"))
		pwrctrl->pcm_flags = val;
	else if (!strcmp(cmd, "pcm_flags_cust"))
		pwrctrl->pcm_flags_cust = val;
	else if (!strcmp(cmd, "pcm_reserve"))
		pwrctrl->pcm_reserve = val;
	else if (!strcmp(cmd, "timer_val"))
		pwrctrl->timer_val = val;
	else if (!strcmp(cmd, "timer_val_cust"))
		pwrctrl->timer_val_cust = val;
	else if (!strcmp(cmd, "wake_src"))
		pwrctrl->wake_src = val;
	else if (!strcmp(cmd, "wake_src_cust"))
		pwrctrl->wake_src_cust = val;
	else if (!strcmp(cmd, "r0_ctrl_en"))
		pwrctrl->r0_ctrl_en = val;
	else if (!strcmp(cmd, "r7_ctrl_en"))
		pwrctrl->r7_ctrl_en = val;
	else if (!strcmp(cmd, "infra_dcm_lock"))
		pwrctrl->infra_dcm_lock = val;
	else if (!strcmp(cmd, "pcm_apsrc_req"))
		pwrctrl->pcm_apsrc_req = val;
	else if (!strcmp(cmd, "pcm_f26m_req"))
		pwrctrl->pcm_f26m_req = val;

	else if (!strcmp(cmd, "mcusys_idle_mask"))
		pwrctrl->mcusys_idle_mask = val;
	else if (!strcmp(cmd, "ca15top_idle_mask"))
		pwrctrl->ca15top_idle_mask = val;
	else if (!strcmp(cmd, "ca7top_idle_mask"))
		pwrctrl->ca7top_idle_mask = val;
	else if (!strcmp(cmd, "wfi_op"))
		pwrctrl->wfi_op = val;
	else if (!strcmp(cmd, "ca15_wfi0_en"))
		pwrctrl->ca15_wfi0_en = val;
	else if (!strcmp(cmd, "ca15_wfi1_en"))
		pwrctrl->ca15_wfi1_en = val;
	else if (!strcmp(cmd, "ca15_wfi2_en"))
		pwrctrl->ca15_wfi2_en = val;
	else if (!strcmp(cmd, "ca15_wfi3_en"))
		pwrctrl->ca15_wfi3_en = val;
	else if (!strcmp(cmd, "ca7_wfi0_en"))
		pwrctrl->ca7_wfi0_en = val;
	else if (!strcmp(cmd, "ca7_wfi1_en"))
		pwrctrl->ca7_wfi1_en = val;
	else if (!strcmp(cmd, "ca7_wfi2_en"))
		pwrctrl->ca7_wfi2_en = val;
	else if (!strcmp(cmd, "ca7_wfi3_en"))
		pwrctrl->ca7_wfi3_en = val;

	else if (!strcmp(cmd, "gce_req_mask"))
		pwrctrl->gce_req_mask = val;

	else if (!strcmp(cmd, "conn_mask"))
		pwrctrl->conn_mask = val;

	else if (!strcmp(cmd, "disp0_req_mask"))
		pwrctrl->disp0_req_mask = val;
	else if (!strcmp(cmd, "disp1_req_mask"))
		pwrctrl->disp1_req_mask = val;
	else if (!strcmp(cmd, "mfg_req_mask"))
		pwrctrl->mfg_req_mask = val;
	else if (!strcmp(cmd, "vdec_req_mask"))
		pwrctrl->vdec_req_mask = val;
	else if (!strcmp(cmd, "mm_ddr_req_mask"))
		pwrctrl->mm_ddr_req_mask = val;

	else if (!strcmp(cmd, "syspwreq_mask"))
		pwrctrl->syspwreq_mask = val;
	else if (!strcmp(cmd, "srclkenai_mask"))
		pwrctrl->srclkenai_mask = val;

	else if (!strcmp(cmd, "param1"))
		pwrctrl->param1 = val;
	else if (!strcmp(cmd, "param2"))
		pwrctrl->param2 = val;
	else if (!strcmp(cmd, "param3"))
		pwrctrl->param3 = val;
	else
		return -EINVAL;

	return count;
}

static ssize_t suspend_ctrl_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	return store_pwr_ctrl(__spm_suspend.pwrctrl, buf, count);
}

static ssize_t dpidle_ctrl_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	return store_pwr_ctrl(__spm_dpidle.pwrctrl, buf, count);
}

static ssize_t sodi_ctrl_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	return 0;
}

static ssize_t ddrdfs_ctrl_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	return 0;
}

#if defined(SPM_VCORE_EN)
static ssize_t vcorefs_ctrl_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	return store_pwr_ctrl(__spm_vcore_dvfs.pwrctrl, buf, count);
}
#endif

/*
 * golden_dump_xxx Function
 */
static ssize_t golden_dump_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	char *p = buf;

	WARN_ON(p - buf >= PAGE_SIZE);
	return p - buf;
}

/*
 * Init Function
 */
DEFINE_ATTR_RO(suspend_pcm);
DEFINE_ATTR_RO(dpidle_pcm);
DEFINE_ATTR_RO(sodi_pcm);
DEFINE_ATTR_RO(ddrdfs_pcm);
#if defined(SPM_VCORE_EN)
DEFINE_ATTR_RO(vcorefs_pcm);
#endif

DEFINE_ATTR_RW(suspend_ctrl);
DEFINE_ATTR_RW(dpidle_ctrl);
DEFINE_ATTR_RW(sodi_ctrl);
DEFINE_ATTR_RW(ddrdfs_ctrl);
#if defined(SPM_VCORE_EN)
DEFINE_ATTR_RW(vcorefs_ctrl);
#endif

DEFINE_ATTR_RO(golden_dump);

static struct attribute *spm_attrs[] = {
	/* for spm_lp_scen.pcmdesc */
	__ATTR_OF(suspend_pcm),
	__ATTR_OF(dpidle_pcm),
	__ATTR_OF(sodi_pcm),
	__ATTR_OF(ddrdfs_pcm),
#if defined(SPM_VCORE_EN)
	__ATTR_OF(vcorefs_pcm),
#endif

	/* for spm_lp_scen.pwrctrl */
	__ATTR_OF(suspend_ctrl),
	__ATTR_OF(dpidle_ctrl),
	__ATTR_OF(sodi_ctrl),
	__ATTR_OF(ddrdfs_ctrl),
#if defined(SPM_VCORE_EN)
	__ATTR_OF(vcorefs_ctrl),
#endif

	/* other debug interface */
	__ATTR_OF(golden_dump),

	/* must */
	NULL,
};

static struct attribute_group spm_attr_group = {
	.name = "spm",
	.attrs = spm_attrs,
};

int spm_fs_init(void)
{
	int r;

	/* create /sys/power/spm/xxx */
	r = sysfs_create_group(power_kobj, &spm_attr_group);
	if (r)
		spm_err("FAILED TO CREATE /sys/power/spm (%d)\n", r);

	return r;
}

MODULE_DESCRIPTION("SPM-FS Driver v0.1");
