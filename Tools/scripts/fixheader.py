#! /usr/bin/env python

# Add some standard cpp magic to a header file

import sys
import string

def main():
	args = sys.argv[1:]
	for file in args:
		process(file)

def process(file):
	try:
		f = open(file, 'r')
	except IOError, msg:
		sys.stderr.write('%s: can\'t open: %s\n' % (file, str(msg)))
		return
	data = f.read()
	f.close()
	if data[:2] <> '/*':
		sys.stderr.write('%s does not begin with C comment\n' % file)
		return
	try:
		f = open(file, 'w')
	except IOError, msg:
		sys.stderr.write('%s: can\'t write: %s\n' % (file, str(msg)))
		return
	sys.stderr.write('Processing %s ...\n' % file)
	magic = 'Py_'
	for c in file:
		if c in string.letters + string.digits:
			magic = magic + string.upper(c)
		else: magic = magic + '_'
	sys.stdout = f
	print '#ifndef', magic
	print '#define', magic
	print '#ifdef __cplusplus'
	print 'extern "C" {'
	print '#endif'
	print
	f.write(data)
	print
	print '#ifdef __cplusplus'
	print '}'
	print '#endif'
	print '#endif /*', '!'+magic, '*/'

main()
