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
#include "targets.h"

#define crclen 256

class GENERIC_CRC8
{
private:
    uint8_t crc8tab[crclen];
    uint8_t crcpoly;

public:
    GENERIC_CRC8(uint8_t poly);
    uint8_t calc(const uint8_t data);
    uint8_t calc(const uint8_t *data, uint16_t len, uint8_t crc = 0);
};
