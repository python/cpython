#! /usr/local/bin/python

# www1.py -- print the contents of a URL on stdout

import sys
import urllib

def main():
	if len(sys.argv) != 2 or sys.argv[1][:1] == '-':
		print "Usage:", sys.argv[0], "url"
		sys.exit(2)
	url = sys.argv[1]
	fp = urllib.urlopen(url)
	while 1:
		line = fp.readline()
		if not line: break
		print line,

main()
