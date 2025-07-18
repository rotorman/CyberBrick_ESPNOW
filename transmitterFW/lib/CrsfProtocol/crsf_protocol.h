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

#include <cstdint>
#include <cmath>
#include "crc.h"

extern const char device_name[];

#define PACKED __attribute__((packed))

#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR
#endif

#define CRSF_CRC_POLY 0xd5

#define CRSF_CHANNEL_VALUE_MIN  172 // 987us - actual CRSF min is 0 with E.Limits on
#define CRSF_CHANNEL_VALUE_1000 191
#define CRSF_CHANNEL_VALUE_MID  992
#define CRSF_CHANNEL_VALUE_2000 1792
#define CRSF_CHANNEL_VALUE_MAX  1811 // 2012us - actual CRSF max is 1984 with E.Limits on
#define CRSF_MAX_PACKET_LEN 64

#define CRSF_SYNC_BYTE 0xC8

#define CRSF_PAYLOAD_SIZE_MAX 62
#define CRSF_FRAME_NOT_COUNTED_BYTES 2
#define CRSF_FRAME_SIZE(payload_size) ((payload_size) + 2) // See crsf_header_t.frame_size
#define CRSF_EXT_FRAME_SIZE(payload_size) (CRSF_FRAME_SIZE(payload_size) + 2)
#define CRSF_FRAME_SIZE_MAX (CRSF_PAYLOAD_SIZE_MAX + CRSF_FRAME_NOT_COUNTED_BYTES)
#define CRSF_FRAME_CRC_SIZE 1
#define CRSF_FRAME_LENGTH_EXT_TYPE_CRC 4 // length of Extended Dest/Origin, TYPE and CRC fields combined

#define CRSF_TELEMETRY_LENGTH_INDEX 1
#define CRSF_TELEMETRY_TYPE_INDEX 2
#define CRSF_TELEMETRY_FIELD_ID_INDEX 5
#define CRSF_TELEMETRY_FIELD_CHUNK_INDEX 6
#define CRSF_TELEMETRY_CRC_LENGTH 1
#define CRSF_TELEMETRY_TOTAL_SIZE(x) (x + CRSF_FRAME_LENGTH_EXT_TYPE_CRC)

//////////////////////////////////////////////////////////////

#define CRSF_MSP_REQ_PAYLOAD_SIZE 8
#define CRSF_MSP_RESP_PAYLOAD_SIZE 58
#define CRSF_MSP_MAX_PAYLOAD_SIZE (CRSF_MSP_REQ_PAYLOAD_SIZE > CRSF_MSP_RESP_PAYLOAD_SIZE ? CRSF_MSP_REQ_PAYLOAD_SIZE : CRSF_MSP_RESP_PAYLOAD_SIZE)

typedef enum : uint8_t
{
    CRSF_FRAMETYPE_LINK_STATISTICS = 0x14,
    CRSF_FRAMETYPE_RC_CHANNELS_PACKED = 0x16,
    // Extended Header Frames, range from 0x28 to 0x96
    CRSF_FRAMETYPE_DEVICE_PING = 0x28,
    CRSF_FRAMETYPE_DEVICE_INFO = 0x29,
    CRSF_FRAMETYPE_COMMAND = 0x32,
    CRSF_FRAMETYPE_HANDSET = 0x3A
} crsf_frame_type_e;

typedef enum : uint8_t {
    CRSF_COMMAND_SUBCMD_RX = 0x10
} crsf_command_e;

typedef enum : uint8_t {
    CRSF_COMMAND_SUBCMD_RX_BIND = 0x01,
    CRSF_COMMAND_MODEL_SELECT_ID = 0x05,
    CRSF_HANDSET_SUBCMD_TIMING = 0x10,
} crsf_subcommand_e;

typedef enum : uint8_t
{
    CRSF_ADDRESS_BROADCAST = 0x00,
    CRSF_ADDRESS_FLIGHT_CONTROLLER = 0xC8,
    CRSF_ADDRESS_RADIO_TRANSMITTER = 0xEA,
    CRSF_ADDRESS_CRSF_RECEIVER = 0xEC,
    CRSF_ADDRESS_CRSF_TRANSMITTER = 0xEE
} crsf_addr_e;

//typedef struct crsf_addr_e asas;

