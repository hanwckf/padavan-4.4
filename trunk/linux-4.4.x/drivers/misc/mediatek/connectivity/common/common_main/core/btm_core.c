/*
* Copyright (C) 2011-2014 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/atomic.h>
#include "osal_typedef.h"
#include "osal.h"
#include "stp_dbg.h"
#include "stp_core.h"
#include "btm_core.h"
#include "wmt_plat.h"

#define PFX_BTM                         "[STP-BTM] "
#define STP_BTM_LOG_LOUD                 4
#define STP_BTM_LOG_DBG                  3
#define STP_BTM_LOG_INFO                 2
#define STP_BTM_LOG_WARN                 1
#define STP_BTM_LOG_ERR                  0

INT32 gBtmDbgLevel = STP_BTM_LOG_INFO;
INT32 host_trigger_assert_flag;

#define STP_BTM_LOUD_FUNC(fmt, arg...) \
do { \
	if (gBtmDbgLevel >= STP_BTM_LOG_LOUD) \
		pr_warn(PFX_BTM "%s: "  fmt, __func__, ##arg); \
} while (0)
#define STP_BTM_DBG_FUNC(fmt, arg...) \
do { \
	if (gBtmDbgLevel >= STP_BTM_LOG_DBG) \
		pr_warn(PFX_BTM "%s: "  fmt, __func__, ##arg); \
} while (0)
#define STP_BTM_INFO_FUNC(fmt, arg...) \
do { \
	if (gBtmDbgLevel >= STP_BTM_LOG_INFO) \
		pr_warn(PFX_BTM "[I]%s: "  fmt, __func__, ##arg); \
} while (0)
#define STP_BTM_WARN_FUNC(fmt, arg...) \
do { \
	if (gBtmDbgLevel >= STP_BTM_LOG_WARN) \
		pr_warn(PFX_BTM "[W]%s: "  fmt, __func__, ##arg); \
} while (0)
#define STP_BTM_ERR_FUNC(fmt, arg...) \
do { \
	if (gBtmDbgLevel >= STP_BTM_LOG_ERR) \
		pr_err(PFX_BTM "[E]%s(%d):ERROR! "   fmt, __func__, __LINE__, ##arg); \
} while (0)
#define STP_BTM_TRC_FUNC(f) \
do { \
	if (gBtmDbgLevel >= STP_BTM_LOG_DBG) \
		pr_warn(PFX_BTM "<%s> <%d>\n", __func__, __LINE__); \
} while (0)

#define ASSERT(expr)

MTKSTP_BTM_T stp_btm_i;
MTKSTP_BTM_T *stp_btm = &stp_btm_i;

const PINT8 g_btm_op_name[] = {
	"STP_OPID_BTM_RETRY",
	"STP_OPID_BTM_RST",
	"STP_OPID_BTM_DBG_DUMP",
	"STP_OPID_BTM_DUMP_TIMEOUT",
#if CFG_WMT_LTE_COEX_HANDLING
	"STP_OPID_BTM_WMT_LTE_COEX",
#endif
	"STP_OPID_BTM_EXIT"
};

static INT32 _stp_btm_handler(MTKSTP_BTM_T *stp_btm, P_STP_BTM_OP pStpOp)
{
	INT32 ret = -1;
	INT32 dump_sink = 1;	/* core dump target, 0: aee; 1: netlink */

	if (pStpOp == NULL)
		return -1;

	switch (pStpOp->opId) {
	case STP_OPID_BTM_EXIT:
		/* TODO: clean all up? */
		ret = 0;
		break;

		/*tx timeout retry */
	case STP_OPID_BTM_RETRY:
		stp_do_tx_timeout();
		ret = 0;
		break;

		/*whole chip reset */
	case STP_OPID_BTM_RST:
		STP_BTM_INFO_FUNC("whole chip reset start!\n");
		STP_BTM_INFO_FUNC("....+\n");
		if (stp_btm->wmt_notify) {
			stp_btm->wmt_notify(BTM_RST_OP);
			ret = 0;
		} else {
			STP_BTM_ERR_FUNC("stp_btm->wmt_notify is NULL.");
			ret = -1;
		}

		STP_BTM_INFO_FUNC("whole chip reset end!\n");
		host_trigger_assert_flag = 0;
		break;

	case STP_OPID_BTM_DBG_DUMP:
		/*Notify the wmt to get dump data */
		STP_BTM_DBG_FUNC("wmt dmp notification\n");
		dump_sink = mtk_wcn_stp_coredump_flag_get();
		stp_dbg_core_dump(dump_sink);
		break;

	case STP_OPID_BTM_DUMP_TIMEOUT:
#define FAKECOREDUMPEND "coredump end - fake"
		dump_sink = mtk_wcn_stp_coredump_flag_get();
		if (dump_sink == 2) {
			UINT8 tmp[32];

			tmp[0] = '[';
			tmp[1] = 'M';
			tmp[2] = ']';
			tmp[3] = (UINT8)osal_sizeof(FAKECOREDUMPEND);
			tmp[4] = 0;
			osal_memcpy(&tmp[5], FAKECOREDUMPEND, osal_sizeof(FAKECOREDUMPEND));
			/* stp_dump case, append fake coredump end message */
			stp_dbg_set_coredump_timer_state(CORE_DUMP_DOING);
			STP_BTM_WARN_FUNC("generate fake coredump message\n");
			stp_dbg_dump_send_retry_handler((PINT8)&tmp, (INT32)osal_sizeof(FAKECOREDUMPEND)+5);
		}
		stp_dbg_poll_cpupcr(5, 1, 1);
		/* Flush dump data, and reset compressor */
		STP_BTM_INFO_FUNC("Flush dump data\n");
		stp_dbg_core_dump_flush(0, MTK_WCN_BOOL_TRUE);
		ret = mtk_wcn_stp_coredump_timeout_handle();
		break;
#if CFG_WMT_LTE_COEX_HANDLING
	case STP_OPID_BTM_WMT_LTE_COEX:
		ret = wmt_idc_msg_to_lte_handing();
		break;
#endif
	default:
		ret = -1;
		break;
	}

	return ret;
}

