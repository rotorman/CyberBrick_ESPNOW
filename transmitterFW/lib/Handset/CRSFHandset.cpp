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
#include "CRSFHandset.h"
#include "FIFO.h"
#include "helpers.h"
#include "device.h"

#include <hal/uart_ll.h>
#include <soc/soc.h>
#include <soc/uart_reg.h>
#include <rom/gpio.h>

// UART0 is used since for full duplex connection we can connect directly through IO_MUX and not the Matrix
// for better performance, and on other targets (mostly using pin 13), it always uses Matrix
HardwareSerial CRSFHandset::Port(0);
RTC_DATA_ATTR int rtcModelId = 0;

static constexpr int HANDSET_TELEMETRY_FIFO_SIZE = 128; // this is the smallest telemetry FIFO size in EdgeTX with CRSF defined

/// Out FIFO to buffer messages ///
static constexpr auto CRSF_SERIAL_OUT_FIFO_SIZE = 256U;
static FIFO<CRSF_SERIAL_OUT_FIFO_SIZE> SerialOutFIFO;

/// UART Handling ///
uint32_t CRSFHandset::GoodPktsCountResult = 0;
uint32_t CRSFHandset::BadPktsCountResult = 0;

uint8_t CRSFHandset::modelId = 0;
bool CRSFHandset::elrsLUAmode = false;
bool CRSFHandset::halfDuplex = false;

/// EdgeTX mixer sync ///
static const int32_t EdgeTXsyncPacketIntervalMS = 200; // in ms
static const int32_t EdgeTXsyncOffsetSafeMargin100NS = 1000; // 100us

/// UART Handling ///
static const int32_t TxToHandsetBauds[] = {400000, 115200, 5250000, 3750000, 1870000, 921600, 2250000};
uint8_t CRSFHandset::UARTcurrentBaudIdx = 6;   // only used for baud-cycling, initialized to the end so the next one we try is the first in the list
uint32_t CRSFHandset::UARTrequestedBaud = 5250000;

// for the UART wdt, every 1000ms we change bauds when connect is lost
static const int UARTwdtIntervalMS = 1000;

void CRSFHandset::Begin()
{
    UARTwdtLastCheckedMS = millis() + UARTwdtIntervalMS; // allows a delay before the first time the UARTwdt() function is called

    #if not defined(GPIO_PIN_RCSIGNAL_RX) || not defined(GPIO_PIN_RCSIGNAL_TX)
        #error "GPIO_PIN_RCSIGNAL_RX and GPIO_PIN_RCSIGNAL_TX must be defined for the RF module to be able to talk to the handset"
    #endif

    halfDuplex = (GPIO_PIN_RCSIGNAL_TX == GPIO_PIN_RCSIGNAL_RX);

    portDISABLE_INTERRUPTS();
    UARTinverted = halfDuplex; // on a half duplex UART, go with inverted
    CRSFHandset::Port.begin(UARTrequestedBaud, SERIAL_8N1,
                     GPIO_PIN_RCSIGNAL_RX, GPIO_PIN_RCSIGNAL_TX,
                     false, 0);
    // Arduino defaults every ESP32 stream to a 1000ms timeout, need to explicitly override this
    CRSFHandset::Port.setTimeout(0);
    if (halfDuplex)
    {
        duplex_set_RX();
    }
    portENABLE_INTERRUPTS();
    flush_port_input();
    if (esp_reset_reason() != ESP_RST_POWERON)
    {
        modelId = rtcModelId;
        if (RecvModelUpdate) RecvModelUpdate();
    }
}

void CRSFHandset::End()
{
    uint32_t startTime = millis();
    while (SerialOutFIFO.peek() > 0)
    {
        handleInput();
        if (millis() - startTime > 1000)
        {
            break;
        }
    }
}

void CRSFHandset::flush_port_input()
{
    // Make sure there is no garbage on the UART at the start
    while (CRSFHandset::Port.available())
    {
        CRSFHandset::Port.read();
    }
}

