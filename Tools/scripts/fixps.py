#! /usr/local/bin/python

# Fix Python script(s) to reference the interpreter in /usr/local/bin.

import sys
import regex
import regsub


def main():
	for file in sys.argv[1:]:
		try:
			f = open(file, 'r+')
		except IOError:
			print f, ': can\'t open for update'
			continue
		line = f.readline()
		if regex.match('^#! */usr/local/python', line) < 0:
			print file, ': not a /usr/local/python script'
			f.close()
			continue
		rest = f.read()
		line = regsub.sub('/usr/local/python', \
			'/usr/local/bin/python', line)
		print file, ':', `line`
		f.seek(0)
		f.write(line)
		f.write(rest)
		f.close()


main()
