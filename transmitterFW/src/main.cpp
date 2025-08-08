/*
 * This file belongs to the CyberBrick ESP-NOW transmitter & receiver project,
 * hosted originally at:
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

/* In your EdgeTX radio, enable CRSF for either the internal or the external RF module:
   MODEL -> Internal RF or External RF -> Mode: CRSF
   You can use any in EdgeTX selectable baud rate.
*/

#include "targets.h"
#include "common.h"
#include "config.h"
#include "CRSF.h"
#include "helpers.h"
#include "hwTimer.h"
#include "CRSFHandset.h"
#include "lua.h"
#include "devHandset.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

#define RF_RC_INTERVAL_US 20000U // 50 Hz
#define CONNECTIONLOSTTIMEOUT_MS 200
#define TLM_REPORT_INTERVAL_MS 240U

const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/// define some libs to use ///
ESPNOW_EEPROM eeprom;
TxConfig config;

// Connection state information
bool connectionHasModelMatch = false;
bool InBindingMode = false;
connectionState_e connectionState = RECEIVERDISCONNECTED;

// Current state of channels, CRSF format
volatile uint16_t ChannelData[CRSF_NUM_CHANNELS];

extern device_t LUA_device;
uint32_t TLMpacketReportedMS = 0;

static bool commitInProgress = false;
volatile bool busyTransmitting = false;
static volatile bool ModelUpdatePending = false;
static volatile unsigned long uiLastESPNOWsentOKMS = 0;
static volatile bool bBindModeMACreceived = false;

esp_now_peer_info_t peerInfo;

device_affinity_t ui_devices[] = {
  {&Handset_device, 1},
  {&LUA_device, 1},
};

bool setupHardwareFromOptions();
bool initESPNOW();
void ESPNOW_OnDataSentCB(const uint8_t *mac_addr, esp_now_send_status_t status);
void EnterBindingMode();
static void CheckConfigChangePending();
static void ConfigChangeCommit();
void ICACHE_RAM_ATTR OnESPNOWBindDataRecv(const uint8_t * mac_addr, const uint8_t *data, int data_len);
void ModelUpdateReq();
static void UARTconnected();
static void UARTdisconnected();
void ICACHE_RAM_ATTR timerCallback();

void setup()
{
  pinMode(GPIO_PIN_BOOT0, INPUT); // setup so that we can detect pin-change for passthrough mode of the optional ExpressLRS module backpack
  if (setupHardwareFromOptions())
  {
    // Register the devices with the framework
    devicesRegister(ui_devices, ARRAY_SIZE(ui_devices));
    // Initialise the devices
    devicesInit();

    handset->registerCallbacks(UARTconnected, UARTdisconnected, ModelUpdateReq, EnterBindingMode);

    eeprom.Begin(); // Init the eeprom
    config.SetStorageProvider(&eeprom); // Pass pointer to the Config class for access to storage
    config.Load(); // Load the stored values from eeprom

    bool init_success = true;

    if (!init_success)
    {
      setConnectionState(radioFailed);
    }
    else
    {
      hwTimer::updateIntervalUS(RF_RC_INTERVAL_US);
      handset->setPacketIntervalUS(RF_RC_INTERVAL_US);
      hwTimer::init(timerCallback);
      setConnectionState(noHandsetCommunication);
    }
  }

  devicesStart();
}

void loop()
{
  uint32_t nowMS = millis();

  // Update UI devices
  devicesUpdate(nowMS);

  if (connectionState > MODE_STATES)
  {
    return;
  }

  CheckConfigChangePending();

  /* Send TLM updates to handset if connected + reporting period
   * is elapsed. This keeps handset happy dispite of the telemetry ratio */
  if ((connectionState == RECEIVERCONNECTED) && (uiLastESPNOWsentOKMS != 0) &&
      (nowMS >= (uint32_t)(TLM_REPORT_INTERVAL_MS + TLMpacketReportedMS)))
  {
    uint8_t linkStatisticsFrame[CRSF_FRAME_NOT_COUNTED_BYTES + CRSF_FRAME_SIZE(sizeof(crsfLinkStatistics_t))];

    CRSFHandset::makeLinkStatisticsPacket(linkStatisticsFrame);
    handset->sendTelemetryToTX(linkStatisticsFrame);
    TLMpacketReportedMS = nowMS;
  }
}

bool setupHardwareFromOptions()
{
  if (!options_init())
  {
    setConnectionState(hardwareUndefined);
    return false;
  }

  // TODO! Add timeout or bailout
  while (!initESPNOW()) {}

  return true;
}

