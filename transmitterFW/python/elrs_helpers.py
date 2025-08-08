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

import os
import re
import subprocess

def git_cmd(*args):
    return subprocess.check_output(["git"] + list(args), stderr=subprocess.DEVNULL).decode("utf-8").rstrip('\r\n')


def get_git_version():
    """
    Return a dict with keys
    version: The version tag if HEAD is a version, or branch otherwise
    sha: the 6 character short sha for the current HEAD revison, falling back to
        VERSION file if not in a git repo
    """
    ver = "ver.unknown"
    sha = "000000"

    try:
        sha = git_cmd("rev-parse", "HEAD")
        ver = git_cmd("rev-parse", "--abbrev-ref", "HEAD")
        # failure here is acceptable, unnamed commits might not have a branch
        # associated
        try:
            ver = re.sub(r".*/", "", git_cmd("describe",
                         "--all", "--exact-match"))
        except:
            pass
    except:
        if os.path.exists("VERSION"):
            with open("VERSION") as _f:
                data = _f.readline()
                _f.close()
            sha = data.split()[1].strip()

    return dict(version=ver, sha=sha[:6])


class ElrsUploadResult:
        # SUCCESS
        Success = 0
        # ERROR: Unspecified
        ErrorGeneral = -1
        # ERROR: target mismatch
        ErrorMismatch = -2