static P_OSAL_OP _stp_btm_get_op(MTKSTP_BTM_T *stp_btm, P_OSAL_OP_Q pOpQ)
{
	P_OSAL_OP pOp;
	/* INT32 ret = 0; */

	if (!pOpQ) {
		STP_BTM_WARN_FUNC("!pOpQ\n");
		return NULL;
	}

	osal_lock_unsleepable_lock(&(stp_btm->wq_spinlock));
	/* acquire lock success */
	RB_GET(pOpQ, pOp);
	osal_unlock_unsleepable_lock(&(stp_btm->wq_spinlock));

	if (!pOp)
		STP_BTM_DBG_FUNC("RB_GET fail\n");
	return pOp;
}

static INT32 _stp_btm_put_op(MTKSTP_BTM_T *stp_btm, P_OSAL_OP_Q pOpQ, P_OSAL_OP pOp)
{
	INT32 ret;
	P_OSAL_OP pOp_latest = NULL;
	P_OSAL_OP pOp_current = NULL;
	INT32 flag_latest = 1;
	INT32 flag_current = 1;

	if (!pOpQ || !pOp) {
		STP_BTM_WARN_FUNC("invalid input param: 0x%p, 0x%p\n", pOpQ, pOp);
		return 0;	/* ;MTK_WCN_BOOL_FALSE; */
	}
	ret = 0;

	osal_lock_unsleepable_lock(&(stp_btm->wq_spinlock));
	/* acquire lock success */
	if (&stp_btm->rFreeOpQ == pOpQ) {
		if (!RB_FULL(pOpQ))
			RB_PUT(pOpQ, pOp);
		else
			ret = -1;
	} else if (pOp->op.opId == STP_OPID_BTM_RST || pOp->op.opId == STP_OPID_BTM_DUMP_TIMEOUT) {
		if (!RB_FULL(pOpQ)) {
			RB_PUT(pOpQ, pOp);
			STP_BTM_ERR_FUNC("RB_PUT: 0x%x\n", pOp->op.opId);
		} else
			ret = -1;
	} else {
		pOp_current = stp_btm_get_current_op(stp_btm);
		if (pOp_current) {
			if (pOp_current->op.opId == STP_OPID_BTM_RST ||
			    pOp_current->op.opId == STP_OPID_BTM_DUMP_TIMEOUT) {
				STP_BTM_ERR_FUNC("current: 0x%x\n", pOp_current->op.opId);
				flag_current = 0;
			}
		}

		RB_GET_LATEST(pOpQ, pOp_latest);
		if (pOp_latest) {
			if (pOp_latest->op.opId == STP_OPID_BTM_RST ||
				pOp_latest->op.opId == STP_OPID_BTM_DUMP_TIMEOUT) {
				STP_BTM_ERR_FUNC("latest: 0x%x\n", pOp_latest->op.opId);
				flag_latest = 0;
			}
			if (pOp_latest->op.opId == pOp->op.opId) {
				flag_latest = 0;
				STP_BTM_DBG_FUNC("With the latest a command repeat: latest 0x%x,current 0x%x\n",
						pOp_latest->op.opId, pOp->op.opId);
			}
		}
		if (flag_current && flag_latest) {
			if (!RB_FULL(pOpQ)) {
				RB_PUT(pOpQ, pOp);
				STP_BTM_INFO_FUNC("RB_PUT: 0x%x\n", pOp->op.opId);
			} else
				ret = -1;
		} else
			ret = -1;

	}
	osal_unlock_unsleepable_lock(&(stp_btm->wq_spinlock));

	if (ret) {
		STP_BTM_DBG_FUNC("RB_FULL(0x%p) %d ,rFreeOpQ = %p, rActiveOpQ = %p\n",
			pOpQ, RB_COUNT(pOpQ), &stp_btm->rFreeOpQ, &stp_btm->rActiveOpQ);
		return 0;
	}
	/* STP_BTM_WARN_FUNC("RB_COUNT = %d\n",RB_COUNT(pOpQ)); */
	return 1;
}

