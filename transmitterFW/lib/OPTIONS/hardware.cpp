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

#include "options.h"
#include "helpers.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

typedef enum {
    INT,
    BOOL,
    FLOAT,
    ARRAY,
    COUNT
} datatype_t;

static const struct {
    const nameType position;
    const char *name;
    const datatype_t type;
} fields[] = {
    {HARDWARE_serial_rx, "serial_rx", INT},
    {HARDWARE_serial_tx, "serial_tx", INT},
    {HARDWARE_serial1_rx, "serial1_rx", INT},
    {HARDWARE_serial1_tx, "serial1_tx", INT},
    {HARDWARE_radio_busy, "radio_busy", INT},
    {HARDWARE_radio_busy_2, "radio_busy_2", INT},
    {HARDWARE_radio_dio0, "radio_dio0", INT},
    {HARDWARE_radio_dio0_2, "radio_dio0_2", INT},
    {HARDWARE_radio_dio1, "radio_dio1", INT},
    {HARDWARE_radio_dio1_2, "radio_dio1_2", INT},
    {HARDWARE_radio_miso, "radio_miso", INT},
    {HARDWARE_radio_mosi, "radio_mosi", INT},
    {HARDWARE_radio_nss, "radio_nss", INT},
    {HARDWARE_radio_nss_2, "radio_nss_2", INT},
    {HARDWARE_radio_rst, "radio_rst", INT},
    {HARDWARE_radio_rst_2, "radio_rst_2", INT},
    {HARDWARE_radio_sck, "radio_sck", INT},
    {HARDWARE_radio_dcdc, "radio_dcdc", BOOL},
    {HARDWARE_radio_rfo_hf, "radio_rfo_hf", BOOL},
    {HARDWARE_radio_rfsw_ctrl, "radio_rfsw_ctrl", ARRAY},
    {HARDWARE_radio_rfsw_ctrl_count, "radio_rfsw_ctrl", COUNT},
    {HARDWARE_ant_ctrl, "ant_ctrl", INT},
    {HARDWARE_ant_ctrl_compl, "ant_ctrl_compl", INT},
    {HARDWARE_power_enable, "power_enable", INT},
    {HARDWARE_power_apc2, "power_apc2", INT},
    {HARDWARE_power_rxen, "power_rxen", INT},
    {HARDWARE_power_txen, "power_txen", INT},
    {HARDWARE_power_rxen_2, "power_rxen_2", INT},
    {HARDWARE_power_txen_2, "power_txen_2", INT},
    {HARDWARE_power_lna_gain, "power_lna_gain", INT},
    {HARDWARE_power_min, "power_min", INT},
    {HARDWARE_power_high, "power_high", INT},
    {HARDWARE_power_max, "power_max", INT},
    {HARDWARE_power_default, "power_default", INT},
    {HARDWARE_power_pdet, "power_pdet", INT},
    {HARDWARE_power_pdet_intercept, "power_pdet_intercept", FLOAT},
    {HARDWARE_power_pdet_slope, "power_pdet_slope", FLOAT},
    {HARDWARE_power_control, "power_control", INT},
    {HARDWARE_power_values, "power_values", ARRAY},
    {HARDWARE_power_values_count, "power_values", COUNT},
    {HARDWARE_power_values2, "power_values2", ARRAY},
    {HARDWARE_power_values_dual, "power_values_dual", ARRAY},
    {HARDWARE_power_values_dual_count, "power_values_dual", COUNT},
    {HARDWARE_joystick, "joystick", INT},
    {HARDWARE_joystick_values, "joystick_values", ARRAY},
    {HARDWARE_five_way1, "five_way1", INT},
    {HARDWARE_five_way2, "five_way2", INT},
    {HARDWARE_five_way3, "five_way3", INT},
    {HARDWARE_button, "button", INT},
    {HARDWARE_button_led_index, "button_led_index", INT},
    {HARDWARE_button2, "button2", INT},
    {HARDWARE_button2_led_index, "button2_led_index", INT},
    {HARDWARE_led, "led", INT},
    {HARDWARE_led_blue, "led_blue", INT},
    {HARDWARE_led_blue_invert, "led_blue_invert", BOOL},
    {HARDWARE_led_green, "led_green", INT},
    {HARDWARE_led_green_invert, "led_green_invert", BOOL},
    {HARDWARE_led_green_red, "led_green_red", INT},
    {HARDWARE_led_red, "led_red", INT},
    {HARDWARE_led_red_invert, "led_red_invert", BOOL},
    {HARDWARE_led_red_green, "led_red_green", INT},
    {HARDWARE_led_rgb, "led_rgb", INT},
    {HARDWARE_led_rgb_isgrb, "led_rgb_isgrb", BOOL},
    {HARDWARE_ledidx_rgb_status, "ledidx_rgb_status", ARRAY},
    {HARDWARE_ledidx_rgb_status_count, "ledidx_rgb_status", COUNT},
    {HARDWARE_ledidx_rgb_boot, "ledidx_rgb_boot", ARRAY},
    {HARDWARE_ledidx_rgb_boot_count, "ledidx_rgb_boot", COUNT},
    {HARDWARE_screen_cs, "screen_cs", INT},
    {HARDWARE_screen_dc, "screen_dc", INT},
    {HARDWARE_screen_mosi, "screen_mosi", INT},
    {HARDWARE_screen_rst, "screen_rst", INT},
    {HARDWARE_screen_sck, "screen_sck", INT},
    {HARDWARE_screen_sda, "screen_sda", INT},
    {HARDWARE_screen_type, "screen_type", INT},
    {HARDWARE_screen_reversed, "screen_reversed", BOOL},
    {HARDWARE_screen_bl, "screen_bl", INT},
    {HARDWARE_i2c_scl, "i2c_scl", INT},
    {HARDWARE_i2c_sda, "i2c_sda", INT},
    {HARDWARE_misc_gsensor_int, "misc_gsensor_int", INT},
    {HARDWARE_misc_buzzer, "misc_buzzer", INT},
    {HARDWARE_misc_fan_en, "misc_fan_en", INT},
    {HARDWARE_misc_fan_pwm, "misc_fan_pwm", INT},
    {HARDWARE_misc_fan_tacho, "misc_fan_tacho", INT},
    {HARDWARE_misc_fan_speeds, "misc_fan_speeds", ARRAY},
    {HARDWARE_misc_fan_speeds_count, "misc_fan_speeds", COUNT},
    {HARDWARE_gsensor_stk8xxx, "gsensor_stk8xxx", BOOL},
    {HARDWARE_thermal_lm75a, "thermal_lm75a", BOOL},
    {HARDWARE_pwm_outputs, "pwm_outputs", ARRAY},
    {HARDWARE_pwm_outputs_count, "pwm_outputs", COUNT},
    {HARDWARE_vbat, "vbat", INT},
    {HARDWARE_vbat_offset, "vbat_offset", INT},
    {HARDWARE_vbat_scale, "vbat_scale", INT},
    {HARDWARE_vbat_atten, "vbat_atten", INT},
};

