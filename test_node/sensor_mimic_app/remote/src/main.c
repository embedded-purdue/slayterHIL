/*
 * This program builds on the esp32s3's appcpu -- remote CPU. It 
 * busy-waits a SPI subordinate transaction and ends an IPC message to
 * the main core on completion.
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>

/* constants */
// TODO: put in global .h file later
#define TRANS_SIZE (12)
#define TRANS_ERR (0)

/* devices and send buffer */
static const struct device *ipm_dev;
static const struct device *spi_dev;

/*
 * Wait for SPI transaction. 
 * Sends raw data to main core when done.
 */
int main(void)
{
	/* set up IPM device */
	ipm_dev = DEVICE_DT_GET(DT_NODELABEL(ipm0));
	if (!ipm_dev) {
		printk("Failed to get IPM device.\n\r");
		return 0;
	}

	/* set up SPI device */
	spi_dev = DEVICE_DT_GET(DT_NODELABEL(spibb0));
	struct spi_config config;
	config.operation = SPI_OP_MODE_SLAVE | SPI_WORD_SET(8);

	if (!device_is_ready(spi_dev)) {
		printk("%s: device not ready.\n", spi_dev->name);
		return 0;
	}

	/* set uup SPI buffers */
    // read
    uint8_t read_buf[TRANS_SIZE] = {0};
    struct spi_buf spi_read_buf = {&read_buf, TRANS_SIZE};
    const struct spi_buf_set rx_buf = {&spi_read_buf, 1};

    // write; dummy empty
    uint8_t write_buf[TRANS_SIZE] = {0};
    struct spi_buf spi_write_buf = {&write_buf, TRANS_SIZE};
    const struct spi_buf_set tx_buf = {&spi_write_buf, 1};

	// test output
	for (int i = 0; i < TRANS_SIZE; i++) write_buf[i] = i + 4;

	/* forever accept SPI transactions */
	while (1) {
		// reset the read buffer
		memset(&read_buf, 0, TRANS_SIZE);

		// block for SPI
		// TODO: better method of indiciation for errors
	    int trans_res = spi_transceive(spi_dev, &config, &tx_buf, &rx_buf);
		// if (trans_res) memset(&read_buf, TRANS_ERR, TRANS_SIZE);

		ipm_send(ipm_dev, -1, TRANS_SIZE, &read_buf, TRANS_SIZE);
	}

	return 0;
}
