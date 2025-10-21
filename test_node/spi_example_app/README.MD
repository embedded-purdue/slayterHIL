# SPI Example App

## Running the project
To build, execute
`build-esp32-spi-example`

To run,execute 
`run-esp32-spi-example`

On your board, connect GPIO11 AND GPIO13. This will connect MOSI to MISO, and emulate receiving messages from the suboordinate. We are essentially printing out on MOSI and re-reading that output back in through MISO.

## Expected results
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

# How was the device tree node constructed?
## Useful Resources
1. https://docs.zephyrproject.org/latest/build/dts/troubleshooting.html
2. https://blog.golioth.io/how-to-use-generic-spi-devices-with-zephyr/
3. https://docs.zephyrproject.org/latest/doxygen/html/group__spi__interface.html

## Step 1: Gathering Reference Files

### test_node/zephyr/dts/xtensa/espressif/esp32s3/esp32s3_common.dtsi

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

### test_node/zephyr/boards/espressif/esp32s3_devkitc/esp32s3_devkitc_procpu.dts

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


### test_node/zephyr/boards/espressif/esp32s3_devkitc/esp32s3_devkitc-pinctrl.dtsi

1. This is the pinctrl for our board. It provides the physical wiring, including the default pinmux group `spim2_default` that we saw was used in `esp32s3_devkitc_procpu.dts`
2. This is where we can find `spim2_default`: 
```dts
spim2_default: spim2_default {
	group1 {
		pinmux = <SPIM2_MISO_GPIO13>,
			 <SPIM2_SCLK_GPIO12>,
			 <SPIM2_CSEL_GPIO10>;
	};

	group2 {
		pinmux = <SPIM2_MOSI_GPIO11>;
		output-low;
	};
};
```
3. We can always change it to remap to the GPIOs we want. However, for sipmlicity, we will stick to this. 
4. The reason why SPIM2_MOSI_GPIO11 is in a different group is becasue we specify a setting that sets it apart from the other 3 GPIOs: `output-low`. Grouping is just a way to group pins with the same settings together.

### test_node/zephyr/dts/bindings/spi/espressif,esp32-spi.yaml

1. This is the "interface" file with the expected properties of a SPI controller on the ESP32.

### test_node/zephyr/dts/bindings/test/vnd,spi-device.yaml

1. This is the "interface" file with the expected properties of a device living on the SPI controller. Not to be confused with the spi controller itself! 
2. It's generic, not built for the esp32 specifically, but it works.

## Step 2: Creating our own DTS overlay file.
Controllers such as SPI expect a 'bus' device to use. So we will have the main SPI controller node, and the device sub-node.

### Specify and define the SPI controller node.

We want to edit the default overlay for spi2. Thus, we will reference it with `&`.

```dts
&spi2 {
   ...
};
```

We will declare it as compatible with the `espressif,esp32-spi` interface, which will enforce the set of rules in `espressif,esp32-spi` onto this node. We will also define `cs-gpios`, which doesn't have a provided default.
- `cs-gpios` is an array of GPIOs to specify all the chip-select devices that this SPI controller feeds into; for each chipselect, you may have another device reading from the manager (the ESP32). 
- we only have 1 device that needs to read, so we will have an array of just 1. 

```dts
&spi2 {
	compatible = "espressif,esp32-spi";
    cs-gpios = <&gpio0 10 GPIO_ACTIVE_LOW>;
};
```

### Specify the devices living at each ChipSelect.

For our spi master controller, we also expect a device to output to (our suboordinate). This defines the existence of that device.
- we declare it as compatible to the `vnd,spi-device,` which will enforce the set of rules in `vnd,spi-device` onto this node.
- The name "esp32" does not matter. We are simply naming it esp32 because we will feed our output at MOSI back into MISO for testing purposes. If we were talking to a raspberry pi, for example, we'd probably label it as "pi".
- `reg = <0>` connects it to the CS at index 0 in `cs-gpios`. 

```dts
&spi2 {
    compatible = "espressif,esp32-spi";
    cs-gpios = <&gpio0 10 GPIO_ACTIVE_LOW>;

    esp32: esp32@0 {
        compatible = "vnd,spi-device";
        reg = <0>;
        spi-max-frequency = <1600000>;
        label = "esp32";
    };
};
```
