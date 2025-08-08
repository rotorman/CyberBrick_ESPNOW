# Micropython scripts to flash [CyberBrick Core](https://eu.store.bambulab.com/de/products/multi-function-controller-core-1pcs) receiver modules

Under this folder you can find [ESP-NOW MicroPython](https://makerworld.com/en/cyberbrick/api-doc/library/espnow.html#module-espnow) code for the model/receiver side as an alternative to the original App driven intuitively usable, but from features more limited, [CyberBrick ecosystem](https://eu.store.bambulab.com/de/collections/cyberbrick).

<img src="https://blog.bambulab.com/content/images/size/w320/2025/03/Frame-62.png" height="120px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/4/4e/Micropython-logo.svg/500px-Micropython-logo.svg.png" height="120px"> <img height="200" alt="CyberBrick Core Module (A11)" src="https://github.com/user-attachments/assets/356cfe8e-2753-4c96-b5f2-a7980646a871" /> <img src="https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2020/01/esp-now-logo.png" height="100px">

The examples in this branch currently include code for [truck](https://makerworld.com/de/models/1396031-cyberbrick-official-truck), [forklift](https://makerworld.com/de/models/1395994-cyberbrick-official-forklift) and the [bulldozer by MottN](https://makerworld.com/de/models/1461532-bulldozer-cyberbrick-rc). Use these examples to adapt the code to further models. You can also find a debug script, that prints incoming [CRSF RC channels packet](https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md#0x16-rc-channels-packed-payload) values onto REPL terminal.

What are the benefits of the code in this branch in comparison to the original CyberBrick MicroPython stack:
- Fully open-source with no hidden/frozen MicroPython modules used.
- Smoother control - full 12-bits are used to control the servos and also the motors, instead of only ca. 41 steps for motor and 102 steps for servo angle in original control. Also, the motor control PWM frequency is increased from 50 Hz to 500 Hz.
- Fully customizable NeoPixel LED driver. Actuate the various LED pixels from remote or from some logic combination of other channel states.

Downsides:
- current code is not yet able to control more than 6 PWM outputs simultanously (each servo needs 1 and each brushed motor 2 PWM outputs, so you could for example instead control 2 brushed motors and 2 servos, but not 4 servos and 2 brushed motors at the same time with this codebase). This can be remedied, by moving at least 2 PWM controls to a timer and software PWM based solution, similarly as CyberBrick original code does.
- No fancy intuitive graphical configuration environment, as is the case with CyberBrick apps. Here, the user has to configure the output mapping in receiver side MicroPython code.

The handset, running [EdgeTX](https://edgetx.org/) firmware, sends, via custom ESP-NOW flashed ExpressLRS transmitter module (code in folder [./transmitterFW](https://github.com/rotorman/CyberBrick_ESPNOW/tree/main/transmitterFW)) channel data according to [CRSF specifications](https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md) - [16 proportional channels in 11-bit resolution](https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md#0x16-rc-channels-packed-payload). The channel order, range, mixing and further parameters can be adjusted in the EdgeTX radio.

The most widely used mapping of the first 4 control channels are (Mode 2, AETR):

- ch1: left horizontal (LH) stick
- ch2: left vertical (LV) stick
- ch3: right vertical (RV) stick
- ch4: right horizontal (RH) stick

The mapping for the [truck](https://makerworld.com/de/models/1396031-cyberbrick-official-truck) is:
- Servo1: steering, channel 1 (right horizontal stick)
- Motor1: throttle, channel 3 (left vertical stick)
- NeoPixel_Channel2: driven in code by throttle (LV) and steering (RH)

The mapping for the [forklift](https://makerworld.com/de/models/1395994-cyberbrick-official-forklift) is:
- Servo1: fork up/down (channel 6)
- Motor1: right track (mix of channel 3 (left vertical stick) and channel 1 (right horizontal stick))
- Motor2: left track  (mix of channel 3 (left vertical stick) and channel 1 (right horizontal stick))
- NeoPixel_Channel2: front lights, mode driven by channel 7 (3-way switch), brightness controlled by channel 8

The mapping for the [bulldozer](https://makerworld.com/de/models/1461532-bulldozer-cyberbrick-rc) is:
- Servo1: blade up/down (channel 6)
- Motor1: right track (mix of channel 3 (left vertical stick) and channel 1 (right horizontal stick))
- Motor2: left track  (mix of channel 3 (left vertical stick) and channel 1 (right horizontal stick))
- NeoPixel_Channel1: cabin lights, driven in code by channel 7 (3-way switch), brightness controlled by channel 8
- NeoPixel_Channel2: front lights, driven in code by channel 9 (2-way switch)

Instructions steps for writing the script to your CyberBrick Core:

1. Download or check out (git clone) this repository.
2. If you do not already have a [REPL capable MicroPython development environment setup](https://makerworld.com/en/cyberbrick/api-doc/cyberbrick_core/start/index.html#setting-up-the-development-environment), I suggest you take a look at [Arduino Lab for MicroPython](https://labs.arduino.cc/en/labs/micropython), which in my opinon will be easiest to work with.
Another easy to use option is [Thonny](https://thonny.org/).
3. Hook up the CyberBrick Core receiver to your computer, open Arduino Lab for MicroPython and press Connect. On first start of Arduino lab for MicroPython you are asked to pick a folder where to store the files locally. Provide here the folder, where you extracted or git cloned this repository. You can directly provide the `receiverPY` subfolder of the repo.
4. Open the Files window (in the top right corner of Arduino Lab for MicroPython). You might want to backup your `boot.py` from the board. To do this, select `boot.py` in the left side and then click the button with right arrow icon in the middle of the Arduino Lab for MicroPython to start copying the `boot.py` file from your CyberBrick core to your computer.
5. Navigate to one of the example model subfolders on the right side. Select both files (`boot.py` and the main model MicroPython code) and then click the left arrow icon to copy them to your receiver side CyberBrick Core module. You will be overwriting your stock `boot.py` file while doing so, thus be sure you made a backup into one local folder structure above in the previous step. If not, not much is lost, as that original state of the `boot.py` file is also available on CyberBrick Git repository at: https://raw.githubusercontent.com/CyberBrick-Official/CyberBrick_Controller_Core/refs/heads/master/src/app_rc/boot.py
6. After the files have been copied, power cycle your CyberBrick Core (the power cycling is only necessary once, after the first time copying the scripts from this repository while overwriting the factory `boot.py`), then reconnect and hit the “Reset” button. To validate that you performed all steps so far correctly, you should see the CyberBrick Core MAC address being printed in REPL terminal. If that is not the case, start over, as you likely missed a detail.
7. Do not assemble the CyberBrick Core receiver side module back into the 3D printed model yet, as you will need to pair it with the transmitter using the Bind function of the [Lua script](https://github.com/rotorman/CyberBrick_ESPNOW/tree/main/transmitterLua) running on an EdgeTX radio, while pressing the USER KEY button on the CyberBrick Core. For this, and assuming the transmitter side ExpressLRS module was already correctly flashed and EdgeTX channels configured, power up the EdgeTX radio, and start the Lua script. Select "Bind" in the Lua script and inside 5 seconds press and hold for at least 1 second the USER KEY button on the CyberBrick Core. You should see in REPL messages that the CyberBrick core is broadcasting it's MAC address. If the bind was successful, EdgeTX radio should display the correct MAC address in the Lua script.
8. Press "Disconnect" in the Arduino Lab for MicroPython software to detach it from your receiver core module and disconnect the USB cable from it as well. You can now assemble the CyberBrick Core module into your model.
9. If you wish to bind multiple models to your EdgeTX radio, be sure to create a model profile in EdgeTX for each model and assign a differing receiver number under internal or external RF module settings for each model, before triggering Bind from the Lua. This way the radio will remember the bind info for up to 64 models (this limitation stems from current EdgeTX code). For more about assigning receiver number in EdgeTX see EdgeTX User Manual sections - for [color radios](https://manual.edgetx.org/color-radios/model-settings/model-setup/internal-external-rf#receiver-number) 

<img src="https://raw.githubusercontent.com/wiki/rotorman/CyberBrick_ESPNOW/assets/color_int_receivernumber.png"> <img src="https://raw.githubusercontent.com/wiki/rotorman/CyberBrick_ESPNOW/assets/color_ext_receivernumber.png">

or for [black-and-white screen radios](https://manual.edgetx.org/bw-radios/model-select/setup#internal-external-rf)

<img src="https://raw.githubusercontent.com/wiki/rotorman/CyberBrick_ESPNOW/assets/bw_int_receivernumber.png"> <img src="https://raw.githubusercontent.com/wiki/rotorman/CyberBrick_ESPNOW/assets/bw_ext_receivernumber.png">
