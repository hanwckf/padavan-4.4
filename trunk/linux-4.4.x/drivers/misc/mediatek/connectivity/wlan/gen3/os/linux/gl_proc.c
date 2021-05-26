/*
* Copyright (C) 2016 MediaTek Inc.
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

/*
** Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/os/linux/gl_proc.c#2
*/

/*
 * ! \file   "gl_proc.c"
 *  \brief  This file defines the interface which can interact with users in /proc fs.
 *
 *   Detail description.
 */

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "gl_os.h"
#include "gl_kal.h"
#include "debug.h"
#include "wlan_lib.h"
#include "debug.h"

#ifdef FW_CFG_SUPPORT
#include "fwcfg.h"
#endif
/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define PROC_MCR_ACCESS                         "mcr"
#define PROC_ROOT_NAME							"wlan"
#ifdef FW_CFG_SUPPORT
#define PROC_CFG_NAME							"cfg"
#endif

#if CFG_SUPPORT_DEBUG_FS
#define PROC_ROAM_PARAM							"roam_param"
#define PROC_COUNTRY							"country"
#endif
#define PROC_DRV_STATUS                         "status"
#define PROC_RX_STATISTICS                      "rx_statistics"
#define PROC_TX_STATISTICS                      "tx_statistics"
#define PROC_DBG_LEVEL_NAME                     "dbgLevel"
#define PROC_PKT_DELAY_DBG			"pktDelay"
#define PROC_SET_CAM				"setCAM"

#define PROC_MCR_ACCESS_MAX_USER_INPUT_LEN      20
#define PROC_RX_STATISTICS_MAX_USER_INPUT_LEN   10
#define PROC_TX_STATISTICS_MAX_USER_INPUT_LEN   10
#define PROC_DBG_LEVEL_MAX_USER_INPUT_LEN       20
#define PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN      30
#define PROC_UID_SHELL							2000
#define PROC_GID_WIFI							1010

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
static P_GLUE_INFO_T g_prGlueInfo_proc;
static UINT_32 u4McrOffset;
static struct proc_dir_entry *gprProcRoot;
static UINT_8 aucDbModuleName[][PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN] = {
	"INIT", "HAL", "INTR", "REQ", "TX", "RX", "RFTEST", "EMU", "SW1", "SW2",
	"SW3", "SW4", "HEM", "AIS", "RLM", "MEM", "CNM", "RSN", "BSS", "SCN",
	"SAA", "AAA", "P2P", "QM", "SEC", "BOW", "WAPI", "ROAMING", "TDLS", "OID",
	"NIC"
};
static UINT_8 aucProcBuf[1536];

#if FW_CFG_SUPPORT
static P_GLUE_INFO_T gprGlueInfo;
#endif
/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static ssize_t procDbgLevelRead(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	UINT_8 *temp = &aucProcBuf[0];
	UINT_32 u4CopySize = 0;
	UINT_16 i;
	UINT_16 u2ModuleNum = 0;

	/* if *f_ops>0, we should return 0 to make cat command exit */
	if (*f_pos > 0)
		return 0;

	kalStrCpy(temp, "\nTEMP|LOUD|INFO|TRACE | EVENT|STATE|WARN|ERROR\n"
			"bit7|bit6|bit5|bit4 | bit3|bit2|bit1|bit0\n\n"
			"Usage: Module Index:Module Level, such as 0x00:0xff\n\n"
			"Debug Module\tIndex\tLevel\tDebug Module\tIndex\tLevel\n\n");
	temp += kalStrLen(temp);

	u2ModuleNum = (sizeof(aucDbModuleName) / PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN) & 0xfe;
	for (i = 0; i < u2ModuleNum; i += 2)
		SPRINTF(temp, ("DBG_%s_IDX\t(0x%02x):\t0x%02x\tDBG_%s_IDX\t(0x%02x):\t0x%02x\n",
				&aucDbModuleName[i][0], i, aucDebugModule[i],
				&aucDbModuleName[i+1][0], i+1, aucDebugModule[i+1]));

	if ((sizeof(aucDbModuleName) / PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN) & 0x1)
		SPRINTF(temp, ("DBG_%s_IDX\t(0x%02x):\t0x%02x\n",
				&aucDbModuleName[u2ModuleNum][0], u2ModuleNum, aucDebugModule[u2ModuleNum]));

	u4CopySize = kalStrLen(aucProcBuf);
	if (u4CopySize > count)
		u4CopySize = count;
	if (copy_to_user(buf, aucProcBuf, u4CopySize)) {
		DBGLOG(HAL, ERROR, "copy to user failed\n");
		return -EFAULT;
	}

	*f_pos += u4CopySize;
	return (ssize_t)u4CopySize;
}

