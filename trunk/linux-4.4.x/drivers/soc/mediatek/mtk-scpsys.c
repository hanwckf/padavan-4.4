/*
 * Copyright (c) 2015 Pengutronix, Sascha Hauer <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/regulator/consumer.h>
#include <linux/soc/mediatek/infracfg.h>

#include <dt-bindings/power/mt2701-power.h>
#include <dt-bindings/power/mt2712-power.h>
#include <dt-bindings/power/mt7622-power.h>
#include <dt-bindings/power/mt8173-power.h>

#define SPM_VDE_PWR_CON			0x0210
#define SPM_MFG_PWR_CON			0x0214
#define SPM_VEN_PWR_CON			0x0230
#define SPM_ISP_PWR_CON			0x0238
#define SPM_DIS_PWR_CON			0x023c
#define SPM_CONN_PWR_CON		0x0280
#define SPM_VEN2_PWR_CON		0x0298
#define SPM_AUDIO_PWR_CON		0x029c	/* MT8173, MT2712 */
#define SPM_BDP_PWR_CON			0x029c	/* MT2701 */
#define SPM_ETH_PWR_CON			0x02a0
#define SPM_HIF_PWR_CON			0x02a4
#define SPM_IFR_MSC_PWR_CON		0x02a8
#define SPM_MFG_2D_PWR_CON		0x02c0
#define SPM_MFG_ASYNC_PWR_CON		0x02c4
#define SPM_USB_PWR_CON			0x02cc
#define SPM_USB2_PWR_CON		0x02d4	/* MT2712 */
#define SPM_ETHSYS_PWR_CON		0x02e0	/* MT7622 */
#define SPM_HIF0_PWR_CON		0x02e4	/* MT7622 */
#define SPM_HIF1_PWR_CON		0x02e8	/* MT7622 */

#define SPM_PWR_STATUS			0x060c
#define SPM_PWR_STATUS_2ND		0x0610

#define PWR_RST_B_BIT			BIT(0)
#define PWR_ISO_BIT			BIT(1)
#define PWR_ON_BIT			BIT(2)
#define PWR_ON_2ND_BIT			BIT(3)
#define PWR_CLK_DIS_BIT			BIT(4)

#define PWR_STATUS_CONN			BIT(1)
#define PWR_STATUS_DISP			BIT(3)
#define PWR_STATUS_MFG			BIT(4)
#define PWR_STATUS_ISP			BIT(5)
#define PWR_STATUS_VDEC			BIT(7)
#define PWR_STATUS_BDP			BIT(14)
#define PWR_STATUS_ETH			BIT(15)
#define PWR_STATUS_HIF			BIT(16)
#define PWR_STATUS_IFR_MSC		BIT(17)
#define PWR_STATUS_USB2			BIT(19)	/* MT2712 */
#define PWR_STATUS_VENC_LT		BIT(20)
#define PWR_STATUS_VENC			BIT(21)
#define PWR_STATUS_MFG_2D		BIT(22)	/* MT8173 */
#define PWR_STATUS_MFG_ASYNC		BIT(23)	/* MT8173 */
#define PWR_STATUS_AUDIO		BIT(24)	/* MT8173, MT2712 */
#define PWR_STATUS_USB			BIT(25)	/* MT8173, MT2712 */
#define PWR_STATUS_ETHSYS		BIT(24)	/* MT7622 */
#define PWR_STATUS_HIF0			BIT(25)	/* MT7622 */
#define PWR_STATUS_HIF1			BIT(26)	/* MT7622 */

enum clk_id {
	CLK_NONE,
	CLK_MM,
	CLK_MFG,
	CLK_VENC,
	CLK_VENC_LT,
	CLK_ETHIF,
	CLK_HIF_SEL,
	CLK_MAX,
};

#define MAX_CLKS	2

struct bus_prot_ext {
	u32 set_ofs;
	u32 clr_ofs;
	u32 en_ofs;
	u32 sta_ofs;
};

