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
LOG_MODULE_REGISTER(esp32_spis, CONFIG_SPI_LOG_LEVEL);

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

/* -------------------- SANITY CHECK DEFINES ---------------------------*/
/* sanity check for cpu-driven mode 0 MSB slave single transfer transaction */

/* everythign should be a 0 */
/* but keep an eye on conf_bitlen and update, which seem like they're for master mode, but might be needed */
#define CMD_PRESTART_EXPECTED(hw) (hw->cmd.val == 0)

/* addr empty given that there's no addr phase in single cpu-driven transfer */
#define ADDR_EXPECTED(hw) (hw->addr == 0)

/* q_pol and d_pol */
#define CTRL_ALLOWED_MASK     \
	((1u << 18) | /* q_pol */ \
	 (1u << 19))  /* d_pol */
/* q_pol and d_pol are expected to be high */
#define CTRL_EXPECTED(hw) (                       \
	((hw->ctrl.val & ~CTRL_ALLOWED_MASK) == 0) && \
	(hw->ctrl.d_pol == 1) && (hw->ctrl.q_pol == 1))

/* clock reg should be 0 bcz these are all master sclk timing tools */
#define CLOCK_EXPECTED(hw) ( \
	hw->clock.val == 0)

/* doutdin = 1 for full duplex communication
 * tsck_i_edge = 0 for mode 0
 * rsck_i_edge = 0 for mode 0
 * usr_conf_nxt = 0 to indicate a non-segmented transfer
 * sio = 0 to disbale 3-line half duplex
 * usr_dummy_idle = 0
 * usr_mosi and usr_miso = 1 to enable full duplex transfer
 * usr_dummy = 0
 * usr_addr = 0
 * usr_command = 0
 */
#define USER_ALLOWED_MASK        \
	((1u << 0) |  /* doutdin */  \
	 (1u << 27) | /* usr_mosi */ \
	 (1u << 28))  /* usr_miso */
#define USER_EXPECTED(hw) (                       \
	((hw->user.val & ~USER_ALLOWED_MASK) == 0) && \
	(hw->user.doutdin == 1) && (hw->user.usr_mosi == 1) && (hw->user.usr_miso == 1))

/* user1 and 2 seems to be mostly master settings, or non-full-duplex-single-transfer slave settings */
#define USER1_EXPECTED(hw) ( \
	(hw->user1.val == 0))

#define USER2_EXPECTED(hw) ( \
	(hw->user2.val == 0))

/* ms_data_bitlen is used in slave dma transfers, but not cpu-controlled transfer */
#define MS_DLEN_EXPECTED(hw) ( \
	(hw->ms_dlen.val == 0))

/* cs0_dis = 0: cs0 we need and cannot disable it
 * cs1-5 =1? are unused on slave. However, it's unspecified whether they need to be explicitly disabled.
 * ck_dis = 1? This is relevant to master but doens't say whether should be explicitly disabled
 * clk_data_dtr_en, data_dtr_en, addr_dtr_en, cmd_dtr_en = 0: we want single data trasnfer mode
 * slave_cs_pol = 0
 */
#define MISC_ALLOWED_MASK      \
	((1u << 1) | /* cs1_dis */ \
	 (1u << 2) | /* cs2_dis */ \
	 (1u << 3) | /* cs3_dis */ \
	 (1u << 4) | /* cs4_dis */ \
	 (1u << 5) | /* cs5_dis */ \
	 (1u << 6))	 /* ck_dis */
#define MISC_EXPECTED(hw) ( \
	(hw->misc.val & ~MISC_ALLOWED_MASK) == 0)

/* we want 0 input delay */
#define DIN_DOUT_EXPECTED(hw) (                          \
	(hw->din_mode.val == 0) && (hw->din_num.val == 0) && \
	(hw->dout_mode.val == 0))

/* non DMA transfer */
#define DMA_CONF_EXPECTED(hw) ( \
	hw->dma_conf.val == 0)

/* we are going no interrupts for now, and just polling raw */
#define DMA_INTS_EXPECTED(hw) (                                 \
	(hw->dma_int_ena.val == 0) && (hw->dma_int_clr.val == 0) && \
	(hw->dma_int_raw.val == 0) && (hw->dma_int_st.val == 0))

/* clk_mode: 0 for now bcz we are operating in mode 0
 * clk_mode_13: 0
 * rsck_data_out: 0
 * rdbuf_bitlen_en: 0; this stores master-read-slave data len
 * wrbuf_bitlen_en: 1; this stores master-write-slave data len
 * slave_mode: 1
 * usr_conf: 0
 */

#define SLAVE_ALLOWED_MASK              \
	((1u << 11) | /* wrbuf_bitlen_en */ \
	 (1u << 26))  /* slave_mode */
#define SLAVE_EXPECTED(hw) (                        \
	((hw->slave.val & ~SLAVE_ALLOWED_MASK) == 0) && \
	(hw->slave.slave_mode == 1) && (hw->slave.wrbuf_bitlen_en == 1))

