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

#include "targets.h"
#include "common.h"
#include "helpers.h"
#include "device.h"

///////////////////////////////////////
// Even though we aren't using anything this keeps the PIO dependency analyzer happy!

#include <soc/soc_caps.h>
#define MULTICORE (SOC_CPU_CORES_NUM > 1)

///////////////////////////////////////

static device_affinity_t *uiDevices;
static uint8_t deviceCount;

static uint32_t eventFired[2] = {0, 0};
static bool lastModelMatch[2] = {false, false};

static unsigned long deviceTimeout[16] = {0};

#if MULTICORE
static TaskHandle_t xDeviceTask = NULL;
static SemaphoreHandle_t taskSemaphore;
static SemaphoreHandle_t completeSemaphore;
[[noreturn]] static void deviceTask(void *pvArgs);
#define CURRENT_CORE  xPortGetCoreID()
#else
#define CURRENT_CORE -1
#endif

void devicesRegister(device_affinity_t *devices, uint8_t count)
{
    uiDevices = devices;
    deviceCount = count;

    #if MULTICORE
        taskSemaphore = xSemaphoreCreateBinary();
        completeSemaphore = xSemaphoreCreateBinary();
        disableCore0WDT();
        xTaskCreatePinnedToCore(deviceTask, "deviceTask", 32768, NULL, 0, &xDeviceTask, 0);
    #endif
}

void devicesInit()
{
    int32_t core = CURRENT_CORE;

    for(size_t i=0 ; i<deviceCount ; i++) {
        if (uiDevices[i].core == core || core == -1) {
            if (uiDevices[i].device->initialize && !(uiDevices[i].device->initialize)()) {
                uiDevices[i].device->start = nullptr;
                uiDevices[i].device->event = nullptr;
                uiDevices[i].device->timeout = nullptr;
            }
        }
    }
    #if MULTICORE
    if (core == 1)
    {
        xSemaphoreGive(taskSemaphore);
        xSemaphoreTake(completeSemaphore, portMAX_DELAY);
    }
    #endif
}

void devicesStart()
{
    int32_t core = CURRENT_CORE;
    unsigned long now = millis();

    for(size_t i=0 ; i<deviceCount ; i++)
    {
        if (uiDevices[i].core == core || core == -1) {
            deviceTimeout[i] = 0xFFFFFFFF;
            if (uiDevices[i].device->start)
            {
                int delay = (uiDevices[i].device->start)();
                deviceTimeout[i] = delay == DURATION_NEVER ? 0xFFFFFFFF : now + delay;
            }
        }
    }
    #if MULTICORE
    if (core == 1)
    {
        xSemaphoreGive(taskSemaphore);
        xSemaphoreTake(completeSemaphore, portMAX_DELAY);
    }
    #endif
}

void devicesStop()
{
    #if MULTICORE
    vTaskDelete(xDeviceTask);
    #endif
}

void devicesTriggerEvent(uint32_t events)
{
    eventFired[0] |= events;
    eventFired[1] |= events;
    #if MULTICORE
    // Release the semaphore so the tasks on core 0 run now
    xSemaphoreGive(taskSemaphore);
    #endif
}

static int _devicesUpdate(unsigned long now)
{
    const int32_t core = CURRENT_CORE;
    const int32_t coreMulti = (core == -1) ? 0 : core;

    bool newModelMatch = connectionHasModelMatch;
    uint32_t events = eventFired[coreMulti];
    eventFired[coreMulti] = 0;
    bool handleEvents = events != 0 || lastModelMatch[coreMulti] != newModelMatch;
    lastModelMatch[coreMulti] = newModelMatch;

    if (handleEvents)
    {
        for(size_t i=0 ; i<deviceCount ; i++)
        {
            if ((uiDevices[i].core == core || core == -1) && (uiDevices[i].device->event && (uiDevices[i].device->subscribe & events) != 0))
            {
                int delay = (uiDevices[i].device->event)();
                if (delay != DURATION_IGNORE)
                {
                    deviceTimeout[i] = delay == DURATION_NEVER ? 0xFFFFFFFF : now + delay;
                }
            }
        }
    }

    int smallest_delay = DURATION_NEVER;
    for(size_t i=0 ; i<deviceCount ; i++)
    {
        if ((uiDevices[i].core == core || core == -1) && uiDevices[i].device->timeout)
        {
            int delay = deviceTimeout[i] == 0xFFFFFFFF ? DURATION_NEVER : (int)(deviceTimeout[i]-now);
            if (now >= deviceTimeout[i])
            {
                delay = (uiDevices[i].device->timeout)();
                deviceTimeout[i] = delay == DURATION_NEVER ? 0xFFFFFFFF : now + delay;
            }
            if (delay != DURATION_NEVER)
            {
                smallest_delay = (smallest_delay == DURATION_NEVER) ? delay : std::min(smallest_delay, delay);
            }
        }
    }
    return smallest_delay;
}

void devicesUpdate(unsigned long now)
{
    _devicesUpdate(now);
}

#if MULTICORE
[[noreturn]] static void deviceTask(void *pvArgs)
{
    xSemaphoreTake(taskSemaphore, portMAX_DELAY);
    devicesInit();
    xSemaphoreGive(completeSemaphore);
    xSemaphoreTake(taskSemaphore, portMAX_DELAY);
    devicesStart();
    xSemaphoreGive(completeSemaphore);
    for (;;)
    {
        int delay = _devicesUpdate(millis());
        // sleep the core until the desired time, or it's awakened by an event
        xSemaphoreTake(taskSemaphore, delay == DURATION_NEVER ? portMAX_DELAY : pdMS_TO_TICKS(delay));
    }
}
#endif
