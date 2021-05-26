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
#include <net/genetlink.h>
#include "inc/mtk_ir_core.h"
#include "inc/mtk_ir_dev.h"

struct netlink_msgq {
	spinlock_t msgq_lock;
	u8 *read;
	u8 *write;
	u8 *end;
	u8 *start;
};


#define MESSAGE_MAX_QUEUE_SIZE  (IR_NETLINK_MSG_SIZE * 10)	/* 1024 */
/**
*message_head******************message_head****************** message_head
*			   |----message-----|                            |----message-----|
**/

char message[MESSAGE_MAX_QUEUE_SIZE] = { 0 };	/* here is used to store  kernel log; */

static struct task_struct *k_thread_netlink;
static struct netlink_msgq s_msg_q;
static atomic_t ir_log_to = ATOMIC_INIT(0);

void mtk_ir_set_log_to(int value)
{
	atomic_set(&ir_log_to, value);
}

int mtk_ir_get_log_to(void)
{
	return atomic_read(&ir_log_to);
}

static void mtk_ir_netlink_msg_q_init(void)
{

	spin_lock_init(&(s_msg_q.msgq_lock));
	s_msg_q.start = message;
	s_msg_q.read = s_msg_q.start;
	s_msg_q.write = s_msg_q.start;
	s_msg_q.end = &(message[MESSAGE_MAX_QUEUE_SIZE]);	/* be careful here */

	MTK_IR_LOG("s_msg_q.start 0x%p\n", s_msg_q.start);
	MTK_IR_LOG("s_msg_q.read 0x%p\n", s_msg_q.read);
	MTK_IR_LOG("s_msg_q.write 0x%p\n", s_msg_q.write);
	MTK_IR_LOG("s_msg_q.end  0x%p\n", s_msg_q.end);
}

#if 0
int mtk_ir_netlink_msg_q_send(unsigned char *pv_msg, int z_size)	/* include the "/0" */
{
	/* 37 */
	u8 *write;
	unsigned long __flags;

	if ((z_size <= 0) || (z_size >= MESSAGE_MAX_QUEUE_SIZE)) {
		MTK_IR_LOG("message size is %d, invalid size !!!\n ", z_size);
		return 0;
	}

	spin_lock_irqsave(&(s_msg_q.msgq_lock), __flags);
	write = s_msg_q.write + z_size - 1;

	if (write > s_msg_q.end) {	/* size is too long; */
		write = s_msg_q.start;

		MTK_IR_LOG(" buff (write-end)(%d), message_head(%d), need_size(%d)!!!\n ",
			   (s_msg_q.end - s_msg_q.write + 1), IR_NETLINK_MESSAGE_HEADER, z_size);
		if ((s_msg_q.end - s_msg_q.write + 1) < IR_NETLINK_MESSAGE_HEADER) {
			/* so the gap between s_msg_q.write to  s_msg_q.end  is too small to store a header */
		} else {
			struct message_head message = { MESSAGE_NONE, 0 };
			/* we store a header here to tell mtk_ir_netlink_msg_q_receive function , go to start */
			memcpy(s_msg_q.write, &message, IR_NETLINK_MESSAGE_HEADER);

		}

		if ((write + z_size - 1) >= s_msg_q.read) {
			MTK_IR_LOG(" buff (write-read)(%d),  need_size(%d)!!!\n ",
				   (s_msg_q.read - write), z_size);
			spin_unlock_irqrestore(&(s_msg_q.msgq_lock), __flags);
			return 0;
		}
		s_msg_q.write = s_msg_q.start;
		memcpy(s_msg_q.write, pv_msg, z_size);	/* copy message to buffer */
		s_msg_q.write += z_size;

	} else {
		memcpy(s_msg_q.write, pv_msg, z_size);	/* copy message to buffer */
		s_msg_q.write += z_size;
	}

	spin_unlock_irqrestore(&(s_msg_q.msgq_lock), __flags);
	wake_up_process(k_thread_netlink);
	return 0;
}
#endif

