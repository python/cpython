#! /usr/bin/env python
"""Add reference count annotations to the Python/C API Reference."""
__version__ = '$Revision$'

import getopt
import os
import string
import sys

import refcounts


PREFIX = r"\begin{cfuncdesc}{PyObject*}{"


def main():
    rcfile = os.path.join(os.path.dirname(refcounts.__file__), os.pardir,
                          "api", "refcounts.dat")
    outfile = "-"
    opts, args = getopt.getopt(sys.argv[1:], "o:r:", ["output=", "refcounts="])
    for opt, arg in opts:
        if opt in ("-o", "--output"):
            outfile = arg
        elif opt in ("-r", "--refcounts"):
            rcfile = arg
    rcdict = refcounts.load(rcfile)
    if outfile == "-":
        output = sys.stdout
    else:
        output = open(outfile, "w")
    if not args:
        args = ["-"]
    prefix = PREFIX
    prefix_len = len(prefix)
    for infile in args:
        if infile == "-":
            input = sys.stdin
        else:
            input = open(infile)
        while 1:
            line = input.readline()
            if not line:
                break
            if line[:prefix_len] == prefix:
                s = string.split(line[prefix_len:], '}', 1)[0]
                try:
                    info = rcdict[s]
                except KeyError:
                    sys.stderr.write("No refcount data for %s\n" % s)
                else:
                    if info.result_type == "PyObject*":
                        rc = info.result_refs and "New" or "Borrowed"
                        line = r"\begin{cfuncdesc}[%s]{PyObject*}{" % rc \
                               + line[prefix_len:]
            output.write(line)
        if infile != "-":
            input.close()
    if outfile != "-":
        output.close()


if __name__ == "__main__":
    main()
