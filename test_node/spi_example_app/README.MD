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
2. See `&spi2`. The `&` (it's easiest to think of this like a C reference) is used since the `spi2` node is already defined in `esp32s3_common.dtsi`.
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
- The node label `esp32` does not matter; this is like a variable name. We are simply naming it esp32 because we will feed our output at MOSI back into MISO for testing purposes. We could name it `pi`, `mydevice`, etc. and it would behave exactly the same. 
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

## Step 3: Creating our C program.

API details are at https://docs.zephyrproject.org/latest/doxygen/html/group__spi__interface.html. As long as we can pass our SPI device and/or controller into the functions we want, we will be able to use this API to perform pre-defined SPI drivr functions.  

### Includes and Defines

The zephyr-pro




vided APIs we need to write our application.
- `spi.h` defines all the SPI driver function calls that we can make, as specified in the SPI api. 
- `devicetree.h` defines all of Zephyr's devicetree functions, such as getting the node from a label, and more.
```
#include <zephyr/drivers/spi.h>
#include <zephyr/devicetree.h>
```

Defining macros for constants is good code practice, so we don't have magic numbers later.
- `LEN_TRANSCEIVE` will be the number of bytes we expect from one READ/WRITE in this program specifically. 
- `DELAY` is the delay in microseconds to wait before starting the transmission and before releasing the CS line.
- `SPI_DEVICE_NAME` is the name of our spi device's node label, which we defined in the devicetree as `esp32`.
```
#define LEN_TRANSCEIVE 4
#define DELAY 0
#define SPI_DEVICE_NAME esp32
```

### Getting our SPI device structures

Looking at the documentation in the API, a lot of functions require information of type `struct spi_dt_spec`. Specifically, the transceive/read/write functions we want to use need this. We will thus get our device from the device tree: 
- be careful what you pass into the first parameter. We need to pass in the spi DEVICE, not the spi controller. This is because `SPI_DT_SPEC_GET` has a sub-macro which uses the device, which it expects to be a bus, in order to get the remaining properties from the parent controller.
- operation_flags is after this.

```c
static const struct spi_dt_spec spi_device = SPI_DT_SPEC_GET(
    DT_NODELABEL(SPI_DEVICE_NAME),
    operation_flags,
    DELAY);
```
Operation flags is an expected parameter of us. It defines the properties of our spi controller, which will reflect in our device driver. Are we currently in `MASTER` mode while communicating to this device? What is our word length? Are we transferring the most significant or least significant bits first?
- `SPI_OP_MODE_MASTER`: our controller is a manager, not a suboordinate. 
- `SPI_TRANSFER_MSB`: most significant bit first. To learn more, review the SPI protocol and how bits are transferred.
- `SPI_WORD_SET(8)`: The expected length of each word we will send/receive is 8 bits. 

```c
static const spi_operation_t operation_flags = (
    SPI_OP_MODE_MASTER |
    SPI_TRANSFER_MSB |
    SPI_WORD_SET(8));
```

### creating our read and write buffers

To read/write/transceive, we need a place to read from or feed to. The SPI api expects pointers to buffers of type `spi_buf_set`, so we will create these. We will do the following for both our rx buffer and our tx buffer. 
- I want to read and write messages of `LEN_TRANSCEIVE`, so we will create buffers of that length. These are the buffers we will be interacting with in order to feed or read the SPI data. 
- `rx_buf` and `tx_buf` are what the SPI APIs will read from / write to. As you can see from the trail of reference, the data originates from `read_buf` and `write_buf`.

```c
uint8_t read_buf[LEN_TRANSCEIVE] = {0};
struct spi_buf spi_read_buf = {&read_buf, LEN_TRANSCEIVE};
const struct spi_buf_set rx_buf = {&spi_read_buf, 1};
```

### Transceiving the data.

We will use the `spi_transceive_dt` function provided by the API, so we can read and write data at the same time. However, note that there are other functions we could have chosen to use: `spi_read_dt` for reading only, `spi_write_dt` for writing only, and more.
- We pass in the addresses of `tx_buf` and `rx_buf` so it may feed into these buffers. 

```c
int transceive_res = spi_transceive_dt(
    &spi_device,
    &tx_buf,
    &rx_buf);
```

This is the basic setup of the program. See `spi_example.c` to view the rest.

## Step 4: Editing our prj.conf

Prj.conf specifies the software dependencies that we will need, often providing the correct .c and .h files that we used in the api.

```bash
CONFIG_ESP32_SPIM=y
```
1. See test_node/zephyr/drivers/spi/CMakeLists.txt. This is where our driver code is defined
2. There exists `spi_esp32_spim.c`. Looking at the top of the c file, there is `#define DT_DRV_COMPAT espressif_esp32_spi`, which is related to when we declared our SPI controller as `compatible = "espressif,esp32-spi"`. 
3. However, notice in the CMAKE file, `spi_esp32_spim.c` is only defined if `CONFIG_ESP32_SPIM` is defined.
4. Therefore, in our prj.conf, we will add `CONFIG_ESP32_SPIM=y`.

```bash
CONFIG_SPI=y
```
1. How to deduce this? Who knows. It was provided by a tutorial linked in the useful resources section.
2. However, if we don't define this, `esp32` will never be registered upon link-time as a valid spi device. Zephyr will search for `esp32`, but never find it. This is because the spi subsystem doesn't get linked into the final binary. 

We are done!




