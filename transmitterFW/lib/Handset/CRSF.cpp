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

#include "CRSF.h"

#include "common.h"
#include "FIFO.h"

crsfLinkStatistics_t CRSF::LinkStatistics;
GENERIC_CRC8 crsf_crc(CRSF_CRC_POLY);

/***
 * @brief: Convert `version` (string) to a integer version representation
 * e.g. "2.2.15 ISM24G" => 0x0002020f
 * Assumes all version fields are < 256, the number portion
 * MUST be followed by a space to correctly be parsed
 ***/
uint32_t CRSF::VersionStrToU32(const char *verStr)
{
    uint32_t retVal = 0;
    uint8_t accumulator = 0;
    char c;
    bool trailing_data = false;
    while ((c = *verStr))
    {
        ++verStr;
        // A decimal indicates moving to a new version field
        if (c == '.')
        {
            retVal = (retVal << 8) | accumulator;
            accumulator = 0;
            trailing_data = false;
        }
        // Else if this is a number add it up
        else if (c >= '0' && c <= '9')
        {
            accumulator = (accumulator * 10) + (c - '0');
            trailing_data = true;
        }
        // Anything except [0-9.] ends the parsing
        else
        {
            break;
        }
    }
    if (trailing_data)
    {
        retVal = (retVal << 8) | accumulator;
    }
    // If the version ID is < 1.0.0, we could not parse it,
    if (retVal < 0x010000)
    {
        retVal = 0x010000;
    }
    return retVal;
}

void CRSF::GetDeviceInformation(uint8_t *frame, uint8_t fieldCount)
{
    const uint8_t size = strlen(device_name)+1;
    auto *device = (deviceInformationPacket_t *)(frame + sizeof(crsf_ext_header_t) + size);
    // Packet starts with device name
    memcpy(frame + sizeof(crsf_ext_header_t), device_name, size);
    // Followed by the device
    device->serialNo = htobe32(0x454E5458); // ['E', 'N', 'T', 'X']
    device->hardwareVer = 0; // currently unused
    device->softwareVer = htobe32(VersionStrToU32(version)); // Example "Firmware: V 1.15" results to [ 0x00, 0x00, 0x01, 0x0f ]
    device->fieldCnt = fieldCount;
    device->parameterVersion = 0;
}