static ssize_t procDbgLevelWrite(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	UINT_32 u4NewDbgModule, u4NewDbgLevel;
	UINT_8 *temp = &aucProcBuf[0];
	UINT_32 u4CopySize = sizeof(aucProcBuf);

	kalMemSet(aucProcBuf, 0, u4CopySize);
	if (u4CopySize >= count+1)
		u4CopySize = count;

	if (copy_from_user(aucProcBuf, buffer, u4CopySize)) {
		pr_err("error of copy from user\n");
		return -EFAULT;
	}
	aucProcBuf[u4CopySize] = '\0';


	while (temp) {
		if (sscanf(temp, "0x%x:0x%x", &u4NewDbgModule, &u4NewDbgLevel) != 2)  {
			pr_info("debug module and debug level should be one byte in length\n");
			break;
		}
		if (u4NewDbgModule == 0xFF) {
			UINT_8 i = 0;

			for (; i < DBG_MODULE_NUM; i++)
				aucDebugModule[i] = u4NewDbgLevel & DBG_CLASS_MASK;

			break;
		} else if (u4NewDbgModule >= DBG_MODULE_NUM) {
			pr_info("debug module index should less than %d\n", DBG_MODULE_NUM);
			break;
		}
		aucDebugModule[u4NewDbgModule] =  u4NewDbgLevel & DBG_CLASS_MASK;
		temp = kalStrChr(temp, ',');
		if (!temp)
			break;
		temp++; /* skip ',' */
	}
	return count;
}


static const struct file_operations dbglevel_ops = {
	.owner = THIS_MODULE,
	.read = procDbgLevelRead,
	.write = procDbgLevelWrite,
};

static ssize_t procPktDelayDbgCfgRead(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	UINT_8 *temp = &aucProcBuf[0];
	UINT_32 u4CopySize = 0;
	UINT_8 ucTxRxFlag;
	UINT_8 ucTxIpProto;
	UINT_16 u2TxUdpPort;
	UINT_32 u4TxDelayThreshold;
	UINT_8 ucRxIpProto;
	UINT_16 u2RxUdpPort;
	UINT_32 u4RxDelayThreshold;

	/* if *f_ops>0, we should return 0 to make cat command exit */
	if (*f_pos > 0)
		return 0;

	kalStrCpy(temp,
		"\nUsage: txLog/rxLog/reset 1(ICMP)/6(TCP)/11(UDP) Dst/SrcPortNum DelayThreshold(us)\n"
		"Print tx delay log,                                   such as: echo txLog 0 0 0 > pktDelay\n"
		"Print tx UDP delay log,                               such as: echo txLog 11 0 0 > pktDelay\n"
		"Print tx UDP dst port19305 delay log,                 such as: echo txLog 11 19305 0 > pktDelay\n"
		"Print rx UDP src port19305 delay more than 500us log, such as: echo rxLog 11 19305 500 > pktDelay\n"
		"Print tx TCP delay more than 500us log,               such as: echo txLog 6 0 500 > pktDelay\n"
		"Close log,                                            such as: echo reset 0 0 0 > pktDelay\n\n");
	temp += kalStrLen(temp);

	StatsEnvGetPktDelay(&ucTxRxFlag, &ucTxIpProto, &u2TxUdpPort, &u4TxDelayThreshold,
		&ucRxIpProto, &u2RxUdpPort, &u4RxDelayThreshold);

	if (ucTxRxFlag & BIT(0)) {
		SPRINTF(temp, ("txLog %x %d %d\n", ucTxIpProto, u2TxUdpPort, u4TxDelayThreshold));
		temp += kalStrLen(temp);
	}
	if (ucTxRxFlag & BIT(1)) {
		SPRINTF(temp, ("rxLog %x %d %d\n", ucRxIpProto, u2RxUdpPort, u4RxDelayThreshold));
		temp += kalStrLen(temp);
	}
	if (ucTxRxFlag == 0)
		SPRINTF(temp, ("reset 0 0 0, there is no tx/rx delay log\n"));

	u4CopySize = kalStrLen(aucProcBuf);
	if (u4CopySize > count)
		u4CopySize = count;
	if (copy_to_user(buf, aucProcBuf, u4CopySize)) {
		pr_err("copy to user failed\n");
		return -EFAULT;
	}

	*f_pos += u4CopySize;
	return (ssize_t)u4CopySize;
}

