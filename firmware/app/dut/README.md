# SlayterHIL DUT 

Zephyr Setup for Reference(https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies)

## Setup & Flash DUT (Ubuntu)

1. Activate python VENV
```bash
    cd slayterHIL #
    source .venv/bin/activate
```

2. Build ESP32 Image
```bash
    cd firmware/zephyr
    west build -p always -b esp32s3_devkitc/esp32s3/procpu ../app/dut 
```

3. Flash and Monitor
```bash 
    west flash
    python -m serial.tools.miniterm /dev/[ttyUSB0] 115200 #replace with your USB port
```
4. If you get the usergroup error
```bash
    ls -l /dev/[ttyUSB0] #or whatever port
    sudo usermod -aG [group] $USER #group likely dialout or uucp
    newgrp [group]
    groups $USER #to check you're in the usergroup
```

5. If you get an error about a missing file like
```bash
    Error: cannot find filepath ~/slayterHIL/test-node/cmake pristine
```
    - remove slayterHIL/firmware/zephyr and then go thru these steps again

```bash
    cd firmware
    west update
    west packages pip --install
    cd zephyr
    west sdk install
```