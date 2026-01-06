/*
 * Copyright (c) 2021 Marc Reilly - Creative Product Design
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT zephyr_spis_bitbang
#include <zephyr/logging/log_ctrl.h>

// TODO: DELETE
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
LOG_MODULE_REGISTER(spis_bitbang, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include "spis_context.h"

struct spis_bitbang_data {
	struct spi_context ctx;
	int bits;
	int wait_us;
	int dfs;
};

struct spis_bitbang_config {
	struct gpio_dt_spec clk_gpio;
	struct gpio_dt_spec mosi_gpio;
	struct gpio_dt_spec miso_gpio;
};

static int spis_bitbang_configure(const struct spis_bitbang_config *info,
			    struct spis_bitbang_data *data,
			    const struct spi_config *config)
{
	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half Duplex not supported");
		return -ENOTSUP;
	}

	if (config->operation & SPI_OP_MODE_MASTER) {
		LOG_ERR("Master mode not supported");
		return -ENOTSUP;
	}

	if (config->operation & (SPI_LINES_DUAL | SPI_LINES_QUAD | SPI_LINES_OCTAL)) {
		LOG_ERR("Unsupported configuration");
		return -ENOTSUP;
	}

	const int bits = SPI_WORD_SIZE_GET(config->operation);

	if (bits > 32) {
		LOG_ERR("Word sizes > 32 bits not supported");
		return -ENOTSUP;
	}

	data->bits = bits;
	data->dfs = ((data->bits - 1) / 8) + 1;

	/* As there is no uint24_t, it is assumed uint32_t will be used as the buffer base type. */
	if (data->dfs == 3) {
		data->dfs = 4;
	}

	data->ctx.config = config;

	return 0;
}

static int spis_bitbang_transceive(const struct device *dev,
			      const struct spi_config *spi_cfg,
			      const struct spi_buf_set *tx_bufs,
			      const struct spi_buf_set *rx_bufs)
{
	// look into spi_context_lock
	const struct spis_bitbang_config *info = dev->config;
	struct spis_bitbang_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int rc;
	const struct gpio_dt_spec *miso = &info->miso_gpio;
	const struct gpio_dt_spec *mosi = &info->mosi_gpio;
	const struct gpio_dt_spec *cs = data->ctx.cs_gpios;

	rc = spis_bitbang_configure(info, data, spi_cfg);
	if (rc < 0) {
		return rc;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->dfs);

	int clock_state = 0;
	int cpha = 0;
	bool loop = false;
	bool lsb = false;

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
		clock_state = 1;
	}
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
		cpha = 1;
	}
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_LOOP) {
		loop = true;
	}
	if (spi_cfg->operation & SPI_TRANSFER_LSB) {
		lsb = true;
	}

	// wait for CS to go low
	while (gpio_pin_get_dt(cs) == 0) {
		/* no op */
	}

	while (gpio_pin_get_dt(cs) && 
		(spi_context_tx_buf_on(ctx) || spi_context_rx_buf_on(ctx))) {
		uint32_t w = 0;

		if (ctx->tx_len) {
			switch (data->dfs) {
			case 4:
			case 3:
				w = *(uint32_t *)(ctx->tx_buf);
				break;
			case 2:
				w = *(uint16_t *)(ctx->tx_buf);
				break;
			case 1:
				w = *(uint8_t *)(ctx->tx_buf);
				break;
			}
		}

		uint32_t r = 0;
		uint8_t i = 0;
		int b = 0;
		bool do_read = false;

		if (spi_context_rx_buf_on(ctx)) do_read = true;

		while (gpio_pin_get_dt(cs) == 1 && i < data->bits) {
			const int shift = lsb ? i : (data->bits - 1 - i);
			const int d = (w >> shift) & 0x1;

			b = 0;

			/* setup data out first thing */
			gpio_pin_set_dt(miso, d);

			/* wait until first (leading) clock edge */
			while (gpio_pin_get_dt(&info->clk_gpio) == clock_state) {
				/* no op */
			}

			if (!loop && do_read && !cpha) {
				b = gpio_pin_get_dt(mosi);
			}

			/* wait until second (trailing) clock edge */
			while (gpio_pin_get_dt(&info->clk_gpio) != clock_state) {
				/* no op */
			}

			if (!loop && do_read && cpha) {
				b = gpio_pin_get_dt(mosi);
			}

			if (loop) {
				b = d;
			}

			r |= (b ? 0x1 : 0x0) << shift;

			++i;
		}

		if (spi_context_rx_buf_on(ctx)) {
			switch (data->dfs) {
			case 4:
			case 3:
				*(uint32_t *)(ctx->rx_buf) = r;
				break;
			case 2:
				*(uint16_t *)(ctx->rx_buf) = r;
				break;
			case 1:
				*(uint8_t *)(ctx->rx_buf) = r;
				break;
			}
		}

		LOG_DBG(" w: %04x, r: %04x , do_read: %d", w, r, do_read);

		spi_context_update_tx(ctx, data->dfs, 1);
		spi_context_update_rx(ctx, data->dfs, 1);
	}

	spi_context_complete(ctx, dev, 0);

	return 0;
}

