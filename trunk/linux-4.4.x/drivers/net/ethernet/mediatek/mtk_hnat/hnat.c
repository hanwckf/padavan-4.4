/*   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   Copyright (C) 2014-2016 Sean Wang <sean.wang@mediatek.com>
 *   Copyright (C) 2016-2017 John Crispin <blogic@openwrt.org>
 */

#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/if.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset.h>

#include "nf_hnat_mtk.h"
#include "hnat.h"

struct mtk_hnat *hnat_priv;

int (*ra_sw_nat_hook_rx)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(ra_sw_nat_hook_rx);
int (*ra_sw_nat_hook_tx)(struct sk_buff *skb, int gmac_no) = NULL;
EXPORT_SYMBOL(ra_sw_nat_hook_tx);

void (*ppe_dev_register_hook)(struct net_device *dev) = NULL;
EXPORT_SYMBOL(ppe_dev_register_hook);
void (*ppe_dev_unregister_hook)(struct net_device *dev) = NULL;
EXPORT_SYMBOL(ppe_dev_unregister_hook);

static void hnat_sma_build_entry(unsigned long data)
{
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, SMA, SMA_FWD_CPU_BUILD_ENTRY);
}

static void hnat_reset_timestamp(unsigned long data)
{
	struct foe_entry *entry;
	int hash_index;

	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, TCP_AGE, 0);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, UDP_AGE, 0);
	writel(0, hnat_priv->fe_base + 0x0010);

	for (hash_index = 0; hash_index < hnat_priv->foe_etry_num; hash_index++) {
		entry = hnat_priv->foe_table_cpu + hash_index;
		if (entry->bfib1.state == BIND)
			entry->ipv4_hnapt.udib1.time_stamp = 0;
	}

	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, TCP_AGE, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, UDP_AGE, 1);
	mod_timer(&hnat_priv->hnat_reset_timestamp_timer, jiffies + 14400 * HZ);
}

static void cr_set_bits(void __iomem *reg, u32 bs)
{
	u32 val = readl(reg);

	val |= bs;
	writel(val, reg);
}

static void cr_clr_bits(void __iomem *reg, u32 bs)
{
	u32 val = readl(reg);

	val &= ~bs;
	writel(val, reg);
}

void cr_set_field(void __iomem *reg, u32 field, u32 val)
{
	unsigned int tv = readl(reg);

	tv &= ~field;
	tv |= ((val) << (ffs((unsigned int)field) - 1));
	writel(tv, reg);
}

/*boundary entry can't be used to accelerate data flow*/
static void exclude_boundary_entry(struct foe_entry *foe_table_cpu)
{
	int entry_base = 0;
	int bad_entry, i, j;
	struct foe_entry *foe_entry;
	/*these entries are boundary every 128 entries*/
	int boundary_entry_offset[7] = { 12, 25, 38, 51, 76, 89, 102 };

	if (!foe_table_cpu)
		return;

	for (i = 0; entry_base < hnat_priv->foe_etry_num; i++) {
		/* set boundary entries as static*/
		for (j = 0; j < 7; j++) {
			bad_entry = entry_base + boundary_entry_offset[j];
			foe_entry = &foe_table_cpu[bad_entry];
			foe_entry->udib1.sta = 1;
		}
		entry_base = (i + 1) * 128;
	}
}

void set_gmac_ppe_fwd(int id, int enable)
{
	void __iomem *reg;
	u32 val;

	reg = hnat_priv->fe_base + (id ? GDMA2_FWD_CFG : GDMA1_FWD_CFG);

	if (enable) {
		cr_set_bits(reg, BITS_GDM_ALL_FRC_P_PPE);

		return;
	}

	/*disabled */
	val = readl(reg);
	if ((val & GDM_ALL_FRC_MASK) == BITS_GDM_ALL_FRC_P_PPE)
		cr_set_field(reg, GDM_ALL_FRC_MASK,
			     BITS_GDM_ALL_FRC_P_CPU_PDMA);
}

