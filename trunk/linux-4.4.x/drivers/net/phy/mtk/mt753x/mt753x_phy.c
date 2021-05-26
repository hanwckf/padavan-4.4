// SPDX-License-Identifier:	GPL-2.0+
/*
 * Common part for MediaTek MT753x gigabit switch
 *
 * Copyright (C) 2018 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <linux/kernel.h>
#include <linux/delay.h>

#include "mt753x.h"
#include "mt753x_regs.h"
#include "mt753x_phy.h"

u32 tc_phy_read_dev_reg(struct gsw_mt753x *gsw, u32 port_num, u32 dev_addr, u32 reg_addr)
{
	u32 phy_val;
    phy_val = gsw->mmd_read(gsw, port_num, dev_addr, reg_addr);
    
    //printk("switch phy cl45 r %d 0x%x 0x%x = %x\n",port_num, dev_addr, reg_addr, phy_val);
	//switch_phy_read_cl45(port_num, dev_addr, reg_addr, &phy_val);
	return phy_val;
}

void tc_phy_write_dev_reg(struct gsw_mt753x *gsw, u32 port_num, u32 dev_addr, u32 reg_addr, u32 write_data)
{
	u32 phy_val;
    gsw->mmd_write(gsw, port_num, dev_addr, reg_addr, write_data);
    phy_val = gsw->mmd_read(gsw, port_num, dev_addr, reg_addr);
    //printk("switch phy cl45 w %d 0x%x 0x%x 0x%x --> read back 0x%x\n",port_num, dev_addr, reg_addr, write_data, phy_val);
	//switch_phy_write_cl45(port_num, dev_addr, reg_addr, write_data);
}

void switch_phy_write(struct gsw_mt753x *gsw, u32 port_num, u32 reg_addr, u32 write_data){
	gsw->mii_write(gsw, port_num, reg_addr, write_data);
}

u32 switch_phy_read(struct gsw_mt753x *gsw, u32 port_num, u32 reg_addr){
	return gsw->mii_read(gsw, port_num, reg_addr);
}

const u8 MT753x_ZCAL_TO_R50ohm_GE_TBL_100[64] = {
	127, 127, 127, 127, 127, 127, 127, 127,
	127, 127, 127, 127, 127, 123, 122, 117,
	115, 112, 103, 100, 98, 87, 85, 83,
	81, 72, 70, 68, 66, 64, 55, 53,
	52, 50, 49, 48, 38, 36, 35, 34,
	33, 32, 22, 21, 20, 19, 18, 17,
	16, 7, 6, 5, 4, 3, 2, 1,
	0, 0, 0, 0, 0, 0, 0, 0
};

const u8 MT753x_TX_OFFSET_TBL[64] = {
	0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18,
	0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
	0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x8,
	0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f
};

u8 ge_cal_flag;

u8 all_ge_ana_cal_wait(struct gsw_mt753x *gsw, u32 delay, u32 phyaddr) // for EN7512 
{
	u8 all_ana_cal_status;	
	u32 cnt, tmp_1e_17c;
	//tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017c, 0x0001);	// da_calin_flag pull high
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x17c, 0x0001);
	//printk("delay = %d\n", delay);
	
	cnt = 10000;
	do {
		udelay(delay);
		cnt--;
		all_ana_cal_status = tc_phy_read_dev_reg(gsw, PHY0, 0x1e, 0x17b) & 0x1;

	} while ((all_ana_cal_status == 0) && (cnt != 0));


	if(all_ana_cal_status == 1) {
		tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x17c, 0);
		return all_ana_cal_status;
	} else {
		tmp_1e_17c = tc_phy_read_dev_reg(gsw, PHY0, 0x1e, 0x17c);
		if ((tmp_1e_17c & 0x1) != 1) {
			pr_info("FIRST MDC/MDIO write error\n");
			pr_info("FIRST 1e_17c = %x\n", tc_phy_read_dev_reg(gsw, PHY0, 0x1e, 0x17c));

		}
		printk("re-K again\n");
        
		tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x17c, 0);
		tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x17c, 0x0001);
		cnt = 10000;
		do {
			udelay(delay);
			cnt--;
			tmp_1e_17c = tc_phy_read_dev_reg(gsw, PHY0, 0x1e, 0x17c);
			if ((tmp_1e_17c & 0x1) != 1) {
				pr_info("SECOND MDC/MDIO write error\n");
				pr_info("SECOND 1e_17c = %x\n", tc_phy_read_dev_reg(gsw, PHY0, 0x1e, 0x17c));
				tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x17c, 0x0001);
				tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x17c, 0x0001);
				tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x17c, 0x0001);
			}
		} while ((cnt != 0) && (tmp_1e_17c == 0));

		cnt = 10000;
		do {
			udelay(delay);
			cnt--;
			all_ana_cal_status = tc_phy_read_dev_reg(gsw, PHY0, 0x1e, 0x17b) & 0x1;
	
		} while ((all_ana_cal_status == 0) && (cnt != 0));
	
		tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x17c, 0);
	}

    if(all_ana_cal_status == 0){
        pr_info("!!!!!!!!!!!! dev1Eh_reg17b ERROR\n");
    }
	
	return all_ana_cal_status;
}




int ge_cal_rext(struct gsw_mt753x *gsw, u8 phyaddr, u32 delay)
{
	u8 rg_zcal_ctrl, all_ana_cal_status;
	u16 ad_cal_comp_out_init;
	u16 dev1e_e0_ana_cal_r5;
	int calibration_polarity;
	u8 cnt = 0;
	u16 dev1e_17a_tmp, dev1e_e0_tmp;

	/* *** Iext/Rext Cal start ************ */
	all_ana_cal_status = ANACAL_INIT;
	/* analog calibration enable, Rext calibration enable */
	/* 1e_db[12]:rg_cal_ckinv, [8]:rg_ana_calen, [4]:rg_rext_calen, [0]:rg_zcalen_a */
	/* 1e_dc[0]:rg_txvos_calen */
	/* 1e_e1[4]:rg_cal_refsel(0:1.2V) */
	//tc_phy_write_dev_reg(phyaddr, 0x1e, 0x00db, 0x1110)
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00db, 0x1110);
	//tc_phy_write_dev_reg(phyaddr, 0x1e, 0x00dc, 0x0000);
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00dc, 0);
	//tc_phy_write_dev_reg(phyaddr, 0x1e, 0x00e1, 0x0000);
	//tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00e1, 0x10);
	
	rg_zcal_ctrl = 0x20;/* start with 0 dB */
	dev1e_e0_ana_cal_r5 = tc_phy_read_dev_reg(gsw,  PHY0, 0x1e, 0xe0); // get default value
	/* 1e_e0[5:0]:rg_zcal_ctrl */
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0xe0, rg_zcal_ctrl);
	all_ana_cal_status = all_ge_ana_cal_wait(gsw, delay, phyaddr);/* delay 20 usec */

	if (all_ana_cal_status == 0) {
		all_ana_cal_status = ANACAL_ERROR;
		printk(" GE Rext AnaCal ERROR init!   \r\n");
		return -1;
	}
	/* 1e_17a[8]:ad_cal_comp_out */
	ad_cal_comp_out_init = (tc_phy_read_dev_reg(gsw,  PHY0, 0x1e, 0x017a) >> 8) & 0x1;
	if (ad_cal_comp_out_init == 1)
		calibration_polarity = -1;
	else /* ad_cal_comp_out_init == 0 */
		calibration_polarity = 1;
	cnt = 0;
	while (all_ana_cal_status < ANACAL_ERROR) {
		cnt++;
		rg_zcal_ctrl += calibration_polarity;
		tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0xe0, (rg_zcal_ctrl));
		all_ana_cal_status = all_ge_ana_cal_wait(gsw, delay, phyaddr); /* delay 20 usec */
		dev1e_17a_tmp = tc_phy_read_dev_reg(gsw, PHY0, 0x1e, 0x017a);
		if (all_ana_cal_status == 0) {
			all_ana_cal_status = ANACAL_ERROR;
			printk("  GE Rext AnaCal ERROR 2!   \r\n");
			return -1;
		} else if (((dev1e_17a_tmp >> 8) & 0x1) != ad_cal_comp_out_init) {
			all_ana_cal_status = ANACAL_FINISH;
			//printk("  GE Rext AnaCal Done! (%d)(0x%x)  \r\n", cnt, rg_zcal_ctrl);
		} else {
			dev1e_17a_tmp = tc_phy_read_dev_reg(gsw, PHY0, 0x1e, 0x017a);
			dev1e_e0_tmp =	tc_phy_read_dev_reg(gsw, PHY0, 0x1e, 0xe0);
			if ((rg_zcal_ctrl == 0x3F) || (rg_zcal_ctrl == 0x00)) {
				all_ana_cal_status = ANACAL_SATURATION;  /* need to FT(IC fail?) */
				printk(" GE Rext AnaCal Saturation!  \r\n");
				rg_zcal_ctrl = 0x20;  /* 0 dB */
			} 
		}
	}

	if (all_ana_cal_status == ANACAL_ERROR) {
		rg_zcal_ctrl = 0x20;  /* 0 dB */
		tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00e0, (dev1e_e0_ana_cal_r5 | rg_zcal_ctrl));
	} else if(all_ana_cal_status == ANACAL_FINISH){
		//tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00e0, (dev1e_e0_ana_cal_r5 | rg_zcal_ctrl));
		tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00e0, ((rg_zcal_ctrl << 8) | rg_zcal_ctrl));
		printk("0x1e-e0 = %x\n", tc_phy_read_dev_reg(gsw,  PHY0, 0x1e, 0x00e0));
		/* ****  1f_115[2:0] = rg_zcal_ctrl[5:3]  // Mog review */
		tc_phy_write_dev_reg(gsw, PHY0, 0x1f, 0x0115, ((rg_zcal_ctrl & 0x3f) >> 3));
		printk("0x1f-115 = %x\n", tc_phy_read_dev_reg(gsw,  PHY0, 0x1f, 0x115));
		printk("  GE Rext AnaCal Done! (%d)(0x%x)  \r\n", cnt, rg_zcal_ctrl);
		ge_cal_flag = 1;
	} else {
		printk("GE Rxet cal something wrong2\n");
	}
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00db, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00db, 0x0000);
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00dc, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dc, 0x0000);
}

