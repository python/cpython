#! /usr/local/bin/python

import sys
import os
import string

def main():
	args = sys.argv[1:]
	if not args:
		print 'no files'
		sys.exit(1)
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