static ssize_t procPktDelayDbgCfgWrite(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
#define MODULE_NAME_LENGTH 7
#define MODULE_RESET 0
#define MODULE_TX 1
#define MODULE_RX 2

	UINT_32 u4CopySize = sizeof(aucProcBuf);
	UINT_8 *temp = &aucProcBuf[0];
	UINT_8 aucModule[MODULE_NAME_LENGTH];
	UINT_32 u4DelayThreshold = 0;
	UINT_16 u2PortNum = 0;
	UINT_32 u4IpProto = 0;
	UINT_8 aucResetArray[MODULE_NAME_LENGTH] = "reset";
	UINT_8 aucTxArray[MODULE_NAME_LENGTH] = "txLog";
	UINT_8 aucRxArray[MODULE_NAME_LENGTH] = "rxLog";
	UINT_8 ucTxOrRx = 0;

	kalMemSet(aucProcBuf, 0, u4CopySize);
	if (u4CopySize >= count + 1)
		u4CopySize = count;

	if (copy_from_user(aucProcBuf, buffer, u4CopySize)) {
		pr_err("error of copy from user\n");
		return -EFAULT;
	}
	aucProcBuf[u4CopySize] = '\0';

	while (temp) {
		/* pick up a string and teminated after meet : */
		if (sscanf(temp, "%s %x %d %d", aucModule, &u4IpProto, &u2PortNum, &u4DelayThreshold) != 4)  {
			pr_info("read param fail, aucModule=%s\n", aucModule);
			break;
		}

		if (kalStrnCmp(aucModule, aucResetArray, MODULE_NAME_LENGTH) == 0) {
			ucTxOrRx = MODULE_RESET;
		} else if (kalStrnCmp(aucModule, aucTxArray, MODULE_NAME_LENGTH) == 0) {
			ucTxOrRx = MODULE_TX;
		} else if (kalStrnCmp(aucModule, aucRxArray, MODULE_NAME_LENGTH) == 0) {
			ucTxOrRx = MODULE_RX;
		} else {
			pr_info("input module error!\n");
			break;
		}

		temp = kalStrChr(temp, ',');
		if (!temp)
			break;
		temp++; /* skip ',' */
	}

	StatsEnvSetPktDelay(ucTxOrRx, (UINT_8)u4IpProto, u2PortNum, u4DelayThreshold);

	return count;
}

static const struct file_operations proc_pkt_delay_dbg_ops = {
	.owner = THIS_MODULE,
	.read  = procPktDelayDbgCfgRead,
	.write = procPktDelayDbgCfgWrite,
};

static ssize_t procSetCamCfgWrite(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
#define MODULE_NAME_LEN_1 5

	UINT_32 u4CopySize = sizeof(aucProcBuf);
	UINT_8 *temp = &aucProcBuf[0];
	BOOLEAN fgSetCamCfg = FALSE;
	UINT_8 aucModule[MODULE_NAME_LEN_1];
	UINT_32 u4Enabled;
	UINT_8 aucModuleArray[MODULE_NAME_LEN_1] = "CAM";
	BOOLEAN fgParamValue = TRUE;

	kalMemSet(aucProcBuf, 0, u4CopySize);
	if (u4CopySize >= count + 1)
		u4CopySize = count;

	if (copy_from_user(aucProcBuf, buffer, u4CopySize)) {
		pr_err("error of copy from user\n");
		return -EFAULT;
	}
	aucProcBuf[u4CopySize] = '\0';
	temp = &aucProcBuf[0];
	while (temp) {
		/* pick up a string and teminated after meet : */
		if (sscanf(temp, "%s %d", aucModule, &u4Enabled) != 2)  {
			pr_info("read param fail, aucModule=%s\n", aucModule);
			fgParamValue = FALSE;
			break;
		}

		if (kalStrnCmp(aucModule, aucModuleArray, MODULE_NAME_LEN_1) == 0) {
			if (u4Enabled)
				fgSetCamCfg = TRUE;
			else
				fgSetCamCfg = FALSE;
		}
		temp = kalStrChr(temp, ',');
		if (!temp)
			break;
		temp++; /* skip ',' */
	}

	if (fgParamValue)
		nicConfigProcSetCamCfgWrite(fgSetCamCfg);

	return count;
}

