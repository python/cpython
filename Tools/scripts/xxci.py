#! /ufs/guido/bin/sgi/python
#! /usr/local/python

# xxci
#
# check in files for which rcsdiff returns nonzero exit status

import sys
import posix
from stat import *
import path
import commands
import fnmatch
import string

EXECMAGIC = '\001\140\000\010'

MAXSIZE = 200*1024 # Files this big must be binaries and are skipped.

def getargs():
	args = sys.argv[1:]
	if args:
		return args
	print 'No arguments, checking almost *, in "ls -t" order'
	list = []
	for file in posix.listdir('.'):
		if not skipfile(file):
			list.append((getmtime(file), file))
	list.sort()
	if not list:
		print 'Nothing to do -- exit 1'
		sys.exit(1)
	list.sort()
	list.reverse()
	for mtime, file in list: args.append(file)
	return args

def getmtime(file):
	try:
		st = posix.stat(file)
		return st[ST_MTIME]
	except posix.error:
		return -1

badnames = ['tags', 'TAGS', 'xyzzy', 'nohup.out', 'core']
badprefixes = ['.', ',', '@', '#', 'o.']
badsuffixes = \
	['~', '.a', '.o', '.old', '.bak', '.orig', '.new', '.prev', '.not', \
	 '.pyc', '.elc']

def setup():
	global ignore
	ignore = badnames[:]
	for p in badprefixes:
		ignore.append(p + '*')
	for p in badsuffixes:
		ignore.append('*' + p)
	try:
		f = open('.xxcign', 'r')
	except IOError:
		return
	ignore = ignore + string.split(f.read())

def skipfile(file):
	for p in ignore:
		if fnmatch.fnmatch(file, p): return 1
	try:
		st = posix.lstat(file)
	except posix.error:
		return 1 # Doesn't exist -- skip it
	# Skip non-plain files.
	if not S_ISREG(st[ST_MODE]): return 1
	# Skip huge files -- probably binaries.
	if st[ST_SIZE] >= MAXSIZE: return 1
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

setup()
go(getargs())
