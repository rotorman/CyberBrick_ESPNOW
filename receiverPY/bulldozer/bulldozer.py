"""
This file belongs to the CyberBrick ESP-NOW transmitter & receiver project, hosted originally at:
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
Control for the Bulldozer model by MottN
https://makerworld.com/de/models/1461532-bulldozer-cyberbrick-rc

The handset, running EdgeTX firmware, sends, via custom ESP-NOW flashed
ExpressLRS transmitter module channel data according to CRSF specifications:
16 proportional channels in slightly lower than 11-bit resolution.
The channel order, range, mixing and further parameters can be adjusted
in the EdgeTX radio.

The outputs of X11 shield are driven from following inputs, matching
the most widely used Mode 2, AETR mapping:
* Servo1: blade up/down (channel 6)
* Motor1: right track (mix of channel 3 (left vertical stick) and channel 1 (right horizontal stick))
* Motor2: left track  (mix of channel 3 (left vertical stick) and channel 1 (right horizontal stick))
* NeoPixel_Channel1: cabin lights, driven in code by channel 7 (3-way switch),
                     brightness controlled by channel 8
* NeoPixel_Channel2: front lights, driven in code by channel 9 (2-way switch)

In Bulldozer, the 6 NeoPixels are all connected as follows:
channel1, 1 - cabin back right, 2 - cabin front right, 3 - cabin front left, 4 - cabin back left
channel2, 1 - front left, 2 - front right
"""

from machine import Pin, PWM
import network
import espnow
from neopixel import NeoPixel
import utime
import struct

button = Pin(9, Pin.IN) # User key/button on CyberBrick Core

# If you wish to change the WiFi channel, change this value (valid range is between 1 and 11):
wifi_channel = 1
# Remember to change it ALSO in the transmitter firmware!

# Initialize all servo outputs with 1.5ms pulse length in 20ms period
S1 = PWM(Pin(3), freq=50, duty_u16=4915) # servo center 1.5ms equals to 65535/20 * 1.5 = 4915
#S2 = PWM(Pin(2), freq=50, duty_u16=4915)
#S3 = PWM(Pin(1), freq=50, duty_u16=4915)
#S4 = PWM(Pin(0), freq=50, duty_u16=4915)

# Initialize motor 1 output to idle (a brushed motor is controlled via 2 pins on HTD8811)
M1A = PWM(Pin(4), freq=500, duty_u16=0)
M1B = PWM(Pin(5), freq=500, duty_u16=0)
M2A = PWM(Pin(6), freq=500, duty_u16=0)
M2B = PWM(Pin(7), freq=500, duty_u16=0)

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
  sta.config(channel=wifi_channel,txpower=20,pm=sta.PM_NONE,reconnects=0)
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

#Initialize NeoPixel LED string 1 with 4 pixels
LEDstring1pin = Pin(21, Pin.OUT)
LEDstring1 = NeoPixel(LEDstring1pin, 4)
for i in range(4):
  LEDstring1[i] = (0, 0, 0) # default all off
LEDstring1.write()

#Initialize NeoPixel LED string 2 with 2 pixels
LEDstring2pin = Pin(20, Pin.OUT)
LEDstring2 = NeoPixel(LEDstring2pin, 2)
for i in range(2):
  LEDstring2[i] = (0, 0, 0) # default all off
LEDstring2.write()

