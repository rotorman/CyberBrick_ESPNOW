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

#include "hwTimer.h"

void (*hwTimer::callbackFunc)() = nullptr;

volatile bool hwTimer::running = false;
volatile uint32_t hwTimer::HWtimerIntervalUS = TimerIntervalUSDefault;

// Internal implementation specific variables
static hw_timer_t *timer = NULL;
static portMUX_TYPE isrMutex = portMUX_INITIALIZER_UNLOCKED;

void ICACHE_RAM_ATTR hwTimer::init(void (*callbackf)())
{
    if (!timer)
    {
        hwTimer::callbackFunc = callbackf;
        timer = timerBegin(0, (APB_CLK_FREQ / 1000000), true);
        timerAttachInterrupt(timer, hwTimer::callback, true);
    }
}

void ICACHE_RAM_ATTR hwTimer::stop()
{
    if (timer && running)
    {
        running = false;
        timerAlarmDisable(timer);
    }
}

void ICACHE_RAM_ATTR hwTimer::resume()
{
    if (timer && !running)
    {
        // The timer must be restarted so that the new timerAlarmWrite() period is set.
        timerRestart(timer);
        timerAlarmWrite(timer, HWtimerIntervalUS, true);
        running = true;
        timerAlarmEnable(timer);
    }
}

void ICACHE_RAM_ATTR hwTimer::updateIntervalUS(uint32_t timeUS)
{
    // timer should not be running when updateIntervalUS() is called
    HWtimerIntervalUS = timeUS;
    if (timer)
    {
        timerAlarmWrite(timer, HWtimerIntervalUS, true);
    }
}

void ICACHE_RAM_ATTR hwTimer::callback(void)
{
    if (running)
    {
        portENTER_CRITICAL_ISR(&isrMutex);
        callbackFunc();
        portEXIT_CRITICAL_ISR(&isrMutex);
    }
}
