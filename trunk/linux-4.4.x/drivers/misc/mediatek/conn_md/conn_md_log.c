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

#include "conn_md_log.h"

int g_conn_md_dbg_lvl = CONN_MD_LOG_INFO;

/*Log defination*/
int __conn_md_log_print(const char *str, ...)
{
	va_list args;
	char temp_sring[DBG_LOG_STR_SIZE];

	va_start(args, str);
	vsnprintf(temp_sring, DBG_LOG_STR_SIZE, str, args);
	va_end(args);

	pr_err("%s", temp_sring);

/* print(KERN_INFO "%s",temp_sring); */

	return 0;
}

int __conn_md_get_log_lvl(void)
{
	/* return CONN_MD_LOG_INFO; */
	return g_conn_md_dbg_lvl;

}

#define CONN_MD_LOG_LOUD    4
#define CONN_MD_LOG_DBG     3
#define CONN_MD_LOG_INFO    2
#define CONN_MD_LOG_WARN    1
#define CONN_MD_LOG_ERR     0

int conn_md_log_set_lvl(int log_lvl)
{
	/* return CONN_MD_LOG_INFO; */
	g_conn_md_dbg_lvl = log_lvl;

	if (g_conn_md_dbg_lvl > CONN_MD_LOG_LOUD) {
		CONN_MD_ERR_FUNC("log_lvl(%d) is too big, round to %d\n", log_lvl, CONN_MD_LOG_LOUD);
		g_conn_md_dbg_lvl = CONN_MD_LOG_LOUD;
	}

	if (g_conn_md_dbg_lvl < CONN_MD_LOG_ERR) {
		CONN_MD_ERR_FUNC("log_lvl(%d) is too small, round to %d\n", log_lvl, CONN_MD_LOG_ERR);
		g_conn_md_dbg_lvl = CONN_MD_LOG_ERR;
	}

	return g_conn_md_dbg_lvl;

}
