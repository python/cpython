"""Extract version information from Include/patchlevel.h."""

import re
import sys
from pathlib import Path
from typing import Literal, NamedTuple

CPYTHON_ROOT = Path(
    __file__,  # cpython/Doc/tools/extensions/patchlevel.py
    "..",  # cpython/Doc/tools/extensions
    "..",  # cpython/Doc/tools
    "..",  # cpython/Doc
    "..",  # cpython
).resolve()
PATCHLEVEL_H = CPYTHON_ROOT / "Include" / "patchlevel.h"

RELEASE_LEVELS = {
    "PY_RELEASE_LEVEL_ALPHA": "alpha",
    "PY_RELEASE_LEVEL_BETA": "beta",
    "PY_RELEASE_LEVEL_GAMMA": "candidate",
    "PY_RELEASE_LEVEL_FINAL": "final",
}


class version_info(NamedTuple):  # noqa: N801
    major: int  #: Major release number
    minor: int  #: Minor release number
    micro: int  #: Patch release number
    releaselevel: Literal["alpha", "beta", "candidate", "final"]
    serial: int  #: Serial release number


def get_header_version_info() -> version_info:
    # Capture PY_ prefixed #defines.
    pat = re.compile(r"\s*#define\s+(PY_\w*)\s+(\w+)", re.ASCII)

    defines = {}
    patchlevel_h = PATCHLEVEL_H.read_text(encoding="utf-8")
    for line in patchlevel_h.splitlines():
        if (m := pat.match(line)) is not None:
            name, value = m.groups()
            defines[name] = value

    return version_info(
        major=int(defines["PY_MAJOR_VERSION"]),
        minor=int(defines["PY_MINOR_VERSION"]),
        micro=int(defines["PY_MICRO_VERSION"]),
        releaselevel=RELEASE_LEVELS[defines["PY_RELEASE_LEVEL"]],
        serial=int(defines["PY_RELEASE_SERIAL"]),
    )


def format_version_info(info: version_info) -> tuple[str, str]:
    version = f"{info.major}.{info.minor}"
    release = f"{info.major}.{info.minor}.{info.micro}"
    if info.releaselevel != "final":
        suffix = {"alpha": "a", "beta": "b", "candidate": "rc"}
        release += f"{suffix[info.releaselevel]}{info.serial}"
    return version, release


def get_version_info():
    try:
        info = get_header_version_info()
        return format_version_info(info)
    except OSError:
        version, release = format_version_info(sys.version_info)
        print(
            f"Failed to get version info from Include/patchlevel.h, "
            f"using version of this interpreter ({release}).",
            file=sys.stderr,
        )
        return version, release


if __name__ == "__main__":
    short_ver, full_ver = format_version_info(get_header_version_info())
    if sys.argv[1:2] == ["--short"]:
        print(short_ver)
    else:
        print(full_ver)
