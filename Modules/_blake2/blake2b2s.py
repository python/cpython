#!/usr/bin/python3

import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
BLAKE2 = os.path.join(HERE, 'impl')

PUBLIC_SEARCH = re.compile(r'\ int (blake2[bs]p?[a-z_]*)\(')


def getfiles():
    for name in os.listdir(BLAKE2):
        name = os.path.join(BLAKE2, name)
        if os.path.isfile(name):
            yield name


def find_public():
    public_funcs = set()
    for name in getfiles():
        with open(name) as f:
            for line in f:
                # find public functions
                mo = PUBLIC_SEARCH.search(line)
                if mo:
                    public_funcs.add(mo.group(1))

    for f in sorted(public_funcs):
        print('#define {0:<18} PyBlake2_{0}'.format(f))

    return public_funcs


def main():
    lines = []
    with open(os.path.join(HERE, 'blake2b_impl.c')) as f:
        for line in f:
            line = line.replace('blake2b', 'blake2s')
            line = line.replace('BLAKE2b', 'BLAKE2s')
            line = line.replace('BLAKE2B', 'BLAKE2S')
            lines.append(line)
    with open(os.path.join(HERE, 'blake2s_impl.c'), 'w') as f:
        f.write(''.join(lines))
    # find_public()


if __name__ == '__main__':
    main()