//-----------------------------------------------------------------
int ge_cal_r50(struct gsw_mt753x *gsw, u8 phyaddr, u32 delay)
{
	u8 rg_zcal_ctrl, all_ana_cal_status, calibration_pair;
	u16 ad_cal_comp_out_init;
	u16 dev1e_e0_ana_cal_r5;
	int calibration_polarity;
	u8 cnt = 0;
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00db, 0x1100);	// 1e_db[12]:rg_cal_ckinv, [8]:rg_ana_calen, [4]:rg_rext_calen, [0]:rg_zcalen_a
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00dc, 0x0000);	// 1e_dc[0]:rg_txvos_calen

	for(calibration_pair = ANACAL_PAIR_A; calibration_pair <= ANACAL_PAIR_D; calibration_pair ++) {
		rg_zcal_ctrl = 0x20;  						// start with 0 dB
		dev1e_e0_ana_cal_r5 = (tc_phy_read_dev_reg(gsw,  PHY0, 0x1e, 0x00e0) & (~0x003f));
		tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00e0, (dev1e_e0_ana_cal_r5 | rg_zcal_ctrl));	// 1e_e0[5:0]:rg_zcal_ctrl
		if(calibration_pair == ANACAL_PAIR_A)
		{
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00db, 0x1101);	// 1e_db[12]:rg_cal_ckinv, [8]:rg_ana_calen, [4]:rg_rext_calen, [0]:rg_zcalen_a
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dc, 0x0000);	
			//printk("R50 pair A 1e_db=%x 1e_db=%x\n", tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x00db), tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x00dc));

		}
		else if(calibration_pair == ANACAL_PAIR_B)
		{
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00db, 0x1100);	// 1e_db[12]:rg_cal_ckinv, [8]:rg_ana_calen, [4]:rg_rext_calen, [0]:rg_zcalen_a
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dc, 0x1000);	// 1e_dc[12]:rg_zcalen_b
			//printk("R50 pair B 1e_db=%x 1e_db=%x\n", tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x00db),tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x00dc));

		}
		else if(calibration_pair == ANACAL_PAIR_C)
		{
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00db, 0x1100);	// 1e_db[12]:rg_cal_ckinv, [8]:rg_ana_calen, [4]:rg_rext_calen, [0]:rg_zcalen_a
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dc, 0x0100);	// 1e_dc[8]:rg_zcalen_c
			//printk("R50 pair C 1e_db=%x 1e_db=%x\n", tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x00db), tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x00dc));

		}
		else // if(calibration_pair == ANACAL_PAIR_D)
		{
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00db, 0x1100);	// 1e_db[12]:rg_cal_ckinv, [8]:rg_ana_calen, [4]:rg_rext_calen, [0]:rg_zcalen_a
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dc, 0x0010);	// 1e_dc[4]:rg_zcalen_d
			//printk("R50 pair D 1e_db=%x 1e_db=%x\n", tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x00db), tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x00dc));

		}

		all_ana_cal_status = all_ge_ana_cal_wait(gsw, delay, phyaddr); // delay 20 usec
		if(all_ana_cal_status == 0)
		{
			all_ana_cal_status = ANACAL_ERROR;	
			printk( "GE R50 AnaCal ERROR init!   \r\n");
			return -1;
		}
	
		ad_cal_comp_out_init = (tc_phy_read_dev_reg(gsw,  PHY0, 0x1e, 0x017a)>>8) & 0x1;		// 1e_17a[8]:ad_cal_comp_out	
		if(ad_cal_comp_out_init == 1)
			calibration_polarity = -1;
		else
			calibration_polarity = 1;

		cnt = 0;
		while(all_ana_cal_status < ANACAL_ERROR)
		{
			cnt ++;
			rg_zcal_ctrl += calibration_polarity;
			tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00e0, (dev1e_e0_ana_cal_r5 | rg_zcal_ctrl));
			all_ana_cal_status = all_ge_ana_cal_wait(gsw, delay, phyaddr); // delay 20 usec

			if(all_ana_cal_status == 0)
			{
				all_ana_cal_status = ANACAL_ERROR;	
				printk( "  GE R50 AnaCal ERROR 2!   \r\n");
				return -1;
			}
			else if(((tc_phy_read_dev_reg(gsw,  PHY0, 0x1e, 0x017a)>>8)&0x1) != ad_cal_comp_out_init) 
			{
				all_ana_cal_status = ANACAL_FINISH;	
			}
			else {
				if((rg_zcal_ctrl == 0x3F)||(rg_zcal_ctrl == 0x00))	
				{
					all_ana_cal_status = ANACAL_SATURATION;  // need to FT
					printk( " GE R50 AnaCal Saturation!  \r\n");
				}
			}
		}
		
		if(all_ana_cal_status == ANACAL_ERROR) {	
			rg_zcal_ctrl = 0x20;  // 0 dB
			//tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00e0, (dev1e_e0_ana_cal_r5 | rg_zcal_ctrl));
		}
		else {
			rg_zcal_ctrl = MT753x_ZCAL_TO_R50ohm_GE_TBL_100[rg_zcal_ctrl - 9];	// wait Mog zcal/r50 mapping table
			printk( " GE R50 AnaCal Done! (%d) (0x%x)(0x%x) \r\n", cnt, rg_zcal_ctrl, (rg_zcal_ctrl|0x80));
		}
		
		if(calibration_pair == ANACAL_PAIR_A) {
			ad_cal_comp_out_init = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0174) & (~0x7f00);
			//ad_cal_comp_out_init = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0174);
			//printk( " GE-a 1e_174(0x%x)(0x%x), 1e_175(0x%x)  \r\n", tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0174), ad_cal_comp_out_init, tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0175));
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0174, (ad_cal_comp_out_init | (((rg_zcal_ctrl<<8)&0xff00) | 0x8000)));	// 1e_174[15:8]
			//printk( " GE-a 1e_174(0x%x), 1e_175(0x%x)  \r\n", tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0174), tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0175));
		}
		else if(calibration_pair == ANACAL_PAIR_B) {
			ad_cal_comp_out_init = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0174) & (~0x007f);
			//ad_cal_comp_out_init = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0174);
			//printk( " GE-b 1e_174(0x%x)(0x%x), 1e_175(0x%x)  \r\n", tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0174), ad_cal_comp_out_init, tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0175));
			
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0174, (ad_cal_comp_out_init | (((rg_zcal_ctrl<<0)&0x00ff) | 0x0080)));	// 1e_174[7:0]
			//printk( " GE-b 1e_174(0x%x), 1e_175(0x%x)  \r\n", tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0174), tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0175));
		}
		else if(calibration_pair == ANACAL_PAIR_C) {
			ad_cal_comp_out_init = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0175) & (~0x7f00);
			//ad_cal_comp_out_init = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0175);
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0175, (ad_cal_comp_out_init | (((rg_zcal_ctrl<<8)&0xff00) | 0x8000)));	// 1e_175[15:8]
			//printk( " GE-c 1e_174(0x%x), 1e_175(0x%x)  \r\n", tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0174), tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0175));
		} else {// if(calibration_pair == ANACAL_PAIR_D) 
			ad_cal_comp_out_init = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0175) & (~0x007f);
			//ad_cal_comp_out_init = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0175);
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0175, (ad_cal_comp_out_init | (((rg_zcal_ctrl<<0)&0x00ff) | 0x0080)));	// 1e_175[7:0]
			//printk( " GE-d 1e_174(0x%x), 1e_175(0x%x)  \r\n", tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0174), tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0175));
		}
		//tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00e0, ((rg_zcal_ctrl<<8)|rg_zcal_ctrl));
	}
	
	printk( " GE 1e_174(0x%x), 1e_175(0x%x)  \r\n", tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0174), tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x0175));
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00db, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00db, 0x0000);
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00dc, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dc, 0x0000);
}

