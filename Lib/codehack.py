# A subroutine for extracting a function name from a code object
# (with cache)

import sys
from stat import *
import string
import os
import linecache

# XXX The functions getcodename() and getfuncname() are now obsolete
# XXX as code and function objects now have a name attribute --
# XXX co.co_name and f.func_name.
# XXX getlineno() is now also obsolete because of the new attribute
# XXX of code objects, co.co_firstlineno.

# Extract the function or class name from a code object.
# This is a bit of a hack, since a code object doesn't contain
# the name directly.  So what do we do:
# - get the filename (which *is* in the code object)
# - look in the code string to find the first SET_LINENO instruction
#   (this must be the first instruction)
# - get the line from the file
# - if the line starts with 'class' or 'def' (after possible whitespace),
#   extract the following identifier
#
# This breaks apart when the function was read from <stdin>
# or constructed by exec(), when the file is not accessible,
# and also when the file has been modified or when a line is
# continued with a backslash before the function or class name.
#
# Because this is a pretty expensive hack, a cache is kept.

SET_LINENO = 127 # The opcode (see "opcode.h" in the Python source)
identchars = string.letters + string.digits + '_' # Identifier characters

_namecache = {} # The cache

def getcodename(co):
	try:
		return co.co_name
	except AttributeError:
		pass
	key = `co` # arbitrary but uniquely identifying string
	if _namecache.has_key(key): return _namecache[key]
	filename = co.co_filename
	code = co.co_code
	name = ''
	if ord(code[0]) == SET_LINENO:
		lineno = ord(code[1]) | ord(code[2]) << 8
		line = linecache.getline(filename, lineno)
		words = string.split(line)
		if len(words) >= 2 and words[0] in ('def', 'class'):
			name = words[1]
			for i in range(len(name)):
				if name[i] not in identchars:
					name = name[:i]
					break
	_namecache[key] = name
	return name

# Use the above routine to find a function's name.

def getfuncname(func):
	try:
		return func.func_name
	except AttributeError:
		pass
	return getcodename(func.func_code)

# A part of the above code to extract just the line number from a code object.

def getlineno(co):
	try:
		return co.co_firstlineno
	except AttributeError:
		pass
	code = co.co_code
	if ord(code[0]) == SET_LINENO:
		return ord(code[1]) | ord(code[2]) << 8
	else:
		return -1