P_OSAL_OP _stp_btm_get_free_op(MTKSTP_BTM_T *stp_btm)
{
	P_OSAL_OP pOp;

	if (stp_btm) {
		pOp = _stp_btm_get_op(stp_btm, &stp_btm->rFreeOpQ);
		if (pOp)
			osal_memset(&pOp->op, 0, sizeof(pOp->op));

		return pOp;
	} else
		return NULL;
}

INT32 _stp_btm_put_act_op(MTKSTP_BTM_T *stp_btm, P_OSAL_OP pOp)
{
	INT32 bRet = 0;
	INT32 bCleanup = 0;
	INT32 wait_ret = -1;

	P_OSAL_SIGNAL pSignal = NULL;

	if (!stp_btm || !pOp) {
		STP_BTM_ERR_FUNC("Input NULL pointer\n");
		return bRet;
	}
	do {
		pSignal = &pOp->signal;

		if (pSignal->timeoutValue) {
			pOp->result = -9;
			osal_signal_init(&pOp->signal);
		}

		/* put to active Q */
		bRet = _stp_btm_put_op(stp_btm, &stp_btm->rActiveOpQ, pOp);
		if (bRet == 0) {
			STP_BTM_DBG_FUNC("put active queue fail\n");
			bCleanup = 1;	/* MTK_WCN_BOOL_TRUE; */
			break;
		}
		/* wake up wmtd */
		osal_trigger_event(&stp_btm->STPd_event);

		if (pSignal->timeoutValue == 0) {
			bRet = 1;	/* MTK_WCN_BOOL_TRUE; */
			/* clean it in wmtd */
			break;
		}

		/* wait result, clean it here */
		bCleanup = 1;	/* MTK_WCN_BOOL_TRUE; */

		/* check result */
		wait_ret = osal_wait_for_signal_timeout(&pOp->signal);

		STP_BTM_DBG_FUNC("wait completion:%d\n", wait_ret);
		if (!wait_ret) {
			STP_BTM_ERR_FUNC("wait completion timeout\n");
			/* TODO: how to handle it? retry? */
		} else {
			if (pOp->result)
				STP_BTM_WARN_FUNC("op(%d) result:%d\n", pOp->op.opId, pOp->result);
			bRet = (pOp->result) ? 0 : 1;
		}
	} while (0);

	if (bCleanup) {
		/* put Op back to freeQ */
		_stp_btm_put_op(stp_btm, &stp_btm->rFreeOpQ, pOp);
	}

	return bRet;
}

