#!/usr/bin/env python3
# Compare that multiple installs of Python don't have conflicting files
#
# This is a requirement for Debian's Multi-Arch installs of Python
# https://www.debian.org/doc/debian-policy/ch-controlfields.html#multi-arch

from argparse import ArgumentParser
from pathlib import Path


def compare_trees(base: Path) -> bool:
    seen: dict[str, str] = {}
    success: bool = True
    for tree in base.iterdir():
        if not tree.is_file():
            continue

        hashes: dict[str, str] = {}
        print(f"Examining {tree}")
        with tree.open("r") as f:
            for line in f:
                digest, path = line.strip().split("\t")
                hashes[path] = digest

        for path, digest in hashes.items():
            if path not in seen:
                seen[path] = digest
                continue
            if digest != seen[path]:
                print(f"Mismatch found in {tree}: {path}")
                print(f"{digest} != {seen[path]}")
                success = False
    return success


def main() -> None:
    p = ArgumentParser("Compare multiple hash-r files")
    p.add_argument(
        "base_directory",
        type=Path,
        help="Directory containing hashes of Python installs.",
    )
    args = p.parse_args()
    if not compare_trees(args.base_directory):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
