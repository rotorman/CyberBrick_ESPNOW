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

/**
 * @brief Abstract class that is extended to provide an interface to a handset.
 *
 */
class Handset
{
public:
    /**
     * @brief Start the handset protocol
     */
    virtual void Begin() = 0;

    /**
     * @brief End the handset protocol
     */
    virtual void End() = 0;

    /**
     * @brief register a function to be called when the protocol has read an RC data packet from the handset
     * @param callback
     */
    void setRCDataCallback(void (*callback)()) { RCdataCallback = callback; }
    /**
     * @brief register a function to be called when a request to update a parameter is sent from the handset
     * @param callback
     */
    void registerParameterUpdateCallback(void (*callback)(uint8_t type, uint8_t index, uint8_t arg)) { RecvParameterUpdate = callback; }
    /**
     * Register callback functions for state information about the connection or handset
     * @param connectedCallback called when the protocol detects a stable connection to the handset
     * @param disconnectedCallback called when the protocol loses its connection to the handset
     * @param RecvModelUpdateCallback called when the handset sends a message to set the current model number
     */
    void registerCallbacks(void (*connectedCallback)(), void (*disconnectedCallback)(), void (*RecvModelUpdateCallback)(), void (*bindingCommandCallback)())
    {
        connected = connectedCallback;
        disconnected = disconnectedCallback;
        RecvModelUpdate = RecvModelUpdateCallback;
        OnBindingCommand = bindingCommandCallback;
    }

    /**
     * @brief Process any pending input data from the handset
     */
    virtual void handleInput() = 0;

    /**
     * Called to set the expected packet interval from the handset.
     * This can be used to synchronise the packets from the handset.
     * @param PacketIntervalUS in microseconds
     */
    virtual void setPacketIntervalUS(int32_t PacketIntervalUS) { RequestedRCpacketIntervalUS = PacketIntervalUS; }

    /**
     * @return the maximum number of bytes that the protocol can send to the handset in a single message
     */
    virtual uint8_t GetMaxPacketBytes() const { return 255; }

    /**
     * Depending on the baud-rate selected and the module type (full/half duplex) will determine the minimum
     * supported packet interval.
     * @return the minimum interval between packets supported by the current configuration.
     */
    virtual int getMinPacketInterval() const { return 1; }

    /**
     * @brief Called to indicate to the protocol that a packet has just been sent over-the-air
     * This is used to synchronise the packets from the handset to the OTA protocol to minimise latency
     */
    virtual void JustSentRFpacket() {}
    /**
     * Send a telemetry packet back to the handset
     * @param data
     */
    virtual void sendTelemetryToTX(uint8_t *data) {}

    /**
     * @return the time in microseconds when the last RC packet was received from the handset
     */
    uint32_t GetRCdataLastRecvUS() const { return RCdataLastRecvUS; }

protected:
    virtual ~Handset() = default;

    bool controllerConnected = false;
    void (*RCdataCallback)() = nullptr;  // called when there is new RC data
    void (*disconnected)() = nullptr;    // called when RC packet stream is lost
    void (*connected)() = nullptr;       // called when RC packet stream is regained
    void (*RecvModelUpdate)() = nullptr; // called when model id changes, ie command from Radio
    void (*RecvParameterUpdate)(uint8_t type, uint8_t index, uint8_t arg) = nullptr; // called when recv parameter update req, ie from LUA
    void (*OnBindingCommand)() = nullptr; // Called when a binding command is received

    volatile uint32_t RCdataLastRecvUS = 0;
    int32_t RequestedRCpacketIntervalUS = 5000; // default to 200 Hz as per 'normal'
};

extern Handset *handset;
