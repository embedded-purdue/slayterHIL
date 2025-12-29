/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private API for SPI drivers
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPIS_CONTEXT_H_
#define ZEPHYR_DRIVERS_SPI_SPIS_CONTEXT_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device_runtime.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef DT_SPI_CTX_HAS_NO_CS_GPIOS
#define spis_context_cs_configure(...) 0
#else /* DT_SPI_CTX_HAS_NO_CS_GPIOS */
/*
 * This function initializes all the chip select GPIOs associated with a spi controller.
 * The context first must be initialized using the SPI_CONTEXT_CS_GPIOS_INITIALIZE macro.
 * This function should be called during the device init sequence so that
 * all the CS lines are configured properly before the first transfer begins.
 * Note: If a controller has native CS control in SPI hardware, they should also be initialized
 * during device init by the driver with hardware-specific code.
 */
static inline int spis_context_cs_configure(struct spi_context *ctx)
{
	int ret;
	const struct gpio_dt_spec *cs_gpio;

	for (cs_gpio = ctx->cs_gpios; cs_gpio < &ctx->cs_gpios[ctx->num_cs_gpios]; cs_gpio++)
	{
		if (!device_is_ready(cs_gpio->port))
		{
			LOG_ERR("CS GPIO port %s pin %d is not ready",
					cs_gpio->port->name, cs_gpio->pin);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(cs_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0)
		{
			return ret;
		}
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SPI_SPIS_CONTEXT_H_ */
