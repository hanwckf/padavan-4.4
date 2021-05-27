#ifdef CONFIG_DEBUG_FS
/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <linux/export.h>

#include "hfi.h"
#include "debugfs.h"
#include "device.h"
#include "qp.h"
#include "sdma.h"

static struct dentry *hfi1_dbg_root;

#define private2dd(file) (file_inode(file)->i_private)
#define private2ppd(file) (file_inode(file)->i_private)

#define DEBUGFS_SEQ_FILE_OPS(name) \
static const struct seq_operations _##name##_seq_ops = { \
	.start = _##name##_seq_start, \
	.next  = _##name##_seq_next, \
	.stop  = _##name##_seq_stop, \
	.show  = _##name##_seq_show \
}
#define DEBUGFS_SEQ_FILE_OPEN(name) \
static int _##name##_open(struct inode *inode, struct file *s) \
{ \
	struct seq_file *seq; \
	int ret; \
	ret =  seq_open(s, &_##name##_seq_ops); \
	if (ret) \
		return ret; \
	seq = s->private_data; \
	seq->private = inode->i_private; \
	return 0; \
}

#define DEBUGFS_FILE_OPS(name) \
static const struct file_operations _##name##_file_ops = { \
	.owner   = THIS_MODULE, \
	.open    = _##name##_open, \
	.read    = seq_read, \
	.llseek  = seq_lseek, \
	.release = seq_release \
}

#define DEBUGFS_FILE_CREATE(name, parent, data, ops, mode)	\
do { \
	struct dentry *ent; \
	ent = debugfs_create_file(name, mode, parent, \
		data, ops); \
	if (!ent) \
		pr_warn("create of %s failed\n", name); \
} while (0)


#define DEBUGFS_SEQ_FILE_CREATE(name, parent, data) \
	DEBUGFS_FILE_CREATE(#name, parent, data, &_##name##_file_ops, S_IRUGO)

static void *_opcode_stats_seq_start(struct seq_file *s, loff_t *pos)
__acquires(RCU)
{
	struct hfi1_opcode_stats_perctx *opstats;

	rcu_read_lock();
	if (*pos >= ARRAY_SIZE(opstats->stats))
		return NULL;
	return pos;
}

static void *_opcode_stats_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct hfi1_opcode_stats_perctx *opstats;

	++*pos;
	if (*pos >= ARRAY_SIZE(opstats->stats))
		return NULL;
	return pos;
}


static void _opcode_stats_seq_stop(struct seq_file *s, void *v)
__releases(RCU)
{
	rcu_read_unlock();
}

static int _opcode_stats_seq_show(struct seq_file *s, void *v)
{
	loff_t *spos = v;
	loff_t i = *spos, j;
	u64 n_packets = 0, n_bytes = 0;
	struct hfi1_ibdev *ibd = (struct hfi1_ibdev *)s->private;
	struct hfi1_devdata *dd = dd_from_dev(ibd);

	for (j = 0; j < dd->first_user_ctxt; j++) {
		if (!dd->rcd[j])
			continue;
		n_packets += dd->rcd[j]->opstats->stats[i].n_packets;
		n_bytes += dd->rcd[j]->opstats->stats[i].n_bytes;
	}
	if (!n_packets && !n_bytes)
		return SEQ_SKIP;
	seq_printf(s, "%02llx %llu/%llu\n", i,
		(unsigned long long) n_packets,
		(unsigned long long) n_bytes);

	return 0;
}

DEBUGFS_SEQ_FILE_OPS(opcode_stats);
DEBUGFS_SEQ_FILE_OPEN(opcode_stats)
DEBUGFS_FILE_OPS(opcode_stats);

static void *_ctx_stats_seq_start(struct seq_file *s, loff_t *pos)
{
	struct hfi1_ibdev *ibd = (struct hfi1_ibdev *)s->private;
	struct hfi1_devdata *dd = dd_from_dev(ibd);

	if (!*pos)
		return SEQ_START_TOKEN;
	if (*pos >= dd->first_user_ctxt)
		return NULL;
	return pos;
}

