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

#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "common.h"
#include "CRSFHandset.h"
#include "UnusedPeriph.h"
#include "hwTimer.h"

/***** TODO! Adjust the values in this section to YOUR setup! *****/

// Model receiver's MAC address(es) - replace with YOUR CyberBrick receiver Core MAC address(es)!
// The example below lists 3 models. If you wish to control only one model, remove the bottom two lines (models 1 and 2).
// You can control/add up to 20 models to the list below (limitation of ESP32 ESP-NOW peer address table).
uint8_t cyberbrickRxMAC[][6] =
  {
    {0xa1, 0xb1, 0xc1, 0xd1, 0xe1, 0xf1}, // Model 0 receiver MAC address
    {0xa2, 0xb2, 0xc2, 0xd2, 0xe2, 0xf2}, // Model 1 receiver MAC address
    {0xa3, 0xb3, 0xc3, 0xd3, 0xe3, 0xf3}  // Model 2 receiver MAC address
  };
// You can pick a model to control in EdgeTX under:
// MODEL -> Internal RF or External RF -> Receiver <number>
// where the number matches the model number in the above list.

/******************************************************************/

// The following is replied in a CRSF ping response telegram to the handset and
// displayed as a module identification in EdgeTX under:
// System -> About -> Modules / RX version
const char device_name[] = "CyberBrick TX"; // max. 16 characters
char versionID[] = "1.0.0";

// Current state of channels, CRSF format
volatile uint16_t ChannelData[CRSF_NUM_CHANNELS];
connectionState_e connectionState = awatingFirstPacket;

CRSFHandset *handset = new CRSFHandset();
esp_now_peer_info_t peerInfo;

bool SendRCdataToRF();
void timerCallback();
bool initESPNOW();
void ESPNOW_OnDataSentCB(const uint8_t *mac_addr, esp_now_send_status_t status);
static void UARTconnected();
static void UARTdisconnected();
void ModelUpdateReq();

// Initialization
void setup() {
  pinMode(GPIO_PIN_BOOT0, INPUT); // setup so that we can detect pin-change for passthrough mode of the optional ExpressLRS module backpack
  initUnusedDevices();
  handset->Begin();
  handset->registerCallbacks(UARTconnected, UARTdisconnected, ModelUpdateReq);

  while (!initESPNOW()) {}
  hwTimer::init(timerCallback);
  hwTimer::updateIntervalUS(RF_FRAME_RATE_US);
  setConnectionState(awatingFirstPacket);
}

// Main execution loop
void loop() {
  handset->handleInput();
  delay(1); // yield
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

  // Register peers
  memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
  peerInfo.channel = 0;
  peerInfo.encrypt = false;  
  bool bResult = true;
  for (int i = 0; i < sizeof(cyberbrickRxMAC)/6; i++)
  { 
    // Iterate through the peer addresses
    memcpy(peerInfo.peer_addr, cyberbrickRxMAC[i], 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
      bResult = false;
    }
  }
  
  // Register callback to get the status of the transmitted ESP-NOW packet
  if (esp_now_register_send_cb(ESPNOW_OnDataSentCB) != ESP_OK)
    bResult = false;
  
  return bResult;
}

/*
 * Called from timer ISR when there is a CRSF connection from the handset
 */
void ICACHE_RAM_ATTR timerCallback()
{
  // Do not transmit until in disconnected/connected state
  if (connectionState == awaitingModelId)
    return;

  SendRCdataToRF();
}

bool ICACHE_RAM_ATTR SendRCdataToRF()
{
  // Send message via ESP-NOW
  uint8_t modelid = handset->getModelID();
  bool bResult = false;
  if (modelid <= sizeof(cyberbrickRxMAC)/6) // Plausibility check that we are not accessing cyberbrickRxMAC array out of bounds
  {
    esp_err_t result = esp_now_send(cyberbrickRxMAC[modelid], (uint8_t *) &ChannelData, sizeof(ChannelData));
   
    if (result == ESP_OK) {
      bResult = true;
    }
  }
  return bResult;
}

// ESP-NOW callback, called when data is sent
void ESPNOW_OnDataSentCB(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS)
  {
    handset->JustSentRFpacket();
  }
}

static void UARTdisconnected()
{
  hwTimer::stop();
  setConnectionState(disconnected);
}

static void UARTconnected()
{
  if (connectionState == disconnected)
    setConnectionState(connected);

  if (connectionState == awatingFirstPacket)
  {
    // When CRSF first connects, always go into a brief delay before
    // starting to transmit, to make sure a ModelID update isn't coming
    // right behind it
    setConnectionState(awaitingModelId);
  }

  // Start the timer to get EdgeTX sync going and a ModelID update sent
  hwTimer::resume();
}

void ModelUpdateReq()
{
  if (connectionState == awaitingModelId)
  {
    setConnectionState(connected);
  }
}
