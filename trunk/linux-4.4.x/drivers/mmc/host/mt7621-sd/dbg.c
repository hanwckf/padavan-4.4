/*
 * Copyright (c) 2014-2015 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include "dbg.h"
#include "mt7621_sd.h"
#include <linux/seq_file.h>


/* for debug zone */
unsigned int sd_debug_zone[4] = {
	0,
	0,
	0,
	0
};

#if defined(MT6575_SD_DEBUG)
static char cmd_buf[256];
/* for driver profile */
#define TICKS_ONE_MS  (13000)
u32 gpt_enable;
u32 sdio_pro_enable;   /* make sure gpt is enabled */
u32 sdio_pro_time;     /* no more than 30s */
struct sdio_profile sdio_perfomance = {0};

u32 msdc_time_calc(u32 old_L32, u32 old_H32, u32 new_L32, u32 new_H32)
{
	u32 ret = 0;

	if (new_H32 == old_H32) {
		ret = new_L32 - old_L32;
	} else if (new_H32 == (old_H32 + 1)) {
		if (new_L32 > old_L32)
			pr_debug("msdc old_L<0x%x> new_L<0x%x>\n",
				 old_L32, new_L32);
		ret = (0xffffffff - old_L32);
		ret += new_L32;
	} else {
		pr_debug("msdc old_H<0x%x> new_H<0x%x>\n", old_H32, new_H32);
	}

	return ret;
}

void msdc_sdio_profile(struct sdio_profile *result)
{
	struct cmd_profile *cmd;
	u32 i;

	pr_debug("sdio === performance dump ===\n");
	pr_debug("sdio === total execute tick<%d> time<%dms> Tx<%dB> Rx<%dB>\n",
		 result->total_tc, result->total_tc / TICKS_ONE_MS,
		 result->total_tx_bytes, result->total_rx_bytes);

	/* CMD52 Dump */
	cmd = &result->cmd52_rx;
	cmd = &result->cmd52_tx;

	/* CMD53 Rx bytes + block mode */
	for (i = 0; i < 512; i++)
		cmd = &result->cmd53_rx_byte[i];

	for (i = 0; i < 100; i++)
		cmd = &result->cmd53_rx_blk[i];

	/* CMD53 Tx bytes + block mode */
	for (i = 0; i < 512; i++)
		cmd = &result->cmd53_tx_byte[i];

	for (i = 0; i < 100; i++)
		cmd = &result->cmd53_tx_blk[i];

	pr_debug("sdio === performance dump done ===\n");
}

void msdc_performance(u32 opcode, u32 sizes, u32 bRx, u32 ticks)
{
	struct sdio_profile *result = &sdio_perfomance;
	struct cmd_profile *cmd;
	u32 block;

	if (sdio_pro_enable == 0)
		return;

	if (opcode == 52) {
		cmd = bRx ?  &result->cmd52_rx : &result->cmd52_tx;
	} else if (opcode == 53) {
		if (sizes < 512) {
			cmd = bRx ? &result->cmd53_rx_byte[sizes] :
				    &result->cmd53_tx_byte[sizes];
		} else {
			block = sizes / 512;
			if (block >= 99) {
				pr_info("cmd53 error blocks\n");
				while (1)
					;
			}
			cmd = bRx ? &result->cmd53_rx_blk[block] :
				    &result->cmd53_tx_blk[block];
		}
	} else {
		return;
	}

	/* update the members */
	if (ticks > cmd->max_tc)
		cmd->max_tc = ticks;
	if (cmd->min_tc == 0 || ticks < cmd->min_tc)
		cmd->min_tc = ticks;
	cmd->tot_tc += ticks;
	cmd->tot_bytes += sizes;
	cmd->count++;

	if (bRx)
		result->total_rx_bytes += sizes;
	else
		result->total_tx_bytes += sizes;
	result->total_tc += ticks;

	/* dump when total_tc > 30s */
	if (result->total_tc >= sdio_pro_time * TICKS_ONE_MS * 1000) {
		msdc_sdio_profile(result);
		memset(result, 0, sizeof(struct sdio_profile));
	}
}

