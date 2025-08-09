"""
This file belongs to the CyberBrick ESP-NOW transmitter & receiver project,
hosted originally at:
https://github.com/rotorman/CyberBrick_ESPNOW
Copyright (C) 2025, Risto Koiva

License GPL-3.0: https://www.gnu.org/licenses/gpl-3.0.html

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
"""

"""
CyberBrick ESP-NOW receiver script in CyberBrick MicroPython flavor.
To be copied to CyberBrick Core, paired with X11 remote control receiver shield.
Control for the CyberBrick official truck:
https://makerworld.com/de/models/1396031-cyberbrick-official-truck

The handset, running EdgeTX firmware, sends, via custom ESP-NOW flashed
ExpressLRS transmitter module channel data according to CRSF specifications
PR#28 (https://github.com/tbs-fpv/tbs-crsf-spec/pull/28) 32 proportional
channels in slightly lower than 11-bit resolution.
The channel order, range, mixing and further parameters can be adjusted
in the EdgeTX radio.

The outputs of X11 shield are driven from following inputs, matching
the most widely used Mode 2, AETR mapping:
* Servo1: steering, channel 1 (right horizontal stick)
* Motor1: throttle, channel 3 (left vertical stick)
* NeoPixel_Channel2: driven in code by throttle (LV) and steering (RH)

In CyberBrick official truck, the 4 NeoPixels are all connected to channel2:
LED 1 - FrontLeft, LED 2 - FrontRight, LED 3 - BackLeft, LED 4 - BackRight
"""

from machine import Pin, PWM
import network
import espnow
from neopixel import NeoPixel
import utime
import struct

button = Pin(9, Pin.IN) # User key/button on CyberBrick Core

# Initialize all servo outputs with 1.5ms pulse length in 20ms period
S1 = PWM(Pin(3), freq=50, duty_u16=4915) # servo center 1.5ms equals to 65535/20 * 1.5 = 4915
#S2 = PWM(Pin(2), freq=50, duty_u16=4915)
#S3 = PWM(Pin(1), freq=50, duty_u16=4915)
#S4 = PWM(Pin(0), freq=50, duty_u16=4915)

# Initialize motor 1 output to idle (a brushed motor is controlled via 2 pins on HTD8811)
M1A = PWM(Pin(4), freq=500, duty_u16=0)
M1B = PWM(Pin(5), freq=500, duty_u16=0)

# Initialize Wi-Fi in station mode
sta = network.WLAN(network.STA_IF)
sta.active(True)
mac = sta.config('mac')
mac_address = ':'.join('%02x' % b for b in mac)
print("MAC address of the receiver:", mac_address)

def wifi_reset():
  # Reset Wi-Fi to AP_IF off, STA_IF on and disconnected
  sta = network.WLAN(network.WLAN.IF_STA); sta.active(False)
  ap = network.WLAN(network.WLAN.IF_AP); ap.active(False)
  sta.active(True)
  while not sta.active():
    utime.sleep_ms(100)
  while sta.isconnected():
    utime.sleep_ms(100)
  sta = network.WLAN(network.STA_IF)
  sta.active(True)
  sta.config(txpower=20,pm=sta.PM_NONE,reconnects=0)
  sta.disconnect()

wifi_reset()

# Initialize ESP-NOW
e = espnow.ESPNow()

def enow_reset():
  try:
      e.active(True)
  except OSError as err:
      print("Failed to initialize ESP-NOW:", err)
      raise

enow_reset()

def send_bind():
  # Transmit cyclically own MAC, as long as user key/button is pressed
  peer = b'\xff\xff\xff\xff\xff\xff' # Broadcast address
  e.add_peer(peer)
  print("Entered bind mode")
  while (button.value() == 0):
    # Button pressed and held
    e.send(peer, mac) # transmit own MAC address
    print("Broadcasting own MAC address:", mac_address)
    utime.sleep_ms(500) # 2 Hz
  # Button released
  e.del_peer(peer)
  e.active(False)
  wifi_reset()
  enow_reset()
  print("Exited bind mode")

# Drive NeoPixel on CyberBrick Core
npcore = Pin(8, Pin.OUT)
np = NeoPixel(npcore, 1)
np[0] = (0, 10, 0) # dim green
np.write()

#Initialize NeoPixel LED string 2 with 4 pixels
LEDstring2pin = Pin(20, Pin.OUT)
LEDstring2 = NeoPixel(LEDstring2pin, 4)
for i in range(4):
  LEDstring2[i] = (0, 0, 0) # default all off
LEDstring2.write()

blinkertime_ms            = const(750)  # 1.5 Hz
CRSFdeadzoneplusminus     = const(50)
SERVORAWdeadzoneplusminus = const(100)