int ge_cal_tx_offset(struct gsw_mt753x *gsw,  u8 phyaddr, u32 delay)
{
	u8 all_ana_cal_status, calibration_pair;
	u16 ad_cal_comp_out_init;
	int calibration_polarity, tx_offset_temp;
	u8 tx_offset_reg_shift, tabl_idx, i;
	u8 cnt = 0;
	u16 tx_offset_reg, reg_temp, cal_temp;
	//switch_phy_write(phyaddr, R0, 0x2100);//harry tmp
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00db, 0x0100);	// 1e_db[12]:rg_cal_ckinv, [8]:rg_ana_calen, [4]:rg_rext_calen, [0]:rg_zcalen_a
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00dc, 0x0001);	// 1e_dc[0]:rg_txvos_calen
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0096, 0x8000);	// 1e_96[15]:bypass_tx_offset_cal, Hw bypass, Fw cal
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x003e, 0xf808);	// 1e_3e
	for(i = 0; i <= 4; i++)
		tc_phy_write_dev_reg(gsw, i, 0x1e, 0x00dd, 0x0000);	
	for(calibration_pair = ANACAL_PAIR_A; calibration_pair <= ANACAL_PAIR_D; calibration_pair ++)
	{
		tabl_idx = 31;
		tx_offset_temp = MT753x_TX_OFFSET_TBL[tabl_idx];

		if(calibration_pair == ANACAL_PAIR_A) {
			//tc_phy_write_dev_reg(phyaddr, 0x1e, 0x145, 0x5010);
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dd, 0x1000);				// 1e_dd[12]:rg_txg_calen_a
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017d, (0x8000|DAC_IN_0V));	// 1e_17d:dac_in0_a
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0181, (0x8000|DAC_IN_0V));	// 1e_181:dac_in1_a
			//printk("tx offset pairA 1e_dd = %x, 1e_17d=%x, 1e_181=%x\n", tc_phy_read_dev_reg(phyaddr, 0x1e, 0x00dd), tc_phy_read_dev_reg(phyaddr, 0x1e, 0x017d), tc_phy_read_dev_reg(phyaddr, 0x1e, 0x0181));
			reg_temp = (tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x0172) & (~0x3f00));
			tx_offset_reg_shift = 8;									// 1e_172[13:8]
			tx_offset_reg = 0x0172;

			//tc_phy_write_dev_reg(phyaddr, 0x1e, tx_offset_reg, (reg_temp|(tx_offset_temp<<tx_offset_reg_shift)));
		} else if(calibration_pair == ANACAL_PAIR_B) {
			//tc_phy_write_dev_reg(phyaddr, 0x1e, 0x145, 0x5018);
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dd, 0x0100);				// 1e_dd[8]:rg_txg_calen_b
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017e, (0x8000|DAC_IN_0V));	// 1e_17e:dac_in0_b
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0182, (0x8000|DAC_IN_0V));	// 1e_182:dac_in1_b
			//printk("tx offset pairB 1e_dd = %x, 1e_17d=%x, 1e_181=%x\n", tc_phy_read_dev_reg(phyaddr, 0x1e, 0x00dd), tc_phy_read_dev_reg(phyaddr, 0x1e, 0x017d), tc_phy_read_dev_reg(phyaddr, 0x1e, 0x0181));
			reg_temp = (tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x0172) & (~0x003f));
			tx_offset_reg_shift = 0;									// 1e_172[5:0]
			tx_offset_reg = 0x0172;
			//tc_phy_write_dev_reg(phyaddr, 0x1e, tx_offset_reg, (reg_temp|(tx_offset_temp<<tx_offset_reg_shift)));
		} else if(calibration_pair == ANACAL_PAIR_C) {
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dd, 0x0010);				// 1e_dd[4]:rg_txg_calen_c
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017f, (0x8000|DAC_IN_0V));	// 1e_17f:dac_in0_c
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0183, (0x8000|DAC_IN_0V));	// 1e_183:dac_in1_c
			reg_temp = (tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x0173) & (~0x3f00));
			//printk("tx offset pairC 1e_dd = %x, 1e_17d=%x, 1e_181=%x\n", tc_phy_read_dev_reg(phyaddr, 0x1e, 0x00dd), tc_phy_read_dev_reg(phyaddr, 0x1e, 0x017d), tc_phy_read_dev_reg(phyaddr, 0x1e, 0x0181));
			tx_offset_reg_shift = 8;									// 1e_173[13:8]
			tx_offset_reg = 0x0173;
			//tc_phy_write_dev_reg(phyaddr, 0x1e, tx_offset_reg, (reg_temp|(tx_offset_temp<<tx_offset_reg_shift)));
		} else {// if(calibration_pair == ANACAL_PAIR_D)
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dd, 0x0001);				// 1e_dd[0]:rg_txg_calen_d
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0180, (0x8000|DAC_IN_0V));	// 1e_180:dac_in0_d
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0184, (0x8000|DAC_IN_0V));	// 1e_184:dac_in1_d
			//printk("tx offset pairD 1e_dd = %x, 1e_17d=%x, 1e_181=%x\n", tc_phy_read_dev_reg(phyaddr, 0x1e, 0x00dd), tc_phy_read_dev_reg(phyaddr, 0x1e, 0x017d), tc_phy_read_dev_reg(phyaddr, 0x1e, 0x0181));
			reg_temp = (tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x0173) & (~0x003f));
			tx_offset_reg_shift = 0;									// 1e_173[5:0]
			tx_offset_reg = 0x0173;
			//tc_phy_write_dev_reg(phyaddr, 0x1e, tx_offset_reg, (reg_temp|(tx_offset_temp<<tx_offset_reg_shift)));
		}
		tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_offset_reg, (reg_temp|(tx_offset_temp<<tx_offset_reg_shift)));	// 1e_172, 1e_173
		all_ana_cal_status = all_ge_ana_cal_wait(gsw, delay, phyaddr); // delay 20 usec
		if(all_ana_cal_status == 0) {
			all_ana_cal_status = ANACAL_ERROR;	
			printk( " GE Tx offset AnaCal ERROR init!   \r\n");
			return -1;
		}
	
		ad_cal_comp_out_init = (tc_phy_read_dev_reg(gsw, PHY0, 0x1e, 0x017a)>>8) & 0x1;		// 1e_17a[8]:ad_cal_comp_out	
		if(ad_cal_comp_out_init == 1)
			calibration_polarity = 1;
		else
			calibration_polarity = -1;

		cnt = 0;
		//printk("TX offset cnt = %d, tabl_idx= %x, offset_val = %x\n", cnt, tabl_idx, MT753x_TX_OFFSET_TBL[tabl_idx]);
		while(all_ana_cal_status < ANACAL_ERROR) {
			
			cnt ++;
			tabl_idx += calibration_polarity;
			//tx_offset_temp += calibration_polarity;
			//cal_temp = tx_offset_temp;
			cal_temp = MT753x_TX_OFFSET_TBL[tabl_idx];
			//printk("TX offset cnt = %d, tabl_idx= %x, offset_val = %x\n", cnt, tabl_idx, MT753x_TX_OFFSET_TBL[tabl_idx]);
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_offset_reg, (reg_temp|(cal_temp<<tx_offset_reg_shift)));

			all_ana_cal_status = all_ge_ana_cal_wait(gsw, delay, phyaddr); // delay 20 usec
			if(all_ana_cal_status == 0) {
				all_ana_cal_status = ANACAL_ERROR;	
				printk( " GE Tx offset AnaCal ERROR init 2!   \r\n");
				return -1;
			} else if(((tc_phy_read_dev_reg(gsw, PHY0, 0x1e, 0x017a)>>8)&0x1) != ad_cal_comp_out_init) {
				all_ana_cal_status = ANACAL_FINISH;	
			} else {
				if((tabl_idx == 0)||(tabl_idx == 0x3f)) {
					all_ana_cal_status = ANACAL_SATURATION;  // need to FT
					printk( " GE Tx offset AnaCal Saturation!  \r\n");
				}
			}
		}
		
		if(all_ana_cal_status == ANACAL_ERROR) {	
			tx_offset_temp = TX_AMP_OFFSET_0MV;
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_offset_reg, (reg_temp|(tx_offset_temp<<tx_offset_reg_shift)));
		} else {
			printk( " GE Tx offset AnaCal Done! (pair-%d)(%d)(0x%x) 0x1e_%x=0x%x\n", calibration_pair, cnt, MT753x_TX_OFFSET_TBL[tabl_idx], tx_offset_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_offset_reg));
		}
	}

	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017d, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017e, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017f, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0180, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0181, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0182, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0183, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0184, 0x0000);
	
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00db, 0x0000);	// disable analog calibration circuit
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00dc, 0x0000);	// disable Tx offset calibration circuit
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00db, 0x0000);	// disable analog calibration circuit
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dc, 0x0000);	// disable Tx offset calibration circuit
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x003e, 0x0000);	// disable Tx VLD force mode
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dd, 0x0000);	// disable Tx offset/amplitude calibration circuit	
}