static int hnat_start(void)
{
	u32 foe_table_sz;
	u32 foe_mib_tb_sz;
	int etry_num_cfg;

	/* mapp the FOE table */
	for (etry_num_cfg = DEF_ETRY_NUM_CFG ; etry_num_cfg >= 0 ; etry_num_cfg--, hnat_priv->foe_etry_num /= 2) {
		foe_table_sz = hnat_priv->foe_etry_num * sizeof(struct foe_entry);
		hnat_priv->foe_table_cpu = dma_alloc_coherent(
			hnat_priv->dev, foe_table_sz, &hnat_priv->foe_table_dev, GFP_KERNEL);

		if (hnat_priv->foe_table_cpu)
			break;
	}

	if (!hnat_priv->foe_table_cpu)
		return -1;
	dev_info(hnat_priv->dev, "FOE entry number = %d\n", hnat_priv->foe_etry_num);

	writel(hnat_priv->foe_table_dev, hnat_priv->ppe_base + PPE_TB_BASE);
	memset(hnat_priv->foe_table_cpu, 0, foe_table_sz);

	if (hnat_priv->data->version == MTK_HNAT_V1)
		exclude_boundary_entry(hnat_priv->foe_table_cpu);

	if (hnat_priv->data->per_flow_accounting) {
		foe_mib_tb_sz = hnat_priv->foe_etry_num * sizeof(struct mib_entry);
		hnat_priv->foe_mib_cpu = dma_alloc_coherent(hnat_priv->dev, foe_mib_tb_sz,
						       &hnat_priv->foe_mib_dev, GFP_KERNEL);
		if (!hnat_priv->foe_mib_cpu)
			return -1;
		writel(hnat_priv->foe_mib_dev, hnat_priv->ppe_base + PPE_MIB_TB_BASE);
		memset(hnat_priv->foe_mib_cpu, 0, foe_mib_tb_sz);

		hnat_priv->acct =
			kzalloc(hnat_priv->foe_etry_num * sizeof(struct hnat_accounting),
				GFP_KERNEL);
		if (!hnat_priv->acct)
			return -1;
	}
	/* setup hashing */
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, TB_ETRY_NUM, etry_num_cfg);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, HASH_MODE, HASH_MODE_1);
	writel(HASH_SEED_KEY, hnat_priv->ppe_base + PPE_HASH_SEED);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, XMODE, 0);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, TB_ENTRY_SIZE, ENTRY_80B);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, SMA, SMA_FWD_CPU_BUILD_ENTRY);

	/* set ip proto */
	writel(0xFFFFFFFF, hnat_priv->ppe_base + PPE_IP_PROT_CHK);

	/* setup caching */
	cr_set_field(hnat_priv->ppe_base + PPE_CAH_CTRL, CAH_X_MODE, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_CAH_CTRL, CAH_X_MODE, 0);
	cr_set_field(hnat_priv->ppe_base + PPE_CAH_CTRL, CAH_EN, 1);

	/* enable FOE */
	cr_set_bits(hnat_priv->ppe_base + PPE_FLOW_CFG,
		    BIT_UDP_IP4F_NAT_EN | BIT_IPV4_NAT_EN | BIT_IPV4_NAPT_EN |
		    BIT_IPV4_NAT_FRAG_EN | BIT_IPV4_HASH_GREK |
		    BIT_IPV4_DSL_EN | BIT_IPV6_6RD_EN |
		    BIT_IPV6_3T_ROUTE_EN | BIT_IPV6_5T_ROUTE_EN);

	/* setup FOE aging */
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, NTU_AGE, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, UNBD_AGE, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_UNB_AGE, UNB_MNP, 1000);
	cr_set_field(hnat_priv->ppe_base + PPE_UNB_AGE, UNB_DLTA, 3);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, TCP_AGE, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, UDP_AGE, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, FIN_AGE, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_BND_AGE_0, UDP_DLTA, 12);
	cr_set_field(hnat_priv->ppe_base + PPE_BND_AGE_0, NTU_DLTA, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_BND_AGE_1, FIN_DLTA, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_BND_AGE_1, TCP_DLTA, 7);

	/* setup FOE ka */
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, SCAN_MODE, 2);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, KA_CFG, 3);
	cr_set_field(hnat_priv->ppe_base + PPE_KA, KA_T, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_KA, TCP_KA, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_KA, UDP_KA, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_BIND_LMT_1, NTU_KA, 1);

	/* setup FOE rate limit */
	cr_set_field(hnat_priv->ppe_base + PPE_BIND_LMT_0, QURT_LMT, 16383);
	cr_set_field(hnat_priv->ppe_base + PPE_BIND_LMT_0, HALF_LMT, 16383);
	cr_set_field(hnat_priv->ppe_base + PPE_BIND_LMT_1, FULL_LMT, 16383);
	/* setup binding threshold as 30 packets per second */
	cr_set_field(hnat_priv->ppe_base + PPE_BNDR, BIND_RATE, 0x1E);

	/* setup FOE cf gen */
	cr_set_field(hnat_priv->ppe_base + PPE_GLO_CFG, PPE_EN, 1);
	writel(0, hnat_priv->ppe_base + PPE_DFT_CPORT); /* pdma */
	/* writel(0x55555555, hnat_priv->ppe_base + PPE_DFT_CPORT); */ /* qdma */
	cr_set_field(hnat_priv->ppe_base + PPE_GLO_CFG, TTL0_DRP, 1);

	/*enable ppe mib counter*/
	if (hnat_priv->data->per_flow_accounting) {
		cr_set_field(hnat_priv->ppe_base + PPE_MIB_CFG, MIB_EN, 1);
		cr_set_field(hnat_priv->ppe_base + PPE_MIB_CFG, MIB_READ_CLEAR, 1);
		cr_set_field(hnat_priv->ppe_base + PPE_MIB_CAH_CTRL, MIB_CAH_EN, 1);
	}

	hnat_priv->g_ppdev = dev_get_by_name(&init_net, hnat_priv->ppd);

	dev_info(hnat_priv->dev, "hwnat start\n");

	return 0;
}

