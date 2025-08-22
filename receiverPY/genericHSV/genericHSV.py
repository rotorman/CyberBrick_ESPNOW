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
To be copied to CyberBrick Core on the model side.

Generic receiver script with the following mapping on X11 receiver shield:

 ch1 - brushed motor1
 ch2 - brushed motor2
 ch3 - servo1 (0.5ms to 2.5ms range)
 ch4 - servo2 (0.5ms to 2.5ms range)

ch17 - Neopixel string1 LED1 hue
ch18 - Neopixel string1 LED1 brightness
ch19 - Neopixel string1 LED2 hue
ch20 - Neopixel string1 LED2 brightness
ch21 - Neopixel string1 LED3 hue
ch22 - Neopixel string1 LED3 brightness
ch23 - Neopixel string1 LED4 hue
ch24 - Neopixel string1 LED4 brightness

ch25 - Neopixel string2 LED1 hue
ch26 - Neopixel string2 LED1 brightness
ch27 - Neopixel string2 LED2 hue
ch28 - Neopixel string2 LED2 brightness
ch29 - Neopixel string2 LED3 hue
ch30 - Neopixel string2 LED3 brightness
ch31 - Neopixel string2 LED4 hue
ch32 - Neopixel string2 LED4 brightness

The handset, running EdgeTX firmware, sends, via custom ESP-NOW flashed
ExpressLRS transmitter module channel data according to CRSF specifications
PR#28 (https://github.com/tbs-fpv/tbs-crsf-spec/pull/28) 32 proportional
channels in slightly lower than 11-bit resolution.
The channel order, range, mixing and further parameters can be adjusted
in the EdgeTX radio.

The incoming value range of the channel data with 100% range in EdgeTX
is from 173 to 1811, with 992 being middle.
If the output range is increased to -121.1% to +121.1% under OUTPUTS in
EdgeTX, the value range increases form 0 to 1984, with 992 still being
the middle position.

