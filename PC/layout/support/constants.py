"""
Constants for generating the layout.
"""

__author__ = "Steve Dower <steve.dower@python.org>"
__version__ = "3.8"

import struct
import sys

VER_MAJOR, VER_MINOR, VER_MICRO, VER_FIELD4 = struct.pack(">i", sys.hexversion)
VER_FIELD3 = VER_MICRO << 8 | VER_FIELD4
VER_NAME = {"alpha": "a", "beta": "b", "rc": "rc"}.get(
    sys.version_info.releaselevel, ""
)
VER_SERIAL = sys.version_info.serial if VER_NAME else ""
VER_DOT = "{}.{}".format(VER_MAJOR, VER_MINOR)

PYTHON_DLL_NAME = "python{}{}.dll".format(VER_MAJOR, VER_MINOR)
PYTHON_STABLE_DLL_NAME = "python{}.dll".format(VER_MAJOR)
PYTHON_ZIP_NAME = "python{}{}.zip".format(VER_MAJOR, VER_MINOR)
PYTHON_PTH_NAME = "python{}{}._pth".format(VER_MAJOR, VER_MINOR)

PYTHON_CHM_NAME = "python{}{}{}{}{}.chm".format(
    VER_MAJOR, VER_MINOR, VER_MICRO, VER_NAME, VER_SERIAL
)

IS_X64 = sys.maxsize > 2 ** 32
