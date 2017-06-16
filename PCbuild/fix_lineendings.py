#! /usr/bin/env python3
#
# Fixes line endings of batch files to ensure they remain LF.
#

from pathlib import Path

__author__ = "Steve Dower <steve.dower@python.org>"
__version__ = "1.0.0.0"

def fix(p):
    with open(p, 'rb') as f:
        data = f.read().replace(b'\r\n', b'\n')
    with open(p, 'wb') as f:
        f.write(data)

ROOT_DIR = Path(__file__).resolve().parent.parent

if __name__ == '__main__':
    count = 0
    print('Fixing:')
    for f in ROOT_DIR.rglob('*.bat'):
        print(f' - {f.name}')
        fix(f)
        count += 1
    print()
    print(f'Fixed {count} files')
