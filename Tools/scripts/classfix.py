#! /usr/local/python

# Fix Python source files to use the new class definition syntax,
# i.e.,
#	class C(base, base, ...): ...
# instead of
#	class C() = base(), base(), ...: ...
#
# Command line arguments are files or directories to be processed.
# Directories are searched recursively for files whose name looks
# like a python module.
# Symbolic links are always ignored (except as explicit directory
# arguments).  Of course, the original file is kept as a back-up
# (with a "~" attached to its name).
#
# Undoubtedly you can do this using find and sed, but this is
# a nice example of Python code that recurses down a directory tree
# and uses regular expressions.  Also note several subtleties like
# preserving the file's mode and avoiding to even write a temp file
# when no changes are needed for a file.
#
# Changes made are reported to stdout in a diff-like format.

import sys
import regexp
import posix
import path
import string
from stat import *

err = sys.stderr.write
dbg = err
rep = sys.stdout.write

def main():
	bad = 0
	if not sys.argv[1:]: # No arguments
		err('usage: classfix file-or-directory ...\n')
		sys.exit(2)
	for arg in sys.argv[1:]:
		if path.isdir(arg):
			if recursedown(arg): bad = 1
		elif path.islink(arg):
			err(arg + ': will not process symbolic links\n')
			bad = 1
		else:
			if fix(arg): bad = 1
	sys.exit(bad)

ispython = regexp.compile('^[a-zA-Z0-9_]+\.py$').match # This is a method!

def recursedown(dirname):
	dbg('recursedown(' + `dirname` + ')\n')
	bad = 0
	try:
		names = posix.listdir(dirname)
	except posix.error, msg:
		err(dirname + ': cannot list directory: ' + `msg` + '\n')
		return 1
	for name in names:
		if name in ('.', '..'): continue
		fullname = path.join(dirname, name)
		if path.islink(fullname): pass
		elif path.isdir(fullname):
			if recursedown(fullname): bad = 1
		elif ispython(name):
			if fix(fullname): bad = 1
	return bad

# This expression doesn't catch *all* class definition headers,
# but it's darn pretty close.
classexpr = '^([ \t]*class +[a-zA-Z0-9_]+) *\( *\) *((=.*)?):'
findclass = regexp.compile(classexpr).match # This is a method!

baseexpr = '^ *(.*) *\( *\) *$'
findbase = regexp.compile(baseexpr).match # This is a method, too!

def fix(filename):
##	dbg('fix(' + `filename` + ')\n')
	try:
		f = open(filename, 'r')
	except IOError, msg:
		err(filename + ': cannot open: ' + `msg` + '\n')
		return 1
	head, tail = path.split(filename)
	tempname = path.join(head, '@' + tail)
	tf = None
	# If we find a match, we rewind the file and start over but
	# now copy everything to a temp file.
	while 1:
		line = f.readline()
		if not line: break
		res = findclass(line)
		if not res:
			if tf: tf.write(line)
			continue
		if not tf:
			try:
				tf = open(tempname, 'w')
			except IOError, msg:
				f.close()
				err(tempname+': cannot create: '+`msg`+'\n')
				return 1
			rep(filename + ':\n')
			# Rewind the input file and start all over:
			f.seek(0)
			continue
		a0, b0 = res[0] # Whole match (up to ':')
		a1, b1 = res[1] # First subexpression (up to classname)
		a2, b2 = res[2] # Second subexpression (=.*)
		head = line[:b1]
		tail = line[b0:] # Unmatched rest of line
		if a2 = b2: # No base classes -- easy case
			newline = head + ':' + tail
		else:
			# Get rid of leading '='
			basepart = line[a2+1:b2]
			# Extract list of base expressions
			bases = string.splitfields(basepart, ',')
			# Strip trailing '()' from each base expression
			for i in range(len(bases)):
				res = findbase(bases[i])
				if res:
					(x0, y0), (x1, y1) = res
					bases[i] = bases[i][x1:y1]
			# Join the bases back again and build the new line
			basepart = string.joinfields(bases, ', ')
			newline = head + '(' + basepart + '):' + tail
		rep('< ' + line)
		rep('> ' + newline)
		tf.write(newline)
	f.close()
	if not tf: return 0 # No changes
	
	# Finishing touch -- move files

	# First copy the file's mode to the temp file
	try:
		statbuf = posix.stat(filename)
		posix.chmod(tempname, statbuf[ST_MODE] & 07777)
	except posix.error, msg:
		err(tempname + ': warning: chmod failed (' + `msg` + ')\n')
	# Then make a backup of the original file as filename~
	try:
		posix.rename(filename, filename + '~')
	except posix.error, msg:
		err(filename + ': warning: backup failed (' + `msg` + ')\n')
	# Now move the temp file to the original file
	try:
		posix.rename(tempname, filename)
	except posix.error, msg:
		err(filename + ': rename failed (' + `msg` + ')\n')
		return 1
	# Return succes
	return 0

main()
