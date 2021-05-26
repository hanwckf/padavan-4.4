/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2017 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
#include "frame_engine.h"
#include "foe_fdb.h"
#include "hwnat_ioctl.h"
#include "util.h"
#include "api.h"
#include "hwnat_config.h"
#include "hwnat_define.h"
#include "hnat_dbg_proc.h"
#include "mcast_tbl.h"
#include <linux/seq_file.h>
#include <net/ra_nat.h>

struct proc_dir_entry *hnat_proc_reg_dir;
static struct proc_dir_entry *proc_cpu_reason;
static struct proc_dir_entry *proc_hnat_entry;
static struct proc_dir_entry *proc_hnat_setting;
static struct proc_dir_entry *proc_hnat_multicast;

int dbg_cpu_reason;
EXPORT_SYMBOL(dbg_cpu_reason);

int dbg_entry_state = BIND;
typedef int (*CPU_REASON_SET_FUNC) (int par1, int par2, int par3);
typedef int (*ENTRY_SET_FUNC) (int par1, int par2, int par3);
typedef int (*CR_SET_FUNC) (int par1, int par2, int par3);
typedef int (*MULTICAST_SET_FUNC) (int par1, int par2, int par3);

int hnat_set_usage(int level, int ignore2, int ignore3)
{
	pr_info("Choose CPU reason 'x'");
	pr_info(" we can see which entry indx has cpu reason 'x'\n");
	pr_info("echo \"1 [cpu_reason]\" > /proc/%s\n",
		PROCREG_CPU_REASON);

	pr_info("(2)IPv4(IPv6) TTL(hop limit) = 0\n");
	pr_info("(3)IPv4(IPv6) has option(extension) header\n");
	pr_info("(7)No flow is assigned\n");
	pr_info("(8)IPv4 HNAT doesn't support IPv4 /w fragment\n");
	pr_info("(9)IPv4 HNAPT/DS-Lite doesn't support IPv4 /w fragment\n");
	pr_info("(10)IPv4 HNAPT/DS-Lite can't find TCP/UDP sport/dport\n");
	pr_info("(11)IPv6 5T-route/6RD can't find TCP/UDP sport/dport\n");
	pr_info("(12) Ingress packet is TCP fin/syn/rst\n");
	pr_info("(13) FOE Un-hit\n");
	pr_info("(14) FOE Hit unbind\n");
	pr_info("(15) FOE Hit unbind & rate reach\n");
	pr_info("(16) Hit bind PPE TCP FIN entry\n");
	pr_info("(17) Hit bind PPE entry and TTL(hop limit) = 1\n");
	pr_info("(18) Hit bind and VLAN replacement violation\n");
	pr_info("(19) Hit bind and keep alive with unicast old-header packet\n");
	pr_info("(20) Hit bind and keep alive with multicast new-header packet\n");
	pr_info("(21) Hit bind and keep alive with duplicate old-header packet\n");
	pr_info("(22) FOE Hit bind & force to CPU\n");
	/* Hit bind and remove tunnel IP header, */
	/* but inner IP has option/next header */
	pr_info("(23) HIT_BIND_WITH_OPTION_HEADER\n");
	pr_info("(28) Hit bind and exceed MTU\n");
	pr_info("(27) HIT_BIND_PACKET_SAMPLING\n");
	pr_info("(24) Switch clone multicast packet to CPU\n");
	pr_info("(25) Switch clone multicast packet to GMAC1 & CPU\n");
	pr_info("(26) HIT_PRE_BIND\n");
	debug_level = level;
	pr_info("debug_level = %d\n", debug_level);
	return 0;
}

int entry_set_usage(int level, int ignore2, int ignore3)
{
	pr_info("<Usage> Get all bind entry  : cat /proc/%s/%s\n",
		HNAT_PROCREG_DIR, PROCREG_PPE_ENTRY);

	pr_info("<Usage> set entry state  : echo 1 [STATE] > /proc/%s/%s\n",
		HNAT_PROCREG_DIR, PROCREG_PPE_ENTRY);
	pr_info("<Usage> get entry detail  : echo 2 [index] > /proc/%s/%s\n",
		HNAT_PROCREG_DIR, PROCREG_PPE_ENTRY);
	pr_info("<Usage> delete entry  : echo 3 [index] > /proc/%s/%s\n",
		HNAT_PROCREG_DIR, PROCREG_PPE_ENTRY);
	pr_info("STATE: NVALID = 0, UNBIND = 1, BIND = 2, FIN = 3\n");
	pr_info("<Usage> set debug level : echo 0 [level] >  /proc/%s/%s\n",
		HNAT_PROCREG_DIR, PROCREG_PPE_ENTRY);
	debug_level = level;
	pr_info("debug_level = %d\n", debug_level);

	return 0;
}

