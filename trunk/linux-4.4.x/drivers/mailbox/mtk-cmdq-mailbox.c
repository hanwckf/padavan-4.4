/*
 * Copyright (c) 2015 MediaTek Inc.
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

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mailbox_controller.h>
#include <linux/mailbox/mtk-cmdq-mailbox.h>
#include <linux/soc/mediatek/mtk-cmdq.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#define CMDQ_THR_MAX_COUNT		4 /* main, sub, general(misc) */
#define CMDQ_OP_CODE_MASK		(0xff << CMDQ_OP_CODE_SHIFT)
#define CMDQ_TIMEOUT_MS			1000
#define CMDQ_IRQ_MASK			0xffff
#define CMDQ_NUM_CMD(t)			(t->cmd_buf_size / CMDQ_INST_SIZE)

#define CMDQ_CURR_IRQ_STATUS		0x10
#define CMDQ_THR_SLOT_CYCLES		0x30

#define CMDQ_THR_BASE			0x100
#define CMDQ_THR_SIZE			0x80
#define CMDQ_THR_WARM_RESET		0x00
#define CMDQ_THR_ENABLE_TASK		0x04
#define CMDQ_THR_SUSPEND_TASK		0x08
#define CMDQ_THR_CURR_STATUS		0x0c
#define CMDQ_THR_IRQ_STATUS		0x10
#define CMDQ_THR_IRQ_ENABLE		0x14
#define CMDQ_THR_CURR_ADDR		0x20
#define CMDQ_THR_END_ADDR		0x24
#define CMDQ_THR_WAIT_TOKEN		0x30

#define CMDQ_THR_ENABLED		0x1
#define CMDQ_THR_DISABLED		0x0
#define CMDQ_THR_SUSPEND		0x1
#define CMDQ_THR_RESUME			0x0
#define CMDQ_THR_STATUS_SUSPENDED	BIT(1)
#define CMDQ_THR_DO_WARM_RESET		BIT(0)
#define CMDQ_THR_ACTIVE_SLOT_CYCLES	0x3200
#define CMDQ_THR_IRQ_DONE		0x1
#define CMDQ_THR_IRQ_ERROR		0x12
#define CMDQ_THR_IRQ_EN			(CMDQ_THR_IRQ_ERROR | CMDQ_THR_IRQ_DONE)
#define CMDQ_THR_IS_WAITING		BIT(31)

#define CMDQ_JUMP_BY_OFFSET		0x10000000
#define CMDQ_JUMP_BY_PA			0x10000001

struct cmdq_thread {
	struct mbox_chan	*chan;
	void __iomem		*base;
	struct list_head	task_busy_list;
	struct timer_list	timeout;
	bool			atomic_exec;
};

struct cmdq_task {
	struct cmdq		*cmdq;
	struct list_head	list_entry;
	dma_addr_t		pa_base;
	struct cmdq_thread	*thread;
	struct cmdq_pkt		*pkt; /* the packet sent from mailbox client */
};

struct cmdq_buf_dump {
	struct cmdq		*cmdq;
	struct work_struct	dump_work;
	bool			timeout; /* 0: error, 1: timeout */
	void			*cmd_buf;
	size_t			cmd_buf_size;
	u32			pa_offset; /* pa_curr - pa_base */
};

struct cmdq {
	struct mbox_controller	mbox;
	void __iomem		*base;
	u32			irq;
	struct workqueue_struct	*buf_dump_wq;
	struct cmdq_thread	thread[CMDQ_THR_MAX_COUNT];
	struct clk		*clock;
	bool			suspended;
};

