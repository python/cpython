#! /usr/bin/env python

# Change the #! line occurring in Python scripts.  The new interpreter
# pathname must be given with a -i option.
#
# Command line arguments are files or directories to be processed.
# Directories are searched recursively for files whose name looks
# like a python module.
# Symbolic links are always ignored (except as explicit directory
# arguments).  Of course, the original file is kept as a back-up
# (with a "~" attached to its name).
#
# Undoubtedly you can do this using find and sed or perl, but this is
# a nice example of Python code that recurses down a directory tree
# and uses regular expressions.  Also note several subtleties like
# preserving the file's mode and avoiding to even write a temp file
# when no changes are needed for a file.
#
# NB: by changing only the function fixfile() you can turn this
# into a program for a different change to Python programs...

import sys
import regex
import os
from stat import *
import string
import getopt

err = sys.stderr.write
dbg = err
rep = sys.stdout.write

new_interpreter = None

def main():
	global new_interpreter
	usage = ('usage: %s -i /interpreter file-or-directory ...\n' %
		 sys.argv[0])
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'i:')
	except getopt.error, msg:
		err(msg + '\n')
		err(usage)
		sys.exit(2)
	for o, a in opts:
		if o == '-i':
			new_interpreter = a
	if not new_interpreter or new_interpreter[0] != '/' or not args:
		err('-i option or file-or-directory missing\n')
		err(usage)
		sys.exit(2)
	bad = 0
	for arg in args:
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
	line = f.readline()
	fixed = fixline(line)
	if line == fixed:
		rep(filename+': no change\n')
		f.close()
		return
	head, tail = os.path.split(filename)
	tempname = os.path.join(head, '@' + tail)
	try:
		g = open(tempname, 'w')
	except IOError, msg:
		f.close()
		err(tempname+': cannot create: '+`msg`+'\n')
		return 1
	rep(filename + ': updating\n')
	g.write(fixed)
	BUFSIZE = 8*1024
	while 1:
		buf = f.read(BUFSIZE)
		if not buf: break
		g.write(buf)
	g.close()
	f.close()

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

def fixline(line):
	if line[:2] != '#!':
		return line
	if string.find(line, "python") < 0:
		return line
	return '#! %s\n' % new_interpreter

main()