int ge_cal_tx_amp(struct gsw_mt753x *gsw, u8 phyaddr, u32 delay)
{
	u8	all_ana_cal_status, calibration_pair, i;
	u16	ad_cal_comp_out_init;
	int	calibration_polarity;
	u32	tx_amp_reg_shift; 
	u16	reg_temp;
	u32	tx_amp_temp, tx_amp_reg, cnt=0, tx_amp_reg_100;
	u32	debug_tmp, reg_backup, reg_tmp; 
	u32	orig_1e_11, orig_1f_300;

	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00db, 0x1100);	// 1e_db[12]:rg_cal_ckinv, [8]:rg_ana_calen, [4]:rg_rext_calen, [0]:rg_zcalen_a
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00dc, 0x0001);	// 1e_dc[0]:rg_txvos_calen
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00e1, 0x0010);	// 1e_e1[4]:select 1V
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x003e, 0xf808);	// 1e_3e:enable Tx VLD

	orig_1e_11 = tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x11);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x11, 0xff00);
//	tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x27a, 0x33);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0xc9, 0xffff);
	orig_1f_300 = tc_phy_read_dev_reg(gsw, phyaddr, 0x1f, 0x300);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x300, 0x4);
	for(i = 0; i <= 4; i++)
		tc_phy_write_dev_reg(gsw, i, 0x1e, 0x00dd, 0x0000);
	for(calibration_pair = ANACAL_PAIR_A; calibration_pair <= ANACAL_PAIR_D; calibration_pair ++) {
		tx_amp_temp = 0x20;	// start with 0 dB

		if(calibration_pair == ANACAL_PAIR_A) {
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dd, 0x1000);				// 1e_dd[12]:tx_a amp calibration enable
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017d, (0x8000|DAC_IN_2V));	// 1e_17d:dac_in0_a	
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0181, (0x8000|DAC_IN_2V));	// 1e_181:dac_in1_a
			reg_temp = (tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x012) & (~0xfc00));
			tx_amp_reg_shift = 10;										// 1e_12[15:10]
			tx_amp_reg = 0x12;
			tx_amp_reg_100 = 0x16;
		} else if(calibration_pair == ANACAL_PAIR_B) {
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dd, 0x0100);				// 1e_dd[8]:tx_b amp calibration enable
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017e, (0x8000|DAC_IN_2V));	// 1e_17e:dac_in0_b
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0182, (0x8000|DAC_IN_2V));	// 1e_182:dac_in1_b
			reg_temp = (tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x017) & (~0x3f00));
			tx_amp_reg_shift = 8;										// 1e_17[13:8]
			tx_amp_reg = 0x17;
			tx_amp_reg_100 = 0x18;
		} else if(calibration_pair == ANACAL_PAIR_C) {
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dd, 0x0010);				// 1e_dd[4]:tx_c amp calibration enable
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017f, (0x8000|DAC_IN_2V));	// 1e_17f:dac_in0_c
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0183, (0x8000|DAC_IN_2V));	// 1e_183:dac_in1_c
			reg_temp = (tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x019) & (~0x3f00));
			tx_amp_reg_shift = 8;										// 1e_19[13:8]
			tx_amp_reg = 0x19;
			tx_amp_reg_100 = 0x20;
		} else { //if(calibration_pair == ANACAL_PAIR_D)
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dd, 0x0001);				// 1e_dd[0]:tx_d amp calibration enable
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0180, (0x8000|DAC_IN_2V));	// 1e_180:dac_in0_d
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0184, (0x8000|DAC_IN_2V));	// 1e_184:dac_in1_d
			reg_temp = (tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x021) & (~0x3f00));
			tx_amp_reg_shift = 8;										// 1e_21[13:8]
			tx_amp_reg = 0x21;
			tx_amp_reg_100 = 0x22;
		}
		tc_phy_write_dev_reg( gsw, phyaddr, 0x1e, tx_amp_reg, (tx_amp_temp|(tx_amp_temp<<tx_amp_reg_shift)));	// 1e_12, 1e_17, 1e_19, 1e_21
		tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100, (tx_amp_temp|(tx_amp_temp<<tx_amp_reg_shift)));
		all_ana_cal_status = all_ge_ana_cal_wait(gsw, delay, phyaddr); 	// delay 20 usec
		if(all_ana_cal_status == 0) {
			all_ana_cal_status = ANACAL_ERROR;	
			printk( " GE Tx amp AnaCal ERROR init init!   \r\n");
			return -1;
		}
	
		ad_cal_comp_out_init = (tc_phy_read_dev_reg(gsw,  PHY0, 0x1e, 0x017a)>>8) & 0x1;		// 1e_17a[8]:ad_cal_comp_out
		if(ad_cal_comp_out_init == 1)
			calibration_polarity = -1;
		else
			calibration_polarity = 1;

		cnt =0;
		while(all_ana_cal_status < ANACAL_ERROR) {
			cnt ++;
			tx_amp_temp += calibration_polarity;
			//printk("tx_amp : %x, 1e %x = %x\n", tx_amp_temp, tx_amp_reg, (reg_temp|(tx_amp_temp<<tx_amp_reg_shift)));
			tc_phy_write_dev_reg( gsw, phyaddr, 0x1e, tx_amp_reg, (tx_amp_temp|(tx_amp_temp<<tx_amp_reg_shift)));
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100, (tx_amp_temp|(tx_amp_temp<<tx_amp_reg_shift)));
			all_ana_cal_status = all_ge_ana_cal_wait(gsw, delay, phyaddr); // delay 20 usec
			if(all_ana_cal_status == 0) {
				all_ana_cal_status = ANACAL_ERROR;	
				printk( " GE Tx amp AnaCal ERROR 2!   \r\n");
				return -1;
			} else if(((tc_phy_read_dev_reg(gsw,  PHY0, 0x1e, 0x017a)>>8)&0x1) != ad_cal_comp_out_init) {
				//printk("TX AMP ANACAL_FINISH\n");
				all_ana_cal_status = ANACAL_FINISH;
				if (phyaddr == 0) {
					if (calibration_pair == ANACAL_PAIR_A)
						tx_amp_temp = tx_amp_temp - 2;
					else if(calibration_pair == ANACAL_PAIR_B)
						tx_amp_temp = tx_amp_temp - 1;
					else if(calibration_pair == ANACAL_PAIR_C)
						tx_amp_temp = tx_amp_temp - 2;
					else if(calibration_pair == ANACAL_PAIR_D)
						tx_amp_temp = tx_amp_temp - 1;
				} else if (phyaddr == 1) {
					if (calibration_pair == ANACAL_PAIR_A)
						tx_amp_temp = tx_amp_temp - 1;
					else if(calibration_pair == ANACAL_PAIR_B)
						tx_amp_temp = tx_amp_temp ;
					else if(calibration_pair == ANACAL_PAIR_C)
						tx_amp_temp = tx_amp_temp - 1;
					else if(calibration_pair == ANACAL_PAIR_D)
						tx_amp_temp = tx_amp_temp - 1;
				} else if (phyaddr == 2) {
					if (calibration_pair == ANACAL_PAIR_A)
						tx_amp_temp = tx_amp_temp;
					else if(calibration_pair == ANACAL_PAIR_B)
						tx_amp_temp = tx_amp_temp - 1;
					else if(calibration_pair == ANACAL_PAIR_C)
						tx_amp_temp = tx_amp_temp;
					else if(calibration_pair == ANACAL_PAIR_D)
						tx_amp_temp = tx_amp_temp - 1;
				} else if (phyaddr == 3) {
					tx_amp_temp = tx_amp_temp;
				} else if (phyaddr == 4) {
					if (calibration_pair == ANACAL_PAIR_A)
						tx_amp_temp = tx_amp_temp;
					else if(calibration_pair == ANACAL_PAIR_B)
						tx_amp_temp = tx_amp_temp - 1;
					else if(calibration_pair == ANACAL_PAIR_C)
						tx_amp_temp = tx_amp_temp;
					else if(calibration_pair == ANACAL_PAIR_D)
						tx_amp_temp = tx_amp_temp;
				}								
				reg_temp = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, tx_amp_reg)&(~0xff00);
				tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100,(tx_amp_temp|((tx_amp_temp)<<tx_amp_reg_shift)));
				tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg, (tx_amp_temp|((tx_amp_temp)<<tx_amp_reg_shift)));
				if (phyaddr == 0) {
					if ((tx_amp_reg == 0x12) || (tx_amp_reg == 0x17)) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg, ((tx_amp_temp|((tx_amp_temp)<<tx_amp_reg_shift)) + 7));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
					}
					if (tx_amp_reg_100 == 0x16) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100,(tx_amp_temp|((tx_amp_temp+1+4)<<tx_amp_reg_shift)));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
					}
					if (tx_amp_reg_100 == 0x18) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100,(tx_amp_temp|((tx_amp_temp+4)<<tx_amp_reg_shift)));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
					}
				} else if (phyaddr == 1) {
					if (tx_amp_reg == 0x12) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg, ((tx_amp_temp|((tx_amp_temp)<<tx_amp_reg_shift)) + 9));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
					}
					if (tx_amp_reg == 0x17){
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg, ((tx_amp_temp|((tx_amp_temp)<<tx_amp_reg_shift)) + 7));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
					}
					if (tx_amp_reg_100 == 0x16) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100,(tx_amp_temp|((tx_amp_temp+4)<<tx_amp_reg_shift)));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
					}
					if (tx_amp_reg_100 == 0x18) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100,(tx_amp_temp|((tx_amp_temp-1+4)<<tx_amp_reg_shift)));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
					}
				} else if (phyaddr == 2) {
					if ((tx_amp_reg == 0x12) || (tx_amp_reg == 0x17)) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg, ((tx_amp_temp|((tx_amp_temp)<<tx_amp_reg_shift)) + 6));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
					}
					if ((tx_amp_reg_100 == 0x16) || (tx_amp_reg_100 == 0x18)) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100,(tx_amp_temp|((tx_amp_temp-1+4)<<tx_amp_reg_shift)));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
					}
				} else if (phyaddr == 3) {
					if (tx_amp_reg == 0x12) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg, ((tx_amp_temp|((tx_amp_temp)<<tx_amp_reg_shift)) + 4));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
					}
					if (tx_amp_reg == 0x17) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg, ((tx_amp_temp|((tx_amp_temp)<<tx_amp_reg_shift)) + 7));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
					}
					if (tx_amp_reg_100 == 0x16) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100,(tx_amp_temp|((tx_amp_temp-2+4)<<tx_amp_reg_shift)));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
					}
					if (tx_amp_reg_100 == 0x18) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100,(tx_amp_temp|((tx_amp_temp-1+3)<<tx_amp_reg_shift)));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
					}
				} else if (phyaddr == 4) {
					if ((tx_amp_reg == 0x12) || (tx_amp_reg == 0x17)) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg, ((tx_amp_temp|((tx_amp_temp)<<tx_amp_reg_shift)) + 5));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
					}
					if (tx_amp_reg_100 == 0x16) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100,(tx_amp_temp|((tx_amp_temp-2+4)<<tx_amp_reg_shift)));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
					}
					if (tx_amp_reg_100 == 0x18) {
						//printk("before : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
						tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100,(tx_amp_temp|((tx_amp_temp-1+4)<<tx_amp_reg_shift)));
						//printk("after : PORT[%d] 1e_%x = %x\n", phyaddr, tx_amp_reg_100, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg_100));
					}
				}	

				if (calibration_pair == ANACAL_PAIR_A){
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x12);
					reg_tmp = ((reg_backup & 0xfc00) >> 10);
					reg_tmp -= 8;
                                       reg_backup = 0x0000;
                                       reg_backup |= ((reg_tmp << 10) | (reg_tmp << 0));
					tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x12, reg_backup);
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x12);
					//printk("PORT[%d] 1e.012 = %x (OFFSET_1000M_PAIR_A)\n", phyaddr, reg_backup);
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x16);
					reg_tmp = ((reg_backup & 0x3f) >> 0);
					reg_tmp -= 8;
					reg_backup = (reg_backup & (~0x3f));
					reg_backup |= (reg_tmp << 0);
					tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x16, reg_backup);
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x16);
					//printk("PORT[%d] 1e.016 = %x (OFFSET_TESTMODE_1000M_PAIR_A)\n", phyaddr, reg_backup);
				}
				else if(calibration_pair == ANACAL_PAIR_B){
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x17);
					reg_tmp = ((reg_backup & 0x3f00) >> 8);
					reg_tmp -= 8;
                                       reg_backup = 0x0000;
                                       reg_backup |= ((reg_tmp << 8) | (reg_tmp << 0));
					tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x17, reg_backup);
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x17);
					//printk("PORT[%d] 1e.017 = %x (OFFSET_1000M_PAIR_B)\n", phyaddr, reg_backup);
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x18);
					reg_tmp = ((reg_backup & 0x3f) >> 0);
					reg_tmp -= 8;
					reg_backup = (reg_backup & (~0x3f));
					reg_backup |= (reg_tmp << 0);
					tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x18, reg_backup);
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x18);
					//printk("PORT[%d] 1e.018 = %x (OFFSET_TESTMODE_1000M_PAIR_B)\n", phyaddr, reg_backup);
				}
				else if(calibration_pair == ANACAL_PAIR_C){
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x19);
					reg_tmp = ((reg_backup & 0x3f00) >> 8);
					reg_tmp -= 8;
					reg_backup = (reg_backup & (~0x3f00));
					reg_backup |= (reg_tmp << 8);
					tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x19, reg_backup);
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x19);
					//printk("PORT[%d] 1e.019 = %x (OFFSET_1000M_PAIR_C)\n", phyaddr, reg_backup);
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x20);
					reg_tmp = ((reg_backup & 0x3f) >> 0);
					reg_tmp -= 8;
					reg_backup = (reg_backup & (~0x3f));
					reg_backup |= (reg_tmp << 0);
					tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x20, reg_backup);
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x20);
					//printk("PORT[%d] 1e.020 = %x (OFFSET_TESTMODE_1000M_PAIR_C)\n", phyaddr, reg_backup);
				}
				else if(calibration_pair == ANACAL_PAIR_D){
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x21);
					reg_tmp = ((reg_backup & 0x3f00) >> 8);
					reg_tmp -= 8;
					reg_backup = (reg_backup & (~0x3f00));
					reg_backup |= (reg_tmp << 8);
					tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x21, reg_backup);
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x21);
					//printk("PORT[%d] 1e.021 = %x (OFFSET_1000M_PAIR_D)\n", phyaddr, reg_backup);
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x22);
					reg_tmp = ((reg_backup & 0x3f) >> 0);
					reg_tmp -= 8;
					reg_backup = (reg_backup & (~0x3f));
					reg_backup |= (reg_tmp << 0);
					tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x22, reg_backup);
					reg_backup = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x22);
					//printk("PORT[%d] 1e.022 = %x (OFFSET_TESTMODE_1000M_PAIR_D)\n", phyaddr, reg_backup);
				}

				if (calibration_pair == ANACAL_PAIR_A){
					//printk("PORT (%d) TX_AMP PAIR (A) FINAL CALIBRATION RESULT\n", phyaddr);
					debug_tmp = tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x12);
					//printk("1e.012 = 0x%x\n", debug_tmp);
					debug_tmp = tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x16);
					//printk("1e.016 = 0x%x\n", debug_tmp);
				}
	
				else if(calibration_pair == ANACAL_PAIR_B){
					//printk("PORT (%d) TX_AMP PAIR (A) FINAL CALIBRATION RESULT\n", phyaddr);
					debug_tmp = tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x17);
					//printk("1e.017 = 0x%x\n", debug_tmp);
					debug_tmp = tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x18);
					//printk("1e.018 = 0x%x\n", debug_tmp);
				}
				else if(calibration_pair == ANACAL_PAIR_C){
					//printk("PORT (%d) TX_AMP PAIR (A) FINAL CALIBRATION RESULT\n", phyaddr);
					debug_tmp = tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x19);
					//printk("1e.019 = 0x%x\n", debug_tmp);
					debug_tmp = tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x20);
					//printk("1e.020 = 0x%x\n", debug_tmp);
				}
				else if(calibration_pair == ANACAL_PAIR_D){
					//printk("PORT (%d) TX_AMP PAIR (A) FINAL CALIBRATION RESULT\n", phyaddr);
					debug_tmp = tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x21);
					//printk("1e.021 = 0x%x\n", debug_tmp);
					debug_tmp = tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x22);
					//printk("1e.022 = 0x%x\n", debug_tmp);
				}


				printk( " GE Tx amp AnaCal Done! (pair-%d)(1e_%x = 0x%x)\n", calibration_pair, tx_amp_reg, tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg));
				
			} else {
				if((tx_amp_temp == 0x3f)||(tx_amp_temp == 0x00)) {
					all_ana_cal_status = ANACAL_SATURATION;  // need to FT
					printk( " GE Tx amp AnaCal Saturation!  \r\n");
				}
			}
		}

		if(all_ana_cal_status == ANACAL_ERROR) {	
			tx_amp_temp = 0x20;
			tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, tx_amp_reg, (reg_temp|(tx_amp_temp<<tx_amp_reg_shift)));
		}
	}
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017d, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017e, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x017f, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0180, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0181, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0182, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0183, 0x0000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x0184, 0x0000);
	
	/* disable analog calibration circuit */
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00db, 0x0000);
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00dc, 0x0000);	// disable Tx offset calibration circuit
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00db, 0x0000);	// disable analog calibration circuit
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dc, 0x0000);	// disable Tx offset calibration circuit
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x003e, 0x0000);	// disable Tx VLD force mode
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x00dd, 0x0000);	// disable Tx offset/amplitude calibration circuit
	
	

	//tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x273, 0x2000);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0xc9, 0x0fff);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x145, 0x1000);

	/* Restore CR to default */
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x11, orig_1e_11);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x300, orig_1f_300);
}

