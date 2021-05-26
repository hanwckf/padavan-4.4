/**
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
**/
#include "inc/mtk_ir_core.h"
#include "inc/mtk_ir_common.h"
#include "inc/mtk_ir_dev.h"

#define IR_DEV_MINOR   MISC_DYNAMIC_MINOR

struct mtk_ir_dev_usr_list	/* every user process has one list */
{
	struct list_head list;
	struct kfifo fifo_irmsg;
	spinlock_t fifo_lock;
	struct semaphore sem;	/* for same process open */
	wait_queue_head_t wait_poll;
	char name[16];		/* process name */
	pid_t pid;		/* process pid */
	pid_t tgid;		/* process tgid */
};

static struct mtk_ir_msg msg;
#define SCANCODE_SIZE sizeof(msg.scancode)
#define SCANCODE_COUNT 4

#define MESSAGE_SIZE MTK_IR_CHUNK_SIZE
#define MESSAGE_COUNT 4

static LIST_HEAD(head_ir_usr_list);

static atomic_t mtk_dev_open_num = ATOMIC_INIT(0);
/* static struct semaphore sem_list; //  head_ir_usr_list  sema */
static struct task_struct *k_thread;
static struct kfifo fifo_scancode;	/* store scancde */

static DEFINE_SPINLOCK(fifo_scancode_lock);
static DEFINE_SEMAPHORE(sem_list);
static void mtk_ir_dev_add_usr_list(struct mtk_ir_dev_usr_list *p_listadd);
static void mtk_ir_dev_remove_usr_list(struct mtk_ir_dev_usr_list *p_listremove);
static void mtk_ir_dev_free_usr_list(void);

void mtk_ir_dev_put_scancode(u32 ui4scancode)
{
	unsigned long __flags;
	unsigned int __ret;

	if (atomic_read(&mtk_dev_open_num) == 0)	/* no user open, so ignore key */
		return;

	spin_lock_irqsave(&fifo_scancode_lock, __flags);
	__ret = kfifo_is_full(&fifo_scancode);
	spin_unlock_irqrestore(&fifo_scancode_lock, __flags);

	if (__ret) {		/* can put key */
		MTK_IR_LOG("fifo_scancode buffer full !!!!\n");
		return;
	}

	kfifo_in_locked(&fifo_scancode, (void *)&ui4scancode,
					SCANCODE_SIZE, &fifo_scancode_lock);	/* put scan code */
	MTK_IR_LOG("mtk_ir_dev_put_scancode fifo_scancode:0x%08x\n", ui4scancode);

	if (unlikely(k_thread == NULL))
		MTK_IR_ERR("mtk_ir_dev_thread not running !!!\n");
	else {
		MTK_IR_LOG("wake up mtk_ir_dev_thread!\n");
		wake_up_process(k_thread);	/* mtk_ir_dev_thread */
	}
}

void mtk_ir_dev_add_usr_list(struct mtk_ir_dev_usr_list *p_listadd)
{
	ASSERT(p_listadd != NULL);
	MTK_IR_LOG(" begin\n");

	down(&sem_list);	/* when modify list,we must lock sema */
	atomic_inc(&mtk_dev_open_num);
	list_add(&(p_listadd->list), &head_ir_usr_list);
	up(&sem_list);

	MTK_IR_LOG("success num =%d\n ", atomic_read(&mtk_dev_open_num));
}

void mtk_ir_dev_remove_usr_list(struct mtk_ir_dev_usr_list *p_listremove)
{

	int num;

	ASSERT(p_listremove != NULL);
	MTK_IR_LOG(" begin\n");

	down(&sem_list);	/* when modify list,we must lock sema */
	list_del(&(p_listremove->list));
	atomic_dec(&mtk_dev_open_num);

	kfifo_free(&(p_listremove->fifo_irmsg));
	up(&(p_listremove->sem));
	kfree(p_listremove);
	up(&sem_list);

	num = atomic_read(&mtk_dev_open_num);
	if (!num)
		mtk_ir_set_log_to(0);

	MTK_IR_LOG(" success num = %d\n ", atomic_read(&mtk_dev_open_num));
}


void mtk_ir_dev_free_usr_list(void)
{
	struct list_head *list_cursor;
	struct mtk_ir_dev_usr_list *entry;
	int num;

	MTK_IR_TRD_LOG("mtk_ir_dev_free_usr_list  begin\n");
	down(&sem_list);	/* when modify list,we must lock sema */
	if (!list_empty(&head_ir_usr_list)) {
		list_for_each(list_cursor, &head_ir_usr_list) {
			entry = list_entry(list_cursor, struct mtk_ir_dev_usr_list, list);
			list_del(list_cursor);
			kfifo_free(&(entry->fifo_irmsg));
			kfree(entry);
			atomic_dec(&mtk_dev_open_num);
		}
	}
	up(&sem_list);

	num = atomic_read(&mtk_dev_open_num);
	if (!num)
		mtk_ir_set_log_to(0);

	MTK_IR_TRD_LOG(" success num = %d\n ", atomic_read(&mtk_dev_open_num));
}

