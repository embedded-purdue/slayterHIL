#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>

#define LEN_TRANSCEIVE 12
#define SPIBB_NODE	DT_NODELABEL(spibb0)

int main(void)
{
	printk("hello world!");
	const struct device *const dev = DEVICE_DT_GET(SPIBB_NODE);

	struct spi_config config;
	config.operation = SPI_OP_MODE_SLAVE | SPI_WORD_SET(8);

	if (!device_is_ready(dev)) {
		printk("%s: device not ready.\n", dev->name);
		return 0;
	}

    // read
    uint8_t read_buf[LEN_TRANSCEIVE] = {0};
    struct spi_buf spi_read_buf = {&read_buf, LEN_TRANSCEIVE};
    const struct spi_buf_set rx_buf = {&spi_read_buf, 1};

    // write
    uint8_t write_buf[LEN_TRANSCEIVE] = {0};
    struct spi_buf spi_write_buf = {&write_buf, LEN_TRANSCEIVE};
    const struct spi_buf_set tx_buf = {&spi_write_buf, 1};

	// populate the write buffer
    for (int i = 0; i < LEN_TRANSCEIVE; i++) write_buf[i] = i + 4;

    // transceive
	printk("Transceiving\n");
    int transceive_res = spi_transceive(dev, &config, &tx_buf, &rx_buf);

	// transceive result should be 0 (if master, which we are) upon end.
    if (transceive_res)
    {
        printk("Error on transcceiving\n");
        return 1;
    }

	// print out what has been read
    for (int i = 0; i < LEN_TRANSCEIVE; i++) printk("received word %d: %d\n", i, read_buf[i]);

	return 0;
}
