#! /usr/bin/env python

import marshal
import os
import dis
import types

def dump(obj):
    print obj
    for attr in dir(obj):
        if attr.startswith('co_'):
            val = getattr(obj, attr)
            print "\t", attr, repr(val)

def loadCode(path):
    f = open(path)
    f.read(8)
    co = marshal.load(f)
    f.close()
    return co

def walk(co, match=None):
    if match is None or co.co_name == match:
        dump(co)
        print
        dis.dis(co)
    for obj in co.co_consts:
        if type(obj) == types.CodeType:
            walk(obj, match)

def main(filename, codename=None):
    co = loadCode(filename)
    walk(co, codename)

if __name__ == "__main__":
    import sys
    if len(sys.argv) == 3:
        filename, codename = sys.argv[1:]
    else:
        filename = sys.argv[1]
        codename = None
    if filename.endswith('.py') and os.path.exists(filename+"c"):
        filename += "c"
    main(filename, codename)
