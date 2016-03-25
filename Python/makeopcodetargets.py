#! /usr/bin/env python
"""Generate C code for the jump table of the threaded code interpreter
(for compilers supporting computed gotos or "labels-as-values", such as gcc).
"""

import opcode
import os
import sys


def write_contents(f):
    """Write C code contents to the target file object.
    """
    targets = ['_unknown_opcode'] * 256
    for opname, op in opcode.opmap.items():
        targets[op] = "TARGET_%s" % opname
    f.write("static void *opcode_targets[256] = {\n")
    f.write(",\n".join(["    &&%s" % s for s in targets]))
    f.write("\n};\n")


def main():
    if len(sys.argv) >= 3:
        sys.exit("Too many arguments")
    if len(sys.argv) == 2:
        target = sys.argv[1]
    else:
        target = "Python/opcode_targets.h"
    with open(target, "w") as f:
        write_contents(f)
    print("Jump table written into %s" % target)


if __name__ == "__main__":
    main()