static void *_ctx_stats_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct hfi1_ibdev *ibd = (struct hfi1_ibdev *)s->private;
	struct hfi1_devdata *dd = dd_from_dev(ibd);

	if (v == SEQ_START_TOKEN)
		return pos;

	++*pos;
	if (*pos >= dd->first_user_ctxt)
		return NULL;
	return pos;
}

static void _ctx_stats_seq_stop(struct seq_file *s, void *v)
{
	/* nothing allocated */
}

static int _ctx_stats_seq_show(struct seq_file *s, void *v)
{
	loff_t *spos;
	loff_t i, j;
	u64 n_packets = 0;
	struct hfi1_ibdev *ibd = (struct hfi1_ibdev *)s->private;
	struct hfi1_devdata *dd = dd_from_dev(ibd);

	if (v == SEQ_START_TOKEN) {
		seq_puts(s, "Ctx:npkts\n");
		return 0;
	}

	spos = v;
	i = *spos;

	if (!dd->rcd[i])
		return SEQ_SKIP;

	for (j = 0; j < ARRAY_SIZE(dd->rcd[i]->opstats->stats); j++)
		n_packets += dd->rcd[i]->opstats->stats[j].n_packets;

	if (!n_packets)
		return SEQ_SKIP;

	seq_printf(s, "  %llu:%llu\n", i, n_packets);
	return 0;
}

DEBUGFS_SEQ_FILE_OPS(ctx_stats);
DEBUGFS_SEQ_FILE_OPEN(ctx_stats)
DEBUGFS_FILE_OPS(ctx_stats);

static void *_qp_stats_seq_start(struct seq_file *s, loff_t *pos)
__acquires(RCU)
{
	struct qp_iter *iter;
	loff_t n = *pos;

	rcu_read_lock();
	iter = qp_iter_init(s->private);
	if (!iter)
		return NULL;

	while (n--) {
		if (qp_iter_next(iter)) {
			kfree(iter);
			return NULL;
		}
	}

	return iter;
}

static void *_qp_stats_seq_next(struct seq_file *s, void *iter_ptr,
				   loff_t *pos)
{
	struct qp_iter *iter = iter_ptr;

	(*pos)++;

	if (qp_iter_next(iter)) {
		kfree(iter);
		return NULL;
	}

	return iter;
}

static void _qp_stats_seq_stop(struct seq_file *s, void *iter_ptr)
__releases(RCU)
{
	rcu_read_unlock();
}

static int _qp_stats_seq_show(struct seq_file *s, void *iter_ptr)
{
	struct qp_iter *iter = iter_ptr;

	if (!iter)
		return 0;

	qp_iter_print(s, iter);

	return 0;
}

DEBUGFS_SEQ_FILE_OPS(qp_stats);
DEBUGFS_SEQ_FILE_OPEN(qp_stats)
DEBUGFS_FILE_OPS(qp_stats);

static void *_sdes_seq_start(struct seq_file *s, loff_t *pos)
__acquires(RCU)
{
	struct hfi1_ibdev *ibd;
	struct hfi1_devdata *dd;

	rcu_read_lock();
	ibd = (struct hfi1_ibdev *)s->private;
	dd = dd_from_dev(ibd);
	if (!dd->per_sdma || *pos >= dd->num_sdma)
		return NULL;
	return pos;
}

static void *_sdes_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct hfi1_ibdev *ibd = (struct hfi1_ibdev *)s->private;
	struct hfi1_devdata *dd = dd_from_dev(ibd);

	++*pos;
	if (!dd->per_sdma || *pos >= dd->num_sdma)
		return NULL;
	return pos;
}


static void _sdes_seq_stop(struct seq_file *s, void *v)
__releases(RCU)
{
	rcu_read_unlock();
}

static int _sdes_seq_show(struct seq_file *s, void *v)
{
	struct hfi1_ibdev *ibd = (struct hfi1_ibdev *)s->private;
	struct hfi1_devdata *dd = dd_from_dev(ibd);
	loff_t *spos = v;
	loff_t i = *spos;

	sdma_seqfile_dump_sde(s, &dd->per_sdma[i]);
	return 0;
}

DEBUGFS_SEQ_FILE_OPS(sdes);
DEBUGFS_SEQ_FILE_OPEN(sdes)
DEBUGFS_FILE_OPS(sdes);

