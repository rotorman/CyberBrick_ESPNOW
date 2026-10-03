/*
 * This file belongs to the CyberBrick ESP-NOW transmitter & receiver project, hosted originally at:
 * https://github.com/rotorman/CyberBrick_ESPNOW
 * Copyright (C) 2026, Risto Kõiva
 *
 * Large parts of the code are based on the wonderful ExpressLRS project:
 * https://github.com/ExpressLRS/ExpressLRS and https://github.com/ExpressLRS/targets
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

/* I/O definitions for:
   * SenduWing H17 with internal ExpressLRS module
     https://www.senduwing.com/product/h17/
   The configuration stems from https://github.com/JCCGG/Targets/blob/h17_elrs/TX/Senduwing%20H17%202400.json
 */

#ifndef MODULE_IO_DEFINITIONS_H
#define MODULE_IO_DEFINITIONS_H

// Communication with the handset
#define GPIO_PIN_RCSIGNAL_RX_IN 3
#define GPIO_PIN_RCSIGNAL_TX_OUT 1

// Radio module
#define GPIO_PIN_RADIO_BUSY_IN 15
#define GPIO_PIN_RADIO_DIO1_IN 13
#define GPIO_PIN_RADIO_NRST_OUT 4
#define GPIO_PIN_RADIO_SPI_NSS_OUT 25

// Radio SPI-bus
#define GPIO_PIN_RADIO_SPI_MISO_IN 14
#define GPIO_PIN_RADIO_SPI_MOSI_OUT 27
#define GPIO_PIN_RADIO_SPI_SCK_OUT 26

// Radio power control
#define GPIO_PIN_RADIOPWR_RX_EN_OUT 33
#define GPIO_PIN_RADIOPWR_TX_EN_OUT 32

// Backpack
#define GPIO_PIN_BACKPACK_RX_IN 21
#define GPIO_PIN_BACKPACK_TX_OUT 22
#define GPIO_PIN_BACKPACK_BOOT_OUT 19
#define GPIO_PIN_BACKPACK_EN_OUT 20
#define BACKPACK_BAUD 460800

#endif // MODULE_IO_DEFINITIONS_H