//-----------------------------------------------------------------

int phy_calibration(struct gsw_mt753x *gsw, u8 phyaddr)
{
	u32	reg_tmp,reg_tmp0, reg_tmp1, i;
	u32 CALDLY = 40;
	u32 orig_1e_11, orig_1e_185, orig_1e_e1, orig_1f_100;
	int ret;
	/* set [12]AN disable, [8]full duplex, [13/6]1000Mbps */
	//tc_phy_write_dev_reg(phyaddr, 0x0,  0x0140);
	switch_phy_write(gsw, phyaddr, R0, 0x140);

	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x145, 0x1010);/* fix mdi */
	orig_1e_185 = tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, RG_185);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, RG_185, 0);/* disable tx slew control */
	orig_1f_100 = tc_phy_read_dev_reg(gsw, phyaddr, 0x1f, 0x100);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x100, 0xc000);/* BG voltage output */
	//tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x403, 0x1099); //bypass efuse

#if (1)
	//	1f_27c[12:8] cr_da_tx_i2mpb_10m	Trimming TX bias setup(@10M)
	//tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x27c, 0x1f1f);
	//tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x27c, 0x3300);

	//reg_tmp1 = tc_phy_read_dev_reg(gsw,  PHY0, 0x1f, 0x27c);
	//dev1Fh_reg273h TXVLD DA register	- Adjust voltage mode TX amplitude.
	//tc_phy_write_dev_reg(phyaddr, 0x1f, 0x273, 0);
	//tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x273, 0x1000);
	//reg_tmp1 = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1f, 0x273);
	//printk("reg_tmp1273 = %x\n", reg_tmp1);
	/*1e_11 TX  overshoot Enable (PAIR A/B/C/D) in gbe mode*/

	orig_1e_11 = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x11);
	reg_tmp = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x11);
	reg_tmp = reg_tmp | (0xf << 12);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x11, reg_tmp);
	orig_1e_e1 = tc_phy_read_dev_reg(gsw, PHY0, 0x1e, 0x00e1);
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00e1, 0x10);
	/* calibration start ============ */
	printk("CALDLY = %d\n", CALDLY);
	if(ge_cal_flag == 0){
		ret = ge_cal_rext(gsw, 0, CALDLY);
		if (ret == -1){
			printk("ge_cal_rext error K port =%d\n", phyaddr);
			return ret;
		}
		ge_cal_flag = 1;
	}

	/* *** R50 Cal start ***************************** */
	/*phyaddress = 0*/
	ret = ge_cal_r50(gsw, phyaddr, CALDLY);
	if (ret == -1){
		printk("R50 error K port =%d\n", phyaddr);
		return ret;
	}
	/* *** R50 Cal end *** */
	/* *** Tx offset Cal start *********************** */
	ret = ge_cal_tx_offset(gsw, phyaddr, CALDLY);
	if (ret == -1){
		printk("ge_cal_tx_offset error K port =%d\n", phyaddr);
		return ret;
	}
	/* *** Tx offset Cal end *** */

	/* *** Tx Amp Cal start *** */
	ret = ge_cal_tx_amp(gsw, phyaddr, CALDLY);
	if (ret == -1){
		printk("ge_cal_tx_amp error K port =%d\n", phyaddr);
		return ret;
	}
	/* *** Tx Amp Cal end *** */
	/*tmp maybe changed*/
	//tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x27c, 0x1111);
	//tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x27b, 0x47);
	//tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x273, 0x2000);

	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x3a8, 0x0810);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x3aa, 0x0008);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x3ab, 0x0810);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x3ad, 0x0008);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x3ae, 0x0106);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x3b0, 0x0001);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x3b1, 0x0106);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x3b3, 0x0001);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x18c, 0x0001);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x18d, 0x0001);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x18e, 0x0001);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x18f, 0x0001);

	/*da_tx_bias1_b_tx_standby = 5'b10 (dev1eh_reg3aah[12:8])*/
	reg_tmp = tc_phy_read_dev_reg(gsw, phyaddr, 0x1e, 0x3aa);
	reg_tmp = reg_tmp & ~(0x1f00);
	reg_tmp = reg_tmp | 0x2 << 8;
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x3aa, reg_tmp);

	/*da_tx_bias1_a_tx_standby = 5'b10 (dev1eh_reg3a9h[4:0])*/
	reg_tmp = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1e, 0x3a9);
	reg_tmp = reg_tmp & ~(0x1f);
	reg_tmp = reg_tmp | 0x2;
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x3a9, reg_tmp);

	/* Restore CR to default */
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, RG_185, orig_1e_185);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x100, orig_1f_100);
	tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x11, orig_1e_11);
	tc_phy_write_dev_reg(gsw, PHY0, 0x1e, 0x00e1, orig_1e_e1);