static INT32 _stp_btm_wait_for_msg(PVOID pvData)
{
	MTKSTP_BTM_T *stp_btm = (MTKSTP_BTM_T *) pvData;

	return (!RB_EMPTY(&stp_btm->rActiveOpQ)) || osal_thread_should_stop(&stp_btm->BTMd);
}

static INT32 _stp_btm_proc(PVOID pvData)
{
	MTKSTP_BTM_T *stp_btm = (MTKSTP_BTM_T *) pvData;
	P_OSAL_OP pOp;
	INT32 id;
	INT32 result;

	if (!stp_btm) {
		STP_BTM_WARN_FUNC("!stp_btm\n");
		return -1;
	}

	for (;;) {
		pOp = NULL;

		osal_wait_for_event(&stp_btm->STPd_event, _stp_btm_wait_for_msg, (PVOID) stp_btm);

		if (osal_thread_should_stop(&stp_btm->BTMd)) {
			STP_BTM_INFO_FUNC("should stop now...\n");
			/* TODO: clean up active opQ */
			break;
		}

		if (stp_btm->gDumplogflag) {
			/* pr_warn("enter place1\n"); */
			stp_btm->gDumplogflag = 0;
			continue;
		}

		/* get Op from activeQ */
		pOp = _stp_btm_get_op(stp_btm, &stp_btm->rActiveOpQ);

		if (!pOp) {
			STP_BTM_WARN_FUNC("get_lxop activeQ fail\n");
			continue;
		}

		id = osal_op_get_id(pOp);

		STP_BTM_DBG_FUNC("======> lxop_get_opid = %d, %s, remaining count = *%d*\n",
				 id, (id >= osal_array_size(g_btm_op_name)) ? ("???") : (g_btm_op_name[id]),
				 RB_COUNT(&stp_btm->rActiveOpQ));

		if (id >= STP_OPID_BTM_NUM) {
			STP_BTM_WARN_FUNC("abnormal opid id: 0x%x\n", id);
			result = -1;
			goto handler_done;
		}

		osal_lock_unsleepable_lock(&(stp_btm->wq_spinlock));
		stp_btm_set_current_op(stp_btm, pOp);
		osal_unlock_unsleepable_lock(&(stp_btm->wq_spinlock));
		result = _stp_btm_handler(stp_btm, &pOp->op);
		osal_lock_unsleepable_lock(&(stp_btm->wq_spinlock));
		stp_btm_set_current_op(stp_btm, NULL);
		osal_unlock_unsleepable_lock(&(stp_btm->wq_spinlock));

handler_done:

		if (result) {
			STP_BTM_WARN_FUNC("opid id(0x%x)(%s) error(%d)\n", id,
				(id >= osal_array_size(g_btm_op_name)) ? ("???") : (g_btm_op_name[id]), result);
		}

		if (osal_op_is_wait_for_signal(pOp)) {
			osal_op_raise_signal(pOp, result);
		} else {
			/* put Op back to freeQ */
			_stp_btm_put_op(stp_btm, &stp_btm->rFreeOpQ, pOp);
		}

		if (id == STP_OPID_BTM_EXIT) {
			break;
		} else if (id == STP_OPID_BTM_RST) {
			/* prevent multi reset case */
			stp_btm_reset_btm_wq(stp_btm);
			mtk_wcn_stp_coredump_start_ctrl(0);
		}
	}

	STP_BTM_INFO_FUNC("exits\n");

	return 0;
}

