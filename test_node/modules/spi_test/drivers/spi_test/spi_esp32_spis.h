/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_SPI_ESP32_SPIS_H_
#define DRIVERS_SPI_ESP32_SPIS_H_

#include <zephyr/drivers/pinctrl.h>
#include <hal/spi_hal.h>
#include <hal/spi_slave_hal.h>
#ifdef SOC_GDMA_SUPPORTED
#include <hal/gdma_hal.h>
#endif

struct spis_esp32_config
{
    spi_dev_t *spi;
    const struct device *clock_dev;
    int duty_cycle;
    int input_delay_ns;
    int irq_source;
    int irq_priority;
    int irq_flags;
    const struct pinctrl_dev_config *pcfg;
    clock_control_subsys_t clock_subsys;
    bool use_iomux;
    bool dma_enabled;
    int dma_host;
#if defined(SOC_GDMA_SUPPORTED)
    const struct device *dma_dev;
    uint8_t dma_tx_ch;
    uint8_t dma_rx_ch;
#else
    int dma_clk_src;
#endif
    int cs_setup;
    int cs_hold;
    bool line_idle_low;
    spi_clock_source_t clock_source;
};

struct spis_esp32_data
{
    struct spi_context ctx;
    spi_slave_hal_context_t hal;       // changed
    spi_slave_hal_config_t hal_config; // changed
#ifdef SOC_GDMA_SUPPORTED
    gdma_hal_context_t hal_gdma;
#endif
    // spi_hal_timing_conf_t timing_config;
    // spi_hal_dev_config_t dev_config;        already in slave config
    // spi_hal_trans_config_t trans_config;     master transactions
    uint8_t dfs;
    lldesc_t dma_desc_tx;
    lldesc_t dma_desc_rx;
    // uint32_t clock_source_hz;
};

#endif /* DRIVERS_SPI_ESP32_SPIS_H_ */
