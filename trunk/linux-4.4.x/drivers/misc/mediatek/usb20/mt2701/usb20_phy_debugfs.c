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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include "musb_core.h"
#include "musb_debug.h"
#include "mtk_musb.h"
#define MYDBG(fmt, args...) pr_warn("MTK_ICUSB [DBG], <%s(), %d> " fmt, __func__, __LINE__, ## args)

/* general */
#define BIT_WIDTH_1		1
#define MSK_WIDTH_1		0x1
#define VAL_MAX_WDITH_1	0x1
#define VAL_0_WIDTH_1		0x0
#define VAL_1_WIDTH_1		0x1
#define STRNG_0_WIDTH_1	"0"
#define STRNG_1_WIDTH_1	"1"

#define BIT_WIDTH_3		3
#define MSK_WIDTH_3		0x7
#define VAL_MAX_WDITH_3		0x7
#define VAL_0_WIDTH_3		0x0
#define VAL_1_WIDTH_3		0x1
#define VAL_2_WIDTH_3		0x2
#define VAL_3_WIDTH_3		0x3
#define VAL_4_WIDTH_3		0x4
#define VAL_5_WIDTH_3		0x5
#define VAL_6_WIDTH_3		0x6
#define VAL_7_WIDTH_3		0x7
#define STRNG_0_WIDTH_3	"000"
#define STRNG_1_WIDTH_3	"001"
#define STRNG_2_WIDTH_3	"010"
#define STRNG_3_WIDTH_3	"011"
#define STRNG_4_WIDTH_3	"100"
#define STRNG_5_WIDTH_3	"101"
#define STRNG_6_WIDTH_3	"110"
#define STRNG_7_WIDTH_3	"111"

/* specific */
#define FILE_RG_USB20_TERM_VREF_SEL "RG_USB20_TERM_VREF_SEL"
#define MSK_RG_USB20_TERM_VREF_SEL MSK_WIDTH_3
#define SHFT_RG_USB20_TERM_VREF_SEL 0
#define OFFSET_RG_USB20_TERM_VREF_SEL 0x5

#define FILE_RG_USB20_HSTX_SRCTRL "RG_USB20_HSTX_SRCTRL"
#define MSK_RG_USB20_HSTX_SRCTRL MSK_WIDTH_3
#define SHFT_RG_USB20_HSTX_SRCTRL 4
#define OFFSET_RG_USB20_HSTX_SRCTRL 0x15

#define FILE_RG_USB20_VRT_VREF_SEL "RG_USB20_VRT_VREF_SEL"
#define MSK_RG_USB20_VRT_VREF_SEL MSK_WIDTH_3
#define SHFT_RG_USB20_VRT_VREF_SEL 4
#define OFFSET_RG_USB20_VRT_VREF_SEL 0x5

#define FILE_RG_USB20_INTR_EN "RG_USB20_INTR_EN"
#define MSK_RG_USB20_INTR_EN MSK_WIDTH_1
#define SHFT_RG_USB20_INTR_EN 5
#define OFFSET_RG_USB20_INTR_EN 0x0

static struct dentry *usb20_phy_debugfs_root;

void usb20_phy_debugfs_write_width1(u8 offset, u8 shift, char *buf)
{
	u8 clr_val = 0, set_val = 0;

	MYDBG("s(%s)\n", buf);
	if (!strncmp(buf, STRNG_0_WIDTH_1, BIT_WIDTH_1)) {
		MYDBG("%s case\n", STRNG_0_WIDTH_1);
		clr_val = VAL_1_WIDTH_1;
	}
	if (!strncmp(buf, STRNG_1_WIDTH_1, BIT_WIDTH_1)) {
		MYDBG("%s case\n", STRNG_1_WIDTH_1);
		set_val = VAL_1_WIDTH_1;
	}

	if (clr_val || set_val) {
		clr_val = VAL_MAX_WDITH_1 - set_val;
		MYDBG("offset:%x, clr_val:%x, set_val:%x, before shft\n", offset, clr_val, set_val);
		clr_val <<= shift;
		set_val <<= shift;
		MYDBG("offset:%x, clr_val:%x, set_val:%x, after shft\n", offset, clr_val, set_val);
		USBPHY_CLR8(offset, clr_val);
		USBPHY_SET8(offset, set_val);
	} else {
		MYDBG("do nothing\n");
	}
}

