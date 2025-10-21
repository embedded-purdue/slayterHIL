# SPI Example App

## Running the project
To build, execute
`build-esp32-spi-example`

To run,execute 
`run-esp32-spi-example`

On your board, connect GPIO11 AND GPIO13. This will connect MOSI to MISO, and emulate receiving messages from the suboordinate. We are essentially printing out on MOSI and re-reading that output back in through MISO.

## expected results
In src/spi_example.c, we see a 4-byte buffer called write_buf. We will populate this with 0, 1, 2, and 3.
```c
for (int i = 0; i < LEN_TRANSCEIVE; i++) write_buf[i] = i;
```

Then, we print out read_buf. If the application is working correcty, we should read back 0, 1, 2, and 3. 
```bash
*** Booting Zephyr OS build v4.2.0-6442-gbbfd206a47ee ***
received word 0: 0
received word 1: 1
received word 2: 2
received word 3: 3
```
