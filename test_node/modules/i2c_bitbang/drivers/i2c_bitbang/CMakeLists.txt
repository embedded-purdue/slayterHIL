zephyr_library()
zephyr_library_sources(
    i2c_bitbang.c
    i2c_bitbang_gpio.c
)
zephyr_include_directories(.)