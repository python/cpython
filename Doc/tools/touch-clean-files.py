#!/usr/bin/env python3
"""
Touch files that must pass Sphinx nit-picky mode
so they are rebuilt and we can catch regressions.
"""

from pathlib import Path

# Input file has blank line between entries to reduce merge conflicts
with Path("Doc/tools/clean-files.txt").open() as clean_files:
    CLEAN = [
        Path(filename.strip())
        for filename in clean_files
        if filename.strip() and not filename.startswith("#")
    ]

print("Touching:")
for filename in CLEAN:
    print(filename)
    filename.touch()