static int cmdq_thread_suspend(struct cmdq *cmdq, struct cmdq_thread *thread)
{
	u32 status;

	writel(CMDQ_THR_SUSPEND, thread->base + CMDQ_THR_SUSPEND_TASK);

	/* If already disabled, treat as suspended successful. */
	if (!(readl(thread->base + CMDQ_THR_ENABLE_TASK) & CMDQ_THR_ENABLED))
		return 0;

	if (readl_poll_timeout_atomic(thread->base + CMDQ_THR_CURR_STATUS,
			status, status & CMDQ_THR_STATUS_SUSPENDED, 0, 10)) {
		dev_err(cmdq->mbox.dev, "suspend GCE thread 0x%x failed\n",
			(u32)(thread->base - cmdq->base));
		return -EFAULT;
	}

	return 0;
}

static void cmdq_thread_resume(struct cmdq_thread *thread)
{
	writel(CMDQ_THR_RESUME, thread->base + CMDQ_THR_SUSPEND_TASK);
}

static int cmdq_thread_reset(struct cmdq *cmdq, struct cmdq_thread *thread)
{
	u32 warm_reset;

	writel(CMDQ_THR_DO_WARM_RESET, thread->base + CMDQ_THR_WARM_RESET);
	if (readl_poll_timeout_atomic(thread->base + CMDQ_THR_WARM_RESET,
			warm_reset, !(warm_reset & CMDQ_THR_DO_WARM_RESET),
			0, 10)) {
		dev_err(cmdq->mbox.dev, "reset GCE thread 0x%x failed\n",
			(u32)(thread->base - cmdq->base));
		return -EFAULT;
	}
	writel(CMDQ_THR_ACTIVE_SLOT_CYCLES, cmdq->base + CMDQ_THR_SLOT_CYCLES);
	return 0;
}

static void cmdq_thread_disable(struct cmdq *cmdq, struct cmdq_thread *thread)
{
	cmdq_thread_reset(cmdq, thread);
	writel(CMDQ_THR_DISABLED, thread->base + CMDQ_THR_ENABLE_TASK);
}

/* notify GCE to re-fetch commands by setting GCE thread PC */
static void cmdq_thread_invalidate_fetched_data(struct cmdq_thread *thread)
{
	writel(readl(thread->base + CMDQ_THR_CURR_ADDR),
	       thread->base + CMDQ_THR_CURR_ADDR);
}

static void cmdq_task_insert_into_thread(struct cmdq_task *task)
{
	struct device *dev = task->cmdq->mbox.dev;
	struct cmdq_thread *thread = task->thread;
	struct cmdq_task *prev_task = list_last_entry(
			&thread->task_busy_list, typeof(*task), list_entry);
	u64 *prev_task_base = prev_task->pkt->va_base;

	/* let previous task jump to this task */
	dma_sync_single_for_cpu(dev, prev_task->pa_base,
				prev_task->pkt->cmd_buf_size, DMA_TO_DEVICE);
	prev_task_base[CMDQ_NUM_CMD(prev_task->pkt) - 1] =
		(u64)CMDQ_JUMP_BY_PA << 32 | task->pa_base;
	dma_sync_single_for_device(dev, prev_task->pa_base,
				   prev_task->pkt->cmd_buf_size, DMA_TO_DEVICE);

	cmdq_thread_invalidate_fetched_data(thread);
}

static bool cmdq_command_is_wfe(u64 cmd)
{
	u64 wfe_option = CMDQ_WFE_UPDATE | CMDQ_WFE_WAIT | CMDQ_WFE_WAIT_VALUE;
	u64 wfe_op = (u64)(CMDQ_CODE_WFE << CMDQ_OP_CODE_SHIFT) << 32;
	u64 wfe_mask = (u64)CMDQ_OP_CODE_MASK << 32 | 0xffffffff;

	return ((cmd & wfe_mask) == (wfe_op | wfe_option));
}

/* we assume tasks in the same display GCE thread are waiting the same event. */
static void cmdq_task_remove_wfe(struct cmdq_task *task)
{
	struct device *dev = task->cmdq->mbox.dev;
	u64 *base = task->pkt->va_base;
	int i;

	dma_sync_single_for_cpu(dev, task->pa_base, task->pkt->cmd_buf_size,
				DMA_TO_DEVICE);
	for (i = 0; i < CMDQ_NUM_CMD(task->pkt); i++)
		if (cmdq_command_is_wfe(base[i]))
			base[i] = (u64)CMDQ_JUMP_BY_OFFSET << 32 |
				  CMDQ_JUMP_PASS;
	dma_sync_single_for_device(dev, task->pa_base, task->pkt->cmd_buf_size,
				   DMA_TO_DEVICE);
}

