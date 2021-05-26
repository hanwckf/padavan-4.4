/*
 * Copyright (C) 2015-2017 MediaTek Inc.
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

#ifndef _MTK_SYSTIMER_H_
#define _MTK_SYSTIMER_H_

/**
 *	request_systimer - request one systimer
 *	@timer_id: output value, timer_id must be globally unique;
 *		a pointer to a integer where the timer id is to be stored.
 *	@func: timer interrupt handler which is defined by user
 *	@data: input parameter for interrupt func
 *
 * Used to request one systimer for timer interrupt defined by user.
 */
extern int request_systimer(unsigned int *timer_id,
				void (*func)(unsigned long),
				unsigned long data);

/**
 *	free_systimer - release a systimer
 *	@timer_id: systimer id to free
 *
 * Used to free one systimer device allocated with request_systimer().
 */
extern int free_systimer(unsigned int timer_id);

/**
 *	systimer_start - start a systimer
 *	@timer_id: systimer id to use
 *	@usec: time interval in unit of microseconds
 *
 * Used to start a systimer with XXX us expired.
 * This function may be called from interrupt context.
 */
extern int systimer_start(unsigned int timer_id, unsigned int usec);

/**
 *	systimer_stop - stop a systimer
 *	@timer_id: systimer id to stop
 *
 * Used to stop a systimer started with systimer_start().
 * This function may be called from interrupt context.
 */
extern int systimer_stop(unsigned int timer_id);

#endif
