#!/usr/bin/env python3
"""
Touch files that must pass Sphinx nit-picky mode
so they are rebuilt and we can catch regressions.
"""

from pathlib import Path

CLEAN = [
    "Doc/library/gc.rst",
    "Doc/library/sqlite3.rst",
    "Doc/whatsnew/3.12.rst",
]
print("Touching:")
for filename in CLEAN:
    print(filename)
    Path(filename).touch()
