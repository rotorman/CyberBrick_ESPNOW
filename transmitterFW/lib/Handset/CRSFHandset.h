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

#include "handset.h"
#include "crsf_protocol.h"
#include "HardwareSerial.h"
#include "common.h"
#include "driver/uart.h"

class CRSFHandset final : public Handset
{

public:
    /////Variables/////
    void Begin() override;
    void End() override;

    void handleInput() override;
    void handleOutput(int receivedBytes);

    static HardwareSerial Port;

    static uint8_t modelId;         // The model ID as received from the Transmitter
    static bool elrsLUAmode;

    static uint32_t GoodPktsCountResult; // need to latch the results
    static uint32_t BadPktsCountResult;  // need to latch the results

    static void makeLinkStatisticsPacket(uint8_t *buffer);

    static void packetQueueExtended(uint8_t type, void *data, uint8_t len);

    void setPacketIntervalUS(int32_t PacketIntervalUS) override;
    void JustSentRFpacket() override;
    void sendTelemetryToTX(uint8_t *data) override;

    static uint8_t getModelID() { return modelId; }

    uint8_t GetMaxPacketBytes() const override { return maxPacketBytes; }
    static uint32_t GetCurrentBaudRate() { return UARTrequestedBaud; }
    static bool isHalfDuplex() { return halfDuplex; }
    int getMinPacketInterval() const override;

private:
    inBuffer_U inBuffer = {};

    /// EdgeTX mixer sync ///
    volatile uint32_t dataLastRecv = 0;
    volatile int32_t EdgeTXsyncOffset100NS = 0;
    volatile int32_t EdgeTXsyncWindowUS = 0;
    volatile int32_t EdgeTXsyncWindowSizeUS = 0;
    uint32_t EdgeTXsyncLastSentMS = 0;

    /// UART Handling ///
    uint8_t SerialInPacketPtr = 0; // index where we are reading/writing
    static bool halfDuplex;
    bool transmitting = false;
    uint32_t GoodPktsCount = 0;
    uint32_t BadPktsCount = 0;
    uint32_t UARTwdtLastCheckedMS = 0;
    uint8_t maxPacketBytes = CRSF_MAX_PACKET_LEN;
    uint8_t maxPeriodBytes = CRSF_MAX_PACKET_LEN;

    static uint8_t UARTcurrentBaudIdx;
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