typedef union {
    int int_value;
    bool bool_value;
    float float_value;
    int16_t *array_value;
} data_holder_t;

static data_holder_t hardware[HARDWARE_LAST];
static String builtinHardwareConfig;

String& getHardware()
{
    File file = SPIFFS.open("/hardware.json", "r");
    if (!file || file.isDirectory())
    {
        if (file)
        {
            file.close();
        }
        // Try JSON at the end of the firmware
        return builtinHardwareConfig;
    }
    builtinHardwareConfig = file.readString();
    return builtinHardwareConfig;
}

static void hardware_ClearAllFields()
{
    for (size_t i=0 ; i<ARRAY_SIZE(fields) ; i++) {
        switch (fields[i].type) {
            case INT:
                hardware[fields[i].position].int_value = -1;
                break;
            case BOOL:
                hardware[fields[i].position].bool_value = false;
                break;
            case FLOAT:
                hardware[fields[i].position].float_value = 0.0;
                break;
            case ARRAY:
                hardware[fields[i].position].array_value = nullptr;
                break;
            case COUNT:
                hardware[fields[i].position].int_value = 0;
                break;
        }
    }
}

static void hardware_LoadFieldsFromDoc(JsonDocument &doc)
{
    for (size_t i=0 ; i<ARRAY_SIZE(fields) ; i++) {
        if (doc.containsKey(fields[i].name)) {
            switch (fields[i].type) {
                case INT:
                    hardware[fields[i].position].int_value = doc[fields[i].name];
                    break;
                case BOOL:
                    hardware[fields[i].position].bool_value = doc[fields[i].name];
                    break;
                case FLOAT:
                    hardware[fields[i].position].float_value = doc[fields[i].name];
                    break;
                case ARRAY:
                    {
                        JsonArray array = doc[fields[i].name].as<JsonArray>();
                        hardware[fields[i].position].array_value = new int16_t[array.size()];
                        copyArray(doc[fields[i].name], hardware[fields[i].position].array_value, array.size());
                    }
                    break;
                case COUNT:
                    {
                        JsonArray array = doc[fields[i].name].as<JsonArray>();
                        hardware[fields[i].position].int_value = array.size();
                    }
                    break;
            }
        }
    }
}