static const struct file_operations proc_set_cam_ops = {
	.owner = THIS_MODULE,
	.write = procSetCamCfgWrite,
};

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
/*----------------------------------------------------------------------------*/
/*!
* \brief The PROC function for reading MCR register to User Space, the offset of
*        the MCR is specified in u4McrOffset.
*
* \param[in] page       Buffer provided by kernel.
* \param[in out] start  Start Address to read(3 methods).
* \param[in] off        Offset.
* \param[in] count      Allowable number to read.
* \param[out] eof       End of File indication.
* \param[in] data       Pointer to the private data structure.
*
* \return number of characters print to the buffer from User Space.
*/
/*----------------------------------------------------------------------------*/
static ssize_t procMCRRead(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	P_GLUE_INFO_T prGlueInfo;
	PARAM_CUSTOM_MCR_RW_STRUCT_T rMcrInfo;
	UINT_32 u4BufLen;
	UINT_32 u4Count;
	UINT_8 *temp = &aucProcBuf[0];
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;

	/* Kevin: Apply PROC read method 1. */
	if (*f_pos > 0)
		return 0;	/* To indicate end of file. */

	prGlueInfo = g_prGlueInfo_proc;

	rMcrInfo.u4McrOffset = u4McrOffset;

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryMcrRead, (PVOID)&rMcrInfo, sizeof(rMcrInfo), TRUE, TRUE, TRUE, &u4BufLen);
	kalMemZero(aucProcBuf, sizeof(aucProcBuf));
	SPRINTF(temp, ("MCR (0x%08xh): 0x%08x\n", rMcrInfo.u4McrOffset, rMcrInfo.u4McrData));

	u4Count = kalStrLen(aucProcBuf);
	if (copy_to_user(buf, aucProcBuf, u4Count)) {
		pr_err("copy to user failed\n");
		return -EFAULT;
	}

	*f_pos += u4Count;

	return (int)u4Count;

}				/* end of procMCRRead() */

/*----------------------------------------------------------------------------*/
/*!
* \brief The PROC function for writing MCR register to HW or update u4McrOffset
*        for reading MCR later.
*
* \param[in] file   pointer to file.
* \param[in] buffer Buffer from user space.
* \param[in] count  Number of characters to write
* \param[in] data   Pointer to the private data structure.
*
* \return number of characters write from User Space.
*/
/*----------------------------------------------------------------------------*/
static ssize_t procMCRWrite(struct file *file, const char __user *buffer,
										size_t count, loff_t *data)
{
	P_GLUE_INFO_T prGlueInfo;
	char acBuf[PROC_MCR_ACCESS_MAX_USER_INPUT_LEN + 1];	/* + 1 for "\0" */
	int i4CopySize;
	PARAM_CUSTOM_MCR_RW_STRUCT_T rMcrInfo;
	UINT_32 u4BufLen;
	WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;
	int num = 0;

	ASSERT(data);

	i4CopySize = (count < (sizeof(acBuf) - 1)) ? count : (sizeof(acBuf) - 1);
	if (copy_from_user(acBuf, buffer, i4CopySize))
		return 0;
	acBuf[i4CopySize] = '\0';

	num = sscanf(acBuf, "0x%x 0x%x", &rMcrInfo.u4McrOffset, &rMcrInfo.u4McrData);
	switch (num) {
	case 2:
		/* NOTE: Sometimes we want to test if bus will still be ok, after accessing
		 * the MCR which is not align to DW boundary.
		 */
		/* if (IS_ALIGN_4(rMcrInfo.u4McrOffset)) */
		{
			prGlueInfo = (P_GLUE_INFO_T) netdev_priv((struct net_device *)data);

			u4McrOffset = rMcrInfo.u4McrOffset;

			/* printk("Write 0x%lx to MCR 0x%04lx\n", */
			/* rMcrInfo.u4McrOffset, rMcrInfo.u4McrData); */

			rStatus = kalIoctl(prGlueInfo,
					   wlanoidSetMcrWrite,
					   (PVOID)&rMcrInfo, sizeof(rMcrInfo), FALSE, FALSE, TRUE, &u4BufLen);

		}
		break;

	case 1:
		/* if (IS_ALIGN_4(rMcrInfo.u4McrOffset)) */
		{
			u4McrOffset = rMcrInfo.u4McrOffset;
		}
		break;

	default:
		break;
	}

	return count;

}				/* end of procMCRWrite() */

static const struct file_operations mcr_ops = {
	.owner = THIS_MODULE,
	.read = procMCRRead,
	.write = procMCRWrite,
};

#if CFG_SUPPORT_DEBUG_FS
static ssize_t procRoamRead(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	UINT_32 u4CopySize;
	WLAN_STATUS rStatus;
	UINT_32 u4BufLen;

	/* if *f_pos > 0, it means has read successed last time, don't try again */
	if (*f_pos > 0)
		return 0;

	rStatus = kalIoctl(g_prGlueInfo_proc, wlanoidGetRoamParams, aucProcBuf, sizeof(aucProcBuf),
							TRUE, FALSE, TRUE, &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, "failed to read roam params\n");
		return -EINVAL;
	}

	u4CopySize = kalStrLen(aucProcBuf);
	if (copy_to_user(buf, aucProcBuf, u4CopySize)) {
		pr_err("copy to user failed\n");
		return -EFAULT;
	}
	*f_pos += u4CopySize;

	return (INT_32)u4CopySize;
}