struct scp_domain_data {
	const char *name;
	u32 sta_mask;
	int ctl_offs;
	u32 sram_pdn_bits;
	u32 sram_pdn_ack_bits;
	u32 bus_prot_mask;
	struct bus_prot_ext bp_ext;
	enum clk_id clk_id[MAX_CLKS];
	bool active_wakeup;
};

struct scp;

struct scp_domain {
	struct generic_pm_domain genpd;
	struct scp *scp;
	struct clk *clk[MAX_CLKS];
	u32 sta_mask;
	void __iomem *ctl_addr;
	u32 sram_pdn_bits;
	u32 sram_pdn_ack_bits;
	u32 bus_prot_mask;
	struct bus_prot_ext bp_ext;
	bool active_wakeup;
};

struct scp {
	struct scp_domain *domains;
	struct genpd_onecell_data pd_data;
	struct device *dev;
	void __iomem *base;
	struct regmap *infracfg;
};

static int scpsys_domain_is_on(struct scp_domain *scpd)
{
	struct scp *scp = scpd->scp;

	u32 status = readl(scp->base + SPM_PWR_STATUS) & scpd->sta_mask;
	u32 status2 = readl(scp->base + SPM_PWR_STATUS_2ND) & scpd->sta_mask;

	/*
	 * A domain is on when both status bits are set. If only one is set
	 * return an error. This happens while powering up a domain
	 */

	if (status && status2)
		return true;
	if (!status && !status2)
		return false;

	return -EINVAL;
}

static int scpsys_power_on(struct generic_pm_domain *genpd)
{
	struct scp_domain *scpd = container_of(genpd, struct scp_domain, genpd);
	struct scp *scp = scpd->scp;
	unsigned long timeout;
	bool expired;
	void __iomem *ctl_addr = scpd->ctl_addr;
	u32 sram_pdn_ack = scpd->sram_pdn_ack_bits;
	u32 val;
	int ret;
	int i;

	for (i = 0; i < MAX_CLKS && scpd->clk[i]; i++) {
		ret = clk_prepare_enable(scpd->clk[i]);
		if (ret) {
			for (--i; i >= 0; i--)
				clk_disable_unprepare(scpd->clk[i]);

			goto err_clk;
		}
	}

	val = readl(ctl_addr);
	val |= PWR_ON_BIT;
	writel(val, ctl_addr);
	val |= PWR_ON_2ND_BIT;
	writel(val, ctl_addr);

	/* wait until PWR_ACK = 1 */
	timeout = jiffies + HZ;
	expired = false;
	while (1) {
		ret = scpsys_domain_is_on(scpd);
		if (ret > 0)
			break;

		if (expired) {
			ret = -ETIMEDOUT;
			goto err_pwr_ack;
		}

		cpu_relax();

		if (time_after(jiffies, timeout))
			expired = true;
	}

	val &= ~PWR_CLK_DIS_BIT;
	writel(val, ctl_addr);

	val &= ~PWR_ISO_BIT;
	writel(val, ctl_addr);

	val |= PWR_RST_B_BIT;
	writel(val, ctl_addr);

	val &= ~scpd->sram_pdn_bits;
	writel(val, ctl_addr);

	/* wait until SRAM_PDN_ACK all 0 */
	timeout = jiffies + HZ;
	expired = false;
	while (sram_pdn_ack && (readl(ctl_addr) & sram_pdn_ack)) {

		if (expired) {
			ret = -ETIMEDOUT;
			goto err_pwr_ack;
		}

		cpu_relax();

		if (time_after(jiffies, timeout))
			expired = true;
	}

	if (scpd->bus_prot_mask) {
		if (((scpd->bp_ext.set_ofs && scpd->bp_ext.clr_ofs) || scpd->bp_ext.en_ofs) &&
				scpd->bp_ext.sta_ofs)
			ret = mtk_infracfg_clear_bus_protection_ext(scp->infracfg,
					scpd->bus_prot_mask, scpd->bp_ext.clr_ofs,
					scpd->bp_ext.sta_ofs, scpd->bp_ext.en_ofs);
		else
			ret = mtk_infracfg_clear_bus_protection(scp->infracfg,
					scpd->bus_prot_mask);
		if (ret)
			goto err_pwr_ack;
	}

	return 0;

err_pwr_ack:
	for (i = MAX_CLKS - 1; i >= 0; i--) {
		if (scpd->clk[i])
			clk_disable_unprepare(scpd->clk[i]);
	}
err_clk:
	dev_err(scp->dev, "Failed to power on domain %s\n", genpd->name);

	return ret;
}

