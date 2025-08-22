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

#include "crsf_protocol.h"
#include "HardwareSerial.h"
#include "common.h"
#include "driver/uart.h"

class CRSFHandset final
{
public:
    /**
     * @brief Start the CRSF handset protocol
     */
    void Begin();

    /**
     * @brief register a function to be called when the protocol has read an RC data packet from the handset
     * @param callback
     */
    void setRCDataCallback(void (*callback)()) { RCdataCallback = callback; }

    /**
     * Register callback functions for state information about the connection or handset
     * @param connectedCallback called when the protocol detects a stable connection to the handset
     * @param disconnectedCallback called when the protocol loses its connection to the handset
     * @param RecvModelUpdateCallback called when the handset sends a message to set the current model number
     */
    void registerCallbacks(void (*connectedCallback)(), void (*disconnectedCallback)(), void (*RecvModelUpdateCallback)())
    {
        connected = connectedCallback;
        disconnected = disconnectedCallback;
        RecvModelUpdate = RecvModelUpdateCallback;
    }

    /**
     * @brief Process any pending input data from the CRSF handset
     */
    void handleInput();

	void handleOutput(int receivedBytes);

	static HardwareSerial Port;
	
	static uint8_t modelId;         // The model ID as received from the handset
	
    static void makeLinkStatisticsPacket(uint8_t *buffer);

    static void packetQueueExtended(uint8_t type, void *data, uint8_t len);
	
    /**
     * @return the maximum number of bytes that the protocol can send to the handset in a single message
     */
    uint8_t GetMaxPacketBytes() const { return maxPacketBytes; }

    /**
     * Depending on the baud-rate selected and the module type (full/half duplex) will determine the minimum
     * supported packet interval.
     * @return the minimum interval between packets supported by the current configuration.
     */
    int getMinPacketInterval() const;

    /**
     * @brief Called to indicate to the protocol that a packet has just been sent over-the-air
     * This is used to synchronise the packets from the handset to the OTA protocol to minimise latency
     */
    void JustSentRFpacket();

    /**
     * Send a telemetry packet back to the handset
     * @param data
     */
	void sendTelemetryToTX(uint8_t *data);

    static uint8_t getModelID() { return modelId; }

    /**
     * @return the time in microseconds when the last RC packet was received from the handset
     */
    uint32_t GetRCdataLastRecv() const { return RCdataLastRecv; }

	static uint32_t GetCurrentBaudRate() { return UARTrequestedBaud; }
    static bool isHalfDuplex() { return halfDuplex; }
	
private:
    bool controllerConnected = false;
    void (*RCdataCallback)() = nullptr;  // called when there is new RC data
    void (*disconnected)() = nullptr;    // called when RC packet stream is lost
    void (*connected)() = nullptr;       // called when RC packet stream is regained
    void (*RecvModelUpdate)() = nullptr; // called when model id changes, ie command from Radio

    volatile uint32_t RCdataLastRecv = 0;
    int32_t RequestedRCpacketIntervalUS = RF_FRAME_RATE_US;

    inBuffer_U inBuffer = {};

    /// EdgeTX mixer sync ///
    volatile uint32_t dataLastRecv = 0;
    volatile int32_t EdgeTXsyncOffset = 0;
    volatile int32_t EdgeTXsyncWindow = 0;
    volatile int32_t EdgeTXsyncWindowSize = 1;
    uint32_t EdgeTXsyncLastSent = 0;

    /// UART Handling ///
    uint8_t SerialInPacketPtr = 0; // index where we are reading/writing
    static bool halfDuplex;
    bool transmitting = false;
    uint32_t GoodPktsCount = 0;
    uint32_t BadPktsCount = 0;
    uint32_t UARTwdtLastChecked = 0;
    uint8_t maxPacketBytes = CRSF_MAX_PACKET_LEN;
    uint8_t maxPeriodBytes = CRSF_MAX_PACKET_LEN;

    static uint32_t UARTrequestedBaud;
    bool UARTinverted = false;
    void sendSyncPacketToTX();
    void adjustMaxPacketSize();
    void duplex_set_RX() const;
    void duplex_set_TX() const;
    void RcPacketToChannelsData(bool bExtendedChannels);
    bool processInternalCrsfPackage(uint8_t *package);
    void alignBufferToSync(uint8_t startIdx);
    bool ProcessPacket();
    bool UARTwdt();
    uint32_t autobaud();	
    void flush_port_input();
};

