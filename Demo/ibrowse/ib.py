#! /usr/local/bin/python

# Call ibrowse (the info file browser) under UNIX.

import sys
import ibrowse

if len(sys.argv) > 1:
	file = sys.argv[1]
	if len(sys.argv) > 2:
		if len(sys.argv) > 3:
			sys.stdout = sys.stderr
			print 'usage:', sys.argv[0], '[file [node]]'
			sys.exit(2)
		else:
			node = sys.argv[2]
	else:
		node = ''
	ibrowse.start('(' + file + ')' + node)
else:
	ibrowse.main()