static inline INT32 _stp_btm_dump_type(MTKSTP_BTM_T *stp_btm, ENUM_STP_BTM_OPID_T opid)
{
	P_OSAL_OP pOp;
	INT32 bRet;
	INT32 retval;

	pOp = _stp_btm_get_free_op(stp_btm);
	if (!pOp) {
		STP_BTM_WARN_FUNC("get_free_lxop fail\n");
		return -1;	/* break; */
	}

	pOp->op.opId = opid;
	pOp->signal.timeoutValue = 0;
	bRet = _stp_btm_put_act_op(stp_btm, pOp);
	STP_BTM_DBG_FUNC("OPID(%d) type(%zd) bRet(%d)\n\n", pOp->op.opId, pOp->op.au4OpData[0], bRet);
	retval = (bRet == 0) ? STP_BTM_OPERATION_FAIL : STP_BTM_OPERATION_SUCCESS;

	return retval;
}

static inline INT32 _stp_btm_notify_wmt_rst_wq(MTKSTP_BTM_T *stp_btm)
{
	INT32 retval;

	if (stp_btm == NULL)
		return STP_BTM_OPERATION_FAIL;

	retval = _stp_btm_dump_type(stp_btm, STP_OPID_BTM_RST);
	return retval;
}

static inline INT32 _stp_btm_notify_stp_retry_wq(MTKSTP_BTM_T *stp_btm)
{
	INT32 retval;

	if (stp_btm == NULL)
		return STP_BTM_OPERATION_FAIL;

	retval = _stp_btm_dump_type(stp_btm, STP_OPID_BTM_RETRY);
	return retval;
}


static inline INT32 _stp_btm_notify_coredump_timeout_wq(MTKSTP_BTM_T *stp_btm)
{
	INT32 retval;

	if (!stp_btm)
		return STP_BTM_OPERATION_FAIL;

	stp_btm_reset_btm_wq(stp_btm);
	retval = _stp_btm_dump_type(stp_btm, STP_OPID_BTM_DUMP_TIMEOUT);
	return retval;
}


static inline INT32 _stp_btm_notify_wmt_dmp_wq(MTKSTP_BTM_T *stp_btm)
{

	INT32 retval;

	if (stp_btm == NULL)
		return STP_BTM_OPERATION_FAIL;

	/* Paged dump */
	retval = _stp_btm_dump_type(stp_btm, STP_OPID_BTM_DBG_DUMP);
	if (!retval)
		mtk_wcn_stp_coredump_start_ctrl(1);
	return retval;
}

INT32 stp_btm_notify_wmt_rst_wq(MTKSTP_BTM_T *stp_btm)
{
	return _stp_btm_notify_wmt_rst_wq(stp_btm);
}

INT32 stp_btm_notify_stp_retry_wq(MTKSTP_BTM_T *stp_btm)
{
	return _stp_btm_notify_stp_retry_wq(stp_btm);
}

INT32 stp_btm_notify_coredump_timeout_wq(MTKSTP_BTM_T *stp_btm)
{
	return _stp_btm_notify_coredump_timeout_wq(stp_btm);
}

INT32 stp_btm_notify_wmt_dmp_wq(MTKSTP_BTM_T *stp_btm)
{
	return _stp_btm_notify_wmt_dmp_wq(stp_btm);
}

INT32 stp_notify_btm_poll_cpupcr_ctrl(UINT32 en)
{
	return stp_dbg_poll_cpupcr_ctrl(en);
}


#if CFG_WMT_LTE_COEX_HANDLING

