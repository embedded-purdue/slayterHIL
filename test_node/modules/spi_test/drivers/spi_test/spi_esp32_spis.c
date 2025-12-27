/*
 * Copyright (c) 2020-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_spis

// TODO: DELETE
#include <zephyr/sys/printk.h>

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <hal/spi_hal.h>
#include <hal/spi_slave_hal.h>
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

static int hello_world(const struct device *dev,
					   const struct spi_config *spi_cfg,
					   const struct spi_buf_set *tx_bufs,
					   const struct spi_buf_set *rx_bufs, bool asynchronous,
					   spi_callback_t cb,
					   void *userdata)
{
	printk("hello world!\n");
	return 0;
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
	.transceive = hello_world,
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

#define ESP32_SPIS_INIT(idx)                                                                                                                       \
                                                                                                                                                   \
	PINCTRL_DT_INST_DEFINE(idx);                                                                                                                   \
                                                                                                                                                   \
	static struct spis_esp32_data spis_data_##idx = {                                                                                              \
		SPI_CONTEXT_INIT_LOCK(spis_data_##idx, ctx),                                                                                               \
		SPI_CONTEXT_INIT_SYNC(spis_data_##idx, ctx),                                                                                               \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(idx), ctx)                                                                                     \
			.hal = {                                                                                                                               \
			.hw = (spi_dev_t *)DT_INST_REG_ADDR(idx),                                                                                              \
			.dma_in = NULL,                                                                                                                        \
			.dma_out = NULL, /* the following should be configured at initialization */                                                            \
			.dmadesc_rx = NULL,                                                                                                                    \
			.dmadesc_tx = NULL,                                                                                                                    \
			.dmadesc_n = 0,                                                                                                                        \
			.tx_dma_chan = 0,                                                                                                                      \
			.rx_dma_chan = 0,                                                                                                                      \
			/* the following to be filled after spi_slave_hal_init */ /* updated to peripheral registers whenspi_slave_hal_setup_device called  */ \
			.rx_lsbfirst = 0,                                                                                                                      \
			.tx_lsbfirst = 0,                                                                                                                      \
			.use_dma = 0, /* the following will be updated at each transaction*/                                                                   \
			.bitlen = 0,                                                                                                                           \
			.tx_buffer = NULL,                                                                                                                     \
			.rx_buffer = NULL,                                                                                                                     \
			.rcv_bitlen = 0,                                                                                                                       \
		},                                                                                                                                         \
		.hal_config = {.host_id = DT_INST_PROP(idx, dma_host), .dma_in = NULL, .dma_out = NULL}};                                                  \
                                                                                                                                                   \
	static const struct spis_esp32_config spis_config_##idx = {                                                                                    \
		.spi = (spi_dev_t *)DT_INST_REG_ADDR(idx),                                                                                                 \
                                                                                                                                                   \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                                                                                      \
		.duty_cycle = 0,                                                                                                                           \
		.input_delay_ns = 0,                                                                                                                       \
		.irq_source = DT_INST_IRQ_BY_IDX(idx, 0, irq),                                                                                             \
		.irq_priority = DT_INST_IRQ_BY_IDX(idx, 0, priority),                                                                                      \
		.irq_flags = DT_INST_IRQ_BY_IDX(idx, 0, flags),                                                                                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                                                                               \
		.clock_subsys =                                                                                                                            \
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(idx, offset),                                                                              \
		.use_iomux = DT_INST_PROP(idx, use_iomux),                                                                                                 \
		.dma_enabled = DT_INST_PROP(idx, dma_enabled),                                                                                             \
		.dma_host = DT_INST_PROP(idx, dma_host),                                                                                                   \
		SPI_DMA_CFG(idx),                                                                                                                          \
		.cs_setup = DT_INST_PROP_OR(idx, cs_setup_time, 0),                                                                                        \
		.cs_hold = DT_INST_PROP_OR(idx, cs_hold_time, 0),                                                                                          \
		.line_idle_low = DT_INST_PROP(idx, line_idle_low),                                                                                         \
		.clock_source = SPI_CLK_SRC_DEFAULT,                                                                                                       \
	};                                                                                                                                             \
                                                                                                                                                   \
	SPI_DEVICE_DT_INST_DEFINE(idx, spis_esp32_init,                                                                                                \
							  NULL, &spis_data_##idx,                                                                                              \
							  &spis_config_##idx, POST_KERNEL,                                                                                     \
							  CONFIG_SPI_INIT_PRIORITY, &spi_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_SPIS_INIT)