static int scpsys_power_off(struct generic_pm_domain *genpd)
{
	struct scp_domain *scpd = container_of(genpd, struct scp_domain, genpd);
	struct scp *scp = scpd->scp;
	unsigned long timeout;
	bool expired;
	void __iomem *ctl_addr = scpd->ctl_addr;
	u32 pdn_ack = scpd->sram_pdn_ack_bits;
	u32 val;
	int ret;
	int i;

	if (scpd->bus_prot_mask) {
		if (((scpd->bp_ext.set_ofs && scpd->bp_ext.clr_ofs) || scpd->bp_ext.en_ofs) &&
				scpd->bp_ext.sta_ofs)
			ret = mtk_infracfg_set_bus_protection_ext(scp->infracfg,
					scpd->bus_prot_mask, scpd->bp_ext.set_ofs,
					scpd->bp_ext.sta_ofs, scpd->bp_ext.en_ofs);
		else
			ret = mtk_infracfg_set_bus_protection(scp->infracfg,
					scpd->bus_prot_mask);
		if (ret)
			goto out;
	}

	val = readl(ctl_addr);
	val |= scpd->sram_pdn_bits;
	writel(val, ctl_addr);

	/* wait until SRAM_PDN_ACK all 1 */
	timeout = jiffies + HZ;
	expired = false;
	while (pdn_ack && (readl(ctl_addr) & pdn_ack) != pdn_ack) {
		if (expired) {
			ret = -ETIMEDOUT;
			goto out;
		}

		cpu_relax();

		if (time_after(jiffies, timeout))
			expired = true;
	}

	val |= PWR_ISO_BIT;
	writel(val, ctl_addr);

	val &= ~PWR_RST_B_BIT;
	writel(val, ctl_addr);

	val |= PWR_CLK_DIS_BIT;
	writel(val, ctl_addr);

	val &= ~PWR_ON_BIT;
	writel(val, ctl_addr);

	val &= ~PWR_ON_2ND_BIT;
	writel(val, ctl_addr);

	/* wait until PWR_ACK = 0 */
	timeout = jiffies + HZ;
	expired = false;
	while (1) {
		ret = scpsys_domain_is_on(scpd);
		if (ret == 0)
			break;

		if (expired) {
			ret = -ETIMEDOUT;
			goto out;
		}

		cpu_relax();

		if (time_after(jiffies, timeout))
			expired = true;
	}

	for (i = 0; i < MAX_CLKS && scpd->clk[i]; i++)
		clk_disable_unprepare(scpd->clk[i]);

	return 0;

out:
	dev_err(scp->dev, "Failed to power off domain %s\n", genpd->name);

	return ret;
}

static bool scpsys_active_wakeup(struct device *dev)
{
	struct generic_pm_domain *genpd;
	struct scp_domain *scpd;

	genpd = pd_to_genpd(dev->pm_domain);
	scpd = container_of(genpd, struct scp_domain, genpd);

	return scpd->active_wakeup;
}

