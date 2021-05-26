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
#ifndef __MTK_IR_DEV_H__
#define __MTK_IR_DEV_H__

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/kernel.h>
#else
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#define IR_IO_MAGIC					'I'
#define CMD_IR_MONITOR_KEY			_IOW(IR_IO_MAGIC, 0, int)	/* monitor ir key */
#define CMD_IR_SET_LOG_ON			_IOW(IR_IO_MAGIC, 1, int)	/* /< [To set ir log] */
#define CMD_IR_GET_PROTOCOL			_IOW(IR_IO_MAGIC, 2, int)	/* /< [ get ir  protocol] */
#define CMD_IR_PRINT_MAP_ARRAY		_IOW(IR_IO_MAGIC, 3, int)	/* /< [ print map array] */
#define CMD_IR_SEND_SCANCODE		_IOW(IR_IO_MAGIC, 4, int)	/* /< [ send scancode ] */
#define CMD_IR_SEND_MAPCODE			_IOW(IR_IO_MAGIC, 5, int)	/* /< [ send  map_code] */
#define CMD_IR_GET_UNIFY			_IOW(IR_IO_MAGIC, 6, int)
#define CMD_IR_GET_DEVNAME			_IOW(IR_IO_MAGIC, 7, int)
#define CMD_IR_SEND_ALL_LINUX_KEY	_IOW(IR_IO_MAGIC, 8, int)
#define CMD_IR_NETLINK				_IOW(IR_IO_MAGIC, 9, int)
#define CMD_IR_NETLINK_TEST_LOG		_IOW(IR_IO_MAGIC, 10, int)
#define CMD_IR_NETLINK_MSG_SIZE		_IOW(IR_IO_MAGIC, 11, int)


typedef enum {
	MESSAGE_NORMAL,
	MESSAGE_KEY_INFO,
	MESSAGE_NONE,
} MESSAGE_TYPE;


struct message_head {
	MESSAGE_TYPE message_type;
	int message_size;	/* actual message load size */
};

/* These are the netlink message types for this protocol */
enum {
	IR_CMD_UNSPEC = 0,
	IR_CMD_ATTR_MSG,
	IR_CMD_ATTR_LOG_TO,
	IR_CMD_SEND_MSG,
	_IR_CMD_MAX,
};


#define IR_NODE_NAME		"ir_dev"	/* /dev/ir_android */
#define IR_INPUT_SCAN_DIR	"/dev/input/"	/* search has created event path */
#define IR_GNL_FAMILY_NAME	"ir_gnl_family"	/* genetlink family name */

#define MAX_KEY_NAME_LEN   48	/* max map key_name len */

#define IR_NETLINK_MESSAGE_HEADER sizeof(struct message_head)
#define IR_NETLINK_MSG_SIZE  (IR_NETLINK_MESSAGE_HEADER + 120)	/* netlink max msg len ,include '/0' */
#define IR_NETLINK_GROUP    1

#ifdef __KERNEL__
extern int mtk_ir_netlink_msg_q_send(unsigned char *pv_msg, int z_size);
extern int __init mtk_ir_dev_init(void);
extern void mtk_ir_dev_exit(void);
extern void mtk_ir_dev_put_scancode(u32 ui4scancode);
extern void mtk_ir_dev_show_info(char **buf, int *plen);
extern void mtk_ir_set_log_to(int value);
extern int mtk_ir_get_log_to(void);
#endif

extern int __init mtk_ir_netlink_init(void);
extern void mtk_ir_netlink_exit(void);

#endif
