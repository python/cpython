# Module 'commands'
#
# Various tools for executing commands and looking at their output and status.

import rand
import posix
import stat
import path


# Get 'ls -l' status for an object into a string
#
def getstatus(file):
	return getoutput('ls -ld' + mkarg(file))


# Get the output from a shell command into a string.
# The exit status is ignored; a trailing newline is stripped.
# Assume the command will work with ' >tempfile 2>&1' appended.
# XXX This should use posix.popen() instead, should it exist.
#
def getoutput(cmd):
	return getstatusoutput(cmd)[1]


# Ditto but preserving the exit status.
# Returns a pair (sts, output)
#
def getstatusoutput(cmd):
	tmp = '/usr/tmp/wdiff' + `rand.rand()`
	sts = -1
	try:
		sts = posix.system(cmd + ' >' + tmp + ' 2>&1')
		text = readfile(tmp)
	finally:
		altsts = posix.system('rm -f ' + tmp)
	if text[-1:] = '\n': text = text[:-1]
	return sts, text


# Return a string containing a file's contents.
#
def readfile(fn):
	st = posix.stat(fn)
	size = st[stat.ST_SIZE]
	if not size: return ''
	try:
		fp = open(fn, 'r')
	except:
		raise posix.error, 'readfile(' + fn + '): open failed'
	try:
		return fp.read(size)
	except:
		raise posix.error, 'readfile(' + fn + '): read failed'


# Make command argument from directory and pathname (prefix space, add quotes).
#
def mk2arg(head, x):
	return mkarg(path.cat(head, x))


# Make a shell command argument from a string.
# Two strategies: enclose in single quotes if it contains none;
# otherwis, enclose in double quotes and prefix quotable characters
# with backslash.
#
def mkarg(x):
	if '\'' not in x:
		return ' \'' + x + '\''
	s = ' "'
	for c in x:
		if c in '\\$"':
			s = s + '\\'
		s = s + c
	s = s + '"'
	return s