static void init_clks(struct platform_device *pdev, struct clk *clk[CLK_MAX])
{
	enum clk_id clk_ids[] = {
		CLK_MM,
		CLK_MFG,
		CLK_VENC,
		CLK_VENC_LT,
		CLK_ETHIF,
		CLK_HIF_SEL
	};

	static const char * const clk_names[] = {
		"mm",
		"mfg",
		"venc",
		"venc_lt",
		"ethif",
		"hif_sel",
	};

	int i;

	for (i = 0; i < ARRAY_SIZE(clk_ids); i++)
		clk[clk_ids[i]] = devm_clk_get(&pdev->dev, clk_names[i]);
}

static struct scp *init_scp(struct platform_device *pdev,
			const struct scp_domain_data *scp_domain_data, int num)
{
	struct genpd_onecell_data *pd_data;
	struct resource *res;
	int i, j;
	struct scp *scp;
	struct clk *clk[CLK_MAX];

	scp = devm_kzalloc(&pdev->dev, sizeof(*scp), GFP_KERNEL);
	if (!scp)
		return ERR_PTR(-ENOMEM);

	scp->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	scp->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(scp->base))
		return ERR_CAST(scp->base);

	scp->infracfg = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
			"infracfg");
	if (IS_ERR(scp->infracfg)) {
		dev_err(&pdev->dev, "Cannot find infracfg controller: %ld\n",
				PTR_ERR(scp->infracfg));
		return ERR_CAST(scp->infracfg);
	}

	scp->domains = devm_kzalloc(&pdev->dev,
				sizeof(*scp->domains) * num, GFP_KERNEL);
	if (!scp->domains)
		return ERR_PTR(-ENOMEM);

	pd_data = &scp->pd_data;

	pd_data->domains = devm_kzalloc(&pdev->dev,
			sizeof(*pd_data->domains) * num, GFP_KERNEL);
	if (!pd_data->domains)
		return ERR_PTR(-ENOMEM);

	pd_data->num_domains = num;

	init_clks(pdev, clk);

	for (i = 0; i < num; i++) {
		struct scp_domain *scpd = &scp->domains[i];
		struct generic_pm_domain *genpd = &scpd->genpd;
		const struct scp_domain_data *data = &scp_domain_data[i];

		for (j = 0; j < MAX_CLKS && data->clk_id[j]; j++) {
			struct clk *c = clk[data->clk_id[j]];

			if (IS_ERR(c)) {
				dev_err(&pdev->dev, "%s: clk unavailable\n",
					data->name);
				return ERR_CAST(c);
			}

			scpd->clk[j] = c;
		}

		pd_data->domains[i] = genpd;
		scpd->scp = scp;

		scpd->sta_mask = data->sta_mask;
		scpd->ctl_addr = scp->base + data->ctl_offs;
		scpd->sram_pdn_bits = data->sram_pdn_bits;
		scpd->sram_pdn_ack_bits = data->sram_pdn_ack_bits;
		scpd->bus_prot_mask = data->bus_prot_mask;
		scpd->bp_ext.set_ofs = data->bp_ext.set_ofs;
		scpd->bp_ext.clr_ofs = data->bp_ext.clr_ofs;
		scpd->bp_ext.en_ofs = data->bp_ext.en_ofs;
		scpd->bp_ext.sta_ofs = data->bp_ext.sta_ofs;
		scpd->active_wakeup = data->active_wakeup;
		for (j = 0; j < MAX_CLKS && data->clk_id[j]; j++)
			scpd->clk[j] = clk[data->clk_id[j]];

		genpd->name = data->name;
		genpd->power_off = scpsys_power_off;
		genpd->power_on = scpsys_power_on;
		genpd->dev_ops.active_wakeup = scpsys_active_wakeup;
	}

	return scp;
}

static void mtk_register_power_domains(struct platform_device *pdev,
				struct scp *scp, int num)
{
	struct genpd_onecell_data *pd_data;
	int i, ret;