int mtk_ir_netlink_msg_q_send(unsigned char *pv_msg, int z_size)	/* include the "/0" */
{
	/* 37 */
	unsigned long __flags;

	spin_lock_irqsave(&(s_msg_q.msgq_lock), __flags);
	if (s_msg_q.write == s_msg_q.end) {
		spin_unlock_irqrestore(&(s_msg_q.msgq_lock), __flags);
		return 0;
	}

	memcpy(s_msg_q.write, pv_msg, z_size);	/* copy message to buffer */
	s_msg_q.write += IR_NETLINK_MSG_SIZE;

	spin_unlock_irqrestore(&(s_msg_q.msgq_lock), __flags);
	wake_up_process(k_thread_netlink);
	return 0;
}

#define IR_CMD_MAX (_IR_CMD_MAX - 1)

static struct genl_family ir_gnl_family = {
	.id = GENL_ID_GENERATE,
	.hdrsize = 0,
	.name = IR_GNL_FAMILY_NAME,
	.version = 1,
	.maxattr = IR_CMD_MAX,
};

static const struct nla_policy ir_gnl_policy[IR_CMD_MAX + 1] = {
	[IR_CMD_ATTR_MSG] = {.type = NLA_STRING},
	[IR_CMD_ATTR_LOG_TO] = {.type = NLA_U32},
};

static int ir_gnl_recv_doit(struct sk_buff *skb, struct genl_info *info)
{
	struct nlmsghdr *nlhdr;
	struct genlmsghdr *genlhdr;
	struct nlattr *nlah;
	char *string;
	u32 pid;
	int log_to;
	int nlalen;

	nlhdr = nlmsg_hdr(skb);
	genlhdr = nlmsg_data(nlhdr);
	nlah = genlmsg_data(genlhdr);
	pid = nlhdr->nlmsg_pid;
	nlalen = nla_len(nlah);

	MTK_IR_LOG("kernel_pid is %d\n", pid);

	if (info->attrs[IR_CMD_ATTR_MSG]) {/* here is message */
		string = nla_data(nlah);
		MTK_IR_LOG("string (%s), nlalen(%d)\n", string, nlalen);
		MTK_IR_LOG("%s\n", string);
	}

	if (info->attrs[IR_CMD_ATTR_LOG_TO]) {
		log_to = nla_get_u32(nlah);
		mtk_ir_set_log_to(log_to);
		MTK_IR_LOG("ir_log_to (%d), nlalen(%d)\n", log_to, nlalen);
	}
	return 0;
}

static struct genl_ops ir_gnl_ops[1] = {
	{
		.cmd = IR_CMD_SEND_MSG,
		.flags = 0,
		.policy = ir_gnl_policy,
		.doit = ir_gnl_recv_doit,
		.dumpit = NULL,
	}
};

static void print_skb(struct sk_buff *skb)
{
#if 0
	MTK_IR_LOG("--------\n");
	MTK_IR_LOG("head 0x%p\n", skb->head);
	MTK_IR_LOG("data 0x%p\n", skb->data);
	MTK_IR_LOG("tail 0x%p\n", skb->tail);
	MTK_IR_LOG("end  0x%p\n", skb->end);
#endif
}

static int mtk_ir_genl_msg_send_to_user(void *data, int len, pid_t pid)
{
	struct sk_buff *skb = NULL;
	size_t size;
	void *head;
	int ret;

	/* NLA_HDRLEN + len + pad */
	size = nla_total_size(len);	/*total length of attribute including padding */

	/* create a new netlink msg */
	/*NLMSG_HDRLEN + GENL_HDRLEN + NLA_HDRLEN + len + pad+pad +pad */
	skb = genlmsg_new(size, GFP_KERNEL);	/* add genmsg_hdr */
	if (!skb) {
		MTK_IR_LOG("genlmsg_new fail !!!\n");
		return -ENOMEM;
	}

	print_skb(skb);

	genlmsg_put(skb, 0, 0, &ir_gnl_family, 0, IR_CMD_SEND_MSG);

	print_skb(skb);

	ret = nla_put(skb, IR_CMD_ATTR_MSG, len, data);

	if (ret) {
		MTK_IR_LOG(" fail, nla_put fail ret(%d)\n", ret);
		goto nlfail;
	}
	head = genlmsg_data(nlmsg_data(nlmsg_hdr(skb)));
	genlmsg_end(skb, head);
	print_skb(skb);
	/* genlmsg_unicast(&init_net,skb,pid); */
	genlmsg_multicast(&ir_gnl_family, skb, 0, IR_NETLINK_GROUP, GFP_KERNEL);

	print_skb(skb);
	return ret;

nlfail:
	nlmsg_free(skb);
	return ret;

	/* kfree_skb(skb); */
	/* skb = NULL; */
}