CRSF_CHANNEL_VALUE_MIN    = const(173)
CRSF_CHANNEL_VALUE_MID    = const(992)
CRSF_CHANNEL_VALUE_MAX    = const(1811)
SERVOPULSE_0_5MS_TICKS    = const(1639) # 65535 equals to 20ms
SERVOPULSE_1MS_TICKS      = const(3277)
SERVOPULSE_1_5MS_TICKS    = const(4915)
SERVORAWmidpoint          = SERVOPULSE_1_5MS_TICKS
SERVOPULSE_2MS_TICKS      = const(6554)
SERVOPULSE_2_5MS_TICKS    = const(8192)
FULLSCALE16BIT            = 65535
PWMGAINCOEFFICIENTPOS     = const(80) # FULLSCALE16BIT/(CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MID)
PWMGAINCOEFFICIENTNEG     = const(80) # FULLSCALE16BIT/(CRSF_CHANNEL_VALUE_MID - CRSF_CHANNEL_VALUE_MIN)

def mapchannel(chvalue, minmapvalue, maxmapvalue):
  if minmapvalue > SERVORAWmidpoint:
    minmapvalue = SERVORAWmidpoint
  if maxmapvalue < SERVORAWmidpoint:
    maxmapvalue = SERVORAWmidpoint
  if chvalue < CRSF_CHANNEL_VALUE_MIN:
    chvalue = CRSF_CHANNEL_VALUE_MIN
  if chvalue > CRSF_CHANNEL_VALUE_MAX:
    chvalue = CRSF_CHANNEL_VALUE_MAX
  if chvalue == CRSF_CHANNEL_VALUE_MID:
    return SERVORAWmidpoint
  if chvalue>CRSF_CHANNEL_VALUE_MID:
    return ((chvalue-CRSF_CHANNEL_VALUE_MID)*(maxmapvalue-SERVORAWmidpoint)/(CRSF_CHANNEL_VALUE_MAX-CRSF_CHANNEL_VALUE_MID)) + SERVORAWmidpoint
  else:
    return SERVORAWmidpoint - ((CRSF_CHANNEL_VALUE_MID-chvalue)*(SERVORAWmidpoint-minmapvalue)/(CRSF_CHANNEL_VALUE_MID-CRSF_CHANNEL_VALUE_MIN))