static bool cmdq_thread_is_in_wfe(struct cmdq_thread *thread)
{
	return readl(thread->base + CMDQ_THR_WAIT_TOKEN) & CMDQ_THR_IS_WAITING;
}

static void cmdq_thread_wait_end(struct cmdq_thread *thread,
				 unsigned long end_pa)
{
	struct device *dev = thread->chan->mbox->dev;
	unsigned long curr_pa;

	if (readl_poll_timeout_atomic(thread->base + CMDQ_THR_CURR_ADDR,
			curr_pa, curr_pa == end_pa, 1, 20))
		dev_err(dev, "GCE thread cannot run to end.\n");
}

static void cmdq_task_exec(struct cmdq_pkt *pkt, struct cmdq_thread *thread)
{
	struct cmdq *cmdq;
	struct cmdq_task *task;
	unsigned long curr_pa, end_pa;

	cmdq = dev_get_drvdata(thread->chan->mbox->dev);

	/* Client should not flush new tasks if suspended. */
	WARN_ON(cmdq->suspended);

	task = kzalloc(sizeof(*task), GFP_ATOMIC);
	task->cmdq = cmdq;
	INIT_LIST_HEAD(&task->list_entry);
	task->pa_base = dma_map_single(cmdq->mbox.dev, pkt->va_base,
				       pkt->cmd_buf_size, DMA_TO_DEVICE);
	task->thread = thread;
	task->pkt = pkt;

	if (list_empty(&thread->task_busy_list)) {
		WARN_ON(clk_enable(cmdq->clock) < 0);
		WARN_ON(cmdq_thread_reset(cmdq, thread) < 0);

		writel(task->pa_base, thread->base + CMDQ_THR_CURR_ADDR);
		writel(task->pa_base + pkt->cmd_buf_size,
		       thread->base + CMDQ_THR_END_ADDR);
		writel(CMDQ_THR_IRQ_EN, thread->base + CMDQ_THR_IRQ_ENABLE);
		writel(CMDQ_THR_ENABLED, thread->base + CMDQ_THR_ENABLE_TASK);

		mod_timer(&thread->timeout,
			  jiffies + msecs_to_jiffies(CMDQ_TIMEOUT_MS));
	} else {
		WARN_ON(cmdq_thread_suspend(cmdq, thread) < 0);
		curr_pa = readl(thread->base + CMDQ_THR_CURR_ADDR);
		end_pa = readl(thread->base + CMDQ_THR_END_ADDR);

		/*
		 * Atomic execution should remove the following wfe, i.e. only
		 * wait event at first task, and prevent to pause when running.
		 */
		if (thread->atomic_exec) {
			/* GCE is executing if command is not WFE */
			if (!cmdq_thread_is_in_wfe(thread)) {
				cmdq_thread_resume(thread);
				cmdq_thread_wait_end(thread, end_pa);
				WARN_ON(cmdq_thread_suspend(cmdq, thread) < 0);
				/* set to this task directly */
				writel(task->pa_base,
				       thread->base + CMDQ_THR_CURR_ADDR);
			} else {
				cmdq_task_insert_into_thread(task);
				cmdq_task_remove_wfe(task);
				smp_mb(); /* modify jump before enable thread */
			}
		} else {
			/* check boundary */
			if (curr_pa == end_pa - CMDQ_INST_SIZE ||
			    curr_pa == end_pa) {
				/* set to this task directly */
				writel(task->pa_base,
				       thread->base + CMDQ_THR_CURR_ADDR);
			} else {
				cmdq_task_insert_into_thread(task);
				smp_mb(); /* modify jump before enable thread */
			}
		}
		writel(task->pa_base + pkt->cmd_buf_size,
		       thread->base + CMDQ_THR_END_ADDR);
		cmdq_thread_resume(thread);
	}
	list_move_tail(&task->list_entry, &thread->task_busy_list);
}

