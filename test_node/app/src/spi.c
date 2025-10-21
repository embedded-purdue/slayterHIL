#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#define OP (SPI_OP_NODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))
static const struct spi_dt_spec spi = SPI_DT_SPEC_GET(DT_NODELABEL(spi2), OP, 0);

int main()
{
}