while True:
  if button.value() == 0:
    # Replicate original CyberBrick behavior to center the Servo1 with user key press
    S1.duty_u16(SERVORAWmidpoint)
    # Broadcast own MAC
    send_bind()
  try:
    # Receive message (host MAC, message, 500ms failsafe timeout)
    host, msg = e.recv(500)
    if msg == None:
      # Failsafe
      # Motor off, no change to steering
      M1A.duty_u16(0)
      M1B.duty_u16(0)
      # blinking red LEDs
      if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
        for i in range(4):
          LEDstring2[i] = (0, 0, 0) # All dark
      else:
        for i in range(4):
          LEDstring2[i] = (255, 0, 0) # All red
      LEDstring2.write()

      e.active(False)
      wifi_reset()
      enow_reset()

    else:
      if len(msg) > 63:
        ch = struct.unpack('<HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH', msg)
        if len(ch) == 32:
          # Received expected CRSF telegram channel count from the handset
          # Blink green
          if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
            np[0] = (0, 0, 0) # Dark phase
          else:
            np[0] = (0, 10, 0) # Dim green phase
          np.write()

          # 1 to 2ms range
          steering = mapchannel(ch[0], SERVOPULSE_1MS_TICKS, SERVOPULSE_2MS_TICKS)
          S1.duty_u16(int(2*SERVORAWmidpoint-steering))

          throttle = ch[2]
          #deadzone check
          if ((throttle < (CRSF_CHANNEL_VALUE_MID+CRSFdeadzoneplusminus)) and (throttle > (CRSF_CHANNEL_VALUE_MID-CRSFdeadzoneplusminus))):
            #deadzone - no forward/backward movement
            M1A.duty_u16(0)
            M1B.duty_u16(0)

            # Check for steering if blinker needs to be actuated
            if (steering > (SERVORAWmidpoint+SERVORAWdeadzoneplusminus)):
              # Actuate right blinker
              LEDstring2[0] = (32, 32, 32) # Front left parking lights
              LEDstring2[2] = (255, 0, 0)  # Back left brake on
              if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
                LEDstring2[1] = (0, 0, 0)    # Front right blinker dark cycle
                LEDstring2[3] = (255, 0, 0)  # Back right brake on
              else:
                LEDstring2[1] = (255, 255, 0) # Front right blinker light cycle
                LEDstring2[3] = (255, 255, 0) # Back  right blinker light cycle
            elif (steering < (SERVORAWmidpoint-SERVORAWdeadzoneplusminus)):
              # Actuate left blinker
              LEDstring2[1] = (32, 32, 32) # Front parking lights
              LEDstring2[3] = (255, 0, 0)  # Back right brake on
              if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
                LEDstring2[0] = (0, 0, 0)    # Front left blinker dark cycle
                LEDstring2[2] = (255, 0, 0)  # Back left brake on
              else:
                LEDstring2[0] = (255, 255, 0) # Front left blinker light cycle
                LEDstring2[2] = (255, 255, 0) # Back  left blinker light cycle
            else:
              # No blinker actuated
              LEDstring2[0] = (32, 32, 32) # Front parking lights
              LEDstring2[1] = (32, 32, 32) # Front parking lights
              LEDstring2[2] = (255, 0, 0) # Brake on
              LEDstring2[3] = (255, 0, 0) # Brake on
            LEDstring2.write()

          else:
            if throttle < CRSF_CHANNEL_VALUE_MID:
              # backwards
              M1A.duty_u16((int)(max(min(PWMGAINCOEFFICIENTNEG*(CRSF_CHANNEL_VALUE_MID-throttle), FULLSCALE16BIT),0)))
              M1B.duty_u16(0)

              # Check for steering if blinker needs to be actuated
              if (steering > (SERVORAWmidpoint+SERVORAWdeadzoneplusminus)):
                # Actuate right blinker
                LEDstring2[0] = (32, 32, 32) # Front parking lights
                LEDstring2[2] = (255, 255, 255) # Reverse on
                if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
                  LEDstring2[1] = (0, 0, 0) # Front right blinker dark cycle
                  LEDstring2[3] = (255, 255, 255) # Reverse on
                else:
                  LEDstring2[1] = (255, 255, 0) # Front right blinker light cycle
                  LEDstring2[3] = (255, 255, 0) # Back  right blinker light cycle
              elif (steering < (SERVORAWmidpoint-SERVORAWdeadzoneplusminus)):
                # Actuate left blinker
                LEDstring2[1] = (32, 32, 32) # Front parking lights
                LEDstring2[3] = (255, 255, 255) # Reverse on
                if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
                  LEDstring2[0] = (0, 0, 0) # Front left blinker dark cycle
                  LEDstring2[2] = (255, 255, 255) # Reverse on
                else:
                  LEDstring2[0] = (255, 255, 0) # Front left blinker light cycle
                  LEDstring2[2] = (255, 255, 0) # Back  left blinker light cycle
              else:
                # No blinker actuated
                LEDstring2[0] = (32, 32, 32) # Front parking lights
                LEDstring2[1] = (32, 32, 32) # Front parking lights
                LEDstring2[2] = (255, 255, 255) # Reverse on
                LEDstring2[3] = (255, 255, 255) # Reverse on
              LEDstring2.write()
            else:
              # forwards
              M1A.duty_u16(0)
              M1B.duty_u16((int)(max(min(PWMGAINCOEFFICIENTPOS*(throttle-CRSF_CHANNEL_VALUE_MID), FULLSCALE16BIT),0)))

              # Check for steering if blinker needs to be actuated
              if (steering > (SERVORAWmidpoint+SERVORAWdeadzoneplusminus)):
                # Actuate right blinker
                LEDstring2[0] = (255, 255, 255) # Front driving light
                LEDstring2[2] = (32, 0, 0) # Dim red backlight
                if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
                  LEDstring2[1] = (0, 0, 0) # Front right blinker dark cycle
                  LEDstring2[3] = (32, 0, 0) # Dim red backlight
                else:
                  LEDstring2[1] = (255, 255, 0) # Front right blinker light cycle
                  LEDstring2[3] = (255, 255, 0) # Back  right blinker light cycle
              elif (steering < (SERVORAWmidpoint-SERVORAWdeadzoneplusminus)):
                # Actuate left blinker
                LEDstring2[1] = (255, 255, 255) # Front driving light
                LEDstring2[3] = (32, 0, 0) # Dim red backlight
                if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
                  LEDstring2[0] = (0, 0, 0) # Front left blinker dark cycle
                  LEDstring2[2] = (32, 0, 0) # Dim red backlight
                else:
                  LEDstring2[0] = (255, 255, 0) # Front left blinker light cycle
                  LEDstring2[2] = (255, 255, 0) # Back  left blinker light cycle
              else:
                # No blinker actuated
                LEDstring2[0] = (255, 255, 255) # Front driving light
                LEDstring2[1] = (255, 255, 255) # Front driving light
                LEDstring2[2] = (32, 0, 0) # Dim red backlight
                LEDstring2[3] = (32, 0, 0) # Dim red backlight
              LEDstring2.write()

      else:
        # Unexpected message length - blink yellow
        if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
          np[0] = (0, 0, 0) # Dark phase
        else:
          np[0] = (8, 8, 0) # Dim yellow phase
        np.write()
         
  except OSError as err:
    print("Error:", err)
    utime.sleep_ms(500)
    e.active(False)
    wifi_reset()
    enow_reset()

  except KeyboardInterrupt:
    print("Stopping receiver...")
    e.active(False)
    sta.active(False)
    break
