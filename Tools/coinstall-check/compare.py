# Compare that multiple installs of Python don't have conflicting files.
#
# This is a requirement for Debian's Multi-Arch installs of Python
# https://www.debian.org/doc/debian-policy/ch-controlfields.html#multi-arch

from argparse import ArgumentParser
from pathlib import Path
from typing import Any
import _colorize
import gzip
import json


def compare_install_manifests(base: Path) -> bool:
    """Compare all json manifests inside the directory at base."""
    hashes_seen: dict[str, tuple[str, str]] = {}
    tags_seen_by_platform: dict[str, set[frozenset[str]]] = {}
    colors = _colorize.get_colors()

    success: bool = True
    for tree in base.iterdir():
        if not tree.is_file():
            continue

        print(f"Examining {tree}")
        with gzip.open(tree) as f:
            data = json.load(f)
        build_details = data["build_details"]
        hashes = data["hashes"]
        tags_seen_by_platform.setdefault(build_details["platform"], set()).add(
            frozenset(build_details["abi"]["flags"])
        )

        for path, digest in hashes.items():
            if is_ignored(path, build_details):
                continue
            if path not in hashes_seen:
                hashes_seen[path] = (digest, tree.name)
                continue
            expected, source_name = hashes_seen[path]
            if digest != expected:
                print(f"{colors.RED}Mismatch found{colors.RESET}: {path}")
                print(f"{digest} ({tree.name}) != {expected} ({source_name})")
                success = False

    # Did we see enough builds to make a useful comparison?
    if len(tags_seen_by_platform) < 2:
        print(
            f"{colors.RED}ERROR{colors.RESET}: Insufficient platforms "
            "(architectures) to compare. Expected >= 2."
        )
        success = False

    for tagsets in tags_seen_by_platform.values():
        if len(tagsets) >= 2:
            break
    else:
        print(
            f"{colors.RED}ERROR{colors.RESET}: Insufficient configuration "
            f"variants tested. Expected >= 2."
        )
        success = False

    return success


def is_ignored(pathname: str, build_details: dict[str, Any]) -> bool:
    """Is this a path that we should ignore?"""

    path = Path(pathname)

    if path.parent.name == "__pycache__":
        # Includes a timestamp, we expect a mismatch
        return True

    if path.is_relative_to("usr/bin"):
        # Only libraries are multi-arch co-installed, only one arch can
        # have binaries in /usr/bin at a time.
        return True

    in_usr_include = path.is_relative_to("usr/include")
    if in_usr_include and path.name == "pyconfig.h":
        # Varies according to config, installed into a tag-specific
        # include directory
        return True

    in_usr_lib = path.is_relative_to("usr/lib")
    in_pkgconfig = in_usr_lib and path.parent.name == "pkgconfig"
    if in_pkgconfig and path.name in ("python3.pc", "python3-embed.pc"):
        # Only the tag-suffixed .pc files are co-installable
        return True

    version = build_details["language"]["version"]
    if (
        in_pkgconfig
        and build_details["abi"]["flags"]  # non-default install
        and path.name in (f"python-{version}.pc", f"python-{version}-embed.pc")
    ):
        # Only the tag-suffixed .pc files are co-installable
        return True

    in_dist_info = path.parent.name.endswith(".dist-info")
    if in_dist_info and path.name in ("RECORD", "WHEEL"):
        # RECORD: Contains hashes, not co-installable.
        # WHEEL: Contains arch and version tags. Tags can be merged but
        # not architectures.
        return True

    return False


def main() -> None:
    p = ArgumentParser("Compare multiple hash-r files")
    p.add_argument(
        "base_directory",
        type=Path,
        help="Directory containing hashes of Python installs.",
    )
    args = p.parse_args()
    if not compare_install_manifests(args.base_directory):
        raise SystemExit(1)

    colors = _colorize.get_colors()
    print(f"{args.base_directory} {colors.GREEN}OK{colors.RESET}")


if __name__ == "__main__":
    main()