static void cmdq_task_exec_done(struct cmdq_task *task, bool err)
{
	struct device *dev = task->cmdq->mbox.dev;
	struct cmdq_cb_data cmdq_cb_data;

	dma_unmap_single(dev, task->pa_base, task->pkt->cmd_buf_size,
			 DMA_TO_DEVICE);
	if (task->pkt->cb.cb) {
		cmdq_cb_data.err = err;
		cmdq_cb_data.data = task->pkt->cb.data;
		task->pkt->cb.cb(cmdq_cb_data);
	}
	list_del(&task->list_entry);
}

static void cmdq_buf_print_write(struct device *dev, u32 offset, u64 cmd)
{
	u32 subsys, addr_base, addr, value;

	subsys = ((u32)(cmd >> (32 + CMDQ_SUBSYS_SHIFT)) & 0x1f);
	addr_base = cmdq_subsys_id_to_base(subsys);
	addr = ((u32)(cmd >> 32) & 0xffff) | (addr_base << CMDQ_SUBSYS_SHIFT);
	value = (u32)(cmd & 0xffffffff);
	dev_err(dev,
		"0x%08x 0x%016llx write register 0x%08x with value 0x%08x\n",
		offset, cmd, addr, value);
}

static void cmdq_buf_print_wfe(struct device *dev, u32 offset, u64 cmd)
{
	u32 event = (u32)(cmd >> 32) & 0x1ff;
	char *event_str;

	/* Display start of frame(SOF) events */
	if (event == cmdq_event_value[CMDQ_EVENT_DISP_OVL0_SOF])
		event_str = "CMDQ_EVENT_DISP_OVL0_SOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_OVL1_SOF])
		event_str = "CMDQ_EVENT_DISP_OVL1_SOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_RDMA0_SOF])
		event_str = "CMDQ_EVENT_DISP_RDMA0_SOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_RDMA1_SOF])
		event_str = "CMDQ_EVENT_DISP_RDMA1_SOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_RDMA2_SOF])
		event_str = "CMDQ_EVENT_DISP_RDMA2_SOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_WDMA0_SOF])
		event_str = "CMDQ_EVENT_DISP_WDMA0_SOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_WDMA1_SOF])
		event_str = "CMDQ_EVENT_DISP_WDMA1_SOF";
	/* Display end of frame(EOF) events */
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_OVL0_EOF])
		event_str = "CMDQ_EVENT_DISP_OVL0_EOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_OVL1_EOF])
		event_str = "CMDQ_EVENT_DISP_OVL1_EOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_RDMA0_EOF])
		event_str = "CMDQ_EVENT_DISP_RDMA0_EOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_RDMA1_EOF])
		event_str = "CMDQ_EVENT_DISP_RDMA1_EOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_RDMA2_EOF])
		event_str = "CMDQ_EVENT_DISP_RDMA2_EOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_WDMA0_EOF])
		event_str = "CMDQ_EVENT_DISP_WDMA0_EOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_WDMA1_EOF])
		event_str = "CMDQ_EVENT_DISP_WDMA1_EOF";
	/* Mutex end of frame(EOF) events */
	else if (event == cmdq_event_value[CMDQ_EVENT_MUTEX0_STREAM_EOF])
		event_str = "CMDQ_EVENT_MUTEX0_STREAM_EOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_MUTEX1_STREAM_EOF])
		event_str = "CMDQ_EVENT_MUTEX1_STREAM_EOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_MUTEX2_STREAM_EOF])
		event_str = "CMDQ_EVENT_MUTEX2_STREAM_EOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_MUTEX3_STREAM_EOF])
		event_str = "CMDQ_EVENT_MUTEX3_STREAM_EOF";
	else if (event == cmdq_event_value[CMDQ_EVENT_MUTEX4_STREAM_EOF])
		event_str = "CMDQ_EVENT_MUTEX4_STREAM_EOF";
	/* Display underrun events */
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_RDMA0_UNDERRUN])
		event_str = "CMDQ_EVENT_DISP_RDMA0_UNDERRUN";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_RDMA1_UNDERRUN])
		event_str = "CMDQ_EVENT_DISP_RDMA1_UNDERRUN";
	else if (event == cmdq_event_value[CMDQ_EVENT_DISP_RDMA2_UNDERRUN])
		event_str = "CMDQ_EVENT_DISP_RDMA2_UNDERRUN";
	else
		event_str = "UNKNOWN";

	dev_err(dev, "0x%08x 0x%016llx %s event %s\n", offset, cmd,
		cmdq_command_is_wfe(cmd) ?  "wait for" : "clear", event_str);
}

