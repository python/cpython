# Temporary file name allocation

import posix
import path


# Changeable parameters (by clients!)...

tempdir = '/usr/tmp'
template = '@'

# Use environment variable $TMPDIR to override default tempdir.

if posix.environ.has_key('TMPDIR'):
	# XXX Could check that it's a writable directory...
	tempdir = posix.environ['TMPDIR']


# Counter for generating unique names

counter = 0


# User-callable function
# XXX Should this have a parameter, like C's mktemp()?
# XXX Should we instead use the model of Standard C's tempnam()?
# XXX By all means, avoid a mess with four different functions like C...

def mktemp():
	global counter
	while 1:
		counter = counter+1
		file = tempdir+'/'+template+`posix.getpid()`+'.'+`counter`
		if not path.exists(file):
			break
	return file
