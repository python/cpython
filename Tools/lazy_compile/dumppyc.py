#! /usr/bin/env python3

import marshal
import dis
import types
import importlib
import importlib.util
import lazy_compile

def dump(obj):
    print(obj)
    for attr in dir(obj):
        if attr.startswith('co_'):
            val = getattr(obj, attr)
            print("\t", attr, repr(val))

def loadCode(path):
    with open(path, 'rb') as fp:
        header = fp.read(16) # discard
        code = marshal.load(fp)
    return code

def walk(co, match=None):
    if match is None or co.co_name == match:
        dump(co)
        print()
        dis.dis(co)
    for obj in co.co_consts:
        if type(obj) == types.CodeType:
            walk(obj, match)

def load(filename, codename=None):
    co = loadCode(filename)
    walk(co, codename)

if __name__ == "__main__":
    import sys
    if len(sys.argv) == 3:
        filename, codename = sys.argv[1:]
    else:
        filename = sys.argv[1]
        codename = None
    if filename.endswith('.py'):
        with open(filename) as fp:
            buf = fp.read()
        ast = lazy_compile.parse(buf, filename)
        co = compile(ast, filename, 'exec')
        walk(co)
    else:
        load(filename, codename)
