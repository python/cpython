# Parse Makefiles and Python Setup(.in) files.

import regex
import string


# Extract variable definitions from a Makefile.
# Return a dictionary mapping names to values.
# May raise IOError.

makevardef = regex.compile('^\([a-zA-Z0-9_]+\)[ \t]*=\(.*\)')

def getmakevars(filename):
	variables = {}
	fp = open(filename)
	try:
		while 1:
			line = fp.readline()
			if not line:
				break
			if makevardef.match(line) < 0:
				continue
			name, value = makevardef.group(1, 2)
			# Strip trailing comment
			i = string.find(value, '#')
			if i >= 0:
				value = value[:i]
			value = string.strip(value)
			variables[name] = value
	finally:
		fp.close()
	return variables


# Parse a Python Setup(.in) file.
# Return two dictionaries, the first mapping modules to their
# definitions, the second mapping variable names to their values.
# May raise IOError.

setupvardef = regex.compile('^\([a-zA-Z0-9_]+\)=\(.*\)')

def getsetupinfo(filename):
	modules = {}
	variables = {}
	fp = open(filename)
	try:
		while 1:
			line = fp.readline()
			if not line:
				break
			# Strip comments
			i = string.find(line, '#')
			if i >= 0:
				line = line[:i]
			if setupvardef.match(line) >= 0:
				name, value = setupvardef.group(1, 2)
				variables[name] = string.strip(value)
			else:
				words = string.split(line)
				if words:
					modules[words[0]] = words[1:]
	finally:
		fp.close()
	return modules, variables


# Test the above functions.

def test():
	import sys
	import os
	if not sys.argv[1:]:
		print 'usage: python parsesetup.py Makefile*|Setup* ...'
		sys.exit(2)
	for arg in sys.argv[1:]:
		base = os.path.basename(arg)
		if base[:8] == 'Makefile':
			print 'Make style parsing:', arg
			v = getmakevars(arg)
			prdict(v)
		elif base[:5] == 'Setup':
			print 'Setup style parsing:', arg
			m, v = getsetupinfo(arg)
			prdict(m)
			prdict(v)
		else:
			print arg, 'is neither a Makefile nor a Setup file'
			print '(name must begin with "Makefile" or "Setup")'

def prdict(d):
	keys = d.keys()
	keys.sort()
	for key in keys:
		value = d[key]
		print "%-15s" % key, str(value)

if __name__ == '__main__':
	test()
