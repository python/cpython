#! /usr/bin/env python

# Fix Python script(s) to reference the interpreter via /usr/bin/env python.

import sys
import regex
import regsub


def main():
	for file in sys.argv[1:]:
		try:
			f = open(file, 'r+')
		except IOError:
			print file, ': can\'t open for update'
			continue
		line = f.readline()
		if regex.match('^#! */usr/local/bin/python', line) < 0:
			print file, ': not a /usr/local/bin/python script'
			f.close()
			continue
		rest = f.read()
		line = regsub.sub('/usr/local/bin/python',
				  '/usr/bin/env python', line)
		print file, ':', `line`
		f.seek(0)
		f.write(line)
		f.write(rest)
		f.close()


main()
