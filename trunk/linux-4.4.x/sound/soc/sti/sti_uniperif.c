/*
 * Copyright (C) STMicroelectronics SA 2015
 * Authors: Arnaud Pouliquen <arnaud.pouliquen@st.com>
 *          for STMicroelectronics.
 * License terms:  GNU General Public License (GPL), version 2
 */

#include <linux/module.h>
#include <linux/pinctrl/consumer.h>

#include "uniperif.h"

/*
 * sti_uniperiph_dai_create_ctrl
 * This function is used to create Ctrl associated to DAI but also pcm device.
 * Request is done by front end to associate ctrl with pcm device id
 */
static int sti_uniperiph_dai_create_ctrl(struct snd_soc_dai *dai)
{
	struct sti_uniperiph_data *priv = snd_soc_dai_get_drvdata(dai);
	struct uniperif *uni = priv->dai_data.uni;
	struct snd_kcontrol_new *ctrl;
	int i;

	if (!uni->num_ctrls)
		return 0;

	for (i = 0; i < uni->num_ctrls; i++) {
		/*
		 * Several Control can have same name. Controls are indexed on
		 * Uniperipheral instance ID
		 */
		ctrl = &uni->snd_ctrls[i];
		ctrl->index = uni->info->id;
		ctrl->device = uni->info->id;
	}

	return snd_soc_add_dai_controls(dai, uni->snd_ctrls, uni->num_ctrls);
}

/*
 * DAI
 */
int sti_uniperiph_dai_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct snd_dmaengine_dai_dma_data *dma_data;
	int transfer_size;

	transfer_size = params_channels(params) * UNIPERIF_FIFO_FRAMES;

	dma_data = snd_soc_dai_get_dma_data(dai, substream);
	dma_data->maxburst = transfer_size;

	return 0;
}

int sti_uniperiph_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sti_uniperiph_data *priv = snd_soc_dai_get_drvdata(dai);

	priv->dai_data.uni->daifmt = fmt;

	return 0;
}

static int sti_uniperiph_dai_suspend(struct snd_soc_dai *dai)
{
	struct sti_uniperiph_data *priv = snd_soc_dai_get_drvdata(dai);
	struct uniperif *uni = priv->dai_data.uni;
	int ret;

	/* The uniperipheral should be in stopped state */
	if (uni->state != UNIPERIF_STATE_STOPPED) {
		dev_err(uni->dev, "%s: invalid uni state( %d)",
			__func__, (int)uni->state);
		return -EBUSY;
	}

	/* Pinctrl: switch pinstate to sleep */
	ret = pinctrl_pm_select_sleep_state(uni->dev);
	if (ret)
		dev_err(uni->dev, "%s: failed to select pinctrl state",
			__func__);

	return ret;
}

static int sti_uniperiph_dai_resume(struct snd_soc_dai *dai)
{
	struct sti_uniperiph_data *priv = snd_soc_dai_get_drvdata(dai);
	struct uniperif *uni = priv->dai_data.uni;
	int ret;

	if (of_device_is_compatible(dai->dev->of_node, "st,sti-uni-player")) {
		ret = uni_player_resume(uni);
		if (ret)
			return ret;
	}

	/* pinctrl: switch pinstate to default */
	ret = pinctrl_pm_select_default_state(uni->dev);
	if (ret)
		dev_err(uni->dev, "%s: failed to select pinctrl state",
			__func__);

	return ret;
}

static int sti_uniperiph_dai_probe(struct snd_soc_dai *dai)
{
	struct sti_uniperiph_data *priv = snd_soc_dai_get_drvdata(dai);
	struct sti_uniperiph_dai *dai_data = &priv->dai_data;

	/* DMA settings*/
	if (of_device_is_compatible(dai->dev->of_node, "st,sti-uni-player"))
		snd_soc_dai_init_dma_data(dai, &dai_data->dma_data, NULL);
	else
		snd_soc_dai_init_dma_data(dai, NULL, &dai_data->dma_data);

	dai_data->dma_data.addr = dai_data->uni->fifo_phys_address;
	dai_data->dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;

	return sti_uniperiph_dai_create_ctrl(dai);
}