int cr_set_usage(int level, int ignore2, int ignore3)
{
	pr_info("<Usage> get hnat CR  : cat /proc/%s/%s\n",
		HNAT_PROCREG_DIR, PROCREG_PPE_SETTING);
	pr_info("<Usage> set binding threshold : echo 1 [threshold] >  /proc/%s/%s\n",
		HNAT_PROCREG_DIR, PROCREG_PPE_SETTING);
	pr_info("<Usage> set bind lifetime");
	pr_info(" :echo 2 [tcp_life] [udp_life] [fin_life] >  /proc/%s/%s\n",
		HNAT_PROCREG_DIR, PROCREG_PPE_SETTING);
	pr_info("<Usage> set keep alive interval");
	pr_info(": echo 3 [tcp_interval] [udp_interval]  >  /proc/%s/%s\n",
		HNAT_PROCREG_DIR, PROCREG_PPE_SETTING);
	pr_info("<Usage> set debug level : echo 0 [level] >  /proc/%s/%s\n",
		HNAT_PROCREG_DIR, PROCREG_PPE_SETTING);

	debug_level = level;
	pr_info("debug_level = %d\n", debug_level);

	return 0;
}

int multicast_set_usage(int level, int ignore2, int ignore3)
{
	pr_info("<Usage> get hnat multicast table  : cat /proc/%s/%s\n",
		HNAT_PROCREG_DIR, PROCREG_PPE_MULTICAST);

	pr_info("<Usage> set debug level : echo 0 [level] >  /proc/%s/%s\n",
		HNAT_PROCREG_DIR, PROCREG_PPE_SETTING);

	debug_level = level;
	pr_info("debug_level = %d\n", debug_level);

	return 0;
}

int binding_threshold(int threshold, int ignore1, int ignore2)
{
	pr_info("Binding Threshold =%d\n", threshold);
	reg_write(PPE_FOE_BNDR, threshold);
	return 0;
}

int bind_life_time(int tcp_life, int udp_life, int fin_life)
{
	pr_info("tcp_life = %d, udp_life = %d, fin_life = %d\n",
		tcp_life, udp_life, fin_life);
	ppe_set_bind_lifetime(tcp_life, udp_life, fin_life);
	return 0;
}

int keep_alive_interval(int tcp_interval, int udp_interval, int ignore2)
{
	if (tcp_interval > 255 || udp_interval > 255) {
		tcp_interval = 255;
		udp_interval = 255;
		pr_info("TCP/UDP keep alive max interval = 255\n");
	} else {
		pr_info("tcp_interval = %d, udp_interval = %d\n",
			tcp_interval, udp_interval);
	}

	ppe_set_ka_interval(tcp_interval, udp_interval);
	return 0;
}

int hnat_cpu_reason(int cpu_reason, int ignore1, int ignore2)
{
	dbg_cpu_reason = cpu_reason;
	pr_info("show cpu reason = %d entry index = %d\n",
		cpu_reason, hwnat_dbg_entry);
	/* foe_dump_entry(hwnat_dbg_entry); */

	return 0;
}

int entry_set_state(int state, int ignore1, int ignore2)
{
	dbg_entry_state = state;
	pr_info("ENTRY STATE = %s\n",
		dbg_entry_state ==
		0 ? "Invalid" : dbg_entry_state ==
		1 ? "Unbind" : dbg_entry_state ==
		2 ? "BIND" : dbg_entry_state ==   3 ?
		"FIN" : "Unknown");
	return 0;
}

int entry_detail(int index, int ignore1, int ignore2)
{
	foe_dump_entry(index);
	return 0;
}

int entry_delete(int index, int ignore1, int ignore2)
{
	pr_info("delete entry idx = %d\n", index);
	foe_del_entry_by_num(index);
	return 0;
}

static const CPU_REASON_SET_FUNC hnat_set_func[] = {
	[0] = hnat_set_usage,
	[1] = hnat_cpu_reason,
};

static const ENTRY_SET_FUNC entry_set_func[] = {
	[0] = entry_set_usage,
	[1] = entry_set_state,
	[2] = entry_detail,
	[3] = entry_delete,
};