static ssize_t procRoamWrite(struct file *file, const char __user *buffer,
										size_t count, loff_t *data)
{
	WLAN_STATUS rStatus;
	UINT_32 u4BufLen = 0;
	UINT_32 u4CopySize = sizeof(aucProcBuf);

	kalMemSet(aucProcBuf, 0, u4CopySize);
	if (u4CopySize >= count+1)
		u4CopySize = count;

	if (copy_from_user(aucProcBuf, buffer, u4CopySize)) {
		pr_err("error of copy from user\n");
		return -EFAULT;
	}
	aucProcBuf[u4CopySize] = '\0';

	if (kalStrnCmp(aucProcBuf, "force_roam", 10) == 0)
		rStatus = kalIoctl(g_prGlueInfo_proc, wlanoidSetForceRoam, NULL, 0,
						FALSE, FALSE, TRUE, &u4BufLen);
	else
		rStatus = kalIoctl(g_prGlueInfo_proc, wlanoidSetRoamParams, aucProcBuf,
					kalStrLen(aucProcBuf), FALSE, FALSE, TRUE, &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, "failed to set roam params: %s\n", aucProcBuf);
		return -EINVAL;
	}
	return count;
}

static const struct file_operations roam_ops = {
	.owner = THIS_MODULE,
	.read = procRoamRead,
	.write = procRoamWrite,
};

static ssize_t procCountryRead(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	UINT_32 u4CopySize;
	UINT_16 u2CountryCode = 0;
	UINT_32 u4BufLen;
	WLAN_STATUS rStatus;

	/* if *f_pos > 0, it means has read successed last time, don't try again */
	if (*f_pos > 0)
		return 0;

	rStatus = kalIoctl(g_prGlueInfo_proc, wlanoidGetCountryCode, &u2CountryCode, 2, TRUE, FALSE, TRUE, &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, "failed to get country code\n");
		return -EINVAL;
	}
	if (u2CountryCode)
		kalSprintf(aucProcBuf, "Current Country Code: %c%c\n", (u2CountryCode>>8) & 0xff, u2CountryCode & 0xff);
	else
		kalStrCpy(aucProcBuf, "Current Country Code: NULL\n");

	u4CopySize = kalStrLen(aucProcBuf);
	if (copy_to_user(buf, aucProcBuf, u4CopySize)) {
		pr_err("copy to user failed\n");
		return -EFAULT;
	}
	*f_pos += u4CopySize;

	return (INT_32)u4CopySize;
}

static ssize_t procCountryWrite(struct file *file, const char __user *buffer,
										size_t count, loff_t *data)
{
	UINT_32 u4BufLen = 0;
	WLAN_STATUS rStatus;
	UINT_32 u4CopySize = sizeof(aucProcBuf);

	kalMemSet(aucProcBuf, 0, u4CopySize);
	if (u4CopySize >= count+1)
		u4CopySize = count;

	if (copy_from_user(aucProcBuf, buffer, u4CopySize)) {
		pr_err("error of copy from user\n");
		return -EFAULT;
	}

	aucProcBuf[u4CopySize] = '\0';
	rStatus = kalIoctl(g_prGlueInfo_proc, wlanoidSetCountryCode, &aucProcBuf[0], 2, FALSE, FALSE, TRUE, &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, "failed set country code: %s\n", aucProcBuf);
		return -EINVAL;
	}
	return count;
}

static const struct file_operations country_ops = {
	.owner = THIS_MODULE,
	.read = procCountryRead,
	.write = procCountryWrite,
};
#endif

INT_32 procInitFs(VOID)
{
	struct proc_dir_entry *prEntry;

	if (init_net.proc_net == (struct proc_dir_entry *)NULL) {
		pr_err("init proc fs fail: proc_net == NULL\n");
		return -ENOENT;
	}

	/*
	 * Directory: Root (/proc/net/wlan0)
	 */

	gprProcRoot = proc_mkdir(PROC_ROOT_NAME, init_net.proc_net);
	if (!gprProcRoot) {
		pr_err("gprProcRoot == NULL\n");
		return -ENOENT;
	}
	proc_set_user(gprProcRoot, KUIDT_INIT(PROC_UID_SHELL), KGIDT_INIT(PROC_GID_WIFI));

	prEntry = proc_create(PROC_DBG_LEVEL_NAME, 0664, gprProcRoot, &dbglevel_ops);
	if (prEntry == NULL) {
		pr_err("Unable to create /proc entry dbgLevel\n\r");
		return -1;
	}
	proc_set_user(prEntry, KUIDT_INIT(PROC_UID_SHELL), KGIDT_INIT(PROC_GID_WIFI));
	return 0;
}				/* end of procInitProcfs() */

