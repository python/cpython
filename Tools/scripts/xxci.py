#! /ufs/guido/bin/sgi/python
#! /usr/local/python

# xxci
#
# check in files for which rcsdiff returns nonzero exit status

import sys
import posix
import stat
import path
import commands

EXECMAGIC = '\001\140\000\010'

MAXSIZE = 200*1024 # Files this big must be binaries and are skipped.

def getargs():
	args = sys.argv[1:]
	if args:
		return args
	print 'No arguments, checking almost *'
	for file in posix.listdir('.'):
		if not skipfile(file):
			args.append(file)
	if not args:
		print 'Nothing to do -- exit 1'
		sys.exit(1)
	args.sort()
	return args

badnames = ['tags', 'TAGS', 'xyzzy', 'nohup.out', 'core']
badprefixes = ['.', ',', '@', '#', 'o.']
badsuffixes = \
	['~', '.a', '.o', '.old', '.bak', '.orig', '.new', '.prev', '.not', \
	 '.pyc', '.elc']
# XXX Should generalize even more to use fnmatch!

def skipfile(file):
	if file in badnames or \
		badprefix(file) or badsuffix(file) or \
		path.islink(file) or path.isdir(file):
		return 1
	# Skip huge files -- probably binaries.
	try:
		st = posix.stat(file)
	except posix.error:
		return 1 # Doesn't exist -- skip it
	if st[stat.ST_SIZE] >= MAXSIZE: return 1
	# Skip executables
	try:
		data = open(file, 'r').read(len(EXECMAGIC))
		if data == EXECMAGIC: return 1
	except:
		pass
	return 0

def badprefix(file):
	for bad in badprefixes:
		if file[:len(bad)] == bad: return 1
	return 0

def badsuffix(file):
	for bad in badsuffixes:
		if file[-len(bad):] == bad: return 1
	return 0

def go(args):
	for file in args:
		print file + ':'
		if differing(file):
			sts = posix.system('rcsdiff ' + file) # ignored
			if askyesno('Check in ' + file + ' ? '):
				sts = posix.system('rcs -l ' + file) # ignored
				sts = posix.system('ci -l ' + file)

def differing(file):
	try:
		this = open(file, 'r').read()
		that = posix.popen('co -p '+file+' 2>/dev/null', 'r').read()
		return this <> that
	except:
		return 1

def askyesno(prompt):
	s = raw_input(prompt)
	return s in ['y', 'yes']

go(getargs())