static const CR_SET_FUNC cr_set_func[] = {
	[0] = cr_set_usage,
	[1] = binding_threshold,
	[2] = bind_life_time,
	[3] = keep_alive_interval,
};

static const MULTICAST_SET_FUNC multicast_set_func[] = {
	[0] = multicast_set_usage,
};

ssize_t cpu_reason_write(struct file *file, const char __user *buffer,
			 size_t count, loff_t *data)
{
	char buf[32];
	char *p_buf;
	int len = count;
	long arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0;
	char *p_token = NULL;
	char *p_delimiter = " \t";
	int ret;

	if (len >= sizeof(buf)) {
		pr_info("input handling fail!\n");
		len = sizeof(buf) - 1;
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	pr_info("write parameter data = %s\n", buf);

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);
	if (!p_token)
		arg0 = 0;
	else
		ret = kstrtol(p_token, 10, &arg0);

	switch (arg0) {
	case 0:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		break;
	case 1:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		break;
	case 2:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg2 = 0;
		else
			ret = kstrtol(p_token, 10, &arg2);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg3 = 0;
		else
			ret = kstrtol(p_token, 10, &arg3);
		break;
	}

	if (hnat_set_func[arg0] &&
	    (ARRAY_SIZE(hnat_set_func) > arg0)) {
		(*hnat_set_func[arg0]) (arg1, arg2, arg3);
	} else {
		pr_info("no handler defined for command id(0x%08lx)\n\r", arg0);
		(*hnat_set_func[0]) (0, 0, 0);
	}

	return len;
}

ssize_t entry_write(struct file *file, const char __user *buffer,
		    size_t count, loff_t *data)
{
	char buf[32];
	char *p_buf;
	int len = count;
	long arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0;
	char *p_token = NULL;
	char *p_delimiter = " \t";
	int ret;

	if (len >= sizeof(buf)) {
		pr_info("input handling fail!\n");
		len = sizeof(buf) - 1;
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	pr_info("write parameter data = %s\n\r", buf);

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);
	if (!p_token)
		arg0 = 0;
	else
		ret = kstrtol(p_token, 10, &arg0);

	switch (arg0) {
	case 0:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		break;
	case 1:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		break;
	case 2:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg2 = 0;
		else
			ret = kstrtol(p_token, 10, &arg2);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg3 = 0;
		else
			ret = kstrtol(p_token, 10, &arg3);
		break;
	case 3:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg2 = 0;
		else
			ret = kstrtol(p_token, 10, &arg2);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg3 = 0;
		else
			ret = kstrtol(p_token, 10, &arg3);
		break;
	}

	if (entry_set_func[arg0] &&
	    (ARRAY_SIZE(entry_set_func) > arg0)) {
		(*entry_set_func[arg0]) (arg1, arg2, arg3);
	} else {
		pr_info("no handler defined for command id(0x%08lx)\n\r", arg0);
		(*entry_set_func[0]) (0, 0, 0);
	}

	return len;
}

ssize_t setting_write(struct file *file, const char __user *buffer,
		      size_t count, loff_t *data)
{
	char buf[32];
	char *p_buf;
	int len = count;
	long arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0;
	char *p_token = NULL;
	char *p_delimiter = " \t";
	int ret;

	if (len >= sizeof(buf)) {
		pr_info("input handling fail!\n");
		len = sizeof(buf) - 1;
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	pr_info("write parameter data = %s\n\r", buf);

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);
	if (!p_token)
		arg0 = 0;
	else
		ret = kstrtol(p_token, 10, &arg0);

	switch (arg0) {
	case 0:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		break;
	case 1:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		break;
	case 2:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg2 = 0;
		else
			ret = kstrtol(p_token, 10, &arg2);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg3 = 0;
		else
			ret = kstrtol(p_token, 10, &arg3);
		break;
	case 3:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg2 = 0;
		else
			ret = kstrtol(p_token, 10, &arg2);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg3 = 0;
		else
			ret = kstrtol(p_token, 10, &arg3);
		break;
	}

	if (cr_set_func[arg0] &&
	    (ARRAY_SIZE(cr_set_func) > arg0)) {
		(*cr_set_func[arg0]) (arg1, arg2, arg3);
	} else {
		pr_info("no handler defined for command id(0x%08lx)\n\r", arg0);
		(*cr_set_func[0]) (0, 0, 0);
	}

	return len;
}