INT_32 procUninitProcFs(VOID)
{
	remove_proc_subtree(PROC_ROOT_NAME, init_net.proc_net);
	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function clean up a PROC fs created by procInitProcfs().
*
* \param[in] prDev      Pointer to the struct net_device.
* \param[in] pucDevName Pointer to the name of net_device.
*
* \return N/A
*/
/*----------------------------------------------------------------------------*/
INT_32 procRemoveProcfs(VOID)
{
	remove_proc_entry(PROC_MCR_ACCESS, gprProcRoot);
	remove_proc_entry(PROC_PKT_DELAY_DBG, gprProcRoot);
	remove_proc_entry(PROC_SET_CAM, gprProcRoot);
#if CFG_SUPPORT_DEBUG_FS
	remove_proc_entry(PROC_ROAM_PARAM, gprProcRoot);
	remove_proc_entry(PROC_COUNTRY, gprProcRoot);

#endif
	return 0;
} /* end of procRemoveProcfs() */

INT_32 procCreateFsEntry(P_GLUE_INFO_T prGlueInfo)
{
	struct proc_dir_entry *prEntry;

	DBGLOG(INIT, LOUD, "[%s]\n", __func__);
	g_prGlueInfo_proc = prGlueInfo;

	prEntry = proc_create(PROC_MCR_ACCESS, 0664, gprProcRoot, &mcr_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR, "Unable to create /proc entry\n\r");
		return -1;
	}

	prEntry = proc_create(PROC_PKT_DELAY_DBG, 0664, gprProcRoot, &proc_pkt_delay_dbg_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR, "Unable to create /proc entry PktDelayDug\n\r");
		return -ENOENT;
	}
	proc_set_user(prEntry, KUIDT_INIT(PROC_UID_SHELL), KGIDT_INIT(PROC_GID_WIFI));

	prEntry = proc_create(PROC_SET_CAM, 0664, gprProcRoot, &proc_set_cam_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR, "Unable to create /proc entry SetCAM\n\r");
		return -1;
	}
	proc_set_user(prEntry, KUIDT_INIT(PROC_UID_SHELL), KGIDT_INIT(PROC_GID_WIFI));
#if CFG_SUPPORT_DEBUG_FS
	prEntry = proc_create(PROC_ROAM_PARAM, 0664, gprProcRoot, &roam_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR, "Unable to create /proc entry\n\r");
		return -1;
	}
	prEntry = proc_create(PROC_COUNTRY, 0664, gprProcRoot, &country_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR, "Unable to create /proc entry\n\r");
		return -1;
	}
#endif
	return 0;
}

#if 0
/*----------------------------------------------------------------------------*/
/*!
* \brief The PROC function for reading Driver Status to User Space.
*
* \param[in] page       Buffer provided by kernel.
* \param[in out] start  Start Address to read(3 methods).
* \param[in] off        Offset.
* \param[in] count      Allowable number to read.
* \param[out] eof       End of File indication.
* \param[in] data       Pointer to the private data structure.
*
* \return number of characters print to the buffer from User Space.
*/
/*----------------------------------------------------------------------------*/
static int procDrvStatusRead(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	P_GLUE_INFO_T prGlueInfo = ((struct net_device *)data)->priv;
	char *p = page;
	UINT_32 u4Count;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(data);

	/* Kevin: Apply PROC read method 1. */
	if (off != 0)
		return 0;	/* To indicate end of file. */

	SPRINTF(p, ("GLUE LAYER STATUS:"));
	SPRINTF(p, ("\n=================="));

	SPRINTF(p, ("\n* Number of Pending Frames: %ld\n", prGlueInfo->u4TxPendingFrameNum));

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

	wlanoidQueryDrvStatusForLinuxProc(prGlueInfo->prAdapter, p, &u4Count);

	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

	u4Count += (UINT_32) (p - page);

	*eof = 1;

	return (int)u4Count;

}				/* end of procDrvStatusRead() */

/*----------------------------------------------------------------------------*/
/*!
* \brief The PROC function for reading Driver RX Statistic Counters to User Space.
*
* \param[in] page       Buffer provided by kernel.
* \param[in out] start  Start Address to read(3 methods).
* \param[in] off        Offset.
* \param[in] count      Allowable number to read.
* \param[out] eof       End of File indication.
* \param[in] data       Pointer to the private data structure.
*
* \return number of characters print to the buffer from User Space.
*/
/*----------------------------------------------------------------------------*/
static int procRxStatisticsRead(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	P_GLUE_INFO_T prGlueInfo = ((struct net_device *)data)->priv;
	char *p = page;
	UINT_32 u4Count;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(data);

	/* Kevin: Apply PROC read method 1. */
	if (off != 0)
		return 0;	/* To indicate end of file. */

	SPRINTF(p, ("RX STATISTICS (Write 1 to clear):"));
	SPRINTF(p, ("\n=================================\n"));

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

	wlanoidQueryRxStatisticsForLinuxProc(prGlueInfo->prAdapter, p, &u4Count);

	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

	u4Count += (UINT_32) (p - page);

	*eof = 1;

	return (int)u4Count;

}				/* end of procRxStatisticsRead() */

