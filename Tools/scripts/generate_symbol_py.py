#! /usr/bin/env python3
# This script generates the symbol.py source file.

import sys
import re

def main(inFileName="Include/graminit.h", outFileName="Lib/symbol.py"):
    try:
        fp = open(inFileName)
    except OSError as err:
        sys.stderr.write("I/O error: %s\n" % str(err))
        sys.exit(1)
    with fp:
        lines = fp.read().split("\n")
    prog = re.compile(
        "#define[ \t][ \t]*([A-Z0-9][A-Z0-9_]*)[ \t][ \t]*([0-9][0-9]*)",
        re.IGNORECASE)
    tokens = {}
    for line in lines:
        match = prog.match(line)
        if match:
            name, val = match.group(1, 2)
            val = int(val)
            tokens[val] = name          # reverse so we can sort them...
    keys = sorted(tokens.keys())
    # load the output skeleton from the target:
    try:
        fp = open(outFileName)
    except OSError as err:
        sys.stderr.write("I/O error: %s\n" % str(err))
        sys.exit(2)
    with fp:
        format = fp.read().split("\n")
    try:
        start = format.index("#--start constants--") + 1
        end = format.index("#--end constants--")
    except ValueError:
        sys.stderr.write("target does not contain format markers")
        sys.exit(3)
    lines = []
    for val in keys:
        lines.append("%s = %d" % (tokens[val], val))
    format[start:end] = lines
    try:
        fp = open(outFileName, 'w')
    except OSError as err:
        sys.stderr.write("I/O error: %s\n" % str(err))
        sys.exit(4)
    with fp:
        fp.write("\n".join(format))

if __name__ == '__main__':
    main(*sys.argv[1:])
