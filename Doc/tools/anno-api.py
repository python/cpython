#! /usr/bin/env python
"""Add reference count annotations to the Python/C API Reference."""
__version__ = '$Revision$'

import getopt
import os
import sys

import refcounts


PREFIX_1 = r"\begin{cfuncdesc}{PyObject*}{"
PREFIX_2 = r"\begin{cfuncdesc}{PyVarObject*}{"


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
    for infile in args:
        if infile == "-":
            input = sys.stdin
        else:
            input = open(infile)
        while 1:
            line = input.readline()
            if not line:
                break
            prefix = None
            if line.startswith(PREFIX_1):
                prefix = PREFIX_1
            elif line.startswith(PREFIX_2):
                prefix = PREFIX_2
            if prefix:
                s = line[len(prefix):].split('}', 1)[0]
                try:
                    info = rcdict[s]
                except KeyError:
                    sys.stderr.write("No refcount data for %s\n" % s)
                else:
                    if info.result_type in ("PyObject*", "PyVarObject*"):
                        if info.result_refs is None:
                            rc = "Always \NULL{}"
                        else:
                            rc = info.result_refs and "New" or "Borrowed"
                            rc = rc + " reference"
                        line = (r"\begin{cfuncdesc}[%s]{%s}{"
                                % (rc, info.result_type)) \
                                + line[len(prefix):]
            output.write(line)
        if infile != "-":
            input.close()
    if outfile != "-":
        output.close()


if __name__ == "__main__":
    main()
