# THIS IS OBSOLETE -- USE MODULE 'compileall' INSTEAD!

# Utility module to import all modules in the path, in the hope
# that this will update their ".pyc" files.

import os
import sys

# Sabotage 'gl' and 'stdwin' to prevent windows popping up...
for m in 'gl', 'stdwin', 'fl', 'fm':
	sys.modules[m] = sys

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
		if tail == '.py' and head not in exceptions:
			s = 'import ' + head
			print s
			try:
				exec s + '\n'
			except KeyboardInterrupt:
				del names[:]
				print '\n[interrupt]'
				break
			except:
				print 'Sorry:', sys.exc_type + ':',
				print sys.exc_value