static int ppe_busy_wait(void)
{
	unsigned long t_start = jiffies;
	u32 r = 0;

	while (1) {
		r = readl((hnat_priv->ppe_base + 0x0));
		if (!(r & BIT(31)))
			return 0;
		if (time_after(jiffies, t_start + HZ))
			break;
		usleep_range(10, 20);
	}

	dev_notice(hnat_priv->dev, "ppe:%s timeout\n", __func__);

	return -1;
}

static void hnat_stop(void)
{
	u32 foe_table_sz;
	u32 foe_mib_tb_sz;
	struct foe_entry *entry, *end;
	u32 r1 = 0, r2 = 0;

	/* send all traffic back to the DMA engine */
	set_gmac_ppe_fwd(0, 0);
	set_gmac_ppe_fwd(1, 0);

	dev_info(hnat_priv->dev, "hwnat stop\n");

	if (hnat_priv->foe_table_cpu) {
		entry = hnat_priv->foe_table_cpu;
		end = hnat_priv->foe_table_cpu + hnat_priv->foe_etry_num;
		while (entry < end) {
			entry->bfib1.state = INVALID;
			entry++;
		}
	}
	/* disable caching */
	cr_set_field(hnat_priv->ppe_base + PPE_CAH_CTRL, CAH_X_MODE, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_CAH_CTRL, CAH_X_MODE, 0);
	cr_set_field(hnat_priv->ppe_base + PPE_CAH_CTRL, CAH_EN, 0);

	/* flush cache has to be ahead of hnat disable --*/
	cr_set_field(hnat_priv->ppe_base + PPE_GLO_CFG, PPE_EN, 0);

	/* disable scan mode and keep-alive */
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, SCAN_MODE, 0);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, KA_CFG, 0);

	ppe_busy_wait();

	/* disable FOE */
	cr_clr_bits(hnat_priv->ppe_base + PPE_FLOW_CFG,
		    BIT_IPV4_NAPT_EN | BIT_IPV4_NAT_EN | BIT_IPV4_NAT_FRAG_EN |
		    BIT_IPV6_HASH_GREK | BIT_IPV4_DSL_EN |
		    BIT_IPV6_6RD_EN | BIT_IPV6_3T_ROUTE_EN |
		    BIT_IPV6_5T_ROUTE_EN | BIT_FUC_FOE | BIT_FMC_FOE |
		    BIT_FUC_FOE);

	/* disable FOE aging */
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, NTU_AGE, 0);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, UNBD_AGE, 0);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, TCP_AGE, 0);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, UDP_AGE, 0);
	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, FIN_AGE, 0);

	r1 = readl(hnat_priv->fe_base + 0x100);
	r2 = readl(hnat_priv->fe_base + 0x10c);

	dev_info(hnat_priv->dev, "0x100 = 0x%x, 0x10c = 0x%x\n", r1, r2);

	if (((r1 & 0xff00) >> 0x8) >= (r1 & 0xff) ||
	    ((r1 & 0xff00) >> 0x8) >= (r2 & 0xff)) {
		dev_info(hnat_priv->dev, "reset pse\n");
		writel(0x1, hnat_priv->fe_base + 0x4);
	}

	/* free the FOE table */
	foe_table_sz = hnat_priv->foe_etry_num * sizeof(struct foe_entry);
	if (hnat_priv->foe_table_cpu)
		dma_free_coherent(hnat_priv->dev, foe_table_sz, hnat_priv->foe_table_cpu,
				  hnat_priv->foe_table_dev);
	writel(0, hnat_priv->ppe_base + PPE_TB_BASE);

	if (hnat_priv->data->per_flow_accounting) {
		foe_mib_tb_sz = hnat_priv->foe_etry_num * sizeof(struct mib_entry);
		if (hnat_priv->foe_mib_cpu)
			dma_free_coherent(hnat_priv->dev, foe_mib_tb_sz,
					  hnat_priv->foe_mib_cpu, hnat_priv->foe_mib_dev);
		writel(0, hnat_priv->ppe_base + PPE_MIB_TB_BASE);
		kfree(hnat_priv->acct);
	}
}

