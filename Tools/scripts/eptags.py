#! /usr/bin/env python

# eptags
#
# Create a TAGS file for Python programs, usable with GNU Emacs (version 18).
# Tagged are:
# - functions (even inside other defs or classes)
# - classes
# Warns about files it cannot open.
# No warnings about duplicate tags.

import sys
import regex

def main():
	outfp = open('TAGS', 'w')
	args = sys.argv[1:]
	for file in args:
		treat_file(file, outfp)

expr = '^[ \t]*\(def\|class\)[ \t]+\([a-zA-Z0-9_]+\)[ \t]*[:(]'
matcher = regex.compile(expr)

def treat_file(file, outfp):
	try:
		fp = open(file, 'r')
	except:
		print 'Cannot open', file
		return
	charno = 0
	lineno = 0
	tags = []
	size = 0
	while 1:
		line = fp.readline()
		if not line: break
		lineno = lineno + 1
		if matcher.search(line) >= 0:
			(a, b), (a1, b1), (a2, b2) = matcher.regs[:3]
			name = line[a2:b2]
			pat = line[a:b]
			tag = pat + '\177' + `lineno` + ',' + `charno` + '\n'
			tags.append((name, tag))
			size = size + len(tag)
		charno = charno + len(line)
	outfp.write('\f\n' + file + ',' + `size` + '\n')
	for name, tag in tags:
		outfp.write(tag)

main()