	for (i = 0; i < num; i++) {
		struct scp_domain *scpd = &scp->domains[i];
		struct generic_pm_domain *genpd = &scpd->genpd;

		/*
		 * Initially turn on all domains to make the domains usable
		 * with !CONFIG_PM and to get the hardware in sync with the
		 * software.  The unused domains will be switched off during
		 * late_init time.
		 */
		genpd->power_on(genpd);

		pm_genpd_init(genpd, NULL, false);
	}

	/*
	 * We are not allowed to fail here since there is no way to unregister
	 * a power domain. Once registered above we have to keep the domains
	 * valid.
	 */

	pd_data = &scp->pd_data;

	ret = of_genpd_add_provider_onecell(pdev->dev.of_node, pd_data);
	if (ret)
		dev_err(&pdev->dev, "Failed to add OF provider: %d\n", ret);
}

/*
 * MT2701 power domain support
 */

static const struct scp_domain_data scp_domain_data_mt2701[] = {
	[MT2701_POWER_DOMAIN_CONN] = {
		.name = "conn",
		.sta_mask = PWR_STATUS_CONN,
		.ctl_offs = SPM_CONN_PWR_CON,
		.bus_prot_mask = 0x0104,
		.active_wakeup = true,
	},
	[MT2701_POWER_DOMAIN_DISP] = {
		.name = "disp",
		.sta_mask = PWR_STATUS_DISP,
		.ctl_offs = SPM_DIS_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.clk_id = {CLK_MM},
		.bus_prot_mask = 0x0002,
		.active_wakeup = true,
	},
	[MT2701_POWER_DOMAIN_MFG] = {
		.name = "mfg",
		.sta_mask = PWR_STATUS_MFG,
		.ctl_offs = SPM_MFG_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(12, 12),
		.clk_id = {CLK_MFG},
		.active_wakeup = true,
	},
	[MT2701_POWER_DOMAIN_VDEC] = {
		.name = "vdec",
		.sta_mask = PWR_STATUS_VDEC,
		.ctl_offs = SPM_VDE_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(12, 12),
		.clk_id = {CLK_MM},
		.active_wakeup = true,
	},
	[MT2701_POWER_DOMAIN_ISP] = {
		.name = "isp",
		.sta_mask = PWR_STATUS_ISP,
		.ctl_offs = SPM_ISP_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(13, 12),
		.clk_id = {CLK_MM},
		.active_wakeup = true,
	},
	[MT2701_POWER_DOMAIN_BDP] = {
		.name = "bdp",
		.sta_mask = PWR_STATUS_BDP,
		.ctl_offs = SPM_BDP_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.active_wakeup = true,
	},
	[MT2701_POWER_DOMAIN_ETH] = {
		.name = "eth",
		.sta_mask = PWR_STATUS_ETH,
		.ctl_offs = SPM_ETH_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.clk_id = {CLK_ETHIF},
		.active_wakeup = true,
	},
	[MT2701_POWER_DOMAIN_HIF] = {
		.name = "hif",
		.sta_mask = PWR_STATUS_HIF,
		.ctl_offs = SPM_HIF_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.clk_id = {CLK_ETHIF},
		.active_wakeup = true,
	},
	[MT2701_POWER_DOMAIN_IFR_MSC] = {
		.name = "ifr_msc",
		.sta_mask = PWR_STATUS_IFR_MSC,
		.ctl_offs = SPM_IFR_MSC_PWR_CON,
		.active_wakeup = true,
	},
};

#define NUM_DOMAINS_MT2701	ARRAY_SIZE(scp_domain_data_mt2701)

static int __init scpsys_probe_mt2701(struct platform_device *pdev)
{
	struct scp *scp;

	scp = init_scp(pdev, scp_domain_data_mt2701, NUM_DOMAINS_MT2701);
	if (IS_ERR(scp))
		return PTR_ERR(scp);

	mtk_register_power_domains(pdev, scp, NUM_DOMAINS_MT2701);

	return 0;
}

