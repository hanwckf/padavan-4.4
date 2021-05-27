/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <linux/delay.h>
#include "hfi.h"
#include "common.h"
#include "eprom.h"

/*
 * The EPROM is logically divided into two partitions:
 *	partition 0: the first 128K, visible from PCI ROM BAR
 *	partition 1: the rest
 */
#define P0_SIZE (128 * 1024)
#define P1_START P0_SIZE

/* largest erase size supported by the controller */
#define SIZE_32KB (32 * 1024)
#define MASK_32KB (SIZE_32KB - 1)

/* controller page size, in bytes */
#define EP_PAGE_SIZE 256
#define EEP_PAGE_MASK (EP_PAGE_SIZE - 1)

/* controller commands */
#define CMD_SHIFT 24
#define CMD_NOP			    (0)
#define CMD_PAGE_PROGRAM(addr)	    ((0x02 << CMD_SHIFT) | addr)
#define CMD_READ_DATA(addr)	    ((0x03 << CMD_SHIFT) | addr)
#define CMD_READ_SR1		    ((0x05 << CMD_SHIFT))
#define CMD_WRITE_ENABLE	    ((0x06 << CMD_SHIFT))
#define CMD_SECTOR_ERASE_32KB(addr) ((0x52 << CMD_SHIFT) | addr)
#define CMD_CHIP_ERASE		    ((0x60 << CMD_SHIFT))
#define CMD_READ_MANUF_DEV_ID	    ((0x90 << CMD_SHIFT))
#define CMD_RELEASE_POWERDOWN_NOID  ((0xab << CMD_SHIFT))

/* controller interface speeds */
#define EP_SPEED_FULL 0x2	/* full speed */

/* controller status register 1 bits */
#define SR1_BUSY 0x1ull		/* the BUSY bit in SR1 */

/* sleep length while waiting for controller */
#define WAIT_SLEEP_US 100	/* must be larger than 5 (see usage) */
#define COUNT_DELAY_SEC(n) ((n) * (1000000/WAIT_SLEEP_US))

/* GPIO pins */
#define EPROM_WP_N (1ull << 14)	/* EPROM write line */

/*
 * Use the EP mutex to guard against other callers from within the driver.
 * Also covers usage of eprom_available.
 */
static DEFINE_MUTEX(eprom_mutex);
static int eprom_available;	/* default: not available */

/*
 * Turn on external enable line that allows writing on the flash.
 */
static void write_enable(struct hfi1_devdata *dd)
{
	/* raise signal */
	write_csr(dd, ASIC_GPIO_OUT,
		read_csr(dd, ASIC_GPIO_OUT) | EPROM_WP_N);
	/* raise enable */
	write_csr(dd, ASIC_GPIO_OE,
		read_csr(dd, ASIC_GPIO_OE) | EPROM_WP_N);
}

/*
 * Turn off external enable line that allows writing on the flash.
 */
static void write_disable(struct hfi1_devdata *dd)
{
	/* lower signal */
	write_csr(dd, ASIC_GPIO_OUT,
		read_csr(dd, ASIC_GPIO_OUT) & ~EPROM_WP_N);
	/* lower enable */
	write_csr(dd, ASIC_GPIO_OE,
		read_csr(dd, ASIC_GPIO_OE) & ~EPROM_WP_N);
}

/*
 * Wait for the device to become not busy.  Must be called after all
 * write or erase operations.
 */
static int wait_for_not_busy(struct hfi1_devdata *dd)
{
	unsigned long count = 0;
	u64 reg;
	int ret = 0;

	/* starts page mode */
	write_csr(dd, ASIC_EEP_ADDR_CMD, CMD_READ_SR1);
	while (1) {
		udelay(WAIT_SLEEP_US);
		usleep_range(WAIT_SLEEP_US - 5, WAIT_SLEEP_US + 5);
		count++;
		reg = read_csr(dd, ASIC_EEP_DATA);
		if ((reg & SR1_BUSY) == 0)
			break;
		/* 200s is the largest time for a 128Mb device */
		if (count > COUNT_DELAY_SEC(200)) {
			dd_dev_err(dd, "waited too long for SPI FLASH busy to clear - failing\n");
			ret = -ETIMEDOUT;
			break; /* break, not goto - must stop page mode */
		}
	}

	/* stop page mode with a NOP */
	write_csr(dd, ASIC_EEP_ADDR_CMD, CMD_NOP);

	return ret;
}

/*
 * Read the device ID from the SPI controller.
 */
static u32 read_device_id(struct hfi1_devdata *dd)
{
	/* read the Manufacture Device ID */
	write_csr(dd, ASIC_EEP_ADDR_CMD, CMD_READ_MANUF_DEV_ID);
	return (u32)read_csr(dd, ASIC_EEP_DATA);
}

/*
 * Erase the whole flash.
 */