void usb20_phy_debugfs_write_width3(u8 offset, u8 shift, char *buf)
{
	u8 clr_val = 0, set_val = 0;

	MYDBG("s(%s)\n", buf);
	if (!strncmp(buf, STRNG_0_WIDTH_3, BIT_WIDTH_3)) {
		MYDBG("%s case\n", STRNG_0_WIDTH_3);
		clr_val = VAL_7_WIDTH_3;
	}
	if (!strncmp(buf, STRNG_1_WIDTH_3, BIT_WIDTH_3)) {
		MYDBG("%s case\n", STRNG_1_WIDTH_3);
		set_val = VAL_1_WIDTH_3;
	}
	if (!strncmp(buf, STRNG_2_WIDTH_3, BIT_WIDTH_3)) {
		MYDBG("%s case\n", STRNG_2_WIDTH_3);
		set_val = VAL_2_WIDTH_3;
	}
	if (!strncmp(buf, STRNG_3_WIDTH_3, BIT_WIDTH_3)) {
		MYDBG("%s case\n", STRNG_3_WIDTH_3);
		set_val = VAL_3_WIDTH_3;
	}
	if (!strncmp(buf, STRNG_4_WIDTH_3, BIT_WIDTH_3)) {
		MYDBG("%s case\n", STRNG_4_WIDTH_3);
		set_val = VAL_4_WIDTH_3;
	}
	if (!strncmp(buf, STRNG_5_WIDTH_3, BIT_WIDTH_3)) {
		MYDBG("%s case\n", STRNG_5_WIDTH_3);
		set_val = VAL_5_WIDTH_3;
	}
	if (!strncmp(buf, STRNG_6_WIDTH_3, BIT_WIDTH_3)) {
		MYDBG("%s case\n", STRNG_6_WIDTH_3);
		set_val = VAL_6_WIDTH_3;
	}
	if (!strncmp(buf, STRNG_7_WIDTH_3, BIT_WIDTH_3)) {
		MYDBG("%s case\n", STRNG_7_WIDTH_3);
		set_val = VAL_7_WIDTH_3;
	}

	if (clr_val || set_val) {
		clr_val = VAL_MAX_WDITH_3 - set_val;
		MYDBG("offset:%x, clr_val:%x, set_val:%x, before shft\n", offset, clr_val, set_val);
		clr_val <<= shift;
		set_val <<= shift;
		MYDBG("offset:%x, clr_val:%x, set_val:%x, after shft\n", offset, clr_val, set_val);
		USBPHY_CLR8(offset, clr_val);
		USBPHY_SET8(offset, set_val);
	} else {
		MYDBG("do nothing\n");
	}
}

u8 usb20_phy_debugfs_read_val(u8 offset, u8 shft, u8 msk, u8 width, char *str)
{
	u8 val;
	int i, temp;

	val = USBPHY_READ8(offset);
	MYDBG("offset:%x, val:%x, shft:%x, msk:%x\n", offset, val, shft, msk);
	val = val >> shft;
	MYDBG("offset:%x, val:%x, shft:%x, msk:%x\n", offset, val, shft, msk);
	val = val & msk;
	MYDBG("offset:%x, val:%x, shft:%x, msk:%x\n", offset, val, shft, msk);

	temp = val;
	str[width] = '\0';
	for (i = (width - 1); i >= 0; i--) {
		if (val % 2)
			str[i] = '1';
		else
			str[i] = '0';

		MYDBG("str[%d]:%c\n", i, str[i]);
		val /= 2;
	}

	MYDBG("str(%s)\n", str);


	return val;
}

static int rg_usb20_term_vref_sel_show(struct seq_file *s, void *unused)
{
	u8 val;
	char str[16];

	val =
	    usb20_phy_debugfs_read_val(OFFSET_RG_USB20_TERM_VREF_SEL, SHFT_RG_USB20_TERM_VREF_SEL,
				       MSK_RG_USB20_TERM_VREF_SEL, BIT_WIDTH_3, str);
	seq_printf(s, "%s", str);
	return 0;
}

static int rg_usb20_hstx_srctrl_show(struct seq_file *s, void *unused)
{
	u8 val;
	char str[16];

	val =
	    usb20_phy_debugfs_read_val(OFFSET_RG_USB20_HSTX_SRCTRL, SHFT_RG_USB20_HSTX_SRCTRL,
				       MSK_RG_USB20_HSTX_SRCTRL, BIT_WIDTH_3, str);
	seq_printf(s, "%s", str);
	return 0;
}

static int rg_usb20_vrt_vref_sel_show(struct seq_file *s, void *unused)
{
	u8 val;
	char str[16];

	val =
	    usb20_phy_debugfs_read_val(OFFSET_RG_USB20_VRT_VREF_SEL, SHFT_RG_USB20_VRT_VREF_SEL,
				       MSK_RG_USB20_VRT_VREF_SEL, BIT_WIDTH_3, str);
	seq_printf(s, "%s", str);
	return 0;
}

