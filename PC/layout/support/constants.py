"""
Constants for generating the layout.
"""

__author__ = "Steve Dower <steve.dower@python.org>"
__version__ = "3.8"

import os
import struct
import sys


def _unpack_hexversion():
    try:
        hexversion = int(os.getenv("PYTHON_HEXVERSION"), 16)
    except (TypeError, ValueError):
        hexversion = sys.hexversion
    return struct.pack(">i", hexversion)


def _get_suffix(field4):
    name = {0xA0: "a", 0xB0: "b", 0xC0: "rc"}.get(field4 & 0xF0, "")
    if name:
        serial = field4 & 0x0F
        return f"{name}{serial}"
    return ""


VER_MAJOR, VER_MINOR, VER_MICRO, VER_FIELD4 = _unpack_hexversion()
VER_SUFFIX = _get_suffix(VER_FIELD4)
VER_FIELD3 = VER_MICRO << 8 | VER_FIELD4
VER_DOT = "{}.{}".format(VER_MAJOR, VER_MINOR)

PYTHON_DLL_NAME = "python{}{}.dll".format(VER_MAJOR, VER_MINOR)
PYTHON_STABLE_DLL_NAME = "python{}.dll".format(VER_MAJOR)
PYTHON_ZIP_NAME = "python{}{}.zip".format(VER_MAJOR, VER_MINOR)
PYTHON_PTH_NAME = "python{}{}._pth".format(VER_MAJOR, VER_MINOR)

PYTHON_CHM_NAME = "python{}{}{}{}.chm".format(
    VER_MAJOR, VER_MINOR, VER_MICRO, VER_SUFFIX
)

FREETHREADED_PYTHON_DLL_NAME = "python{}{}t.dll".format(VER_MAJOR, VER_MINOR)
FREETHREADED_PYTHON_STABLE_DLL_NAME = "python{}t.dll".format(VER_MAJOR)