static int erase_chip(struct hfi1_devdata *dd)
{
	int ret;

	write_enable(dd);

	write_csr(dd, ASIC_EEP_ADDR_CMD, CMD_WRITE_ENABLE);
	write_csr(dd, ASIC_EEP_ADDR_CMD, CMD_CHIP_ERASE);
	ret = wait_for_not_busy(dd);

	write_disable(dd);

	return ret;
}

/*
 * Erase a range using the 32KB erase command.
 */
static int erase_32kb_range(struct hfi1_devdata *dd, u32 start, u32 end)
{
	int ret = 0;

	if (end < start)
		return -EINVAL;

	if ((start & MASK_32KB) || (end & MASK_32KB)) {
		dd_dev_err(dd,
			"%s: non-aligned range (0x%x,0x%x) for a 32KB erase\n",
			__func__, start, end);
		return -EINVAL;
	}

	write_enable(dd);

	for (; start < end; start += SIZE_32KB) {
		write_csr(dd, ASIC_EEP_ADDR_CMD, CMD_WRITE_ENABLE);
		write_csr(dd, ASIC_EEP_ADDR_CMD,
						CMD_SECTOR_ERASE_32KB(start));
		ret = wait_for_not_busy(dd);
		if (ret)
			goto done;
	}

done:
	write_disable(dd);

	return ret;
}

/*
 * Read a 256 byte (64 dword) EPROM page.
 * All callers have verified the offset is at a page boundary.
 */
static void read_page(struct hfi1_devdata *dd, u32 offset, u32 *result)
{
	int i;

	write_csr(dd, ASIC_EEP_ADDR_CMD, CMD_READ_DATA(offset));
	for (i = 0; i < EP_PAGE_SIZE/sizeof(u32); i++)
		result[i] = (u32)read_csr(dd, ASIC_EEP_DATA);
	write_csr(dd, ASIC_EEP_ADDR_CMD, CMD_NOP); /* close open page */
}

/*
 * Read length bytes starting at offset.  Copy to user address addr.
 */
static int read_length(struct hfi1_devdata *dd, u32 start, u32 len, u64 addr)
{
	u32 offset;
	u32 buffer[EP_PAGE_SIZE/sizeof(u32)];
	int ret = 0;

	/* reject anything not on an EPROM page boundary */
	if ((start & EEP_PAGE_MASK) || (len & EEP_PAGE_MASK))
		return -EINVAL;

	for (offset = 0; offset < len; offset += EP_PAGE_SIZE) {
		read_page(dd, start + offset, buffer);
		if (copy_to_user((void __user *)(addr + offset),
						buffer, EP_PAGE_SIZE)) {
			ret = -EFAULT;
			goto done;
		}
	}

done:
	return ret;
}

/*
 * Write a 256 byte (64 dword) EPROM page.
 * All callers have verified the offset is at a page boundary.
 */
static int write_page(struct hfi1_devdata *dd, u32 offset, u32 *data)
{
	int i;

	write_csr(dd, ASIC_EEP_ADDR_CMD, CMD_WRITE_ENABLE);
	write_csr(dd, ASIC_EEP_DATA, data[0]);
	write_csr(dd, ASIC_EEP_ADDR_CMD, CMD_PAGE_PROGRAM(offset));
	for (i = 1; i < EP_PAGE_SIZE/sizeof(u32); i++)
		write_csr(dd, ASIC_EEP_DATA, data[i]);
	/* will close the open page */
	return wait_for_not_busy(dd);
}

/*
 * Write length bytes starting at offset.  Read from user address addr.
 */
static int write_length(struct hfi1_devdata *dd, u32 start, u32 len, u64 addr)
{
	u32 offset;
	u32 buffer[EP_PAGE_SIZE/sizeof(u32)];
	int ret = 0;

	/* reject anything not on an EPROM page boundary */
	if ((start & EEP_PAGE_MASK) || (len & EEP_PAGE_MASK))
		return -EINVAL;

	write_enable(dd);

	for (offset = 0; offset < len; offset += EP_PAGE_SIZE) {
		if (copy_from_user(buffer, (void __user *)(addr + offset),
						EP_PAGE_SIZE)) {
			ret = -EFAULT;
			goto done;
		}
		ret = write_page(dd, start + offset, buffer);
		if (ret)
			goto done;
	}

done:
	write_disable(dd);
	return ret;
}

/*
 * Perform the given operation on the EPROM.  Called from user space.  The
 * user credentials have already been checked.
 *
 * Return 0 on success, -ERRNO on error
 */