#endif
}

void rx_dc_offset(struct gsw_mt753x *gsw, u8 phyaddr)
{
    u32 reg_tmp1;

    pr_info("PORT %d RX_DC_OFFSET\n", phyaddr);
    tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x96, 0x8000);
    tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x37, 0x3);
    tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x107, 0x4000);
    tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x171, 0x1e5);
    tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x39, 0x200f);
    udelay(40);
    tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x39, 0x000f);
    udelay(40);
    tc_phy_write_dev_reg(gsw, phyaddr, 0x1e, 0x171, 0x65);
}

void check_rx_dc_offset_pair_a(struct gsw_mt753x *gsw, u8 phyaddr)
{
    u32 reg_tmp;
    u8 reg_val;

    tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x15, (phyaddr << 13) | 0x114f);
    reg_tmp = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1f, 0x1a);
    reg_tmp = reg_tmp & 0xff;
    pr_info("before pairA output = %x\n", reg_tmp);
    udelay(40);
    tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x15, (phyaddr << 13) | 0x1142);
    udelay(40);
    reg_tmp = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1f, 0x1a);
    reg_tmp = reg_tmp & 0xff;   
    pr_info("after pairA output = %x\n", reg_tmp);
    if ((reg_tmp & 0x80) != 0)
        reg_tmp = (~reg_tmp) + 1;
    if ((reg_tmp & 0xff) >4)
        pr_info("pairA RX_DC_OFFSET error");
}

