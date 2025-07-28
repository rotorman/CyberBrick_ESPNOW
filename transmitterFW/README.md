# Firmware to flash internal and external ExpressLRS modules

The firmware can be built and flashed into the hardware using [Visual Studio Code](https://code.visualstudio.com/) and [PlatformIO](https://platformio.org/) extension paired with an [Arduino framework](https://docs.platformio.org/en/latest/frameworks/arduino.html) for [Espressif ESP32](https://docs.platformio.org/en/latest/platforms/espressif32.html#platform-espressif32) platform, similar to the development of the ExpressLRS firmware. More about setting up a development toolchain, see: https://www.expresslrs.org/software/toolchain-install/

As a first step you should try to build std. ExpressLRS firmware with Visual Studio Code and PlatformIO. As only if the latter succeeds, it does make sense to continue to load the PlatformIO project from this repository. Be also aware that by opening a new PlatformIO project for the first time, depending on your local configuration, a significant amount of time can pass, before all the necessary frameworks and libraries are downloaded. Keep an eye for progress bars in VSCode and always wait until previous operations are completed, before continuing. Even on a potent new computer and fast Internet connectivity, this can still take many many minutes, as sometimes multiple gigabytes of data need to be downloaded and unpacked.

<img src="https://www.expresslrs.org/assets/external/raw.githubusercontent.com/ExpressLRS/ExpressLRS-hardware/master/img/hardware.png" height="65px"> <img src="https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2020/01/esp-now-logo.png" height="65px"> <img src="https://avatars.githubusercontent.com/u/64278475" height="65px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/9/9a/Visual_Studio_Code_1.35_icon.svg/250px-Visual_Studio_Code_1.35_icon.svg.png" height="65px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/c/cd/PlatformIO_logo.svg/500px-PlatformIO_logo.svg.png" height="65px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/8/87/Arduino_Logo.svg/330px-Arduino_Logo.svg.png" height="65px"> <img src="https://raw.githubusercontent.com/EdgeTX/edgetx/refs/heads/main/companion/src/images/icon.png" height="65px">

The code supports following handsets running [EdgeTX](https://edgetx.org/) (radios listed alphabetically):

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

* [BetaFPV Micro 2.4 GHz](https://betafpv.com/collections/expresslrs-series-accessories/products/elrs-micro-tx-module)
* [BetaFPV Micro 1W 2.4 GHz](https://betafpv.com/collections/expresslrs-series-accessories/products/elrs-micro-tx-module)
* [HappyModel ES24](https://www.happymodel.cn/index.php/2021/04/12/happymodel-2-4g-expresslrs-elrs-micro-tx-module-es24tx/)
* [HappyModel ES24 Pro](https://www.happymodel.cn/index.php/2021/11/11/happymodel-es24tx-pro-micro-expresslrs-elrs-tx-module/)
* [Jumper AION Nano](https://www.jumper-rc.com/modules/jumper-2-4g-elrs-tx-module/)
* [RadioMaster Ranger](https://radiomasterrc.com/products/ranger-2-4ghz-elrs-module)
* [RadioMaster Ranger Micro](https://radiomasterrc.com/products/ranger-micro-2-4ghz-elrs-module)
* [RadioMaster Ranger Nano](https://radiomasterrc.com/products/ranger-nano-2-4ghz-elrs-module)

Flashing the internal ExpressLRS module(s) is possible via EdgeTX passthrough (triggered in the background while uploading firmware from VSCode and PlatformIO by [Python script](https://github.com/rotorman/CyberBrick_ESPNOW/transmitterFW/python/EdgeTXpassthrough.py)). External modules can be flashed via UART (via USB-to-serial adapter).

**NOTE!** In order to successfully bind the EdgeTX radio and the CyberBrick receiver(s), you need to first read out the CyberBrick Core WiFi MAC address(es) and then enter them into the [main.cpp](https://github.com/rotorman/CyberBrick_ESPNOW/blob/main/transmitterFW/src/main.cpp#L41-L43).

Numerous parts of the code used in this repository stem from the wonderful [ExpressLRS project](https://github.com/ExpressLRS/ExpressLRS/).

The original development was carried out using an [ESP32DevKitCv4](https://www.az-delivery.de/en/products/esp-32-dev-kit-c-v4), paired with a radio running [EdgeTX](https://edgetx.org/) firmware. The code is setup for in-circuit-debugging with [ESP-Prog](https://docs.espressif.com/projects/esp-iot-solution/en/latest/hw-reference/ESP-Prog_guide.html) on ESP32DevKitCv4 target. You can find more info about this in the [Wiki section](https://github.com/rotorman/CyberBrick_ESPNOW/wiki/In%E2%80%90Circuit%E2%80%90Debugging), incl. a detailed hookup scheme.
