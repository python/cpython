#! /usr/local/bin/python

# www3.py -- print the contents of a URL on stdout
# - error checking
# - Error 302 handling

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
			m = msg[3]
			if msg[1] == 302:
				if m.has_key('location'):
					url = m['location']
					print 'Location:', url
					return my_urlopen(url)
				elif m.has_key('uri'):
					url = m['uri']
					print 'URI:', url
					return my_urlopen(url)
				print '(Error 302 w/o Location/URI header???)'
			print msg[:3]
			for line in m.headers:
				sys.stdout.write(line)
		else:
			print msg
		sys.exit(1)

main()