static void hnat_release_netdev(void)
{
	int i;
	struct extdev_entry *ext_entry;

	for (i = 0; i < MAX_EXT_DEVS && hnat_priv->ext_if[i]; i++) {
		ext_entry = hnat_priv->ext_if[i];
		if (ext_entry->dev)
			dev_put(ext_entry->dev);
		ext_if_del(ext_entry);
		kfree(ext_entry);
	}

	if (hnat_priv->g_ppdev)
		dev_put(hnat_priv->g_ppdev);
}

static struct notifier_block nf_hnat_netdevice_nb __read_mostly = {
	.notifier_call = nf_hnat_netdevice_event,
};

static struct notifier_block nf_hnat_netevent_nb __read_mostly = {
	.notifier_call = nf_hnat_netevent_handler,
};

int hnat_enable_hook(void)
{
	/* register hook functions used by WHNAT module.
	 */
	if (hnat_priv->data->version == MTK_HNAT_V2) {
		ra_sw_nat_hook_tx = mtk_sw_nat_hook_tx;
		ra_sw_nat_hook_rx = NULL;
		ppe_dev_register_hook = mtk_ppe_dev_register_hook;
		ppe_dev_unregister_hook = mtk_ppe_dev_unregister_hook;
	}

	if (hnat_register_nf_hooks())
		return -1;

	hook_toggle = 1;

	return 0;
}