/*
 * MT2712 power domain support
 */

static const struct scp_domain_data scp_domain_data_mt2712[] = {
	[MT2712_POWER_DOMAIN_MM] = {
		.name = "mm",
		.sta_mask = PWR_STATUS_DISP,
		.ctl_offs = SPM_DIS_PWR_CON,
		.sram_pdn_bits = GENMASK(8, 8),
		.sram_pdn_ack_bits = GENMASK(12, 12),
		.clk_id = {CLK_NONE},
		.active_wakeup = true,
	},
	[MT2712_POWER_DOMAIN_VDEC] = {
		.name = "vdec",
		.sta_mask = PWR_STATUS_VDEC,
		.ctl_offs = SPM_VDE_PWR_CON,
		.sram_pdn_bits = GENMASK(8, 8),
		.sram_pdn_ack_bits = GENMASK(12, 12),
		.clk_id = {CLK_MM},
		.active_wakeup = true,
	},
	[MT2712_POWER_DOMAIN_VENC] = {
		.name = "venc",
		.sta_mask = PWR_STATUS_VENC,
		.ctl_offs = SPM_VEN_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.clk_id = {CLK_MM},
		.active_wakeup = true,
	},
	[MT2712_POWER_DOMAIN_ISP] = {
		.name = "isp",
		.sta_mask = PWR_STATUS_ISP,
		.ctl_offs = SPM_ISP_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(13, 12),
		.clk_id = {CLK_MM},
		.active_wakeup = true,
	},
	[MT2712_POWER_DOMAIN_AUDIO] = {
		.name = "audio",
		.sta_mask = PWR_STATUS_AUDIO,
		.ctl_offs = SPM_AUDIO_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.clk_id = {CLK_NONE},
		.active_wakeup = true,
	},
	[MT2712_POWER_DOMAIN_USB] = {
		.name = "usb",
		.sta_mask = PWR_STATUS_USB,
		.ctl_offs = SPM_USB_PWR_CON,
		.sram_pdn_bits = GENMASK(10, 8),
		.sram_pdn_ack_bits = GENMASK(14, 12),
		.clk_id = {CLK_NONE},
		.active_wakeup = true,
	},
	[MT2712_POWER_DOMAIN_USB2] = {
		.name = "usb2",
		.sta_mask = PWR_STATUS_USB2,
		.ctl_offs = SPM_USB2_PWR_CON,
		.sram_pdn_bits = GENMASK(10, 8),
		.sram_pdn_ack_bits = GENMASK(14, 12),
		.clk_id = {CLK_NONE},
		.active_wakeup = true,
	},
	[MT2712_POWER_DOMAIN_MFG] = {
		.name = "mfg",
		.sta_mask = PWR_STATUS_MFG,
		.ctl_offs = SPM_MFG_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(19, 16),
		.clk_id = {CLK_NONE},
		.active_wakeup = true,
	},
};

#define NUM_DOMAINS_MT2712	ARRAY_SIZE(scp_domain_data_mt2712)

static int __init scpsys_probe_mt2712(struct platform_device *pdev)
{
	struct scp *scp;

	scp = init_scp(pdev, scp_domain_data_mt2712, NUM_DOMAINS_MT2712);
	if (IS_ERR(scp))
		return PTR_ERR(scp);

	mtk_register_power_domains(pdev, scp, NUM_DOMAINS_MT2712);

	return 0;
}

/*
 * MT7622 power domain support
 */

