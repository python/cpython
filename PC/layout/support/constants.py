"""
Constants for generating the layout.
"""

__author__ = "Steve Dower <steve.dower@python.org>"
__version__ = "3.8"

import os
import pathlib
import re
import struct
import sys


def _unpack_hexversion():
    try:
        hexversion = int(os.getenv("PYTHON_HEXVERSION"), 16)
        return struct.pack(">i", hexversion)
    except (TypeError, ValueError):
        pass
    if os.getenv("PYTHONINCLUDE"):
        try:
            return _read_patchlevel_version(pathlib.Path(os.getenv("PYTHONINCLUDE")))
        except OSError:
            pass
    return struct.pack(">i", sys.hexversion)


def _get_suffix(field4):
    name = {0xA0: "a", 0xB0: "b", 0xC0: "rc"}.get(field4 & 0xF0, "")
    if name:
        serial = field4 & 0x0F
        return f"{name}{serial}"
    return ""


def _read_patchlevel_version(sources):
    if not sources.match("Include"):
        sources /= "Include"
    values = {}
    with open(sources / "patchlevel.h", "r", encoding="utf-8") as f:
        for line in f:
            m = re.match(r'#\s*define\s+(PY_\S+?)\s+(\S+)', line.strip(), re.I)
            if m and m.group(2):
                v = m.group(2)
                if v.startswith('"'):
                    v = v[1:-1]
                else:
                    v = values.get(v, v)
                    if isinstance(v, str):
                        try:
                            v = int(v, 16 if v.startswith("0x") else 10)
                        except ValueError:
                            pass
                values[m.group(1)] = v
    return (
        values["PY_MAJOR_VERSION"],
        values["PY_MINOR_VERSION"],
        values["PY_MICRO_VERSION"],
        values["PY_RELEASE_LEVEL"] << 4 | values["PY_RELEASE_SERIAL"],
    )


def check_patchlevel_version(sources):
    got = _read_patchlevel_version(sources)
    if got != (VER_MAJOR, VER_MINOR, VER_MICRO, VER_FIELD4):
        return f"{got[0]}.{got[1]}.{got[2]}{_get_suffix(got[3])}"


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
