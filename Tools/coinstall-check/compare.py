#!/usr/bin/env python3
# Compare that multiple installs of Python don't have conflicting files
#
# This is a requirement for Debian's Multi-Arch installs of Python
# https://www.debian.org/doc/debian-policy/ch-controlfields.html#multi-arch
#
# Excluded from this should be:
# * /usr/bin/*: only libraries are multi-arch co-installed, one arch's binaries
#               are installed at a time
# * pyconfig.h: Varies according to config, installed into a tag-specific
#               directory.
# * Non-tag suffixed .pc files: Only the suffixed versions are co-installable
# * .dist-info/RECORD: Contains hashes, not co-installable.
# * .dist-info/WHEEL: Contains arch and version tags. Can be merged in some
#                     cases, but not typically co-installable.

from argparse import ArgumentParser
from hashlib import file_digest
from pathlib import Path


def hash_tree(base: Path, algorithm: str = "sha512") -> dict[str, str]:
    print(f"Hashing {base}")
    seen: dict[str, str] = {}
    for dirpath, dirnames, filenames in base.walk():
        if dirpath.name == "__pycache__":
            # Includes a timestamp, we expect a mismatch
            continue
        for file in filenames:
            filepath = dirpath / file
            with filepath.open("rb") as f:
                digest = file_digest(f, algorithm)
            seen[str(filepath.relative_to(base))] = digest.hexdigest()
    return seen


def compare_trees(base: Path) -> bool:
    seen: dict[str, str] = {}
    success: bool = True
    for tree in base.iterdir():
        if not tree.is_dir():
            continue
        hashes = hash_tree(tree)
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
    p = ArgumentParser("Compare multiple installs of Python")
    p.add_argument(
        "base_directory",
        type=Path,
        help=(
            "Directory below which multiple Pythons are installed, "
            "each inside their own directory."
        ),
    )
    args = p.parse_args()
    if not compare_trees(args.base_directory):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
