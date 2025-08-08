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

#include "common.h"
#include "device.h"
#include "lua.h"
#include "config.h"
#include "CRSF.h"
#include "CRSFHandset.h"
#include "helpers.h"

const char STR_EMPTYSPACE[] = { 0 };

#define STR_LUA_ALLAUX         "AUX1;AUX2;AUX3;AUX4;AUX5;AUX6;AUX7;AUX8;AUX9;AUX10"

#define STR_LUA_ALLAUX_UPDOWN  "AUX1" LUASYM_ARROW_UP ";AUX1" LUASYM_ARROW_DN ";AUX2" LUASYM_ARROW_UP ";AUX2" LUASYM_ARROW_DN \
                               ";AUX3" LUASYM_ARROW_UP ";AUX3" LUASYM_ARROW_DN ";AUX4" LUASYM_ARROW_UP ";AUX4" LUASYM_ARROW_DN \
                               ";AUX5" LUASYM_ARROW_UP ";AUX5" LUASYM_ARROW_DN ";AUX6" LUASYM_ARROW_UP ";AUX6" LUASYM_ARROW_DN \
                               ";AUX7" LUASYM_ARROW_UP ";AUX7" LUASYM_ARROW_DN ";AUX8" LUASYM_ARROW_UP ";AUX8" LUASYM_ARROW_DN \
                               ";AUX9" LUASYM_ARROW_UP ";AUX9" LUASYM_ARROW_DN ";AUX10" LUASYM_ARROW_UP ";AUX10" LUASYM_ARROW_DN


#define HAS_RADIO (GPIO_PIN_SCK != UNDEF_PIN)

static char version_domain[20+1+6+1];
static const char folderNameSeparator[2] = {' ',':'};
static const char luastrOffOn[] = "Off;On";
static char luaBadGoodString[10];
char modelMACca[2*6+5+1];

static struct luaItem_string luaModelMAC = {
    {"Model MAC", CRSF_INFO},
    modelMACca
};

static struct luaItem_command luaBind = {
    {"Bind", CRSF_COMMAND},
    lcsIdle, // step
    STR_EMPTYSPACE
};

static struct luaItem_string luaInfo = {
    {"Bad/Good", (crsf_value_type_e)(CRSF_INFO | CRSF_FIELD_ELRS_HIDDEN)},
    STR_EMPTYSPACE
};

static struct luaItem_string luaELRSversion = {
    {version_domain, CRSF_INFO},
    commit
};

void luaUpdateMAC();
static void luadevUpdateStrings();

extern TxConfig config;
extern void sendELRSstatus();

static void luahandSimpleSendCmd(struct luaPropertiesCommon *item, uint8_t arg)
{
  const char *msg = "Sending...";
  static uint32_t lastLcsPoll;
  if (arg < lcsCancel)
  {
    lastLcsPoll = millis();
    if ((void *)item == (void *)&luaBind)
    {
      msg = "Binding...";
      EnterBindingMode();
    }
    sendLuaCommandResponse((struct luaItem_command *)item, lcsExecuting, msg);
  } /* if doExecute */
  else if(arg == lcsCancel || ((millis() - lastLcsPoll)> BINDINGTIMEOUTMS))
  {
    luadevUpdateStrings();
    sendLuaCommandResponse((struct luaItem_command *)item, lcsIdle, STR_EMPTYSPACE);
  }
}

/***
 * @brief: Update the display strings
 * The bad/good item is hidden on our Lua and only displayed in other systems that don't poll our status
 * Called from luaRegisterDevicePingCallback
 ****/
static void luadevUpdateStrings()
{
  itoa(CRSFHandset::BadPktsCountResult, luaBadGoodString, 10);
  strcat(luaBadGoodString, "/");
  itoa(CRSFHandset::GoodPktsCountResult, luaBadGoodString + strlen(luaBadGoodString), 10);
  luaUpdateMAC();
}

static void registerLuaParameters()
{
  registerLUAParameter(&luaModelMAC);

  registerLUAParameter(&luaBind, &luahandSimpleSendCmd);
  
  registerLUAParameter(&luaInfo);
  if (strlen(version) < 21) {
    strlcpy(version_domain, version, 21);
    strlcat(version_domain, " ", sizeof(version_domain));
  } else {
    strlcpy(version_domain, version, 18);
    strlcat(version_domain, "... ", sizeof(version_domain));
  }
  strlcat(version_domain, "ESP-NOW", sizeof(version_domain));
  registerLUAParameter(&luaELRSversion);
}

static int event()
{
  if (connectionState > FAILURE_STATES)
  {
    return DURATION_NEVER;
  }

  return DURATION_IMMEDIATELY;
}

/***
 * @brief: Update the dynamic strings
 ***/
void luaUpdateMAC()
{
  uint64_t mac = config.GetModelMAC();
  if (mac == 0)
  {
    sprintf(modelMACca, "Model not bound");
  }
  else
  {
    uint8_t modelMAC[6];
    modelMAC[0] = (uint8_t)((mac & 0x0000FF0000000000)>>40);
    modelMAC[1] = (uint8_t)((mac & 0x000000FF00000000)>>32);
    modelMAC[2] = (uint8_t)((mac & 0x00000000FF000000)>>24);
    modelMAC[3] = (uint8_t)((mac & 0x0000000000FF0000)>>16);
    modelMAC[4] = (uint8_t)((mac & 0x000000000000FF00)>>8);
    modelMAC[5] = (uint8_t)(mac & 0x00000000000000FF);

    snprintf(modelMACca, sizeof(modelMACca),
      "%02X:%02X:%02X:%02X:%02X:%02X",
      modelMAC[0], modelMAC[1], modelMAC[2], modelMAC[3], modelMAC[4], modelMAC[5]);
  }
  sendELRSstatus();
}

static int timeout()
{
  luaHandleUpdateParameter();
  return DURATION_IMMEDIATELY;
}

static int start()
{
  if (connectionState > FAILURE_STATES)
  {
    return DURATION_NEVER;
  }
  handset->registerParameterUpdateCallback(luaParamUpdateReq);
  registerLuaParameters();

  setLuaStringValue(&luaInfo, luaBadGoodString);
  luaRegisterDevicePingCallback(&luadevUpdateStrings);

  event();
  return DURATION_IMMEDIATELY;
}

device_t LUA_device = {
  .initialize = nullptr,
  .start = start,
  .event = event,
  .timeout = timeout,
  .subscribe = EVENT_ALL
};
