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

#ifndef __CONN_MD_EXP_H_
#define __CONN_MD_EXP_H_

#include "port_ipc.h"		/*data structure is defined here, mediatek/kernel/drivers/eccci */
#include "ccci_ipc_task_ID.h"	/*IPC task id is defined here, mediatek/kernel/drivers/eccci */
typedef unsigned int uint32;
typedef unsigned char uint8;
typedef unsigned short uint16;
#ifdef CHAR
#undef CHAR
#endif

typedef enum {
	CONN_MD_ERR_NO_ERR = 0,
	CONN_MD_ERR_DEF_ERR = -1,
	CONN_MD_ERR_INVALID_PARAM = -2,
	CONN_MD_ERR_OTHERS = -4,

} CONN_MD_ERR_CODE;

/*For IDC test*/
typedef int (*CONN_MD_MSG_RX_CB) (ipc_ilm_t *ilm);

typedef struct {
	CONN_MD_MSG_RX_CB rx_cb;
} CONN_MD_BRIDGE_OPS, *P_CONN_MD_BRIDGE_OPS;

extern int mtk_conn_md_bridge_reg(uint32 u_id, CONN_MD_BRIDGE_OPS *p_ops);
extern int mtk_conn_md_bridge_unreg(uint32 u_id);
extern int mtk_conn_md_bridge_send_msg(ipc_ilm_t *ilm);

#if 0
static int __weak mtk_conn_md_bridge_reg(uint32 u_id, CONN_MD_BRIDGE_OPS *p_ops)
{
	pr_err("MTK_CONN Weak FUNCTION~~~\n");
	return 0;
}

static int __weak mtk_conn_md_bridge_unreg(uint32 u_id)
{
	pr_err("MTK_CONN Weak FUNCTION~~~\n");
	return 0;
}

static int __weak mtk_conn_md_bridge_send_msg(ipc_ilm_t *ilm)
{
	pr_err("MTK_CONN Weak FUNCTION~~~\n");
	return 0;
}
#endif

#endif /*__CONN_MD_EXP_H_*/