static inline INT32 _stp_notify_btm_handle_wmt_lte_coex(MTKSTP_BTM_T *stp_btm)
{
	INT32 retval;

	if (stp_btm == NULL)
		return STP_BTM_OPERATION_FAIL;

	retval = _stp_btm_dump_type(stp_btm, STP_OPID_BTM_WMT_LTE_COEX);
	return retval;
}

INT32 stp_notify_btm_handle_wmt_lte_coex(MTKSTP_BTM_T *stp_btm)
{
	return _stp_notify_btm_handle_wmt_lte_coex(stp_btm);
}

#endif
MTKSTP_BTM_T *stp_btm_init(VOID)
{
	INT32 i = 0x0;
	INT32 ret = -1;

	osal_unsleepable_lock_init(&stp_btm->wq_spinlock);
	osal_event_init(&stp_btm->STPd_event);
	stp_btm->wmt_notify = wmt_lib_btm_cb;

	RB_INIT(&stp_btm->rFreeOpQ, STP_BTM_OP_BUF_SIZE);
	RB_INIT(&stp_btm->rActiveOpQ, STP_BTM_OP_BUF_SIZE);

	/* Put all to free Q */
	for (i = 0; i < STP_BTM_OP_BUF_SIZE; i++) {
		osal_signal_init(&(stp_btm->arQue[i].signal));
		_stp_btm_put_op(stp_btm, &stp_btm->rFreeOpQ, &(stp_btm->arQue[i]));
	}

	/*Generate PSM thread, to servie STP-CORE for packet retrying and core dump receiving */
	stp_btm->BTMd.pThreadData = (PVOID) stp_btm;
	stp_btm->BTMd.pThreadFunc = (PVOID) _stp_btm_proc;
	osal_memcpy(stp_btm->BTMd.threadName, BTM_THREAD_NAME, osal_strlen(BTM_THREAD_NAME));

	ret = osal_thread_create(&stp_btm->BTMd);
	if (ret < 0) {
		STP_BTM_ERR_FUNC("osal_thread_create fail...\n");
		goto ERR_EXIT1;
	}

	/* Start STPd thread */
	ret = osal_thread_run(&stp_btm->BTMd);
	if (ret < 0) {
		STP_BTM_ERR_FUNC("osal_thread_run FAILS\n");
		goto ERR_EXIT1;
	}

	return stp_btm;

ERR_EXIT1:

	return NULL;

}

INT32 stp_btm_deinit(MTKSTP_BTM_T *stp_btm)
{

	INT32 ret = -1;

	STP_BTM_INFO_FUNC("btm deinit\n");

	if (!stp_btm)
		return STP_BTM_OPERATION_FAIL;

	ret = osal_thread_destroy(&stp_btm->BTMd);
	if (ret < 0) {
		STP_BTM_ERR_FUNC("osal_thread_destroy FAILS\n");
		return STP_BTM_OPERATION_FAIL;
	}

	return STP_BTM_OPERATION_SUCCESS;
}


INT32 stp_btm_reset_btm_wq(MTKSTP_BTM_T *stp_btm)
{
	UINT32 i = 0;

	osal_lock_unsleepable_lock(&(stp_btm->wq_spinlock));
	RB_INIT(&stp_btm->rFreeOpQ, STP_BTM_OP_BUF_SIZE);
	RB_INIT(&stp_btm->rActiveOpQ, STP_BTM_OP_BUF_SIZE);
	osal_unlock_unsleepable_lock(&(stp_btm->wq_spinlock));
	/* Put all to free Q */
	for (i = 0; i < STP_BTM_OP_BUF_SIZE; i++) {
		osal_signal_init(&(stp_btm->arQue[i].signal));
		_stp_btm_put_op(stp_btm, &stp_btm->rFreeOpQ, &(stp_btm->arQue[i]));
	}

	return 0;
}


