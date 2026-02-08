/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_i2c_bitbang

/**
 * @file
 * @brief Driver for software driven I2C using GPIO lines
 *
 * This driver implements an I2C interface by driving two GPIO lines under
 * software control.
 *
 * The GPIO pins used must be configured (through devicetree and pinmux) with
 * suitable flags, i.e. the SDA pin as open-collector/open-drain with a pull-up
 * resistor (possibly as an external component attached to the pin).
 *
 * When the SDA pin is read it must return the state of the physical hardware
 * line, not just the last state written to it for output.
 *
 * The SCL pin should be configured in the same manner as SDA, or, if it is
 * known that the hardware attached to pin doesn't attempt clock stretching,
 * then the SCL pin may be a push/pull output.
 */

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_bitbang_gpio);

#include "i2c-priv.h"
#include "i2c_bitbang.h"

/* Driver config */
struct i2c_gpio_config {
	struct gpio_dt_spec scl_gpio;
	struct gpio_dt_spec sda_gpio;
	uint32_t bitrate;
};

/* Driver instance data */
struct i2c_gpio_context {
	struct i2c_bitbang bitbang;	/* Bit-bang library data */
	enum i2c_config_state config_state;
	struct k_mutex mutex;
};

static void i2c_gpio_set_scl(void *io_context, int state)
{
	const struct i2c_gpio_config *config = io_context;

	gpio_pin_set_dt(&config->scl_gpio, state);
}

static int i2c_gpio_get_scl(void *io_context)
{
	const struct i2c_gpio_config *config = io_context;
	int rc = gpio_pin_get_dt(&config->scl_gpio);

	return rc != 0;
}

/*static void i2c_gpio_set_sda(void *io_context, int state)
{
	const struct i2c_gpio_config *config = io_context;

	gpio_pin_set_dt(&config->sda_gpio, state);
}*/
static void i2c_gpio_set_sda(void *io_context, int state)
{
    const struct i2c_gpio_config *config = io_context;
    int before, after, err;
    
    /* Read current state */
    before = gpio_pin_get_dt(&config->sda_gpio);
    
    if (state) {
        /* Release SDA (Hi-Z) - configure as INPUT to let pull-up take over */
        err = gpio_pin_configure_dt(&config->sda_gpio, GPIO_INPUT);
        LOG_DBG("set_sda(1): INPUT mode, err=%d", err);
    } else {
        /* Pull SDA LOW - configure as OUTPUT and drive LOW */
        err = gpio_pin_configure_dt(&config->sda_gpio, GPIO_OUTPUT_LOW);
        LOG_DBG("set_sda(0): OUTPUT_LOW mode, err=%d", err);
    }
    
    /* Small delay to let pin settle */
    k_busy_wait(1);
    
    /* Read back to verify */
    after = gpio_pin_get_dt(&config->sda_gpio);
    
    LOG_DBG("set_sda(%d): before=%d, after=%d, err=%d", state, before, after, err);
}

static int i2c_gpio_get_sda(void *io_context)
{
	const struct i2c_gpio_config *config = io_context;
	int rc = gpio_pin_get_dt(&config->sda_gpio);

	/* Default high as that would be a NACK */
	return rc != 0;
}

static const struct i2c_bitbang_io io_fns = {
	.set_scl = &i2c_gpio_set_scl,
	.get_scl = &i2c_gpio_get_scl,
	.set_sda = &i2c_gpio_set_sda,
	.get_sda = &i2c_gpio_get_sda,
};

static int i2c_gpio_configure(const struct device *dev, uint32_t dev_config)
{
	struct i2c_gpio_context *context = dev->data;
	int rc;

	k_mutex_lock(&context->mutex, K_FOREVER);

	rc = i2c_bitbang_configure(&context->bitbang, dev_config);

	k_mutex_unlock(&context->mutex);

	return rc;
}

static int i2c_gpio_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_gpio_context *context = dev->data;
	int rc;

	k_mutex_lock(&context->mutex, K_FOREVER);

	rc = i2c_bitbang_get_config(&context->bitbang, config);
	if (rc < 0) {
		LOG_ERR("I2C controller not configured: %d", rc);
	}

	k_mutex_unlock(&context->mutex);

	return rc;
}

static int i2c_gpio_transfer(const struct device *dev, struct i2c_msg *msgs,
				uint8_t num_msgs, uint16_t slave_address)
{
	struct i2c_gpio_context *context = dev->data;

	/* ensure that this gpio is in the right mode before transfer*/
	if(context->config_state == I2C_GPIO_TARGET) {
		LOG_ERR("I2C controller configured as target");
		return -EBUSY;
	} else if (context->config_state == I2C_GPIO_UNCONFIGURED) {
		context->config_state = I2C_GPIO_CONTROLLER;
	}


	int rc;

	k_mutex_lock(&context->mutex, K_FOREVER);

	rc = i2c_bitbang_transfer(&context->bitbang, msgs, num_msgs,
				    slave_address);

	k_mutex_unlock(&context->mutex);

	return rc;
}

static int i2c_gpio_target_register(const struct device *dev,
				    struct i2c_target_config *cfg)
{
	const struct i2c_gpio_config *config = dev->config;
	struct i2c_gpio_context *context = dev->data;

	/* ensure that this gpio is in the right mode before registering*/
	if(context->config_state == I2C_GPIO_CONTROLLER) {
		LOG_ERR("I2C controller configured as controller");
		return -EBUSY;
	} else if (context->config_state == I2C_GPIO_UNCONFIGURED) {
		context->config_state = I2C_GPIO_TARGET;
	} else if (context->bitbang.target_cfg != NULL) {
		LOG_ERR("I2C controller already has a target registered");
		return -EBUSY;
	}