ssize_t multicast_write(struct file *file, const char __user *buffer,
			size_t count, loff_t *data)
{
	char buf[32];
	char *p_buf;
	int len = count;
	long arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0;
	char *p_token = NULL;
	char *p_delimiter = " \t";
	int ret;

	if (len >= sizeof(buf)) {
		pr_info("input handling fail!\n");
		len = sizeof(buf) - 1;
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	pr_info("write parameter data = %s\n\r", buf);

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);
	if (!p_token)
		arg0 = 0;
	else
		ret = kstrtol(p_token, 10, &arg0);

	switch (arg0) {
	case 0:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		break;
	case 1:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		break;
	case 2:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg2 = 0;
		else
			ret = kstrtol(p_token, 10, &arg2);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg3 = 0;
		else
			ret = kstrtol(p_token, 10, &arg3);
		break;
	case 3:
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg1 = 0;
		else
			ret = kstrtol(p_token, 10, &arg1);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg2 = 0;
		else
			ret = kstrtol(p_token, 10, &arg2);
		p_token = strsep(&p_buf, p_delimiter);
		if (!p_token)
			arg3 = 0;
		else
			ret = kstrtol(p_token, 10, &arg3);
		break;
	}

	if (multicast_set_func[arg0] &&
	    (ARRAY_SIZE(multicast_set_func) > arg0)) {
		(*multicast_set_func[arg0]) (arg1, arg2, arg3);
	} else {
		pr_info("no handler defined for command id(0x%08lx)\n\r", arg0);
		(*multicast_set_func[0]) (0, 0, 0);
	}

	return len;
}

int cpu_reason_read(struct seq_file *seq, void *v)
{
	int i;

	pr_info("============ CPU REASON =========\n");
	pr_info("(2)IPv4(IPv6) TTL(hop limit) = %u\n",
		dbg_cpu_reason_cnt[0]);
	pr_info("(3)Ipv4(IPv6) has option(extension) header = %u\n",
		dbg_cpu_reason_cnt[1]);
	pr_info("(7)No flow is assigned = %u\n", dbg_cpu_reason_cnt[2]);
	pr_info("(8)IPv4 HNAT doesn't support IPv4 /w fragment = %u\n",
		dbg_cpu_reason_cnt[3]);
	pr_info("(9)IPv4 HNAPT/DS-Lite doesn't support IPv4 /w fragment = %u\n",
		dbg_cpu_reason_cnt[4]);
	pr_info("(10)IPv4 HNAPT/DS-Lite can't find TCP/UDP sport/dport = %u\n",
		dbg_cpu_reason_cnt[5]);
	pr_info("(11)IPv6 5T-route/6RD can't find TCP/UDP sport/dport = %u\n",
		dbg_cpu_reason_cnt[6]);
	pr_info("(12)Ingress packet is TCP fin/syn/rst = %u\n",
		dbg_cpu_reason_cnt[7]);
	pr_info("(13)FOE Un-hit = %u\n", dbg_cpu_reason_cnt[8]);
	pr_info("(14)FOE Hit unbind = %u\n", dbg_cpu_reason_cnt[9]);
	pr_info("(15)FOE Hit unbind & rate reach = %u\n", dbg_cpu_reason_cnt[10]);
	pr_info("(16)Hit bind PPE TCP FIN entry = %u\n", dbg_cpu_reason_cnt[11]);
	pr_info("(17)Hit bind PPE entry and TTL(hop limit) = 1 and TTL(hot limit) - 1 = %u\n",
		dbg_cpu_reason_cnt[12]);
	pr_info("(18)Hit bind and VLAN replacement violation = %u\n",
		dbg_cpu_reason_cnt[13]);
	pr_info("(19)Hit bind and keep alive with unicast old-header packet = %u\n",
		dbg_cpu_reason_cnt[14]);
	pr_info("(20)Hit bind and keep alive with multicast new-header packet = %u\n",
		dbg_cpu_reason_cnt[15]);
	pr_info("(21)Hit bind and keep alive with duplicate old-header packet = %u\n",
		dbg_cpu_reason_cnt[16]);
	pr_info("(22)FOE Hit bind & force to CPU = %u\n", dbg_cpu_reason_cnt[17]);
	pr_info("(28)Hit bind and exceed MTU =%u\n", dbg_cpu_reason_cnt[18]);
	pr_info("(24)Hit bind multicast packet to CPU = %u\n",
		dbg_cpu_reason_cnt[19]);
	pr_info("(25)Hit bind multicast packet to GMAC & CPU = %u\n",
		dbg_cpu_reason_cnt[20]);
	pr_info("(26)Pre bind = %u\n", dbg_cpu_reason_cnt[21]);

	for (i = 0; i < 22; i++)
		dbg_cpu_reason_cnt[i] = 0;
	return 0;
}