/* read the per-device counters */
static ssize_t dev_counters_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	u64 *counters;
	size_t avail;
	struct hfi1_devdata *dd;
	ssize_t rval;

	rcu_read_lock();
	dd = private2dd(file);
	avail = hfi1_read_cntrs(dd, *ppos, NULL, &counters);
	rval =  simple_read_from_buffer(buf, count, ppos, counters, avail);
	rcu_read_unlock();
	return rval;
}

/* read the per-device counters */
static ssize_t dev_names_read(struct file *file, char __user *buf,
			      size_t count, loff_t *ppos)
{
	char *names;
	size_t avail;
	struct hfi1_devdata *dd;
	ssize_t rval;

	rcu_read_lock();
	dd = private2dd(file);
	avail = hfi1_read_cntrs(dd, *ppos, &names, NULL);
	rval =  simple_read_from_buffer(buf, count, ppos, names, avail);
	rcu_read_unlock();
	return rval;
}

struct counter_info {
	char *name;
	const struct file_operations ops;
};

/*
 * Could use file_inode(file)->i_ino to figure out which file,
 * instead of separate routine for each, but for now, this works...
 */

/* read the per-port names (same for each port) */
static ssize_t portnames_read(struct file *file, char __user *buf,
			      size_t count, loff_t *ppos)
{
	char *names;
	size_t avail;
	struct hfi1_devdata *dd;
	ssize_t rval;

	rcu_read_lock();
	dd = private2dd(file);
	/* port number n/a here since names are constant */
	avail = hfi1_read_portcntrs(dd, *ppos, 0, &names, NULL);
	rval = simple_read_from_buffer(buf, count, ppos, names, avail);
	rcu_read_unlock();
	return rval;
}

/* read the per-port counters */
static ssize_t portcntrs_debugfs_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	u64 *counters;
	size_t avail;
	struct hfi1_devdata *dd;
	struct hfi1_pportdata *ppd;
	ssize_t rval;

	rcu_read_lock();
	ppd = private2ppd(file);
	dd = ppd->dd;
	avail = hfi1_read_portcntrs(dd, *ppos, ppd->port - 1, NULL, &counters);
	rval = simple_read_from_buffer(buf, count, ppos, counters, avail);
	rcu_read_unlock();
	return rval;
}

/*
 * read the per-port QSFP data for ppd
 */
static ssize_t qsfp_debugfs_dump(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct hfi1_pportdata *ppd;
	char *tmp;
	int ret;

	rcu_read_lock();
	ppd = private2ppd(file);
	tmp = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!tmp) {
		rcu_read_unlock();
		return -ENOMEM;
	}

	ret = qsfp_dump(ppd, tmp, PAGE_SIZE);
	if (ret > 0)
		ret = simple_read_from_buffer(buf, count, ppos, tmp, ret);
	rcu_read_unlock();
	kfree(tmp);
	return ret;
}

/* Do an i2c write operation on the chain for the given HFI. */
static ssize_t __i2c_debugfs_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos, u32 target)
{
	struct hfi1_pportdata *ppd;
	char *buff;
	int ret;
	int i2c_addr;
	int offset;
	int total_written;

	rcu_read_lock();
	ppd = private2ppd(file);

	buff = kmalloc(count, GFP_KERNEL);
	if (!buff) {
		ret = -ENOMEM;
		goto _return;
	}

	ret = copy_from_user(buff, buf, count);
	if (ret > 0) {
		ret = -EFAULT;
		goto _free;
	}

	i2c_addr = (*ppos >> 16) & 0xff;
	offset = *ppos & 0xffff;

	total_written = i2c_write(ppd, target, i2c_addr, offset, buff, count);
	if (total_written < 0) {
		ret = total_written;
		goto _free;
	}

	*ppos += total_written;

	ret = total_written;

 _free:
	kfree(buff);
 _return:
	rcu_read_unlock();
	return ret;
}

/* Do an i2c write operation on chain for HFI 0. */
static ssize_t i2c1_debugfs_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	return __i2c_debugfs_write(file, buf, count, ppos, 0);
}