void CRSFHandset::makeLinkStatisticsPacket(uint8_t *buffer)
{
    constexpr uint8_t payloadLen = sizeof(crsfLinkStatistics_t);

    buffer[0] = CRSF_ADDRESS_RADIO_TRANSMITTER;
    buffer[1] = CRSF_FRAME_SIZE(payloadLen);
    buffer[2] = CRSF_FRAMETYPE_LINK_STATISTICS;
    memcpy(&buffer[3], (uint8_t *)&CRSF::LinkStatistics, payloadLen);
    buffer[payloadLen + 3] = crsf_crc.calc(&buffer[2], payloadLen + 1);
}

/**
 * Build a an extended type packet and queue it in the SerialOutFIFO
 * This is just a regular packet with 2 extra bytes with the sub src and target
 **/
void CRSFHandset::packetQueueExtended(uint8_t type, void *data, uint8_t len)
{
    uint8_t buf[6] = {
        (uint8_t)(len + 6),
        CRSF_ADDRESS_RADIO_TRANSMITTER,
        (uint8_t)(len + 4),
        type,
        CRSF_ADDRESS_RADIO_TRANSMITTER,
        CRSF_ADDRESS_CRSF_TRANSMITTER
    };

    // CRC - Starts at type, ends before CRC
    uint8_t crc = crsf_crc.calc(&buf[3], sizeof(buf)-3);
    crc = crsf_crc.calc((byte *)data, len, crc);

    SerialOutFIFO.lock();
    if (SerialOutFIFO.ensure(buf[0] + 1))
    {
        SerialOutFIFO.pushBytes(buf, sizeof(buf));
        SerialOutFIFO.pushBytes((byte *)data, len);
        SerialOutFIFO.push(crc);
    }
    SerialOutFIFO.unlock();
}

void CRSFHandset::sendTelemetryToTX(uint8_t *data)
{
    if (controllerConnected)
    {
        uint8_t size = CRSF_FRAME_SIZE(data[CRSF_TELEMETRY_LENGTH_INDEX]);
        if (size > CRSF_MAX_PACKET_LEN)
        {
            // Error - too much data, does not fit
            return;
        }

        data[0] = CRSF_ADDRESS_RADIO_TRANSMITTER;
        SerialOutFIFO.lock();
        if (SerialOutFIFO.ensure(size + 1))
        {
            SerialOutFIFO.push(size); // length
            SerialOutFIFO.pushBytes(data, size);
        }
        SerialOutFIFO.unlock();
    }
}

void ICACHE_RAM_ATTR CRSFHandset::setPacketIntervalUS(int32_t PacketIntervalUS)
{
    RequestedRCpacketIntervalUS = PacketIntervalUS;
    EdgeTXsyncOffset100NS = 0;
    EdgeTXsyncWindowUS = 0;
    EdgeTXsyncWindowSizeUS = std::max((int32_t)1, (int32_t)(20000/RequestedRCpacketIntervalUS));
    EdgeTXsyncLastSentMS -= EdgeTXsyncPacketIntervalMS;
    adjustMaxPacketSize();
}

void ICACHE_RAM_ATTR CRSFHandset::JustSentRFpacket()
{
    // read them in this order to prevent a potential race condition
    uint32_t last = dataLastRecv;
    uint32_t m = micros();
    auto delta = (int32_t)(m - last);

    if (delta >= (int32_t)RequestedRCpacketIntervalUS)
    {
        // missing/late packet, force resync
        EdgeTXsyncOffset100NS = -(delta % RequestedRCpacketIntervalUS) * 10;
        EdgeTXsyncWindowUS = 0;
        EdgeTXsyncLastSentMS -= EdgeTXsyncPacketIntervalMS;
    }
    else
    {
        // The number of packets in the sync window is how many will fit in 20ms.
        // This gives quite quite coarse changes for 50Hz, but more fine grained changes at 1000Hz.
        EdgeTXsyncWindowUS = std::min(EdgeTXsyncWindowUS + 1, (int32_t)EdgeTXsyncWindowSizeUS);
        EdgeTXsyncOffset100NS = ((EdgeTXsyncOffset100NS * (EdgeTXsyncWindowUS-1)) + delta * 10) / EdgeTXsyncWindowUS;
    }
}

