#! /usr/bin/env python

"""Combine similar index entries into an entry and subentries.

For example:

    \item {foobar} (in module flotz), 23
    \item {foobar} (in module whackit), 4323

becomes

    \item {foobar}
      \subitem in module flotz, 23
      \subitem in module whackit, 4323

Note that an item which matches the format of a collapsable item but which
isn't part of a group of similar items is not modified.
"""
__version__ = '$Revision$'

import re
import io
import sys


def cmp_entries(e1, e2):
    return cmp(e1[1].lower(), e2[1].lower()) or cmp(e1, e2)


def dump_entries(write, entries):
    if len(entries) == 1:
        write("  \\item %s (%s)%s\n" % entries[0])
        return
    write("  \item %s\n" % entries[0][0])
    # now sort these in a case insensitive manner:
    if len(entries) > 0:
        entries.sort(cmp_entries)
    for xxx, subitem, pages in entries:
        write("    \subitem %s%s\n" % (subitem, pages))


breakable_re = re.compile(
    r"  \\item (.*) [(](.*)[)]((?:(?:, \d+)|(?:, \\[a-z]*\{\d+\}))+)")


def process(ifn, ofn=None):
    if ifn == "-":
        ifp = sys.stdin
    else:
        ifp = open(ifn)
    if ofn is None:
        ofn = ifn
    ofp = io.StringIO()
    entries = []
    match = breakable_re.match
    write = ofp.write
    while 1:
        line = ifp.readline()
        if not line:
            break
        m = match(line)
        if m:
            entry = m.group(1, 2, 3)
            if entries and entries[-1][0] != entry[0]:
                dump_entries(write, entries)
                entries = []
            entries.append(entry)
        elif entries:
            dump_entries(write, entries)
            entries = []
            write(line)
        else:
            write(line)
    del write
    del match
    ifp.close()
    data = ofp.getvalue()
    ofp.close()
    if ofn == "-":
        ofp = sys.stdout
    else:
        ofp = open(ofn, "w")
    ofp.write(data)
    ofp.close()


def main():
    import getopt
    outfile = None
    opts, args = getopt.getopt(sys.argv[1:], "o:")
    for opt, val in opts:
        if opt in ("-o", "--output"):
            outfile = val
    filename = args[0]
    outfile = outfile or filename
    process(filename, outfile)


if __name__ == "__main__":
    main()
