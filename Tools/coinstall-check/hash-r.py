# Export a SHA-512 manifest of installed files, so that we can ensure that
# multiple installs of Python don't have conflicting files.
#
# This is a requirement for Debian's Multi-Arch installs of Python
# https://www.debian.org/doc/debian-policy/ch-controlfields.html#multi-arch

import gzip
import json
from argparse import ArgumentParser
from hashlib import file_digest
from pathlib import Path
from typing import Any, cast


def load_build_details(base: Path) -> dict[str, Any]:
    for path in base.glob("usr/lib/python*/build-details*.json"):
        details = json.loads(path.read_bytes())
        return cast(dict[str, Any], details)
    raise AssertionError(f"build-details.json not found in {base}")


def hash_tree(base: Path, algorithm: str = "sha512") -> dict[str, str]:
    hashes: dict[str, str] = {}
    for dirpath, dirnames, filenames in base.walk():
        for file in filenames:
            filepath = dirpath / file
            with filepath.open("rb") as f:
                digest = file_digest(f, algorithm)
            hashes[str(filepath.relative_to(base))] = digest.hexdigest()
    return hashes


def write_json(destdir: Path, output: Path) -> None:
    """Hash the Python install at destdir, write gzipped JSON to output."""
    data = {
        "build_details": load_build_details(destdir),
        "hashes": hash_tree(destdir),
    }
    with gzip.open(output, "wt") as f:
        f.write(json.dumps(data))


def main() -> None:
    p = ArgumentParser("Hash a Python install for comparison later")
    p.add_argument(
        "destdir",
        type=Path,
        help="Directory below which Python is installed",
    )
    p.add_argument(
        "-o",
        "--output",
        type=Path,
        help="Output file (gzipped)",
        required=True,
    )
    args = p.parse_args()
    write_json(args.destdir, args.output)


if __name__ == "__main__":
    main()