static const struct snd_soc_dai_driver sti_uniperiph_dai_template = {
	.probe = sti_uniperiph_dai_probe,
	.suspend = sti_uniperiph_dai_suspend,
	.resume = sti_uniperiph_dai_resume
};

static const struct snd_soc_component_driver sti_uniperiph_dai_component = {
	.name = "sti_cpu_dai",
};

static int sti_uniperiph_cpu_dai_of(struct device_node *node,
				    struct sti_uniperiph_data *priv)
{
	const char *str;
	int ret;
	struct device *dev = &priv->pdev->dev;
	struct sti_uniperiph_dai *dai_data = &priv->dai_data;
	struct snd_soc_dai_driver *dai = priv->dai;
	struct snd_soc_pcm_stream *stream;
	struct uniperif *uni;

	uni = devm_kzalloc(dev, sizeof(*uni), GFP_KERNEL);
	if (!uni)
		return -ENOMEM;

	*dai = sti_uniperiph_dai_template;
	ret = of_property_read_string(node, "dai-name", &str);
	if (ret < 0) {
		dev_err(dev, "%s: dai name missing.\n", __func__);
		return -EINVAL;
	}
	dai->name = str;

	/* Get resources */
	uni->mem_region = platform_get_resource(priv->pdev, IORESOURCE_MEM, 0);

	if (!uni->mem_region) {
		dev_err(dev, "Failed to get memory resource");
		return -ENODEV;
	}

	uni->base = devm_ioremap_resource(dev, uni->mem_region);

	if (IS_ERR(uni->base))
		return PTR_ERR(uni->base);

	uni->fifo_phys_address = uni->mem_region->start +
				     UNIPERIF_FIFO_DATA_OFFSET(uni);

	uni->irq = platform_get_irq(priv->pdev, 0);
	if (uni->irq < 0) {
		dev_err(dev, "Failed to get IRQ resource");
		return -ENXIO;
	}

	dai_data->uni = uni;

	if (of_device_is_compatible(node, "st,sti-uni-player")) {
		uni_player_init(priv->pdev, uni);
		stream = &dai->playback;
	} else {
		uni_reader_init(priv->pdev, uni);
		stream = &dai->capture;
	}
	dai->ops = uni->dai_ops;

	stream->stream_name = dai->name;
	stream->channels_min = uni->hw->channels_min;
	stream->channels_max = uni->hw->channels_max;
	stream->rates = uni->hw->rates;
	stream->formats = uni->hw->formats;

	return 0;
}

static const struct snd_dmaengine_pcm_config dmaengine_pcm_config = {
	.prepare_slave_config = snd_dmaengine_pcm_prepare_slave_config,
};

static int sti_uniperiph_probe(struct platform_device *pdev)
{
	struct sti_uniperiph_data *priv;
	struct device_node *node = pdev->dev.of_node;
	int ret;

	/* Allocate the private data and the CPU_DAI array */
	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	priv->dai = devm_kzalloc(&pdev->dev, sizeof(*priv->dai), GFP_KERNEL);
	if (!priv->dai)
		return -ENOMEM;

	priv->pdev = pdev;

	ret = sti_uniperiph_cpu_dai_of(node, priv);

	dev_set_drvdata(&pdev->dev, priv);

	ret = devm_snd_soc_register_component(&pdev->dev,
					      &sti_uniperiph_dai_component,
					      priv->dai, 1);
	if (ret < 0)
		return ret;

	return devm_snd_dmaengine_pcm_register(&pdev->dev,
					       &dmaengine_pcm_config, 0);
}

static const struct of_device_id snd_soc_sti_match[] = {
	{ .compatible = "st,sti-uni-player", },
	{ .compatible = "st,sti-uni-reader", },
	{},
};

static struct platform_driver sti_uniperiph_driver = {
	.driver = {
		.name = "sti-uniperiph-dai",
		.of_match_table = snd_soc_sti_match,
	},
	.probe = sti_uniperiph_probe,
};
module_platform_driver(sti_uniperiph_driver);

MODULE_DESCRIPTION("uniperipheral DAI driver");
MODULE_AUTHOR("Arnaud Pouliquen <arnaud.pouliquen@st.com>");
MODULE_LICENSE("GPL v2");