void check_rx_dc_offset_pair_b(struct gsw_mt753x *gsw, u8 phyaddr)
{
    u32 reg_tmp;
    u8 reg_val;

    tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x15, (phyaddr << 13) | 0x1151);
    reg_tmp = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1f, 0x1a);
    reg_tmp = reg_tmp & 0xff;
    pr_info("before pairB output = %x\n", reg_tmp);
    udelay(40);
    tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x15, (phyaddr << 13) | 0x1143);
    udelay(40);
    reg_tmp = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1f, 0x1a);
    reg_tmp = reg_tmp & 0xff;   
    pr_info("after pairB output = %x\n", reg_tmp);
    if ((reg_tmp & 0x80) != 0)
        reg_tmp = (~reg_tmp) + 1;
    if ((reg_tmp & 0xff) >4)
        pr_info("pairB RX_DC_OFFSET error");
}

void check_rx_dc_offset_pair_c(struct gsw_mt753x *gsw, u8 phyaddr)
{
    u32 reg_tmp;
    u8 reg_val;

    tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x15, (phyaddr << 13) | 0x1153);
    reg_tmp = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1f, 0x1a);
    reg_tmp = reg_tmp & 0xff;
    pr_info("before pairC output = %x\n", reg_tmp);
    udelay(40);
    tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x15, (phyaddr << 13) | 0x1144);
    udelay(40);
    reg_tmp = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1f, 0x1a);
    reg_tmp = reg_tmp & 0xff;   
    pr_info("after pairC output = %x\n", reg_tmp);
    if ((reg_tmp & 0x80) != 0)
        reg_tmp = (~reg_tmp) + 1;
    if ((reg_tmp & 0xff) >4)
        pr_info("pairC RX_DC_OFFSET error");
}

