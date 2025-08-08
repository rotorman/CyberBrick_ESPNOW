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

#include "targets.h"
#include "options.h"

#define QUOTE(arg) #arg
#define STR(macro) QUOTE(macro)
const unsigned char target_name[] = "\xBE\xEF\xCA\xFE" STR(TARGET_NAME);
const uint8_t target_name_size = sizeof(target_name);
const char commit[] {LATEST_COMMIT, 0};
const char version[] = {LATEST_VERSION, 0};

#include <ArduinoJson.h>
#include <StreamString.h>
#include <SPIFFS.h>
#include <esp_partition.h>
#include "esp_ota_ops.h"

char product_name[ELRSOPTS_PRODUCTNAME_SIZE+1];
char device_name[ELRSOPTS_DEVICENAME_SIZE+1];

firmware_options_t firmwareOptions;

// hardware_init prototype here as it is called by options_init()
extern bool hardware_init(EspFlashStream &strmFlash);

static StreamString builtinOptions;
String& getOptions()
{
    return builtinOptions;
}

void saveOptions(Stream &stream, bool customised)
{
    JsonDocument doc;

    doc["customised"] = customised;
    doc["flash-discriminator"] = firmwareOptions.flash_discriminator;

    serializeJson(doc, stream);
}

void saveOptions()
{
    File options = SPIFFS.open("/options.json", "w");
    saveOptions(options, true);
    options.close();
}

/**
 * @brief:  Checks if the strmFlash currently is pointing to something that looks like
 *          a string (not all 0xFF). Position in the stream will not be changed.
 * @return: true if appears to have a string
 */
bool options_HasStringInFlash(EspFlashStream &strmFlash)
{
    uint32_t firstBytes;
    size_t pos = strmFlash.getPosition();
    strmFlash.readBytes((uint8_t *)&firstBytes, sizeof(firstBytes));
    strmFlash.setPosition(pos);

    return firstBytes != 0xffffffff;
}

/**
 * @brief:  Internal read options from the flash stream at the end of the sketch
 *          Fills the firmwareOptions variable
 * @return: true if either was able to be parsed
 */
static void options_LoadFromFlashOrFile(EspFlashStream &strmFlash)
{
    JsonDocument flashDoc;

    // Try OPTIONS JSON at the end of the firmware, after PRODUCTNAME DEVICENAME
    constexpr size_t optionConfigOffset = ELRSOPTS_PRODUCTNAME_SIZE + ELRSOPTS_DEVICENAME_SIZE;
    strmFlash.setPosition(optionConfigOffset);
    if (options_HasStringInFlash(strmFlash))
    {
        DeserializationError error = deserializeJson(flashDoc, strmFlash);
        if (error)
        {
            return;
        }
    }

    JsonDocument &doc = flashDoc;

    firmwareOptions.flash_discriminator = doc["flash-discriminator"] | 0U;

    builtinOptions.clear();
    saveOptions(builtinOptions, doc["customised"] | false);
}

/**
 * @brief:  Initializes product_name / device_name either from flash or static values
 * @return: true if the names came from flash, or false if the values are default
*/
static bool options_LoadProductAndDeviceName(EspFlashStream &strmFlash)
{
    if (options_HasStringInFlash(strmFlash))
    {
        strmFlash.setPosition(0);
        // Product name
        strmFlash.readBytes(product_name, ELRSOPTS_PRODUCTNAME_SIZE);
        product_name[ELRSOPTS_PRODUCTNAME_SIZE] = '\0';
        // Device name
        strmFlash.readBytes(device_name, ELRSOPTS_DEVICENAME_SIZE);
        device_name[ELRSOPTS_DEVICENAME_SIZE] = '\0';

        return true;
    }
    else
    {
        strcpy(product_name, "Unified TX");
        strcpy(device_name, "Unified TX");
        return false;
    }
}

bool options_init()
{
    uint32_t baseAddr = 0;
    SPIFFS.begin(true);
    const esp_partition_t *runningPart = esp_ota_get_running_partition();
    if (runningPart)
    {
        baseAddr = runningPart->address;
    }

    EspFlashStream strmFlash;
    strmFlash.setBaseAddress(baseAddr + ESP.getSketchSize());

    // Product / Device Name
    options_LoadProductAndDeviceName(strmFlash);
    // options.json
    options_LoadFromFlashOrFile(strmFlash);
    // hardware.json
    bool hasHardware = hardware_init(strmFlash);

    return hasHardware;
}
