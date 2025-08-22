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
#include <Arduino.h>

#undef ICACHE_RAM_ATTR //fix to allow both esp32 and esp8266 to use ICACHE_RAM_ATTR for mapping to IRAM
#define ICACHE_RAM_ATTR IRAM_ATTR

#define GPIO_PIN_BOOT0 0
#define CRSF_NUM_CHANNELS 32U
#define RF_FRAME_RATE_US 20000U // 50 Hz

typedef enum
{
    awatingFirstPacket,
    awaitingModelId,
    connected,
    disconnected
} connectionState_e;

extern connectionState_e connectionState;

inline void setConnectionState(connectionState_e newState) {
    connectionState = newState;
}

extern volatile uint16_t ChannelData[CRSF_NUM_CHANNELS]; // Current state of channels, CRSF format
