#include <zephyr/drivers/spi.h>
#include <zephyr/devicetree.h>

// our own specifiers
#define LEN_TRANSCEIVE 4
#define DELAY 0
#define SPI_DEVICE_NAME esp32

// operation flags we want
static const spi_operation_t operation_flags = (

    SPI_OP_MODE_MASTER |
    SPI_TRANSFER_MSB |
    SPI_WORD_SET(8));

static const struct spi_dt_spec spi_device = SPI_DT_SPEC_GET(
    DT_NODELABEL(SPI_DEVICE_NAME),
    operation_flags,
    DELAY);

int main()
{
    /*
    - I want to read four bytes so I need a four-byte array.
    - That array needs to be a member of a spi_buf
    - spi_buf needs to be a member of a spi_buf_set
    */
    // read
    uint8_t read_buf[LEN_TRANSCEIVE] = {0};
    struct spi_buf spi_read_buf = {&read_buf, LEN_TRANSCEIVE};
    const struct spi_buf_set rx_buf = {&spi_read_buf, 1};

    // write
    uint8_t write_buf[LEN_TRANSCEIVE] = {0};
    struct spi_buf spi_write_buf = {&write_buf, LEN_TRANSCEIVE};
    const struct spi_buf_set tx_buf = {&spi_write_buf, 1};

    // populate the write buffer
    for (int i = 0; i < LEN_TRANSCEIVE; i++)
        write_buf[i] = i;

    // transceive
    int transceive_res = spi_transceive_dt(
        &spi_device,
        &tx_buf,
        &rx_buf);

    // transceive result should be 0 (if master, which we are) upon end.
    if (transceive_res)
    {
        printk("Error on transcceiving\n");
        return 1;
    }

    // print out what has been read
    for (int i = 0; i < LEN_TRANSCEIVE; i++)
        printk("received word %d: %d\n", i, read_buf[i]);

    return 0;
}