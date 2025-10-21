#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

// figure out what flags we want later.
// we are declaring the esp32 as master right now.
static const struct spi_dt_spec spi = SPI_DT_SPEC_GET(
    DT_NODELABEL(esp32),
    SPI_OP_MODE_MASTER | SPI_WORD_SET(8),
    0);

int main()
{
    printk("%d\n", DT_HAS_ALIAS(myspi));
}