#!/usr/bin/env python

# Released to the public domain by Skip Montanaro, 28 March 2002

"""
findsyms.py - try to identify undocumented symbols exported by modules

Usage:    findsyms.py librefdir

For each lib*.tex file in the libref manual source directory, identify which
module is documented, import the module if possible, then search the LaTeX
source for the symbols global to that module.  Report any that don't seem to
be documented.

Certain exceptions are made to the list of undocumented symbols:

    * don't mention symbols in which all letters are upper case on the
      assumption they are manifest constants

    * don't mention symbols that are themselves modules

    * don't mention symbols that match those exported by os, math, string,
      types, or __builtin__ modules

Finally, if a name is exported by the module but fails a getattr() lookup,
that anomaly is reported.
"""

import __builtin__
import getopt
import glob
import math
import os
import re
import string
import sys
import types
import warnings

def usage():
    print >> sys.stderr, """
usage: %s dir
where 'dir' is the Library Reference Manual source directory.
""" % os.path.basename(sys.argv[0])
    
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "")
    except getopt.error:
        usage()
        return

    if not args:
        usage()
        return

    libdir = args[0]
    
    warnings.filterwarnings("error")

    pat = re.compile(r"\\declaremodule\s*{[^}]*}\s*{([^}]*)}")

    missing = []
    filelist = glob.glob(os.path.join(libdir, "lib*.tex"))
    filelist.sort()
    for f in filelist:
        mod = f[3:-4]
        if not mod: continue
        data = open(f).read()
        mods = re.findall(pat, data)
        if not mods:
            print "No module declarations found in", f
            continue
        for modname in mods:
            # skip special modules
            if modname.startswith("__"):
                continue
            try:
                mod = __import__(modname)
            except ImportError:
                missing.append(modname)
                continue
            except DeprecationWarning:
                print "Deprecated module:", modname
                continue
            if hasattr(mod, "__all__"):
                all = mod.__all__
            else:
                all = [k for k in dir(mod) if k[0] != "_"]
            mentioned = 0
            all.sort()
            for name in all:
                if data.find(name) == -1:
                    # certain names are predominantly used for testing
                    if name in ("main","test","_test"):
                        continue
                    # is it some sort of manifest constant?
                    if name.upper() == name:
                        continue
                    try:
                        item = getattr(mod, name)
                    except AttributeError:
                        print "  ", name, "exposed, but not an attribute"
                        continue
                    # don't care about modules that might be exposed
                    if type(item) == types.ModuleType:
                        continue
                    # check a few modules which tend to be import *'d
                    isglobal = 0
                    for m in (os, math, string, __builtin__, types):
                        if hasattr(m, name) and item == getattr(m, name):
                            isglobal = 1
                            break
                    if isglobal: continue
                    if not mentioned:
                        print "Not mentioned in", modname, "docs:"
                        mentioned = 1
                    print "  ", name
    if missing:
        missing.sort()
        print "Could not import:"
        print "  ", ", ".join(missing)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
