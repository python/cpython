# Temporary file name allocation
#
# XXX This tries to be not UNIX specific, but I don't know beans about
# how to choose a temp directory or filename on MS-DOS or other
# systems so it may have to be changed...


import os


# Parameters that the caller may set to override the defaults

tempdir = None
template = None


# Function to calculate the directory to use

def gettempdir():
	global tempdir
	if tempdir == None:
		try:
			tempdir = os.environ['TMPDIR']
		except (KeyError, AttributeError):
			if os.name == 'posix':
				tempdir = '/usr/tmp' # XXX Why not /tmp?
			else:
				tempdir = os.getcwd() # XXX Is this OK?
	return tempdir


# Function to calculate a prefix of the filename to use

def gettempprefix():
	global template
	if template == None:
		if os.name == 'posix':
			template = '@' + `os.getpid()` + '.'
		else:
			template = 'tmp' # XXX might choose a better one
	return template


# Counter for generating unique names

counter = 0


# User-callable function to return a unique temporary file name

def mktemp():
	global counter
	dir = gettempdir()
	pre = gettempprefix()
	while 1:
		counter = counter + 1
		file = os.path.join(dir, pre + `counter`)
		if not os.path.exists(file):
			return file
