#! /usr/local/bin/python

# www2.py -- print the contents of a URL on stdout
# - error checking

import sys
import urllib
import types

def main():
	if len(sys.argv) != 2 or sys.argv[1][:1] == '-':
		print "Usage:", sys.argv[0], "url"
		sys.exit(2)
	url = sys.argv[1]
	fp = my_urlopen(url)
	while 1:
		line = fp.readline()
		if not line: break
		sys.stdout.write(line)

def my_urlopen(url):
	try:
		fp = urllib.urlopen(url)
		return fp
	except IOError, msg:
		if type(msg) == types.TupleType and len(msg) == 4:
			print msg[:3]
			m = msg[3]
			for line in m.headers:
				sys.stdout.write(line)
		else:
			print msg
		sys.exit(1)

main()
