#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
from pathlib import Path

CURRENT_DIR = Path(__file__).parent
MISC_DIR = CURRENT_DIR.parent
REPO_ROOT = MISC_DIR.parent
LIB_DIR = REPO_ROOT / "Lib"

parser = argparse.ArgumentParser(prog="make_symlinks.py")
parser.add_argument(
    "--dry-run",
    action="store_true",
    help="Don't actually symlink or delete anything",
)
parser.add_argument(
    "--force",
    action="store_true",
    help="Delete destination paths if they exist",
)

args = parser.parse_args()

for link in (CURRENT_DIR / ".gitignore").read_text().splitlines():
    link = link.strip()
    if not link or link.startswith('#'):
        continue

    src = LIB_DIR / link
    dst = CURRENT_DIR / link
    src_at_root = src.relative_to(REPO_ROOT)
    dst_at_root = dst.relative_to(REPO_ROOT)
    if args.force:
        if dst.exists():
            if args.dry_run:
                print(f"{dst_at_root} already exists, would delete")
            else:
                print(f"{dst_at_root} already exists, deleting")
                dst.unlink()
    elif (
        dst.is_symlink()
        and src.resolve(strict=True) == dst.resolve(strict=True)
    ):
        print(f"{dst_at_root} already exists, skipping")
        continue

    # we specifically want relative path links with ..
    src_rel = os.path.relpath(src, CURRENT_DIR)
    action = "symlinking" if not args.dry_run else "would symlink"
    print(f"{action} {src_at_root} at {dst_at_root}")
    if not args.dry_run:
        os.symlink(src_rel, dst)
