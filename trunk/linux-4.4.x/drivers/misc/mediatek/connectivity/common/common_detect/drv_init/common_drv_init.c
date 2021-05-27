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

#ifdef DFT_TAG
#undef DFT_TAG
#endif
#define DFT_TAG         "[WMT-MOD-INIT]"

#include "wmt_detect.h"
#include "common_drv_init.h"



int do_common_drv_init(int chip_id)
{
	int i_ret = 0;
	int i_ret_tmp = 0;

	WMT_DETECT_INFO_FUNC("start to do common driver init, chipid:0x%08x\n", chip_id);

	wmt_detect_set_chip_type(chip_id);

	/* HIF-SDIO driver init */
	i_ret_tmp = mtk_wcn_hif_sdio_drv_init();
	i_ret += i_ret_tmp;
	WMT_DETECT_DBG_FUNC("HIF-SDIO driver init, i_ret:%d\n", i_ret);

	/* WMT driver init */
	i_ret_tmp = mtk_wcn_common_drv_init();
	i_ret += i_ret_tmp;
	WMT_DETECT_DBG_FUNC("COMBO COMMON driver init, i_ret:%d\n", i_ret);

	/* STP-UART driver init */
	i_ret_tmp = mtk_wcn_stp_uart_drv_init();
	i_ret += i_ret_tmp;
	WMT_DETECT_DBG_FUNC("STP-UART driver init, i_ret:%d\n", i_ret);

	/* STP-SDIO driver init */
	i_ret_tmp = mtk_wcn_stp_sdio_drv_init();
	i_ret += i_ret_tmp;
	WMT_DETECT_DBG_FUNC("STP-SDIO driver init, i_ret:%d\n", i_ret);

	WMT_DETECT_INFO_FUNC("common driver init finish:%d\n", i_ret);
	return i_ret;

}
