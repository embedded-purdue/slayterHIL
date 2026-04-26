#pragma once

void send_spi(char *msg, size_t size, int fd, int bits, int speed, int mode);

void init_spi(int fd_out, uint8_t *mode_out, uint8_t *bits_out, uint32_t *speed_out);

