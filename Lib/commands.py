# Module 'commands'
#
# Various tools for executing commands and looking at their output and status.
#
# NB This only works (and is only relevant) for UNIX.


# Get 'ls -l' status for an object into a string
#
def getstatus(file):
	return getoutput('ls -ld' + mkarg(file))


# Get the output from a shell command into a string.
# The exit status is ignored; a trailing newline is stripped.
# Assume the command will work with '{ ... ; } 2>&1' around it..
#
def getoutput(cmd):
	return getstatusoutput(cmd)[1]


# Ditto but preserving the exit status.
# Returns a pair (sts, output)
#
def getstatusoutput(cmd):
	import posix
	pipe = posix.popen('{ ' + cmd + '; } 2>&1', 'r')
	text = pipe.read()
	sts = pipe.close()
	if sts == None: sts = 0
	if text[-1:] == '\n': text = text[:-1]
	return sts, text


# Make command argument from directory and pathname (prefix space, add quotes).
#
def mk2arg(head, x):
	import posixpath
	return mkarg(posixpath.join(head, x))


# Make a shell command argument from a string.
# Two strategies: enclose in single quotes if it contains none;
# otherwise, enclose in double quotes and prefix quotable characters
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
