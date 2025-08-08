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
#include <stdio.h>

#define TimerIntervalUSDefault 4000

/**
 * @brief Hardware abstraction for the hardware timer to provide precise timing.
 *
 * The timer provides a callback.
 */
class hwTimer
{
public:
    /**
     * @brief Initialise the timer with the specified callback function.
     *
     * The timer is initialised in the stopped state and must be started with resume().
     */
    static void init(void (*callbackf)());

    /**
     * @brief Stop the timer.
     *
     * The timer is stopped and no more callbacks are performed.
     */
    static void stop();

    /**
     * @brief Resume the timer.
     *
     * The timer is started from this instant, callbacks will be called.
     * The timer fires a callback immediately after it is resumed.
     */
    static void resume();

    /**
     * @brief Change the interval between callbacks.
     * The timer should not be running when updateIntervalUS() is called.
     *
     * @param time in microseconds.
     */
    static void updateIntervalUS(uint32_t timeUS = TimerIntervalUSDefault);

    static volatile bool running;

private:
    static void callback();

    static void (*callbackFunc)();

    static volatile uint32_t HWtimerIntervalUS;
};