static void cmdq_buf_print_mask(struct device *dev, u32 offset, u64 cmd)
{
	u32 arg_b = (u32)(cmd & 0xffffffff);

	dev_err(dev, "0x%08x 0x%016llx mask 0x%08x\n",
		offset, cmd, ~arg_b);
}

static void cmdq_buf_print_misc(struct device *dev, u32 offset, u64 cmd)
{
	enum cmdq_code op = (enum cmdq_code)(cmd >> (32 + CMDQ_OP_CODE_SHIFT));
	char *cmd_str;

	switch (op) {
	case CMDQ_CODE_JUMP:
		cmd_str = "jump";
		break;
	case CMDQ_CODE_EOC:
		cmd_str = "eoc";
		break;
	default:
		cmd_str = "unknown";
		break;
	}

	dev_err(dev, "0x%08x 0x%016llx %s\n", offset, cmd, cmd_str);
}

static void cmdq_buf_dump_work(struct work_struct *work_item)
{
	struct cmdq_buf_dump *buf_dump = container_of(work_item,
			struct cmdq_buf_dump, dump_work);
	struct device *dev = buf_dump->cmdq->mbox.dev;
	u64 *buf = buf_dump->cmd_buf;
	enum cmdq_code op;
	u32 i, offset = 0;

	dev_err(dev, "dump %s task start ----------\n",
		buf_dump->timeout ? "timeout" : "error");
	dev_err(dev, "pa_curr - pa_base = 0x%x", buf_dump->pa_offset);
	for (i = 0; i < CMDQ_NUM_CMD(buf_dump); i++) {
		op = (enum cmdq_code)(buf[i] >> (32 + CMDQ_OP_CODE_SHIFT));
		switch (op) {
		case CMDQ_CODE_WRITE:
			cmdq_buf_print_write(dev, offset, buf[i]);
			break;
		case CMDQ_CODE_WFE:
			cmdq_buf_print_wfe(dev, offset, buf[i]);
			break;
		case CMDQ_CODE_MASK:
			cmdq_buf_print_mask(dev, offset, buf[i]);
			break;
		default:
			cmdq_buf_print_misc(dev, offset, buf[i]);
			break;
		}
		offset += CMDQ_INST_SIZE;
	}
	dev_err(dev, "dump %s task end   ----------\n",
		buf_dump->timeout ? "timeout" : "error");

	kfree(buf_dump->cmd_buf);
	kfree(buf_dump);
}