/* Do an i2c write operation on chain for HFI 1. */
static ssize_t i2c2_debugfs_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	return __i2c_debugfs_write(file, buf, count, ppos, 1);
}

/* Do an i2c read operation on the chain for the given HFI. */
static ssize_t __i2c_debugfs_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos, u32 target)
{
	struct hfi1_pportdata *ppd;
	char *buff;
	int ret;
	int i2c_addr;
	int offset;
	int total_read;

	rcu_read_lock();
	ppd = private2ppd(file);

	buff = kmalloc(count, GFP_KERNEL);
	if (!buff) {
		ret = -ENOMEM;
		goto _return;
	}

	i2c_addr = (*ppos >> 16) & 0xff;
	offset = *ppos & 0xffff;

	total_read = i2c_read(ppd, target, i2c_addr, offset, buff, count);
	if (total_read < 0) {
		ret = total_read;
		goto _free;
	}

	*ppos += total_read;

	ret = copy_to_user(buf, buff, total_read);
	if (ret > 0) {
		ret = -EFAULT;
		goto _free;
	}

	ret = total_read;

 _free:
	kfree(buff);
 _return:
	rcu_read_unlock();
	return ret;
}

/* Do an i2c read operation on chain for HFI 0. */
static ssize_t i2c1_debugfs_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	return __i2c_debugfs_read(file, buf, count, ppos, 0);
}

/* Do an i2c read operation on chain for HFI 1. */
static ssize_t i2c2_debugfs_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	return __i2c_debugfs_read(file, buf, count, ppos, 1);
}

/* Do a QSFP write operation on the i2c chain for the given HFI. */
static ssize_t __qsfp_debugfs_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos, u32 target)
{
	struct hfi1_pportdata *ppd;
	char *buff;
	int ret;
	int total_written;

	rcu_read_lock();
	if (*ppos + count > QSFP_PAGESIZE * 4) { /* base page + page00-page03 */
		ret = -EINVAL;
		goto _return;
	}

	ppd = private2ppd(file);

	buff = kmalloc(count, GFP_KERNEL);
	if (!buff) {
		ret = -ENOMEM;
		goto _return;
	}

	ret = copy_from_user(buff, buf, count);
	if (ret > 0) {
		ret = -EFAULT;
		goto _free;
	}

	total_written = qsfp_write(ppd, target, *ppos, buff, count);
	if (total_written < 0) {
		ret = total_written;
		goto _free;
	}

	*ppos += total_written;

	ret = total_written;

 _free:
	kfree(buff);
 _return:
	rcu_read_unlock();
	return ret;
}

/* Do a QSFP write operation on i2c chain for HFI 0. */
static ssize_t qsfp1_debugfs_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	return __qsfp_debugfs_write(file, buf, count, ppos, 0);
}

/* Do a QSFP write operation on i2c chain for HFI 1. */
static ssize_t qsfp2_debugfs_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	return __qsfp_debugfs_write(file, buf, count, ppos, 1);
}

/* Do a QSFP read operation on the i2c chain for the given HFI. */
static ssize_t __qsfp_debugfs_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos, u32 target)
{
	struct hfi1_pportdata *ppd;
	char *buff;
	int ret;
	int total_read;

	rcu_read_lock();
	if (*ppos + count > QSFP_PAGESIZE * 4) { /* base page + page00-page03 */
		ret = -EINVAL;
		goto _return;
	}

	ppd = private2ppd(file);

	buff = kmalloc(count, GFP_KERNEL);
	if (!buff) {
		ret = -ENOMEM;
		goto _return;
	}

	total_read = qsfp_read(ppd, target, *ppos, buff, count);
	if (total_read < 0) {
		ret = total_read;
		goto _free;
	}

	*ppos += total_read;

	ret = copy_to_user(buf, buff, total_read);
	if (ret > 0) {
		ret = -EFAULT;
		goto _free;
	}

	ret = total_read;

 _free:
	kfree(buff);
 _return:
	rcu_read_unlock();
	return ret;
}

/* Do a QSFP read operation on i2c chain for HFI 0. */
static ssize_t qsfp1_debugfs_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	return __qsfp_debugfs_read(file, buf, count, ppos, 0);
}

