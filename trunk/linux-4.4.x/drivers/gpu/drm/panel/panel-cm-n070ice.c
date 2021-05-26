/*
 * Copyright (c) 2015 MediaTek Inc.
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

#include <linux/backlight.h>
#include <drm/drmP.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>

#include <linux/backlight.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

struct cm {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct regulator *supply;
	struct gpio_desc *enable_gpio;

	bool prepared;
	bool enabled;

	int error;
};

#define cm_dcs_write_seq(ctx, seq...) \
({\
	const u8 d[] = { seq };\
	BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64, "DCS sequence too big for stack");\
	cm_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

#define cm_dcs_write_seq_static(ctx, seq...) \
({\
	static const u8 d[] = { seq };\
	cm_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

static inline struct cm *panel_to_cm(struct drm_panel *panel)
{
	return container_of(panel, struct cm, panel);
}

static void cm_dcs_write(struct cm *ctx, const void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;

	if (ctx->error < 0)
		return;

	ret = mipi_dsi_dcs_write_buffer(dsi, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "fail to write cm_dcs_write ret\n");
		ctx->error = ret;
	}
}

static void cm_panel_init(struct cm *ctx)
{
	cm_dcs_write_seq_static(ctx, MIPI_DCS_SOFT_RESET);
	msleep(20);

	cm_dcs_write_seq_static(ctx, 0xae, 0x0b);
	cm_dcs_write_seq_static(ctx, 0xee, 0xea);
	cm_dcs_write_seq_static(ctx, 0xef, 0x5f);
	cm_dcs_write_seq_static(ctx, 0xf2, 0x68);
	cm_dcs_write_seq_static(ctx, 0xee, 0x00);
	cm_dcs_write_seq_static(ctx, 0xef, 0x00);
}

static int cm_disable(struct drm_panel *panel)
{
	struct cm *ctx = panel_to_cm(panel);

	if (!ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = false;

	return 0;
}

static int cm_unprepare(struct drm_panel *panel)
{
	struct cm *ctx = panel_to_cm(panel);

	if (!ctx->prepared)
		return 0;

	cm_dcs_write_seq_static(ctx, MIPI_DCS_SET_DISPLAY_OFF);
	cm_dcs_write_seq_static(ctx, MIPI_DCS_ENTER_SLEEP_MODE);
	msleep(120);

	if (ctx->enable_gpio)
		gpiod_set_value(ctx->enable_gpio, 0);

	regulator_disable(ctx->supply);

	ctx->error = 0;
	ctx->prepared = false;

	return 0;
}

static int cm_prepare(struct drm_panel *panel)
{
	struct cm *ctx = panel_to_cm(panel);
	int ret;

	if (ctx->prepared)
		return 0;

	ret = regulator_enable(ctx->supply);
	if (ret < 0) {
		dev_err(panel->dev, "failed to enable supply: %d\n", ret);
		return ret;
	}
	usleep_range(1000, 2000);
	gpiod_set_value(ctx->enable_gpio, 1);
	msleep(20);

	cm_panel_init(ctx);

	ret = ctx->error;
	if (ret < 0)
		cm_unprepare(panel);

	ctx->prepared = true;

	return ret;
}

static int cm_enable(struct drm_panel *panel)
{
	struct cm *ctx = panel_to_cm(panel);

	if (ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = true;

	return 0;
}

static const struct drm_display_mode default_mode = {
	.clock = 67860,
	.hdisplay = 800,
	.hsync_start = 800 + 14,
	.hsync_end = 800 + 14 + 1,
	.htotal = 800 + 14 + 1 + 30,
	.vdisplay = 1280,
	.vsync_start = 1280 + 20,
	.vsync_end = 1280 + 20 + 1,
	.vtotal = 1280 + 20 + 1 + 10,
	.vrefresh = 60,
};

struct panel_desc {
	const struct drm_display_mode *modes;
	unsigned int num_modes;

	unsigned int bpc;

	struct {
		unsigned int width;
		unsigned int height;
	} size;

	/**
	 * @prepare: the time (in milliseconds) that it takes for the panel to
	 *           become ready and start receiving video data
	 * @enable: the time (in milliseconds) that it takes for the panel to
	 *          display the first valid frame after starting to receive
	 *          video data
	 * @disable: the time (in milliseconds) that it takes for the panel to
	 *           turn the display off (no content is visible)
	 * @unprepare: the time (in milliseconds) that it takes for the panel
	 *             to power itself down completely
	 */
	struct {
		unsigned int prepare;
		unsigned int enable;
		unsigned int disable;
		unsigned int unprepare;
	} delay;
};

static int cm_get_modes(struct drm_panel *panel)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		dev_err(panel->drm->dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			default_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);
	drm_mode_probed_add(panel->connector, mode);

	panel->connector->display_info.width_mm = 94;
	panel->connector->display_info.height_mm = 150;
	panel->connector->display_info.bpc = 8;

	return 1;
}

static const struct drm_panel_funcs cm_drm_funcs = {
	.disable = cm_disable,
	.unprepare = cm_unprepare,
	.prepare = cm_prepare,
	.enable = cm_enable,
	.get_modes = cm_get_modes,
};

static int cm_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct cm *ctx;
	struct device_node *backlight;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(struct cm), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO
			  | MIPI_DSI_MODE_LPM
			  | MIPI_DSI_CLOCK_NON_CONTINUOUS;

	ctx->supply = devm_regulator_get(dev, "vsys");
	if (IS_ERR(ctx->supply))
		return PTR_ERR(ctx->supply);

	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		ctx->backlight = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!ctx->backlight)
			return -EPROBE_DEFER;
	}

	ctx->enable_gpio = devm_gpiod_get(dev, "pm-enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->enable_gpio)) {
		dev_err(dev, "cannot get enable-gpios %ld\n",
			PTR_ERR(ctx->enable_gpio));
		return PTR_ERR(ctx->enable_gpio);
	}
	ret = gpiod_direction_output(ctx->enable_gpio, 1);
	if (ret < 0) {
		dev_err(dev, "cannot configure enable-gpios %d\n", ret);
		return ret;
	}

	ctx->prepared = false;

	drm_panel_init(&ctx->panel);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &cm_drm_funcs;

	ret = drm_panel_add(&ctx->panel);
	if (ret < 0)
		return ret;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

	return ret;
}

static int cm_remove(struct mipi_dsi_device *dsi)
{
	struct cm *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id cm_of_match[] = {
	{ .compatible = "cm,n070ice", },
	{ }
};

MODULE_DEVICE_TABLE(of, cm_of_match);

static struct mipi_dsi_driver cm_driver = {
	.probe = cm_probe,
	.remove = cm_remove,
	.driver = {
		.name = "panel-n070ice",
		.owner = THIS_MODULE,
		.of_match_table = cm_of_match,
	},
};

module_mipi_dsi_driver(cm_driver);

MODULE_AUTHOR("Jitao Shi <jitao.shi@mediatek.com>");
MODULE_DESCRIPTION("CM n070ice LCD Panel Driver");
MODULE_LICENSE("GPL v2");