static int mtk_ir_dev_open(struct inode *inode, struct file *file)
{
	int ret;
	struct mtk_ir_dev_usr_list *newlist;

	newlist = kzalloc(sizeof(struct mtk_ir_dev_usr_list), GFP_KERNEL);
	if (!newlist)
		return -ENOMEM;

	spin_lock_init(&(newlist->fifo_lock));
	ret = kfifo_alloc(&(newlist->fifo_irmsg), MESSAGE_SIZE * MESSAGE_COUNT, GFP_KERNEL);
	if (ret) {
		kfree(newlist);
		return -ENOMEM;
	}
	kfifo_reset(&(newlist->fifo_irmsg));
	sema_init(&(newlist->sem), SEMA_STATE_UNLOCK);
	INIT_LIST_HEAD(&(newlist->list));
	init_waitqueue_head(&(newlist->wait_poll));
	memcpy(newlist->name, current->comm, 16);
	newlist->pid = current->pid;
	newlist->tgid = current->tgid;
	file->private_data = newlist;	/* record this private data */

	nonseekable_open(inode, file);
	mtk_ir_dev_add_usr_list(newlist);

	MTK_IR_LOG("open by proc(%s) pid(%d) tgid(%d) success\n",
		   current->comm, current->pid, current->tgid);
	return 0;
}


static ssize_t mtk_ir_dev_read(struct file *file, char *buffer, size_t length, loff_t *ppos)
{
	int ret = 0, copy = 0;
	unsigned long __flags;
	unsigned int __ret;
	struct mtk_ir_dev_usr_list *p_list = file->private_data;
	struct kfifo *fifo_irmsg = &(p_list->fifo_irmsg);
	struct semaphore *sem = &(p_list->sem);
	struct mtk_ir_msg ir_data;
	wait_queue_head_t *wait_poll = &(p_list->wait_poll);
	spinlock_t *fifo_lock = &(p_list->fifo_lock);

	DECLARE_WAITQUEUE(wait, current);

	if (down_interruptible(sem)) {
		ret = -ERESTARTSYS;	/* signal interrupt */
		MTK_IR_LOG("down_interruptible(sem) fail!!!\n");
		goto out_unlocked;
	}

	if (length != MTK_IR_CHUNK_SIZE) {
		ret = -EINVAL;
		goto out_locked;
	}
	add_wait_queue(wait_poll, &wait);
	set_current_state(TASK_INTERRUPTIBLE);

	while ((copy == 0) && (ret == 0)) {
		MTK_IR_TRD_LOG("begin spin_lock_irqsave!!!\n");
		spin_lock_irqsave(fifo_lock, __flags);
		__ret = kfifo_is_empty(fifo_irmsg);
		spin_unlock_irqrestore(fifo_lock, __flags);
		MTK_IR_TRD_LOG("end spin_lock_irqsave!!!\n");

		if (__ret) {	/* fifo empty */
			if (file->f_flags & O_NONBLOCK) {	/* NONBLOCK OPEN */
				ret = -EWOULDBLOCK;
				break;
			}
			if (signal_pending(current)) {
				MTK_IR_LOG(" signal_pending !!!\n");
				ret = -ERESTARTSYS;
				break;
			}

			up(sem);	/* for the same process ,but diffrent thread's read */
			MTK_IR_TRD_LOG("begin schedule !!!\n");
			schedule();	/* sleep */
			MTK_IR_TRD_LOG("end schedule !!!\n");
			/* has been wake up */

			set_current_state(TASK_INTERRUPTIBLE);	/* for next time use */

			if (down_interruptible(sem)) {	/* prevent same process,s thread */
				ret = -ERESTARTSYS;
				MTK_IR_LOG(" down_interruptible !!!\n");
				remove_wait_queue(&(p_list->wait_poll), &wait);
				set_current_state(TASK_RUNNING);
				goto out_unlocked;
			}
		} else {
			ret = kfifo_out_locked(fifo_irmsg, &ir_data, MTK_IR_CHUNK_SIZE, fifo_lock);

			ret = copy_to_user((void *)buffer, &ir_data, MTK_IR_CHUNK_SIZE);
			if (!ret)
				copy = MTK_IR_CHUNK_SIZE;
			else
				ret = -EFAULT;
		}
	}

	remove_wait_queue(wait_poll, &wait);
	set_current_state(TASK_RUNNING);
	MTK_IR_LOG("copy to (%s pid = %d) scancode(0X%08X), keycode(0X%08X),\n",
		   p_list->name, p_list->pid, ir_data.scancode, ir_data.keycode);

out_locked:
	up(sem);

out_unlocked:
	return ret ? ret : copy;
}

