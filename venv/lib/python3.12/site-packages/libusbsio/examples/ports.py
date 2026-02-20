#!/usr/bin/env python3
#
# Copyright 2021-2022,2025 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# NXP USBSIO Library - I2C example code
#

import os
import sys
import logging
import logging.config

# add path to local libusbsio package so it can be tested without installing it
addpath = os.path.join(os.path.dirname(__file__), "..")
sys.path.append(addpath)

from libusbsio import *

# enable basic console logging
logging.basicConfig()

# example on how to load DLL from a specific directory
#sio = usbsio(r"..\..\bin_debug\x64", loglevel=logging.DEBUG)

# load DLL from default directory
sio = usbsio()

############################################################################
############################################################################
# the main code

# calling GetNumPorts is mandatory as it also scans for all connected USBSIO devices
numports = sio.GetNumPorts()
print("SIO ports = %d" % numports)

all_devices=[]

if not numports:
    print("No USBSIO device found")

for i in range(0, numports):
    info = sio.GetDeviceInfo(i)

    if sio.Open(i):
        print(f"Instance {i}:")
        if info:
            print(f"    HID product: {info.manufacturer_string} {info.product_string}")
            print(f"    HID serial: {info.serial_number}")
            print(f"    HID path: {info.path}")
            all_devices.insert(0, (i, info.serial_number, info.path) )
        print("    LIB version = '%s'" % sio.GetVersion())
        print("    I2C ports = %d" % sio.GetNumI2CPorts())
        print("    SPI ports = %d" % sio.GetNumSPIPorts())
        print("    Max data size = %d" % sio.GetMaxDataSize())
        sio.Close()

print("\nTest re-open all devices using serial numbers:")
for d in all_devices:
    print(f"Opening serial_number {d[1]} (was device #{d[0]})")
    if sio.OpenBySerial(d[1]):
        print(f"  ...success: {sio.GetVersion()}")
        sio.Close()
    else:
        print(f"  ...failed")

print("\nTest re-open all devices using USB HID paths:")
for d in all_devices:
    print(f"Opening path {d[2]} (was device #{d[0]})")
    if sio.OpenByPath(d[2]):
        print(f"  ...success: {sio.GetVersion()}")
        sio.Close()
    else:
        print(f"  ...failed")

