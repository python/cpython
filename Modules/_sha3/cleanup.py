#!/usr/bin/env python
# Copyright (C) 2012   Christian Heimes (christian@python.org)
# Licensed to PSF under a Contributor Agreement.
#
# cleanup Keccak sources

import os
import re

CPP1 = re.compile("^//(.*)")
CPP2 = re.compile(r"\ //(.*)")

STATICS = ("void ", "int ", "HashReturn ",
           "const UINT64 ", "UINT16 ", "    int prefix##")

HERE = os.path.dirname(os.path.abspath(__file__))
KECCAK = os.path.join(HERE, "kcp")

def getfiles():
    for name in os.listdir(KECCAK):
        name = os.path.join(KECCAK, name)
        if os.path.isfile(name):
            yield name

def cleanup(f):
    buf = []
    for line in f:
        # mark all functions and global data as static
        #if line.startswith(STATICS):
        #    buf.append("static " + line)
        #    continue
        # remove UINT64 typedef, we have our own
        if line.startswith("typedef unsigned long long int"):
            buf.append("/* %s */\n" % line.strip())
            continue
        ## remove #include "brg_endian.h"
        if "brg_endian.h" in line:
            buf.append("/* %s */\n" % line.strip())
            continue
        # transform C++ comments into ANSI C comments
        line = CPP1.sub(r"/*\1 */\n", line)
        line = CPP2.sub(r" /*\1 */\n", line)
        buf.append(line)
    return "".join(buf)

for name in getfiles():
    with open(name) as f:
        res = cleanup(f)
    with open(name, "w") as f:
        f.write(res)
