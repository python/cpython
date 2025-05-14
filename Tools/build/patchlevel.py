"""Extract version information from Include/patchlevel.h."""

import re
import sys
from pathlib import Path
from typing import Literal, NamedTuple

CPYTHON_ROOT = Path(
    __file__,  # cpython/Tools/build/patchlevel.py
    "..",  # cpython/Tools/build
    "..",  # cpython/Tools
    "..",  # cpython
).resolve()
PATCHLEVEL_H = CPYTHON_ROOT / "Include" / "patchlevel.h"

RELEASE_LEVELS = {
    "PY_RELEASE_LEVEL_ALPHA": "alpha",
    "PY_RELEASE_LEVEL_BETA": "beta",
    "PY_RELEASE_LEVEL_GAMMA": "candidate",
    "PY_RELEASE_LEVEL_FINAL": "final",
}


class version_info(NamedTuple):
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


def get_version_hex(info: version_info) -> int:
    """Convert a version_info object to a hex version number."""
    levels = {"alpha": 0xA, "beta": 0xB, "candidate": 0xC, "final": 0xF}
    return (
        (info.major << 24)
        | (info.minor << 16)
        | (info.micro << 8)
        | (levels[info.releaselevel] << 4)
        | (info.serial << 0)
    )


def parse_str_version(version: str) -> version_info:
    """Convert a version string to a version_info object."""
    tag_cre = re.compile(r"(\d+)(?:\.(\d+)(?:\.(\d+))?)?(?:([ab]|rc)(\d+))?$")
    result = tag_cre.match(version)
    if not result:
        raise ValueError(f"Invalid version string: {version}")

    parts = list(result.groups())
    levels = {"a": "alpha", "b": "beta", "rc": "candidate"}
    return version_info(
        major=int(parts[0]),
        minor=int(parts[1]),
        micro=int(parts[2] or 0),
        releaselevel=levels.get(parts[3], "final"),
        serial=int(parts[4]) if parts[4] else 0,
    )


def parse_hex_version(version: int) -> version_info:
    """Convert a hex version number to a version_info object."""
    if not isinstance(version, int):
        raise ValueError(f"Invalid hex version: {version}")

    levels = {0xA: "alpha", 0xB: "beta", 0xC: "candidate", 0xF: "final"}
    return version_info(
        major=(version >> 24) & 0xFF,
        minor=(version >> 16) & 0xFF,
        micro=(version >> 8) & 0xFF,
        releaselevel=levels.get((version >> 4) & 0xF, "final"),
        serial=version & 0xF,
    )


def main() -> None:
    import argparse

    parser = argparse.ArgumentParser(color=True)
    parser.add_argument(
        "version",
        nargs="?",
        help="version to convert (default: repo version)",
    )
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--short",
        action="store_true",
        help="print version as x.y",
    )
    group.add_argument(
        "--hex",
        action="store_true",
        help="print version as a 4-byte hex number",
    )
    args = parser.parse_args()

    if args.version:
        try:
            info = parse_str_version(args.version)
        except ValueError:
            info = parse_hex_version(int(args.version, 16))
    else:
        info = get_header_version_info()

    short_ver, full_ver = format_version_info(info)
    if args.short:
        print(short_ver)
    elif args.hex:
        print(hex(get_version_hex(info)))
    else:
        print(full_ver)


if __name__ == "__main__":
    main()