void CRSFHandset::sendSyncPacketToTX()
{
    uint32_t now = millis();
    if (controllerConnected && (now - EdgeTXsyncLastSentMS) >= EdgeTXsyncPacketIntervalMS)
    {
        int32_t packetRate100US = RequestedRCpacketIntervalUS * 10; //convert from us to right format
        int32_t offset = EdgeTXsyncOffset100NS - EdgeTXsyncOffsetSafeMargin100NS; // offset so that EdgeTX always has some headroom

        struct etxSyncData {
            uint8_t subType; // CRSF_HANDSET_SUBCMD_TIMING
            uint32_t rate; // Big-Endian
            uint32_t offset; // Big-Endian
        } PACKED;

        uint8_t buffer[sizeof(etxSyncData)];
        auto * const sync = (struct etxSyncData * const)buffer;

        sync->subType = CRSF_HANDSET_SUBCMD_TIMING;
        sync->rate = htobe32(packetRate100US);
        sync->offset = htobe32(offset);

        packetQueueExtended(CRSF_FRAMETYPE_HANDSET, buffer, sizeof(buffer));

        EdgeTXsyncLastSentMS = now;
    }
}

void CRSFHandset::RcPacketToChannelsData(bool bExtendedChannels) // data is packed as 11 bits per channel
{
    auto payload = (uint8_t const * const)&inBuffer.asRCPacket_t.channels;
    constexpr unsigned srcBits = 11;
    constexpr unsigned dstBits = 11;
    constexpr unsigned inputChannelMask = (1 << srcBits) - 1;
    constexpr unsigned precisionShift = dstBits - srcBits;

    // code from BetaFlight rx/crsf.cpp / bitpacker_unpack
    uint8_t bitsMerged = 0;
    uint32_t readValue = 0;
    unsigned readByteIndex = 0;
    for (uint8_t n=0; n<16; n++)
    {
        while (bitsMerged < srcBits)
        {
            uint8_t readByte = payload[readByteIndex++];
            readValue |= ((uint32_t) readByte) << bitsMerged;
            bitsMerged += 8;
        }
        ChannelData[(bExtendedChannels) ? n+16 : n] = (uint16_t)((readValue & inputChannelMask) << precisionShift);
        readValue >>= srcBits;
        bitsMerged -= srcBits;
    }

    // Call the registered RCdataCallback, if there is one, so it can modify the channel data if it needs to.
    if (RCdataCallback) RCdataCallback();
}

bool CRSFHandset::processInternalCrsfPackage(uint8_t *package)
{
    const crsf_ext_header_t *header = (crsf_ext_header_t *)package;
    const crsf_frame_type_e packetType = (crsf_frame_type_e)header->type;

    // Enter Binding Mode
    if (packetType == CRSF_FRAMETYPE_COMMAND
        && header->frame_size >= 6 // official CRSF is 7 bytes with two CRCs
        && header->dest_addr == CRSF_ADDRESS_CRSF_TRANSMITTER
        && header->orig_addr == CRSF_ADDRESS_RADIO_TRANSMITTER
        && header->payload[0] == CRSF_COMMAND_SUBCMD_RX
        && header->payload[1] == CRSF_COMMAND_SUBCMD_RX_BIND)
    {
        if (OnBindingCommand) OnBindingCommand();
        return true;
    }

    if (packetType >= CRSF_FRAMETYPE_DEVICE_PING &&
        (header->dest_addr == CRSF_ADDRESS_CRSF_TRANSMITTER || header->dest_addr == CRSF_ADDRESS_BROADCAST) &&
        (header->orig_addr == CRSF_ADDRESS_RADIO_TRANSMITTER || header->orig_addr == CRSF_ADDRESS_ELRS_LUA))
    {
        elrsLUAmode = header->orig_addr == CRSF_ADDRESS_ELRS_LUA; // 0xEF

        if (packetType == CRSF_FRAMETYPE_COMMAND && header->payload[0] == CRSF_COMMAND_SUBCMD_RX && header->payload[1] == CRSF_COMMAND_MODEL_SELECT_ID)
        {
            modelId = header->payload[2];
            rtcModelId = modelId;
            if (RecvModelUpdate) RecvModelUpdate();
        }
        else
        {
            if (RecvParameterUpdate) RecvParameterUpdate(packetType, header->payload[0], header->payload[1]);
        }

        return true;
    }

    return false;
}

