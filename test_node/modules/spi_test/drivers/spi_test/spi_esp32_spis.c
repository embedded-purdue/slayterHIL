/*
 * Copyright (c) 2020-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_spis
#include <zephyr/logging/log_ctrl.h>

// TODO: DELETE
#include <zephyr/sys/printk.h>

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <hal/spi_slave_hal.h>
#include <hal/spi_hal.h>
#include <esp_attr.h>
#include <esp_clk_tree.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_spi, CONFIG_SPI_LOG_LEVEL);

#include <soc.h>
#include <esp_memory_utils.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#ifdef SOC_GDMA_SUPPORTED
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#endif
#include <zephyr/drivers/clock_control.h>
#include "test_spi_context.h"
#include "spi_esp32_spis.h"

#define SPI_DMA_MAX_BUFFER_SIZE 4092

#define SPI_DMA_RX 0
#define SPI_DMA_TX 1

static inline uint8_t spi_esp32_get_frame_size(const struct spi_config *spi_cfg)
{
	uint8_t dfs = SPI_WORD_SIZE_GET(spi_cfg->operation);

	dfs /= 8;
	if ((dfs == 0) || (dfs > 4))
	{
		LOG_WRN("Unsupported dfs, 1-byte size will be used");
		dfs = 1;
	}
	return dfs;
}

static int IRAM_ATTR spis_esp32_configure(const struct device *dev,
										  const struct spi_config *spi_cfg)
{
	const struct spis_esp32_config *cfg = dev->config;
	struct spis_esp32_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	spi_slave_hal_context_t *hal = &data->hal;
	// spi_hal_dev_config_t *hal_dev = &data->dev_config;
	int freq;

	if (spi_context_configured(ctx, spi_cfg))
	{
		return 0;
	}

	ctx->config = spi_cfg;

	if (spi_cfg->operation & SPI_HALF_DUPLEX)
	{
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP)
	{
		LOG_ERR("Loopback mode is not supported");
		return -ENOTSUP;
	}

	// hal_dev->cs_pin_id = ctx->config->slave;
	int ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret)
	{
		LOG_ERR("Failed to configure SPI pins");
		return ret;
	}

	// /* input parameters to calculate timing configuration */
	// spi_hal_timing_param_t timing_param = {
	// 	.half_duplex = hal_dev->half_duplex,
	// 	.no_compensate = hal_dev->no_compensate,
	// 	.expected_freq = spi_cfg->frequency,
	// 	.duty_cycle = cfg->duty_cycle == 0 ? 128 : cfg->duty_cycle,
	// 	.input_delay_ns = cfg->input_delay_ns,
	// 	.use_gpio = !cfg->use_iomux,
	// 	.clk_src_hz = data->clock_source_hz,
	// };

	// spi_hal_cal_clock_conf(&timing_param, &freq, &hal_dev->timing_conf);

	// data->trans_config.dummy_bits = hal_dev->timing_conf.timing_dummy;

	hal->tx_lsbfirst = spi_cfg->operation & SPI_TRANSFER_LSB ? 1 : 0;
	hal->rx_lsbfirst = spi_cfg->operation & SPI_TRANSFER_LSB ? 1 : 0;

	// data->trans_config.line_mode.data_lines = spi_esp32_get_line_mode(spi_cfg->operation);

	// /* multiline for command and address not supported */
	// data->trans_config.line_mode.addr_lines = 1;
	// data->trans_config.line_mode.cmd_lines = 1;

	/* SPI mode */
	hal->mode = 0;
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA)
	{
		hal->mode = BIT(0);
	}
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL)
	{
		hal->mode |= BIT(1);
	}

	// /* Chip select setup and hold times */
	// /* GPIO CS have their own delay parameter*/
	// if (!spi_cs_is_gpio(spi_cfg))
	// {
	// 	hal_dev->cs_hold = cfg->cs_hold;
	// 	hal_dev->cs_setup = cfg->cs_setup;
	// }

	spi_slave_hal_setup_device(hal);

	/* Workaround to handle default state of MISO and MOSI lines */
#ifndef CONFIG_SOC_SERIES_ESP32
	spi_dev_t *hw = hal->hw;

	if (cfg->line_idle_low)
	{
		hw->ctrl.d_pol = 0;
		hw->ctrl.q_pol = 0;
	}
	else
	{
		hw->ctrl.d_pol = 1;
		hw->ctrl.q_pol = 1;
	}
#endif

	/*
	 * Workaround for ESP32S3 and ESP32Cx SoC's. This dummy transaction is needed
	 * to sync CLK and software controlled CS when SPI is in mode 3
	 */
