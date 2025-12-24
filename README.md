# CyberBrick ESP-NOW transmitter & receiver

<img width="898" height="470" alt="EdgeTX radios control CyberBrick models" src="https://github.com/user-attachments/assets/a372add2-57e0-49bd-9552-04f00adca2f2" />

This project builds upon my previous work (https://github.com/rotorman/CyberBrick_Controller_Core/tree/espnowdemo/src/community/app_espnow), where I added via custom MicroPython scripts an ESP-NOW communication as an alternative to the original intuitively usable, but from features more limited, original [CyberBrick](https://wiki.bambulab.com/en/makerworld/cyberbrick/intro) codebase.

The code in this repository contains a custom open-source firmware development for a large number of [ExpressLRS](https://github.com/ExpressLRS/ExpressLRS/) RF module(s) and custom MicroPython scripts for the [Bambu Lab's/MakerWorld's CyberBrick Core](https://eu.store.bambulab.com/de/products/multi-function-controller-core-1pcs) receiver modules in order to talk to each other w/o any hardware modifications needed. With the custom firmware from this project flashed, a radio transmitter, running open-source [EdgeTX](https://edgetx.org/) firmware, and equipped with an ExpressLRS RF module, talks via [ESP-NOW](https://www.espressif.com/en/solutions/low-power-solutions/esp-now) wireless protocol with the [CyberBrick Core](https://eu.store.bambulab.com/de/products/multi-function-controller-core-1pcs) in the [3D printed models](https://makerworld.com/de/search/models?keyword=cyberbrick).

<img alt="EdgeTXplusCyberBrickAreGreatFun" src="https://github.com/user-attachments/assets/dae157e8-c7b3-4e4b-a221-7fbd8e9f20e9" width="800px" />

A brief video showing the code in action:

[![Video showing EdgeTX radio controlling CyberBrick model](https://github.com/user-attachments/assets/06b0f400-b9bc-4b4c-b449-c6c62e31536d)](https://www.youtube.com/watch?v=XFTqVabXlMM)

A longer video showing the steps required to get everything setup:

[![Step-by-step tutorial on how-to control CyberBrick models from an EdgeTX R/C handset](https://github.com/user-attachments/assets/ffa889b2-61e8-4d91-8037-4cc81470f3fb)](https://www.youtube.com/watch?v=SotQRQ-ImwE)

You can find the C++ source code to flash numerous internal and external ExpressLRS modules under [transmitterFW](https://github.com/rotorman/CyberBrick_ESPNOW/tree/main/transmitterFW) subfolder and the MicroPython scripts for the CyberBrick Core modules under the [receiverPY](https://github.com/rotorman/CyberBrick_ESPNOW/tree/main/receiverPY) subfolder.

The firmware can be built and flashed into the hardware using [Visual Studio Code](https://code.visualstudio.com/) and [PlatformIO](https://platformio.org/) extension paired with an [Arduino framework](https://docs.platformio.org/en/latest/frameworks/arduino.html) for [Espressif ESP32](https://docs.platformio.org/en/latest/platforms/espressif32.html#platform-espressif32) platform, similar to the development of the [ExpressLRS firmware](https://www.expresslrs.org/software/toolchain-install/).

The MicroPython scripts can be copied to the CyberBrick Core with any REPL capable editor, such as [Arduino Lab for MicroPython](https://labs.arduino.cc/en/labs/micropython) or [Thonny](https://thonny.org/).

## Compatible radios
The code should work with the following handsets running [EdgeTX](https://edgetx.org/) (radios listed alphabetically):

* [HelloRadioSky V14](https://www.helloradiosky.com/)
* [HelloRadioSky V16](https://www.helloradiosky.com/)
* [Jumper T-14 with 900 MHz and 2.4 GHz internal ELRS modules](https://www.jumper-rc.com/transmitters/t14-3/)
* [Jumper T-15 with 900 MHz and 2.4 GHz internal ELRS modules](https://www.jumper-rc.com/transmitters/t15/)
* [Jumper T20 and T20v2, both with 900MHz and 2.4 GHz internal ELRS modules and T20 Gemini](https://www.jumper-rc.com/transmitters/t20-2/)
* [Jumper T-Lite](https://www.jumper-rc.com/transmitters/t-lite-v2/)
* [Jumper T-Pro with 900 MHz and 2.4 GHz internal ELRS modules](https://www.jumper-rc.com/transmitters/)
* [Jumper T-Pro S with 900 MHz and 2.4 GHz internal ELRS modules](https://www.jumper-rc.com/transmitters/t-pro-s-2/)
* [RadioMaster Boxer with internal ELRS module](https://radiomasterrc.com/products/boxer-radio-controller-m2)
* [RadioMaster MT12 with internal ELRS module](https://radiomasterrc.com/products/mt12-surface-radio-controller)
* [RadioMaster Pocket with internal ELRS module](https://radiomasterrc.com/products/pocket-radio-controller-m2)
* [RadioMaster TX12 mkII with internal ELRS module](https://radiomasterrc.com/products/tx12-mark-ii-radio-controller)
* [RadioMaster TX16s mkII with internal ELRS module](https://radiomasterrc.com/products/tx16s-mark-ii-radio-controller)
* [RadioMaster Zorro with internal ELRS module](https://radiomasterrc.com/products/zorro-radio-controller?variant=46486367371456)

The code supports following external RF modules (modules listed alphabetically):

* [HappyModel ES24TX](https://www.happymodel.cn/index.php/2021/04/12/happymodel-2-4g-expresslrs-elrs-micro-tx-module-es24tx/)
* [HappyModel ES24TX Pro](https://www.happymodel.cn/index.php/2021/11/11/happymodel-es24tx-pro-micro-expresslrs-elrs-tx-module/)
* [Jumper AION Nano](https://www.jumper-rc.com/modules/jumper-2-4g-elrs-tx-module/)
* [RadioMaster Ranger](https://radiomasterrc.com/products/ranger-2-4ghz-elrs-module)
* [RadioMaster Ranger Micro](https://radiomasterrc.com/products/ranger-micro-2-4ghz-elrs-module)
* [RadioMaster Ranger Nano](https://radiomasterrc.com/products/ranger-nano-2-4ghz-elrs-module)

Do note, that I at present time, I have tested the code only with RadioMaster TX16s mkII with internal ExpressLRS module, RadioMaster MT12 with internal 2.4 GHz ExpressLRS module, with Jumper T15 with internal 2.4 GHz ExpressLRS module, HappyModel ES24TX Pro, RadioMaster Ranger and ESP32 development kit (ESP32DevKitCv4).

## Flashing
Flashing the internal ExpressLRS module(s) is possible via EdgeTX passthrough (triggered in the background while uploading firmware from VSCode and PlatformIO by [Python script](https://github.com/rotorman/CyberBrick_ESPNOW/transmitterFW/python/EdgeTXpassthrough.py)). External modules can be flashed via UART (via USB-to-serial adapter). Some (external) modules come with an integrated USB-to-serial adapter, others might need an external adapter, like [FTDI](https://www.sparkfun.com/ftdi-cable-5v-vcc-3-3v-i-o.html) or [Silabs CP2102](https://betafpv.com/collections/expresslrs-series-accessories/products/expresslrs-recovery-dongle), to flash them.

## Credits
Large parts of the code used in this repository stem from the wonderful [ExpressLRS project](https://github.com/ExpressLRS/ExpressLRS/).

<img src="https://raw.githubusercontent.com/ExpressLRS/ExpressLRS-hardware/master/img/hardware.png" height="50px"> <img src="https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2020/01/esp-now-logo.png" height="50px"> <img src="https://avatars.githubusercontent.com/u/64278475" height="50px"> <img src="https://blog.bambulab.com/content/images/size/w320/2025/03/Frame-62.png" height="50px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/9/9a/Visual_Studio_Code_1.35_icon.svg/250px-Visual_Studio_Code_1.35_icon.svg.png" height="50px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/c/cd/PlatformIO_logo.svg/500px-PlatformIO_logo.svg.png" height="50px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/8/87/Arduino_Logo.svg/330px-Arduino_Logo.svg.png" height="50px"> <img src="https://raw.githubusercontent.com/EdgeTX/edgetx/refs/heads/main/companion/src/images/icon.png" height="50px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/4/4e/Micropython-logo.svg/500px-Micropython-logo.svg.png" height="50px">
