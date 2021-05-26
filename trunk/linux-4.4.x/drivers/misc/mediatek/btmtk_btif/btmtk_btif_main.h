/*
 *  Copyright (c) 2017 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __BTMTK_USB_MAIN_H__
#define __BTMTK_USB_MAIN_H__

#include <net/bluetooth/bluetooth.h>
#include "btmtk_define.h"

/* Please keep sync with btmtk_btif_set_state function */
enum {
	BTMTK_BTIF_STATE_UNKNOWN,
	BTMTK_BTIF_STATE_INIT,
	BTMTK_BTIF_STATE_DISCONNECT,
	BTMTK_BTIF_STATE_PROBE,
	BTMTK_BTIF_STATE_WORKING,
	BTMTK_BTIF_STATE_EARLY_SUSPEND,
	BTMTK_BTIF_STATE_SUSPEND,
	BTMTK_BTIF_STATE_RESUME,
	BTMTK_BTIF_STATE_LATE_RESUME,
	BTMTK_BTIF_STATE_FW_DUMP,
	BTMTK_BTIF_STATE_SUSPEND_DISCONNECT,
	BTMTK_BTIF_STATE_SUSPEND_PROBE,
	BTMTK_BTIF_STATE_RESUME_DISCONNECT,
	BTMTK_BTIF_STATE_RESUME_PROBE,
	BTMTK_BTIF_STATE_RESUME_FW_DUMP,
};

/* Please keep sync with btmtk_fops_set_state function */
enum {
	BTMTK_FOPS_STATE_UNKNOWN,	/* deinit in stpbt destroy */
	BTMTK_FOPS_STATE_INIT,		/* init in stpbt created */
	BTMTK_FOPS_STATE_OPENED,	/* open in fops_open */
	BTMTK_FOPS_STATE_CLOSING,	/* during closing */
	BTMTK_FOPS_STATE_CLOSED,	/* closed */
};

struct OSAL_UNSLEEPABLE_LOCK {
	spinlock_t	lock;
	unsigned long	flag;
};

struct ring_buffer_struct {
	struct OSAL_UNSLEEPABLE_LOCK	spin_lock;
	unsigned char			buffer[META_BUFFER_SIZE];	/* MTKSTP_BUFFER_SIZE:1024 */
	unsigned int			read_p;				/* indicate the current read index */
	unsigned int			write_p;			/* indicate the current write index */
};

struct btmtk_btif_data {
	struct semaphore	wr_mtx;
	struct semaphore	rd_mtx;
	int			meta_tx;
	spinlock_t		txlock;

	int				 suspend_count;

	/* request for different io operation */
	unsigned char	w_request;
	unsigned char	r_request;

	/* io buffer for usb control transfer */
	unsigned char	*io_buf;

	unsigned char		*rom_patch;
	unsigned char		*rom_patch_bin_file_name;
	unsigned char		*rom_patch_image;
	unsigned int		rom_patch_image_len;
	unsigned int		chip_id;
	unsigned int		rom_patch_offset;
	unsigned int		rom_patch_len;

	unsigned char		*i_buf;
	unsigned char		*o_buf;
	unsigned char		*i_fwlog_buf;
	unsigned char		*o_fwlog_buf;

	struct ring_buffer_struct	*metabuffer;
	struct ring_buffer_struct	*fwlog_metabuffer;

	unsigned char		interrupt_urb_submitted;
	unsigned char		bulk_urb_submitted;

	unsigned char		event_state;

	struct sk_buff_head	fwlog_queue;
	spinlock_t		fwlog_lock;
	u32			fwlog_queue_count;
	unsigned long       stp_btif_id;
};

enum {
	BTMTK_EVENT_STATE_UNKNOWN,
	BTMTK_EVENT_STATE_POWERING_ON,
	BTMTK_EVENT_STATE_POWER_ON,
	BTMTK_EVENT_STATE_WOBLE,
	BTMTK_EVENT_STATE_POWERING_OFF,
	BTMTK_EVENT_STATE_POWER_OFF,
	BTMTK_EVENT_STATE_WAIT_EVENT,
	BTMTK_EVENT_STATE_ERROR
};

/**
 * Extern functions
 */
int btmtk_btif_send_data(const unsigned char *buffer, const unsigned int length);
int btmtk_btif_meta_send_data(const unsigned char *buffer,
		const unsigned int length);

#endif /* __BTMTK_USB_MAIN_H__ */