static int rg_usb20_intr_en_show(struct seq_file *s, void *unused)
{
	u8 val;
	char str[16];

	val =
	    usb20_phy_debugfs_read_val(OFFSET_RG_USB20_INTR_EN, SHFT_RG_USB20_INTR_EN,
				       MSK_RG_USB20_INTR_EN, BIT_WIDTH_1, str);
	seq_printf(s, "%s", str);
	return 0;
}

static int rg_usb20_term_vref_sel_open(struct inode *inode, struct file *file)
{
	return single_open(file, rg_usb20_term_vref_sel_show, inode->i_private);
}

static int rg_usb20_hstx_srctrl_open(struct inode *inode, struct file *file)
{
	return single_open(file, rg_usb20_hstx_srctrl_show, inode->i_private);
}

static int rg_usb20_vrt_vref_sel_open(struct inode *inode, struct file *file)
{
	return single_open(file, rg_usb20_vrt_vref_sel_show, inode->i_private);
}

static int rg_usb20_intr_en_open(struct inode *inode, struct file *file)
{
	return single_open(file, rg_usb20_intr_en_show, inode->i_private);
}

static ssize_t rg_usb20_term_vref_sel_write(struct file *file,
					    const char __user *ubuf, size_t count, loff_t *ppos)
{
	char buf[18];

	memset(buf, 0x00, sizeof(buf));

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;
	usb20_phy_debugfs_write_width3(OFFSET_RG_USB20_TERM_VREF_SEL, SHFT_RG_USB20_TERM_VREF_SEL,
				       buf);
	return count;
}

static ssize_t rg_usb20_hstx_srctrl_write(struct file *file,
					  const char __user *ubuf, size_t count, loff_t *ppos)
{
	char buf[18];

	memset(buf, 0x00, sizeof(buf));

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;
	usb20_phy_debugfs_write_width3(OFFSET_RG_USB20_HSTX_SRCTRL, SHFT_RG_USB20_HSTX_SRCTRL, buf);
	return count;
}

static ssize_t rg_usb20_vrt_vref_sel_write(struct file *file,
					   const char __user *ubuf, size_t count, loff_t *ppos)
{
	char buf[18];

	memset(buf, 0x00, sizeof(buf));

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;
	usb20_phy_debugfs_write_width3(OFFSET_RG_USB20_VRT_VREF_SEL, SHFT_RG_USB20_VRT_VREF_SEL,
				       buf);
	return count;
}

static ssize_t rg_usb20_intr_en_write(struct file *file,
				      const char __user *ubuf, size_t count, loff_t *ppos)
{
	char buf[18];

	memset(buf, 0x00, sizeof(buf));

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;
	usb20_phy_debugfs_write_width1(OFFSET_RG_USB20_INTR_EN, SHFT_RG_USB20_INTR_EN, buf);
	return count;
}

static const struct file_operations rg_usb20_term_vref_sel_fops = {
	.open = rg_usb20_term_vref_sel_open,
	.write = rg_usb20_term_vref_sel_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations rg_usb20_hstx_srctrl_fops = {
	.open = rg_usb20_hstx_srctrl_open,
	.write = rg_usb20_hstx_srctrl_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations rg_usb20_vrt_vref_sel_fops = {
	.open = rg_usb20_vrt_vref_sel_open,
	.write = rg_usb20_vrt_vref_sel_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations rg_usb20_intr_en_fops = {
	.open = rg_usb20_intr_en_open,
	.write = rg_usb20_intr_en_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int usb20_phy_init_debugfs(void)
{
	struct dentry *root;
	struct dentry *file;
	int ret;

	root = debugfs_create_dir("usb20_phy", NULL);
	if (!root) {
		ret = -ENOMEM;
		goto err0;
	}

	file = debugfs_create_file(FILE_RG_USB20_TERM_VREF_SEL, S_IRUGO | S_IWUSR,
				   root, NULL, &rg_usb20_term_vref_sel_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}
	file = debugfs_create_file(FILE_RG_USB20_HSTX_SRCTRL, S_IRUGO | S_IWUSR,
				   root, NULL, &rg_usb20_hstx_srctrl_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}
	file = debugfs_create_file(FILE_RG_USB20_VRT_VREF_SEL, S_IRUGO | S_IWUSR,
				   root, NULL, &rg_usb20_vrt_vref_sel_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}
	file = debugfs_create_file(FILE_RG_USB20_INTR_EN, S_IRUGO | S_IWUSR,
				   root, NULL, &rg_usb20_intr_en_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	usb20_phy_debugfs_root = root;
	return 0;

err1:
	debugfs_remove_recursive(root);

err0:
	return ret;
}

void /* __init_or_exit */ usb20_phy_exit_debugfs(struct musb *musb)
{
	debugfs_remove_recursive(usb20_phy_debugfs_root);
}