static void cmdq_buf_dump_schedule(struct cmdq_task *task, bool timeout,
				   u32 pa_curr)
{
	struct device *dev = task->cmdq->mbox.dev;
	struct cmdq_buf_dump *buf_dump;

	buf_dump = kmalloc(sizeof(*buf_dump), GFP_ATOMIC);
	buf_dump->cmdq = task->cmdq;
	buf_dump->timeout = timeout;
	buf_dump->cmd_buf = kmalloc(task->pkt->cmd_buf_size, GFP_ATOMIC);
	buf_dump->cmd_buf_size = task->pkt->cmd_buf_size;
	buf_dump->pa_offset = pa_curr - task->pa_base;
	dma_sync_single_for_cpu(dev, task->pa_base,
				task->pkt->cmd_buf_size, DMA_TO_DEVICE);
	memcpy(buf_dump->cmd_buf, task->pkt->va_base, task->pkt->cmd_buf_size);
	dma_sync_single_for_device(dev, task->pa_base,
				   task->pkt->cmd_buf_size, DMA_TO_DEVICE);
	INIT_WORK(&buf_dump->dump_work, cmdq_buf_dump_work);
	queue_work(task->cmdq->buf_dump_wq, &buf_dump->dump_work);
}

static void cmdq_task_handle_error(struct cmdq_task *task, u32 pa_curr)
{
	struct cmdq_thread *thread = task->thread;
	struct cmdq_task *next_task;

	dev_err(task->cmdq->mbox.dev, "task 0x%p error\n", task);
	cmdq_buf_dump_schedule(task, false, pa_curr);
	WARN_ON(cmdq_thread_suspend(task->cmdq, thread) < 0);
	next_task = list_first_entry_or_null(&thread->task_busy_list,
			struct cmdq_task, list_entry);
	if (next_task)
		writel(next_task->pa_base, thread->base + CMDQ_THR_CURR_ADDR);
	cmdq_thread_resume(thread);
}

static void cmdq_thread_irq_handler(struct cmdq *cmdq,
				    struct cmdq_thread *thread)
{
	struct cmdq_task *task, *tmp, *curr_task = NULL;
	u32 curr_pa, irq_flag, task_end_pa;
	bool err;

	irq_flag = readl(thread->base + CMDQ_THR_IRQ_STATUS);
	writel(~irq_flag, thread->base + CMDQ_THR_IRQ_STATUS);

	/*
	 * When ISR call this function, another CPU core could run
	 * "release task" right before we acquire the spin lock, and thus
	 * reset / disable this GCE thread, so we need to check the enable
	 * bit of this GCE thread.
	 */
	if (!(readl(thread->base + CMDQ_THR_ENABLE_TASK) & CMDQ_THR_ENABLED))
		return;

	if (irq_flag & CMDQ_THR_IRQ_ERROR)
		err = true;
	else if (irq_flag & CMDQ_THR_IRQ_DONE)
		err = false;
	else
		return;

	curr_pa = readl(thread->base + CMDQ_THR_CURR_ADDR);

	list_for_each_entry_safe(task, tmp, &thread->task_busy_list,
				 list_entry) {
		task_end_pa = task->pa_base + task->pkt->cmd_buf_size;
		if (curr_pa >= task->pa_base && curr_pa < task_end_pa)
			curr_task = task;

		if (!curr_task || curr_pa == task_end_pa - CMDQ_INST_SIZE) {
			cmdq_task_exec_done(task, false);
			kfree(task);
		} else if (err) {
			cmdq_task_exec_done(task, true);
			cmdq_task_handle_error(curr_task, curr_pa);
			kfree(task);
		}

		if (curr_task)
			break;
	}

	if (list_empty(&thread->task_busy_list)) {
		cmdq_thread_disable(cmdq, thread);
		clk_disable(cmdq->clock);
	} else {
		mod_timer(&thread->timeout,
			  jiffies + msecs_to_jiffies(CMDQ_TIMEOUT_MS));
	}
}

static irqreturn_t cmdq_irq_handler(int irq, void *dev)
{
	struct cmdq *cmdq = dev;
	unsigned long irq_status, flags = 0L;
	int bit;

	irq_status = readl(cmdq->base + CMDQ_CURR_IRQ_STATUS) & CMDQ_IRQ_MASK;
	if (!(irq_status ^ CMDQ_IRQ_MASK))
		return IRQ_NONE;

	for_each_clear_bit(bit, &irq_status, fls(CMDQ_IRQ_MASK)) {
		struct cmdq_thread *thread = &cmdq->thread[bit];

		spin_lock_irqsave(&thread->chan->lock, flags);
		cmdq_thread_irq_handler(cmdq, thread);
		spin_unlock_irqrestore(&thread->chan->lock, flags);
	}
	return IRQ_HANDLED;
}

