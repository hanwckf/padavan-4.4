/*
 * Author: yucong xiong <yucong.xion@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/io.h>
#include "cust_alsps.h"
#include "cm36686.h"
#include <linux/sched.h>
#include <alsps.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

/******************************************************************************
 * configuration
*******************************************************************************/
/*----------------------------------------------------------------------------*/

#define CM36686_DRIVER_NAME     "cm36686_driver"
#define ALSPS_DEV_NAME1	"alsps_1"
#define ALSPS_DEV_NAME2	"alsps_2"

/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               pr_debug(APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)    pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    pr_debug(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    pr_debug(APS_TAG fmt, ##args)

#define I2C_FLAG_WRITE	0
#define I2C_FLAG_READ	1

/******************************************************************************
 * extern functions
*******************************************************************************/
static int cm36686_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int cm36686_i2c_remove(struct i2c_client *client);
static int cm36686_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int cm36686_i2c_suspend(struct device *dev);
static int cm36686_i2c_resume(struct device *dev);

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id cm36686_i2c_id[] = {
		{ALSPS_DEV_NAME1, 0},
		{ALSPS_DEV_NAME2, 0},
		{}
	};

#ifdef CM36686_PS_EINT_ENABLE
static unsigned long long int_top_time;
#endif
/* Maintain alsps cust info here */
struct alsps_hw alsps_cust[CMP_DEVICE_NUM];
static struct alsps_hw *hw[CMP_DEVICE_NUM] = {
		&alsps_cust[0],
		&alsps_cust[1]
	};

struct platform_device *alspsPltFmDev;

/*----------------------------------------------------------------------------*/
struct cm36686_priv {
	struct alsps_hw *hw;
	struct i2c_client *client;
	struct work_struct eint_work;

	/*misc */
	u16 als_modulus;
	atomic_t i2c_retry;
	atomic_t als_suspend;
	atomic_t als_debounce;	/*debounce time after enabling als */
	atomic_t als_deb_on;	/*indicates if the debounce is on */
#ifdef CONFIG_64BIT
	atomic64_t als_deb_end;	/*the jiffies representing the end of debounce */
#else
	atomic_t als_deb_end;	/*the jiffies representing the end of debounce */
#endif
	atomic_t ps_mask;	/*mask ps: always return far away */
	atomic_t ps_debounce;	/*debounce time after enabling ps */
	atomic_t ps_deb_on;	/*indicates if the debounce is on */
#ifdef CONFIG_64BIT
	atomic64_t ps_deb_end;	/*the jiffies representing the end of debounce */
#else
	atomic_t ps_deb_end;	/*the jiffies representing the end of debounce */
#endif
	atomic_t ps_suspend;
	atomic_t trace;
	atomic_t init_done;
	struct device_node *irq_node;
	int irq;

	/*data */
	u16 als;
	u16 ps;
	/* u8                   _align; */
	u16 als_level_num;
	u16 als_value_num;
	u32 als_level[C_CUST_ALS_LEVEL - 1];
	u32 als_value[C_CUST_ALS_LEVEL];
	int ps_cali;

	atomic_t als_cmd_val;	/*the cmd value can't be read, stored in ram */
	atomic_t ps_cmd_val;	/*the cmd value can't be read, stored in ram */
	atomic_t ps_thd_val_high;	/*the cmd value can't be read, stored in ram */
	atomic_t ps_thd_val_low;	/*the cmd value can't be read, stored in ram */
	atomic_t als_thd_val_high;	/*the cmd value can't be read, stored in ram */
	atomic_t als_thd_val_low;	/*the cmd value can't be read, stored in ram */
	atomic_t ps_thd_val;
	ulong enable;		/*enable mask */
	ulong pending_intr;	/*pending interrupt */
};
/*----------------------------------------------------------------------------*/

#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,alsps_1"},
	{.compatible = "mediatek,alsps_2"},
	{},
};
#endif
#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops CM36686_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(cm36686_i2c_suspend, cm36686_i2c_resume)
};
#endif
static struct i2c_driver cm36686_i2c_driver = {
	.probe = cm36686_i2c_probe,
	.remove = cm36686_i2c_remove,
	.detect = cm36686_i2c_detect,
	.id_table = cm36686_i2c_id,
	.driver = {
		.name = CM36686_DRIVER_NAME,
#ifdef CONFIG_PM_SLEEP
		.pm   = &CM36686_pm_ops,
#endif
#ifdef CONFIG_OF
		.of_match_table = alsps_of_match,
#endif
	},
};

/*----------------------------------------------------------------------------*/
struct PS_CALI_DATA_STRUCT {
	int close;
	int far_away;
	int valid;
};

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static struct i2c_client *cm36686_i2c_client[CMP_DEVICE_NUM];
static struct cm36686_priv *cm36686_obj[CMP_DEVICE_NUM];
static int intr_flag = 1;	/* hw default away after enable. */

static int cm36686_local_init(void);
static int cm36686_remove(void);
static int cm36686_init_flag = -1;	/* 0<==>OK -1 <==> fail */
static struct alsps_init_info cm36686_init_info = {
	.name = "cm36686",
	.init = cm36686_local_init,
	.uninit = cm36686_remove,
};

/*----------------------------------------------------------------------------*/
static DEFINE_MUTEX(cm36686_mutex);
static DEFINE_MUTEX(cm36686_i2c_mutex);
/*----------------------------------------------------------------------------*/
enum CMC_BIT {
	CMC_BIT_ALS = 1,
	CMC_BIT_PS = 2,
};
/*-----------------------------CMC for debugging-------------------------------*/
enum CMC_TRC {
	CMC_TRC_ALS_DATA = 0x0001,
	CMC_TRC_PS_DATA = 0x0002,
	CMC_TRC_EINT = 0x0004,
	CMC_TRC_IOCTL = 0x0008,
	CMC_TRC_I2C = 0x0010,
	CMC_TRC_CVT_ALS = 0x0020,
	CMC_TRC_CVT_PS = 0x0040,
	CMC_TRC_CVT_AAL = 0x0080,
	CMC_TRC_DEBUG = 0x8000,
};

static u8 *g_i2c_buff;
static u8 *g_i2c_addr;

static int msg_dma_alloc(void);
static void msg_dma_release(void);

static int get_dev_idex_by_dev(struct device *dev)
{
	u8 idx = -1;

	if (strcmp(dev->of_node->name, ALSPS_DEV_NAME1) == 0)
		idx = 0;
	else if (strcmp(dev->of_node->name, ALSPS_DEV_NAME2) == 0)
		idx = 1;

	return idx;
}
static int get_dev_idex_by_client(struct i2c_client *client)
{
	u8 idx = -1;

	if (strcmp(client->name, ALSPS_DEV_NAME1) == 0)
		idx = 0;
	else if (strcmp(client->name, ALSPS_DEV_NAME2) == 0)
		idx = 1;

	return idx;
}

static int msg_dma_alloc(void)
{
	if (g_i2c_buff ==  NULL) {
		g_i2c_buff = kzalloc(CMP_DMA_MAX_TRANSACTION_LEN, GFP_KERNEL);
		if (!g_i2c_buff) {
			APS_ERR("[DMA][Error] Allocate DMA I2C Buffer failed!\n");
			return -1;
		}
	}

	if (g_i2c_addr ==  NULL) {
		g_i2c_addr = kzalloc(CMP_REG_ADDR_LEN, GFP_KERNEL);
		if (!g_i2c_addr) {
			APS_ERR("[DMA]Allocate DMA I2C addr buf failed!\n");
			kfree(g_i2c_buff);
			g_i2c_buff = NULL;
			return -1;
		}
	}
	return 0;
}

static void msg_dma_release(void)
{
	kfree(g_i2c_buff);
	g_i2c_buff = NULL;

	kfree(g_i2c_addr);
	g_i2c_addr = NULL;
}

static s32 i2c_dma_read(struct i2c_client *client, u8 addr, u8 *rxbuf, int len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg[2];

	if (rxbuf == NULL)
		return -1;

	memset(&msg, 0, 2 * sizeof(struct i2c_msg));
	memcpy(g_i2c_addr, &addr, CMP_REG_ADDR_LEN);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = g_i2c_addr;
	msg[0].len = CMP_REG_ADDR_LEN;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = g_i2c_buff;
	msg[1].len = len;

	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg[0], 2);
		if (ret < 0)
			continue;
		memcpy(rxbuf, g_i2c_buff, len);
		return 0;
	}
	APS_ERR("Dma I2C Read Error: 0x%4X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}

static s32 i2c_dma_write(struct i2c_client *client, u8 addr, u8 *txbuf, s32 len)
{
	int ret;
	s32 retry = 0;
	struct i2c_msg msg;

	if ((txbuf == NULL) && (len > 0))
		return -1;

	memset(&msg, 0, sizeof(struct i2c_msg));
	*g_i2c_buff = addr;

	msg.addr = (client->addr);
	msg.flags = 0;
	msg.buf = g_i2c_buff;
	msg.len = CMP_REG_ADDR_LEN + len;

	if (txbuf)
		memcpy(g_i2c_buff + CMP_REG_ADDR_LEN, txbuf, len);
	for (retry = 0; retry < 5; ++retry) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret < 0)
			continue;
		return 0;
	}
	APS_ERR("Dma I2C Write Error: 0x%04X, %d bytes, err-code: %d\n", addr, len, ret);
	return ret;
}