int hnat_disable_hook(void)
{
	int hash_index;
	struct foe_entry *entry;

	ra_sw_nat_hook_tx = NULL;
	hnat_unregister_nf_hooks();

	cr_set_field(hnat_priv->ppe_base + PPE_TB_CFG, SMA, SMA_ONLY_FWD_CPU);
	for (hash_index = 0; hash_index < hnat_priv->foe_etry_num; hash_index++) {
		entry = hnat_priv->foe_table_cpu + hash_index;
		if (entry->bfib1.state == BIND) {
			entry->ipv4_hnapt.udib1.state = INVALID;
			entry->ipv4_hnapt.udib1.time_stamp =
				readl((hnat_priv->fe_base + 0x0010)) & 0xFF;
		}
	}

	/* clear HWNAT cache */
	cr_set_field(hnat_priv->ppe_base + PPE_CAH_CTRL, CAH_X_MODE, 1);
	cr_set_field(hnat_priv->ppe_base + PPE_CAH_CTRL, CAH_X_MODE, 0);
	cr_set_field(hnat_priv->ppe_base + PPE_CAH_CTRL, CAH_EN, 1);
	mod_timer(&hnat_priv->hnat_sma_build_entry_timer, jiffies + 3 * HZ);
	hook_toggle = 0;

	return 0;
}

