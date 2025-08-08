/*
 * This file belongs to the CyberBrick ESP-NOW transmitter & receiver project, hosted originally at:
 * https://github.com/rotorman/CyberBrick_ESPNOW
 * Copyright (C) 2025, Risto Kõiva
 *
 * Large parts of the code are based on the wonderful ExpressLRS project:
 * https://github.com/ExpressLRS/ExpressLRS
 *
 * License GPL-3.0: https://www.gnu.org/licenses/gpl-3.0.html
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

#include "targets.h"

extern const unsigned char target_name[];
extern const uint8_t target_name_size;
extern const char commit[];
extern const char version[];

typedef struct _options {
    uint8_t     _magic_[8];     // this is the magic constant so the configurator can find this options block
    uint16_t    _version_;      // the version of this structure
    uint32_t    flash_discriminator;    // Discriminator value used to determine if the device has been reflashed and therefore
                                        // the SPIFSS settings are obsolete and the flashed settings should be used in preference
} __attribute__((packed)) firmware_options_t;

// Layout is PRODUCTNAME DEVICENAME OPTIONS HARDWARE
constexpr size_t ELRSOPTS_PRODUCTNAME_SIZE = 128;
constexpr size_t ELRSOPTS_DEVICENAME_SIZE = 16;
constexpr size_t ELRSOPTS_OPTIONS_SIZE = 512;
constexpr size_t ELRSOPTS_HARDWARE_SIZE = 2048;

extern firmware_options_t firmwareOptions;
extern bool options_init();

extern char product_name[];
extern char device_name[];
extern String& getOptions();
extern String& getHardware();
extern void saveOptions();

#include "EspFlashStream.h"
bool options_HasStringInFlash(EspFlashStream &strmFlash);