#if 0
static int mtk_ir_netlink_thread(void *pvArg)
{
	struct message_head *phead = NULL;
	struct message_head head;
	int ret;
	unsigned long __flags;

	while (!kthread_should_stop()) {
		MTK_IR_LOG("mtk_ir_netlink_thread begin\n");
		set_current_state(TASK_INTERRUPTIBLE);

		spin_lock_irqsave(&(s_msg_q.msgq_lock), __flags);
		ret = (s_msg_q.read == s_msg_q.write);
		spin_unlock_irqrestore(&(s_msg_q.msgq_lock), __flags);

		if (ret)	/* null data */
			schedule();	/*  */

		set_current_state(TASK_RUNNING);

		if (kthread_should_stop())	/* other place want to stop this thread; */
			continue;

		spin_lock_irqsave(&(s_msg_q.msgq_lock), __flags);
		/* read near buffer end, */

		MTK_IR_LOG("s_msg_q.read 0x%p\n", s_msg_q.read);
		if ((s_msg_q.end - s_msg_q.read + 1) < IR_NETLINK_MESSAGE_HEADER)
			s_msg_q.read = s_msg_q.start;

		phead = (struct message_head *)(s_msg_q.read);

		MTK_IR_LOG("!!!!!!!!!!!!!!!!!!!!!!!!!!--1\n");
		MTK_IR_LOG("s_msg_q.start 0x%p\n", s_msg_q.start);
		MTK_IR_LOG("s_msg_q.read 0x%p\n", s_msg_q.read);
		MTK_IR_LOG("s_msg_q.write 0x%p\n", s_msg_q.write);
		MTK_IR_LOG("s_msg_q.end  0x%p\n", s_msg_q.end);
		MTK_IR_LOG("head 0x%p\n", phead);
		head.message_type = phead->message_type;
		head.message_size = phead->message_size;


		if ((head.message_type == MESSAGE_NONE) && (head.message_size == 0)) {
			s_msg_q.read = s_msg_q.start;
			phead = (struct message_head *)(s_msg_q.read);
			head.message_type = phead->message_type;
			head.message_size = phead->message_size;

			MTK_IR_LOG("head 0x%p\n", phead);
			MTK_IR_LOG("head0 0x%p\n", phead);
			MTK_IR_LOG("head1 0x%p\n", phead);
		}

		MTK_IR_LOG("!!!!!!!!!!!!!!!!!!!!!!!!!!--2\n");

		MTK_IR_LOG("read message_type(%d), message_size(%d)\n", head.message_type,
			   head.message_size);
		MTK_IR_LOG("read message: %s\n", s_msg_q.read + IR_NETLINK_MESSAGE_HEADER);
		spin_unlock_irqrestore(&(s_msg_q.msgq_lock), __flags);
		mtk_ir_genl_msg_send_to_user(s_msg_q.read,
					     (head.message_size + IR_NETLINK_MESSAGE_HEADER), 0);

		s_msg_q.read += (head.message_size + IR_NETLINK_MESSAGE_HEADER);
		MTK_IR_LOG("head->message_size(%d)!!!!\n", head.message_size);
		MTK_IR_LOG("s_msg_q.read_last 0x%p !!!!\n", s_msg_q.read);

	}
	return 0;
}
#endif

