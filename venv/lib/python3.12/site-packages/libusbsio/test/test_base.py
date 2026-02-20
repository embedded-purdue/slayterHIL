#!/usr/bin/env python3
#
# Copyright 2021,2025 NXP
# SPDX-License-Identifier: BSD-3-Clause
#
# TEST CODE of NXP USBSIO Library - main test entry point
#

import argparse
import functools
import logging
import os
import sys
import unittest

import pytest

# add path to local libusbsio package so it can be tested without installing it
addpath = os.path.join(os.path.dirname(__file__), "..")
sys.path.append(addpath)

from libusbsio import *

# enable basic console logging
LOGLEVEL = logging.INFO

# device dependent settings
(T_LPCLINK2, T_MCULINK_PRO, T_MCULINK_55S36) = (1, 2, 3)
# TARGET = T_MCULINK_PRO
# TARGET = T_MCULINK_55S36
TARGET = T_LPCLINK2

# Use port at specific index
ENFORCE_PORTINDEX = 0

# Enforce SERIAL NUMBER (will use OpenBySerial)
#ENFORCE_SERIAL="MRA1AQJR"  # Example serial number format
ENFORCE_SERIAL= None

# Enforce HID PATH (will use OpenByPath)
#ENFORCE_HIDPATH=br'\\?\hid#vid_1fc9&pid_0090&mi_03#7&170a3cfb&0&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}'
ENFORCE_HIDPATH=None

# Use alternate DLL path if needed
#ENFORCE_DLLPATH = r"..\bin_debug\x64" # Example alternate DLL path
ENFORCE_DLLPATH = None

# VID/PID settings for specific targets
if TARGET == T_LPCLINK2:
    VIDPIDS = [LIBUSBSIO.VIDPID_LPCLINK2]
elif TARGET >= T_MCULINK_PRO:
    VIDPIDS = [LIBUSBSIO.VIDPID_MCULINK]
else:
    raise Exception("You must set testing TARGET!")

# Enumerate both LPC-Link2 and MCU-Link VID/PID combinations
# note that this will only work with ENFORCE_SERIAL or ENFORCE_HIDPATH set
# VIDPIDS = [LIBUSBSIO.VIDPID_ALL_NXP]

# enable to force logging to console
logging.basicConfig()

# change to enable tests which are normally skipped
RUN_SLOW_TESTS = 0
RUN_KNOWN_ISSUES = 0

# only one of the following ENFORCE options should be active
if (bool(ENFORCE_PORTINDEX) + bool(ENFORCE_SERIAL) + bool(ENFORCE_HIDPATH)) > 1:
    raise Exception(f"Only one ENFORCE option can be active at a time")

def known_issue(func):
    """Decorator to mark known-issues failing tests"""

    @functools.wraps(func)
    def wrapper(self, *args, **kwargs):
        if RUN_KNOWN_ISSUES:
            self.logger.warning("This test is marked as @known_issue")
            return func(self, *args, **kwargs)
        else:
            self.skipTest("Test marked as @known_issue")

    return wrapper


class TestBase(unittest.TestCase):
    def setUp(self):
        self.logger = logging.getLogger("test")
        self.logger.setLevel(LOGLEVEL)
        self.sio: LIBUSBSIO = LIBUSBSIO(dllpath=ENFORCE_DLLPATH,loglevel=LOGLEVEL)
        self.spi: LIBUSBSIO.SPI = None
        self.i2c: LIBUSBSIO.I2C = None

    def tearDown(self):
        if self.spi:
            del self.spi
            self.spi = None
        if self.i2c:
            del self.i2c
            self.i2c = None
        if self.sio:
            del self.sio
            self.sio = None

    def OpenDefaultPort(self):
        ok = self.sio.IsOpen()
        if not ok:
            if ENFORCE_HIDPATH:
                # If ENFORCE_HIDPATH is set, use OpenByPath method
                ok = self.sio.OpenByPath(ENFORCE_HIDPATH)
                self.assertTrue(ok, "OpenByPath succeeded")
            else:
                # other Open methods need HID enumeration
                ports = self.sio.GetNumPorts(vidpids=VIDPIDS)
                self.assertTrue(ports > 0, "After enumeration, we have at least one port to open")
                if ports > 0:
                    if ENFORCE_SERIAL:
                        # If ENFORCE_SERIAL is set, use OpenBySerial method
                        ok = self.sio.OpenBySerial(ENFORCE_SERIAL)
                        self.assertTrue(ok, "OpenBySerial succeeded")
                    else:
                        # If no specific path or serial is enforced, try opening the specified
                        ok = self.sio.Open(ENFORCE_PORTINDEX)
                        self.assertTrue(ok, f"Open at index {ENFORCE_PORTINDEX} succeeded")
        if ok:
            self.assertTrue(self.sio.IsOpen())
        return ok

if __name__ == "__main__":
    unittest.main()
