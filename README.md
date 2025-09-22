# SlayterHIL

A hardware-in-the-loop test system codesigned with a drone flight controller.

## Setup to work on test node

_Note: If you are on a Windows PC, use WSL2 (look up install instructions, ubuntu recommended) and follow the instructions for linux_

1. Install prerequisites

    - [Python3](https://www.geeksforgeeks.org/python/download-and-install-python-3-latest-version/)

    - Complete **only** the [Install Dependencies](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies) section of the [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#getting-started-guide).

    - [Just command runner](https://github.com/casey/just?tab=readme-ov-file#installation) - This is used like a makefile as a hotkey for all of our project-specific commands
        - If you are using Windows, installing chocolatey package manager is recommended. After installing chocolatey, you can install just by running `choco install just`.
        - If you are on Mac, use the [Homebrew](https://brew.sh/) command `brew install just`.

    - Windows + WSL2: [usbipd-win](https://learn.microsoft.com/en-us/windows/wsl/connect-usb) - This is used to expose the USB ports to WSL2 so that flashing works properly

2. Clone this repository:
    ```bash
    git clone git@github.com:embedded-purdue/slayterHIL.git
    cd slayterHIL
    ```

3. Create and activate a Python virtual environment:
    ```bash
    python -m venv .venv
    
    source .venv/bin/activate     # Linux/macOS
    .venv\Scripts\activate.bat    # PC, Command Prompt
    .venv\Scripts\Activate.ps1    # PC, Powershell
    source .venv/Scripts/activate # PC. Git bash
    ```

4. Install west
    ```bash
    pip install west
    ```

5. Update the workspace
    ```bash
    cd test_node
    west update
    west packages pip --install
    ```

6. Install the Zephyr SDK
    1. navigate to `slayterHIL/test_node/zephyr`
        ```bash
        cd zephyr
        ```
    2.  Perform installation 
        ```bash
        west sdk install
        ```

7. Try to build and run
    1. navigate to `slayterHIL/test_node`
        ```bash
        cd ..
        ```
    2.  Build the sim to verify it works 
        ```bash
        just run-sim
        ```

8. Test flash
    1. navigate to `slayterHIL/test_node/zephyr`
    3. (FOR WSL USERS; SKIP IF NOT) plug in board and use usbipd to connect it
       1. on Powershell (run with admin), run `usbipd list` and find the busid for the MCU

          *example output:*
          ```bash
          Connected:
          BUSID  VID:PID    DEVICE                                                                      STATE
          1-2    10c4:ea60  CP2102N USB to UART Bridge Controller                         Not shared
          1-3    06cb:016c  Synaptics UWP WBDI                                            Not shared
          1-10   8087:0033  Intel(R) Wireless Bluetooth(R)                                Not shared

          Persisted:
          GUID                                  DEVICE
          ```
       2. run `usbipd bind --busid [busid]` for the example above, [busid] is `1-2`. If it works, there should be no output
       3. run `usbipd attach --wsl --busid [busid]`. This will attach the port to WSL.
      
          *example output:*
          ```bash
          usbipd: info: Using WSL distribution 'archlinux' to attach; the device will be available in all WSL 2 distributions.
          usbipd: info: Loading vhci_hcd module.
          usbipd: info: Detected networking mode 'nat'.
          usbipd: info: Using IP address 172.21.112.1 to reach the host.
          ```
          *note: make sure the distro is the one you are using (if you have more than one installed)*
       4. now, in your WSL terminal, run `lsusb` and make sure the USB device shows
           1. if `lsusb` is not installed, you may need to install `usbutils` (look up)
    4. (FOR MAC USERS; SKIP IF NOT) plug in board and run:
       1. ```bash
          ls /dev/ | grep tty
          ```
       2. You should see a list of connected devices. We are looking for something similar to either of the following: 
          ```bash
          tty.usbserial-210
          tty.usbmodem2101
          ```
          usbserial will appear if you're connected to the expressif's UART path, while usbmodem will appear if you're connected to its USB path
       3. Copy the device name (eg. `tty.usbserial-210`)
       4. Navigate to testnode/justfile and replace the value in the `ESP_PORT` line with your device name. It will now be able to communicate to the board using this name.
          ```
          ESP_PORT         := "/dev/tty.usbserial-210"
          ```
    6. now that the USB is connected, test build the `hello_world` sample by running `west build -p always -b esp32s3_devkitc/esp32s3/procpu samples/hello_world`
    7. flash to the usb with `west flash`
       1. if this is the first time attempting to flash after connecting the USB, you may see an error like the following:

          ```bash
          /dev/ttyUSB0 failed to connect: Could not open /dev/ttyUSB0, the port is busy or doesn't exist.
          ([Errno 13] could not open port /dev/ttyUSB0: [Errno 13] Permission denied: '/dev/ttyUSB0')

          Hint: Try to add user into dialout or uucp group.


          A fatal error occurred: Could not connect to an Espressif device on any of the 1 available serial ports.
          FATAL ERROR: command exited with status 2: esptool --baud 921600 --before default-reset --after hard-reset write-flash -u --flash-mode dio --flash-freq 80m --flash-size 8MB 0x0 /home/evinl/es@p/slayterHIL/test_node/zephyr/build/zephyr/zephyr.bin
          ```
       2. to fix this issue, you need add yourself to the correct usergroup:
          1. run `ls -l /dev/[ttyUSB0]` <- ttyUSB0 is whichever port it complained about in the error

             *example output:*
             ```bash
             crw-rw---- 1 root uucp 188, 0 Sep 14 14:14 /dev/ttyUSB0
             ```
          2. the user group you need to add yourself to is after `root`. In the example above, it is `uucp`, but it may also be `dialout` or something else
          3. run `sudo usermod -aG [group] $USER`
          4. run `newgrp [group]`
          5. to check that you are in the group, run `groups $USER`

             *example output:*
             ```bash
             evinl : evinl wheel uucp docker
             ```
    8. The example should now be successfully flashed. In order to check, run `esptool --port /dev/[ttyUSB0] chip-id`. *Note: esptool is automatically installed with zephyr/west (I think). If it is not, install it with `pip`
    9. If everything works, the output should show at the very least the name of the MCU (for the ESP32-S3, it will say something like `Connected to ESP32-S3 on /dev/[ttyUSB0]` and also warn that it has no chip id. This means that the USB port is properly communicating.
    10. You can also run `read-flash` instead of `chip-id` if you want
  
9. Test Debugger
     1. Ensure you are plugged into the UART connection (the debugger)
     2. `just run-esp32`
     3. Once the board is done flashing, press the `reset` button
     4. Your program in app/src/main.c should run and print `Hello World`. 