/* Do a QSFP read operation on i2c chain for HFI 1. */
static ssize_t qsfp2_debugfs_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	return __qsfp_debugfs_read(file, buf, count, ppos, 1);
}

#define DEBUGFS_OPS(nm, readroutine, writeroutine)	\
{ \
	.name = nm, \
	.ops = { \
		.read = readroutine, \
		.write = writeroutine, \
		.llseek = generic_file_llseek, \
	}, \
}

static const struct counter_info cntr_ops[] = {
	DEBUGFS_OPS("counter_names", dev_names_read, NULL),
	DEBUGFS_OPS("counters", dev_counters_read, NULL),
	DEBUGFS_OPS("portcounter_names", portnames_read, NULL),
};

static const struct counter_info port_cntr_ops[] = {
	DEBUGFS_OPS("port%dcounters", portcntrs_debugfs_read, NULL),
	DEBUGFS_OPS("i2c1", i2c1_debugfs_read, i2c1_debugfs_write),
	DEBUGFS_OPS("i2c2", i2c2_debugfs_read, i2c2_debugfs_write),
	DEBUGFS_OPS("qsfp_dump%d", qsfp_debugfs_dump, NULL),
	DEBUGFS_OPS("qsfp1", qsfp1_debugfs_read, qsfp1_debugfs_write),
	DEBUGFS_OPS("qsfp2", qsfp2_debugfs_read, qsfp2_debugfs_write),
};

void hfi1_dbg_ibdev_init(struct hfi1_ibdev *ibd)
{
	char name[sizeof("port0counters") + 1];
	char link[10];
	struct hfi1_devdata *dd = dd_from_dev(ibd);
	struct hfi1_pportdata *ppd;
	int unit = dd->unit;
	int i, j;

	if (!hfi1_dbg_root)
		return;
	snprintf(name, sizeof(name), "%s_%d", class_name(), unit);
	snprintf(link, sizeof(link), "%d", unit);
	ibd->hfi1_ibdev_dbg = debugfs_create_dir(name, hfi1_dbg_root);
	if (!ibd->hfi1_ibdev_dbg) {
		pr_warn("create of %s failed\n", name);
		return;
	}
	ibd->hfi1_ibdev_link =
		debugfs_create_symlink(link, hfi1_dbg_root, name);
	if (!ibd->hfi1_ibdev_link) {
		pr_warn("create of %s symlink failed\n", name);
		return;
	}
	DEBUGFS_SEQ_FILE_CREATE(opcode_stats, ibd->hfi1_ibdev_dbg, ibd);
	DEBUGFS_SEQ_FILE_CREATE(ctx_stats, ibd->hfi1_ibdev_dbg, ibd);
	DEBUGFS_SEQ_FILE_CREATE(qp_stats, ibd->hfi1_ibdev_dbg, ibd);
	DEBUGFS_SEQ_FILE_CREATE(sdes, ibd->hfi1_ibdev_dbg, ibd);
	/* dev counter files */
	for (i = 0; i < ARRAY_SIZE(cntr_ops); i++)
		DEBUGFS_FILE_CREATE(cntr_ops[i].name,
				    ibd->hfi1_ibdev_dbg,
				    dd,
				    &cntr_ops[i].ops, S_IRUGO);
	/* per port files */
	for (ppd = dd->pport, j = 0; j < dd->num_pports; j++, ppd++)
		for (i = 0; i < ARRAY_SIZE(port_cntr_ops); i++) {
			snprintf(name,
				 sizeof(name),
				 port_cntr_ops[i].name,
				 j + 1);
			DEBUGFS_FILE_CREATE(name,
					    ibd->hfi1_ibdev_dbg,
					    ppd,
					    &port_cntr_ops[i].ops,
					    port_cntr_ops[i].ops.write == NULL ?
					    S_IRUGO : S_IRUGO|S_IWUSR);
		}
}

void hfi1_dbg_ibdev_exit(struct hfi1_ibdev *ibd)
{
	if (!hfi1_dbg_root)
		goto out;
	debugfs_remove(ibd->hfi1_ibdev_link);
	debugfs_remove_recursive(ibd->hfi1_ibdev_dbg);
out:
	ibd->hfi1_ibdev_dbg = NULL;
	synchronize_rcu();
}

