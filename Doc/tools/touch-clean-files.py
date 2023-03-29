#!/usr/bin/env python3
"""
Touch files that must pass Sphinx nit-picky mode
so they are rebuilt and we can catch regressions.
"""

from pathlib import Path

# Input file has blank line between entries to reduce merge conflicts
CLEAN = Path("Doc/tools/clean-files.txt").read_text(encoding="UTF-8").split("\n")

print("Touching:")
for filename in CLEAN:
    if filename:
        print(filename)
        Path(filename).touch()