bool CRSFHandset::ProcessPacket()
{
    bool packetReceived = false;

    CRSFHandset::dataLastRecv = micros();

    if (!controllerConnected)
    {
        // CRSF UART Connected
        controllerConnected = true;
        if (connected) connected();
    }

    const uint8_t packetType = inBuffer.asRCPacket_t.header.type;
    uint8_t *SerialInBuffer = inBuffer.asUint8_t;

    if (packetType == CRSF_FRAMETYPE_RC_CHANNELS_PACKED)
    {
        RCdataLastRecvUS = micros();
        RcPacketToChannelsData(false);
        packetReceived = true;
    }
    else if (packetType == CRSF_FRAMETYPE_RC_EXTENDED_CHANNELS_PACKED)
    {
        RCdataLastRecvUS = micros();
        RcPacketToChannelsData(true);
        packetReceived = true;
    }
    // check for all extended frames that are a broadcast or a message to the FC
    else if (packetType >= CRSF_FRAMETYPE_DEVICE_PING &&
            (SerialInBuffer[3] == CRSF_ADDRESS_FLIGHT_CONTROLLER || SerialInBuffer[3] == CRSF_ADDRESS_BROADCAST || SerialInBuffer[3] == CRSF_ADDRESS_CRSF_RECEIVER))
    {
        packetReceived = true;
    }

    packetReceived |= processInternalCrsfPackage(SerialInBuffer);

    return packetReceived;
}

void CRSFHandset::alignBufferToSync(uint8_t startIdx)
{
    uint8_t *SerialInBuffer = inBuffer.asUint8_t;

    for (unsigned int i=startIdx ; i<SerialInPacketPtr ; i++)
    {
        // If we find a header byte then move that and trailing bytes to the head of the buffer and let's go!
        if (SerialInBuffer[i] == CRSF_ADDRESS_CRSF_TRANSMITTER || SerialInBuffer[i] == CRSF_SYNC_BYTE)
        {
            SerialInPacketPtr -= i;
            memmove(SerialInBuffer, &SerialInBuffer[i], SerialInPacketPtr);
            return;
        }
    }

    // If no header found then discard this entire buffer
    SerialInPacketPtr = 0;
}

void CRSFHandset::handleInput()
{
    uint8_t *SerialInBuffer = inBuffer.asUint8_t;

    if (UARTwdt())
    {
        return;
    }

    if (transmitting)
    {
        // if currently transmitting in half-duplex mode then check if the TX buffers are empty.
        // If there is still data in the transmit buffers then exit, and we'll check next go round.
        if (!uart_ll_is_tx_idle(UART_LL_GET_HW(0)))
        {
            return;
        }
        // All done transmitting; go back to receive mode
        transmitting = false;
        duplex_set_RX();
        flush_port_input();
    }

    // Add new data, and then discard bytes until we start with header byte
    auto toRead = std::min(CRSFHandset::Port.available(), CRSF_MAX_PACKET_LEN - SerialInPacketPtr);
    SerialInPacketPtr += CRSFHandset::Port.readBytes(&SerialInBuffer[SerialInPacketPtr], toRead);
    alignBufferToSync(0);

    // Make sure we have at least a packet header and a length byte
    if (SerialInPacketPtr < 3)
        return;

    // Sanity check: A total packet must be at least [sync][len][type][crc] (if no payload) and at most CRSF_MAX_PACKET_LEN
    const uint32_t totalLen = SerialInBuffer[1] + 2;
    if (totalLen < 4 || totalLen > CRSF_MAX_PACKET_LEN)
    {
        // Start looking for another packet after this start byte
        alignBufferToSync(1);
        return;
    }

    // Only proceed one there are enough bytes in the buffer for the entire packet
    if (SerialInPacketPtr < totalLen)
        return;

    uint8_t CalculatedCRC = crsf_crc.calc(&SerialInBuffer[2], totalLen - 3);
    if (CalculatedCRC == SerialInBuffer[totalLen - 1])
    {
        GoodPktsCount++;
        if (ProcessPacket())
        {
            handleOutput(totalLen);
            if (RCdataCallback)
            {
                RCdataCallback();
            }
        }
    }
    else
    {
        // UART CRC failure
        BadPktsCount++;
    }

    SerialInPacketPtr -= totalLen;
    memmove(SerialInBuffer, &SerialInBuffer[totalLen], SerialInPacketPtr);
}

