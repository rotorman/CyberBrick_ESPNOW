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

#ifndef H_CRSF
#define H_CRSF

#include "crsf_protocol.h"

class CRSF
{
public:
    static crsfLinkStatistics_t LinkStatistics; // Link Statistics Stored as Struct
    static void GetDeviceInformation(uint8_t *frame, uint8_t fieldCount);
    static uint32_t VersionStrToU32(const char *verStr);
};

extern GENERIC_CRC8 crsf_crc;

#endif