void init_unused_peripherals()
{
// Radio
#if defined(GPIO_PIN_BUSY)
    pinMode(GPIO_PIN_BUSY, INPUT);
#endif

#if defined(GPIO_PIN_BUSY_2)
    pinMode(GPIO_PIN_BUSY_2, INPUT);
#endif

#if defined(GPIO_PIN_DIO0)
    pinMode(GPIO_PIN_DIO0, INPUT);
#endif

#if defined(GPIO_PIN_DIO0_2)
    pinMode(GPIO_PIN_DIO0_2, INPUT);
#endif

#if defined(GPIO_PIN_DIO1)
    pinMode(GPIO_PIN_DIO1, INPUT);
#endif

#if defined(GPIO_PIN_DIO1_2)
    pinMode(GPIO_PIN_DIO1_2, INPUT);
#endif

#if defined(GPIO_PIN_MISO)
    pinMode(GPIO_PIN_MISO, INPUT);
#endif

#if defined(GPIO_PIN_MOSI)
    pinMode(GPIO_PIN_MOSI, OUTPUT);
    digitalWrite(GPIO_PIN_MOSI, LOW);
#endif

#if defined(GPIO_PIN_NSS)
    pinMode(GPIO_PIN_NSS, OUTPUT);
    digitalWrite(GPIO_PIN_NSS, HIGH); // RF chip ChipSelect is low active, keep inactive
#endif

#if defined(GPIO_PIN_NSS_2)
    pinMode(GPIO_PIN_NSS_2, OUTPUT);
    digitalWrite(GPIO_PIN_NSS_2, HIGH); // RF chip ChipSelect is low active, keep inactive
#endif

#if defined(GPIO_PIN_RST)
    pinMode(GPIO_PIN_RST, OUTPUT);
    digitalWrite(GPIO_PIN_RST, LOW); // Keep radio in reset
#endif

#if defined(GPIO_PIN_RST_2)
    pinMode(GPIO_PIN_RST_2, OUTPUT);
    digitalWrite(GPIO_PIN_RST_2, LOW); // Keep radio in reset
#endif

#if defined(GPIO_PIN_SCK)
    pinMode(GPIO_PIN_SCK, OUTPUT);
    digitalWrite(GPIO_PIN_SCK, LOW); // SPI mode 0 (CPOL=0, CPHA=0) default state is low
#endif

// Radio Antenna
#if defined(GPIO_PIN_ANT_CTRL)
    pinMode(GPIO_PIN_ANT_CTRL, OUTPUT);
    digitalWrite(GPIO_PIN_ANT_CTRL, HIGH); // Select first antenna by default
#endif

#if defined(GPIO_PIN_ANT_CTRL_COMPL)
    pinMode(GPIO_PIN_ANT_CTRL_COMPL, OUTPUT);
    digitalWrite(GPIO_PIN_ANT_CTRL_COMPL, LOW); // Select first antenna by default
#endif

// Radio power
#if defined(GPIO_PIN_PA_ENABLE)
    pinMode(GPIO_PIN_PA_ENABLE, OUTPUT);
    digitalWrite(GPIO_PIN_PA_ENABLE, LOW); // Keep RF PA off
#endif

#if defined(GPIO_PIN_RFamp_APC2) && not defined(PLATFORM_ESP32_S3) // ESP32-S3 does not have a DAC
    dacWrite(GPIO_PIN_RFamp_APC2, 0);
#endif

#if defined(GPIO_PIN_RX_ENABLE)
    pinMode(GPIO_PIN_RX_ENABLE, OUTPUT);
    digitalWrite(GPIO_PIN_RX_ENABLE, LOW); // Keep RF PA receive off
#endif

#if defined(GPIO_PIN_TX_ENABLE)
    pinMode(GPIO_PIN_TX_ENABLE, OUTPUT);
    digitalWrite(GPIO_PIN_TX_ENABLE, LOW); // Keep RF PA transmit off
#endif

#if defined(GPIO_PIN_RX_ENABLE_2)
    pinMode(GPIO_PIN_RX_ENABLE_2, OUTPUT);
    digitalWrite(GPIO_PIN_RX_ENABLE_2, LOW); // Keep RF PA receive off
#endif

#if defined(GPIO_PIN_TX_ENABLE_2)
    pinMode(GPIO_PIN_TX_ENABLE_2, OUTPUT);
    digitalWrite(GPIO_PIN_TX_ENABLE_2, LOW); // Keep RF PA transmit off
#endif

#if defined(GPIO_PIN_PA_PDET)
    analogSetPinAttenuation(GPIO_PIN_PA_PDET, ADC_0db);
    analogRead(GPIO_PIN_PA_PDET);
#endif

// Input
#if defined(GPIO_PIN_JOYSTICK)
    analogRead(GPIO_PIN_JOYSTICK);
#endif

#if defined(GPIO_PIN_FIVE_WAY_INPUT1)
    pinMode(GPIO_PIN_FIVE_WAY_INPUT1, INPUT);
#endif

#if defined(GPIO_PIN_FIVE_WAY_INPUT2)
    pinMode(GPIO_PIN_FIVE_WAY_INPUT2, INPUT);
#endif

#if defined(GPIO_PIN_FIVE_WAY_INPUT3)
    pinMode(GPIO_PIN_FIVE_WAY_INPUT3, INPUT);
#endif

#if defined(GPIO_PIN_BUTTON)
    pinMode(GPIO_PIN_BUTTON, INPUT);
#endif

#if defined(USER_BUTTON_LED)
    pinMode(USER_BUTTON_LED, OUTPUT);
    digitalWrite(USER_BUTTON_LED, LOW);
#endif

#if defined(GPIO_PIN_BUTTON2)
    pinMode(GPIO_PIN_BUTTON2, INPUT);
#endif

#if defined(USER_BUTTON2_LED)
    pinMode(USER_BUTTON2_LED, OUTPUT);
    digitalWrite(USER_BUTTON2_LED, LOW);
#endif

// Lighting
#if defined(GPIO_PIN_LED_BLUE)
    pinMode(GPIO_PIN_LED_BLUE, OUTPUT);
    digitalWrite(GPIO_PIN_LED_BLUE, LOW ^ GPIO_LED_BLUE_INVERTED);
#endif

#if defined(GPIO_PIN_LED_GREEN)
    pinMode(GPIO_PIN_LED_GREEN, OUTPUT);
    digitalWrite(GPIO_PIN_LED_GREEN, LOW ^ GPIO_LED_GREEN_INVERTED);
#endif

#if defined(GPIO_PIN_LED_RED)
    pinMode(GPIO_PIN_LED_RED, OUTPUT);
    digitalWrite(GPIO_PIN_LED_RED, LOW ^ GPIO_LED_RED_INVERTED);
#endif

#if defined(GPIO_PIN_LED_WS2812)
    pinMode(GPIO_PIN_LED_WS2812, OUTPUT);
    digitalWrite(GPIO_PIN_LED_WS2812, LOW);
#endif

// OLED/TFT
#if defined(GPIO_PIN_SCREEN_CS)
    pinMode(GPIO_PIN_SCREEN_CS, OUTPUT);
    digitalWrite(GPIO_PIN_SCREEN_CS, HIGH); // SCREEN ChipSelect is low active, keep inactive
#endif

#if defined(GPIO_PIN_SCREEN_DC)
    pinMode(GPIO_PIN_SCREEN_DC, OUTPUT);
    digitalWrite(GPIO_PIN_SCREEN_DC, HIGH); // Default to data mode (data/control)
#endif

#if defined(GPIO_PIN_SCREEN_MOSI)
    pinMode(GPIO_PIN_SCREEN_MOSI, OUTPUT);
    digitalWrite(GPIO_PIN_SCREEN_MOSI, LOW);
#endif

#if defined(GPIO_PIN_BUTTON2)
    pinMode(GPIO_PIN_BUTTON2, INPUT);
#endif

#if defined(GPIO_PIN_SCREEN_RST)
    pinMode(GPIO_PIN_SCREEN_RST, OUTPUT);
    digitalWrite(GPIO_PIN_SCREEN_RST, HIGH); // Set display into reset state
#endif

#if defined(GPIO_PIN_SCREEN_SCK)
    pinMode(GPIO_PIN_SCREEN_SCK, OUTPUT_OPEN_DRAIN | PULLUP);
    digitalWrite(GPIO_PIN_SCREEN_SCK, HIGH); // Default safe state for SPI CLK and I2C SCL
#endif

#if defined(GPIO_PIN_SCREEN_SDA)
    pinMode(GPIO_PIN_SCREEN_SDA, OUTPUT_OPEN_DRAIN | PULLUP);
    digitalWrite(GPIO_PIN_SCREEN_SDA, HIGH); // Default safe state for I2C SDA
#endif

#if defined(GPIO_PIN_SCREEN_BL) // Backlight
    pinMode(GPIO_PIN_SCREEN_BL, OUTPUT);
    digitalWrite(GPIO_PIN_SCREEN_BL, HIGH); // OFF = HIGH
#endif

// I2C
#if defined(GPIO_PIN_SCL)
    pinMode(GPIO_PIN_SCL, OUTPUT_OPEN_DRAIN | PULLUP);
    digitalWrite(GPIO_PIN_SCL, HIGH); // Default safe state for I2C SCL
#endif

#if defined(GPIO_PIN_SDA)
    pinMode(GPIO_PIN_SDA, OUTPUT_OPEN_DRAIN | PULLUP);
    digitalWrite(GPIO_PIN_SDA, HIGH); // Default safe state for I2C SDA
#endif

// Backpack
#if defined(GPIO_PIN_DEBUG_RX)
    pinMode(GPIO_PIN_DEBUG_RX, INPUT);
#endif

#if defined(GPIO_PIN_DEBUG_TX)
    pinMode(GPIO_PIN_DEBUG_TX, OUTPUT);
    digitalWrite(GPIO_PIN_DEBUG_TX, LOW);
#endif

#if defined(GPIO_PIN_BACKPACK_BOOT)
    pinMode(GPIO_PIN_BACKPACK_BOOT, OUTPUT);		
    digitalWrite(GPIO_PIN_BACKPACK_BOOT, LOW); // Do not put Backpack into bootloader mode
#endif

#if defined(GPIO_PIN_BACKPACK_EN)
    pinMode(GPIO_PIN_BACKPACK_EN, OUTPUT);
    digitalWrite(GPIO_PIN_BACKPACK_EN, LOW); // Turn backpack off
#endif

// Misc sensors & things
#if defined(GPIO_PIN_FAN_EN)
    pinMode(GPIO_PIN_FAN_EN, OUTPUT);
    digitalWrite(GPIO_PIN_FAN_EN, LOW);
#endif

#if defined(GPIO_PIN_FAN_PWM)
    pinMode(GPIO_PIN_FAN_PWM, OUTPUT);
    digitalWrite(GPIO_PIN_FAN_PWM, LOW);
#endif

#if defined(GPIO_PIN_FAN_TACHO)
    pinMode(GPIO_PIN_FAN_TACHO, INPUT);
#endif

#if defined(GPIO_PIN_GSENSOR_INT)
    pinMode(GPIO_PIN_GSENSOR_INT, INPUT_PULLUP);
#endif
}