void CRSFHandset::handleOutput(int receivedBytes)
{
    static uint8_t CRSFoutBuffer[CRSF_MAX_PACKET_LEN] = {0};
    // both static to split up larger packages
    static uint8_t packageLengthRemaining = 0;
    static uint8_t sendingOffset = 0;

    if (!controllerConnected)
    {
        SerialOutFIFO.lock();
        SerialOutFIFO.flush();
        SerialOutFIFO.unlock();
        return;
    }

    if (packageLengthRemaining == 0 && SerialOutFIFO.size() == 0)
    {
        sendSyncPacketToTX(); // calculate mixer sync packet if needed
    }

    // if partial package remaining, or data in the output FIFO that needs to be written
    if (packageLengthRemaining > 0 || SerialOutFIFO.size() > 0) {
        uint8_t periodBytesRemaining = HANDSET_TELEMETRY_FIFO_SIZE;
        if (halfDuplex)
        {
            periodBytesRemaining = std::min((maxPeriodBytes - receivedBytes % maxPeriodBytes), (int)maxPacketBytes);
            periodBytesRemaining = std::max(periodBytesRemaining, (uint8_t)10);
            if (!transmitting)
            {
                transmitting = true;
                duplex_set_TX();
            }
        }

        do
        {
            SerialOutFIFO.lock();
            // no package is in transit so get new data from the fifo
            if (packageLengthRemaining == 0)
            {
                packageLengthRemaining = SerialOutFIFO.pop();
                SerialOutFIFO.popBytes(CRSFoutBuffer, packageLengthRemaining);
                sendingOffset = 0;
            }
            SerialOutFIFO.unlock();

            // if the package is long we need to split it, so it fits in the sending interval
            uint8_t writeLength = std::min(packageLengthRemaining, periodBytesRemaining);

            // write the packet out, if it's a large package the offset holds the starting position
            CRSFHandset::Port.write(CRSFoutBuffer + sendingOffset, writeLength);

            sendingOffset += writeLength;
            packageLengthRemaining -= writeLength;
            periodBytesRemaining -= writeLength;
        } while(periodBytesRemaining != 0 && SerialOutFIFO.size() != 0);
    }
}

void CRSFHandset::duplex_set_RX() const
{
    ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)GPIO_PIN_RCSIGNAL_RX, GPIO_MODE_INPUT));
    if (UARTinverted)
    {
        gpio_matrix_in((gpio_num_t)GPIO_PIN_RCSIGNAL_RX, U0RXD_IN_IDX, true);
        gpio_pulldown_en((gpio_num_t)GPIO_PIN_RCSIGNAL_RX);
        gpio_pullup_dis((gpio_num_t)GPIO_PIN_RCSIGNAL_RX);
    }
    else
    {
        gpio_matrix_in((gpio_num_t)GPIO_PIN_RCSIGNAL_RX, U0RXD_IN_IDX, false);
        gpio_pullup_en((gpio_num_t)GPIO_PIN_RCSIGNAL_RX);
        gpio_pulldown_dis((gpio_num_t)GPIO_PIN_RCSIGNAL_RX);
    }
}