bool initESPNOW()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);
  WiFi.begin("network-name", "pass-to-network", 1);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    ESP.restart();
  }

  // Register model MAC as peer
  memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  bool bResult = true;

  // Extract model MAC address
  uint8_t peerMAC[6];
  uint64_t currentModelMAC = config.GetModelMAC();
  peerMAC[0] = (uint8_t)((currentModelMAC & 0x0000FF0000000000)>>40);
  peerMAC[1] = (uint8_t)((currentModelMAC & 0x000000FF00000000)>>32);
  peerMAC[2] = (uint8_t)((currentModelMAC & 0x00000000FF000000)>>24);
  peerMAC[3] = (uint8_t)((currentModelMAC & 0x0000000000FF0000)>>16);
  peerMAC[4] = (uint8_t)((currentModelMAC & 0x000000000000FF00)>>8);
  peerMAC[5] = (uint8_t)((currentModelMAC & 0x00000000000000FF));

  memcpy(peerInfo.peer_addr, peerMAC, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    bResult = false;
  }

  // Register callback to get the status of the transmitted ESP-NOW packet
  if (esp_now_register_send_cb(ESPNOW_OnDataSentCB) != ESP_OK)
    bResult = false;
  
  return bResult;
}

// ESP-NOW callback, called when data is sent
void ESPNOW_OnDataSentCB(const uint8_t *mac_addr, esp_now_send_status_t status) {
  uint32_t const now = millis();
  if (status == ESP_NOW_SEND_SUCCESS)
  {
    connectionHasModelMatch = true;
    uiLastESPNOWsentOKMS = now;
    setConnectionState(RECEIVERCONNECTED);
  }
  else
  {
    if (now > (uiLastESPNOWsentOKMS + CONNECTIONLOSTTIMEOUT_MS))
    {
      setConnectionState(RECEIVERDISCONNECTED);
    }
  }
  busyTransmitting = false;
}

void EnterBindingMode()
{
  if (InBindingMode)
      return;

  // Disable the TX timer and wait for any TX to complete
  //hwTimer::stop();
  while (busyTransmitting);

  InBindingMode = true;

  // Clear previous ESP-NOW entry
  uint8_t peerMAC[6];
  uint64_t currentModelMAC = config.GetModelMAC();
  peerMAC[0] = (uint8_t)((currentModelMAC & 0x0000FF0000000000)>>40);
  peerMAC[1] = (uint8_t)((currentModelMAC & 0x000000FF00000000)>>32);
  peerMAC[2] = (uint8_t)((currentModelMAC & 0x00000000FF000000)>>24);
  peerMAC[3] = (uint8_t)((currentModelMAC & 0x0000000000FF0000)>>16);
  peerMAC[4] = (uint8_t)((currentModelMAC & 0x000000000000FF00)>>8);
  peerMAC[5] = (uint8_t)((currentModelMAC & 0x00000000000000FF));
  memcpy(peerInfo.peer_addr, peerMAC, 6);
  esp_now_del_peer(peerInfo.peer_addr);

  // Add broadcast address as peer to receive binding info without filter
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  esp_now_add_peer(&peerInfo);
  bBindModeMACreceived = false;
  // Register receive callback
  esp_now_register_recv_cb(OnESPNOWBindDataRecv);
}

static void CheckConfigChangePending()
{
  if (config.IsModified() || ModelUpdatePending)
  {
    // wait until no longer transmitting
    while (busyTransmitting);
    // Set the commitInProgress flag to prevent any other RF SPI traffic during the commit from RX or scheduled TX
    commitInProgress = true;
    ConfigChangeCommit();
  }
}

static void ConfigChangeCommit()
{
  // Write the uncommitted eeprom values (may block for a while)
  uint32_t changes = config.Commit();
  // Clear the commitInProgress flag so normal processing resumes
  commitInProgress = false;
  // UpdateFolderNames is expensive so it is called directly instead of in event() which gets called a lot
  devicesTriggerEvent(changes);
}

void ClearMAC()
{
  // Clear previous ESP-NOW entry
  uint8_t peerMAC[6];
  uint64_t currentModelMAC = config.GetModelMAC();
  peerMAC[0] = (uint8_t)((currentModelMAC & 0x0000FF0000000000)>>40);
  peerMAC[1] = (uint8_t)((currentModelMAC & 0x000000FF00000000)>>32);
  peerMAC[2] = (uint8_t)((currentModelMAC & 0x00000000FF000000)>>24);
  peerMAC[3] = (uint8_t)((currentModelMAC & 0x0000000000FF0000)>>16);
  peerMAC[4] = (uint8_t)((currentModelMAC & 0x000000000000FF00)>>8);
  peerMAC[5] = (uint8_t)((currentModelMAC & 0x00000000000000FF));
  memcpy(peerInfo.peer_addr, peerMAC, 6);
  esp_now_del_peer(peerInfo.peer_addr);

  config.SetModelMAC(ESPNOW_BROADCAST_U64);
  ModelUpdatePending = true;
  setConnectionState(RECEIVERDISCONNECTED);
}

