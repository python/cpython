# Utility module to import all modules in the path, in the hope
# that this will update their ".pyc" files.

# First, see if this is the Mac or UNIX
try:
	import posix
	os = posix
	import path
except NameError:
	import mac
	os = mac
	import macpath
	path = macpath

import sys

exceptions = ['importall']

for dir in sys.path:
	print 'Listing', dir
	try:
		names = os.listdir(dir)
	except os.error:
		print 'Can\'t list', dir
		names = []
	names.sort()
	for name in names:
		head, tail = name[:-3], name[-3:]
		if tail = '.py' and head not in exceptions:
			s = 'import ' + head
			print s
			try:
				exec(s + '\n')
			except:
				print 'Sorry:', sys.exc_type, sys.exc_value
