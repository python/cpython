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
import string
import StringIO
import sys


def strcasecmp(e1, e2, lower=string.lower):
    return cmp(lower(e1[1]), lower(e2[1])) or cmp(e1, e2)


def dump_entries(ofp, entries):
    if len(entries) == 1:
	ofp.write("  \\item %s (%s)%s\n" % entries[0])
	return
    ofp.write("  \item %s\n" % entries[0][0])
    # now sort these in a case insensitive manner:
    entries.sort(strcasecmp)
    for xxx, subitem, pages in entries:
	ofp.write("    \subitem %s%s\n" % (subitem, pages))


breakable_re = re.compile(r"  \\item (.*) [(](.*)[)]((?:, \d+)+)")

def main():
    import getopt
    outfile = None
    opts, args = getopt.getopt(sys.argv[1:], "o:")
    for opt, val in opts:
	if opt in ("-o", "--output"):
	    outfile = val
    filename = args[0]
    outfile = outfile or filename
    if filename == "-":
	fp = sys.stdin
    else:
	fp = open(filename)
    ofp = StringIO.StringIO()
    item, subitem = None, None
    entries = []
    while 1:
	line = fp.readline()
	if not line:
	    break
	m = breakable_re.match(line)
	if m:
	    entry = m.group(1, 2, 3)
	    if entries:
		if entries[-1][0] != entry[0]:
		    dump_entries(ofp, entries)
		    entries = []
	    entries.append(entry)
	elif entries:
	    dump_entries(ofp, entries)
	    entries = []
	    ofp.write(line)
	else:
	    pass
	    ofp.write(line)
    fp.close()
    if outfile == "-":
	fp = sys.stdout
    else:
	fp = open(outfile, "w")
    fp.write(ofp.getvalue())
    fp.close()


if __name__ == "__main__":
    main()