void CRSFHandset::duplex_set_TX() const
{
    ESP_ERROR_CHECK(gpio_set_pull_mode((gpio_num_t)GPIO_PIN_RCSIGNAL_TX, GPIO_FLOATING));
    ESP_ERROR_CHECK(gpio_set_pull_mode((gpio_num_t)GPIO_PIN_RCSIGNAL_RX, GPIO_FLOATING));
    if (UARTinverted)
    {
        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)GPIO_PIN_RCSIGNAL_TX, 0));
        ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)GPIO_PIN_RCSIGNAL_TX, GPIO_MODE_OUTPUT));
        constexpr uint8_t MATRIX_DETACH_IN_LOW = 0x30; // routes 0 to matrix slot
        gpio_matrix_in(MATRIX_DETACH_IN_LOW, U0RXD_IN_IDX, false); // Disconnect RX from all pads
        gpio_matrix_out((gpio_num_t)GPIO_PIN_RCSIGNAL_TX, U0TXD_OUT_IDX, true, false);
    }
    else
    {
        ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)GPIO_PIN_RCSIGNAL_TX, 1));
        ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)GPIO_PIN_RCSIGNAL_TX, GPIO_MODE_OUTPUT));
        constexpr uint8_t MATRIX_DETACH_IN_HIGH = 0x38; // routes 1 to matrix slot
        gpio_matrix_in(MATRIX_DETACH_IN_HIGH, U0RXD_IN_IDX, false); // Disconnect RX from all pads
        gpio_matrix_out((gpio_num_t)GPIO_PIN_RCSIGNAL_TX, U0TXD_OUT_IDX, false, false);
    }
}

int CRSFHandset::getMinPacketInterval() const
{
    if (CRSFHandset::isHalfDuplex() && CRSFHandset::GetCurrentBaudRate() == 115200) // Packet rate limited to 200Hz if we are on 115k baud on half-duplex module
    {
        return 5000;
    }
    else if (CRSFHandset::GetCurrentBaudRate() == 115200) // Packet rate limited to 250Hz if we are on 115k baud
    {
        return 4000;
    }
    else if (CRSFHandset::GetCurrentBaudRate() == 400000) // Packet rate limited to 500Hz if we are on 400k baud
    {
        return 2000;
    }
    return 1;   // 1-million Hz!
}

void ICACHE_RAM_ATTR CRSFHandset::adjustMaxPacketSize()
{
    const int LUA_CHUNK_QUERY_SIZE = 26;
    // The number of bytes that fit into a CRSF window : baud / 10bits-per-byte / rate(Hz) * 87% (for some leeway)
    // 87% was arrived at by measuring the time taken for a chunk query packet and the processing times and switching times
    // involved from RX -> TX and vice-versa. The maxPacketBytes is used as the Lua chunk size so each chunk can be returned
    // to the handset and not be broken across time-slots as there can be issues with spurious glitches on the s.port pin
    // which switching direction. It also appears that the absolute minimum packet size should be 15 bytes as this will fit
    // the LinkStatistics and EdgeTX sync packets.
    maxPeriodBytes = std::min((int)(UARTrequestedBaud / 10 / (1000000/RequestedRCpacketIntervalUS) * 87 / 100), HANDSET_TELEMETRY_FIFO_SIZE);
    // Maximum number of bytes we can send in a single window, half the period bytes, upto one full CRSF packet.
    maxPacketBytes = std::min(maxPeriodBytes - max(maxPeriodBytes / 2, LUA_CHUNK_QUERY_SIZE), CRSF_MAX_PACKET_LEN);
}