/* clock should be enabled for periph (likely) */
#define CLK_GATE_EXPECTED(hw) (           \
	(hw->clk_gate.clk_en == 1) &&         \
	(hw->clk_gate.mst_clk_active == 1) && \
	(hw->clk_gate.mst_clk_sel == 1))

static inline void log_sanity_check(spi_dev_t *hw)
{
	LOG_ERR("Beginning log sanity check ------");
	LOG_ERR("Expected configuration: Mode 0, CPU-driven Slave Single Transfer");

	if (!CMD_PRESTART_EXPECTED(hw))
		LOG_ERR("CMD reg prestart fail");

	if (!ADDR_EXPECTED(hw))
		LOG_ERR("ADDR reg fail");

	if (!CTRL_EXPECTED(hw))
		LOG_ERR("CTRL reg fail");

	if (!CLOCK_EXPECTED(hw))
		LOG_ERR("CLOCK reg fail");

	if (!USER_EXPECTED(hw))
		LOG_ERR("USER reg fail");

	if (!USER1_EXPECTED(hw))
	{
		LOG_ERR("USER1 reg fail");
		LOG_ERR("dummy_cyclelen: %u | mst_wfull: %u | cs_setup: %u | cs_hold: %u | usr_bitlen: %u",
				hw->user1.usr_dummy_cyclelen, hw->user1.mst_wfull_err_end_en, hw->user1.cs_setup_time, hw->user1.cs_hold_time, hw->user1.usr_addr_bitlen);
	}

	if (!USER2_EXPECTED(hw))
	{
		LOG_ERR("USER2 reg fail");
		LOG_ERR("usr_cmd: %u | mst_rempty: %u | usr_cmd: %u",
				hw->user2.usr_command_value, hw->user2.mst_rempty_err_end_en, hw->user2.usr_command_bitlen);
	}

	if (!MS_DLEN_EXPECTED(hw))
		LOG_ERR("MS_DLEN reg fail");

	if (!MISC_EXPECTED(hw))
		LOG_ERR("MISC reg fail");

	if (!DIN_DOUT_EXPECTED(hw))
		LOG_ERR("DIN/DOUT regs fail");

	if (!DMA_CONF_EXPECTED(hw))
	{
		LOG_ERR("DMA_CONF reg fail; val is %u", hw->dma_conf.val);
		LOG_ERR("outfifo_empty: %u | infifo_full: %u", hw->dma_conf.outfifo_empty, hw->dma_conf.infifo_full);
	}

	if (!DMA_INTS_EXPECTED(hw))
		LOG_ERR("DMA_INT regs fail");

	if (!SLAVE_EXPECTED(hw))
	{
		LOG_ERR("SLAVE reg fail; val is %u", hw->slave.val);
		LOG_ERR("clk_mode: %u | clk_mode_13: %u | rsck_data_out: %u | "
				"rdma_bitlen_en: %u | wrdma_bitlen_en: %u | rdbuf_bitlen_en: %u | "
				"wrbuf_bitlen_en: %u | dma_seg_magic: %u | slave_mode: %u | "
				"soft_reset: %u | usr_conf: %u",
				hw->slave.clk_mode, hw->slave.clk_mode_13, hw->slave.rsck_data_out,
				hw->slave.rddma_bitlen_en, hw->slave.wrdma_bitlen_en, hw->slave.rdbuf_bitlen_en,
				hw->slave.wrbuf_bitlen_en, hw->slave.dma_seg_magic_value, hw->slave.slave_mode,
				hw->slave.soft_reset, hw->slave.usr_conf);
	}

	if (!CLK_GATE_EXPECTED(hw))
		LOG_ERR("CLK_GATE reg fail");

	LOG_ERR("Ending log sanity check ---------");
}

/* -------------------- SANITY CHECK DEFINES ---------------------------*/

static void spis_ll_clk_ena(spi_dev_t *hw)
{
	/* enables internal clock gate */
	/* may or may not be needed for slave ? Unspecified. not in slave_ll_init */
	hw->clk_gate.clk_en = 1;
	hw->clk_gate.mst_clk_active = 1;
	hw->clk_gate.mst_clk_sel = 1;
}

static void spis_hal_setup_trans(spi_slave_hal_context_t *hal)
{
	/* reflects spi_hal_setup_trans and internal fucntiosn */
	/* refactor to (maybe) include that */
	hal->hw->user1.val = 0;
	hal->hw->user2.val = 0;
	hal->hw->ctrl.val &= ~SPI_LL_ONE_LINE_CTRL_MASK;
	hal->hw->user.val &= ~SPI_LL_ONE_LINE_USER_MASK;
	hal->hw->dma_conf.val = 0;
	hal->hw->slave.dma_seg_magic_value = 0;
	spi_ll_cpu_tx_fifo_reset(hal->hw);
	spi_ll_cpu_rx_fifo_reset(hal->hw);
	spi_ll_enable_mosi(hal->hw, (hal->rx_buffer == NULL) ? 0 : 1);
	spi_ll_enable_miso(hal->hw, (hal->tx_buffer == NULL) ? 0 : 1);
	hal->hw->slave.wrbuf_bitlen_en = 1; // enable tracking of read from master
}