/*----------------------------------------------------------------------------*/
/*!
* \brief The PROC function for reset Driver RX Statistic Counters.
*
* \param[in] file   pointer to file.
* \param[in] buffer Buffer from user space.
* \param[in] count  Number of characters to write
* \param[in] data   Pointer to the private data structure.
*
* \return number of characters write from User Space.
*/
/*----------------------------------------------------------------------------*/
static int procRxStatisticsWrite(struct file *file, const char *buffer, unsigned long count, void *data)
{
	P_GLUE_INFO_T prGlueInfo = ((struct net_device *)data)->priv;
	char acBuf[PROC_RX_STATISTICS_MAX_USER_INPUT_LEN + 1];	/* + 1 for "\0" */
	UINT_32 u4CopySize;
	UINT_32 u4ClearCounter;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(data);

	u4CopySize = (count < (sizeof(acBuf) - 1)) ? count : (sizeof(acBuf) - 1);
	copy_from_user(acBuf, buffer, u4CopySize);
	acBuf[u4CopySize] = '\0';

	if (kstrtouint(acBuf, 0, &u4ClearCounter) == 1) {
		if (u4ClearCounter == 1) {
			GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

			wlanoidSetRxStatisticsForLinuxProc(prGlueInfo->prAdapter);

			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);
		}
	}

	return count;

}				/* end of procRxStatisticsWrite() */

/*----------------------------------------------------------------------------*/
/*!
* \brief The PROC function for reading Driver TX Statistic Counters to User Space.
*
* \param[in] page       Buffer provided by kernel.
* \param[in out] start  Start Address to read(3 methods).
* \param[in] off        Offset.
* \param[in] count      Allowable number to read.
* \param[out] eof       End of File indication.
* \param[in] data       Pointer to the private data structure.
*
* \return number of characters print to the buffer from User Space.
*/
/*----------------------------------------------------------------------------*/
static int procTxStatisticsRead(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	P_GLUE_INFO_T prGlueInfo = ((struct net_device *)data)->priv;
	char *p = page;
	UINT_32 u4Count;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(data);

	/* Kevin: Apply PROC read method 1. */
	if (off != 0)
		return 0;	/* To indicate end of file. */

	SPRINTF(p, ("TX STATISTICS (Write 1 to clear):"));
	SPRINTF(p, ("\n=================================\n"));

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

	wlanoidQueryTxStatisticsForLinuxProc(prGlueInfo->prAdapter, p, &u4Count);

	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

	u4Count += (UINT_32) (p - page);

	*eof = 1;

	return (int)u4Count;

}				/* end of procTxStatisticsRead() */

/*----------------------------------------------------------------------------*/
/*!
* \brief The PROC function for reset Driver TX Statistic Counters.
*
* \param[in] file   pointer to file.
* \param[in] buffer Buffer from user space.
* \param[in] count  Number of characters to write
* \param[in] data   Pointer to the private data structure.
*
* \return number of characters write from User Space.
*/
/*----------------------------------------------------------------------------*/
static int procTxStatisticsWrite(struct file *file, const char *buffer, unsigned long count, void *data)
{
	P_GLUE_INFO_T prGlueInfo = ((struct net_device *)data)->priv;
	char acBuf[PROC_RX_STATISTICS_MAX_USER_INPUT_LEN + 1];	/* + 1 for "\0" */
	UINT_32 u4CopySize;
	UINT_32 u4ClearCounter;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(data);

	u4CopySize = (count < (sizeof(acBuf) - 1)) ? count : (sizeof(acBuf) - 1);
	copy_from_user(acBuf, buffer, u4CopySize);
	acBuf[u4CopySize] = '\0';

	if (kstrtouint(acBuf, 0, &u4ClearCounter) == 1) {
		if (u4ClearCounter == 1) {
			GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

			wlanoidSetTxStatisticsForLinuxProc(prGlueInfo->prAdapter);

			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);
		}
	}

	return count;

}				/* end of procTxStatisticsWrite() */
#endif

#ifdef FW_CFG_SUPPORT
#define MAX_CFG_OUTPUT_BUF_LENGTH 1024
static UINT_8 aucCfgBuf[CMD_FORMAT_V1_LENGTH];
static UINT_8 aucCfgQueryKey[MAX_CMD_NAME_MAX_LENGTH];
static UINT_8 aucCfgOutputBuf[MAX_CFG_OUTPUT_BUF_LENGTH];