uint32_t CRSFHandset::autobaud()
{
    static enum { INIT, MEASURED, INVERTED } state;

    if (state == MEASURED)
    {
        UARTinverted = !UARTinverted;
        state = INVERTED;
        return UARTrequestedBaud;
    }
    if (state == INVERTED)
    {
        UARTinverted = !UARTinverted;
        state = INIT;
    }

#if defined(PLATFORM_ESP32_S3)
    if (REG_GET_BIT(UART_CONF0_REG(0), UART_AUTOBAUD_EN) == 0)
#else
    if (REG_GET_BIT(UART_AUTOBAUD_REG(0), UART_AUTOBAUD_EN) == 0)
#endif	
    {
#if defined(PLATFORM_ESP32_S3)
        REG_WRITE(UART_RX_FILT_REG(0), (4 << UART_GLITCH_FILT_S) | UART_GLITCH_FILT_EN); // enable, glitch filter 4
        REG_WRITE(UART_LOWPULSE_REG(0), 4095); // reset register to max value
        REG_WRITE(UART_HIGHPULSE_REG(0), 4095); // reset register to max value
        REG_SET_BIT(UART_CONF0_REG(0), UART_AUTOBAUD_EN); // enable autobaud
#else	
        REG_WRITE(UART_AUTOBAUD_REG(0), 4 << UART_GLITCH_FILT_S | UART_AUTOBAUD_EN); // enable, glitch filter 4
#endif		
        return 400000;
    }
#if defined(PLATFORM_ESP32_S3)
    if (REG_GET_BIT(UART_CONF0_REG(0), UART_AUTOBAUD_EN) && REG_READ(UART_RXD_CNT_REG(0)) < 300)
#else    
    if (REG_GET_BIT(UART_AUTOBAUD_REG(0), UART_AUTOBAUD_EN) && REG_READ(UART_RXD_CNT_REG(0)) < 300)
#endif    
    {
        return 400000;
    }

    state = MEASURED;

#if defined(PLATFORM_ESP32_S3)
    const uint32_t low_period  = REG_READ(UART_LOWPULSE_REG(0));
    const uint32_t high_period = REG_READ(UART_HIGHPULSE_REG(0));
    REG_CLR_BIT(UART_CONF0_REG(0), UART_AUTOBAUD_EN); // disable autobaud
    REG_CLR_BIT(UART_RX_FILT_REG(0), UART_GLITCH_FILT_EN); // disable glitch filtering

    // According to the technical reference
    const int32_t calculatedBaud = UART_CLK_FREQ / (low_period + high_period + 2);
#else
    auto low_period  = (int32_t)REG_READ(UART_LOWPULSE_REG(0));
    auto high_period = (int32_t)REG_READ(UART_HIGHPULSE_REG(0));
    REG_CLR_BIT(UART_AUTOBAUD_REG(0), UART_AUTOBAUD_EN); // disable autobaud


    // sample code at https://github.com/espressif/esp-idf/issues/3336
    // says baud rate = 80000000/min(UART_LOWPULSE_REG, UART_HIGHPULSE_REG);
    // Based on testing use max and add 2 for lowest deviation
    int32_t calculatedBaud = 80000000 / (max(low_period, high_period) + 3);
#endif
    auto bestBaud = TxToHandsetBauds[0];
    for(int TxToHandsetBaud : TxToHandsetBauds)
    {
        if (abs(calculatedBaud - bestBaud) > abs(calculatedBaud - (int32_t)TxToHandsetBaud))
        {
            bestBaud = (int32_t)TxToHandsetBaud;
        }
    }
    return bestBaud;
}

bool CRSFHandset::UARTwdt()
{
    bool retval = false;
    uint32_t nowMS = millis();
    if (nowMS - UARTwdtLastCheckedMS > UARTwdtIntervalMS)
    {
        // If no packets or more bad than good packets, rate cycle/autobaud the UART
        if (BadPktsCount >= GoodPktsCount || !controllerConnected)
        {
            if (controllerConnected)
            {
                if (disconnected) disconnected();
                controllerConnected = false;
            }

            UARTrequestedBaud = autobaud();
            if (UARTrequestedBaud != 0)
            {
                adjustMaxPacketSize();

                SerialOutFIFO.flush();
                CRSFHandset::Port.flush();
                CRSFHandset::Port.updateBaudRate(UARTrequestedBaud);
                if (halfDuplex)
                {
                    duplex_set_RX();
                }
                // cleanup input buffer
                flush_port_input();
            }
            retval = true;
        }

        UARTwdtLastCheckedMS = nowMS;
        if (retval)
        {
            // Speed up the cycling
            UARTwdtLastCheckedMS -= 3 * (UARTwdtIntervalMS >> 2);
        }

        GoodPktsCountResult = GoodPktsCount;
        BadPktsCountResult = BadPktsCount;
        BadPktsCount = 0;
        GoodPktsCount = 0;
    }
    return retval;
}
