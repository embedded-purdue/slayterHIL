/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software driven 'bit-banging' library for I2C
 *
 * This code implements the I2C single master protocol in software by directly
 * manipulating the levels of the SCL and SDA lines of an I2C bus. It supports
 * the Standard-mode and Fast-mode speeds and doesn't support optional
 * protocol feature like 10-bit addresses or clock stretching.
 *
 * Timings and protocol are based Rev. 7 of the I2C specification:
 * https://www.nxp.com/docs/en/user-guide/UM10204.pdf
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include "i2c_bitbang.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(i2c_bitbang_gpio);

/*
 * Indexes into delay table for each part of I2C timing waveform we are
 * interested in. In practice, for Standard and Fast modes, there are only two
 * different numerical values (T_LOW and T_HIGH) so we alias the others to
 * these. (Actually, we're simplifying a little, T_SU_STA could be T_HIGH on
 * Fast mode)
 */
#define T_LOW		0
#define T_HIGH		1
#define T_SU_STA	T_LOW
#define T_HD_STA	T_HIGH
#define T_SU_STP	T_HIGH
#define T_BUF		T_LOW

#define NS_TO_SYS_CLOCK_HW_CYCLES(ns) \
	((uint64_t)sys_clock_hw_cycles_per_sec() * (ns) / NSEC_PER_SEC + 1)

int i2c_bitbang_configure(struct i2c_bitbang *context, uint32_t dev_config)
{
	/* Check for features we don't support */
	if (I2C_ADDR_10_BITS & dev_config) {
		return -ENOTSUP;
	}

	/* Setup speed to use */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		context->delays[T_LOW]  = NS_TO_SYS_CLOCK_HW_CYCLES(4700);
		context->delays[T_HIGH] = NS_TO_SYS_CLOCK_HW_CYCLES(4000);
		break;
	case I2C_SPEED_FAST:
		context->delays[T_LOW]  = NS_TO_SYS_CLOCK_HW_CYCLES(1300);
		context->delays[T_HIGH] = NS_TO_SYS_CLOCK_HW_CYCLES(600);
		break;
	default:
		return -ENOTSUP;
	}

	context->dev_config = dev_config;

	return 0;
}

int i2c_bitbang_get_config(struct i2c_bitbang *context, uint32_t *config)
{
	if (context->dev_config == 0) {
		return -EIO;
	}

	*config = context->dev_config;

	return 0;
}

static void i2c_set_scl(struct i2c_bitbang *context, int state)
{
	context->io->set_scl(context->io_context, state);
#ifdef CONFIG_I2C_GPIO_CLOCK_STRETCHING
	if (state == 1) {
		/* Wait for slave to release the clock */
		WAIT_FOR(context->io->get_scl(context->io_context) != 0,
			 CONFIG_I2C_GPIO_CLOCK_STRETCHING_TIMEOUT_US,
			 ;);
	}
#endif
}

static int i2c_get_scl(struct i2c_bitbang *context)
{
	return context->io->get_scl(context->io_context);
}

static void i2c_set_sda(struct i2c_bitbang *context, int state)
{
	context->io->set_sda(context->io_context, state);
}

static int i2c_get_sda(struct i2c_bitbang *context)
{
	return context->io->get_sda(context->io_context);
}

static void i2c_delay(unsigned int cycles_to_wait)
{
	uint32_t start = k_cycle_get_32();

	/* Wait until the given number of cycles have passed */
	while (k_cycle_get_32() - start < cycles_to_wait) {
	}
}

static void i2c_start(struct i2c_bitbang *context)
{
	if (!i2c_get_sda(context)) {
		/*
		 * SDA is already low, so we need to do something to make it
		 * high. Try pulsing clock low to get slave to release SDA.
		 */
		i2c_set_scl(context, 0);
		i2c_delay(context->delays[T_LOW]);
		i2c_set_scl(context, 1);
		i2c_delay(context->delays[T_SU_STA]);
	}
	i2c_set_sda(context, 0);
	i2c_delay(context->delays[T_HD_STA]);

	i2c_set_scl(context, 0);
	i2c_delay(context->delays[T_LOW]);
}

static void i2c_repeated_start(struct i2c_bitbang *context)
{
	i2c_set_sda(context, 1);
	i2c_set_scl(context, 1);
	i2c_delay(context->delays[T_HIGH]);

	i2c_delay(context->delays[T_SU_STA]);
	i2c_start(context);
}