blinkertime_ms            = const(750)  # 1.5 Hz
CRSFdeadzoneplusminus     = const(50)
SERVORAWdeadzoneplusminus = const(100)
MINLEDCTRLBRIGHTNESS      = const(10)
MAXLEDCTRLBRIHTNESS       = const(255)
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
    send_bind()
  try:
    # Receive message (host MAC, message, 500ms failsafe timeout)
    host, msg = e.recv(500)
    if msg == None:
      # Failsafe
      # Motor off, no change to steering
      M1A.duty_u16(0)
      M1B.duty_u16(0)
      M2A.duty_u16(0)
      M2B.duty_u16(0)
      # Fork to idle
      S1.duty_u16(SERVORAWmidpoint)      
      # blinking red LEDs
      if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
        LEDstring1[0] = (0, 0, 0) # Cabin back right dark
        LEDstring1[1] = (0, 0, 0) # Cabin front right dark
        LEDstring1[2] = (0, 0, 0) # Cabin front left dark
        LEDstring1[3] = (0, 0, 0) # Cabin back left dark
        LEDstring2[0] = (0, 0, 0) # Front left dark
        LEDstring2[1] = (0, 0, 0) # Front right dark
      else:
        for i in range(4):
          LEDstring1[i] = (255, 0, 0) # All red
        for i in range(2):
          LEDstring2[i] = (255, 0, 0) # All red
      LEDstring1.write()
      LEDstring2.write()

      e.active(False)
      wifi_reset()
      enow_reset()

    else:
      if len(msg) > 31:
        ch = struct.unpack('<HHHHHHHHHHHHHHHH', msg)
        if len(ch) == 16:
          # Received expected CRSF telegram channel count from the handset
          # Blink green
          if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
            np[0] = (0, 0, 0) # Dark phase
          else:
            np[0] = (0, 10, 0) # Dim green phase
          np.write()
          
          cabinlightmode = ch[6] # 3-way switch
          brightness = (int)(((ch[7]-CRSF_CHANNEL_VALUE_MIN)*(MAXLEDCTRLBRIHTNESS-MINLEDCTRLBRIGHTNESS)/(CRSF_CHANNEL_VALUE_MAX-CRSF_CHANNEL_VALUE_MIN)) + MINLEDCTRLBRIGHTNESS)
          if brightness < 0:
            brightness = 0
          if brightness > 255:
            brightness = 255
          
          # Cabin lights
          if cabinlightmode > (CRSF_CHANNEL_VALUE_MID + (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MID)/2):
            #Yellow blinking lights
            if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
              for i in range(4):
                LEDstring1[i] = (0, 0, 0) # All dark
            else:
              for i in range(4):
                LEDstring1[i] = (brightness, brightness, 0) # All yellow
          elif cabinlightmode < (CRSF_CHANNEL_VALUE_MID - (CRSF_CHANNEL_VALUE_MID - CRSF_CHANNEL_VALUE_MIN)/2):
            for i in range(4):
              LEDstring1[i] = (brightness, brightness, brightness) # All white
          else:
            for i in range(4):
              LEDstring1[i] = (0, 0, 0) # All dark
          LEDstring1.write()
          
          # Front lights
          if ch[8] > (CRSF_CHANNEL_VALUE_MID + (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MID)/2):
            for i in range(2):
              LEDstring2[i] = (brightness, brightness, brightness) # Front lights on
          else:
            for i in range(2):
              LEDstring2[i] = (0, 0, 0) # Front lights off
          LEDstring2.write()
          
          # 0.5 to 2.5ms range
          blade = mapchannel(ch[5], SERVOPULSE_0_5MS_TICKS, SERVOPULSE_2_5MS_TICKS)
          S1.duty_u16(int(blade))

          steering = ch[0]
          throttle = ch[2]
          
          lefttrack = int(((steering-CRSF_CHANNEL_VALUE_MID) - (CRSF_CHANNEL_VALUE_MID-throttle))/2 + CRSF_CHANNEL_VALUE_MID)
          righttrack = int(((steering-CRSF_CHANNEL_VALUE_MID) + (CRSF_CHANNEL_VALUE_MID-throttle))/2 + CRSF_CHANNEL_VALUE_MID)
          
          if ((righttrack < (CRSF_CHANNEL_VALUE_MID+CRSFdeadzoneplusminus)) and (righttrack > (CRSF_CHANNEL_VALUE_MID-CRSFdeadzoneplusminus))):
            #deadzone - no forward/backward movement
            M1A.duty_u16(0)
            M1B.duty_u16(0)
          else:
            if righttrack > CRSF_CHANNEL_VALUE_MID:
              # backwards
              M1A.duty_u16((int)(min(PWMGAINCOEFFICIENTNEG*(righttrack-CRSF_CHANNEL_VALUE_MID), FULLSCALE16BIT)))
              M1B.duty_u16(0)
            else:
              # forwards
              M1A.duty_u16(0)
              M1B.duty_u16((int)(min(PWMGAINCOEFFICIENTNEG*(CRSF_CHANNEL_VALUE_MID-righttrack), FULLSCALE16BIT)))
                  
          if ((lefttrack < (CRSF_CHANNEL_VALUE_MID+CRSFdeadzoneplusminus)) and (lefttrack > (CRSF_CHANNEL_VALUE_MID-CRSFdeadzoneplusminus))):
            #deadzone - no forward/backward movement
            M2A.duty_u16(0)
            M2B.duty_u16(0)
          else:
            if lefttrack > CRSF_CHANNEL_VALUE_MID:
              # backwards
              M2A.duty_u16((int)(min(PWMGAINCOEFFICIENTNEG*(lefttrack-CRSF_CHANNEL_VALUE_MID), FULLSCALE16BIT)))
              M2B.duty_u16(0)
            else:
              # forwards
              M2A.duty_u16(0)
              M2B.duty_u16((int)(min(PWMGAINCOEFFICIENTNEG*(CRSF_CHANNEL_VALUE_MID-lefttrack), FULLSCALE16BIT)))

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
