#! /usr/local/python

# findlinksto
#
# find symbolic links to a given path

import posix, path, sys

def visit(pattern, dirname, names):
	if path.islink(dirname):
		names[:] = []
		return
	if path.ismount(dirname):
		print 'descend into', dirname
	n = len(pattern)
	for name in names:
		name = path.join(dirname, name)
		try:
			linkto = posix.readlink(name)
			if linkto[:n] == pattern:
				print name, '->', linkto
		except posix.error:
			pass

def main(pattern, args):
	for dirname in args:
		path.walk(dirname, visit, pattern)

main(sys.argv[1], sys.argv[2:])
