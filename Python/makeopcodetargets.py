#! /usr/bin/env python
"""Generate C code for the jump table of the threaded code interpreter
(for compilers supporting computed gotos or "labels-as-values", such as gcc).
"""

# This code should stay compatible with Python 2.3, at least while
# some of the buildbots have Python 2.3 as their system Python.

import imp
import os


def find_module(modname):
    """Finds and returns a module in the local dist/checkout.
    """
    modpath = os.path.join(
        os.path.dirname(os.path.dirname(__file__)), "Lib")
    return imp.load_module(modname, *imp.find_module(modname, [modpath]))

def write_contents(f):
    """Write C code contents to the target file object.
    """
    opcode = find_module("opcode")
    targets = ['_unknown_opcode'] * 256
    for opname, op in opcode.opmap.items():
        if opname == "STOP_CODE":
            continue
        targets[op] = "TARGET_%s" % opname.replace("+0", " ").replace("+", "_")
    f.write("static void *opcode_targets[256] = {\n")
    f.write(",\n".join(["    &&%s" % s for s in targets]))
    f.write("\n};\n")


if __name__ == "__main__":
    import sys
    assert len(sys.argv) < 3, "Too many arguments"
    if len(sys.argv) == 2:
        target = sys.argv[1]
    else:
        target = "Python/opcode_targets.h"
    f = open(target, "w")
    try:
        write_contents(f)
    finally:
        f.close()
