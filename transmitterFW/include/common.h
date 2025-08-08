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
#include <device.h>

#define GPIO_PIN_BOOT0 0
#define BINDINGTIMEOUTMS 5000

typedef enum
{
    RECEIVERCONNECTED,
    AWAITINGMODELIDFROMHANDSET,
    RECEIVERDISCONNECTED,
    MODE_STATES,
    // States below here are special mode states
    noHandsetCommunication,
    // Failure states go below here to display immediately
    FAILURE_STATES,
    radioFailed,
    hardwareUndefined
} connectionState_e;

// Limited to 16 possible ACTIONs by config storage currently
typedef enum : uint8_t {
    ACTION_NONE,
    ACTION_BIND,
    ACTION_RESET_REBOOT,
    ACTION_LAST
} action_e;

enum eAuxChannels : uint8_t
{
    AUX1 = 4,
    AUX2 = 5,
    AUX3 = 6,
    AUX4 = 7,
    AUX5 = 8,
    AUX6 = 9,
    AUX7 = 10,
    AUX8 = 11,
    AUX9 = 12,
    AUX10 = 13,
    AUX11 = 14,
    AUX12 = 15,
    CRSF_NUM_CHANNELS = 16
};

extern bool connectionHasModelMatch;
extern bool InBindingMode;
extern volatile uint16_t ChannelData[CRSF_NUM_CHANNELS]; // Current state of channels, CRSF format

extern connectionState_e connectionState;
inline void setConnectionState(connectionState_e newState) {
    connectionState = newState;
    devicesTriggerEvent(EVENT_CONNECTION_CHANGED);
}

extern void EnterBindingMode(); // defined in main.c
