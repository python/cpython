#! /usr/local/bin/python

# findlinksto
#
# find symbolic links to a given path

import os, sys

def visit(pattern, dirname, names):
	if os.path.islink(dirname):
		names[:] = []
		return
	if os.path.ismount(dirname):
		print 'descend into', dirname
	n = len(pattern)
	for name in names:
		name = os.path.join(dirname, name)
		try:
			linkto = os.readlink(name)
			if linkto[:n] == pattern:
				print name, '->', linkto
		except os.error:
			pass

def main(pattern, args):
	for dirname in args:
		os.path.walk(dirname, visit, pattern)

main(sys.argv[1], sys.argv[2:])
