#! /usr/local/python

# mkreal
#
# turn a symlink to a directory into a real directory

import sys
import posix
import path
from stat import *

join = path.join

error = 'mkreal error'

BUFSIZE = 32*1024

def mkrealfile(name):
	st = posix.stat(name) # Get the mode
	mode = S_IMODE(st[ST_MODE])
	linkto = posix.readlink(name) # Make sure again it's a symlink
	f_in = open(name, 'r') # This ensures it's a file
	posix.unlink(name)
	f_out = open(name, 'w')
	while 1:
		buf = f_in.read(BUFSIZE)
		if not buf: break
		f_out.write(buf)
	del f_out # Flush data to disk before changing mode
	posix.chmod(name, mode)

def mkrealdir(name):
	st = posix.stat(name) # Get the mode
	mode = S_IMODE(st[ST_MODE])
	linkto = posix.readlink(name)
	files = posix.listdir(name)
	posix.unlink(name)
	posix.mkdir(name, mode)
	posix.chmod(name, mode)
	linkto = join('..', linkto)
	#
	for file in files:
		if file not in ('.', '..'):
			posix.symlink(join(linkto, file), join(name, file))

def main():
	sys.stdout = sys.stderr
	progname = path.basename(sys.argv[0])
	args = sys.argv[1:]
	if not args:
		print 'usage:', progname, 'path ...'
		sys.exit(2)
	status = 0
	for name in args:
		if not path.islink(name):
			print progname+':', name+':', 'not a symlink'
			status = 1
		else:
			if path.isdir(name):
				mkrealdir(name)
			else:
				mkrealfile(name)
	sys.exit(status)

main()
