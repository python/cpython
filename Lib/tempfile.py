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
    if tempdir is not None:
	return tempdir
    attempdirs = ['/usr/tmp', '/tmp', os.getcwd(), os.curdir]
    if os.name == 'nt':
	attempdirs.insert(0, 'C:\\TEMP')
	attempdirs.insert(0, '\\TEMP')
    elif os.name == 'mac':
    	import macfs, MACFS
    	try:
    	     refnum, dirid = macfs.FindFolder(MACFS.kOnSystemDisk, MACFS.kTemporaryFolderType, 0)
    	     dirname = macfs.FSSpec((refnum, dirid, '')).as_pathname()
    	     attempdirs.insert(0, dirname)
	except macfs.error:
	    pass
    for envname in 'TMPDIR', 'TEMP', 'TMP':
	if os.environ.has_key(envname):
	    attempdirs.insert(0, os.environ[envname])
    testfile = gettempprefix() + 'test'
    for dir in attempdirs:
	try:
	    filename = os.path.join(dir, testfile)
	    fp = open(filename, 'w')
	    fp.write('blat')
	    fp.close()
	    os.unlink(filename)
	    tempdir = dir
	    break
	except IOError:
	    pass
    if tempdir is None:
	msg = "Can't find a usable temporary directory amongst " + `attempdirs`
	raise IOError, msg
    return tempdir


# Function to calculate a prefix of the filename to use

def gettempprefix():
	global template
	if template == None:
		if os.name == 'posix':
			template = '@' + `os.getpid()` + '.'
		elif os.name == 'mac':
			template = 'Python-Tmp-'
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


class TemporaryFileWrapper:
    """Temporary file wrapper

    This class provides a wrapper around files opened for temporary use.
    In particular, it seeks to automatically remove the file when it is
    no longer needed.
    """
    def __init__(self, file, path):
	self.file=file
	self.path=path

    def close(self):
	self.file.close()
	os.unlink(self.path)

    def __del__(self):
	try: self.close()
	except: pass

    def __getattr__(self, name):
	file=self.__dict__['file']
	a=getattr(file, name)
	setattr(self, name, a)
	return a


def TemporaryFile(mode='w+b', bufsize=-1):
    name=mktemp()
    file=open(name,mode,bufsize)
    try:
	os.unlink(name)
    except os.error:
	# Non-unix -- can't unlink file that's still open, use wrapper
	return TemporaryFileWrapper(file, name)
    else:
	return file
