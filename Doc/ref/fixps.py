#! /usr/bin/env python

"""Dumb script to edit a particular line in the PostScript.

This makes it possible to print on A4 paper.
"""

f = open("ref.ps", "r")

lines = f.readlines()
f.close()
didit = 0

for i in range(100):
    if lines[i] == '/FMAllowPaperSizeMismatch            false def\n':
	lines[i] = '/FMAllowPaperSizeMismatch            true def\n'
	didit = 1
	break

if not didit:
    print "ref.ps not changed"
else:
    print "rewriting edited ref.ps"
    f = open("ref.ps", "w")
    f.writelines(lines)
    f.close()
