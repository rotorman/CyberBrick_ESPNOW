#!/usr/bin/python
"""
This file belongs to the CyberBrick ESP-NOW transmitter & receiver project,
hosted originally at:
https://github.com/rotorman/CyberBrick_ESPNOW
Copyright (C) 2025, Risto Koiva

Large parts of the code are based on the wonderful ExpressLRS project:
https://github.com/ExpressLRS/ExpressLRS

License GPL-3.0: https://www.gnu.org/licenses/gpl-3.0.html

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
"""

from enum import Enum
from typing import NamedTuple

class MCUType(Enum):
    ESP32 = 0
    ESP8266 = 1

class DeviceType(Enum):
    TX = 0
    RX = 1
    TX_Backpack = 2
    VRx_Backpack = 3

class RadioType(Enum):
    SX127X = 0
    SX1280 = 1
    LR1121 = 2

class FirmwareOptions(NamedTuple):
    mcuType: MCUType
    deviceType: DeviceType
    radioChip: RadioType
    luaName: str
    bootloader: str
    offset: int
    firmware: str


class TXType(Enum):
    internal = 'internal'
    external = 'external'

    def __str__(self):
        return self.value


def find_patch_location(mm):
    return mm.find(b'\xBE\xEF\xBA\xBE\xCA\xFE\xF0\x0D')

def get_hardware(mm):
    pos = mm.find(b'\xBE\xEF\xBA\xBE\xCA\xFE\xF0\x0D')
    if pos != -1:
        pos += 8 + 2                    # Skip magic & version

    return pos
