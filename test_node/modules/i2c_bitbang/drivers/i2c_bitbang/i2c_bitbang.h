#include <zephyr/drivers/gpio.h>

/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Functions for setting and getting the state of the I2C lines.
 *
 * These need to be implemented by the user of this library.
 */
struct i2c_bitbang_io {
	/* Set the state of the SCL line (zero/non-zero value) */
	void (*set_scl)(void *io_context, int state);
	/* Return the state of the SCL line (zero/non-zero value) */
	int (*get_scl)(void *io_context);
	/* Set the state of the SDA line (zero/non-zero value) */
	void (*set_sda)(void *io_context, int state);
	/* Return the state of the SDA line (zero/non-zero value) */
	int (*get_sda)(void *io_context);
};

/**
 * @brief I2C configuration state enum
 * 
 * Used internally to track whether the bitbang instance is configured
 * as a controller or target, or is unconfigured.
 */
enum i2c_config_state {
	I2C_GPIO_CONTROLLER,
	I2C_GPIO_TARGET,
	I2C_GPIO_UNCONFIGURED,
};

/**
 * @brief I2C FSM states
 * 
 * used internally to make sure other states are being interrupted
 */
enum i2c_target_state {
	I2C_TARGET_IDLE,
	I2C_TARGET_RECEIVING_ADDRESS,
	I2C_TARGET_RECEIVING_DATA,
	I2C_TARGET_SENDING_DATA,
};

#define I2C_STOP_DETECTED (-1)
#define I2C_RESTART_DETECTED (-2)
#define I2C_CALLBACK_ERROR (-3)
#define I2C_MASTER_NACKED (-4)

/**
 * @brief Instance data for i2c_bitbang
 *
 * A driver or other code wishing to use the i2c_bitbang library should
 * create one of these structures then use it via the library APIs.
 * Structure members are private, and shouldn't be accessed directly.
 */
struct i2c_bitbang {
	const struct i2c_bitbang_io	*io;
	void				*io_context;
	uint32_t			delays[2];
	uint32_t			dev_config;

	/* target configuration */
	struct k_work target_work;
	struct i2c_target_config* target_cfg;
	enum i2c_target_state target_state;
	struct gpio_callback sda_cb;
	uint8_t data_byte;
};

/**
 * @brief Initialize an i2c_bitbang instance
 *
 * @param bitbang	The instance to initialize
 * @param io		Functions to use for controlling I2C bus lines
 * @param io_context	Context pointer to pass to i/o functions when then are
 *			called.
 */
void i2c_bitbang_init(struct i2c_bitbang *bitbang,
			const struct i2c_bitbang_io *io, void *io_context);

/**
 * Implementation of the functionality required by the 'configure' function
 * in struct i2c_driver_api.
 */
int i2c_bitbang_configure(struct i2c_bitbang *bitbang, uint32_t dev_config);

/**
 * Implementation of the functionality required by the 'get_config' function
 * in struct i2c_driver_api.
 */
int i2c_bitbang_get_config(struct i2c_bitbang *context, uint32_t *config);

/**
 * Implementation of the functionality required by the 'recover_bus'
 * function in struct i2c_driver_api.
 */
int i2c_bitbang_recover_bus(struct i2c_bitbang *bitbang);

/**
 * Implementation of the functionality required by the 'transfer' function
 * in struct i2c_driver_api.
 */
int i2c_bitbang_transfer(struct i2c_bitbang *bitbang,
			   struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t slave_address);

/**
 * GPIO ISR for SDA
 */
void i2c_bitbang_gpio_isr(const struct device *dev,
				struct gpio_callback *cb, 
				uint32_t pins);

/**
 * I2C target work handler, scheduled by GPIO ISR on START condition detection
 */
void i2c_bitbang_target_work_handler(struct k_work *work);