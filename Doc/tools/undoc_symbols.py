#! /usr/bin/env python

"""\
This script prints out a list of undocumented symbols found in
Python include files, prefixed by their tag kind.

Pass Python's include files to ctags, parse the output into a
dictionary mapping symbol names to tag kinds.

Then, the .tex files from Python docs are read into a giant string.

Finally all symbols not found in the docs are written to standard
output, prefixed with their tag kind.
"""

# Which kind of tags do we need?
TAG_KINDS = "dpst"

# Doc sections to use
DOCSECTIONS = ["api"]# ["api", "ext"]

# Only print symbols starting with this prefix,
# to get all symbols, use an empty string
PREFIXES = ("Py", "PY")

INCLUDEPATTERN = "*.h"

# end of customization section


# Tested with EXUBERANT CTAGS
# see http://ctags.sourceforge.net
#
# ctags fields are separated by tabs.
# The first field is the name, the last field the type:
# d macro definitions (and #undef names)
# e enumerators
# f function definitions
# g enumeration names
# m class, struct, or union members
# n namespaces
# p function prototypes and declarations
# s structure names
# t typedefs
# u union names
# v variable definitions
# x extern and forward variable declarations

import os, glob, re, sys

def findnames(file, prefixes=()):
    names = {}
    for line in file.xreadlines():
        if line[0] == '!':
            continue
        fields = line.split()
        name, tag = fields[0], fields[-1]
        if tag == 'd' and name.endswith('_H'):
            continue
        if prefixes:
            sw = name.startswith
            for prefix in prefixes:
                if sw(prefix):
                    names[name] = tag
        else:
            names[name] = tag
    return names

def print_undoc_symbols(prefix, docdir, incdir):
    docs = []

    for sect in DOCSECTIONS:
        for file in glob.glob(os.path.join(docdir, sect, "*.tex")):
            docs.append(open(file).read())

    docs = "\n".join(docs)

    incfiles = os.path.join(incdir, INCLUDEPATTERN)

    fp = os.popen("ctags -IPyAPI_FUNC -IPy_GCC_ATTRIBUTE --c-types=%s -f - %s"
                  % (TAG_KINDS, incfiles))
    dict = findnames(fp, prefix)
    names = dict.keys()
    names.sort()
    for name in names:
        if not re.search("%s\\W" % name, docs):
            print dict[name], name

if __name__ == '__main__':
    srcdir = os.path.dirname(sys.argv[0])
    incdir = os.path.normpath(os.path.join(srcdir, "../../Include"))
    docdir = os.path.normpath(os.path.join(srcdir, ".."))

    print_undoc_symbols(PREFIXES, docdir, incdir)