/* -------------------- DUPLICATE CODE WITH MASTER ---------------------------*/

static inline uint8_t
spi_esp32_get_frame_size(const struct spi_config *spi_cfg)
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

/* -------------------- MAIN DRIVER ---------------------------*/

static int IRAM_ATTR spis_esp32_configure(const struct device *dev,
										  const struct spi_config *spi_cfg)
{
	const struct spis_esp32_config *cfg = dev->config;
	struct spis_esp32_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	spi_slave_hal_context_t *hal = &data->hal;
	int freq;

	if (spi_context_configured(ctx, spi_cfg))
	{
		return 0;
	}

	ctx->config = spi_cfg;

	if ((spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE)
	{
		LOG_ERR("Line mode not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_OP_MODE_MASTER)
	{
		LOG_ERR("Master mode not supported");
		return -ENOTSUP;
	}

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

	int ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret)
	{
		LOG_ERR("Failed to configure SPI pins");
		return ret;
	}

	hal->tx_lsbfirst = spi_cfg->operation & SPI_TRANSFER_LSB ? 1 : 0;
	hal->rx_lsbfirst = spi_cfg->operation & SPI_TRANSFER_LSB ? 1 : 0;

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

	if (hal->mode != 0)
	{
		LOG_ERR("Expecting mode 0 for the purposes of initial driver setup");
		return -ENOTSUP;
	}

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
		return -ENOTSUP;
	}
#endif

	return 0;
}

static int IRAM_ATTR spis_esp32_transfer(const struct device *dev)
{
	struct spis_esp32_data *data = dev->data;
	const struct spis_esp32_config *cfg = dev->config;
	struct spi_context *ctx = &data->ctx;
	spi_slave_hal_context_t *hal = &data->hal;

	int err = 0;

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
	hal->bitlen = 96; // configure this; max bitlen should be rd and wr buf

	// clears afifos for cpus
	// doesnt set rx and tx bitlen for slave (internal funcs are empty), but I don't see it in s3 regs anyways
	// configures MOSI and MISO if CONFIG_IDF_TARGET_ESP32
	spis_hal_setup_trans(hal);
	spi_slave_hal_prepare_data(hal);

	log_sanity_check(hal->hw);

	spi_slave_hal_user_start(hal);

	while (!spi_slave_hal_usr_is_done(hal))
	{
		/* nop */
	}
	LOG_ERR("AFTER BLOCK received %u", spi_ll_slave_get_rcv_bitlen(hal->hw));
	hal->hw->slave1.data_bitlen = 96;

	return err;
}

static int spis_transceive(const struct device *dev,
						   const struct spi_config *spi_cfg,
						   const struct spi_buf_set *tx_bufs,
						   const struct spi_buf_set *rx_bufs, bool asynchronous,
						   spi_callback_t cb,
						   void *userdata)
{
	const struct spis_esp32_config *cfg = dev->config;
	struct spis_esp32_data *data = dev->data;
	int ret = 0;

#ifndef CONFIG_SPI_ESP32_INTERRUPT
	if (asynchronous)
	{
		return -ENOTSUP;
	}
#endif

	spi_context_lock(&data->ctx, asynchronous, cb, userdata, spi_cfg);

	data->dfs = spi_esp32_get_frame_size(spi_cfg);

	// sets up tx and rx in ctx
	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->dfs);

	if (data->ctx.tx_buf == NULL && data->ctx.rx_buf == NULL)
	{
		goto done;
	}

	ret = spis_esp32_configure(dev, spi_cfg);
	if (ret)
	{
		goto done;
	}

#ifdef CONFIG_SPI_ESP32_INTERRUPT
	return -ENOTSUP;
#else

	spis_esp32_transfer(dev);

#endif /* CONFIG_SPI_ESP32_INTERRUPT */

	spi_slave_hal_store_result(&data->hal);
	LOG_ERR("bitlen is %u", data->hal.rcv_bitlen);

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

	spis_ll_clk_ena(hal->hw);
	spi_ll_slave_init(hal->hw);

	/* Figure out how to set this via DT ...? Given that we need to know at init */
	if (cfg->dma_enabled)
	{
		LOG_ERR("DMA on slave not supported");
		return -ENOTSUP;
	}

#ifdef CONFIG_SPI_ESP32_INTERRUPT
	LOG_ERR("Interrupts on slave not supported");
	return -ENOTSUP;
#endif

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
			.hw = (spi_dev_t *)DT_INST_REG_ADDR(idx), /* the following should be configured at initialization */                                                     \
			.dmadesc_rx = NULL,                                                                                                                                      \
			.dmadesc_tx = NULL,                                                                                                                                      \
			.dmadesc_n = 0,                                                                                                                                          \
			.tx_dma_chan = 0,                                                                                                                                        \
			.rx_dma_chan = 0,                                                                                                                                        \
			.use_dma = 0 /* configure before spi_slave_hal_init, or figure out how to reset dma setup */                                                             \
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