/*
 * driver stats field names, one line per stat, single string.  Used by
 * programs like hfistats to print the stats in a way which works for
 * different versions of drivers, without changing program source.
 * if hfi1_ib_stats changes, this needs to change.  Names need to be
 * 12 chars or less (w/o newline), for proper display by hfistats utility.
 */
static const char * const hfi1_statnames[] = {
	/* must be element 0*/
	"KernIntr",
	"ErrorIntr",
	"Tx_Errs",
	"Rcv_Errs",
	"H/W_Errs",
	"NoPIOBufs",
	"CtxtsOpen",
	"RcvLen_Errs",
	"EgrBufFull",
	"EgrHdrFull"
};

static void *_driver_stats_names_seq_start(struct seq_file *s, loff_t *pos)
__acquires(RCU)
{
	rcu_read_lock();
	if (*pos >= ARRAY_SIZE(hfi1_statnames))
		return NULL;
	return pos;
}

static void *_driver_stats_names_seq_next(
	struct seq_file *s,
	void *v,
	loff_t *pos)
{
	++*pos;
	if (*pos >= ARRAY_SIZE(hfi1_statnames))
		return NULL;
	return pos;
}

static void _driver_stats_names_seq_stop(struct seq_file *s, void *v)
__releases(RCU)
{
	rcu_read_unlock();
}

static int _driver_stats_names_seq_show(struct seq_file *s, void *v)
{
	loff_t *spos = v;

	seq_printf(s, "%s\n", hfi1_statnames[*spos]);
	return 0;
}

DEBUGFS_SEQ_FILE_OPS(driver_stats_names);
DEBUGFS_SEQ_FILE_OPEN(driver_stats_names)
DEBUGFS_FILE_OPS(driver_stats_names);

static void *_driver_stats_seq_start(struct seq_file *s, loff_t *pos)
__acquires(RCU)
{
	rcu_read_lock();
	if (*pos >= ARRAY_SIZE(hfi1_statnames))
		return NULL;
	return pos;
}

static void *_driver_stats_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	++*pos;
	if (*pos >= ARRAY_SIZE(hfi1_statnames))
		return NULL;
	return pos;
}

static void _driver_stats_seq_stop(struct seq_file *s, void *v)
__releases(RCU)
{
	rcu_read_unlock();
}

static u64 hfi1_sps_ints(void)
{
	unsigned long flags;
	struct hfi1_devdata *dd;
	u64 sps_ints = 0;

	spin_lock_irqsave(&hfi1_devs_lock, flags);
	list_for_each_entry(dd, &hfi1_dev_list, list) {
		sps_ints += get_all_cpu_total(dd->int_counter);
	}
	spin_unlock_irqrestore(&hfi1_devs_lock, flags);
	return sps_ints;
}

static int _driver_stats_seq_show(struct seq_file *s, void *v)
{
	loff_t *spos = v;
	char *buffer;
	u64 *stats = (u64 *)&hfi1_stats;
	size_t sz = seq_get_buf(s, &buffer);

	if (sz < sizeof(u64))
		return SEQ_SKIP;
	/* special case for interrupts */
	if (*spos == 0)
		*(u64 *)buffer = hfi1_sps_ints();
	else
		*(u64 *)buffer = stats[*spos];
	seq_commit(s,  sizeof(u64));
	return 0;
}

DEBUGFS_SEQ_FILE_OPS(driver_stats);
DEBUGFS_SEQ_FILE_OPEN(driver_stats)
DEBUGFS_FILE_OPS(driver_stats);

void hfi1_dbg_init(void)
{
	hfi1_dbg_root  = debugfs_create_dir(DRIVER_NAME, NULL);
	if (!hfi1_dbg_root)
		pr_warn("init of debugfs failed\n");
	DEBUGFS_SEQ_FILE_CREATE(driver_stats_names, hfi1_dbg_root, NULL);
	DEBUGFS_SEQ_FILE_CREATE(driver_stats, hfi1_dbg_root, NULL);
}

void hfi1_dbg_exit(void)
{
	debugfs_remove_recursive(hfi1_dbg_root);
	hfi1_dbg_root = NULL;
}

#endif