static int cmp_i2c_read(struct i2c_client *client, u8 addr, u8 *rxbuf, int len)
{
	int left = len;
	int readLen = 0;
	u8 *rd_buf = rxbuf;
	int ret = 0;

	while (left > 0) {
		mutex_lock(&cm36686_i2c_mutex);
		readLen = left > CMP_DMA_MAX_RD_SIZE ? CMP_DMA_MAX_RD_SIZE : left;
		ret = i2c_dma_read(client, addr, rd_buf, readLen);
		mutex_unlock(&cm36686_i2c_mutex);
		if (ret < 0) {
			APS_ERR("dma read failed!\n");
			return -1;
		}

		left -= readLen;
		if (left > 0) {
			addr += readLen;
			rd_buf += readLen;
		}
	}
	return 0;
}

static s32 cmp_i2c_write(struct i2c_client *client, u8 addr, u8 *txbuf, int len)
{

	int ret = 0;
	int write_len = 0;
	int left = len;
	u8 *wr_buf = txbuf;
	u8 offset = 0;
	u8 wrAddr = addr;

	while (left > 0) {
		mutex_lock(&cm36686_i2c_mutex);
		write_len = left > CMP_DMA_MAX_WR_SIZE ? CMP_DMA_MAX_WR_SIZE : left;
		ret = i2c_dma_write(client, wrAddr, wr_buf, write_len);
		mutex_unlock(&cm36686_i2c_mutex);
		if (ret < 0) {
			APS_ERR("dma i2c write failed!\n");
			return -1;
		}
		offset += write_len;
		left -= write_len;
		if (left > 0) {
			wrAddr = addr + offset;
			wr_buf = txbuf + offset;
		}
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
static void cm36686_power(struct alsps_hw *hw, unsigned int on)
{
}

/********************************************************************/
int cm36686_enable_ps(struct i2c_client *client, int enable)
{
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 databuf[2];

	if (enable == 1) {
		APS_LOG("cm36686_enable_ps enable_ps\n");
		res = cmp_i2c_read(client, CM36686_REG_PS_CONF1_2, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		APS_LOG("PS_CONF1_2 low = %x, high = %x\n",	databuf[0], databuf[1]);

		databuf[0] &= 0xFE;
		res = cmp_i2c_write(client, CM36686_REG_PS_CONF1_2, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		atomic_set(&obj->ps_deb_on, 1);
#ifdef CONFIG_64BIT
		atomic64_set(&obj->ps_deb_end,
			     jiffies + atomic_read(&obj->ps_debounce) / (1000 / HZ));
#else
		atomic_set(&obj->ps_deb_end,
			   jiffies + atomic_read(&obj->ps_debounce) / (1000 / HZ));
#endif
		intr_flag = 1;	/* reset hw status to away after enable. */
	} else {
		APS_LOG("cm36686_enable_ps disable_ps\n");
		res = cmp_i2c_read(client, CM36686_REG_PS_CONF1_2, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}

		APS_LOG("PS_CONF1_2 value: low = %x, high = %x\n", databuf[0], databuf[1]);

		databuf[0] |= 0x01;
		res = cmp_i2c_write(client, CM36686_REG_PS_CONF1_2, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_PS_EXIT_ERR;
		}
		atomic_set(&obj->ps_deb_on, 0);

	}

	return 0;
ENABLE_PS_EXIT_ERR:
	return res;
}

/********************************************************************/
int cm36686_enable_als(struct i2c_client *client, int enable)
{
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	int res;
	u8 databuf[2];

	if (enable == 1) {
		APS_LOG("enable_als\n");

		res = cmp_i2c_read(client, CM36686_REG_ALS_CONF, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}

		APS_LOG("ALS_CONF value: low = %x, high = %x\n", databuf[0], databuf[1]);

		databuf[0] &= 0xFE;
		res = cmp_i2c_write(client, CM36686_REG_ALS_CONF, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}
		atomic_set(&obj->als_deb_on, 1);
#ifdef CONFIG_64BIT
		atomic64_set(&obj->als_deb_end,
			     jiffies + atomic_read(&obj->als_debounce) / (1000 / HZ));
#else
		atomic_set(&obj->als_deb_end,
			   jiffies + atomic_read(&obj->als_debounce) / (1000 / HZ));
#endif

	} else {
		APS_LOG("disable_als\n");
		res = cmp_i2c_write(client, CM36686_REG_ALS_CONF, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}

		APS_LOG("ALS_CONF value:low = %x, high = %x\n", databuf[0],	databuf[1]);

		databuf[0] |= 0x01;
		res = cmp_i2c_write(client, CM36686_REG_ALS_CONF, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto ENABLE_ALS_EXIT_ERR;
		}
		atomic_set(&obj->als_deb_on, 0);
	}
	return 0;
ENABLE_ALS_EXIT_ERR:
	return res;
}

/********************************************************************/
long cm36686_read_ps(struct i2c_client *client, u16 *data)
{
	long res;
	u8 databuf[2];
	struct cm36686_priv *obj = i2c_get_clientdata(client);

	if (!test_bit(CMC_BIT_PS, &obj->enable)) {
		res = cm36686_enable_ps(obj->client, 1);
		if (res) {
			APS_ERR("enable als fail: %ld\n", res);
			return -1;
		}
		set_bit(CMC_BIT_PS, &obj->enable);
	}

	res = cmp_i2c_read(client, CM36686_REG_PS_DATA, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto READ_PS_EXIT_ERR;
	}

	if (atomic_read(&obj->trace) & CMC_TRC_DEBUG) {
		APS_LOG("_PS_DATA value: low = %x, high = %x\n", databuf[0], databuf[1]);
	}

	*data = ((databuf[1] << 8) | databuf[0]);

	if (*data < obj->ps_cali)
		*data = 0;
	else
		*data = *data - obj->ps_cali;
	return 0;
READ_PS_EXIT_ERR:
	return res;
}

/********************************************************************/
long cm36686_read_als(struct i2c_client *client, u16 *data)
{
	long res;
	u8 databuf[2];
	struct cm36686_priv *obj = i2c_get_clientdata(client);

	if (!test_bit(CMC_BIT_ALS, &obj->enable)) {
		res = cm36686_enable_als(obj->client, 1);
		if (res) {
			APS_ERR("enable als fail: %ld\n", res);
			return -1;
		}
		set_bit(CMC_BIT_ALS, &obj->enable);
	}

	res = cmp_i2c_read(client, CM36686_REG_ALS_DATA, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto READ_ALS_EXIT_ERR;
	}

	if (atomic_read(&obj->trace) & CMC_TRC_DEBUG)
		APS_LOG("CM36686_REG_ALS_DATA value: %d\n", ((databuf[1] << 8) | databuf[0]));

	*data = ((databuf[1] << 8) | databuf[0]);

	return 0;
READ_ALS_EXIT_ERR:
	return res;
}

/********************************************************************/
static int cm36686_get_ps_value(struct cm36686_priv *obj, u16 ps)
{
	int val, mask = atomic_read(&obj->ps_mask);
	int invalid = 0;

	val = intr_flag;	/* value between high/low threshold should sync. with hw status. */

	if (ps > atomic_read(&obj->ps_thd_val_high))
		val = 0;	/*close */
	else if (ps < atomic_read(&obj->ps_thd_val_low))
		val = 1;	/*far away */

	if (atomic_read(&obj->ps_suspend)) {
		invalid = 1;
	} else if (atomic_read(&obj->ps_deb_on) == 1) {
#ifdef CONFIG_64BIT
		unsigned long endt = atomic64_read(&obj->ps_deb_end);
#else
		unsigned long endt = atomic_read(&obj->ps_deb_end);
#endif
		if (time_after(jiffies, endt))
			atomic_set(&obj->ps_deb_on, 0);

		if (atomic_read(&obj->ps_deb_on) == 1)
			invalid = 1;
	}

	if (!invalid) {
		if (unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS)) {
			if (mask)
				APS_DBG("PS:  %05d => %05d [M]\n", ps, val);
			else
				APS_DBG("PS:  %05d => %05d\n", ps, val);
		}
		if (test_bit(CMC_BIT_PS, &obj->enable) == 0) {
			/* if ps is disable do not report value */
			APS_DBG("PS: not enable and do not report this value\n");
			return -1;
		} else {
			return val;
		}

	} else {
		if (unlikely(atomic_read(&obj->trace) & CMC_TRC_CVT_PS))
			APS_DBG("PS:  %05d => %05d (-1)\n", ps, val);
		return -1;
	}
}

/********************************************************************/
static int cm36686_get_als_value(struct cm36686_priv *obj, u16 als)
{
	int idx = 0;
	int invalid = 0;
	int level_high = 0;
	int level_low = 0;
	int level_diff = 0;
	int value_high = 0;
	int value_low = 0;
	int value_diff = 0;
	int value = 0;

	if ((obj->als_level_num == 0) || (obj->als_value_num == 0)) {
		APS_ERR("invalid als_level_num = %d, als_value_num = %d\n", obj->als_level_num,
			obj->als_value_num);
		return -1;
	}

	if (atomic_read(&obj->als_deb_on) == 1) {
#ifdef CONFIG_64BIT
		unsigned long endt = (unsigned long)atomic64_read(&obj->als_deb_end);
#else
		unsigned long endt = (unsigned long)atomic_read(&obj->als_deb_end);
#endif
		if (time_after(jiffies, endt))
			atomic_set(&obj->als_deb_on, 0);

		if (atomic_read(&obj->als_deb_on) == 1)
			invalid = 1;
	}

	for (idx = 0; idx < obj->als_level_num; idx++) {
		if (als < obj->hw->als_level[idx])
			break;
	}

	if (idx >= obj->als_level_num || idx >= obj->als_value_num) {
		if (idx < obj->als_value_num)
			value = obj->hw->als_value[idx - 1];
		else
			value = obj->hw->als_value[obj->als_value_num - 1];
	} else {
		level_high = obj->hw->als_level[idx];
		level_low = (idx > 0) ? obj->hw->als_level[idx - 1] : 0;
		level_diff = level_high - level_low;
		value_high = obj->hw->als_value[idx];
		value_low = (idx > 0) ? obj->hw->als_value[idx - 1] : 0;
		value_diff = value_high - value_low;

		if ((level_low >= level_high) || (value_low >= value_high))
			value = value_low;
		else
			value =
			    (level_diff * value_low + (als - level_low) * value_diff +
			     ((level_diff + 1) >> 1)) / level_diff;
	}

	if (!invalid) {
		if (atomic_read(&obj->trace) & CMC_TRC_CVT_AAL)
			APS_DBG("ALS: %d [%d, %d] => %d [%d, %d]\n", als, level_low, level_high,
				value, value_low, value_high);
		return value;

	} else {
		if (atomic_read(&obj->trace) & CMC_TRC_CVT_ALS)
			APS_DBG("ALS: %05d => %05d (-1)\n", als, value);

		return -1;
	}

}

#ifndef DEVICE_ATTRIBUTE_ENABLE
static int dev_idx;
/*-------------------------------attribute file for debugging----------------------------------*/

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t cm36686_show_config(struct device_driver *ddri, char *buf)
{
	ssize_t res = 0;
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm36686_obj[%d] is null!!\n");
		return res;
	}

	res += snprintf(buf + res, PAGE_SIZE, "(%d %d %d %d %d)\n",
		atomic_read(&obj->i2c_retry),
		atomic_read(&obj->als_debounce),
		atomic_read(&obj->ps_mask),
		atomic_read(&obj->ps_thd_val),
		atomic_read(&obj->ps_debounce));
return res;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_config(struct device_driver *ddri, const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];

	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	if (sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb) == 5) {
		atomic_set(&obj->i2c_retry, retry);
		atomic_set(&obj->als_debounce, als_deb);
		atomic_set(&obj->ps_mask, mask);
		atomic_set(&obj->ps_thd_val, thres);
		atomic_set(&obj->ps_debounce, ps_deb);
	} else {
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_trace(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_trace(struct device_driver *ddri, const char *buf, size_t count)
{
	int trace;
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	if (sscanf(buf, "0x%x", &trace) == 1)
		atomic_set(&obj->trace, trace);
	else
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_als(struct device_driver *ddri, char *buf)
{
	int res;
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}
	res = cm36686_read_als(obj->client, &obj->als);
	if (res)
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	else
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", obj->als);
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_ps(struct device_driver *ddri, char *buf)
{
	ssize_t res;
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm3623_obj is null!!\n");
		return 0;
	}

	res = cm36686_read_ps(obj->client, &obj->ps);
	if (res)
		return snprintf(buf, PAGE_SIZE, "ERROR: %d\n", (int)res);
	else
		return snprintf(buf, PAGE_SIZE, "0x%04X\n", obj->ps);
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_reg(struct device_driver *ddri, char *buf)
{
	u8 _bIndex = 0;
	u8 databuf[2] = { 0 };
	ssize_t _tLength = 0;
	int res;
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm3623_obj is null!!\n");
		return 0;
	}

	for (_bIndex = 0; _bIndex < 0x0D; _bIndex++) {
		res = cmp_i2c_read(obj->client, _bIndex, databuf, 2);
		if (res < 0)
			APS_ERR("i2c_master_send function err res = %d\n", res);

		_tLength +=
		    snprintf((buf + _tLength), (PAGE_SIZE - _tLength), "Reg[0x%02X]: 0x%02X\n",
			     _bIndex, databuf[0]);
	}

	return _tLength;

}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_send(struct device_driver *ddri, char *buf)
{
	return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_send(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr, cmd;
	u8 dat;
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	} else if (sscanf(buf, "%x %x", &addr, &cmd) != 2) {
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	dat = (u8) cmd;

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_recv(struct device_driver *ddri, char *buf)
{
	return 0;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_recv(struct device_driver *ddri, const char *buf, size_t count)
{
	int addr;
	int ret;
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}
	ret = kstrtoint(buf, 16, &addr);
	if (ret < 0) {
		APS_ERR("invalid format: '%s'\n", buf);
		return 0;
	}

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	if (obj->hw) {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: %d, (%d %d)\n",
				obj->hw->i2c_num, obj->hw->power_id,
				obj->hw->power_vol);
	} else {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "REGS: %02X %02X %02X %02lX %02lX\n",
			atomic_read(&obj->als_cmd_val),
			atomic_read(&obj->ps_cmd_val),
			atomic_read(&obj->ps_thd_val), obj->enable,
			obj->pending_intr);

	len +=
	    snprintf(buf + len, PAGE_SIZE - len, "MISC: %d %d\n",
		     atomic_read(&obj->als_suspend), atomic_read(&obj->ps_suspend));

	return len;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct cm36686_priv *obj, const char *buf, size_t count, u32 data[],
			     int len)
{
	int idx = 0;
	int ret;
	char *cur = (char *)buf, *end = (char *)(buf + count);

	while (idx < len) {
		while ((cur < end) && IS_SPACE(*cur))
			cur++;

		ret = kstrtoint(cur, 10, &data[idx]);
		if (ret < 0)
			break;

		idx++;
		while ((cur < end) && !IS_SPACE(*cur))
			cur++;
	}
	return idx;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_alslv(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	for (idx = 0; idx < obj->als_level_num; idx++)
		len += snprintf(buf + len, PAGE_SIZE - len, "%d ", obj->hw->als_level[idx]);
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_alslv(struct device_driver *ddri, const char *buf, size_t count)
{
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	} else if (!strcmp(buf, "def")) {
		memcpy(obj->als_level, obj->hw->als_level,
		       sizeof(obj->als_level));
	} else if (obj->als_level_num !=
		   read_int_from_buf(obj, buf, count, obj->hw->als_level,
				     obj->als_level_num)) {
		APS_ERR("invalid format: '%s'\n", buf);
	}
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_alsval(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	int idx;
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	for (idx = 0; idx < obj->als_value_num; idx++)
		len += snprintf(buf + len, PAGE_SIZE - len, "%d ", obj->hw->als_value[idx]);
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_alsval(struct device_driver *ddri, const char *buf, size_t count)
{
	static struct cm36686_priv *obj;

	if (dev_idx > CMP_DEVICE_NUM || dev_idx < 0) {
		APS_ERR("no device with index: %d\n", dev_idx);
		return 0;
	}

	obj = cm36686_obj[dev_idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	} else if (!strcmp(buf, "def")) {
		memcpy(obj->als_value, obj->hw->als_value,
		       sizeof(obj->als_value));
	} else if (obj->als_value_num !=
		   read_int_from_buf(obj, buf, count, obj->hw->als_value,
				     obj->als_value_num)) {
		APS_ERR("invalid format: '%s'\n", buf);
	}
	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_dev_idx(struct device_driver *ddri, char *buf)
{
	int len;

	len = snprintf(buf, PAGE_SIZE, "%d ", dev_idx);
	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_dev_idx(struct device_driver *ddri, const char *buf, size_t count)
{
	int tmp = -1;
	int res = 0;

	res = kstrtoint(buf, 10, &tmp);
	if (tmp < CMP_DEVICE_NUM || tmp >= 0)
		dev_idx = tmp;

	return count;
}

/*---------------------------------------------------------------------------------------*/
static DRIVER_ATTR(als, S_IWUSR | S_IRUGO, cm36686_show_als, NULL);
static DRIVER_ATTR(ps, S_IWUSR | S_IRUGO, cm36686_show_ps, NULL);
static DRIVER_ATTR(config, S_IWUSR | S_IRUGO, cm36686_show_config, cm36686_store_config);
static DRIVER_ATTR(alslv, S_IWUSR | S_IRUGO, cm36686_show_alslv, cm36686_store_alslv);
static DRIVER_ATTR(alsval, S_IWUSR | S_IRUGO, cm36686_show_alsval, cm36686_store_alsval);
static DRIVER_ATTR(trace, S_IWUSR | S_IRUGO, cm36686_show_trace, cm36686_store_trace);
static DRIVER_ATTR(status, S_IWUSR | S_IRUGO, cm36686_show_status, NULL);
static DRIVER_ATTR(send, S_IWUSR | S_IRUGO, cm36686_show_send, cm36686_store_send);
static DRIVER_ATTR(recv, S_IWUSR | S_IRUGO, cm36686_show_recv, cm36686_store_recv);
static DRIVER_ATTR(reg, S_IWUSR | S_IRUGO, cm36686_show_reg, NULL);
static DRIVER_ATTR(device_idx, S_IWUSR | S_IRUGO, cm36686_show_dev_idx, cm36686_store_dev_idx);

/*----------------------------------------------------------------------------*/
static struct driver_attribute *cm36686_attr_list[] = {
	&driver_attr_als,
	&driver_attr_ps,
	&driver_attr_trace,	/*trace log */
	&driver_attr_config,
	&driver_attr_alslv,
	&driver_attr_alsval,
	&driver_attr_status,
	&driver_attr_send,
	&driver_attr_recv,
	&driver_attr_reg,
	&driver_attr_device_idx,
};

/*----------------------------------------------------------------------------*/
static int cm36686_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(ARRAY_SIZE(cm36686_attr_list));

	if (driver == NULL)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, cm36686_attr_list[idx]);
		if (err) {
			APS_ERR("driver_create_file (%s) = %d\n", cm36686_attr_list[idx]->attr.name,
				err);
			break;
		}
	}
	return err;
}

/*----------------------------------------------------------------------------*/
static int cm36686_delete_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(ARRAY_SIZE(cm36686_attr_list));

	if (!driver)
		return -EINVAL;

	for (idx = 0; idx < num; idx++)
		driver_remove_file(driver, cm36686_attr_list[idx]);

	return err;
}
#endif

#ifdef DEVICE_ATTRIBUTE_ENABLE
/*-------------------------------attribute file for debugging----------------------------------*/

/******************************************************************************
 * Sysfs attributes
*******************************************************************************/
static ssize_t cm36686_show_config(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	ssize_t res = 0;
	static struct cm36686_priv *obj;
	int idx;

	idx = get_dev_idex_by_dev(dev);
	if (idx > CMP_DEVICE_NUM || idx < 0) {
		APS_ERR("no device with index: %d\n", idx);
		return 0;
	}

	obj = cm36686_obj[idx];
	if (!obj) {
		APS_ERR("cm36686_obj[%d] is null!!\n", idx);
		return res;
	}

	res += snprintf(buf + res, PAGE_SIZE, "(%d %d %d %d %d)\n",
		atomic_read(&obj->i2c_retry),
		atomic_read(&obj->als_debounce),
		atomic_read(&obj->ps_mask),
		atomic_read(&obj->ps_thd_val),
		atomic_read(&obj->ps_debounce));
	return res;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_config(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int retry, als_deb, ps_deb, mask, thres;
	static struct cm36686_priv *obj;
	int idx;

	idx = get_dev_idex_by_dev(dev);
	if (idx > CMP_DEVICE_NUM || idx < 0) {
		APS_ERR("no device with index: %d\n", idx);
		return 0;
	}

	obj = cm36686_obj[idx];

	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	if (sscanf(buf, "%d %d %d %d %d", &retry, &als_deb, &mask, &thres, &ps_deb) == 5) {
		atomic_set(&obj->i2c_retry, retry);
		atomic_set(&obj->als_debounce, als_deb);
		atomic_set(&obj->ps_mask, mask);
		atomic_set(&obj->ps_thd_val, thres);
		atomic_set(&obj->ps_debounce, ps_deb);
	} else {
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);
	}
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_trace(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	ssize_t res;
	static struct cm36686_priv *obj;
	int idx;

	idx = get_dev_idex_by_dev(dev);
	if (idx > CMP_DEVICE_NUM || idx < 0) {
		APS_ERR("no device with index: %d\n", idx);
		return 0;
	}

	obj = cm36686_obj[idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));
	return res;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_trace(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int trace;
	static struct cm36686_priv *obj;
	int idx;

	idx = get_dev_idex_by_dev(dev);
	if (idx > CMP_DEVICE_NUM || idx < 0) {
		APS_ERR("no device with index: %d\n", idx);
		return 0;
	}

	obj = cm36686_obj[idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	if (sscanf(buf, "0x%x", &trace) == 1)
		atomic_set(&obj->trace, trace);
	else
		APS_ERR("invalid content: '%s', length = %zu\n", buf, count);

	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_als(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int res;
	static struct cm36686_priv *obj;
	int idx;
	ssize_t len;

	mutex_lock(&cm36686_mutex);
	idx = get_dev_idex_by_dev(dev);
	if (idx > CMP_DEVICE_NUM || idx < 0) {
		APS_ERR("no device with index: %d\n", idx);
		return 0;
	}

	obj = cm36686_obj[idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}
	res = cm36686_read_als(obj->client, &obj->als);
	if (res)
		len = snprintf(buf, PAGE_SIZE, "ERROR: %d\n", res);
	else
		len = snprintf(buf, PAGE_SIZE, "0x%04X\n", obj->als);
	mutex_unlock(&cm36686_mutex);

	return len;
}

static ssize_t cm36686_show_reg(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	u8 _bIndex = 0;
	u8 databuf[2] = { 0 };
	ssize_t _tLength = 0;
	int res;
	static struct cm36686_priv *obj;
	int idx;

	idx = get_dev_idex_by_dev(dev);
	if (idx > CMP_DEVICE_NUM || idx < 0) {
		APS_ERR("no device with index: %d\n", idx);
		return 0;
	}

	obj = cm36686_obj[idx];
	if (!obj) {
		APS_ERR("cm3623_obj is null!!\n");
		return 0;
	}

	for (_bIndex = 0; _bIndex < 0x0D; _bIndex++) {
		res = cmp_i2c_read(obj->client, _bIndex, databuf, 2);
		if (res < 0)
			APS_ERR("i2c_master_send function err res = %d\n", res);

		_tLength +=
		    snprintf((buf + _tLength), (PAGE_SIZE - _tLength), "Reg[0x%02X]: 0x%02X\n",
			     _bIndex, databuf[0]);
	}

	return _tLength;

}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_status(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	static struct cm36686_priv *obj;
	int idx;

	idx = get_dev_idex_by_dev(dev);
	if (idx > CMP_DEVICE_NUM || idx < 0) {
		APS_ERR("no device with index: %d\n", idx);
		return 0;
	}

	obj = cm36686_obj[idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	if (obj->hw) {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: %d, (%d %d)\n",
				obj->hw->i2c_num, obj->hw->power_id,
				obj->hw->power_vol);
	} else {
		len += snprintf(buf + len, PAGE_SIZE - len, "CUST: NULL\n");
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "REGS: %02X %02X %02X %02lX %02lX\n",
			atomic_read(&obj->als_cmd_val),
			atomic_read(&obj->ps_cmd_val),
			atomic_read(&obj->ps_thd_val), obj->enable,
			obj->pending_intr);

	len +=
	    snprintf(buf + len, PAGE_SIZE - len, "MISC: %d %d\n",
		     atomic_read(&obj->als_suspend), atomic_read(&obj->ps_suspend));

	return len;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#define IS_SPACE(CH) (((CH) == ' ') || ((CH) == '\n'))
/*----------------------------------------------------------------------------*/
static int read_int_from_buf(struct cm36686_priv *obj, const char *buf, size_t count, u32 data[],
			     int len)
{
	int idx = 0;
	int ret;
	char *cur = (char *)buf, *end = (char *)(buf + count);

	while (idx < len) {
		while ((cur < end) && IS_SPACE(*cur))
			cur++;

		ret = kstrtoint(cur, 10, &data[idx]);
		if (ret < 0)
			break;

		idx++;
		while ((cur < end) && !IS_SPACE(*cur))
			cur++;
	}
	return idx;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_alslv(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	int idx;
	static struct cm36686_priv *obj;

	idx = get_dev_idex_by_dev(dev);
	if (idx > CMP_DEVICE_NUM || idx < 0) {
		APS_ERR("no device with index: %d\n", idx);
		return 0;
	}

	obj = cm36686_obj[idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	for (idx = 0; idx < obj->als_level_num; idx++)
		len += snprintf(buf + len, PAGE_SIZE - len, "%d ", obj->hw->als_level[idx]);
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_alslv(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	static struct cm36686_priv *obj;
	u8 idx = -1;

	idx = get_dev_idex_by_dev(dev);
	if (idx > CMP_DEVICE_NUM || idx < 0) {
		APS_ERR("no device with index: %d\n", idx);
		return 0;
	}

	obj = cm36686_obj[idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	} else if (!strcmp(buf, "def")) {
		memcpy(obj->als_level, obj->hw->als_level,
		       sizeof(obj->als_level));
	} else if (obj->als_level_num !=
		   read_int_from_buf(obj, buf, count, obj->hw->als_level,
				     obj->als_level_num)) {
		APS_ERR("invalid format: '%s'\n", buf);
	}
	return count;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_show_alsval(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	int idx;
	static struct cm36686_priv *obj;

	idx = get_dev_idex_by_dev(dev);
	if (idx > CMP_DEVICE_NUM || idx < 0) {
		APS_ERR("no device with index: %d\n", idx);
		return 0;
	}

	obj = cm36686_obj[idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	}

	for (idx = 0; idx < obj->als_value_num; idx++)
		len += snprintf(buf + len, PAGE_SIZE - len, "%d ", obj->hw->als_value[idx]);
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t cm36686_store_alsval(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t count)
{
	static struct cm36686_priv *obj;
	u8 idx = -1;

	idx = get_dev_idex_by_dev(dev);
	if (idx > CMP_DEVICE_NUM || idx < 0) {
		APS_ERR("no device with index: %d\n", idx);
		return 0;
	}

	obj = cm36686_obj[idx];
	if (!obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return 0;
	} else if (!strcmp(buf, "def")) {
		memcpy(obj->als_value, obj->hw->als_value,
		       sizeof(obj->als_value));
	} else if (obj->als_value_num !=
		   read_int_from_buf(obj, buf, count, obj->hw->als_value,
				     obj->als_value_num)) {
		APS_ERR("invalid format: '%s'\n", buf);
	}
	return count;
}

/*---------------------------------------------------------------------------------------*/
static DEVICE_ATTR(als, S_IWUSR | S_IRUGO, cm36686_show_als, NULL);
static DEVICE_ATTR(config, S_IWUSR | S_IRUGO, cm36686_show_config, cm36686_store_config);
static DEVICE_ATTR(alslv, S_IWUSR | S_IRUGO, cm36686_show_alslv, cm36686_store_alslv);
static DEVICE_ATTR(alsval, S_IWUSR | S_IRUGO, cm36686_show_alsval, cm36686_store_alsval);
static DEVICE_ATTR(trace, S_IWUSR | S_IRUGO, cm36686_show_trace, cm36686_store_trace);
static DEVICE_ATTR(status, S_IWUSR | S_IRUGO, cm36686_show_status, NULL);
static DEVICE_ATTR(reg, S_IWUSR | S_IRUGO, cm36686_show_reg, NULL);
/*----------------------------------------------------------------------------*/
static struct attribute *cm36686_attr_list[] = {
	&dev_attr_als.attr,
	&dev_attr_trace.attr,
	&dev_attr_config.attr,
	&dev_attr_alslv.attr,
	&dev_attr_alsval.attr,
	&dev_attr_status.attr,
	&dev_attr_reg.attr,
	NULL
};

static struct attribute_group cm36686_attr_group = {
	.attrs = cm36686_attr_list
};

/*----------------------------------------------------------------------------*/
static int cm36686_create_attr(struct device *dev)
{
	int err = 0;

	if (dev == NULL)
		return -EINVAL;

	err = sysfs_create_group(&(dev->kobj),	&cm36686_attr_group);
	if (err)
		APS_ERR("device attr create fail: %d\n", err);

	return err;
}

/*----------------------------------------------------------------------------*/
static int cm36686_delete_attr(struct device *dev)
{
	int err = 0;

	if (!dev)
		return -EINVAL;

	err = sysfs_create_group(&(dev->kobj), &cm36686_attr_group);
	return err;
}
#endif

/*----------------------------------------------------------------------------*/
#ifdef CM36686_PS_EINT_ENABLE
/*----------------------------------interrupt functions--------------------------------*/
static int cm36686_check_intr(struct i2c_client *client)
{
	int res;
	u8 databuf[2];

	databuf[0] = CM36686_REG_PS_DATA;
	res = cmp_i2c_read(client, CM36686_REG_PS_DATA, databuf, 2);
	if (res < 0) {
		APS_ERR("read ps data fail: %d\n", res);
		goto EXIT_ERR;
	}

	APS_LOG("PS_DATA value low = %x, high = %x\n", databuf[0], databuf[1]);

	res = cmp_i2c_read(client, CM36686_REG_INT_FLAG, databuf, 2);
	if (res < 0) {
		APS_ERR("read int flag fail: %d\n", res);
		goto EXIT_ERR;
	}

	APS_LOG("INT_FLAG value: low = %x, high = %x\n", databuf[0], databuf[1]);

	if (databuf[1] & 0x02) {
		intr_flag = 0;	/* for close */
	} else if (databuf[1] & 0x01) {
		intr_flag = 1;	/* for away */
	} else {
		res = -1;
		APS_ERR("cm36686_check_intr fail databuf[1]&0x01: %d\n", res);
		goto EXIT_ERR;
	}

	return 0;
EXIT_ERR:
	APS_ERR("cm36686_check_intr dev: %d\n", res);
	return res;
}

/*----------------------------------------------------------------------------*/
static void cm36686_eint_work(struct work_struct *work)
{
	struct cm36686_priv *obj =
	    (struct cm36686_priv *)container_of(work, struct cm36686_priv, eint_work);
	int res = 0;
	int i;

	res = cm36686_check_intr(obj->client);
	if (res) {
		goto EXIT_INTR_ERR;
	} else {
		APS_LOG("cm36686 interrupt value = %d\n", intr_flag);
		res = ps_report_interrupt_data(intr_flag);
	}
	for (i = 0; i < CMP_DEVICE_NUM; i++)
		enable_irq(cm36686_obj[i]->irq);
	return;

EXIT_INTR_ERR:
	for (i = 0; i < CMP_DEVICE_NUM; i++)
		enable_irq(cm36686_obj[i]->irq);
	APS_ERR("cm36686_eint_work err: %d\n", res);
}

/*----------------------------------------------------------------------------*/
static void cm36686_eint_func(int irq)
{
	struct cm36686_priv *obj = NULL;
	int i;

	for (i = 0; i < CMP_DEVICE_NUM; i++) {
		if (irq == cm36686_obj[i]->irq)
		obj = cm36686_obj[i];
		break;
	}

	if (!obj)
		return;
	int_top_time = sched_clock();
	schedule_work(&obj->eint_work);
}

#if defined(CONFIG_OF)
static irqreturn_t cm36686_eint_handler(int irq, void *desc)
{
	int i;

	for (i = 0; i < CMP_DEVICE_NUM; i++)
		disable_irq_nosync(cm36686_obj[i]->irq);
	cm36686_eint_func(irq);

	return IRQ_HANDLED;
}
#endif
/*----------------------------------------------------------------------------*/
int cm36686_setup_eint(struct i2c_client *client)
{
#if defined(CONFIG_OF)
	u32 ints[2] = { 0, 0 };
#endif
	int ret;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct device_node *irq_node;
	int idx = -1;
	struct cm36686_priv *obj;

	idx = get_dev_idex_by_client(client);
	if (idx > CMP_DEVICE_NUM || idx < 0) {
		APS_ERR("no device with index: %d\n", idx);
		return -1;
	}

	obj = cm36686_obj[idx];
	irq_node = obj->irq_node;
	alspsPltFmDev = get_alsps_platformdev();
	pinctrl = devm_pinctrl_get(&alspsPltFmDev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		APS_ERR("Cannot find alsps pinctrl!\n");
		return -EINVAL;
	}
	pins_default = pinctrl_lookup_state(pinctrl, "default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		APS_ERR("Cannot find alsps pinctrl default!\n");
		return -EINVAL;
	}
	pinctrl_select_state(pinctrl, pins_default);

	if (irq_node) {
		ret = of_property_read_u32_array(irq_node, "debounce", ints,
					   ARRAY_SIZE(ints));
		if (ret == 0) {
			gpio_request(ints[0], "p-sensor");
			gpio_set_debounce(ints[0], ints[1]);
			APS_LOG("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);
		}
		obj->irq = irq_of_parse_and_map(irq_node, 0);
		APS_LOG("irq = %d\n", obj->irq);
		if (!obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		ret = request_irq(obj->irq, cm36686_eint_handler,
				IRQF_TRIGGER_NONE, "ALS-eint", NULL);
		if (ret) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
	} else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}
	return 0;
}
#endif

/*-------------------------------MISC device related------------------------------------------*/

/************************************************************/
static int cm36686_open_1(struct inode *inode, struct file *file)
{
	file->private_data = cm36686_i2c_client[0];

	if (!file->private_data) {
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}

static int cm36686_open_2(struct inode *inode, struct file *file)
{
	file->private_data = cm36686_i2c_client[1];

	if (!file->private_data) {
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}
	return nonseekable_open(inode, file);
}

/************************************************************/
static int cm36686_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

/************************************************************/
static int set_psensor_threshold(struct i2c_client *client)
{
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	int res = 0;
	u8 databuf[2];

	APS_LOG("set_psensor_threshold function high: 0x%x, low:0x%x\n",
		atomic_read(&obj->ps_thd_val_high), atomic_read(&obj->ps_thd_val_low));
	databuf[0] = atomic_read(&obj->ps_thd_val_low);
	databuf[1] = atomic_read(&obj->ps_thd_val_low) >> 8;
	res = cmp_i2c_write(client, CM36686_REG_PS_THDL, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		return -1;
	}

	databuf[0] = atomic_read(&obj->ps_thd_val_high);
	databuf[1] = atomic_read(&obj->ps_thd_val_high) >> 8;
	res = cmp_i2c_write(client, CM36686_REG_PS_THDH, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		return -1;
	}
	return 0;

}

static long cm36686_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	long err = 0;
	void __user *ptr = (void __user *)arg;
	int dat;
	uint32_t enable;
	int ps_result;
	int ps_cali;
	int threshold[2];

	mutex_lock(&cm36686_mutex);
	switch (cmd) {
	case ALSPS_SET_PS_MODE:
		if (copy_from_user(&enable, ptr, sizeof(enable))) {
			err = -EFAULT;
			goto err_out;
		}
		if (enable) {
			err = cm36686_enable_ps(obj->client, 1);
			if (err) {
				APS_ERR("enable ps fail: %ld\n", err);
				goto err_out;
			}

			set_bit(CMC_BIT_PS, &obj->enable);
		} else {
			err = cm36686_enable_ps(obj->client, 0);
			if (err) {
				APS_ERR("disable ps fail: %ld\n", err);
				goto err_out;
			}
			clear_bit(CMC_BIT_PS, &obj->enable);
		}
		break;

	case ALSPS_GET_PS_MODE:
		enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
		if (copy_to_user(ptr, &enable, sizeof(enable))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_PS_DATA:
		err = cm36686_read_ps(obj->client, &obj->ps);
		if (err)
			goto err_out;

		dat = cm36686_get_ps_value(obj, obj->ps);
		if (copy_to_user(ptr, &dat, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_PS_RAW_DATA:
		err = cm36686_read_ps(obj->client, &obj->ps);
		if (err)
			goto err_out;

		dat = obj->ps;
		if (copy_to_user(ptr, &dat, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_SET_ALS_MODE:

		if (copy_from_user(&enable, ptr, sizeof(enable))) {
			err = -EFAULT;
			goto err_out;
		}
		if (enable) {
			err = cm36686_enable_als(obj->client, 1);
			if (err) {
				APS_ERR("enable als fail: %ld\n", err);
				goto err_out;
			}
			set_bit(CMC_BIT_ALS, &obj->enable);
		} else {
			err = cm36686_enable_als(obj->client, 0);
			if (err) {
				APS_ERR("disable als fail: %ld\n", err);
				goto err_out;
			}
			clear_bit(CMC_BIT_ALS, &obj->enable);
		}
		break;

	case ALSPS_GET_ALS_MODE:
		enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
		if (copy_to_user(ptr, &enable, sizeof(enable))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_ALS_DATA:
		err = cm36686_read_als(obj->client, &obj->als);
		if (err)
			goto err_out;

		dat = cm36686_get_als_value(obj, obj->als);
		if (copy_to_user(ptr, &dat, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_ALS_RAW_DATA:
		err = cm36686_read_als(obj->client, &obj->als);
		if (err)
			goto err_out;

		dat = obj->als;
		if (copy_to_user(ptr, &dat, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

		/*----------------------------------for factory mode test---------------------------------------*/
	case ALSPS_GET_PS_TEST_RESULT:
		err = cm36686_read_ps(obj->client, &obj->ps);
		if (err)
			goto err_out;

		if (obj->ps > atomic_read(&obj->ps_thd_val_low))
			ps_result = 0;
		else
			ps_result = 1;

		if (copy_to_user(ptr, &ps_result, sizeof(ps_result))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_IOCTL_CLR_CALI:
		if (copy_from_user(&dat, ptr, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		if (dat == 0)
			obj->ps_cali = 0;

		break;

	case ALSPS_IOCTL_GET_CALI:
		ps_cali = obj->ps_cali;
		if (copy_to_user(ptr, &ps_cali, sizeof(ps_cali))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_IOCTL_SET_CALI:
		if (copy_from_user(&ps_cali, ptr, sizeof(ps_cali))) {
			err = -EFAULT;
			goto err_out;
		}

		obj->ps_cali = ps_cali;
		break;

	case ALSPS_SET_PS_THRESHOLD:
		if (copy_from_user(threshold, ptr, sizeof(threshold))) {
			err = -EFAULT;
			goto err_out;
		}
		APS_LOG("%s set threshold high: 0x%x, low: 0x%x\n", __func__, threshold[0],
			threshold[1]);
		atomic_set(&obj->ps_thd_val_high, (threshold[0] + obj->ps_cali));
		atomic_set(&obj->ps_thd_val_low, (threshold[1] + obj->ps_cali));	/* need to confirm */

		set_psensor_threshold(obj->client);

		break;

	case ALSPS_GET_PS_THRESHOLD_HIGH:
		threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
		APS_LOG("%s get threshold high: 0x%x\n", __func__, threshold[0]);
		if (copy_to_user(ptr, &threshold[0], sizeof(threshold[0]))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_PS_THRESHOLD_LOW:
		threshold[0] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
		APS_LOG("%s get threshold low: 0x%x\n", __func__, threshold[0]);
		if (copy_to_user(ptr, &threshold[0], sizeof(threshold[0]))) {
			err = -EFAULT;
			goto err_out;
		}
		break;
		/*------------------------------------------------------------------------------------------*/

	default:
		APS_ERR("%s not supported = 0x%04x", __func__, cmd);
		err = -ENOIOCTLCMD;
		break;
	}

err_out:
	mutex_unlock(&cm36686_mutex);

	return err;
}

#ifdef CONFIG_COMPAT
static long compat_cm36686_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long arg_ioctl = (unsigned long)compat_ptr(arg);

	APS_FUN();
	if (!filp->f_op || !filp->f_op->unlocked_ioctl) {
		APS_ERR("compat_ion_ioctl file has no f_op or no f_op->unlocked_ioctl.\n");
		return -ENOTTY;
	}

	switch (cmd) {
	case COMPAT_ALSPS_SET_PS_MODE:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_SET_PS_MODE, arg_ioctl);
	case COMPAT_ALSPS_GET_PS_MODE:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_GET_PS_MODE, arg_ioctl);
	case COMPAT_ALSPS_GET_PS_DATA:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_GET_PS_DATA, arg_ioctl);
	case COMPAT_ALSPS_GET_PS_RAW_DATA:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_GET_PS_RAW_DATA, arg_ioctl);
	case COMPAT_ALSPS_SET_ALS_MODE:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_SET_ALS_MODE, arg_ioctl);
	case COMPAT_ALSPS_GET_ALS_MODE:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_GET_ALS_MODE, arg_ioctl);
	case COMPAT_ALSPS_GET_ALS_DATA:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_GET_ALS_DATA, arg_ioctl);
	case COMPAT_ALSPS_GET_ALS_RAW_DATA:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_GET_ALS_RAW_DATA, arg_ioctl);
	case COMPAT_ALSPS_GET_PS_TEST_RESULT:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_GET_PS_TEST_RESULT, arg_ioctl);
	case COMPAT_ALSPS_GET_ALS_TEST_RESULT:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_GET_ALS_TEST_RESULT, arg_ioctl);
	case COMPAT_ALSPS_GET_PS_THRESHOLD_HIGH:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_GET_PS_THRESHOLD_HIGH, arg_ioctl);
	case COMPAT_ALSPS_GET_PS_THRESHOLD_LOW:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_GET_PS_THRESHOLD_LOW, arg_ioctl);
	case COMPAT_ALSPS_GET_ALS_THRESHOLD_HIGH:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_GET_ALS_THRESHOLD_HIGH, arg_ioctl);
	case COMPAT_ALSPS_GET_ALS_THRESHOLD_LOW:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_GET_ALS_THRESHOLD_LOW, arg_ioctl);
	case COMPAT_ALSPS_IOCTL_CLR_CALI:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_IOCTL_CLR_CALI, arg_ioctl);
	case COMPAT_ALSPS_IOCTL_GET_CALI:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_IOCTL_GET_CALI, arg_ioctl);
	case COMPAT_ALSPS_IOCTL_SET_CALI:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_IOCTL_SET_CALI, arg_ioctl);
	case COMPAT_ALSPS_SET_PS_THRESHOLD:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_SET_PS_THRESHOLD, arg_ioctl);
	case COMPAT_ALSPS_SET_ALS_THRESHOLD:
		return filp->f_op->unlocked_ioctl(filp, ALSPS_SET_ALS_THRESHOLD, arg_ioctl);
	case COMPAT_AAL_SET_ALS_MODE:
		return filp->f_op->unlocked_ioctl(filp, AAL_SET_ALS_MODE, arg_ioctl);
	case COMPAT_AAL_GET_ALS_MODE:
		return filp->f_op->unlocked_ioctl(filp, AAL_GET_ALS_MODE, arg_ioctl);
	case COMPAT_AAL_GET_ALS_DATA:
		return filp->f_op->unlocked_ioctl(filp, AAL_GET_ALS_DATA, arg_ioctl);
	default:
		APS_ERR("compat_ion_ioctl : No such command!! 0x%x\n", cmd);
		return -ENOIOCTLCMD;
	}
}
#endif
/********************************************************************/
/*------------------------------misc device related operation functions------------------------------------*/
static const struct file_operations cm36686_fops_1 = {
	.owner = THIS_MODULE,
	.open = cm36686_open_1,
	.release = cm36686_release,
	.unlocked_ioctl = cm36686_unlocked_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl = compat_cm36686_unlocked_ioctl,
#endif
};

static const struct file_operations cm36686_fops_2 = {
	.owner = THIS_MODULE,
	.open = cm36686_open_2,
	.release = cm36686_release,
	.unlocked_ioctl = cm36686_unlocked_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl = compat_cm36686_unlocked_ioctl,
#endif
};


static struct miscdevice cm36686_device_1 = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps_1",
	.fops = &cm36686_fops_1,
};

static struct miscdevice cm36686_device_2 = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps_2",
	.fops = &cm36686_fops_2,
};

/*--------------------------------------------------------------------------------*/
static int cm36686_init_client(struct i2c_client *client)
{
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	u8 databuf[2];
	int res = 0;

	APS_FUN();
	if (obj->hw->polling_mode_als == 1)
		databuf[0] = 0x41; /*0b01000001;*/
	else
		databuf[0] = 0x47; /*0b01000111;*/
	databuf[1] = 0x00; /*0b00000000;*/
	res = cmp_i2c_write(client, CM36686_REG_ALS_CONF, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	APS_LOG("cm36686 ps CM36686_REG_ALS_CONF command!\n");

	databuf[0] = 0x63; /*0b01100011;*/
	if (obj->hw->polling_mode_ps == 1)
		databuf[1] = 0x08; /*0b00001000;*/
	else
		databuf[1] = 0x0B; /*0b00001011;*/
	res = cmp_i2c_write(client, CM36686_REG_PS_CONF1_2, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	APS_LOG("cm36686 ps CM36686_REG_PS_CONF1_2 command!\n");

	databuf[0] = 0x14; /*0b00010100;*/
	databuf[1] = 0x02; /*0b00000010;*/
	res = cmp_i2c_write(client, CM36686_REG_PS_CONF3_MS, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}
	APS_LOG("cm36686 ps CM36686_REG_PS_CONF3_MS command!\n");

	databuf[0] = 0x00;
	databuf[1] = 0x00;
	res = cmp_i2c_write(client, CM36686_REG_PS_CANC, databuf, 2);
	if (res < 0) {
		APS_ERR("i2c_master_send function err\n");
		goto EXIT_ERR;
	}

	APS_LOG("cm36686 ps CM36686_REG_PS_CANC command!\n");

	if (obj->hw->polling_mode_als == 0) {
		databuf[0] = 0x00;
		databuf[1] = atomic_read(&obj->als_thd_val_high);
		res = cmp_i2c_write(client, CM36686_REG_ALS_THDH, databuf, 2);
			if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}
		databuf[0] = 0x00;
		databuf[1] = atomic_read(&obj->als_thd_val_low);	/* threshold value need to confirm */
		res = cmp_i2c_write(client, CM36686_REG_ALS_THDL, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}
	}

	if (obj->hw->polling_mode_ps == 0) {
		databuf[0] = atomic_read(&obj->ps_thd_val_low);
		databuf[1] = atomic_read(&obj->ps_thd_val_low) >> 8;
		res = cmp_i2c_write(client, CM36686_REG_PS_THDL, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}

		databuf[0] = (u8)atomic_read(&obj->ps_thd_val_high);
		databuf[1] = (u8)atomic_read(&obj->ps_thd_val_high) >> 8;
		res = cmp_i2c_write(client, CM36686_REG_PS_THDH, databuf, 2);
		if (res < 0) {
			APS_ERR("i2c_master_send function err\n");
			goto EXIT_ERR;
		}
	}
#ifdef CM36686_PS_EINT_ENABLE
	res = cm36686_setup_eint(client);
	if (res) {
		APS_ERR("setup eint: %d\n", res);
		return res;
	}
#endif
	return CM36686_SUCCESS;

EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return res;
}

/*--------------------------------------------------------------------------------*/
#if 0
/* if use  this typ of enable , sensor should report inputEvent(val ,stats, div) to HAL */
static int als_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

static int als_enable_nodata(int en)
{
	int res = 0;

	APS_LOG("cm36686_obj als enable value = %d\n", en);

	mutex_lock(&cm36686_mutex);
	if (en)
		set_bit(CMC_BIT_ALS, &cm36686_obj->enable);
	else
		clear_bit(CMC_BIT_ALS, &cm36686_obj->enable);
	mutex_unlock(&cm36686_mutex);

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return -1;
	}

	res = cm36686_enable_als(cm36686_obj->client, en);
	if (res) {
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}

	return 0;
}

static int als_set_delay(u64 ns)
{
	return 0;
}

static int als_get_data(int *value, int *status)
{
	int err = 0;
	struct cm36686_priv *obj = NULL;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return -1;
	}
	obj = cm36686_obj;
	err = cm36686_read_als(obj->client, &obj->als);
	if (err) {
		err = -1;
	} else {
		*value = cm36686_get_als_value(obj, obj->als);
		if (*value < 0)
			err = -1;
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	return err;
}

/* if use  this typ of enable , sensor should report inputEvent(val ,stats, div) to HAL */
static int ps_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

static int ps_enable_nodata(int en)
{
	int res = 0;

	APS_LOG("cm36686_obj als enable value = %d\n", en);

	mutex_lock(&cm36686_mutex);
	if (en)
		set_bit(CMC_BIT_PS, &cm36686_obj->enable);
	else
		clear_bit(CMC_BIT_PS, &cm36686_obj->enable);
	mutex_unlock(&cm36686_mutex);
	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return -1;
	}
	res = cm36686_enable_ps(cm36686_obj->client, en);

	if (res) {
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}
	return 0;

}

static int ps_set_delay(u64 ns)
{
	return 0;
}

static int ps_get_data(int *value, int *status)
{
	int err = 0;

	if (!cm36686_obj) {
		APS_ERR("cm36686_obj is null!!\n");
		return -1;
	}

	err = cm36686_read_ps(cm36686_obj->client, &cm36686_obj->ps);
	if (err) {
		err = -1;
	} else {
		*value = cm36686_get_ps_value(cm36686_obj, cm36686_obj->ps);
		if (*value < 0)
			err = -1;
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	return err;
}
#endif

/*-----------------------------------i2c operations----------------------------------*/
static int cm36686_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cm36686_priv *obj;
	int err = 0;
/*	struct als_control_path als_ctl = { 0 };
	struct als_data_path als_data = { 0 };
	struct ps_control_path ps_ctl = { 0 };
	struct ps_data_path ps_data = { 0 };
*/
	int idx = -1;
	int err_cnt;

	APS_FUN();
	idx = get_dev_idex_by_client(client);
	if (idx < 0 || idx > CMP_DEVICE_NUM) {
		APS_ERR("No Device with index:%d\n", idx);
		return -1;
	}
	if (!(hw[idx]))
		return -1;

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj) {
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));

	cm36686_obj[idx] = obj;
	obj->hw = hw[idx];

#ifdef CM36686_PS_EINT_ENABLE
	INIT_WORK(&obj->eint_work, cm36686_eint_work);
#endif
	obj->client = client;
	i2c_set_clientdata(client, obj);

	/*-----------------------------value need to be confirmed-----------------------------------------*/
	atomic_set(&obj->als_debounce, 200);
	atomic_set(&obj->als_deb_on, 0);
#ifdef CONFIG_64BIT
	atomic64_set(&obj->als_deb_end, 0);
#else
	atomic_set(&obj->als_deb_end, 0);
#endif
	atomic_set(&obj->ps_debounce, 200);
	atomic_set(&obj->ps_deb_on, 0);
#ifdef CONFIG_64BIT
	atomic64_set(&obj->ps_deb_end, 0);
#else
	atomic_set(&obj->ps_deb_end, 0);
#endif
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->als_cmd_val, 0xDF);
	atomic_set(&obj->ps_cmd_val, 0xC1);
	atomic_set(&obj->ps_thd_val_high, obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low, obj->hw->ps_threshold_low);
	atomic_set(&obj->als_thd_val_high, obj->hw->als_threshold_high);
	atomic_set(&obj->als_thd_val_low, obj->hw->als_threshold_low);
	atomic_set(&obj->init_done, 0);
	obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, ALS-eint");

	obj->enable = 0;
	obj->pending_intr = 0;
	obj->ps_cali = 0;
	obj->als_level_num = ARRAY_SIZE(obj->hw->als_level);
	obj->als_value_num = ARRAY_SIZE(obj->hw->als_value);
	/*-----------------------------value need to be confirmed-----------------------------------------*/

	WARN_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	WARN_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	clear_bit(CMC_BIT_ALS, &obj->enable);
	clear_bit(CMC_BIT_PS, &obj->enable);

	cm36686_i2c_client[idx] = client;
	if (msg_dma_alloc())
		goto exit;

	err = cm36686_init_client(client);
	if (err)
		goto exit_init_failed;
	APS_LOG("cm36686_init_client() OK!\n");

	if (idx == 0)
		err = misc_register(&cm36686_device_1);
	else
		err = misc_register(&cm36686_device_2);
	if (err) {
		APS_ERR("cm36686_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	/*------------------------cm36686 attribute file for debug--------------------------------------*/
#ifdef DEVICE_ATTRIBUTE_ENABLE
	err = cm36686_create_attr(&(client->dev));
#else
	err = cm36686_create_attr(&(cm36686_init_info.platform_diver_addr->driver));
#endif
	if (err) {
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

#if 0
	als_ctl.is_use_common_factory = false;
	ps_ctl.is_use_common_factory = false;

	als_ctl.open_report_data = als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay = als_set_delay;
	als_ctl.is_report_input_direct = false;
	als_ctl.is_support_batch = false;

	err = als_register_control_path(&als_ctl);
	if (err) {
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	als_data.get_data = als_get_data;
	als_data.vender_div = 100;
	err = als_register_data_path(&als_data);
	if (err) {
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}


	ps_ctl.open_report_data = ps_open_report_data;
	ps_ctl.enable_nodata = ps_enable_nodata;
	ps_ctl.set_delay = ps_set_delay;
	ps_ctl.is_report_input_direct = false;
	ps_ctl.is_support_batch = false;
	ps_ctl.is_polling_mode = obj->hw->polling_mode_ps;

	err = ps_register_control_path(&ps_ctl);
	if (err) {
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	ps_data.get_data = ps_get_data;
	ps_data.vender_div = 100;
	err = ps_register_data_path(&ps_data);
	if (err) {
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	err = batch_register_support_info(ID_LIGHT, als_ctl.is_support_batch, 1, 0);
	if (err)
		APS_ERR("register light batch support err = %d\n", err);

	err = batch_register_support_info(ID_PROXIMITY, ps_ctl.is_support_batch, 1, 0);
	if (err)
		APS_ERR("register proximity batch support err = %d\n", err);
#endif
	cm36686_init_flag = 0;
	APS_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
#if 0
exit_sensor_obj_attach_fail:
#endif
exit_misc_device_register_failed:
	if (idx == 0)
		misc_deregister(&cm36686_device_1);
	else
		misc_deregister(&cm36686_device_2);
exit_init_failed:
	kfree(obj);
exit:
	cm36686_i2c_client[idx] = NULL;
	APS_ERR("%s: err = %d\n", __func__, err);
	err_cnt = 0;
	for (idx = 0; idx < CMP_DEVICE_NUM; idx++) {
		if (cm36686_i2c_client[idx] == NULL)
			err_cnt++;
	}
	if (err_cnt >= CMP_DEVICE_NUM)
		cm36686_init_flag = -1;
	else
		cm36686_init_flag = 0;

	return err;
}

static int cm36686_i2c_remove(struct i2c_client *client)
{
	int err;
	int idx;

	APS_FUN();
	idx = get_dev_idex_by_client(client);
	if (idx < 0 || idx > CMP_DEVICE_NUM)
		return 0;
	/*------------------------cm36686 attribute file for debug--------------------------------------*/
#ifdef DEVICE_ATTRIBUTE_ENABLE
	err = cm36686_delete_attr(&(client->dev));
#else
	err = cm36686_delete_attr(&(cm36686_init_info.platform_diver_addr->driver));
#endif
	if (err)
		APS_ERR("cm36686_delete_attr fail: %d\n", err);
	/*----------------------------------------------------------------------------------------*/

	if (idx == 0)
		misc_deregister(&cm36686_device_1);
	else
		misc_deregister(&cm36686_device_2);

	cm36686_i2c_client[idx] = NULL;
	if (cm36686_i2c_client[0] == NULL && cm36686_i2c_client[1] == NULL)
		msg_dma_release();

	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));
	return 0;

}

static int cm36686_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	int idx;

	idx = get_dev_idex_by_client(client);
	if (idx == 0)
		strncpy(info->type, ALSPS_DEV_NAME1, sizeof(info->type));
	else
		strncpy(info->type, ALSPS_DEV_NAME2, sizeof(info->type));

	return 0;

}

static int cm36686_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	int err;

	APS_FUN();

	if (!obj) {
		APS_ERR("null pointer!!\n");
		return 0;
	}

	atomic_set(&obj->als_suspend, 1);
	err = cm36686_enable_als(obj->client, 0);
	if (err)
		APS_ERR("disable als fail: %d\n", err);
	return 0;
}

static int cm36686_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cm36686_priv *obj = i2c_get_clientdata(client);
	int err;
	struct hwm_sensor_data sensor_data;

	memset(&sensor_data, 0, sizeof(sensor_data));
	APS_FUN();
	if (!obj) {
		APS_ERR("null pointer!!\n");
		return 0;
	}

	atomic_set(&obj->als_suspend, 0);
	if (test_bit(CMC_BIT_ALS, &obj->enable)) {
		err = cm36686_enable_als(obj->client, 1);
		if (err)
			APS_ERR("enable als fail: %d\n", err);
	}
	return 0;
}

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
static int cm36686_remove(void)
{
	int i;

	for (i = 0; i < CMP_DEVICE_NUM; i++)
		cm36686_power(hw[i], 0);

	i2c_del_driver(&cm36686_i2c_driver);
	return 0;
}

/*----------------------------------------------------------------------------*/

static int cm36686_local_init(void)
{
	int i;

	APS_FUN();
	for (i = 0; i < CMP_DEVICE_NUM; i++)
		cm36686_power(hw[i], 1);
	if (i2c_add_driver(&cm36686_i2c_driver)) {
		APS_ERR("add driver error\n");
		return -1;
	}
	if (-1 == cm36686_init_flag) {
		APS_ERR("driver init fail\n");
		return -1;
	}

	return 0;
}


/*----------------------------------------------------------------------------*/
static int __init cm36686_init(void)
{
	const char *name = "mediatek,CM36686";
	char node_name[255];
	int i = 1;
	int res = 0;
	int err = 0;

	APS_FUN();
	for (i = 0; i < CMP_DEVICE_NUM; i++) {
		res = snprintf(node_name, 255, "%s_%d", name, (i + 1)); /*"mediatek, cm36686_1..."*/
		node_name[res] = '\0';
		APS_LOG("node name:%s, %d\n", node_name, res);
		hw[i] = get_alsps_dts_func((const char *)node_name, hw[i]);
		if (!hw[i]) {
			APS_ERR("get_alsps_dts_func fail\n");
			err++;
		}
	}
	if (err >= CMP_DEVICE_NUM)
		return -1;
	alsps_driver_add(&cm36686_init_info);

	return 0;
}

/*----------------------------------------------------------------------------*/
static void __exit cm36686_exit(void)
{
	APS_FUN();
}

/*----------------------------------------------------------------------------*/
module_init(cm36686_init);
module_exit(cm36686_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("andrew yang");
MODULE_DESCRIPTION("cm36686 driver");
MODULE_LICENSE("GPL");