static int msdc_debug_proc_read(struct seq_file *s, void *p)
{
	seq_puts(s, "\n=========================================\n");
	seq_puts(s, "Index<0> + Id + Zone\n");
	seq_puts(s, "-> PWR<9> WRN<8> | FIO<7> OPS<6> FUN<5> CFG<4> | ");
	seq_puts(s, "INT<3> RSP<2> CMD<1> DMA<0>\n");
	seq_puts(s, "-> echo 0 3 0x3ff >msdc_bebug -> host[3] debug zone set ");
	seq_puts(s, "to 0x3ff\n");
	seq_printf(s, "-> MSDC[0] Zone: 0x%.8x\n", sd_debug_zone[0]);
	seq_printf(s, "-> MSDC[1] Zone: 0x%.8x\n", sd_debug_zone[1]);
	seq_printf(s, "-> MSDC[2] Zone: 0x%.8x\n", sd_debug_zone[2]);
	seq_printf(s, "-> MSDC[3] Zone: 0x%.8x\n", sd_debug_zone[3]);

	seq_puts(s, "Index<3> + SDIO_PROFILE + TIME\n");
	seq_puts(s, "-> echo 3 1 0x1E >msdc_bebug -> enable sdio_profile, ");
	seq_puts(s, "30s\n");
	seq_printf(s, "-> SDIO_PROFILE<%d> TIME<%ds>\n",
		   sdio_pro_enable, sdio_pro_time);
	seq_puts(s, "=========================================\n\n");

	return 0;
}

static ssize_t msdc_debug_proc_write(struct file *file,
				     const char __user *buf,
				     size_t count, loff_t *data)
{
	int ret;

	int cmd, p1, p2;
	int id, zone;
	int mode, size;

	if (count == 0)
		return -1;
	if (count > 255)
		count = 255;

	if (copy_from_user(cmd_buf, buf, count))
		return -EFAULT;

	cmd_buf[count] = '\0';
	pr_debug("msdc Write %s\n", cmd_buf);

	ret = sscanf(cmd_buf, "%x %x %x", &cmd, &p1, &p2);
	if (ret != 3)
		return -EINVAL;

	if (cmd == SD_TOOL_ZONE) {
		id = p1;
		zone = p2;
		zone &= 0x3ff;
		pr_debug("msdc host_id<%d> zone<0x%.8x>\n", id, zone);
		if (id >= 0 && id <= 3) {
			sd_debug_zone[id] = zone;
		} else if (id == 4) {
			sd_debug_zone[0] = sd_debug_zone[1] = zone;
			sd_debug_zone[2] = sd_debug_zone[3] = zone;
		} else {
			pr_info("msdc host_id error when set debug zone\n");
		}
	} else if (cmd == SD_TOOL_SDIO_PROFILE) {
		if (p1 == 1) { /* enable profile */
			if (gpt_enable == 0)
				gpt_enable = 1;
			sdio_pro_enable = 1;
			if (p2 == 0)
				p2 = 1;
			if (p2 >= 30)
				p2 = 30;
			sdio_pro_time = p2;
		} else if (p1 == 0) {
			/* todo */
			sdio_pro_enable = 0;
		}
	}

	return count;
}

static int msdc_debug_show(struct inode *inode, struct file *file)
{
	return single_open(file, msdc_debug_proc_read, NULL);
}

static const struct file_operations msdc_debug_fops = {
	.owner		= THIS_MODULE,
	.open		= msdc_debug_show,
	.read		= seq_read,
	.write		= msdc_debug_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void msdc_debug_proc_init(void)
{
	proc_create("msdc_debug", 0660, NULL, &msdc_debug_fops);
}
EXPORT_SYMBOL_GPL(msdc_debug_proc_init);
#endif
