# CyberBrick ESP-NOW transmitter & receiver

<img width="898" height="470" alt="EdgeTX radios control CyberBrick models" src="https://github.com/user-attachments/assets/a372add2-57e0-49bd-9552-04f00adca2f2" />

This project builds upon my previous work (https://github.com/rotorman/CyberBrick_Controller_Core/tree/espnowdemo/src/community/app_espnow), where I added via custom MicroPython scripts an ESP-NOW communication as an alternative to the original intuitively usable, but from features more limited, original [CyberBrick](https://wiki.bambulab.com/en/makerworld/cyberbrick/intro) codebase.

The code in this repository contains a custom open-source firmware development for a large number of [ExpressLRS](https://github.com/ExpressLRS/ExpressLRS/) RF module(s) and custom MicroPython scripts for the [Bambu Lab's/MakerWorld's CyberBrick Core](https://eu.store.bambulab.com/de/products/multi-function-controller-core-1pcs) receiver modules in order to talk to each other w/o any hardware modifications needed. With the custom firmware from this project flashed, a radio transmitter, running open-source [EdgeTX](https://edgetx.org/) firmware, and equipped with an ExpressLRS RF module, talks via [ESP-NOW](https://www.espressif.com/en/solutions/low-power-solutions/esp-now) wireless protocol with the [CyberBrick Core](https://eu.store.bambulab.com/de/products/multi-function-controller-core-1pcs) in the [3D printed models](https://makerworld.com/de/search/models?keyword=cyberbrick).

<img alt="EdgeTXplusCyberBrickAreGreatFun" src="https://github.com/user-attachments/assets/dae157e8-c7b3-4e4b-a221-7fbd8e9f20e9" width="800px" />

A brief video showing the code in action:

[![Video showing EdgeTX radio controlling CyberBrick model](https://github.com/user-attachments/assets/e9a77cc4-9c01-408b-a0df-a8bd3900f830)](https://www.youtube.com/watch?v=XFTqVabXlMM)

You can find the C++ source code to flash numerous internal and external ExpressLRS modules under [transmitterFW](https://github.com/rotorman/CyberBrick_ESPNOW/tree/main/transmitterFW) subfolder and the MicroPython scripts for the CyberBrick Core modules under the [receiverPY](https://github.com/rotorman/CyberBrick_ESPNOW/tree/main/receiverPY) subfolder. [transmitterLua](https://github.com/rotorman/CyberBrick_ESPNOW/tree/main/transmitterLua) subfolder holds the Lua script for EdgeTX radios in order to bind/pair the radio transmitter module with CyberBrick models.

The firmware can be built and flashed into the hardware using [Visual Studio Code](https://code.visualstudio.com/) and [PlatformIO](https://platformio.org/) extension paired with an [Arduino framework](https://docs.platformio.org/en/latest/frameworks/arduino.html) for [Espressif ESP32](https://docs.platformio.org/en/latest/platforms/espressif32.html#platform-espressif32) platform, similar to the development of the [ExpressLRS firmware](https://www.expresslrs.org/software/toolchain-install/).

The MicroPython scripts can be copied to the CyberBrick Core with any REPL capable editor, such as [Arduino Lab for MicroPython](https://labs.arduino.cc/en/labs/micropython) or [Thonny](https://thonny.org/).

## Compatible radios
The code should work with any (internal or external) [ExpressLRS transmitter module](https://www.expresslrs.org/hardware/hardware-selection/#transmitter-selection) with an ESP32 or ESP32-S3 microcontroller paired with an arbitrary [EdgeTX](https://edgetx.org/) handset able to talk with ExpressLRS module(s).

I have tested the code with RadioMaster TX16s mkII with internal ExpressLRS module, RadioMaster MT12 with internal 2.4 GHz ExpressLRS module, Jumper T15 with internal 2.4 GHz ExpressLRS module, external HappyModel ES24TX Pro module, external RadioMaster Ranger module and also with an ESP32 development kit (ESP32DevKitCv4).

## Flashing
Flashing the internal ExpressLRS module(s) is possible via EdgeTX passthrough (triggered in the background while uploading firmware from VSCode and PlatformIO by [Python script](https://github.com/rotorman/CyberBrick_ESPNOW/transmitterFW/python/EdgeTXpassthrough.py)). External modules can be flashed via UART. Some (external) modules come with an integrated USB-to-serial adapter, others might need an external adapter, like [FTDI](https://www.sparkfun.com/ftdi-cable-5v-vcc-3-3v-i-o.html) or [Silabs CP2102](https://betafpv.com/collections/expresslrs-series-accessories/products/expresslrs-recovery-dongle), to flash them.

## Credits
Large parts of the code used in this repository stem from the wonderful [ExpressLRS project](https://github.com/ExpressLRS/ExpressLRS/).

<img src="https://www.expresslrs.org/assets/external/raw.githubusercontent.com/ExpressLRS/ExpressLRS-hardware/master/img/hardware.png" height="50px"> <img src="https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2020/01/esp-now-logo.png" height="50px"> <img src="https://avatars.githubusercontent.com/u/64278475" height="50px"> <img src="https://blog.bambulab.com/content/images/size/w320/2025/03/Frame-62.png" height="50px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/9/9a/Visual_Studio_Code_1.35_icon.svg/250px-Visual_Studio_Code_1.35_icon.svg.png" height="50px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/c/cd/PlatformIO_logo.svg/500px-PlatformIO_logo.svg.png" height="50px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/8/87/Arduino_Logo.svg/330px-Arduino_Logo.svg.png" height="50px"> <img src="https://raw.githubusercontent.com/EdgeTX/edgetx/refs/heads/main/companion/src/images/icon.png" height="50px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/4/4e/Micropython-logo.svg/500px-Micropython-logo.svg.png" height="50px">