#if (defined(CONFIG_SOC_SERIES_ESP32S3) || defined(CONFIG_SOC_SERIES_ESP32C2) ||  \
	 defined(CONFIG_SOC_SERIES_ESP32C3) || defined(CONFIG_SOC_SERIES_ESP32C6)) && \
	!defined(DT_SPI_CTX_HAS_NO_CS_GPIOS)
	if ((ctx->num_cs_gpios != 0) && (hal->mode & (SPI_MODE_CPOL | SPI_MODE_CPHA)))
	{
		// TODO: ADDRESS
		return -ENOTSUP;
		// spi_esp32_transfer(dev);
	}
#endif

	return 0;
}

static int IRAM_ATTR spis_esp32_transfer(const struct device *dev)
{
	LOG_ERR("TRANSFER FUNC");
	struct spis_esp32_data *data = dev->data;
	const struct spis_esp32_config *cfg = dev->config;
	struct spi_context *ctx = &data->ctx;
	spi_slave_hal_context_t *hal = &data->hal;
	// spi_hal_dev_config_t *hal_dev = &data->dev_config;
	// transfer context needed for slave is alr in ctx or cfg
	// spi_hal_trans_config_t *hal_trans = &data->trans_config;

	// length of min(len(tx), len(rx)) * frame size = max transaction in bytes
	// limit is defensive in slave; in master, used for data sending reasons
	// size_t chunk_len_bytes = spi_context_max_continuous_chunk(&data->ctx) * data->dfs;
	// size_t max_buf_sz =
	// 	cfg->dma_enabled ? SPI_DMA_MAX_BUFFER_SIZE : SOC_SPI_MAXIMUM_BUFFER_SIZE;
	// size_t transfer_len_bytes = MIN(chunk_len_bytes, max_buf_sz);
	// size_t transfer_len_frames = transfer_len_bytes / data->dfs;
	// size_t bit_len = transfer_len_bytes << 3;
	// uint8_t *rx_temp = NULL;
	// uint8_t *tx_temp = NULL;
	// size_t dma_len_tx = MIN(ctx->tx_len * data->dfs, SPI_DMA_MAX_BUFFER_SIZE);
	// size_t dma_len_rx = MIN(ctx->rx_len * data->dfs, SPI_DMA_MAX_BUFFER_SIZE);
	// bool prepare_data = true;
	int err = 0;

	if (cfg->dma_enabled)
	{
		LOG_ERR("dma enabled existit");
		return -ENOTSUP;
	}

	/* clean up and prepare SPI hal */
	for (size_t i = 0; i < ARRAY_SIZE(hal->hw->data_buf); ++i)
	{
#if defined(CONFIG_SOC_SERIES_ESP32C6) || defined(CONFIG_SOC_SERIES_ESP32H2)
		hal->hw->data_buf[i].val = 0;
#else
		hal->hw->data_buf[i] = 0;
#endif
	}

	hal->tx_buffer = (uint8_t *)ctx->tx_buf;
	hal->rx_buffer = ctx->rx_buf;

	// /* keep cs line active until last transmission */
	// hal_trans->cs_keep_active =
	// 	(UTIL_OR(IS_ENABLED(DT_SPI_CTX_HAS_NO_CS_GPIOS), (ctx->num_cs_gpios == 0)) &&
	// 	 (ctx->rx_count > 1 || ctx->tx_count > 1 || ctx->rx_len > transfer_len_frames ||
	// 	  ctx->tx_len > transfer_len_frames));

	/* configure SPI */
	// spi_hal_setup_trans(hal, hal_dev, hal_trans);

	// #if defined(SOC_GDMA_SUPPORTED)
	// 	LOG_ERR("gdma supported exit");
	// 	return -ENOTSUP;
	// 	// if (cfg->dma_enabled && hal_trans->rcv_buffer && hal_trans->send_buffer)
	// 	// {
	// 	// 	/* setup DMA channels via DMA driver */
	// 	// 	spi_ll_dma_rx_fifo_reset(hal->hw);
	// 	// 	spi_ll_infifo_full_clr(hal->hw);
	// 	// 	spi_ll_dma_rx_enable(hal->hw, 1);

	// 	// 	err = spi_esp32_gdma_start(dev, SPI_DMA_RX, hal_trans->rcv_buffer,
	// 	// 							   transfer_len_bytes);
	// 	// 	if (err)
	// 	// 	{
	// 	// 		goto free;
	// 	// 	}

	// 	// 	spi_ll_dma_tx_fifo_reset(hal->hw);
	// 	// 	spi_ll_outfifo_empty_clr(hal->hw);
	// 	// 	spi_ll_dma_tx_enable(hal->hw, 1);

	// 	// 	err = spi_esp32_gdma_start(dev, SPI_DMA_TX, hal_trans->send_buffer,
	// 	// 							   transfer_len_bytes);
	// 	// 	if (err)
	// 	// 	{
	// 	// 		goto free;
	// 	// 	}
	// 	// }

	// 	// prepare_data = !cfg->dma_enabled;
	// #endif

	// if (prepare_data)
	// {
	// 	/* only for plain transfers or DMA transfers w/o GDMA */
	// 	spi_hal_prepare_data(hal, hal_dev, hal_trans);
	// }
	LOG_ERR("PREPARING DATA");
	spi_slave_hal_prepare_data(hal);

	/* send data */
	LOG_ERR("STARTING");
	spi_slave_hal_user_start(hal);

	// bring this back alter when starting to do chunks
	// spi_context_update_tx(&data->ctx, data->dfs, transfer_len_frames);

	log_panic();

	while (!spi_slave_hal_usr_is_done(hal))
	{
		printk("blocking!");
		LOG_ERR("BLOCKING");
		log_panic();
	}
	LOG_ERR("AFTER BLOCK");

	// if (!cfg->dma_enabled)
	// {
	// 	/* read data */
	// 	spi_hal_fetch_result(hal);
	// }

	// if (rx_temp)
	// {
	// 	memcpy(&ctx->rx_buf[0], rx_temp, transfer_len_bytes);
	// }

	// spi_context_update_rx(&data->ctx, data->dfs, transfer_len_frames);

	// free:
	// 	k_free(tx_temp);
	// 	k_free(rx_temp);

	return err;
}

