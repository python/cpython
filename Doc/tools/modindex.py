#! /usr/bin/env python

"""Script to convert raw module index data to module index."""

import os
import string
import sys


def parse_line(input):
    lineno = string.split(input)[-1]
    module = input[:len(input)-(len(lineno)+1)]
    return module, lineno
    return 


def cmp_items((s1, line1), (s2, line2)):
    rc = cmp(string.lower(s1), string.lower(s2))
    if rc == 0:
	# Make the lower-case version come first since the upper-case
	# version is usually a helper module for constants and such.
	rc = cmp(s2, s1)
    return rc


def main():
    if sys.argv[1:]:
	infile = sys.argv[1]
    else:
	infile = "-"
    if infile == "-":
	ifp = sys.stdin
	ofp = sys.stdout
	sys.stdout = sys.stderr
    else:
	ifp = open(infile)
	base, ext = os.path.splitext(infile)
	outfile = base + ".ind"
	ofp = open(outfile, "w")
    ofp.write("\\begin{theindex}\n\n")
    lines = ifp.readlines()
    for i in range(len(lines)):
	if lines[i][0] == '\\':
	    lines[i] = '\1' + lines[i]
    lines = map(parse_line, lines)
    lines.sort(cmp_items)
    prev_letter = lines[0][0][0]
    if prev_letter == '\1':
	prev_letter = lines[0][0][1]
    prev_letter = string.lower(prev_letter)
    for module, lineno in lines:
	if module[0] == '\1':
	    module = module[1:]
	if string.lower(module[0]) != prev_letter:
	    ofp.write("\n  \\indexspace\n\n")
	    prev_letter = string.lower(module[0])
	ofp.write("  \\item {\\tt %s} %s\n" % (module, lineno))
    ofp.write("\n\\end{theindex}\n")


if __name__ == "__main__":
    main()