INT32 stp_notify_btm_dump(MTKSTP_BTM_T *stp_btm)
{
	/* pr_warn("%s:enter++\n",__func__); */
	if (stp_btm == NULL) {
		osal_dbg_print("%s: NULL POINTER\n", __func__);
		return -1;
	}
	stp_btm->gDumplogflag = 1;
	osal_trigger_event(&stp_btm->STPd_event);
	return 0;
}

static inline INT32 _stp_btm_do_fw_assert(MTKSTP_BTM_T *stp_btm)
{
	INT32 status = -1;
	INT32 j = 0;
	MTK_WCN_BOOL bRet = MTK_WCN_BOOL_FALSE;
	PUINT8 pbuf;
	INT32 len;

	if (host_trigger_assert_flag == 1) {
		STP_BTM_ERR_FUNC("BTM-CORE: host_trigger_assert_flag is set, end this assert flow!\n");
		return status;
	}
	host_trigger_assert_flag = 1;

	/* send assert command */
	STP_BTM_INFO_FUNC("trigger stp assert process\n");
	if (mtk_wcn_stp_is_sdio_mode()) {
		bRet = stp_btm->wmt_notify(BTM_TRIGGER_STP_ASSERT_OP);
		if (bRet == MTK_WCN_BOOL_FALSE) {
			STP_BTM_INFO_FUNC("trigger stp assert failed\n");
			return status;
		}
	} else if (mtk_wcn_stp_is_btif_fullset_mode()) {
#if BTIF_RXD_BE_BLOCKED_DETECT
		if (stp_dbg_is_btif_rxd_be_blocked()) {
			pbuf = "btif_rxd thread be blocked too long,just collect SYS_FTRACE to DB";
			len = osal_strlen(pbuf);
			stp_dbg_trigger_collect_ftrace(pbuf, len);
	} else
#endif
		wmt_plat_force_trigger_assert(STP_FORCE_TRG_ASSERT_DEBUG_PIN);
	}
	do {
		if (mtk_wcn_stp_coredump_start_get() != 0) {
			status = 0;
			break;
		}
		if ((j >= 10) && (j%10 == 0))
		stp_dbg_poll_cpupcr(5, 1, 1);
		j++;
		STP_BTM_INFO_FUNC("Wait for assert message (%d)\n", j);
		osal_sleep_ms(20);

		if (j > 49) {
			mtk_wcn_stp_dbg_dump_package();
			stp_dbg_set_fw_info("host trigger fw assert timeout",
					osal_strlen("host trigger fw assert timeout"),
					STP_HOST_TRIGGER_ASSERT_TIMEOUT);
			pbuf = "Trigger assert timeout ,just collect SYS_FTRACE to DB";
			len = osal_strlen(pbuf);
			/* wcn_core_dump_timeout(); */
			stp_dbg_trigger_collect_ftrace(pbuf, len);
			break;
		}

	} while (1);
	if (status == 0)
		STP_BTM_INFO_FUNC("trigger stp assert succeed\n");
	return status;
}


INT32 stp_notify_btm_do_fw_assert(MTKSTP_BTM_T *stp_btm)
{
	return _stp_btm_do_fw_assert(stp_btm);
}
INT32 wmt_btm_trigger_reset(VOID)
{
	return stp_btm_notify_wmt_rst_wq(stp_btm);
}

INT32 stp_btm_set_current_op(MTKSTP_BTM_T *stp_btm, P_OSAL_OP pOp)
{
		if (stp_btm) {
			stp_btm->pCurOP = pOp;
			STP_BTM_DBG_FUNC("pOp=0x%p\n", pOp);
			return 0;
		}
		STP_BTM_ERR_FUNC("Invalid pointer\n");
		return -1;
}

P_OSAL_OP stp_btm_get_current_op(MTKSTP_BTM_T *stp_btm)
{
	if (stp_btm)
		return stp_btm->pCurOP;
	STP_BTM_ERR_FUNC("Invalid pointer\n");
	return NULL;
}
