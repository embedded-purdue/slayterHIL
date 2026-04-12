#include "spi_fs.h"
#include "pwm.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(spi_fs_thread, LOG_LEVEL_INF);

// SPI slave config for flight simulator communication
static const spi_operation_t spi_op_flags = (
    SPI_OP_MODE_SLAVE |
    SPI_TRANSFER_MSB |
    SPI_WORD_SET(8));

static const struct spi_dt_spec spi_fs_dev = SPI_DT_SPEC_GET(
    DT_NODELABEL(spi_dev),
    spi_op_flags,
    0);

K_THREAD_STACK_DEFINE(spi_fs_stack, SPI_FS_STACK_SIZE);
struct k_thread spi_fs_thread_data;

void spi_fs_thread(void*, void*, void*) {
    while (1) {
        // get the latest voltage from the PWM thread
        float voltage = pwm_get_latest_voltage();

        // pack into a byte buffer for SPI transfer
        uint8_t tx_buf[sizeof(float)];
        memcpy(tx_buf, &voltage, sizeof(float));

        struct spi_buf spi_tx = {.buf = tx_buf, .len = sizeof(tx_buf)};
        const struct spi_buf_set tx_set = {.buffers = &spi_tx, .count = 1};

        // blocks until the FS master clocks the data out
        int rc = spi_write_dt(&spi_fs_dev, &tx_set);
        if (rc < 0) {
            LOG_ERR("SPI write to FS failed: %d", rc);
        } else {
            LOG_INF("Sent voltage to FS: %.3f V", (double)voltage);
        }
    }
}

void spi_fs_init(void) {
    if (!spi_is_ready_dt(&spi_fs_dev)) {
        LOG_ERR("SPI FS device not ready");
        return;
    }

    k_thread_create(&spi_fs_thread_data, spi_fs_stack,
                    K_THREAD_STACK_SIZEOF(spi_fs_stack),
                    spi_fs_thread,
                    NULL, NULL, NULL,
                    SPI_FS_PRIORITY, 0, K_NO_WAIT);
}