The LEDs data is interpreted as 0% (173) to 100% (1811).
Any value below 173 is interpreted as 0% and any value above 1811 as 100%.
"""

from machine import Pin, PWM
import network
import espnow
from neopixel import NeoPixel
import utime
import struct

# If you wish to change the WiFi channel, change this value (valid range is between 1 and 11):
wifi_channel = 1
# Remember to change it ALSO in the transmitter firmware!

button = Pin(9, Pin.IN) # User key/button on CyberBrick Core

# Initialize all servo outputs with 1.5ms pulse length in 20ms period
S1 = PWM(Pin(3), freq=50, duty_u16=4915) # servo center 1.5ms equals to 65535/20 * 1.5 = 4915
S2 = PWM(Pin(2), freq=50, duty_u16=4915)
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

#Initialize NeoPixel LED string 2 with 4 pixels
LEDstring2pin = Pin(20, Pin.OUT)
LEDstring2 = NeoPixel(LEDstring2pin, 4)
for i in range(4):
  LEDstring2[i] = (0, 0, 0) # default all off
LEDstring2.write()

blinkertime_ms            = 750  # 1.5 Hz
CRSFdeadzoneplusminus     = const(50)

CRSF_CHANNEL_VALUE_MIN    = const(173)
CRSF_CHANNEL_VALUE_MID    = const(992)
CRSF_CHANNEL_VALUE_MAX    = const(1811)
SERVOPULSE_0_5MS_TICKS    = const(1639) # 65535 equals to 20ms
SERVOPULSE_1_5MS_TICKS    = const(4915)
SERVORAWmidpoint          = SERVOPULSE_1_5MS_TICKS
SERVOPULSE_2_5MS_TICKS    = const(8192)
FULLSCALE16BIT            = 65535
PWMGAINCOEFFICIENTPOS     = const(80) # FULLSCALE16BIT/(CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MID)
PWMGAINCOEFFICIENTNEG     = const(80) # FULLSCALE16BIT/(CRSF_CHANNEL_VALUE_MID - CRSF_CHANNEL_VALUE_MIN)

def BrushedMotorControl(channel):
  #deadzone check
  if ((channel < (CRSF_CHANNEL_VALUE_MID+CRSFdeadzoneplusminus)) and (channel > (CRSF_CHANNEL_VALUE_MID-CRSFdeadzoneplusminus))):
    #deadzone - no forward/backward movement
    return 0,0
  else:
    if channel < CRSF_CHANNEL_VALUE_MID:
      # First direction
      return (int)(max(min(PWMGAINCOEFFICIENTNEG*(CRSF_CHANNEL_VALUE_MID-channel), FULLSCALE16BIT)), 0), 0
    else:
      # Rotate in the other direction
      return 0, (int)(max(min(PWMGAINCOEFFICIENTPOS*(channel-CRSF_CHANNEL_VALUE_MID), FULLSCALE16BIT)), 0)

def limitChannel(chvalue):
  if chvalue < CRSF_CHANNEL_VALUE_MIN:
    chvalue = CRSF_CHANNEL_VALUE_MIN
  if chvalue > CRSF_CHANNEL_VALUE_MAX:
    chvalue = CRSF_CHANNEL_VALUE_MAX
  return chvalue
  
def mapchannel(chvalue, minmapvalue, maxmapvalue):
  if minmapvalue > SERVORAWmidpoint:
    minmapvalue = SERVORAWmidpoint
  if maxmapvalue < SERVORAWmidpoint:
    maxmapvalue = SERVORAWmidpoint
  chvalue = limitChannel(chvalue)
  if chvalue == CRSF_CHANNEL_VALUE_MID:
    return SERVORAWmidpoint
  if chvalue>CRSF_CHANNEL_VALUE_MID:
    return ((chvalue-CRSF_CHANNEL_VALUE_MID)*(maxmapvalue-SERVORAWmidpoint)/(CRSF_CHANNEL_VALUE_MAX-CRSF_CHANNEL_VALUE_MID)) + SERVORAWmidpoint
  else:
    return SERVORAWmidpoint - ((CRSF_CHANNEL_VALUE_MID-chvalue)*(SERVORAWmidpoint-minmapvalue)/(CRSF_CHANNEL_VALUE_MID-CRSF_CHANNEL_VALUE_MIN))

def hsv_to_rgb(h, s, v):
    h /= 60.0
    c = v * s
    x = c * (1 - abs((h % 2) - 1))
    m = v - c
    if h < 1:   r,g,b = c, x, 0
    elif h < 2: r,g,b = x, c, 0
    elif h < 3: r,g,b = 0, c, x
    elif h < 4: r,g,b = 0, x, c
    elif h < 5: r,g,b = x, 0, c
    else:       r,g,b = c, 0, x
    return int((r+m)*255), int((g+m)*255), int((b+m)*255)

def channel2hsv(hue_channel, value_channel):
  hue = ((limitChannel(hue_channel) -  CRSF_CHANNEL_VALUE_MIN) * 360) // (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN)
  val = (limitChannel(value_channel) - CRSF_CHANNEL_VALUE_MIN) / (CRSF_CHANNEL_VALUE_MAX - CRSF_CHANNEL_VALUE_MIN)
  return hsv_to_rgb(hue % 360, 1.0, val)
  

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
      M2A.duty_u16(0)
      M2B.duty_u16(0)
      # Fork to idle
      S1.duty_u16(SERVORAWmidpoint)      
      S2.duty_u16(SERVORAWmidpoint)      
      # No signal from remote, blink red
      if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
        np[0] = (0, 0, 0) # Dark phase
        for i in range(4):
          LEDstring1[i] = (0, 0, 0) # All dark
          LEDstring2[i] = (0, 0, 0) # All dark
      else:
        np[0] = (10, 0, 0) # Dim red phase
        for i in range(4):
          LEDstring1[i] = (255, 0, 0) # All red
          LEDstring2[i] = (255, 0, 0) # All red
      LEDstring1.write()
      LEDstring2.write()
      np.write()
      e.active(False)
      wifi_reset()
      enow_reset()

    else:
      if len(msg) > 63:
        ch = struct.unpack('<HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH', msg)
        if len(ch) == 32:
          # Received expected CRSF telegram channel count from the handset

          # 0.5 to 2.5ms range
          S1.duty_u16(int(mapchannel(ch[2], SERVOPULSE_0_5MS_TICKS, SERVOPULSE_2_5MS_TICKS)))
          S2.duty_u16(int(mapchannel(ch[3], SERVOPULSE_0_5MS_TICKS, SERVOPULSE_2_5MS_TICKS)))

          d1,d2 = BrushedMotorControl(ch[0]) # Motor1
          M1A.duty_u16(d1)
          M1B.duty_u16(d2)

          d1,d2 = BrushedMotorControl(ch[1]) # Motor2
          M2A.duty_u16(d1)
          M2B.duty_u16(d2)
          
          # NeoPixel LEDs
          LEDstring1[0] = channel2hsv(ch[16], ch[17])
          LEDstring1[1] = channel2hsv(ch[18], ch[19])
          LEDstring1[2] = channel2hsv(ch[20], ch[21])
          LEDstring1[3] = channel2hsv(ch[22], ch[23])
          LEDstring1.write()

          LEDstring2[0] = channel2hsv(ch[24], ch[25])
          LEDstring2[1] = channel2hsv(ch[26], ch[27])
          LEDstring2[2] = channel2hsv(ch[28], ch[29])
          LEDstring2[3] = channel2hsv(ch[30], ch[31])
          LEDstring2.write()

          # Blink Core LED green
          if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
            np[0] = (0, 0, 0) # Dark phase
          else:
            np[0] = (0, 10, 0) # Dim green phase
          np.write()
      else:
        # Unexpected message length
        print(f"Unexpected ESP-NOW message length: {len(msg)}")
        # Blink yellow
        if ((utime.ticks_ms() % blinkertime_ms) > (blinkertime_ms / 2)):
          np[0] = (0, 0, 0) # Dark phase
        else:
          np[0] = (8, 8, 0) # Dim yellow phase
        np.write()
         
  except OSError as err:
    print("Error:", err)
    time.sleep(0.5)
    e.active(False)
    wifi_reset()
    enow_reset()

  except KeyboardInterrupt:
    print("Stopping receiver...")
    e.active(False)
    sta.active(False)
    break
