#! /usr/bin/env python

# Fix Python source files to use the new equality test operator, i.e.,
#	if x = y: ...
# is changed to
#	if x == y: ...
# The script correctly tokenizes the Python program to reliably
# distinguish between assignments and equality tests.
#
# Command line arguments are files or directories to be processed.
# Directories are searched recursively for files whose name looks
# like a python module.
# Symbolic links are always ignored (except as explicit directory
# arguments).  Of course, the original file is kept as a back-up
# (with a "~" attached to its name).
# It complains about binaries (files containing null bytes)
# and about files that are ostensibly not Python files: if the first
# line starts with '#!' and does not contain the string 'python'.
#
# Changes made are reported to stdout in a diff-like format.
#
# Undoubtedly you can do this using find and sed or perl, but this is
# a nice example of Python code that recurses down a directory tree
# and uses regular expressions.  Also note several subtleties like
# preserving the file's mode and avoiding to even write a temp file
# when no changes are needed for a file.
#
# NB: by changing only the function fixline() you can turn this
# into a program for a different change to Python programs...

import sys
import regex
import os
from stat import *
import string

err = sys.stderr.write
dbg = err
rep = sys.stdout.write

def main():
	bad = 0
	if not sys.argv[1:]: # No arguments
		err('usage: ' + sys.argv[0] + ' file-or-directory ...\n')
		sys.exit(2)
	for arg in sys.argv[1:]:
		if os.path.isdir(arg):
			if recursedown(arg): bad = 1
		elif os.path.islink(arg):
			err(arg + ': will not process symbolic links\n')
			bad = 1
		else:
			if fix(arg): bad = 1
	sys.exit(bad)

ispythonprog = regex.compile('^[a-zA-Z0-9_]+\.py$')
def ispython(name):
	return ispythonprog.match(name) >= 0

def recursedown(dirname):
	dbg('recursedown(' + `dirname` + ')\n')
	bad = 0
	try:
		names = os.listdir(dirname)
	except os.error, msg:
		err(dirname + ': cannot list directory: ' + `msg` + '\n')
		return 1
	names.sort()
	subdirs = []
	for name in names:
		if name in (os.curdir, os.pardir): continue
		fullname = os.path.join(dirname, name)
		if os.path.islink(fullname): pass
		elif os.path.isdir(fullname):
			subdirs.append(fullname)
		elif ispython(name):
			if fix(fullname): bad = 1
	for fullname in subdirs:
		if recursedown(fullname): bad = 1
	return bad

def fix(filename):
##	dbg('fix(' + `filename` + ')\n')
	try:
		f = open(filename, 'r')
	except IOError, msg:
		err(filename + ': cannot open: ' + `msg` + '\n')
		return 1
	head, tail = os.path.split(filename)
	tempname = os.path.join(head, '@' + tail)
	g = None
	# If we find a match, we rewind the file and start over but
	# now copy everything to a temp file.
	lineno = 0
	while 1:
		line = f.readline()
		if not line: break
		lineno = lineno + 1
		if g is None and '\0' in line:
			# Check for binary files
			err(filename + ': contains null bytes; not fixed\n')
			f.close()
			return 1
		if lineno == 1 and g is None and line[:2] == '#!':
			# Check for non-Python scripts
			words = string.split(line[2:])
			if words and regex.search('[pP]ython', words[0]) < 0:
				msg = filename + ': ' + words[0]
				msg = msg + ' script; not fixed\n'
				err(msg)
				f.close()
				return 1
		while line[-2:] == '\\\n':
			nextline = f.readline()
			if not nextline: break
			line = line + nextline
			lineno = lineno + 1
		newline = fixline(line)
		if newline != line:
			if g is None:
				try:
					g = open(tempname, 'w')
				except IOError, msg:
					f.close()
					err(tempname+': cannot create: '+\
					    `msg`+'\n')
					return 1
				f.seek(0)
				lineno = 0
				rep(filename + ':\n')
				continue # restart from the beginning
			rep(`lineno` + '\n')
			rep('< ' + line)
			rep('> ' + newline)
		if g is not None:
			g.write(newline)

	# End of file
	f.close()
	if not g: return 0 # No changes
	
	# Finishing touch -- move files

	# First copy the file's mode to the temp file
	try:
		statbuf = os.stat(filename)
		os.chmod(tempname, statbuf[ST_MODE] & 07777)
	except os.error, msg:
		err(tempname + ': warning: chmod failed (' + `msg` + ')\n')
	# Then make a backup of the original file as filename~
	try:
		os.rename(filename, filename + '~')
	except os.error, msg:
		err(filename + ': warning: backup failed (' + `msg` + ')\n')
	# Now move the temp file to the original file
	try:
		os.rename(tempname, filename)
	except os.error, msg:
		err(filename + ': rename failed (' + `msg` + ')\n')
		return 1
	# Return succes
	return 0


from tokenize import tokenprog

match = {'if':':', 'elif':':', 'while':':', 'return':'\n', \
	 '(':')', '[':']', '{':'}', '`':'`'}

def fixline(line):
	# Quick check for easy case
	if '=' not in line: return line
	
	i, n = 0, len(line)
	stack = []
	while i < n:
		j = tokenprog.match(line, i)
		if j < 0:
			# A bad token; forget about the rest of this line
			print '(Syntax error:)'
			print line,
			return line
		a, b = tokenprog.regs[3] # Location of the token proper
		token = line[a:b]
		i = i+j
		if stack and token == stack[-1]:
			del stack[-1]
		elif match.has_key(token):
			stack.append(match[token])
		elif token == '=' and stack:
			line = line[:a] + '==' + line[b:]
			i, n = a + len('=='), len(line)
		elif token == '==' and not stack:
			print '(Warning: \'==\' at top level:)'
			print line,
	return line


main()
