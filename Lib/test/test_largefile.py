#!python

#----------------------------------------------------------------------
# test largefile support on system where this makes sense
#
#XXX how to only run this when support is there
#XXX how to only optionally run this, it will take along time
#----------------------------------------------------------------------

import test_support
import os, struct, stat, sys


# only run if the current system support large files
f = open(test_support.TESTFN, 'w')
try:
	# 2**31 == 2147483648
	f.seek(2147483649L)
except OverflowError:
	raise test_support.TestSkipped, "platform does not have largefile support"
else:
	f.close()


# create >2GB file (2GB = 2147483648 bytes)
size = 2500000000L
name = test_support.TESTFN


# on Windows this test comsumes large resources:
#  it takes a long time to build the >2GB file and takes >2GB of disk space
# therefore test_support.use_large_resources must be defined to run this test
if sys.platform[:3] == 'win' and not test_support.use_large_resources:
	raise test_support.TestSkipped, \
		"test requires %s bytes and a long time to run" % str(size)



def expect(got_this, expect_this):
	if test_support.verbose:
		print '%s =?= %s ...' % (`got_this`, `expect_this`),
	if got_this != expect_this:
		if test_support.verbose:
			print 'no'
		raise test_support.TestFailed, 'got %s, but expected %s' %\
			(str(got_this), str(expect_this))
	else:
		if test_support.verbose:
			print 'yes'


# test that each file function works as expected for a large (i.e. >2GB, do
# we have to check >4GB) files

if test_support.verbose:
	print 'create large file via seek (may be sparse file) ...'
f = open(name, 'w')
f.seek(size)
f.write('a')
f.flush()
expect(os.fstat(f.fileno())[stat.ST_SIZE], size+1)
if test_support.verbose:
	print 'check file size with os.fstat'
f.close()
if test_support.verbose:
	print 'check file size with os.stat'
expect(os.stat(name)[stat.ST_SIZE], size+1)

if test_support.verbose:
	print 'play around with seek() and read() with the built largefile'
f = open(name, 'r')
expect(f.tell(), 0)
expect(f.read(1), '\000')
expect(f.tell(), 1)
f.seek(0)
expect(f.tell(), 0)
f.seek(0, 0)
expect(f.tell(), 0)
f.seek(42)
expect(f.tell(), 42)
f.seek(42, 0)
expect(f.tell(), 42)
f.seek(42, 1)
expect(f.tell(), 84)
f.seek(0, 1)
expect(f.tell(), 84)
f.seek(0, 2) # seek from the end
expect(f.tell(), size + 1 + 0)
f.seek(-10, 2)
expect(f.tell(), size + 1 - 10)
f.seek(-size-1, 2)
expect(f.tell(), 0)
f.seek(size)
expect(f.tell(), size)
expect(f.read(1), 'a') # the 'a' that was written at the end of the file above
f.close()

if test_support.verbose:
	print 'play around with os.lseek() with the built largefile'
f = open(name, 'r')
expect(os.lseek(f.fileno(), 0, 0), 0)
expect(os.lseek(f.fileno(), 42, 0), 42)
expect(os.lseek(f.fileno(), 42, 1), 84)
expect(os.lseek(f.fileno(), 0, 1), 84)
expect(os.lseek(f.fileno(), 0, 2), size+1+0)
expect(os.lseek(f.fileno(), -10, 2), size+1-10)
expect(os.lseek(f.fileno(), -size-1, 2), 0)
expect(os.lseek(f.fileno(), size, 0), size)
expect(f.read(1), 'a') # the 'a' that was written at the end of the file above
f.close()


# XXX add tests for truncate if it exists
# XXX has truncate ever worked on Windows? specifically on WinNT I get:
#     "IOError: [Errno 13] Permission denied"
##try:
##	newsize = size - 10
##	f.seek(newsize)
##	f.truncate()
##	expect(f.tell(), newsize)
##	newsize = newsize - 1
##	f.seek(0)
##	f.truncate(newsize)
##	expect(f.tell(), newsize)
##except AttributeError:
##	pass

os.unlink(name)

