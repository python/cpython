# Automatic Python regression test.
#
# Some essential parts of the Python interpreter are tested by the module
# 'testall'.  (Despite its name, it doesn't test everything -- that would
# be a truly Herculean task!)  When a test fails, 'testall' raises an
# exception.  When all tests succeed, it produces quite a lot of output.
#
# For a normal regression test, this output is never looked at unless
# something goes wrong.  Thus, it would be wise to suppress the output
# normally.  This module does that, but it doesn't just throw the output
# from 'testall' away -- it compares it with the output from a previous
# run.  If a difference is noticed it raises an exception; if all is well,
# it prints nothing except 'All tests OK.' at the very end.
#
# The output from a previous run is supposed to be in a file 'testall.out'
# somewhere on the search path for modules (sys.path, initialized from
# $PYTHONPATH plus some default places).
#
# Of course, if the normal output of the tests is changed because the
# tests have been changed (rather than a test producing the wrong output),
# 'autotest' will fail as well.  In this case, run 'testall' manually
# and direct its output to 'testall.out'.
#
# The comparison uses (and demonstrates!) a rather new Python feature:
# program output that normally goes to stdout (by 'print' statements
# or by writing directly to sys.stdout) can be redirected to an
# arbitrary class instance as long as it has a 'write' method.

import os
import sys

# Function to find a file somewhere on sys.path
def findfile(filename):
	for dirname in sys.path:
		fullname = os.path.join(dirname, filename)
		if os.path.exists(fullname):
			return fullname
	return filename # Will cause exception later

# Exception raised when the test failed (not the same as in test_support)
TestFailed = 'autotest.TestFailed'

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
		self.fp.close()

# The main program
def main():
	import sys
	filename = findfile('testall.out')
	real_stdout = sys.stdout
	try:
		sys.stdout = Compare(filename)
		import testall
	finally:
		sys.stdout = real_stdout
	print 'All tests OK.'

main()