#if 1
static int mtk_ir_netlink_thread(void *pvArg)
{
	/* struct message_head *phead = NULL; */
	struct message_head head;
	int ret;
	unsigned long __flags;
	char buff[IR_NETLINK_MSG_SIZE] = { 0 };

	while (!kthread_should_stop()) {
		MTK_IR_TRD_LOG("mtk_ir_netlink_thread begin !!\n");
		set_current_state(TASK_RUNNING);

		spin_lock_irqsave(&(s_msg_q.msgq_lock), __flags);
		ret = (s_msg_q.read == s_msg_q.write);
		spin_unlock_irqrestore(&(s_msg_q.msgq_lock), __flags);

		if (ret == 0) {
		spin_lock_irqsave(&(s_msg_q.msgq_lock), __flags);
		memcpy(&head, (struct message_head *)(s_msg_q.read), IR_NETLINK_MESSAGE_HEADER);
#if 0
		MTK_IR_LOG("s_msg_q.read 0x%p\n", s_msg_q.read);
		MTK_IR_LOG("s_msg_q.write 0x%p\n", s_msg_q.write);
		MTK_IR_LOG("read message_type(%d), message_size(%d)\n", head.message_type,
			   head.message_size);
		MTK_IR_LOG("read message: %s\n", s_msg_q.read + IR_NETLINK_MESSAGE_HEADER);
#endif
		memcpy(buff, s_msg_q.read, IR_NETLINK_MESSAGE_HEADER + head.message_size);

		s_msg_q.read += IR_NETLINK_MSG_SIZE;
		if (s_msg_q.read == s_msg_q.end) {
			s_msg_q.read = s_msg_q.start;
			s_msg_q.write = s_msg_q.start;
		}
		spin_unlock_irqrestore(&(s_msg_q.msgq_lock), __flags);
			mtk_ir_genl_msg_send_to_user(buff, (head.message_size + IR_NETLINK_MESSAGE_HEADER), 0);
		}
		set_current_state(TASK_INTERRUPTIBLE);
		MTK_IR_TRD_LOG(" net schedule() >>>>\n");
		schedule();
		MTK_IR_TRD_LOG(" net schedule() <<<<\n");
	}
	return 0;

}
#endif


int __init mtk_ir_netlink_init(void)
{
	int ret = 0;

	MTK_IR_FUN();
	ret = genl_register_family_with_ops(&ir_gnl_family, ir_gnl_ops);
	if (ret) {
		MTK_IR_ERR(" ir_gnl_family register fail ret(%d)\n", ret);
		goto error1;
	}

	MTK_IR_LOG(" ir_gnl_family id is %d\n", ir_gnl_family.id);

	/* ret = genl_register_ops(&ir_gnl_family, &ir_gnl_ops); */

	if (ret) {
		MTK_IR_ERR(" ir_gnl_ops register fail ret(%d)\n", ret);
		goto error2;
	}
	mtk_ir_netlink_msg_q_init();

	ret = mtk_ir_core_create_thread(mtk_ir_netlink_thread,
									NULL,
									"mtk_ir_netlink_thread",
									&k_thread_netlink,
									94);/*RTPM_PRIO_SCRN_UPDATE*/
	if (ret != 0) {
		MTK_IR_ERR(" create mtk_ir_netlink_thread fail\n");
		goto error3;
	} else
		MTK_IR_LOG(" create mtk_ir_netlink_thread[k_thread_netlink].\n");

	MTK_IR_LOG("mtk_ir_netlink_init----\n");
	return ret;

error3:
	/* genl_unregister_ops(&ir_gnl_family, ir_gnl_ops); */
error2:
	genl_unregister_family(&ir_gnl_family);
error1:

	return ret;
}

void mtk_ir_netlink_exit(void)
{
	mtk_ir_set_log_to(0);	/* here must be exchange ir_log_to state, so log to kernel */
	if (!k_thread_netlink)
		return;
	kthread_stop(k_thread_netlink);
	k_thread_netlink = NULL;
	/* genl_unregister_ops(&ir_gnl_family, ir_gnl_ops); */
	genl_unregister_family(&ir_gnl_family);

}
