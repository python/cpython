#! /usr/local/python

# xxci
#
# check in files for which rcsdiff returns nonzero exit status

import sys
import posix
import stat
import path
import commands

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

badnames = ['tags', 'xyzzy']
badprefixes = ['.', ',', '@', '#', 'o.']
badsuffixes = \
	['~', '.a', '.o', '.old', '.bak', '.orig', '.new', '.prev', '.not']
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
	return st[stat.ST_SIZE] >= MAXSIZE

def badprefix(file):
	for bad in badprefixes:
		if file[:len(bad)] = bad: return 1
	return 0

def badsuffix(file):
	for bad in badsuffixes:
		if file[-len(bad):] = bad: return 1
	return 0

def go(args):
	for file in args:
		print file + ':'
		if run('rcsdiff -c', file):
			if askyesno('Check in ' + file + ' ? '):
				sts = run('rcs -l', file) # ignored
				# can't use run() here because it's interactive
				sts = posix.system('ci -l ' + file)

def run(cmd, file):
	sts, output = commands.getstatusoutput(cmd + commands.mkarg(file))
	if sts:
		print output
		print 'Exit status', sts
	return sts

def askyesno(prompt):
	s = raw_input(prompt)
	return s in ['y', 'yes']

go(getargs())
