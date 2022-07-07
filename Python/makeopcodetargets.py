#! /usr/bin/env python
"""Generate C code for the jump table of the threaded code interpreter
(for compilers supporting computed gotos or "labels-as-values", such as gcc).
"""

import os
import sys


try:
    from importlib.machinery import SourceFileLoader
except ImportError:
    import imp

    def find_module(modname):
        """Finds and returns a module in the local dist/checkout.
        """
        modpath = os.path.join(
            os.path.dirname(os.path.dirname(__file__)), "Lib")
        return imp.load_module(modname, *imp.find_module(modname, [modpath]))
else:
    def find_module(modname):
        """Finds and returns a module in the local dist/checkout.
        """
        modpath = os.path.join(
            os.path.dirname(os.path.dirname(__file__)), "Lib", modname + ".py")
        return SourceFileLoader(modname, modpath).load_module()


def write_contents(f):
    """Write C code contents to the target file object.
    """
    opcode = find_module('opcode')
    targets = ['_unknown_opcode'] * 256
    targets[255] = "TARGET_DO_TRACING"
    for opname, op in opcode.opmap.items():
        if not opcode.is_pseudo(op):
            targets[op] = "TARGET_%s" % opname
    next_op = 1
    for opname in opcode._specialized_instructions:
        while targets[next_op] != '_unknown_opcode':
            next_op += 1
        targets[next_op] = "TARGET_%s" % opname
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
