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

Import("env", "projenv")
import os
import shutil
import elrs_helpers
import EdgeTXpassthrough
import UnifiedConfiguration

def add_target_uploadoption(name: str, desc: str) -> None:
    # Add an upload target 'uploadforce' that forces update if target mismatch
    # This must be called after UPLOADCMD is set
    env.AddCustomTarget(
        name=name,
        dependencies="${BUILD_DIR}/${PROGNAME}.bin",
        title=name,
        description=desc,
        actions=env['UPLOADCMD']
    )

def get_version(env):
    return '%s (%s) %s' % (env.get('GIT_VERSION'), env.get('GIT_SHA'), env.get('REG_DOMAIN'))

platform = env.get('PIOPLATFORM', '')
stm = platform in ['ststm32']

target_name = env['PIOENV'].upper()
print("PLATFORM : '%s'" % platform)
print("BUILD ENV: '%s'" % target_name)
print("build version: %s\n\n" % get_version(env))

if platform in ['espressif32']:
    if "_UART" in target_name:
        env.Replace(
            UPLOADER="$PROJECT_DIR/python/external/esptool/esptool.py",
            UPLOAD_SPEED=460800
        )
    if "_ETX" in target_name:
        env.Replace(UPLOADER="$PROJECT_DIR/python/external/esptool/esptool.py")
        env.AddPreAction("upload", EdgeTXpassthrough.init_passthrough)

add_target_uploadoption("uploadforce", "Upload even if target mismatch")

# Remove stale binary so the platform is forced to build a new one and attach options/hardware-layout files
try:
    os.remove(env['PROJECT_BUILD_DIR'] + '/' + env['PIOENV'] +'/'+ env['PROGNAME'] + '.bin')
except FileNotFoundError:
    None
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", UnifiedConfiguration.appendConfiguration)

def copyBootApp0bin(source, target, env):
    file = os.path.join(env.PioPlatform().get_package_dir("framework-arduinoespressif32"), "tools", "partitions", "boot_app0.bin")
    shutil.copy2(file, os.path.join(env['PROJECT_BUILD_DIR'], env['PIOENV']))

if platform in ['espressif32']:
    env.AddPreAction("$BUILD_DIR/${PROGNAME}.bin", copyBootApp0bin)
    if not os.path.exists('hardware'):
        elrs_helpers.git_cmd('clone', 'https://github.com/ExpressLRS/targets', 'hardware')