static ssize_t cfgRead(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	WLAN_STATUS rStatus = WLAN_STATUS_FAILURE;
	UINT_8 *temp = &aucCfgOutputBuf[0];
	UINT_32 u4CopySize = 0;

	struct _CMD_HEADER_T cmdV1Header;
	struct _CMD_FORMAT_V1_T *pr_cmd_v1 = (struct _CMD_FORMAT_V1_T *) cmdV1Header.buffer;

	/* if *f_pos >  0, we should return 0 to make cat command exit */
	if (*f_pos > 0 || gprGlueInfo == NULL)
		return 0;

	kalMemSet(aucCfgOutputBuf, 0, MAX_CFG_OUTPUT_BUF_LENGTH);

	SPRINTF(temp, ("\nprocCfgRead() %s:\n", aucCfgQueryKey));

	/* send to FW */
	cmdV1Header.cmdVersion = CMD_VER_1;
	cmdV1Header.cmdType = CMD_TYPE_QUERY;
	cmdV1Header.itemNum = 1;
	cmdV1Header.cmdBufferLen = sizeof(struct _CMD_FORMAT_V1_T);
	kalMemSet(cmdV1Header.buffer, 0, MAX_CMD_BUFFER_LENGTH);

	pr_cmd_v1->itemStringLength = kalStrLen(aucCfgQueryKey);

	kalMemCopy(pr_cmd_v1->itemString, aucCfgQueryKey, kalStrLen(aucCfgQueryKey));

	rStatus = kalIoctl(gprGlueInfo,
			wlanoidQueryCfgRead,
			(PVOID)&cmdV1Header,
			sizeof(cmdV1Header),
			TRUE,
			TRUE,
			TRUE,
			&u4CopySize);
	if (rStatus == WLAN_STATUS_FAILURE)
		DBGLOG(INIT, ERROR, "prCmdV1Header kalIoctl wlanoidQueryCfgRead fail 0x%x\n", rStatus);

	SPRINTF(temp, ("%s\n", cmdV1Header.buffer));

	u4CopySize = kalStrLen(aucCfgOutputBuf);
	if (u4CopySize > count)
		u4CopySize = count;

	if (copy_to_user(buf, aucCfgOutputBuf, u4CopySize))
		DBGLOG(INIT, ERROR, "copy to user failed\n");

	*f_pos += u4CopySize;
	return (ssize_t)u4CopySize;
}

static ssize_t cfgWrite(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	/* echo xxx xxx > /proc/net/wlan/cfg */
	UINT_8 i = 0;
	UINT_32 u4CopySize = sizeof(aucCfgBuf);
	UINT_8 token_num = 1;

	kalMemSet(aucCfgBuf, 0, u4CopySize);

	if (u4CopySize >= (count + 1))
		u4CopySize = count;

	if (copy_from_user(aucCfgBuf, buf, u4CopySize)) {
		DBGLOG(INIT, ERROR, "copy from user failed\n");
		return -EFAULT;
	}

	for (; i < u4CopySize; i++) {
		if (aucCfgBuf[i] == ' ') {
			token_num++;
			break;
		}
	}

	if (token_num == 1) {
		kalMemSet(aucCfgQueryKey, 0, sizeof(aucCfgQueryKey));
		/* remove the 0x0a */
		memcpy(aucCfgQueryKey, aucCfgBuf, u4CopySize);
		if (aucCfgQueryKey[u4CopySize - 1] == 0x0a)
			aucCfgQueryKey[u4CopySize - 1] = '\0';
	} else {
		if (u4CopySize)
			wlanFwCfgParse(gprGlueInfo->prAdapter, aucCfgBuf);
	}

	return count;
}

static const struct file_operations cfg_ops = {
	.owner = THIS_MODULE,
	.read = cfgRead,
	.write = cfgWrite,
};

INT_32 cfgRemoveProcEntry(void)
{
	remove_proc_entry(PROC_CFG_NAME, gprProcRoot);
	return 0;
}

INT_32 cfgCreateProcEntry(P_GLUE_INFO_T prGlueInfo)
{
	struct proc_dir_entry *prEntry;

	prGlueInfo->pProcRoot = gprProcRoot;
	gprGlueInfo = prGlueInfo;

	prEntry = proc_create(PROC_CFG_NAME, 0664, gprProcRoot, &cfg_ops);
	if (prEntry == NULL) {
		DBGLOG(INIT, ERROR, "Unable to create /proc entry cfg\n\r");
		return -1;
	}
	proc_set_user(prEntry, KUIDT_INIT(PROC_UID_SHELL), KGIDT_INIT(PROC_GID_WIFI));

	return 0;
}
#endif
