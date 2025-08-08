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

#include "config.h"
#include "common.h"
#include "device.h"
#include "helpers.h"

#define ALL_CHANGED         (EVENT_CONFIG_MODEL_CHANGED | EVENT_CONFIG_MAIN_CHANGED)

extern void luaUpdateMAC();

TxConfig::TxConfig()
{
}

void TxConfig::Load()
{
    m_modified = 0;

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    ESP_ERROR_CHECK(nvs_open("ELRS-ESPNOW", NVS_READWRITE, &handle));

    SetModelId(0);
    m_modified = 0;

    for(unsigned i=0; i<CONFIG_TX_MODEL_CNT; i++)
    {
        char model[10] = "model";
        itoa(i, model+5, 10);
        nvs_get_u64(handle, model, &m_model_mac[i]);
        // Replace zero/unassigned value in configuration with a broadcast MAC address
        if (m_model_mac[i] == 0)
        {
          m_model_mac[i] = ESPNOW_BROADCAST_U64;
          nvs_set_u64(handle, model, m_model_mac[i]);
          nvs_commit(handle);
        }

    } // for each model
}

uint32_t TxConfig::Commit()
{
    if (!m_modified)
    {
        // No changes
        return 0;
    }
    // Write parts to NVS
    if (m_modified & EVENT_CONFIG_MODEL_CHANGED)
    {
        char model[10] = "model";
        itoa(m_modelId, model+5, 10);
        nvs_set_u64(handle, model, m_model_mac[m_modelId]);
        nvs_commit(handle);
    }
    uint32_t changes = m_modified;
    m_modified = 0;
    return changes;
}

// Setters
void TxConfig::SetStorageProvider(ESPNOW_EEPROM *eeprom)
{
    if (eeprom)
    {
        m_eeprom = eeprom;
    }
}

void TxConfig::SetModelMAC(uint64_t mac)
{
    m_model_mac[m_modelId] = mac;
    m_modified |= EVENT_CONFIG_MODEL_CHANGED;
    luaUpdateMAC();
}

/**
 * Sets ModelId used for subsequent per-model config gets
 * Returns: true if the model has changed
 **/
bool TxConfig::SetModelId(uint8_t modelId)
{
    if (m_modelId != modelId)
    {
        m_modelId = modelId;
        return true;
    }

    return false;
}