static int hnat_probe(struct platform_device *pdev)
{
	int i;
	int err = 0;
	int index = 0;
	struct resource *res;
	const char *name;
	struct device_node *np;
	unsigned int val;
	struct property *prop;
	struct extdev_entry *ext_entry;
	const struct of_device_id *match;

	hnat_priv = devm_kzalloc(&pdev->dev, sizeof(struct mtk_hnat), GFP_KERNEL);
	if (!hnat_priv)
		return -ENOMEM;

	hnat_priv->foe_etry_num = DEF_ETRY_NUM;

	match = of_match_device(of_hnat_match, &pdev->dev);
	hnat_priv->data = (struct mtk_hnat_data *)match->data;

	hnat_priv->dev = &pdev->dev;
	np = hnat_priv->dev->of_node;

	err = of_property_read_string(np, "mtketh-wan", &name);
	if (err < 0)
		return -EINVAL;

	strncpy(hnat_priv->wan, (char *)name, IFNAMSIZ);
	dev_info(&pdev->dev, "wan = %s\n", hnat_priv->wan);

	err = of_property_read_string(np, "mtketh-ppd", &name);
	if (err < 0)
		strncpy(hnat_priv->ppd, "eth0", IFNAMSIZ);
	else
		strncpy(hnat_priv->ppd, (char *)name, IFNAMSIZ);
	dev_info(&pdev->dev, "ppd = %s\n", hnat_priv->ppd);

	/*get total gmac num in hnat*/
	err = of_property_read_u32_index(np, "mtketh-max-gmac", 0, &val);

	if (err < 0)
		return -EINVAL;

	hnat_priv->gmac_num = val;

	dev_info(&pdev->dev, "gmac num = %d\n", hnat_priv->gmac_num);

	err = of_property_read_u32_index(np, "mtkdsa-wan-port", 0, &val);

	if (err < 0) {
		hnat_priv->wan_dsa_port = NONE_DSA_PORT;
	} else {
		hnat_priv->wan_dsa_port = val;
		dev_info(&pdev->dev, "wan dsa port = %d\n", hnat_priv->wan_dsa_port);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENOENT;

	hnat_priv->fe_base = devm_ioremap_nocache(&pdev->dev, res->start,
					     res->end - res->start + 1);
	if (!hnat_priv->fe_base)
		return -EADDRNOTAVAIL;

	hnat_priv->ppe_base = hnat_priv->fe_base + 0xe00;
	err = hnat_init_debugfs(hnat_priv);
	if (err)
		return err;

	prop = of_find_property(np, "ext-devices", NULL);
	for (name = of_prop_next_string(prop, NULL); name;
	     name = of_prop_next_string(prop, name), index++) {
		ext_entry = kzalloc(sizeof(*ext_entry), GFP_KERNEL);
		if (!ext_entry) {
			err = -ENOMEM;
			goto err_out1;
		}
		strncpy(ext_entry->name, (char *)name, IFNAMSIZ);
		ext_if_add(ext_entry);
	}

	for (i = 0; i < MAX_EXT_DEVS && hnat_priv->ext_if[i]; i++) {
		ext_entry = hnat_priv->ext_if[i];
		dev_info(&pdev->dev, "ext devices = %s\n", ext_entry->name);
	}

	hnat_priv->lvid = 1;
	hnat_priv->wvid = 2;

	err = hnat_start();
	if (err)
		goto err_out;

	if (hnat_priv->data->whnat) {
		err = whnat_adjust_nf_hooks();
		if (err)
			goto err_out;
	}

	err = hnat_enable_hook();
	if (err)
		goto err_out;

	register_netdevice_notifier(&nf_hnat_netdevice_nb);
	register_netevent_notifier(&nf_hnat_netevent_nb);
	if (hnat_priv->data->mcast)
		hnat_mcast_enable();
	init_timer(&hnat_priv->hnat_sma_build_entry_timer);
	hnat_priv->hnat_sma_build_entry_timer.function = hnat_sma_build_entry;
	if (hnat_priv->data->version == MTK_HNAT_V3) {
		init_timer(&hnat_priv->hnat_reset_timestamp_timer);
		hnat_priv->hnat_reset_timestamp_timer.function = hnat_reset_timestamp;
		hnat_priv->hnat_reset_timestamp_timer.expires = jiffies;
		add_timer(&hnat_priv->hnat_reset_timestamp_timer);
	}

	return 0;

err_out:
	hnat_stop();
err_out1:
	hnat_deinit_debugfs(hnat_priv);
	for (i = 0; i < MAX_EXT_DEVS && hnat_priv->ext_if[i]; i++) {
		ext_entry = hnat_priv->ext_if[i];
		ext_if_del(ext_entry);
		kfree(ext_entry);
	}
	return err;
}

static int hnat_remove(struct platform_device *pdev)
{
	unregister_netdevice_notifier(&nf_hnat_netdevice_nb);
	unregister_netevent_notifier(&nf_hnat_netevent_nb);
	hnat_disable_hook();

	if (hnat_priv->data->mcast)
		hnat_mcast_disable();

	hnat_stop();
	hnat_deinit_debugfs(hnat_priv);
	hnat_release_netdev();
	del_timer_sync(&hnat_priv->hnat_sma_build_entry_timer);
	if (hnat_priv->data->version == MTK_HNAT_V3)
		del_timer_sync(&hnat_priv->hnat_reset_timestamp_timer);

	return 0;
}

static const struct mtk_hnat_data hnat_data_v1 = {
	.num_of_sch = 2,
	.whnat = false,
	.per_flow_accounting = false,
	.mcast = false,
	.version = MTK_HNAT_V1,
};

static const struct mtk_hnat_data hnat_data_v2 = {
	.num_of_sch = 2,
	.whnat = true,
	.per_flow_accounting = true,
	.mcast = false,
	.version = MTK_HNAT_V2,
};

static const struct mtk_hnat_data hnat_data_v3 = {
	.num_of_sch = 4,
	.whnat = false,
	.per_flow_accounting = false,
	.mcast = false,
	.version = MTK_HNAT_V3,
};

const struct of_device_id of_hnat_match[] = {
	{ .compatible = "mediatek,mtk-hnat", .data = &hnat_data_v3 },
	{ .compatible = "mediatek,mtk-hnat_v1", .data = &hnat_data_v1 },
	{ .compatible = "mediatek,mtk-hnat_v2", .data = &hnat_data_v2 },
	{ .compatible = "mediatek,mtk-hnat_v3", .data = &hnat_data_v3 },
	{},
};
MODULE_DEVICE_TABLE(of, of_hnat_match);

static struct platform_driver hnat_driver = {
	.probe = hnat_probe,
	.remove = hnat_remove,
	.driver = {
		.name = "mediatek_soc_hnat",
		.of_match_table = of_hnat_match,
	},
};

module_platform_driver(hnat_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Sean Wang <sean.wang@mediatek.com>");
MODULE_AUTHOR("John Crispin <john@phrozen.org>");
MODULE_DESCRIPTION("Mediatek Hardware NAT");
