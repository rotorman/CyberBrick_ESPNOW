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

Import("env")
import os
from random import randint
import sys
import hashlib
import fnmatch
import time
import re
import elrs_helpers

build_flags = env.get('BUILD_FLAGS', [])
json_flags = {}
define = ""
target_name = env.get('PIOENV', '').upper()

isRX = True if '_RX_' in target_name else False

def print_error(error):
    time.sleep(1)
    sys.stdout.write("\n\n\033[47;31m%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n")
    sys.stdout.write("\033[47;31m!!!             ESPNOW Warning Below             !!!\n")
    sys.stdout.write("\033[47;31m%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n")
    sys.stdout.write("\033[47;30m  %s \n" % error)
    sys.stdout.write("\033[47;31m%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n\n\n")
    sys.stdout.flush()
    time.sleep(3)
    raise Exception('!!! %s !!!' % error)


def dequote(str):
    if str[0] == '"' and str[-1] == '"':
        return str[1:-1]
    return str

def process_build_flag(define):
    if define.startswith("-D") or define.startswith("!-D"):
        if "DEVICE_NAME=" in define:
            parts = re.search(r"(.*)=\w*'?\"(.*)\"'?$", define)
            if parts and parts.group(2):
                env['DEVICE_NAME'] = parts.group(2)
        if not define in build_flags:
            build_flags.append(define)

def parse_flags(path):
    global build_flags
    global json_flags
    try:
        with open(path, "r") as _f:
            for define in _f:
                define = define.strip()
                process_build_flag(define)

    except IOError:
        print("File '%s' does not exist" % path)

def process_flags(path):
    global build_flags
    if not os.path.isfile(path):
        return
    parse_flags(path)

def condense_flags():
    global build_flags
    for line in build_flags:
        # Some lines have multiple flags so this will split them and remove them all
        for flag in re.findall(r"!-D\s*[^\s]+", line):
            build_flags = [x.replace(flag,"") for x in build_flags] # remove the removal flag
            build_flags = [x.replace(flag[1:],"") for x in build_flags] # remove the flag if it matches the removal flag
    build_flags = [x for x in build_flags if (x.strip() != "")] # remove any blank items

def version_to_env():
    ver = elrs_helpers.get_git_version()
    env.Append(GIT_SHA = ver['sha'], GIT_VERSION= ver['version'])

def string_to_ascii(str):
    return ",".join(["%s" % ord(char) for char in str])

def get_git_sha():
    return string_to_ascii(env.get('GIT_SHA'))

def get_version():
    return string_to_ascii(env.get('GIT_VERSION'))

json_flags['flash-discriminator'] = randint(1,2**32-1)

version_to_env()
build_flags.append("-DLATEST_COMMIT=" + get_git_sha())
build_flags.append("-DLATEST_VERSION=" + get_version())
build_flags.append("-DTARGET_NAME=" + re.sub("_VIA_.*", "", target_name))
condense_flags()

json_flags['domain'] = 0

env['OPTIONS_JSON'] = json_flags
env['BUILD_FLAGS'] = build_flags
sys.stdout.write("\nbuild flags: %s\n\n" % build_flags)

sys.stdout.write("\u001b[32mBuilding for ESP32 Platform\n")

sys.stdout.flush()
time.sleep(.5)
