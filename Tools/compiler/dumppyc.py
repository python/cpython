#! /usr/bin/env python

import marshal
import dis
import types

def dump(obj):
    print obj
    for attr in dir(obj):
        print "\t", attr, repr(getattr(obj, attr))

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
    main(filename, codename)
