# Routines to force "compilation" of all .py files in a directory
# tree or on sys.path.  By default recursion is pruned at a depth of
# 10 and the current directory, if it occurs in sys.path, is skipped.
# When called as a script, compiles argument directories, or sys.path
# if no arguments.
# After a similar module by Sjoerd Mullender.

import os
import sys
import py_compile

def compile_dir(dir, maxlevels = 10):
	print 'Listing', dir, '...'
	try:
		names = os.listdir(dir)
	except os.error:
		print "Can't list", dir
		names = []
	names.sort()
	for name in names:
		fullname = os.path.join(dir, name)
		if os.path.isfile(fullname):
			head, tail = name[:-3], name[-3:]
			if tail == '.py':
				print 'Compiling', fullname, '...'
				try:
					py_compile.compile(fullname)
				except KeyboardInterrupt:
					del names[:]
					print '\n[interrupt]'
					break
				except:
					if type(sys.exc_type) == type(''):
						exc_type_name = sys.exc_type
					else: exc_type_name = sys.exc_type.__name__
					print 'Sorry:', exc_type_name + ':',
					print sys.exc_value
		elif maxlevels > 0 and \
		     name != os.curdir and name != os.pardir and \
		     os.path.isdir(fullname) and \
		     not os.path.islink(fullname):
			compile_dir(fullname, maxlevels - 1)

def compile_path(skip_curdir = 1):
	for dir in sys.path:
		if dir == os.curdir and skip_curdir:
			print 'Skipping current directory'
		else:
			compile_dir(dir, 0)

def main():
	import getopt
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'l')
	except getopt.error, msg:
		print msg
		print "usage: compileall [-l] [directory ...]"
		print "-l: don't recurse down"
		print "if no arguments, -l sys.path is assumed"
	maxlevels = 10
	for o, a in opts:
		if o == '-l': maxlevels = 0
	if args:
		for dir in sys.argv[1:]:
			compile_dir(dir, maxlevels)
	else:
		compile_path()

if __name__ == '__main__':
	main()