typedef enum : uint8_t
{
    CRSF_UINT8 = 0,
    CRSF_INT8 = 1,
    CRSF_UINT16 = 2,
    CRSF_INT16 = 3,
    CRSF_UINT32 = 4,
    CRSF_INT32 = 5,
    CRSF_UINT64 = 6,
    CRSF_INT64 = 7,
    CRSF_FLOAT = 8,
    CRSF_TEXT_SELECTION = 9,
    CRSF_STRING = 10,
    CRSF_FOLDER = 11,
    CRSF_INFO = 12,
    CRSF_COMMAND = 13,
    CRSF_VTX = 15,
    CRSF_OUT_OF_RANGE = 127,
} crsf_value_type_e;

// These flags are or'ed with the field type above to hide the field from the normal LUA view
#define CRSF_FIELD_HIDDEN       0x80     // marked as hidden in all LUA responses
#define CRSF_FIELD_ELRS_HIDDEN  0x40     // marked as hidden when talking to ELRS specific LUA
#define CRSF_FIELD_TYPE_MASK    ~(CRSF_FIELD_HIDDEN|CRSF_FIELD_ELRS_HIDDEN)

/**
 * Define the shape of a standard header
 */
typedef struct crsf_header_s
{
    uint8_t device_addr; // from crsf_addr_e
    uint8_t frame_size;  // counts size after this byte, so it must be the payload size + 2 (type and crc)
    crsf_frame_type_e type;
    uint8_t payload[0];
} PACKED crsf_header_t;

// Used by extended header frames (type in range 0x28 to 0x96)
typedef struct crsf_ext_header_s
{
    // Common header fields, see crsf_header_t
    uint8_t device_addr;
    uint8_t frame_size;
    crsf_frame_type_e type;
    // Extended fields
    crsf_addr_e dest_addr;
    crsf_addr_e orig_addr;
    uint8_t payload[0];
} PACKED crsf_ext_header_t;

/**
 * Crossfire packed channel structure, each channel is 11 bits
 */
typedef struct crsf_channels_s
{
    unsigned ch0 : 11;
    unsigned ch1 : 11;
    unsigned ch2 : 11;
    unsigned ch3 : 11;
    unsigned ch4 : 11;
    unsigned ch5 : 11;
    unsigned ch6 : 11;
    unsigned ch7 : 11;
    unsigned ch8 : 11;
    unsigned ch9 : 11;
    unsigned ch10 : 11;
    unsigned ch11 : 11;
    unsigned ch12 : 11;
    unsigned ch13 : 11;
    unsigned ch14 : 11;
    unsigned ch15 : 11;
} PACKED crsf_channels_t;

/**
 * Define the shape of a standard packet
 * A 'standard' header followed by the packed channels
 */
typedef struct rcPacket_s
{
    crsf_header_t header;
    crsf_channels_s channels;
} PACKED rcPacket_t;

typedef struct deviceInformationPacket_s
{
    uint32_t serialNo;
    uint32_t hardwareVer;
    uint32_t softwareVer;
    uint8_t parametersCnt;
    uint8_t parameterVersion;
} PACKED deviceInformationPacket_t;

#define DEVICE_INFORMATION_PAYLOAD_LENGTH (sizeof(deviceInformationPacket_t) + strlen(device_name)+1)
#define DEVICE_INFORMATION_LENGTH (sizeof(crsf_ext_header_t) + DEVICE_INFORMATION_PAYLOAD_LENGTH + CRSF_FRAME_CRC_SIZE)

/**
 * Union to allow accessing the input buffer as different data shapes
 * without generating compiler warnings (and relying on undefined C++ behaviour!)
 * Each entry in the union provides a different view of the same memory.
 * This is just the defintion of the union, the declaration of the variable that
 * uses it is later in the file.
 */
union inBuffer_U
{
    uint8_t asUint8_t[CRSF_MAX_PACKET_LEN]; // max 64 bytes for CRSF packet serial buffer
    rcPacket_t asRCPacket_t;    // access the memory as RC data
                                // add other packet types here
};

