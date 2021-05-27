#ifndef __SOC_MEDIATEK_INFRACFG_H
#define __SOC_MEDIATEK_INFRACFG_H

#define INFRA_TOPAXI_PROTECTEN		0x0220
#define INFRA_TOPAXI_PROTECTSTA1	0x0228

#define INFRA_TOPAXI_PROTECTEN1		0x0250
#define INFRA_TOPAXI_PROTECTSTA3	0x0258

#define INFRA_TOPAXI_PROTECTEN_SET	0x0260
#define INFRA_TOPAXI_PROTECTEN_CLR	0x0264

#define INFRA_TOPAXI_PROTECTEN1_SET	0x0270
#define INFRA_TOPAXI_PROTECTEN1_CLR	0x0274

/* MT8173 REG_SET_0 */
#define MT8173_TOP_AXI_PROT_EN_MCI_M2		BIT(0)
#define MT8173_TOP_AXI_PROT_EN_MM_M0		BIT(1)
#define MT8173_TOP_AXI_PROT_EN_MM_M1		BIT(2)
#define MT8173_TOP_AXI_PROT_EN_MMAPB_S		BIT(6)
#define MT8173_TOP_AXI_PROT_EN_L2C_M2		BIT(9)
#define MT8173_TOP_AXI_PROT_EN_L2SS_SMI		BIT(11)
#define MT8173_TOP_AXI_PROT_EN_L2SS_ADD		BIT(12)
#define MT8173_TOP_AXI_PROT_EN_CCI_M2		BIT(13)
#define MT8173_TOP_AXI_PROT_EN_MFG_S		BIT(14)
#define MT8173_TOP_AXI_PROT_EN_PERI_M0		BIT(15)
#define MT8173_TOP_AXI_PROT_EN_PERI_M1		BIT(16)
#define MT8173_TOP_AXI_PROT_EN_DEBUGSYS		BIT(17)
#define MT8173_TOP_AXI_PROT_EN_CQ_DMA		BIT(18)
#define MT8173_TOP_AXI_PROT_EN_GCPU		BIT(19)
#define MT8173_TOP_AXI_PROT_EN_IOMMU		BIT(20)
#define MT8173_TOP_AXI_PROT_EN_MFG_M0		BIT(21)
#define MT8173_TOP_AXI_PROT_EN_MFG_M1		BIT(22)
#define MT8173_TOP_AXI_PROT_EN_MFG_SNOOP_OUT	BIT(23)

/* MT2712 REG_SET_1 */
#define MT2712_TOP_AXI_PROT_EN_MFG_M0_ADB	BIT(0)
#define MT2712_TOP_AXI_PROT_EN_MFG_M1_ADB	BIT(1)

/* MT7622 REG_SET_0 */
#define MT7622_TOP_AXI_PROT_EN_ETHSYS	(BIT(3) | BIT(17))
#define MT7622_TOP_AXI_PROT_EN_HIF0	(BIT(24) | BIT(25))
#define MT7622_TOP_AXI_PROT_EN_HIF1	(BIT(26) | BIT(27) | BIT(28))

int mtk_infracfg_set_bus_protection(struct regmap *infracfg, u32 mask);
int mtk_infracfg_clear_bus_protection(struct regmap *infracfg, u32 mask);

int mtk_infracfg_set_bus_protection_ext(struct regmap *infracfg, u32 mask,
		u32 reg_set, u32 reg_sta, u32 reg_en);
int mtk_infracfg_clear_bus_protection_ext(struct regmap *infracfg, u32 mask,
		u32 reg_clr, u32 reg_sta, u32 reg_en);

#endif /* __SOC_MEDIATEK_INFRACFG_H */
