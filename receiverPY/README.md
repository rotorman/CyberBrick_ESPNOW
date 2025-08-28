# Micropython scripts to flash [CyberBrick Core](https://eu.store.bambulab.com/de/products/multi-function-controller-core-1pcs) receiver modules

Under this folder you can find [ESP-NOW MicroPython](https://makerworld.com/en/cyberbrick/api-doc/library/espnow.html#module-espnow) code for the model/receiver side as an alternative to the original App driven intuitively usable, but from features more limited, [CyberBrick ecosystem](https://eu.store.bambulab.com/de/collections/cyberbrick).

<img src="https://blog.bambulab.com/content/images/size/w320/2025/03/Frame-62.png" height="120px"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/4/4e/Micropython-logo.svg/500px-Micropython-logo.svg.png" height="120px"> <img height="200" alt="CyberBrick Core Module (A11)" src="https://github.com/user-attachments/assets/356cfe8e-2753-4c96-b5f2-a7980646a871" /> <img src="https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2020/01/esp-now-logo.png" height="100px">

The examples in this branch currently include code for [truck](https://makerworld.com/de/models/1396031-cyberbrick-official-truck), [forklift](https://makerworld.com/de/models/1395994-cyberbrick-official-forklift) and the [bulldozer by MottN](https://makerworld.com/de/models/1461532-bulldozer-cyberbrick-rc). Use these examples to adapt the code to further models. In addition you can find two generic receiver scripts that allow you to fully control the receiver side RGB LEDs from your EdgeTX radio. `genericRGB` uses a channel for each of the base colors for each of the maximum 8 LEDs, thus 24 channels to control the LEDs (in addition to first channels controlling the servos and brushed motors). On the other hand `genericHSV` offers an alternative to control the LEDs using hue-saturation-value based input, whereas saturation is set to 1.0 and you control only hue and value from the EdgeTX radio. The latter required 16 channels to control all possible 8 LEDs. You can also find a debug script, that prints incoming [CRSF RC channels packet](https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md#0x16-rc-channels-packed-payload) values onto REPL terminal.

The benefits of the code in this branch in comparison to the original CyberBrick MicroPython stack are:
- Fully open-source with no hidden/frozen MicroPython modules used.
- Smoother control - full 12-bits are used to control the servos and also the motors, instead of only ca. 41 steps for motor and 102 steps for servo angle in original control. Also, the motor control PWM frequency is increased from 50 Hz to 500 Hz.
- Fully customizable NeoPixel LED driver. Actuate the various LED pixels from remote or from some logic combination of other channel states.

Downsides:
- current code is not yet able to control more than 6 PWM outputs simultanously (each servo needs 1 and each brushed motor 2 PWM outputs, so you could for example instead control 2 brushed motors and 2 servos, but not 4 servos and 2 brushed motors at the same time with this codebase). This can be remedied, by moving at least 2 PWM controls to a timer and software PWM based solution, similarly as CyberBrick original code does.
- No fancy intuitive graphical configuration environment, as is the case with CyberBrick apps. Here, the user has to configure the output mapping in receiver side MicroPython code.

The handset, running [EdgeTX](https://edgetx.org/) firmware, sends, via custom ESP-NOW flashed ExpressLRS transmitter module (code in folder [./transmitterFW](https://github.com/rotorman/CyberBrick_ESPNOW/tree/main/transmitterFW)) channel data according to [CRSF specifications](https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md) - [16 proportional channels in 11-bit resolution](https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md#0x16-rc-channels-packed-payload). The channel order, range, mixing and further parameters can be adjusted in the EdgeTX radio.
Optionally, when EdgeTX handset is flashed with the proposed extended CRSF protocol that supports 32-channels ([CRSF protocol extension proposal](https://github.com/tbs-fpv/tbs-crsf-spec/pull/28), [EdgeTX pull request](https://github.com/EdgeTX/edgetx/pull/6504) and [firmware download](https://github.com/rotorman/edgetx/releases/tag/v2.11.3_crsf_32ch)), you can fully exploit the generic scripts that require more than 16 channels radio link transport for proper operation.

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

The mapping for the `genericRGB` is:
- ch1: brushed motor1
- ch2: brushed motor2
- ch3: servo1 (0.5ms to 2.5ms range)
- ch4: servo2 (0.5ms to 2.5ms range)
- ch9: Neopixel string1 LED1 Red
- ch10: Neopixel string1 LED1 Green
- ch11: Neopixel string1 LED1 Blue
- ch12: Neopixel string1 LED2 Red
- ch13: Neopixel string1 LED2 Green
- ch14: Neopixel string1 LED2 Blue
- ch15: Neopixel string1 LED3 Red
- ch16: Neopixel string1 LED3 Green
- ch17: Neopixel string1 LED3 Blue
- ch18: Neopixel string1 LED4 Red
- ch19: Neopixel string1 LED4 Green
- ch20: Neopixel string1 LED4 Blue
- ch21: Neopixel string2 LED1 Red
- ch22: Neopixel string2 LED1 Green
- ch23: Neopixel string2 LED1 Blue
- ch24: Neopixel string2 LED2 Red
- ch25: Neopixel string2 LED2 Green
- ch26: Neopixel string2 LED2 Blue
- ch27: Neopixel string2 LED3 Red
- ch28: Neopixel string2 LED3 Green
- ch29: Neopixel string2 LED3 Blue
- ch30: Neopixel string2 LED4 Red
- ch31: Neopixel string2 LED4 Green
- ch32: Neopixel string2 LED4 Blue

The mapping for the `genericHSV` is:
- ch1: brushed motor1
- ch2: brushed motor2
- ch3: servo1 (0.5ms to 2.5ms range)
- ch4: servo2 (0.5ms to 2.5ms range)
- ch17: Neopixel string1 LED1 hue
- ch18: Neopixel string1 LED1 brightness
- ch19: Neopixel string1 LED2 hue
- ch20: Neopixel string1 LED2 brightness
- ch21: Neopixel string1 LED3 hue
- ch22: Neopixel string1 LED3 brightness
- ch23: Neopixel string1 LED4 hue
- ch24: Neopixel string1 LED4 brightness
- ch25: Neopixel string2 LED1 hue
- ch26: Neopixel string2 LED1 brightness
- ch27: Neopixel string2 LED2 hue
- ch28: Neopixel string2 LED2 brightness
- ch29: Neopixel string2 LED3 hue
- ch30: Neopixel string2 LED3 brightness
- ch31: Neopixel string2 LED4 hue
- ch32: Neopixel string2 LED4 brightness

Instructions steps for copying the script to your CyberBrick Core:

1. Download or check out (git clone) this repository.
2. If you do not already have a [REPL capable MicroPython development environment setup](https://makerworld.com/en/cyberbrick/api-doc/cyberbrick_core/start/index.html#setting-up-the-development-environment), I suggest you take a look at [Arduino Lab for MicroPython](https://labs.arduino.cc/en/labs/micropython), which in my opinon will be easiest to work with.
Another easy to use option is [Thonny](https://thonny.org/).
3. Hook up the CyberBrick Core receiver to your computer, open Arduino Lab for MicroPython and press Connect. On first start of Arduino lab for MicroPython you are asked to pick a folder where to store the files locally. Provide here the folder, where you extracted or git cloned the repository of this PR. You can directly provide the `receiverPY` subfolder of the repo.
4. Open the Files window (in the top right corner of Arduino Lab for MicroPython). You might want to backup your `boot.py` from the board. To do this, select `boot.py` in the left side and then click the button with right arrow icon in the middle of the Arduino Lab for MicroPython to start copying the `boot.py` file from your CyberBrick core to your computer.
5. Navigate to one of the example model subfolders on the right side. Select both files (`boot.py` and the main model MicroPython code) and then click the left arrow icon to copy them to your receiver side CyberBrick Core module. You will be overwriting your stock `boot.py` file while doing so, thus be sure you made a backup into one local folder structure above in the previous step. If not, not much is lost, as that original state of the `boot.py` file is also available on CyberBrick Git repository at: https://raw.githubusercontent.com/CyberBrick-Official/CyberBrick_Controller_Core/refs/heads/master/src/app_rc/boot.py
6. Change back to “Editor” view (top right corner of Arduino Lab for MicroPython). Hit “Reset”. The REPL console window should be open on the bottom of the screen, if not, click the tiny up arrow button on the bottom right corner of the Arduino Lab for MicroPython. Write down the MAC address of your receiver side CyberBrick Core that shold be listed in the console. You will need to enter this in the transmitter side firmware. If you do not see the MAC address listed, power cycle your CyberBrick Core, then reconnect and hit the “Reset” button once more.
7. Disconnect the Arduino Lab for MicroPython software from your receiver core module and disconnect the USB cable as well.
8. Assemble the CyberBrick Core receiver side module back into the 3D printed model.
9. Assuming, the transmitter side ExpressLRS module was already correctly flashed and EdgeTX channels configured, power up the EdgeTX radio, power up your model. You should be able to control the models from your EdgeTX transmitter.
