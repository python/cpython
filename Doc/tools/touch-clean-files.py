#!/usr/bin/env python3
"""
Touch files that must pass Sphinx nit-picky mode
so they are rebuilt and we can catch regressions.
"""

from pathlib import Path

# Exclude these whether they're dirty or clean,
# because they trigger a rebuild of dirty files.
EXCLUDE_FILES = {
    Path("Doc/whatsnew/changelog.rst"),
}

# Subdirectories of Doc/ to exclude.
EXCLUDE_SUBDIRS = {
    ".env",
    ".venv",
    "env",
    "includes",
    "venv",
}

ALL_RST = {
    rst for rst in Path("Doc/").rglob("*.rst") if rst.parts[1] not in EXCLUDE_SUBDIRS
}

with Path("Doc/tools/.nitignore").open() as clean_files:
    DIRTY = {
        Path(filename.strip())
        for filename in clean_files
        if filename.strip() and not filename.startswith("#")
    }

CLEAN = ALL_RST - DIRTY - EXCLUDE_FILES

print("Touching:")
for filename in sorted(CLEAN):
    print(filename)
    filename.touch()
print(f"Touched {len(CLEAN)} files")