static const struct scp_domain_data scp_domain_data_mt7622[] = {
	[MT7622_POWER_DOMAIN_ETHSYS] = {
		.name = "ethsys",
		.sta_mask = PWR_STATUS_ETHSYS,
		.ctl_offs = SPM_ETHSYS_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.clk_id = {CLK_NONE},
		.bus_prot_mask = MT7622_TOP_AXI_PROT_EN_ETHSYS,
		.active_wakeup = true,
	},
	[MT7622_POWER_DOMAIN_HIF0] = {
		.name = "hif0",
		.sta_mask = PWR_STATUS_HIF0,
		.ctl_offs = SPM_HIF0_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.clk_id = {CLK_HIF_SEL},
		.bus_prot_mask = MT7622_TOP_AXI_PROT_EN_HIF0,
		.active_wakeup = true,
	},
	[MT7622_POWER_DOMAIN_HIF1] = {
		.name = "hif1",
		.sta_mask = PWR_STATUS_HIF1,
		.ctl_offs = SPM_HIF1_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.clk_id = {CLK_HIF_SEL},
		.bus_prot_mask = MT7622_TOP_AXI_PROT_EN_HIF1,
		.active_wakeup = true,
	},
};

#define NUM_DOMAINS_MT7622	ARRAY_SIZE(scp_domain_data_mt7622)

static int __init scpsys_probe_mt7622(struct platform_device *pdev)
{
	struct scp *scp;

	scp = init_scp(pdev, scp_domain_data_mt7622, NUM_DOMAINS_MT7622);
	if (IS_ERR(scp))
		return PTR_ERR(scp);

	mtk_register_power_domains(pdev, scp, NUM_DOMAINS_MT7622);

	return 0;
}

/*
 * MT8173 power domain support
 */

static const struct scp_domain_data scp_domain_data_mt8173[] = {
	[MT8173_POWER_DOMAIN_VDEC] = {
		.name = "vdec",
		.sta_mask = PWR_STATUS_VDEC,
		.ctl_offs = SPM_VDE_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(12, 12),
		.clk_id = {CLK_MM},
	},
	[MT8173_POWER_DOMAIN_VENC] = {
		.name = "venc",
		.sta_mask = PWR_STATUS_VENC,
		.ctl_offs = SPM_VEN_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.clk_id = {CLK_MM, CLK_VENC},
	},
	[MT8173_POWER_DOMAIN_ISP] = {
		.name = "isp",
		.sta_mask = PWR_STATUS_ISP,
		.ctl_offs = SPM_ISP_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(13, 12),
		.clk_id = {CLK_MM},
	},
	[MT8173_POWER_DOMAIN_MM] = {
		.name = "mm",
		.sta_mask = PWR_STATUS_DISP,
		.ctl_offs = SPM_DIS_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(12, 12),
		.clk_id = {CLK_MM},
		.bus_prot_mask = MT8173_TOP_AXI_PROT_EN_MM_M0 |
			MT8173_TOP_AXI_PROT_EN_MM_M1,
	},
	[MT8173_POWER_DOMAIN_VENC_LT] = {
		.name = "venc_lt",
		.sta_mask = PWR_STATUS_VENC_LT,
		.ctl_offs = SPM_VEN2_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.clk_id = {CLK_MM, CLK_VENC_LT},
	},
	[MT8173_POWER_DOMAIN_AUDIO] = {
		.name = "audio",
		.sta_mask = PWR_STATUS_AUDIO,
		.ctl_offs = SPM_AUDIO_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.clk_id = {CLK_NONE},
	},
	[MT8173_POWER_DOMAIN_USB] = {
		.name = "usb",
		.sta_mask = PWR_STATUS_USB,
		.ctl_offs = SPM_USB_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.clk_id = {CLK_NONE},
		.active_wakeup = true,
	},
	[MT8173_POWER_DOMAIN_MFG_ASYNC] = {
		.name = "mfg_async",
		.sta_mask = PWR_STATUS_MFG_ASYNC,
		.ctl_offs = SPM_MFG_ASYNC_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = 0,
		.clk_id = {CLK_MFG},
	},
	[MT8173_POWER_DOMAIN_MFG_2D] = {
		.name = "mfg_2d",
		.sta_mask = PWR_STATUS_MFG_2D,
		.ctl_offs = SPM_MFG_2D_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(13, 12),
		.clk_id = {CLK_NONE},
	},
	[MT8173_POWER_DOMAIN_MFG] = {
		.name = "mfg",
		.sta_mask = PWR_STATUS_MFG,
		.ctl_offs = SPM_MFG_PWR_CON,
		.sram_pdn_bits = GENMASK(13, 8),
		.sram_pdn_ack_bits = GENMASK(21, 16),
		.clk_id = {CLK_NONE},
		.bus_prot_mask = MT8173_TOP_AXI_PROT_EN_MFG_S |
			MT8173_TOP_AXI_PROT_EN_MFG_M0 |
			MT8173_TOP_AXI_PROT_EN_MFG_M1 |
			MT8173_TOP_AXI_PROT_EN_MFG_SNOOP_OUT,
	},
};