static void i2c_stop(struct i2c_bitbang *context)
{
	i2c_set_sda(context, 0);
	i2c_delay(context->delays[T_LOW]);

	i2c_set_scl(context, 1);
	i2c_delay(context->delays[T_HIGH]);

	i2c_delay(context->delays[T_SU_STP]);
	i2c_set_sda(context, 1);
	i2c_delay(context->delays[T_BUF]); /* In case we start again too soon */
}

static void i2c_write_bit(struct i2c_bitbang *context, int bit)
{
	/* SDA hold time is zero, so no need for a delay here */
	i2c_set_sda(context, bit);
	i2c_set_scl(context, 1);
	i2c_delay(context->delays[T_HIGH]);
	i2c_set_scl(context, 0);
	i2c_delay(context->delays[T_LOW]);
}

static bool i2c_read_bit(struct i2c_bitbang *context)
{
	bool bit;

	/* SDA hold time is zero, so no need for a delay here */
	i2c_set_sda(context, 1); /* Stop driving low, so slave has control */

	i2c_set_scl(context, 1);
	i2c_delay(context->delays[T_HIGH]);

	bit = i2c_get_sda(context);

	i2c_set_scl(context, 0);
	i2c_delay(context->delays[T_LOW]);
	return bit;
}

static bool i2c_write_byte(struct i2c_bitbang *context, uint8_t byte)
{
	uint8_t mask = 1 << 7;

	do {
		i2c_write_bit(context, byte & mask);
	} while (mask >>= 1);

	/* Return inverted ACK bit, i.e. 'true' for ACK, 'false' for NACK */
	return !i2c_read_bit(context);
}

static uint8_t i2c_read_byte(struct i2c_bitbang *context)
{
	unsigned int byte = 1U;

	do {
		byte <<= 1;
		byte |= i2c_read_bit(context);
	} while (!(byte & (1 << 8)));

	return byte;
}

