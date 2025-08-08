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

import serial, time, sys
import argparse
import serials_find
import external.streamexpect as streamexpect

def dbg_print(line=''):
    sys.stdout.write(line + '\n')
    sys.stdout.flush()


def etx_passthrough_init(port, requestedBaudrate):
    sys.stdout.flush()
    dbg_print("======== PASSTHROUGH INIT ========")
    dbg_print("  Trying to initialize %s @ %s" % (port, requestedBaudrate))

    s = serial.Serial(port=port, baudrate=requestedBaudrate,
        bytesize=8, parity='N', stopbits=1,
        timeout=1, xonxoff=0, rtscts=0)

    with streamexpect.wrap(s) as rl:
        rl.flush()
        rl.write(b"set pulses 0\n")
        rl.expect_bytes(b"set: ", timeout=1.0)
        rl.expect_bytes(b"> ", timeout=1.0)
        rl.write(b"set rfmod 0 power off\n")
        rl.expect_bytes(b"set: ", timeout=1.0)
        rl.expect_bytes(b"> ", timeout=1.0)
        time.sleep(.5)
        rl.write(b"set rfmod 0 bootpin 1\n")
        rl.expect_bytes(b"set: ", timeout=1.0)
        rl.expect_bytes(b"> ", timeout=1.0)
        time.sleep(.1)
        rl.write(b"set rfmod 0 power on\n")
        rl.expect_bytes(b"set: ", timeout=1.0)
        rl.expect_bytes(b"> ", timeout=1.0)
        time.sleep(.1)
        rl.write(b"set rfmod 0 bootpin 0\n")
        rl.expect_bytes(b"set: ", timeout=1.0)
        rl.expect_bytes(b"> ", timeout=1.0)

        cmd = "serialpassthrough rfmod 0 %s" % requestedBaudrate

        dbg_print("Enabling serial passthrough...")
        dbg_print("  CMD: '%s'" % cmd)
        rl.write(cmd.encode("utf-8"))
        rl.write(b'\n')
        time.sleep(.2)

    s.close()
    dbg_print("======== PASSTHROUGH DONE ========")

def init_passthrough(source, target, env):
    env.AutodetectUploadPort([env])
    port = env['UPLOAD_PORT']
    etx_passthrough_init(port, env['UPLOAD_SPEED'])

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="Initialize EdgeTX passthrough to internal module")
    parser.add_argument("-b", "--baud", type=int, default=460800,
        help="Baud rate for passthrough communication")
    parser.add_argument("-p", "--port", type=str,
        help="Override serial port autodetection and use PORT")
    args = parser.parse_args()

    if (args.port == None):
        args.port = serials_find.get_serial_port()

    etx_passthrough_init(args.port, args.baud)