#define NUM_DOMAINS_MT8173	ARRAY_SIZE(scp_domain_data_mt8173)

static int __init scpsys_probe_mt8173(struct platform_device *pdev)
{
	struct scp *scp;
	struct genpd_onecell_data *pd_data;
	int ret;

	scp = init_scp(pdev, scp_domain_data_mt8173, NUM_DOMAINS_MT8173);
	if (IS_ERR(scp))
		return PTR_ERR(scp);

	mtk_register_power_domains(pdev, scp, NUM_DOMAINS_MT8173);

	pd_data = &scp->pd_data;

	ret = pm_genpd_add_subdomain(pd_data->domains[MT8173_POWER_DOMAIN_MFG_ASYNC],
		pd_data->domains[MT8173_POWER_DOMAIN_MFG_2D]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	ret = pm_genpd_add_subdomain(pd_data->domains[MT8173_POWER_DOMAIN_MFG_2D],
		pd_data->domains[MT8173_POWER_DOMAIN_MFG]);
	if (ret && IS_ENABLED(CONFIG_PM))
		dev_err(&pdev->dev, "Failed to add subdomain: %d\n", ret);

	return 0;
}

/*
 * scpsys driver init
 */

static const struct of_device_id of_scpsys_match_tbl[] = {
	{
		.compatible = "mediatek,mt2701-scpsys",
		.data = scpsys_probe_mt2701,
	}, {
		.compatible = "mediatek,mt2712-scpsys",
		.data = scpsys_probe_mt2712,
	}, {
		.compatible = "mediatek,mt7622-scpsys",
		.data = scpsys_probe_mt7622,
	}, {
		.compatible = "mediatek,mt8173-scpsys",
		.data = scpsys_probe_mt8173,
	}, {
		/* sentinel */
	}
};

static int scpsys_probe(struct platform_device *pdev)
{
	int (*probe)(struct platform_device *);
	const struct of_device_id *of_id;

	of_id = of_match_node(of_scpsys_match_tbl, pdev->dev.of_node);
	if (!of_id || !of_id->data)
		return -EINVAL;

	probe = of_id->data;

	return probe(pdev);
}

static struct platform_driver scpsys_drv = {
	.probe = scpsys_probe,
	.driver = {
		.name = "mtk-scpsys",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(of_scpsys_match_tbl),
	},
};

static int __init scpsys_drv_init(void)
{
	return platform_driver_register(&scpsys_drv);
}

/*
 * There are some Mediatek drivers which depend on the power domain driver need
 * to probe in earlier initcall levels. So scpsys driver also need to probe
 * earlier.
 *
 * IOMMU(M4U) and SMI drivers for example. SMI is a bridge between IOMMU and
 * multimedia HW. IOMMU depends on SMI, and SMI is a power domain consumer,
 * so the proper probe sequence should be scpsys -> SMI -> IOMMU driver.
 * IOMMU drivers are initialized during subsys_init by default, so we need to
 * move SMI and scpsys drivers to subsys_init or earlier init levels.
 */
subsys_initcall(scpsys_drv_init);
