zephyr_library()
zephyr_library_sources(
    spi_esp32_spis.c
    ../../../hal/espressif/components/hal/spi_slave_hal.c
    ../../../hal/espressif/components/hal/spi_slave_hal_iram.c
)
zephyr_include_directories(.)
