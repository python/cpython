# Temporary file name allocation

import posix
import path


# Changeable parameters (by clients!)...
# XXX Should the environment variable $TMPDIR override tempdir?

tempdir = '/usr/tmp'
template = '@'


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