int entry_read(struct seq_file *seq, void *v)
{
	struct foe_entry *entry;
	int hash_index;
	int cnt;

	cnt = 0;
	for (hash_index = 0; hash_index < FOE_4TB_SIZ; hash_index++) {
		entry = &ppe_foe_base[hash_index];
		if (entry->bfib1.state == dbg_entry_state) {
			cnt++;
			dbg_dump_entry(hash_index);
		}
	}
	pr_info("Total State = %s cnt = %d\n",
		dbg_entry_state ==
		0 ? "Invalid" : dbg_entry_state ==
		1 ? "Unbind" : dbg_entry_state ==
		2 ? "BIND" : dbg_entry_state ==   3 ? "FIN" : "Unknown", cnt);

	return 0;
}

int setting_read(struct seq_file *seq, void *v)
{
	dbg_dump_cr();
	return 0;
}

int multicast_read(struct seq_file *seq, void *v)
{
	foe_mcast_entry_dump();
	return 0;
}

static int cpu_reason_open(struct inode *inode, struct file *file)
{
	return single_open(file, cpu_reason_read, NULL);
}

static int entry_open(struct inode *inode, struct file *file)
{
	return single_open(file, entry_read, NULL);
}

static int setting_open(struct inode *inode, struct file *file)
{
	return single_open(file, setting_read, NULL);
}

static int multicast_open(struct inode *inode, struct file *file)
{
	return single_open(file, multicast_read, NULL);
}

static const struct file_operations cpu_reason_fops = {
	.owner = THIS_MODULE,
	.open = cpu_reason_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = cpu_reason_write,
	.release = single_release
};

static const struct file_operations hnat_entry_fops = {
	.owner = THIS_MODULE,
	.open = entry_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = entry_write,
	.release = single_release
};

static const struct file_operations hnat_setting_fops = {
	.owner = THIS_MODULE,
	.open = setting_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = setting_write,
	.release = single_release
};

static const struct file_operations hnat_multicast_fops = {
	.owner = THIS_MODULE,
	.open = multicast_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = multicast_write,
	.release = single_release
};

int hnat_debug_proc_init(void)
{
	if (!hnat_proc_reg_dir)
		hnat_proc_reg_dir = proc_mkdir(HNAT_PROCREG_DIR, NULL);

	proc_cpu_reason = proc_create(PROCREG_CPU_REASON, 0,
				      hnat_proc_reg_dir, &cpu_reason_fops);
	if (!proc_cpu_reason)
		pr_debug("!! FAIL to create %s PROC !!\n", PROCREG_CPU_REASON);

	proc_hnat_entry = proc_create(PROCREG_PPE_ENTRY, 0,
				      hnat_proc_reg_dir, &hnat_entry_fops);
	if (!proc_hnat_entry)
		pr_debug("!! FAIL to create %s PROC !!\n", PROCREG_PPE_ENTRY);

	proc_hnat_setting = proc_create(PROCREG_PPE_SETTING, 0,
					hnat_proc_reg_dir, &hnat_setting_fops);
	if (!proc_hnat_setting)
		pr_debug("!! FAIL to create %s PROC !!\n", PROCREG_PPE_ENTRY);

	proc_hnat_multicast = proc_create(PROCREG_PPE_MULTICAST, 0,
					  hnat_proc_reg_dir, &hnat_multicast_fops);
	if (!proc_hnat_multicast)
		pr_debug("!! FAIL to create %s PROC !!\n", PROCREG_PPE_MULTICAST);

	return 0;
}

void hnat_debug_proc_exit(void)
{
	pr_info("proc exit\n");
	if (proc_cpu_reason)
		remove_proc_entry(PROCREG_CPU_REASON, hnat_proc_reg_dir);

	if (proc_hnat_entry)
		remove_proc_entry(PROCREG_PPE_ENTRY, hnat_proc_reg_dir);

	if (proc_hnat_setting)
		remove_proc_entry(PROCREG_PPE_SETTING, hnat_proc_reg_dir);

	if (proc_hnat_multicast)
		remove_proc_entry(PROCREG_PPE_MULTICAST, hnat_proc_reg_dir);

	if (hnat_proc_reg_dir)
		remove_proc_entry(HNAT_PROCREG_DIR, 0);
}

