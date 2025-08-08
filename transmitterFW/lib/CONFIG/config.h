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
#include "espnow_eeprom.h"
#include "options.h"
#include "common.h"
#include <nvs_flash.h>
#include <nvs.h>

#define CONFIG_TX_MODEL_CNT 64U // max supported model count of EdgeTX
#define ESPNOW_BROADCAST_U64 0xFFFFFFFFFFFF // ESP-NOW broadcast MAC FF:FF:FF:FF:FF:FF

class TxConfig
{
public:
    TxConfig();
    void Load();
    uint32_t Commit();

    // Getters
    uint64_t GetModelMAC() const { return m_model_mac[m_modelId]; }
    bool     IsModified() const { return m_modified != 0; }

    // Setters
    void SetStorageProvider(ESPNOW_EEPROM *eeprom);
    void SetModelMAC(uint64_t mac);

    // State setters
    bool SetModelId(uint8_t modelId);

private:
    ESPNOW_EEPROM *m_eeprom;
    uint32_t     m_modified;
    uint64_t     m_model_mac[CONFIG_TX_MODEL_CNT];
    uint8_t      m_modelId;
    nvs_handle   handle;
};

extern TxConfig config;