static int mtk_ir_dev_release(struct inode *inode, struct file *file)
{
	struct mtk_ir_dev_usr_list *p_list = file->private_data;

	mtk_ir_dev_remove_usr_list(p_list);
	return 0;
}

static unsigned int mtk_ir_dev_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	struct mtk_ir_dev_usr_list *p_list = file->private_data;
	struct kfifo *fifo_irmsg = &(p_list->fifo_irmsg);
	spinlock_t *fifo_lock = &(p_list->fifo_lock);
	unsigned long __flags;
	unsigned int __ret;

	poll_wait(file, &(p_list->wait_poll), wait);

	if (signal_pending(current)) {
		MTK_IR_LOG("mtk_ir_dev_poll INTERRUPT!!!\n");
		return -ERESTARTSYS;
	}

	spin_lock_irqsave(fifo_lock, __flags);
	__ret = kfifo_is_empty(fifo_irmsg);
	spin_unlock_irqrestore(fifo_lock, __flags);
	if (!__ret)	/* has data to read */
		mask |= (POLLIN | POLLRDNORM);

	return mask;
}

static long mtk_ir_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	u32 code;
	int ret = 0;
	int size = IR_NETLINK_MSG_SIZE;
	struct mtk_ir_context *cxt = NULL;
	struct rc_dev *rcdev = NULL;	/* mtk_ir_context_obj->rcdev; */

	cxt = mtk_ir_context_obj;
	rcdev = cxt->rcdev;

	switch (cmd) {
	case CMD_IR_SEND_SCANCODE:
		code = (u32) arg;
		mtk_ir_core_send_scancode(code);	/* for /dev/lirc and input_dev and /dev/ir_dev */
		break;

	case CMD_IR_SEND_MAPCODE:
		code = (int)arg;
		mtk_ir_core_send_mapcode_auto_up(code, 250);
		break;

	case CMD_IR_GET_UNIFY:
		if (rcdev == NULL) {
			MTK_IR_LOG("rc dev not initial!!\n");
			ret = -1;
			break;
		}
		ret = copy_to_user((void *)arg,
				   (void *)&(rcdev->input_id), sizeof(struct input_id));
		break;

	case CMD_IR_GET_DEVNAME:
		if (rcdev == NULL) {
			MTK_IR_LOG("rc dev not initial!!\n");
			ret = -1;
			break;
		}
		ret = copy_to_user((void *)arg,
				   (void *)(rcdev->input_name), strlen(rcdev->input_name) + 1);
		break;

	case CMD_IR_NETLINK_MSG_SIZE:
		ret = copy_to_user((void *)arg, (void *)&size, sizeof(size));
		break;

	default:
		break;
	}
	return ret;
}


static int mtk_ir_dev_thread(void *pvArg)
{
	struct list_head *list_cursor;
	struct mtk_ir_dev_usr_list *entry;
	u32 scancode;
	struct mtk_ir_msg msg_temp;
	unsigned int _ret;
	unsigned long _flags;

	while (!kthread_should_stop()) {
		MTK_IR_TRD_LOG(" mtk_ir_dev_thread begin !!\n");
		set_current_state(TASK_INTERRUPTIBLE);

		spin_lock_irqsave(&fifo_scancode_lock, _flags);
		_ret = kfifo_is_empty(&fifo_scancode);	/* check whether fifi is empty */
		spin_unlock_irqrestore(&fifo_scancode_lock, _flags);
		MTK_IR_TRD_LOG(" dev schedule() >>>>\n");
		if (_ret)	/* does not have key */
			schedule();
		MTK_IR_TRD_LOG(" dev schedule() <<<<\n");

		set_current_state(TASK_RUNNING);
		if (kthread_should_stop())	/* other place want to stop this thread; */
			continue;

		_ret = kfifo_out_locked(&fifo_scancode, &scancode, sizeof(scancode),
								&fifo_scancode_lock);	/* get scancode */
		MTK_IR_TRD_LOG(" fifo_scancode:0x%08x\n", scancode);
		if (_ret == 0)	/* does not have key */
			continue;

		if (atomic_read(&mtk_dev_open_num) == 0) {	/* no user open, so ignore key */
			MTK_IR_TRD_LOG("no user, ignore key !!!\n");
			continue;
		}

		ASSERT(_ret == sizeof(scancode));

		mtk_ir_core_get_msg_by_scancode(scancode, &msg_temp);
		MTK_IR_TRD_LOG("scancode:0x%08x, keycode:0x%08x\n", msg_temp.scancode,
			     msg_temp.keycode);

		down(&sem_list);
		MTK_IR_TRD_LOG("mtk_ir_dev_thread end sem_list\n");

		list_for_each(list_cursor, &head_ir_usr_list) {
			entry = list_entry(list_cursor, struct mtk_ir_dev_usr_list, list);
			spin_lock_irqsave(&(entry->fifo_lock), _flags);
			_ret = kfifo_is_full(&(entry->fifo_irmsg));
			spin_unlock_irqrestore(&(entry->fifo_lock), _flags);

			if (_ret)	/* can  not put key, buffer is full */
				MTK_IR_TRD_LOG(" process(%s) pid (%d) buffer full!!\n", entry->name,
					     entry->pid);
			else
				kfifo_in_locked(&(entry->fifo_irmsg), (void *)&msg_temp,
						MESSAGE_SIZE, &(entry->fifo_lock));

			MTK_IR_TRD_LOG("pro(%s) pid (%d) kfifo_len(%d) !!\n",
				     entry->name, entry->pid, kfifo_len(&(entry->fifo_irmsg)));
			wake_up_interruptible(&(entry->wait_poll));
		}
		up(&sem_list);

	}

	return 0;
}

