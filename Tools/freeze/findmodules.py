# Determine the names and filenames of the modules imported by a
# script, recursively.  This is done by scanning for lines containing
# import statements.  (The scanning has only superficial knowledge of
# Python syntax and no knowledge of semantics, so in theory the result
# may be incorrect -- however this is quite unlikely if you don't
# intentionally obscure your Python code.)

import os
import regex
import string
import sys


# Top-level interface.
# First argument is the main program (script).
# Second optional argument is list of modules to be searched as well.

def findmodules(scriptfile, modules = [], path = sys.path):
	todo = {}
	todo['__main__'] = scriptfile
	for name in modules:
		mod = os.path.basename(name)
		if mod[-3:] == '.py': mod = mod[:-3]
		todo[mod] = name
	done = closure(todo)
	return done


# Compute the closure of scanfile() and findmodule().
# Return a dictionary mapping module names to filenames.
# Writes to stderr if a file can't be or read.

def closure(todo):
	done = {}
	while todo:
		newtodo = {}
		for modname in todo.keys():
			if not done.has_key(modname):
				filename = todo[modname]
				if filename is None:
					filename = findmodule(modname)
				done[modname] = filename
				if filename in ('<builtin>', '<unknown>'):
					continue
				try:
					modules = scanfile(filename)
				except IOError, msg:
					sys.stderr.write("%s: %s\n" %
							 (filename, str(msg)))
					continue
				for m in modules:
					if not done.has_key(m):
						newtodo[m] = None
		todo = newtodo
	return done


# Scan a file looking for import statements.
# Return list of module names.
# Can raise IOError.

importstr = '\(^\|:\)[ \t]*import[ \t]+\([a-zA-Z0-9_, \t]+\)'
fromstr   = '\(^\|:\)[ \t]*from[ \t]+\([a-zA-Z0-9_]+\)[ \t]+import[ \t]+'
isimport = regex.compile(importstr)
isfrom = regex.compile(fromstr)

def scanfile(filename):
	allmodules = {}
	f = open(filename, 'r')
	try:
		while 1:
			line = f.readline()
			if not line: break # EOF
			while line[-2:] == '\\\n': # Continuation line
				line = line[:-2] + ' '
				line = line + f.readline()
			if isimport.search(line) >= 0:
				rawmodules = isimport.group(2)
				modules = string.splitfields(rawmodules, ',')
				for i in range(len(modules)):
					modules[i] = string.strip(modules[i])
			elif isfrom.search(line) >= 0:
				modules = [isfrom.group(2)]
			else:
				continue
			for mod in modules:
				allmodules[mod] = None
	finally:
		f.close()
	return allmodules.keys()


# Find the file containing a module, given its name.
# Return filename, or '<builtin>', or '<unknown>'.

builtins = sys.builtin_module_names
if 'sys' not in builtins: builtins.append('sys')
# XXX this table may have to be changed depending on your platform:
tails = ['.so', 'module.so', '.py', '.pyc']

def findmodule(modname, path = sys.path):
	if modname in builtins: return '<builtin>'
	for dirname in path:
		for tail in tails:
			fullname = os.path.join(dirname, modname + tail)
			try:
				f = open(fullname, 'r')
			except IOError:
				continue
			f.close()
			return fullname
	return '<unknown>'


# Test the above functions.

def test():
	if not sys.argv[1:]:
		print 'usage: python findmodules.py scriptfile [morefiles ...]'
		sys.exit(2)
	done = findmodules(sys.argv[1], sys.argv[2:])
	items = done.items()
	items.sort()
	for mod, file in [('Module', 'File')] + items:
		print "%-15s %s" % (mod, file)

if __name__ == '__main__':
	test()
