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
 ch7 - Neopixel string1 LED1
 ch8 - Neopixel string1 LED2
 ch9 - Neopixel string1 LED3
ch10 - Neopixel string1 LED4
ch11 - Neopixel string2 LED1
ch12 - Neopixel string2 LED2
ch13 - Neopixel string2 LED3
ch14 - Neopixel string2 LED4

The handset, running EdgeTX firmware, sends, via custom ESP-NOW flashed
ExpressLRS transmitter module channel data according to CRSF specifications:
16 proportional channels in slightly lower than 11-bit resolution.
The channel order, range, mixing and further parameters can be adjusted
in the EdgeTX radio.

The incoming value range of the channel data with 100% range in EdgeTX
is from 173 to 1811, with 992 being middle.
If the output range is increased to -121.1% to +121.1% under OUTPUTS in
EdgeTX, the value range increases form 0 to 1984, with 992 still being
the middle position.

The LEDs data is interpreted as RGB343 (3 bits for red, 4 for green and
3 for blue) with an offset of 173 (value 173 -> R=0, G=0, B=0).
The input is converted into RGB888 output. The undefined bits are intepreted
as 1, except if the value is 0, then as 0.
Examples:

value | binary (-173) | red  | green | blue
173   | b000 0000 000 | 0    | 0     | 0 (<- dark/black)
174   | b000 0000 001 | 0    | 0     | b0011 1111 = 63
175   | b000 0000 010 | 0    | 0     | b0101 1111 = 95
180   | b000 0000 111 | 0    | 0     | b1111 1111 = 255
181   | b000 0001 000 | 0    | 31    | 0
182   | b000 0001 001 | 0    | 31    | 63
300   | b000 1111 111 | 0    | 255   | 255 (<- cyan)
301   | b001 0000 000 | 63   | 0     | 0
302   | b001 0000 001 | 63   | 0     | 63
773   | b100 1011 000 | 159  | 191   | 0
1196  | b111 1111 111 | 255  | 255   | 255 (<- white)

Any value above 173+1023=1196 is intepreted as white.
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
MAXRGBVALUE               = const(1023) # max of 11-bit value range

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
      
def rgb343(val):
  adjusted = min(val - CRSF_CHANNEL_VALUE_MIN, MAXRGBVALUE)
  if adjusted == 0:
    return 0,0,0
  else:
    r = min(((adjusted & 0b1110000000) >> 2) + 0b11111, 255)
    g = min(((adjusted & 0b0001111000) << 1) + 0b1111, 255)
    b = min(((adjusted & 0b0000000111) << 5) + 0b11111, 255)
    return r,g,b

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
      if len(msg) > 31:
        ch = struct.unpack('<HHHHHHHHHHHHHHHH', msg)
        if len(ch) == 16:
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
          LEDstring1[0] = rgb343(ch[6])
          LEDstring1[1] = rgb343(ch[7])
          LEDstring1[2] = rgb343(ch[8])
          LEDstring1[3] = rgb343(ch[9])
          LEDstring1.write()
          LEDstring2[0] = rgb343(ch[10])
          LEDstring2[1] = rgb343(ch[11])
          LEDstring2[2] = rgb343(ch[12])
          LEDstring2[3] = rgb343(ch[13])
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