void check_rx_dc_offset_pair_d(struct gsw_mt753x *gsw, u8 phyaddr)
{
    u32 reg_tmp;
    u8 reg_val;

    tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x15, (phyaddr << 13) | 0x1155);
    reg_tmp = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1f, 0x1a);
    reg_tmp = reg_tmp & 0xff;
    pr_info("before pairD output = %x\n", reg_tmp);
    udelay(40);
    tc_phy_write_dev_reg(gsw, phyaddr, 0x1f, 0x15, (phyaddr << 13) | 0x1145);
    udelay(40);
    reg_tmp = tc_phy_read_dev_reg(gsw,  phyaddr, 0x1f, 0x1a);
    reg_tmp = reg_tmp & 0xff;   
    pr_info("after pairD output = %x\n", reg_tmp);
    if ((reg_tmp & 0x80) != 0)
        reg_tmp = (~reg_tmp) + 1;
    if ((reg_tmp & 0xff) >4)
        pr_info("pairD RX_DC_OFFSET error");
}


int mt753x_phy_calibration(struct gsw_mt753x *gsw, u8 phyaddr){

	int ret;

	phy_calibration(gsw, phyaddr);

	rx_dc_offset(gsw, phyaddr);
	check_rx_dc_offset_pair_a(gsw, phyaddr);
	check_rx_dc_offset_pair_b(gsw, phyaddr);
	check_rx_dc_offset_pair_c(gsw, phyaddr);
	check_rx_dc_offset_pair_d(gsw, phyaddr);

	return ret;
}
