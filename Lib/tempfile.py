# Temporary file name allocation

import posix
import path


# Changeable parameters (by clients!)...
# XXX Should the environment variable $TMPDIR override tempdir?

tempdir = '/usr/tmp'
template = '@'


# Kludge to hold mutable state

class Struct(): pass
G = Struct()
G.i = 0


# User-callable function
# XXX Should this have a parameter, like C's mktemp()?
# XXX Should we instead use the model of Standard C's tempnam()?
# XXX By all means, avoid a mess with four different functions like C...

def mktemp():
	while 1:
		G.i = G.i+1
		file = tempdir +'/'+ template + `posix.getpid()` +'.'+ `G.i`
		if not path.exists(file):
			break
	return file