static void cmdq_thread_handle_timeout(unsigned long data)
{
	struct cmdq_thread *thread = (struct cmdq_thread *)data;
	struct cmdq *cmdq = container_of(thread->chan->mbox, struct cmdq, mbox);
	struct cmdq_task *task, *tmp;
	unsigned long flags;
	bool first_task = true;

	spin_lock_irqsave(&thread->chan->lock, flags);
	WARN_ON(cmdq_thread_suspend(cmdq, thread) < 0);

	/*
	 * Although IRQ is disabled, GCE continues to execute.
	 * It may have pending IRQ before GCE thread is suspended,
	 * so check this condition again.
	 */
	cmdq_thread_irq_handler(cmdq, thread);

	if (list_empty(&thread->task_busy_list)) {
		cmdq_thread_resume(thread);
		spin_unlock_irqrestore(&thread->chan->lock, flags);
		return;
	}

	dev_err(cmdq->mbox.dev, "timeout\n");
	list_for_each_entry_safe(task, tmp, &thread->task_busy_list,
				 list_entry) {
		if (first_task) {
			cmdq_buf_dump_schedule(task, true, readl(
					thread->base + CMDQ_THR_CURR_ADDR));
			first_task = false;
		}
		cmdq_task_exec_done(task, true);
		kfree(task);
	}

	cmdq_thread_resume(thread);
	cmdq_thread_disable(cmdq, thread);
	clk_disable(cmdq->clock);
	spin_unlock_irqrestore(&thread->chan->lock, flags);
}

static int cmdq_suspend(struct device *dev)
{
	struct cmdq *cmdq = dev_get_drvdata(dev);
	struct cmdq_thread *thread;
	int i;
	bool task_running = false;

	cmdq->suspended = true;

	for (i = 0; i < ARRAY_SIZE(cmdq->thread); i++) {
		thread = &cmdq->thread[i];
		if (!list_empty(&thread->task_busy_list)) {
			mod_timer(&thread->timeout, jiffies + 1);
			task_running = true;
			break;
		}
	}

	if (task_running) {
		dev_warn(dev, "exist running task(s) in suspend\n");
		schedule();
	}

	clk_unprepare(cmdq->clock);
	return 0;
}

static int cmdq_resume(struct device *dev)
{
	struct cmdq *cmdq = dev_get_drvdata(dev);

	WARN_ON(clk_prepare(cmdq->clock) < 0);
	cmdq->suspended = false;
	return 0;
}

static int cmdq_remove(struct platform_device *pdev)
{
	struct cmdq *cmdq = platform_get_drvdata(pdev);

	destroy_workqueue(cmdq->buf_dump_wq);
	mbox_controller_unregister(&cmdq->mbox);
	clk_unprepare(cmdq->clock);
	return 0;
}

static int cmdq_mbox_send_data(struct mbox_chan *chan, void *data)
{
	cmdq_task_exec(data, chan->con_priv);
	return 0;
}

static int cmdq_mbox_startup(struct mbox_chan *chan)
{
	return 0;
}

static void cmdq_mbox_shutdown(struct mbox_chan *chan)
{
}

static bool cmdq_mbox_last_tx_done(struct mbox_chan *chan)
{
	return true;
}

static const struct mbox_chan_ops cmdq_mbox_chan_ops = {
	.send_data = cmdq_mbox_send_data,
	.startup = cmdq_mbox_startup,
	.shutdown = cmdq_mbox_shutdown,
	.last_tx_done = cmdq_mbox_last_tx_done,
};