int i2c_bitbang_transfer(struct i2c_bitbang *context,
			   struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t slave_address)
{
	uint8_t *buf, *buf_end;
	unsigned int flags;
	int result = -EIO;

	if (!num_msgs) {
		return 0;
	}

	/* We want an initial Start condition */
	flags = I2C_MSG_RESTART;

	/* Make sure we're in a good state so slave recognises the Start */
	i2c_set_scl(context, 1);
	flags |= I2C_MSG_STOP;

	do {
		/* Stop flag from previous message? */
		if (flags & I2C_MSG_STOP) {
			i2c_stop(context);
		}

		/* Forget old flags except start flag */
		flags &= I2C_MSG_RESTART;

		/* Start condition? */
		if (flags & I2C_MSG_RESTART) {
			i2c_start(context);
		} else if (msgs->flags & I2C_MSG_RESTART) {
			i2c_repeated_start(context);
		}

		/* Get flags for new message */
		flags |= msgs->flags;

		/* Send address after any Start condition */
		if (flags & I2C_MSG_RESTART) {
			unsigned int byte0 = slave_address << 1;

			byte0 |= (flags & I2C_MSG_RW_MASK) == I2C_MSG_READ;
			if (!i2c_write_byte(context, byte0)) {
				goto finish; /* No ACK received */
			}
			flags &= ~I2C_MSG_RESTART;
		}

		/* Transfer data */
		buf = msgs->buf;
		buf_end = buf + msgs->len;
		if ((flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			/* Read */
			while (buf < buf_end) {
				*buf++ = i2c_read_byte(context);
				/* ACK the byte, except for the last one */
				i2c_write_bit(context, buf == buf_end);
			}
		} else {
			/* Write */
			while (buf < buf_end) {
				if (!i2c_write_byte(context, *buf++)) {
					goto finish; /* No ACK received */
				}
			}
		}

		/* Next message */
		msgs++;
		num_msgs--;
	} while (num_msgs);

	/* Complete without error */
	result = 0;
finish:
	i2c_stop(context);

	return result;
}

int i2c_bitbang_recover_bus(struct i2c_bitbang *context)
{
	int i;

	/*
	 * The I2C-bus specification and user manual (NXP UM10204
	 * rev. 6, section 3.1.16) suggests the master emit 9 SCL
	 * clock pulses to recover the bus.
	 *
	 * The Linux kernel I2C bitbang recovery functionality issues
	 * a START condition followed by 9 STOP conditions.
	 *
	 * Other I2C slave devices (e.g. Microchip ATSHA204a) suggest
	 * issuing a START condition followed by 9 SCL clock pulses
	 * with SDA held high/floating, a REPEATED START condition,
	 * and a STOP condition.
	 *
	 * The latter is what is implemented here.
	 */

	/* Start condition */
	i2c_start(context);

	/* 9 cycles of SCL with SDA held high */
	for (i = 0; i < 9; i++) {
		i2c_write_bit(context, 1);
	}

	/* Another start condition followed by a stop condition */
	i2c_repeated_start(context);
	i2c_stop(context);

	/* Check if bus is clear */
	if (i2c_get_sda(context)) {
		return 0;
	} else {
		return -EBUSY;
	}
}

void i2c_bitbang_init(struct i2c_bitbang *context,
			const struct i2c_bitbang_io *io, void *io_context)
{
	context->io = io;
	context->io_context = io_context;
	i2c_bitbang_configure(context, I2C_SPEED_STANDARD << I2C_SPEED_SHIFT);
}

/* target API functions */

/**
 * @brief Detect a START condition on the bus
 * 
 * this function will be called on SDA falling edge
 * @param context Pointer to the i2c_bitbang instance
 * @return true if START detected, false otherwise
 */
static int i2c_bitbang_detect_start(struct i2c_bitbang *context)
{
	return i2c_get_scl(context) && !i2c_get_sda(context);
}

/**
 * @brief Detect a STOP condition on the bus
 * 
 * this function will be called on SDA rising edge
 * @param context Pointer to the i2c_bitbang instance
 * @return true if STOP detected, false otherwise
 *
static int i2c_bitbang_detect_stop(struct i2c_bitbang *context)
{
	return i2c_get_scl(context) && i2c_get_sda(context);
}
	*/

/**
 * @brief checks if i2c bus is idle
 * 
 * @param context Pointer to the i2c_bitbang instance
 * @return true if bus is idle, false otherwise
 *
static int i2c_bitbang_is_bus_idle(struct i2c_bitbang *context)
{
	return i2c_get_scl(context) && i2c_get_sda(context);
} */

/**
 * @brief reads bit from i2c bus as target
 * 
 * @param context Pointer to the i2c_bitbang instance
 * @return bit read
 */
static int i2c_bitbang_target_read_bit(struct i2c_bitbang* context)
{
	/* wait for SCL to go high */
	while(!i2c_get_scl(context)) {}

	int bit = i2c_get_sda(context);

	/* wait for SCL to go low */
	while(i2c_get_scl(context)) {}

	return bit;
}

/**
 * @brief reads byte from i2c bus as target
 * @param context Pointer to the i2c_bitbang instance
 * @return byte read
 */
static uint8_t i2c_bitbang_target_read_byte(struct i2c_bitbang *context)
{
	uint8_t byte = 0;
	for (int i = 0; i < 8; i++) {
		byte <<= 1;
		byte |= i2c_bitbang_target_read_bit(context);
	}
	return byte;
}

/**
 * @brief reads ACK/NACK from master after target sends a byte
 * 
 * @param context Pointer to the i2c_bitbang instance
 * @return 1 if NACK (master done), 0 if ACK (continue)
 */
static int i2c_bitbang_target_get_ack(struct i2c_bitbang *context)
{
    return i2c_bitbang_target_read_bit(context); /* ACK=0, NACK=1, so return true if NACK */
}

/**
 * @brief reads address on i2c line
 * 
 * @param context Pointer to the i2c_bitbang instance
 * @param addr pointer to address variable
 * @param is_read pointer to read/write flag variable -- 0 means target writes to master
 * @return 0 on success, negative error code otherwise
 */
static int i2c_bitbang_read_address(struct i2c_bitbang *context,
					uint8_t *addr, bool *is_read)
{
	uint8_t byte;

	byte = i2c_bitbang_target_read_byte(context);
	*addr = byte >> 1;
	*is_read = byte & 0x01;

	return 0;
}

/**
 * @brief sends NACK/ACK on bus as target
 * 
 * @param context Pointer to the i2c_bitbang instance
 * @param ack 0 to send ACK, 1 to send NACK
 */
static void i2c_bitbang_target_send_ack(struct i2c_bitbang *context, bool ack)
{
	/* wait for SCL to go low */
	while(i2c_get_scl(context)) {}

	i2c_set_sda(context, ack);

	/* wait for SCL to go high */
	while(!i2c_get_scl(context)) {}

	/* wait for SCL to go low */
	while(i2c_get_scl(context)) {}

	i2c_set_sda(context, 1); /* release SDA */
}

/**
 * @brief sends one bit onto bus as target
 * 
 * @param context Pointer to the i2c_bitbang instance
 * @param bit bit to send
 */
static void i2c_bitbang_target_write_bit(struct i2c_bitbang *context, int bit)
{
	/* wait for SCL to go low */
	while(i2c_get_scl(context)) {}

	i2c_set_sda(context, bit);

	/* wait for SCL to go high */
	while(!i2c_get_scl(context)) {}

	/* wait for SCL to go low */
	while(i2c_get_scl(context)) {}
}

/**
 * @brief sends one byte onto bus as target
 * 
 * @param context Pointer to the i2c_bitbang instance
 * @param byte byte to send
 */
static void i2c_bitbang_target_write_byte(struct i2c_bitbang *context, uint8_t byte)
{
	for (int i = 7; i >= 0; i--) {
		i2c_bitbang_target_write_bit(context, (byte >> i) & 0x01);
	}

	i2c_set_sda(context, 1); /* release SDA for ACK bit from master */
}

/**
 * @brief detects repeated start or stop condition on bus after data reception
 * 
 * @param context Pointer to the i2c_bitbang instance
 * @return 0 if neither detected, else the one that is detected
 */
static int i2c_bitbang_target_detect_postop_condition(struct i2c_bitbang *context)
{
	/* wait for SCL to go high */
	while(!i2c_get_scl(context)) {}

	/* read initial sda state */
	int initial_sda = i2c_get_sda(context);

	/* else, wait for stop by seeing if the SDA line goes high while SCL is high */
	i2c_delay(context->delays[T_SU_STA] + context->delays[T_HIGH]);
	/* TODO: maybe add delay above to wait for stabilization?*/
	if(i2c_get_sda(context) != initial_sda) {
		/* either repeated start or stop was detected */
		context->target_state = I2C_TARGET_IDLE;

		if(!initial_sda) {
			context->target_cfg->callbacks->stop(context->target_cfg);
			return I2C_STOP_DETECTED;
		}
		else return I2C_RESTART_DETECTED;
	}

	return 0;
}
/**
 * @brief reads bytes from bus until STOP is detected
 * 
 * @param context Pointer to the i2c_bitbang instance
 * @return 0 on success, error code corresponding to STOP / RESTART otherwise
 */
static int i2c_bitbang_target_read_bytes(struct i2c_bitbang *context)
{
	uint8_t byte;
	while(1) {
		byte = i2c_bitbang_target_read_byte(context);
		if(context->target_cfg->callbacks->write_received(context->target_cfg, byte))
		{
			/* write_received returned some sort of error */
			i2c_bitbang_target_send_ack(context, 1);
			context->target_state = I2C_TARGET_IDLE;
			return I2C_CALLBACK_ERROR;
		}
		else i2c_bitbang_target_send_ack(context, 0);

		/* check for stop */
		int ret = i2c_bitbang_target_detect_postop_condition(context);
		if(ret) {
			context->target_state = I2C_TARGET_IDLE;
			return ret;
		}
	}

	context->target_state = I2C_TARGET_IDLE;
	return 0;
}

/**
 * @brief writes bytes to bus until NACK or STOP is detected
 * 
 * @param context Pointer to the i2c_bitbang instance
 * @return 0 on success, error code corresponding to STOP / RESTART otherwise
 */
static int i2c_bitbang_target_write_bytes(struct i2c_bitbang *context)
{
	while(1) {
		uint8_t byte;
		if(context->target_cfg->callbacks->read_processed(context->target_cfg, &byte)) {
			/* read_processed returned some sort of error */
			context->target_state = I2C_TARGET_IDLE;
			return I2C_CALLBACK_ERROR;
		}
		i2c_bitbang_target_write_byte(context, byte);

		/* check for ACK then stop conditions */
		if(i2c_bitbang_target_get_ack(context)) {
			/* master NACKed the byte, we're done */
			context->target_state = I2C_TARGET_IDLE;
			return I2C_MASTER_NACKED; /* TODO: check to see if need to return */
		}

		int ret = i2c_bitbang_target_detect_postop_condition(context);
		/* check for stop or repeated start before writing next byte */
		if(ret) {
			context->target_state = I2C_TARGET_IDLE;
			return ret;
		}
	}

	context->target_state = I2C_TARGET_IDLE;
	return 0;
}

/**
 * @brief work handler for i2c target transaction processing
 * 
 * runs after START detected
 */
void i2c_bitbang_target_work_handler(struct k_work *work)
{
	struct i2c_bitbang *context = CONTAINER_OF(work, struct i2c_bitbang, target_work);
	uint8_t addr;
	bool is_read;
	int rc;

	if(context->target_cfg == NULL)
	{
		LOG_ERR("Work handler called with NULL target_cfg");
		return;
	}

PRE_ADDRESS:

	context->target_state = I2C_TARGET_RECEIVING_ADDRESS;

	/* Read address byte */
	rc = i2c_bitbang_read_address(context, &addr, &is_read);
	if (rc < 0) {
		LOG_ERR("Failed to read address byte: %d", rc);
		context->target_state = I2C_TARGET_IDLE;
		return;
	}


	/* Check if address matches our target address */
	if (addr != context->target_cfg->address) {
		LOG_DBG("Address mismatch: received 0x%02x, expected 0x%02x", addr, context->target_cfg->address);
		context->target_state = I2C_TARGET_IDLE;
		return;
	}

	LOG_DBG("Address matched: 0x%02x, is_read=%d", addr, is_read);

	/* Send ACK for address byte */
	i2c_bitbang_target_send_ack(context, 0);/* Send ACK for address byte */

	/* Check for STOP or Repeated START after address ACK */
	int ret = i2c_bitbang_target_detect_postop_condition(context);
	if (ret == I2C_STOP_DETECTED) {
		context->target_state = I2C_TARGET_IDLE;
		return;
	} else if (ret == I2C_RESTART_DETECTED) {
		goto PRE_ADDRESS;
	}

	if (!is_read) {
		context->target_state = I2C_TARGET_RECEIVING_DATA;
		/* Master wants to write to us */
		context->target_cfg->callbacks->write_requested(context->target_cfg);
		int ret = i2c_bitbang_target_read_bytes(context);
		if(ret == I2C_RESTART_DETECTED) {
			goto PRE_ADDRESS;
		}
	} else {
		/* Master wants to read from us */
		context->target_state = I2C_TARGET_SENDING_DATA;
		uint8_t first_byte;
		context->target_cfg->callbacks->read_requested(context->target_cfg, &first_byte);
		i2c_bitbang_target_write_byte(context, first_byte);

		/* check for ACK then stop conditions */
		if(i2c_bitbang_target_get_ack(context)) {
			/* master NACKed the byte, we're done */
			context->target_state = I2C_TARGET_IDLE;
		}

		int ret = i2c_bitbang_target_detect_postop_condition(context);
		/* check for stop or repeated start before writing next byte */
		if(ret == I2C_RESTART_DETECTED) {
			context->target_state = I2C_TARGET_IDLE;
			goto PRE_ADDRESS;
		}
		else if(ret == I2C_STOP_DETECTED) {
			context->target_state = I2C_TARGET_IDLE;
			return;
		}

		if(i2c_bitbang_target_write_bytes(context) == I2C_RESTART_DETECTED) {
			goto PRE_ADDRESS;
		}
	}
}

/**
 * @brief ISR handle for the GPIO pins on SDA to detect START and STOP conditions
 * 
 * @param dev pointer to the GPIO device triggering the interrupt
 * @param cb pointer to the gpio_callback struct for this interrupt
 * @param pins bitmask of the pins that triggered the interrupt
 */
void i2c_bitbang_gpio_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct i2c_bitbang *context = CONTAINER_OF(cb, struct i2c_bitbang, sda_cb);

	if(context->target_state == I2C_TARGET_IDLE) {
		/* check for START condition */
		if(i2c_bitbang_detect_start(context)) {
			/* START detected, schedule work to handle transaction */
			k_work_submit(&context->target_work);
		}
	}
}