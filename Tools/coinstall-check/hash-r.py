#!/usr/bin/env python3
# Export a SHA512 manifest of installed files, so that we can ensure that
# multiple installs of Python don't have conflicting files
#
# This is a requirement for Debian's Multi-Arch installs of Python
# https://www.debian.org/doc/debian-policy/ch-controlfields.html#multi-arch

from argparse import ArgumentParser
from hashlib import file_digest
from pathlib import Path
import json


def load_build_details(base: Path):
    for path in base.glob("usr/lib/python*/build-details*.json"):
        with path.open("rb") as f:
            return json.load(f)


def hash_tree(base: Path, algorithm: str = "sha512") -> dict[str, str]:
    hashes: dict[str, str] = {}
    build_details = load_build_details(base)
    flags = build_details["abi"]["flags"]
    version = build_details["language"]["version"]
    for dirpath, dirnames, filenames in base.walk():
        if dirpath.name == "__pycache__":
            # Includes a timestamp, we expect a mismatch
            continue
        relative_dirpath = dirpath.relative_to(base)
        if relative_dirpath.is_relative_to("usr/bin"):
            # Only libraries are multi-arch co-installed, only one arch can
            # have binaries in /usr/bin at a time.
            continue

        for file in filenames:
            if relative_dirpath.is_relative_to("usr/include") and file == "pyconfig.h":
                # Varies according to config, installed into a tag-specific
                # include directory
                continue

            if (
                relative_dirpath.is_relative_to("usr/lib")
                and relative_dirpath.name == "pkgconfig"
                and file in ("python3.pc", "python3-embed.pc")
            ):
                # Only the tag-suffixed .pc files are co-installable
                continue

            if (
                relative_dirpath.is_relative_to("usr/lib")
                and relative_dirpath.name == "pkgconfig"
                and flags  # non-default install
                and file in (f"python-{version}.pc", f"python-{version}-embed.pc")
            ):
                # Only the tag-suffixed .pc files are co-installable
                continue

            if relative_dirpath.name.endswith(".dist-info") and file in (
                "RECORD",
                "WHEEL",
            ):
                # RECORD: Contains hashes, not co-installable.
                # WHEEL: Contains arch and version tags. Tags can be merged but
                # not architectures.
                continue

            if relative_dirpath.name == "site-packages" and file.endswith(
                (".abi3.so", ".abi3t.so")
            ):
                # abi3 and abi3t are not current co-installable (#122931)
                continue

            filepath = dirpath / file
            with filepath.open("rb") as f:
                digest = file_digest(f, algorithm)
            hashes[str(filepath.relative_to(base))] = digest.hexdigest()
    return hashes


def main() -> None:
    p = ArgumentParser("Hash a python install for comparison later")
    p.add_argument(
        "destdir",
        type=Path,
        help="Directory below which Python is installed",
    )
    args = p.parse_args()
    hashes = hash_tree(args.destdir)
    for path, digest in sorted(hashes.items()):
        print(f"{digest}\t{path}")  # compatible with sha512sum


if __name__ == "__main__":
    main()
