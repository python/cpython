#!/usr/bin/env python3
"""
Touch files that must pass Sphinx nit-picky mode
so they are rebuilt and we can catch regressions.
"""

from pathlib import Path

# Exclude these whether they're dirty or clean,
# because they trigger a rebuild of dirty files.
EXCLUDES = {
    Path("Doc/includes/wasm-notavail.rst"),
    Path("Doc/whatsnew/changelog.rst"),
}

# Exclude "Doc/venv/"
ALL_RST = {rst for rst in Path("Doc/").rglob("*.rst") if rst.parts[1] != "venv"}

with Path("Doc/tools/dirty-files.txt").open() as clean_files:
    DIRTY = {
        Path(filename.strip())
        for filename in clean_files
        if filename.strip() and not filename.startswith("#")
    }

CLEAN = ALL_RST - DIRTY - EXCLUDES

print("Touching:")
for filename in sorted(CLEAN):
    print(filename)
    filename.touch()
print(f"Touched {len(CLEAN)} files")
