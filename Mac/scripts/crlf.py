#! /usr/local/bin/python

# Replace \r by \n -- useful after transferring files from the Mac...
# Run this on UNIX.
# Usage: crlf.py file ...

import sys
import os
import string

def main():
	args = sys.argv[1:]
	if not args:
		print 'usage:', sys.argv[0], 'file ...'
		sys.exit(2)
	for file in args:
		print file, '...'
		data = open(file, 'r').read()
		lines = string.splitfields(data, '\r')
		newdata = string.joinfields(lines, '\n')
		if newdata != data:
			print 'rewriting...'
			os.rename(file, file + '~')
			open(file, 'w').write(newdata)
			print 'done.'
		else:
			print 'no change.'

main()