#ifdef CONFIG_SPI_ASYNC
static int spis_bitbang_transceive_async(const struct device *dev,
				    const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs,
				    spi_callback_t cb,
				    void *userdata)
{
	return -ENOTSUP;
}
#endif

int spis_bitbang_release(const struct device *dev,
			  const struct spi_config *config)
{
	struct spis_bitbang_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	spi_context_unlock_unconditionally(ctx);
	return 0;
}

int spis_bitbang_init(const struct device *dev)
{
	const struct spis_bitbang_config *config = dev->config;
	struct spis_bitbang_data *data = dev->data;
	int rc;

	if (!gpio_is_ready_dt(&config->clk_gpio)) {
		LOG_ERR("GPIO port for clk pin is not ready");
		return -ENODEV;
	}
	rc = gpio_pin_configure_dt(&config->clk_gpio, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Couldn't configure clk pin; (%d)", rc);
		return rc;
	}

	if (!gpio_is_ready_dt(&config->mosi_gpio)) {
		LOG_ERR("GPIO port for mosi pin is not ready");
		return -ENODEV;
	}
	rc = gpio_pin_configure_dt(&config->mosi_gpio, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Couldn't configure mosi pin; (%d)", rc);
		return rc;
	}

	if (!gpio_is_ready_dt(&config->miso_gpio)) {
		LOG_ERR("GPIO port for miso pin is not ready");
		return -ENODEV;
	}
	rc = gpio_pin_configure_dt(&config->miso_gpio, GPIO_OUTPUT_INACTIVE);
	if (rc < 0) {
		LOG_ERR("Couldn't configure miso pin; (%d)", rc);
		return rc;
	}

	if (data->ctx.num_cs_gpios == 0) {
		LOG_ERR("CS pin is needed");
		return -EINVAL;
	}
	if (data->ctx.num_cs_gpios > 1) {
		LOG_WRN("More than 1 CS in slave context is not permitted. Configuring only first CS GPIO.");
	}
	if (!gpio_is_ready_dt(data->ctx.cs_gpios))
	{
		LOG_ERR("CS GPIO not ready");
		return -ENODEV;
	}
	rc = gpio_pin_configure_dt(data->ctx.cs_gpios, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Failed to configure CS pins: %d", rc);
		return rc;
	}

	spi_context_unlock_unconditionally(&data->ctx);
	
	return 0;
}

static DEVICE_API(spi, spis_bitbang_api) = {
	.transceive = spis_bitbang_transceive,
	.release = spis_bitbang_release,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spis_bitbang_transceive_async,
#endif /* CONFIG_SPI_ASYNC */
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
};

#define SPIS_DEFINE(inst)						\
	static struct spis_bitbang_config spis_bitbang_config_##inst = {	\
		.clk_gpio = GPIO_DT_SPEC_INST_GET(inst, clk_gpios),	\
		.mosi_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mosi_gpios, {0}),	\
		.miso_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, miso_gpios, {0}),	\
	};								\
									\
	static struct spis_bitbang_data spis_bitbang_data_##inst = {	\
		SPI_CONTEXT_INIT_LOCK(spis_bitbang_data_##inst, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spis_bitbang_data_##inst, ctx),	\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)	\
	};								\
									\
	SPI_DEVICE_DT_INST_DEFINE(inst,					\
			    spis_bitbang_init,				\
			    NULL,					\
			    &spis_bitbang_data_##inst,			\
			    &spis_bitbang_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SPI_INIT_PRIORITY,			\
			    &spis_bitbang_api);

DT_INST_FOREACH_STATUS_OKAY(SPIS_DEFINE)
