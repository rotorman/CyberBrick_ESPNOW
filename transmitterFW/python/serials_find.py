#!/usr/bin/python
"""
This file belongs to the CyberBrick ESP-NOW transmitter & receiver project,
hosted originally at:
https://github.com/rotorman/CyberBrick_ESPNOW
Copyright (C) 2025, Risto Koiva

Large parts of the code are based on the wonderful ExpressLRS project:
https://github.com/ExpressLRS/ExpressLRS

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

import serial
import sys, glob

def serial_ports():
    """ Lists serial port names

        :raises Exception:
            On unsupported or unknown platforms
        :returns:
            A list of the serial ports available on the system
    """
    result = []
    ports = []

    try:
        from serial.tools.list_ports import comports
        if comports:
            print("  ** Searching for EdgeTX radios **")
            __ports = list(comports())
            for port in __ports:
                if (port.manufacturer and port.manufacturer in ['FTDI', ]) or (port.vid and port.vid == 0x0483):
                    print("      > EdgeTX radio found from '%s'" % port.device)
                    ports.append(port.device)
    except ImportError:
        pass

    if not ports:
        print("  ** No EdgeTX radio found, find all ports **")

        platform = sys.platform.lower()
        if platform.startswith('win'):
            ports = ['COM%s' % (i + 1) for i in range(256)]
        elif platform.startswith('linux') or platform.startswith('cygwin'):
            # this excludes your current terminal "/dev/tty"
            #ports = glob.glob('/dev/tty[A-Za-z]*')
            # List all ttyACM* and ttyUSB* ports only
            ports = glob.glob('/dev/ttyACM*')
            ports.extend(glob.glob('/dev/ttyUSB*'))
        elif platform.startswith('darwin'):
            ports = glob.glob('/dev/tty.usbmodem*')
            ports.extend(glob.glob('/dev/tty.SLAB*'))
            ports.extend(glob.glob('/dev/tty.usbserial*'))
            ports.extend(glob.glob('/dev/tty.wchusbserial*'))
        else:
            raise Exception('Unsupported platform')

    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException) as error:
            if "permission denied" in str(error).lower():
                raise Exception("You don't have permission to use serial port!")
            pass
    result.reverse()
    return result

def get_serial_port(debug=True):
    result = serial_ports()
    if debug:
        print()
        print("Detected the following serial ports on this system:")
        for port in result:
            print("  %s" % port)
        print()

    if len(result) == 0:
        raise Exception('No valid serial port detected or port already open')

    return result[0]

if __name__ == '__main__':
    results = get_serial_port(True)
    print("Found: %s" % (results, ))
