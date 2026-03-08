#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/devicetree.h>

#define LEN_TRANSCEIVE 4
#define SPI_DEVICE_NAME spi_dev

//slave operation flags
static const spi_operation_t operation_flags = (
    SPI_OP_MODE_SLAVE |
    SPI_TRANSFER_MSB |
    SPI_WORD_SET(8));

static const struct spi_dt_spec spi_device = SPI_DT_SPEC_GET(
    DT_NODELABEL(SPI_DEVICE_NAME),
    operation_flags,
    0);

int main(void)
{
    printk("SPI slave started\n");

    if (!spi_is_ready_dt(&spi_device)) {
        printk("SPI device not ready\n");
        return 1;
    }

    while (1) {
        //read buffer: filled by master's MOSI data
        uint8_t read_buf[LEN_TRANSCEIVE] = {0};
        struct spi_buf spi_read_buf = {&read_buf, LEN_TRANSCEIVE};
        const struct spi_buf_set rx_buf = {&spi_read_buf, 1};

        //write buffer: data sent back to master on MISO
        uint8_t write_buf[LEN_TRANSCEIVE] = {0};
        struct spi_buf spi_write_buf = {&write_buf, LEN_TRANSCEIVE};
        const struct spi_buf_set tx_buf = {&spi_write_buf, 1};

        //populate response data (master will read this on MISO)
        for (int i = 0; i < LEN_TRANSCEIVE; i++) {
            write_buf[i] = i + 4;
        }

        //block until master initiates a transaction
        //in slave mode, returns no. of received frames on success (>= 0)
        //or negative error code on failure
        int rc = spi_transceive_dt(&spi_device, &tx_buf, &rx_buf);
        if (rc < 0) {
            printk("SPI transceive error: %d\n", rc);
            continue;
        }
        printk("Received %d frames\n", rc);

        //print what the master sent
        for (int i = 0; i < LEN_TRANSCEIVE; i++) {
            printk("received word %d: %d\n", i, read_buf[i]);
        }
    }

    return 0;
}