	context->bitbang.target_cfg = cfg;
	context->bitbang.target_state = I2C_TARGET_IDLE;
	LOG_DBG("I2C target registered at address 0x%02x", cfg->address);

	/* enable workqueue*/
	k_work_init(&context->bitbang.target_work, i2c_bitbang_target_work_handler);

	/* enable interrupts on gpio pins */
	gpio_init_callback(&context->bitbang.sda_cb, i2c_bitbang_gpio_isr, BIT(config->sda_gpio.pin));

	int err = gpio_add_callback(config->sda_gpio.port, &context->bitbang.sda_cb);

	if (err) {
		LOG_ERR("Failed to add GPIO callback (err %d)", err);
		context->bitbang.target_cfg = NULL;
		context->config_state = I2C_GPIO_UNCONFIGURED;
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->sda_gpio, GPIO_INT_EDGE_BOTH);
	if (err) {
		LOG_ERR("Failed to configure GPIO interrupt (err %d)", err);
		gpio_remove_callback(config->sda_gpio.port, &context->bitbang.sda_cb);
		context->bitbang.target_cfg = NULL;
		context->config_state = I2C_GPIO_UNCONFIGURED;
		return err;
	}

	return 0;
}

static int i2c_gpio_target_unregister(const struct device *dev,
				      struct i2c_target_config *cfg)
{
	const struct i2c_gpio_config *config = dev->config;
	struct i2c_gpio_context *context = dev->data;

	if(context->config_state != I2C_GPIO_TARGET) {
		LOG_ERR("I2C controller not configured as target");
		return -EINVAL;
	} else if (context->bitbang.target_cfg != cfg) {
		LOG_ERR("I2C target config mismatch");
		return -EINVAL;
	}

	context->config_state = I2C_GPIO_UNCONFIGURED;
	context->bitbang.target_cfg = NULL;

	/* clear interrupts on gpio pins */
	gpio_pin_interrupt_configure_dt(&config->sda_gpio, GPIO_INT_DISABLE);

	gpio_remove_callback(config->sda_gpio.port, &context->bitbang.sda_cb);

	k_work_cancel(&context->bitbang.target_work);

	return 0;
}

static int i2c_gpio_recover_bus(const struct device *dev)
{
	struct i2c_gpio_context *context = dev->data;
	int rc;

	k_mutex_lock(&context->mutex, K_FOREVER);

	rc = i2c_bitbang_recover_bus(&context->bitbang);

	k_mutex_unlock(&context->mutex);

	return rc;
}

static DEVICE_API(i2c, api) = {
	.configure = i2c_gpio_configure,
	.get_config = i2c_gpio_get_config,
	.transfer = i2c_gpio_transfer,
	.recover_bus = i2c_gpio_recover_bus,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_gpio_target_register,
	.target_unregister = i2c_gpio_target_unregister,
#endif
};

static int i2c_gpio_init(const struct device *dev)
{
	struct i2c_gpio_context *context = dev->data;
	const struct i2c_gpio_config *config = dev->config;
	uint32_t bitrate_cfg;
	int err;
	context->config_state = I2C_GPIO_UNCONFIGURED;
	context->bitbang.data_byte = 0;

	if (!gpio_is_ready_dt(&config->scl_gpio)) {
		LOG_ERR("SCL GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->scl_gpio, GPIO_INPUT | GPIO_OUTPUT_HIGH);
	if (err == -ENOTSUP) {
		err = gpio_pin_configure_dt(&config->scl_gpio, GPIO_OUTPUT_HIGH);
	}
	
	if (err) {
		LOG_ERR("failed to configure SCL GPIO pin (err %d)", err);
		return err;
	}

	if (!gpio_is_ready_dt(&config->sda_gpio)) {
		LOG_ERR("SDA GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->sda_gpio,
				    GPIO_INPUT | GPIO_OUTPUT_HIGH);
	if (err == -ENOTSUP) {
		err = gpio_pin_configure_dt(&config->sda_gpio,
					    GPIO_OUTPUT_HIGH);
	}
	if (err) {
		LOG_ERR("failed to configure SDA GPIO pin (err %d)", err);
		return err;
	}

	i2c_bitbang_init(&context->bitbang, &io_fns, (void *)config);

	bitrate_cfg = i2c_map_dt_bitrate(config->bitrate);
	err = i2c_bitbang_configure(&context->bitbang, bitrate_cfg);
	if (err) {
		LOG_ERR("failed to configure I2C bitbang (err %d)", err);
		return err;
	}

	err = k_mutex_init(&context->mutex);
	if (err) {
		LOG_ERR("Failed to create the i2c lock mutex : %d", err);
		return err;
	}

	return 0;
}

#define	DEFINE_I2C_GPIO(_num)						\
									\
static struct i2c_gpio_context i2c_gpio_dev_data_##_num;		\
									\
static const struct i2c_gpio_config i2c_gpio_dev_cfg_##_num = {		\
	.scl_gpio	= GPIO_DT_SPEC_INST_GET(_num, scl_gpios),	\
	.sda_gpio	= GPIO_DT_SPEC_INST_GET(_num, sda_gpios),	\
	.bitrate	= DT_INST_PROP(_num, clock_frequency),		\
};									\
									\
I2C_DEVICE_DT_INST_DEFINE(_num,						\
	    i2c_gpio_init,						\
	    NULL,							\
	    &i2c_gpio_dev_data_##_num,					\
	    &i2c_gpio_dev_cfg_##_num,					\
	    POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_I2C_GPIO)
