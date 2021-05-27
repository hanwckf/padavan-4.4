/*
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
 */

/*! \file
 * \brief  Declaration of library functions
 * Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
 */

#ifndef _OSAL_TYPEDEF_H_
#define _OSAL_TYPEDEF_H_

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <asm/current.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/firmware.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#if defined(WMT_PLAT_ALPS) && WMT_PLAT_ALPS
#include <aee.h>
#endif
#include <linux/kfifo.h>
#include <linux/wakelock.h>
#include <linux/log2.h>
#include <linux/atomic.h>

#ifndef _TYPEDEFS_H		/*fix redifine */
typedef char INT8, *PINT8, **PPINT8;
#endif

typedef void VOID, *PVOID, **PPVOID;
typedef char *PINT8, **PPINT8;
typedef short INT16, *PINT16, **PPINT16;
typedef int INT32, *PINT32, **PPINT32;
typedef long LONG, *PLONG, **PPLONG;
typedef long long INT64, *PINT64, **PPINT64;

typedef unsigned char UINT8, *PUINT8, **PPUINT8;
typedef unsigned short UINT16, *PUINT16, **PPUINT16;
typedef unsigned int UINT32, *PUINT32, **PPUINT32;
typedef unsigned long ULONG, *PULONG, **PPULONG;
typedef unsigned long long UINT64, *PUINT64, **PPUINT64;

typedef size_t SIZE_T;

typedef int MTK_WCN_BOOL;
#ifndef MTK_WCN_BOOL_TRUE
#define MTK_WCN_BOOL_FALSE               ((MTK_WCN_BOOL) 0)
#define MTK_WCN_BOOL_TRUE                ((MTK_WCN_BOOL) 1)
#endif

#endif /*_OSAL_TYPEDEF_H_*/
