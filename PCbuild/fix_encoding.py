#! /usr/bin/env python3
#
# Fixes encoding of the project files to add UTF-8 BOM.
#
# Visual Studio insists on having the BOM in project files, and will
# restore it on first edit. This script will go through the relevant
# files and ensure the BOM is included, which should prevent too many
# irrelevant changesets.
#

from pathlib import Path

__author__ = "Steve Dower <steve.dower@python.org>"
__version__ = "1.0.0.0"

def fix(p):
    with open(p, 'r', encoding='utf-8-sig') as f:
        data = f.read()
    with open(p, 'w', encoding='utf-8-sig') as f:
        f.write(data)

ROOT_DIR = Path(__file__).resolve().parent

if __name__ == '__main__':
    count = 0
    print('Fixing:')
    for f in ROOT_DIR.glob('*.vcxproj'):
        print(f' - {f.name}')
        fix(f)
        count += 1
    for f in ROOT_DIR.glob('*.vcxproj.filters'):
        print(f' - {f.name}')
        fix(f)
        count += 1
    print()
    print(f'Fixed {count} files')
