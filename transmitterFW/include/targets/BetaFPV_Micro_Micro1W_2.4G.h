/*
 * This file belongs to the CyberBrick ESP-NOW transmitter & receiver project, hosted originally at:
 * https://github.com/rotorman/CyberBrick_ESPNOW
 * Copyright (C) 2025, Risto Kõiva
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

/* I/O definitions for BetaFPV Micro and 1W Micro 2.4 GHz TX modules
   https://betafpv.com/collections/expresslrs-series-accessories/products/elrs-micro-tx-module
   The configuration stems from https://github.com/ExpressLRS/targets/blob/master/TX/BETAFPV%202400%20Micro.json
   and https://github.com/ExpressLRS/targets/blob/master/TX/BETAFPV%202400%20Micro%201W.json
 */

#ifndef MODULE_IO_DEFINITIONS_H
#define MODULE_IO_DEFINITIONS_H

// Communication with the handset
#define GPIO_PIN_RCSIGNAL_RX_IN 13
#define GPIO_PIN_RCSIGNAL_TX_OUT 13

// Radio module
#define GPIO_PIN_RADIO_BUSY_IN 21
#define GPIO_PIN_RADIO_DIO1_IN 4
#define GPIO_PIN_RADIO_NRST_OUT 14
#define GPIO_PIN_RADIO_SPI_NSS_OUT 5

// Radio SPI-bus
#define GPIO_PIN_RADIO_SPI_MISO_IN 19
#define GPIO_PIN_RADIO_SPI_MOSI_OUT 23
#define GPIO_PIN_RADIO_SPI_SCK_OUT 18

// Radio power control
#define GPIO_PIN_RADIOPWR_RX_EN_OUT 27
#define GPIO_PIN_RADIOPWR_TX_EN_OUT 26

// Backpack
#define GPIO_PIN_BACKPACK_RX_IN 3
#define GPIO_PIN_BACKPACK_TX_OUT 1
#define BACKPACK_BAUD 460800

// User-interface
#define GPIO_PIN_LED_RGB_OUT 16
#define GPIO_PIN_SCREEN_SCK_OUT 32
#define GPIO_PIN_SCREEN_SDA_INOUT 22
#define GPIO_PIN_JOYSTICK_AIN 25

// Miscellaneous
#define GPIO_PIN_FAN_EN_OUT 17

#endif // MODULE_IO_DEFINITIONS_H