void mtk_ir_dev_show_info(char **pbuf, int *plen)
{
	struct list_head *list_cursor;
	struct mtk_ir_dev_usr_list *entry;
	int temp_len;

	down(&sem_list);	/* when modify list,we must lock sema */
	if (!list_empty(&head_ir_usr_list)) {
		list_for_each(list_cursor, &head_ir_usr_list) {
			entry = list_entry(list_cursor, struct mtk_ir_dev_usr_list, list);
			if (!pbuf && !plen)
				MTK_IR_LOG("open by proc(%s) pid(%d) tgid(%d)\n",
					   entry->name, entry->pid, entry->tgid);
			else {
				temp_len = sprintf(*pbuf, "open by proc(%s) pid(%d) tgid(%d)\n",
						   entry->name, entry->pid, entry->tgid);
				*pbuf += temp_len;
				*plen += temp_len;
			}
		}
	}
	up(&sem_list);

	if (!pbuf && !plen)
		MTK_IR_LOG("opened num = %d\n ", atomic_read(&mtk_dev_open_num));
	else {
		temp_len = sprintf(*pbuf, "opened num = %d\n ", atomic_read(&mtk_dev_open_num));
		*pbuf += temp_len;
		*plen += temp_len;
	}
}

static const struct file_operations ir_fops_dev = {
	.owner = THIS_MODULE,
	.read = mtk_ir_dev_read,
	.open = mtk_ir_dev_open,
	.release = mtk_ir_dev_release,
	.poll = mtk_ir_dev_poll,
	.unlocked_ioctl = mtk_ir_dev_ioctl,
};

static struct miscdevice ir_misc_dev = {
	.minor = IR_DEV_MINOR,
	.name = IR_NODE_NAME,
	.fops = &ir_fops_dev,

};

int __init mtk_ir_dev_init(void)
{
	int ret;

	MTK_IR_FUN();

	ret = misc_register(&ir_misc_dev);
	ret = kfifo_alloc(&(fifo_scancode), SCANCODE_SIZE * SCANCODE_COUNT, GFP_KERNEL);
	if (ret) {
		MTK_IR_ERR(" fifo_scancode alloc fail!!!\n");
		ret = -ENOMEM;
		goto fifo_alloc_fail;

	}
	kfifo_reset(&(fifo_scancode));

	ret = mtk_ir_core_create_thread(mtk_ir_dev_thread, NULL, "mtk_ir_dev_thread",
									&k_thread, 94);	/*RTPM_PRIO_SCRN_UPDATE */

	if (ret) {
		MTK_IR_ERR(" create mtk_ir_dev_thread fail!!!\n");
		goto fail_thread;
	} else
		MTK_IR_LOG(" create mtk_ir_dev_thread[k_thread].\n");

	mtk_ir_netlink_init();

	MTK_IR_LOG("mtk_ir_dev_init----\n");
	return ret;

fail_thread:
	kfifo_free(&(fifo_scancode));
fifo_alloc_fail:
	misc_deregister(&ir_misc_dev);
	return ret;
}

void mtk_ir_dev_exit(void)
{
	MTK_IR_FUN();
	mtk_ir_netlink_exit();

	if (!k_thread)
		return;

	up(&sem_list);
	kthread_stop(k_thread);
	k_thread = NULL;

	mtk_ir_dev_free_usr_list();	/* free all list */
	kfifo_free(&(fifo_scancode));
	misc_deregister(&ir_misc_dev);
	MTK_IR_LOG("mtk_ir_dev_exit success\n");
}
