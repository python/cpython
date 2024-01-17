"""
Provides .props file.
"""

import os
import sys

from .constants import *

__all__ = ["get_nuspec_layout"]

PYTHON_NUSPEC_NAME = "python.nuspec"

NUSPEC_DATA = {
    "PYTHON_TAG": VER_DOT,
    "PYTHON_VERSION": os.getenv("PYTHON_NUSPEC_VERSION"),
    "FILELIST": r'    <file src="**\*" exclude="python.png" target="tools" />',
    "GIT": sys._git,
}

NUSPEC_PLATFORM_DATA = dict(
    _keys=("PYTHON_BITNESS", "PACKAGENAME", "PACKAGETITLE"),
    win32=("32-bit", "pythonx86", "Python (32-bit)"),
    amd64=("64-bit", "python", "Python"),
    arm32=("ARM", "pythonarm", "Python (ARM)"),
    arm64=("ARM64", "pythonarm64", "Python (ARM64)"),
    win32t=("32-bit free-threaded", "pythonx86-freethreaded", "Python (32-bit, free-threaded)"),
    amd64t=("64-bit free-threaded", "python-freethreaded", "Python (free-threaded)"),
    arm32t=("ARM free-threaded", "pythonarm-freethreaded", "Python (ARM, free-threaded)"),
    arm64t=("ARM64 free-threaded", "pythonarm64-freethreaded", "Python (ARM64, free-threaded)"),
)

if not NUSPEC_DATA["PYTHON_VERSION"]:
    NUSPEC_DATA["PYTHON_VERSION"] = "{}.{}{}{}".format(
        VER_DOT, VER_MICRO, "-" if VER_SUFFIX else "", VER_SUFFIX
    )

FILELIST_WITH_PROPS = r"""    <file src="**\*" exclude="python.png;python.props" target="tools" />
    <file src="python.props" target="build\native" />"""

NUSPEC_TEMPLATE = r"""<?xml version="1.0"?>
<package>
  <metadata>
    <id>{PACKAGENAME}</id>
    <title>{PACKAGETITLE}</title>
    <version>{PYTHON_VERSION}</version>
    <authors>Python Software Foundation</authors>
    <license type="file">tools\LICENSE.txt</license>
    <projectUrl>https://www.python.org/</projectUrl>
    <description>Installs {PYTHON_BITNESS} Python for use in build scenarios.</description>
    <icon>images\python.png</icon>
    <iconUrl>https://www.python.org/static/favicon.ico</iconUrl>
    <tags>python</tags>
    <repository type="git" url="https://github.com/Python/CPython.git" commit="{GIT[2]}" />
  </metadata>
  <files>
    <file src="python.png" target="images" />
{FILELIST}
  </files>
</package>
"""


def _get_nuspec_data_overrides(ns):
    arch = ns.arch
    if ns.include_freethreaded:
        arch += "t"
    for k, v in zip(NUSPEC_PLATFORM_DATA["_keys"], NUSPEC_PLATFORM_DATA[arch]):
        ev = os.getenv("PYTHON_NUSPEC_" + k)
        if ev:
            yield k, ev
        yield k, v


def get_nuspec_layout(ns):
    if ns.include_all or ns.include_nuspec:
        data = dict(NUSPEC_DATA)
        for k, v in _get_nuspec_data_overrides(ns):
            if not data.get(k):
                data[k] = v
        if ns.include_all or ns.include_props:
            data["FILELIST"] = FILELIST_WITH_PROPS
        nuspec = NUSPEC_TEMPLATE.format_map(data)
        yield "python.nuspec", ("python.nuspec", nuspec.encode("utf-8"))
        yield "python.png", ns.source / "PC" / "icons" / "logox128.png"
