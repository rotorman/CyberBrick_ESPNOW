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

#include "espnow_eeprom.h"
#include "targets.h"
#include <EEPROM.h>

void
ESPNOW_EEPROM::Begin()
{
    EEPROM.begin(RESERVED_EEPROM_SIZE);
}

uint8_t
ESPNOW_EEPROM::ReadByte(const uint32_t address)
{
    if (address >= RESERVED_EEPROM_SIZE)
    {
        // address is out of bounds
        return 0;
    }
    return EEPROM.read(address);
}

void
ESPNOW_EEPROM::WriteByte(const uint32_t address, const uint8_t value)
{
    if (address >= RESERVED_EEPROM_SIZE)
    {
        // address is out of bounds
        return;
    }
    EEPROM.write(address, value);
}

void
ESPNOW_EEPROM::Commit()
{
    EEPROM.commit();
}
