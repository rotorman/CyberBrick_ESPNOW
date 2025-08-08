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

class EspFlashStream : public Stream
{
public:
    EspFlashStream();
    // Set the starting address to use with seek()s
    void setBaseAddress(size_t base);
    size_t getPosition() const { return _flashOffset + _bufferPos; }
    void setPosition(size_t offset);

    // Stream class overrides
    virtual size_t write(uint8_t) { return 0; }
    virtual int available() { return (_bufferPos <= sizeof(_buffer)) ? 1 : 0; }
    virtual int read();
    virtual int peek();

private:
    WORD_ALIGNED_ATTR uint8_t _buffer[4];
    size_t _flashBase;
    size_t _flashOffset;
    uint8_t _bufferPos;

    void fillBuffer();
};