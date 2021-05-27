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
#ifndef _MTK_THERMAL_TIMER_H_
#define _MTK_THERMAL_TIMER_H_
extern int mtkTTimer_register(const char *name, void (*start_timer) (void), void (*cancel_timer) (void));
extern int mtkTTimer_unregister(const char *name);
#endif	/* _MTK_THERMAL_TIMER_H_ */