static struct mbox_chan *cmdq_xlate(struct mbox_controller *mbox,
		const struct of_phandle_args *sp)
{
	int ind = sp->args[0];
	struct cmdq_thread *thread;

	if (ind >= mbox->num_chans)
		return ERR_PTR(-EINVAL);

	thread = mbox->chans[ind].con_priv;
	thread->atomic_exec = (sp->args[1] != 0);
	thread->chan = &mbox->chans[ind];

	return &mbox->chans[ind];
}

static int cmdq_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct cmdq *cmdq;
	int err, i;

	cmdq = devm_kzalloc(dev, sizeof(*cmdq), GFP_KERNEL);
	if (!cmdq)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	cmdq->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(cmdq->base)) {
		dev_err(dev, "failed to ioremap gce\n");
		return PTR_ERR(cmdq->base);
	}

	cmdq->irq = platform_get_irq(pdev, 0);
	if (!cmdq->irq) {
		dev_err(dev, "failed to get irq\n");
		return -EINVAL;
	}
	err = devm_request_irq(dev, cmdq->irq, cmdq_irq_handler, IRQF_SHARED,
			       "mtk_cmdq", cmdq);
	if (err < 0) {
		dev_err(dev, "failed to register ISR (%d)\n", err);
		return err;
	}

	dev_dbg(dev, "cmdq device: addr:0x%p, va:0x%p, irq:%d\n",
		dev, cmdq->base, cmdq->irq);

	cmdq->clock = devm_clk_get(dev, "gce");
	if (IS_ERR(cmdq->clock)) {
		dev_err(dev, "failed to get gce clk\n");
		return PTR_ERR(cmdq->clock);
	}

	cmdq->mbox.dev = dev;
	cmdq->mbox.chans = devm_kcalloc(dev, CMDQ_THR_MAX_COUNT,
					sizeof(*cmdq->mbox.chans), GFP_KERNEL);
	if (!cmdq->mbox.chans)
		return -ENOMEM;

	cmdq->mbox.num_chans = CMDQ_THR_MAX_COUNT;
	cmdq->mbox.ops = &cmdq_mbox_chan_ops;
	cmdq->mbox.of_xlate = cmdq_xlate;

	/* make use of TXDONE_BY_ACK */
	cmdq->mbox.txdone_irq = false;
	cmdq->mbox.txdone_poll = false;

	for (i = 0; i < ARRAY_SIZE(cmdq->thread); i++) {
		cmdq->thread[i].base = cmdq->base + CMDQ_THR_BASE +
				CMDQ_THR_SIZE * i;
		INIT_LIST_HEAD(&cmdq->thread[i].task_busy_list);
		init_timer(&cmdq->thread[i].timeout);
		cmdq->thread[i].timeout.function = cmdq_thread_handle_timeout;
		cmdq->thread[i].timeout.data = (unsigned long)&cmdq->thread[i];
		cmdq->mbox.chans[i].con_priv = &cmdq->thread[i];
	}

	err = mbox_controller_register(&cmdq->mbox);
	if (err < 0) {
		dev_err(dev, "failed to register mailbox: %d\n", err);
		return err;
	}

	cmdq->buf_dump_wq = alloc_ordered_workqueue(
			"%s", WQ_MEM_RECLAIM | WQ_HIGHPRI,
			"cmdq_buf_dump");

	platform_set_drvdata(pdev, cmdq);
	WARN_ON(clk_prepare(cmdq->clock) < 0);

	return 0;
}

static const struct dev_pm_ops cmdq_pm_ops = {
	.suspend = cmdq_suspend,
	.resume = cmdq_resume,
};

static const struct of_device_id cmdq_of_ids[] = {
	{.compatible = "mediatek,mt8173-gce",},
	{}
};

static struct platform_driver cmdq_drv = {
	.probe = cmdq_probe,
	.remove = cmdq_remove,
	.driver = {
		.name = "mtk_cmdq",
		.pm = &cmdq_pm_ops,
		.of_match_table = cmdq_of_ids,
	}
};

builtin_platform_driver(cmdq_drv);
