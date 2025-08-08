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

#include <stdint.h>
#include <cstddef>

#define RESERVED_EEPROM_SIZE 1024

class ESPNOW_EEPROM
{
public:
    void Begin();
    uint8_t ReadByte(const uint32_t address);
    void WriteByte(const uint32_t address, const uint8_t value);
    void Commit();

    // The extEEPROM lib that we use for STM doesn't have the get and put templates
    // These templates need to be reimplemented here
    template <typename T> void Get(uint32_t addr, T &value)
    {
        uint8_t* p = (uint8_t*)(void*)&value;
        size_t   i = sizeof(value);
        while(i--)  *p++ = ReadByte(addr++);
    };

    template <typename T> const void Put(uint32_t addr, const T &value)
    {
        const uint8_t* p = (const uint8_t*)(const void*)&value;
        size_t         i = sizeof(value);
        while(i--)  WriteByte(addr++, *p++);
    };
};