static int spis_transceive(const struct device *dev,
						   const struct spi_config *spi_cfg,
						   const struct spi_buf_set *tx_bufs,
						   const struct spi_buf_set *rx_bufs, bool asynchronous,
						   spi_callback_t cb,
						   void *userdata)
{
	LOG_ERR("TRANSCEIVE FUNC");

	const struct spis_esp32_config *cfg = dev->config;
	struct spis_esp32_data *data = dev->data;
	int ret = 0;

#ifndef CONFIG_SPI_ESP32_INTERRUPT
	if (asynchronous)
	{
		LOG_ERR("ASYNC");
		return -ENOTSUP;
	}
#endif

	if ((spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE)
	{
		return -ENOTSUP;
	}

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	data->dfs = spi_esp32_get_frame_size(spi_cfg);

	// sets up tx and rx in ctx
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->dfs);

	if (data->ctx.tx_buf == NULL && data->ctx.rx_buf == NULL)
	{
		LOG_ERR("TX/RX NULL");
		goto done;
	}

	ret = spis_esp32_configure(dev, spi_cfg);
	if (ret)
	{
		LOG_ERR("RET TRIGGERED");
		goto done;
	}

	// TODO: SEE SLAVE RELPLACEMENT; EHRE I PROBABLY BLOCK
	// spi_context_cs_control(&data->ctx, true);

	// #ifdef CONFIG_SPI_ESP32_INTERRUPT
	// 	return -ENOTSUP;
	// 	// spi_ll_enable_int(cfg->spi);
	// 	// spi_ll_set_int_stat(cfg->spi);
	// #else

	LOG_ERR("JUST BEFORE TRANSFER FUNC");
	spis_esp32_transfer(dev);

	// do
	// {
	// 	spis_esp32_transfer(dev);
	// } while (spi_esp32_transfer_ongoing(data));

	// spi_esp32_complete(dev, data, cfg->spi, 0);
	spi_slave_hal_store_result(&data->hal);

	// #endif /* CONFIG_SPI_ESP32_INTERRUPT */

	printk("hello world!\n");

done:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spis_esp32_transceive(const struct device *dev,
								 const struct spi_config *spi_cfg,
								 const struct spi_buf_set *tx_bufs,
								 const struct spi_buf_set *rx_bufs)
{
	return spis_transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

static int spis_esp32_init(const struct device *dev)
{
	int err;
	const struct spis_esp32_config *cfg = dev->config;
	struct spis_esp32_data *data = dev->data;
	spi_slave_hal_context_t *hal = &data->hal;

	if (!cfg->clock_dev)
	{
		return -EINVAL;
	}

	if (!device_is_ready(cfg->clock_dev))
	{
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Enables SPI peripheral */
	err = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (err < 0)
	{
		LOG_ERR("Error enabling SPI clock");
		return err;
	}

	if (cfg->dma_enabled)
	{
		LOG_ERR("DMA on slave not supported");
		return -ENOTSUP;
	}

	spi_ll_slave_init(hal->hw);

#ifdef CONFIG_SPI_ESP32_INTERRUPT
	LOG_ERR("INTERRUPT on slave not supported");
	return -ENOTSUP;
#endif

	// // TODO: figure out how to make bindings file only expect 1 gpio
	// err = configure_slave_cs(&data->ctx);
	// if (err < 0)
	// {
	// 	return err;
	// }

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_api) = {
	.transceive = spis_esp32_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = NULL,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = NULL,
#endif
	.release = NULL};

#ifdef CONFIG_SOC_SERIES_ESP32
#define GET_AS_CS(idx) .as_cs = DT_INST_PROP(idx, clk_as_cs),
#else
#define GET_AS_CS(idx)
#endif

#if defined(SOC_GDMA_SUPPORTED)
#define SPI_DMA_CFG(idx)                                   \
	.dma_dev = ESP32_DT_INST_DMA_CTLR(idx, tx),            \
	.dma_tx_ch = ESP32_DT_INST_DMA_CELL(idx, tx, channel), \
	.dma_rx_ch = ESP32_DT_INST_DMA_CELL(idx, rx, channel)
#else
#define SPI_DMA_CFG(idx) \
	.dma_clk_src = DT_INST_PROP(idx, dma_clk)
#endif /* defined(SOC_GDMA_SUPPORTED) */

#define ESP32_SPIS_INIT(idx)                                                                                                                                         \
                                                                                                                                                                     \
	PINCTRL_DT_INST_DEFINE(idx);                                                                                                                                     \
                                                                                                                                                                     \
	static struct spis_esp32_data spis_data_##idx = {                                                                                                                \
		SPI_CONTEXT_INIT_LOCK(spis_data_##idx, ctx),                                                                                                                 \
		SPI_CONTEXT_INIT_SYNC(spis_data_##idx, ctx),                                                                                                                 \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(idx), ctx)                                                                                                       \
			.hal = {                                                                                                                                                 \
			.hw = (spi_dev_t *)DT_INST_REG_ADDR(idx),                                                                                                                \
			.dma_in = NULL,                                                                                                                                          \
			.dma_out = NULL, /* the following should be configured at initialization */                                                                              \
			.dmadesc_rx = NULL,                                                                                                                                      \
			.dmadesc_tx = NULL,                                                                                                                                      \
			.dmadesc_n = 0,                                                                                                                                          \
			.tx_dma_chan = 0,                                                                                                                                        \
			.rx_dma_chan = 0,                                                                                                                                        \
			/* the following to be filled after spi_slave_hal_init */ /* updated to peripheral registers whenspi_slave_hal_setup_device called  */                   \
			.use_dma = 0,											  /* the following will be updated at each transaction*/                                         \
			.bitlen = 0,                                                                                                                                             \
			.tx_buffer = NULL,                                                                                                                                       \
			.rx_buffer = NULL,                                                                                                                                       \
			.rcv_bitlen = 0,                                                                                                                                         \
		},                                                                                                                                                           \
		.hal_config = {.host_id = DT_INST_PROP(idx, dma_host) + 1, .dma_in = NULL, .dma_out = NULL}}; /* DMA_HOST + 1 was a pattern reflected in the master driver*/ \
                                                                                                                                                                     \
	static const struct spis_esp32_config spis_config_##idx = {                                                                                                      \
		.spi = (spi_dev_t *)DT_INST_REG_ADDR(idx),                                                                                                                   \
                                                                                                                                                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                                                                                                        \
		.duty_cycle = 0,                                                                                                                                             \
		.input_delay_ns = 0,                                                                                                                                         \
		.irq_source = DT_INST_IRQ_BY_IDX(idx, 0, irq),                                                                                                               \
		.irq_priority = DT_INST_IRQ_BY_IDX(idx, 0, priority),                                                                                                        \
		.irq_flags = DT_INST_IRQ_BY_IDX(idx, 0, flags),                                                                                                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                                                                                                 \
		.clock_subsys =                                                                                                                                              \
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(idx, offset),                                                                                                \
		.use_iomux = DT_INST_PROP(idx, use_iomux),                                                                                                                   \
		.dma_enabled = DT_INST_PROP(idx, dma_enabled),                                                                                                               \
		.dma_host = DT_INST_PROP(idx, dma_host),                                                                                                                     \
		SPI_DMA_CFG(idx),                                                                                                                                            \
		.cs_setup = DT_INST_PROP_OR(idx, cs_setup_time, 0),                                                                                                          \
		.cs_hold = DT_INST_PROP_OR(idx, cs_hold_time, 0),                                                                                                            \
		.line_idle_low = DT_INST_PROP(idx, line_idle_low),                                                                                                           \
		.clock_source = SPI_CLK_SRC_DEFAULT,                                                                                                                         \
	};                                                                                                                                                               \
                                                                                                                                                                     \
	SPI_DEVICE_DT_INST_DEFINE(idx, spis_esp32_init,                                                                                                                  \
							  NULL, &spis_data_##idx,                                                                                                                \
							  &spis_config_##idx, POST_KERNEL,                                                                                                       \
							  CONFIG_SPI_INIT_PRIORITY, &spi_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_SPIS_INIT)