/*
 * 0x14 Link statistics
 * Payload:
 *
 * uint8_t Uplink RSSI Ant. 1 ( dBm * -1 )
 * uint8_t Uplink RSSI Ant. 2 ( dBm * -1 )
 * uint8_t Uplink Package success rate / Link quality ( % )
 * int8_t Uplink SNR ( db )
 * uint8_t Diversity active antenna ( enum ant. 1 = 0, ant. 2 )
 * uint8_t RF Mode ( enum 4fps = 0 , 50fps, 150hz)
 * uint8_t Uplink TX Power ( enum 0mW = 0, 10mW, 25 mW, 100 mW, 500 mW, 1000 mW, 2000mW )
 * uint8_t Downlink RSSI ( dBm * -1 )
 * uint8_t Downlink package success rate / Link quality ( % )
 * int8_t Downlink SNR ( db )
 * Uplink is the connection from the ground to the UAV and downlink the opposite direction.
 */

typedef struct crsfPayloadLinkstatistics_s
{
    uint8_t uplink_RSSI_1;
    uint8_t uplink_RSSI_2;
    uint8_t uplink_Link_quality;
    int8_t uplink_SNR;
    uint8_t active_antenna;
    uint8_t rf_Mode;
    uint8_t uplink_TX_Power;
    uint8_t downlink_RSSI_1;
    uint8_t downlink_Link_quality;
    int8_t downlink_SNR;
} PACKED crsfLinkStatistics_t;

/////inline and utility functions//////

static uint16_t ICACHE_RAM_ATTR fmap(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max)
{
    return ((x - in_min) * (out_max - out_min) * 2 / (in_max - in_min) + out_min * 2 + 1) / 2;
}

// Scale a -100& to +100% crossfire value to 988-2012 (Taranis channel uS)
static inline uint16_t ICACHE_RAM_ATTR CRSF_to_US(uint16_t val)
{
    return fmap(val, CRSF_CHANNEL_VALUE_MIN, CRSF_CHANNEL_VALUE_MAX, 988, 2012);
}

// Scale down a 10-bit value to a -100& to +100% crossfire value
static inline uint16_t ICACHE_RAM_ATTR UINT10_to_CRSF(uint16_t val)
{
    return fmap(val, 0, 1023, CRSF_CHANNEL_VALUE_MIN, CRSF_CHANNEL_VALUE_MAX);
}

// Scale up a -100& to +100% crossfire value to 10-bit
static inline uint16_t ICACHE_RAM_ATTR CRSF_to_UINT10(uint16_t val)
{
    return fmap(val, CRSF_CHANNEL_VALUE_MIN, CRSF_CHANNEL_VALUE_MAX, 0, 1023);
}

// Convert 0-max to the CRSF values for 1000-2000
static inline uint16_t ICACHE_RAM_ATTR N_to_CRSF(uint16_t val, uint16_t max)
{
    return val * (CRSF_CHANNEL_VALUE_2000-CRSF_CHANNEL_VALUE_1000) / max + CRSF_CHANNEL_VALUE_1000;
}

// Convert CRSF to 0-(cnt-1), constrained between 1000us and 2000us
static inline uint16_t ICACHE_RAM_ATTR CRSF_to_N(uint16_t val, uint16_t cnt)
{
    // The span is increased by one to prevent the max val from returning cnt
    if (val <= CRSF_CHANNEL_VALUE_1000)
        return 0;
    if (val >= CRSF_CHANNEL_VALUE_2000)
        return cnt - 1;
    return (val - CRSF_CHANNEL_VALUE_1000) * cnt / (CRSF_CHANNEL_VALUE_2000 - CRSF_CHANNEL_VALUE_1000 + 1);
}

// Returns 1 if val is greater than CRSF_CHANNEL_VALUE_MID
static inline uint8_t ICACHE_RAM_ATTR CRSF_to_BIT(uint16_t val)
{
    return (val > CRSF_CHANNEL_VALUE_MID) ? 1 : 0;
}

// Convert a bit into either the CRSF value for 1000 or 2000
static inline uint16_t ICACHE_RAM_ATTR BIT_to_CRSF(uint8_t val)
{
    return (val) ? CRSF_CHANNEL_VALUE_2000 : CRSF_CHANNEL_VALUE_1000;
}

#if !defined(__linux__)
static inline uint32_t htobe32(uint32_t val)
{
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    return val;
#else
    return __builtin_bswap32(val);
#endif
}
#endif