int handle_eprom_command(const struct hfi1_cmd *cmd)
{
	struct hfi1_devdata *dd;
	u32 dev_id;
	int ret = 0;

	/*
	 * The EPROM is per-device, so use unit 0 as that will always
	 * exist.
	 */
	dd = hfi1_lookup(0);
	if (!dd) {
		pr_err("%s: cannot find unit 0!\n", __func__);
		return -EINVAL;
	}

	/* lock against other callers touching the ASIC block */
	mutex_lock(&eprom_mutex);

	/* some platforms do not have an EPROM */
	if (!eprom_available) {
		ret = -ENOSYS;
		goto done_asic;
	}

	/* lock against the other HFI on another OS */
	ret = acquire_hw_mutex(dd);
	if (ret) {
		dd_dev_err(dd,
			"%s: unable to acquire hw mutex, no EPROM support\n",
			__func__);
		goto done_asic;
	}

	dd_dev_info(dd, "%s: cmd: type %d, len 0x%x, addr 0x%016llx\n",
		__func__, cmd->type, cmd->len, cmd->addr);

	switch (cmd->type) {
	case HFI1_CMD_EP_INFO:
		if (cmd->len != sizeof(u32)) {
			ret = -ERANGE;
			break;
		}
		dev_id = read_device_id(dd);
		/* addr points to a u32 user buffer */
		if (copy_to_user((void __user *)cmd->addr, &dev_id,
								sizeof(u32)))
			ret = -EFAULT;
		break;
	case HFI1_CMD_EP_ERASE_CHIP:
		ret = erase_chip(dd);
		break;
	case HFI1_CMD_EP_ERASE_P0:
		if (cmd->len != P0_SIZE) {
			ret = -ERANGE;
			break;
		}
		ret = erase_32kb_range(dd, 0, cmd->len);
		break;
	case HFI1_CMD_EP_ERASE_P1:
		/* check for overflow */
		if (P1_START + cmd->len > ASIC_EEP_ADDR_CMD_EP_ADDR_MASK) {
			ret = -ERANGE;
			break;
		}
		ret = erase_32kb_range(dd, P1_START, P1_START + cmd->len);
		break;
	case HFI1_CMD_EP_READ_P0:
		if (cmd->len != P0_SIZE) {
			ret = -ERANGE;
			break;
		}
		ret = read_length(dd, 0, cmd->len, cmd->addr);
		break;
	case HFI1_CMD_EP_READ_P1:
		/* check for overflow */
		if (P1_START + cmd->len > ASIC_EEP_ADDR_CMD_EP_ADDR_MASK) {
			ret = -ERANGE;
			break;
		}
		ret = read_length(dd, P1_START, cmd->len, cmd->addr);
		break;
	case HFI1_CMD_EP_WRITE_P0:
		if (cmd->len > P0_SIZE) {
			ret = -ERANGE;
			break;
		}
		ret = write_length(dd, 0, cmd->len, cmd->addr);
		break;
	case HFI1_CMD_EP_WRITE_P1:
		/* check for overflow */
		if (P1_START + cmd->len > ASIC_EEP_ADDR_CMD_EP_ADDR_MASK) {
			ret = -ERANGE;
			break;
		}
		ret = write_length(dd, P1_START, cmd->len, cmd->addr);
		break;
	default:
		dd_dev_err(dd, "%s: unexpected command %d\n",
			__func__, cmd->type);
		ret = -EINVAL;
		break;
	}

	release_hw_mutex(dd);
done_asic:
	mutex_unlock(&eprom_mutex);
	return ret;
}

/*
 * Initialize the EPROM handler.
 */
int eprom_init(struct hfi1_devdata *dd)
{
	int ret = 0;

	/* only the discrete chip has an EPROM, nothing to do */
	if (dd->pcidev->device != PCI_DEVICE_ID_INTEL0)
		return 0;

	/* lock against other callers */
	mutex_lock(&eprom_mutex);
	if (eprom_available)	/* already initialized */
		goto done_asic;

	/*
	 * Lock against the other HFI on another OS - the mutex above
	 * would have caught anything in this driver.  It is OK if
	 * both OSes reset the EPROM - as long as they don't do it at
	 * the same time.
	 */
	ret = acquire_hw_mutex(dd);
	if (ret) {
		dd_dev_err(dd,
			"%s: unable to acquire hw mutex, no EPROM support\n",
			__func__);
		goto done_asic;
	}

	/* reset EPROM to be sure it is in a good state */

	/* set reset */
	write_csr(dd, ASIC_EEP_CTL_STAT,
					ASIC_EEP_CTL_STAT_EP_RESET_SMASK);
	/* clear reset, set speed */
	write_csr(dd, ASIC_EEP_CTL_STAT,
			EP_SPEED_FULL << ASIC_EEP_CTL_STAT_RATE_SPI_SHIFT);

	/* wake the device with command "release powerdown NoID" */
	write_csr(dd, ASIC_EEP_ADDR_CMD, CMD_RELEASE_POWERDOWN_NOID);

	eprom_available = 1;
	release_hw_mutex(dd);
done_asic:
	mutex_unlock(&eprom_mutex);
	return ret;
}
