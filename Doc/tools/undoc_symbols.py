"""This script prints out a list of undocumented symbols found in
Python include files, prefixed by their tag kind.

First, a temporary file is written which contains all Python include
files, with DL_IMPORT simply removed.  This file is passed to ctags,
and the output is parsed into a dictionary mapping symbol names to tag
kinds.

Then, the .tex files from Python docs are read into a giant string.

Finally all symbols not found in the docs are written to standard
output, prefixed with their tag kind.
"""

# Which kind of tags do we need?
TAG_KINDS = "dpt"

# Doc sections to use
DOCSECTIONS = ["api", "ext"]

# Only print symbols starting with this prefix
# to get all symbols, use an empty string
PREFIX = "Py"

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

import os, glob, re, sys, tempfile

def findnames(file, prefix=""):
    names = {}
    for line in file.readlines():
        if line[0] == '!':
            continue
        fields = line.split()
        name, tag = fields[0], fields[-1]
        if tag == 'd' and name.endswith('_H'):
            continue
        if name.startswith(prefix):
            names[name] = tag
    return names

def print_undoc_symbols(prefix):
    incfile = tempfile.mktemp(".h")
    
    fp = open(incfile, "w")

    for file in glob.glob(os.path.join(INCDIR, "*.h")):
        text = open(file).read()
        # remove all DL_IMPORT, they will confuse ctags
        text = re.sub("DL_IMPORT", "", text)
        fp.write(text)
    fp.close()

    docs = []

    for sect in DOCSECTIONS:
        for file in glob.glob(os.path.join(DOCDIR, sect, "*.tex")):
            docs.append(open(file).read())

    docs = "\n".join(docs)

    fp = os.popen("ctags --c-types=%s -f - %s" % (TAG_KINDS, incfile))
    dict = findnames(fp, prefix)
    names = dict.keys()
    names.sort()
    for name in names:
        if docs.find(name) == -1:
            print dict[name], name
    os.remove(incfile)

if __name__ == '__main__':
    global INCDIR
    global DOCDIR
    SRCDIR = os.path.dirname(sys.argv[0])
    INCDIR = os.path.normpath(os.path.join(SRCDIR, "../../Include"))
    DOCDIR = os.path.normpath(os.path.join(SRCDIR, ".."))

    print_undoc_symbols(PREFIX)