bool hardware_init(EspFlashStream &strmFlash)
{
    hardware_ClearAllFields();
    builtinHardwareConfig.clear();

    Stream *strmSrc;
    JsonDocument doc;
    File file = SPIFFS.open("/hardware.json", "r");
    if (!file || file.isDirectory()) {
        constexpr size_t hardwareConfigOffset = ELRSOPTS_PRODUCTNAME_SIZE + ELRSOPTS_DEVICENAME_SIZE + ELRSOPTS_OPTIONS_SIZE;
        strmFlash.setPosition(hardwareConfigOffset);
        if (!options_HasStringInFlash(strmFlash))
        {
            return false;
        }

        strmSrc = &strmFlash;
    }
    else
    {
        strmSrc = &file;
    }

    DeserializationError error = deserializeJson(doc, *strmSrc);
    if (error)
    {
        return false;
    }
    serializeJson(doc, builtinHardwareConfig);
    hardware_LoadFieldsFromDoc(doc);
    init_unused_peripherals();
    return true;
}

const int hardware_pin(nameType name)
{
    return hardware[name].int_value;
}

const bool hardware_flag(nameType name)
{
    return hardware[name].bool_value;
}

const int hardware_int(nameType name)
{
    return hardware[name].int_value;
}

const float hardware_float(nameType name)
{
    return hardware[name].float_value;
}

const int16_t* hardware_i16_array(nameType name)
{
    return hardware[name].array_value;
}

const uint16_t* hardware_u16_array(nameType name)
{
    return (uint16_t *)hardware[name].array_value;
}
