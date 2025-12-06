#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
from pathlib import Path

CURRENT_DIR = Path(__file__).parent
MISC_DIR = CURRENT_DIR.parent
REPO_ROOT = MISC_DIR.parent
LIB_DIR = REPO_ROOT / "Lib"
FILE_LIST = CURRENT_DIR / "typed-stdlib.txt"

parser = argparse.ArgumentParser(prog="make_symlinks.py")
parser.add_argument(
    "--symlink",
    action="store_true",
    help="Create symlinks",
)
parser.add_argument(
    "--clean",
    action="store_true",
    help="Delete any pre-existing symlinks",
)

args = parser.parse_args()

if args.clean:
    for entry in CURRENT_DIR.glob("*"):
        if entry.is_symlink():
            entry_at_root = entry.relative_to(REPO_ROOT)
            print(f"removing pre-existing {entry_at_root}")
            entry.unlink()

for link in FILE_LIST.read_text().splitlines():
    link = link.strip()
    if not link or link.startswith('#'):
        continue

    src = LIB_DIR / link
    dst = CURRENT_DIR / link
    src_at_root = src.relative_to(REPO_ROOT)
    dst_at_root = dst.relative_to(REPO_ROOT)
    if (
        dst.is_symlink()
        and src.resolve(strict=True) == dst.resolve(strict=True)
    ):
        continue

    if not args.symlink and args.clean:
        # when the user called --clean without --symlink, don't report missing
        # symlinks that we just deleted ourselves
        continue

    # we specifically want to create relative-path links with ..
    src_rel = os.path.relpath(src, CURRENT_DIR)
    action = "symlinking" if args.symlink else "missing symlink to"
    print(f"{action} {src_at_root} at {dst_at_root}")
    if args.symlink:
        os.symlink(src_rel, dst)
