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
Remember to press the RESET button to run the program.
```bash
*** Booting Zephyr OS build v4.2.0-6442-gbbfd206a47ee ***
received word 0: 0
received word 1: 1
received word 2: 2
received word 3: 3
```

## How was the device tree node constructed?
### Useful Resources
1. https://docs.zephyrproject.org/latest/build/dts/troubleshooting.html
2. https://blog.golioth.io/how-to-use-generic-spi-devices-with-zephyr/

### Step 1: Gathering Reference Files
test_node/zephyr/dts/xtensa/espressif/esp32s3/esp32s3_common.dtsi
1. This is the SoC-level (not board-level!) device tree layout. It defines the existence of each hardware peripheral the SoC provides, independent of any specific board's wiring
2. Look for spi. We can see spi2 is defined as a module we can use, along with its default properties.
```dts
spi2: spi@60024000 {
		compatible = "espressif,esp32-spi";
		reg = <0x60024000 DT_SIZE_K(4)>;
		interrupts = <SPI2_INTR_SOURCE IRQ_DEFAULT_PRIORITY 0>;
		interrupt-parent = <&intc>;
		clocks = <&clock ESP32_SPI2_MODULE>;
		dma-host = <0>;
		status = "disabled";
};
```

test_node/zephyr/boards/espressif/esp32s3_devkitc/esp32s3_devkitc_procpu.dts
1. This is where all the available, pre-defined controllers are created for us. This is board-level, not just SoC level, like `esp32s3_common.dtsi`
2. See &spi2. This is referencing the spi2 node defined. It's &spi2, because it's referencing the already-created node of the spi hardware module in `esp32s3_common.dtsi`. 
```dts
&spi2 {
	#address-cells = <1>;
	#size-cells = <0>;
	status = "okay";
	pinctrl-0 = <&spim2_default>;
	pinctrl-names = "default";
};
```
3. Notice: it overwrites the `status = "disabled"` and enables the hardware module. It also defines a pinctrl-0, among other parameters that are specific to this board. 
4. spim2_default is referenced elsewhere as per the `&`... let's go find it

test_node/zephyr/boards/espressif/esp32s3_devkitc/esp32s3_devkitc-pinctrl.dtsi
1. This is the pinctrl for our board. It provides the physical wiring, including the default pinmux group `spim2_default` that we saw was used in `esp32s3_devkitc_procpu.dts`
