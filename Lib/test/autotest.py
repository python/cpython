"""
Automatic Python regression test.

The list of individual tests is contained in the `testall' module.
These test some (but not all) essential parts of the Python
interpreter and built-in modules.  When a test fails, an exception is
raised and testing halts.  When a test succeeds, it can produce quite
a lot of output, which is compared against the output from a previous
run.  If a difference is noticed it raises an exception; if all is
well, it prints nothing except 'All tests OK.' at the very end.

The output from a previous run is supposed to be contained in separate
files (one per test) in the `output' subdirectory somewhere on the
search path for modules (sys.path, initialized from $PYTHONPATH plus
some default places).

Of course, if the normal output of the tests is changed because the
tests have been changed (rather than a test producing the wrong
output), 'autotest' will fail as well.  In this case, run 'autotest'
with the -g option.

Usage:

    %s [-g] [-w] [-h] [test1 [test2 ...]]

Options:

    -g, --generate : generate the output files instead of verifying
                     the results

    -w, --warn     : warn about un-importable tests

    -h, --help     : print this message

If individual tests are provided on the command line, only those tests
will be performed or generated.  Otherwise, all tests (as contained in
testall.py) will be performed.

"""

import os
import sys
import getopt
import traceback
import test_support
test_support.verbose = 0
from test_support import *

# Exception raised when the test failed (not the same as in test_support)
TestFailed = 'autotest.TestFailed'
TestMissing = 'autotest.TestMissing'

# defaults
generate = 0
warn = 0



# Function to find a file somewhere on sys.path
def findfile(filename):
	for dirname in sys.path:
		fullname = os.path.join(dirname, filename)
		if os.path.exists(fullname):
			return fullname
	return filename # Will cause exception later



# Class substituted for sys.stdout, to compare it with the given file
class Compare:
	def __init__(self, filename):
		self.fp = open(filename, 'r')
	def write(self, data):
		expected = self.fp.read(len(data))
		if data <> expected:
			raise TestFailed, \
				'Writing: '+`data`+', expected: '+`expected`
	def close(self):
		leftover = self.fp.read()
		if leftover:
			raise TestFailed, 'Unread: '+`leftover`
		self.fp.close()


# The main program
def usage(status):
	print __doc__ % sys.argv[0]
	sys.exit(status)



def do_one_test(t, outdir):
	filename = os.path.join(outdir, t)
	real_stdout = sys.stdout
	if generate:
		print 'Generating:', filename
		fake_stdout = open(filename, 'w')
	else:
		try:
			fake_stdout = Compare(filename)
		except IOError:
			raise TestMissing
	try:
		sys.stdout = fake_stdout
		print t
		unload(t)
		try:
			__import__(t, globals(), locals())
		except ImportError, msg:
			if warn:
				sys.stderr.write(msg+': Un-installed'
						 ' optional module?\n')
			try:
				fake_stdout.close()
			except TestFailed:
				pass
			fake_stdout = None
	finally:
		sys.stdout = real_stdout
		if fake_stdout:
			fake_stdout.close()



def main():
	global generate
	global warn
	try:
		opts, args = getopt.getopt(
			sys.argv[1:], 'ghw',
			['generate', 'help', 'warn'])
	except getopt.error, msg:
		print msg
		usage(1)
	for opt, val in opts:
		if opt in ['-h', '--help']:
			usage(0)
		elif opt in ['-g', '--generate']:
			generate = 1
		elif opt in ['-w', '--warn']:
			warn = 1

	# find the output directory
	outdir = findfile('output')
	if args:
	    tests = args
	else:
	    import testall
	    tests = testall.tests
	failed = 0
	for test in tests:
		print 'testing:', test
		try:
			do_one_test(test, outdir)
		except TestFailed, msg:
			print 'test', test, 'failed'
			failed = failed + 1
	if not failed:
		print 'All tests OK.'
	else:
		print failed, 'tests failed'
			
main()