void ICACHE_RAM_ATTR OnESPNOWBindDataRecv(const uint8_t * mac_addr, const uint8_t *data, int data_len)
{
  if (InBindingMode && !bBindModeMACreceived && (data_len == 6))
  {
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    esp_now_del_peer(peerInfo.peer_addr);
    
    uint64_t receivedMAC = ((uint64_t)(data[0])<<40) +
                           ((uint64_t)(data[1])<<32) +
                           ((uint64_t)(data[2])<<24) +
                           ((uint64_t)(data[3])<<16) +
                           ((uint64_t)(data[4])<<8) +
                           (uint64_t)(data[5]);
    config.SetModelMAC(receivedMAC);
    ModelUpdatePending = true;

    bBindModeMACreceived = true;

    // Exit binding mode
    if (!InBindingMode)
      return;

    InBindingMode = false;
    setConnectionState(RECEIVERDISCONNECTED);
  }
}

void ModelUpdateReq()
{
  // Force synspam with the current rate parameters in case already have a connection established
  if (config.SetModelId(CRSFHandset::getModelID()))
  {
    ModelUpdatePending = true;
  }

  devicesTriggerEvent(EVENT_MODEL_SELECTED);

  // Jump from AWAITINGMODELIDFROMHANDSET to transmitting to break the startup delay now
  // that the ModelID has been confirmed by the handset
  if (connectionState == AWAITINGMODELIDFROMHANDSET)
  {
    setConnectionState(RECEIVERDISCONNECTED);
  }
}

static void UARTconnected()
{
  if (connectionState == noHandsetCommunication || connectionState < MODE_STATES)
  {
    // When CRSF first connects, always go into a brief delay before
    // starting to transmit, to make sure a ModelID update isn't coming
    // right behind it
    setConnectionState(AWAITINGMODELIDFROMHANDSET);
  }

  // But start the timer to get EdgeTX sync going and a ModelID update sent
  hwTimer::resume();
}

static void UARTdisconnected()
{
  hwTimer::stop();
  setConnectionState(noHandsetCommunication);
}

// Called as the timer ISR when there is a CRSF connection from the handset
void ICACHE_RAM_ATTR timerCallback()
{
  /* If we are busy writing to EEPROM (committing config changes) then we generate no further traffic */
  if (commitInProgress)
  {
    return;
  }

  // Sync EdgeTX to this point
  handset->JustSentRFpacket();

  // Do not transmit until in RECEIVERDISCONNECTED/RECEIVERCONNECTED state
  if (connectionState == AWAITINGMODELIDFROMHANDSET)
    return;

  // Send RC channels via RF to model

  // Do not send a stale channels packet to the RX if one has not been received from the handset
  // *Do* send data if a packet has never been received from handset and the timer is running
  // this is the case when bench testing
  uint32_t lastRcDataUS = handset->GetRCdataLastRecvUS();
  if (lastRcDataUS && (micros() - lastRcDataUS > 1000000))
  {
    // Do not send stale or fake zero packet RC!
    return;
  }

  // Extract model MAC address
  uint8_t peerMAC[6];
  uint64_t currentModelMAC = config.GetModelMAC();
  peerMAC[0] = (uint8_t)((currentModelMAC & 0x0000FF0000000000)>>40);
  peerMAC[1] = (uint8_t)((currentModelMAC & 0x000000FF00000000)>>32);
  peerMAC[2] = (uint8_t)((currentModelMAC & 0x00000000FF000000)>>24);
  peerMAC[3] = (uint8_t)((currentModelMAC & 0x0000000000FF0000)>>16);
  peerMAC[4] = (uint8_t)((currentModelMAC & 0x000000000000FF00)>>8);
  peerMAC[5] = (uint8_t)((currentModelMAC & 0x00000000000000FF));

  if (ModelUpdatePending)
  {
    memcpy(peerInfo.peer_addr, peerMAC, 6);
    esp_now_add_peer(&peerInfo);
    ModelUpdatePending = false;
  }

  if (!InBindingMode)
  {
    busyTransmitting = true;
    // Send message via ESP-NOW
    esp_now_send(peerMAC, (uint8_t *) &ChannelData, sizeof(ChannelData));
  }
}
