#! /usr/local/bin/python

# ptags
#
# Create a tags file for Python programs, usable with vi.
# Tagged are:
# - functions (even inside other defs or classes)
# - classes
# - filenames
# Warns about files it cannot open.
# No warnings about duplicate tags.

import sys
import regex
import os

tags = []	# Modified global variable!

def main():
	args = sys.argv[1:]
	for file in args: treat_file(file)
	if tags:
		fp = open('tags', 'w')
		tags.sort()
		for s in tags: fp.write(s)


expr = '^[ \t]*\(def\|class\)[ \t]+\([a-zA-Z0-9_]+\)[ \t]*[:(]'
matcher = regex.compile(expr)

def treat_file(file):
	try:
		fp = open(file, 'r')
	except:
		print 'Cannot open', file
		return
	base = os.path.basename(file)
	if base[-3:] == '.py': base = base[:-3]
	s = base + '\t' + file + '\t' + '1\n'
	tags.append(s)
	while 1:
		line = fp.readline()
		if not line: break
		if matcher.search(line) >= 0:
			(a, b), (a1, b1), (a2, b2) = matcher.regs[:3]
			name = line[a2:b2]
			s = name + '\t' + file + '\t/^' + line[a:b] + '/\n'
			tags.append(s)

main()
