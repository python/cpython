'''
   Tests for commands module
   Nick Mathewson
'''
import unittest
import os, tempfile, re

from test_support import TestSkipped, run_unittest
from commands import *

# The module says:
#   "NB This only works (and is only relevant) for UNIX."
#
# Actually, getoutput should work on any platform with an os.popen, but
# I'll take the comment as given, and skip this suite.

if os.name != 'posix':
    raise TestSkipped('Not posix; skipping test_commands')


class CommandTests(unittest.TestCase):

    def test_getoutput(self):
        self.assertEquals(getoutput('echo xyzzy'), 'xyzzy')
        self.assertEquals(getstatusoutput('echo xyzzy'), (0, 'xyzzy'))

        # we use mktemp in the next line to get a filename which we
        # _know_ won't exist.  This is guaranteed to fail.
        status, output = getstatusoutput('cat ' + tempfile.mktemp())
        self.assertNotEquals(status, 0)

    def test_getstatus(self):
        # This pattern should match 'ls -ld /.' on any posix
        # system, however perversely configured.
        pat = r'''d.........   # It is a directory.
                  \s+\d+       # It has some number of links.
                  \s+\w+\s+\w+ # It has a user and group, which may
                               #     be named anything.
                  \s+\d+       # It has a size.
                  [^/]*        # Skip the date.
                  /.           # and end with the name of the file.
               '''

        self.assert_(re.match(pat, getstatus("/."), re.VERBOSE))


def test_main():
    run_unittest(CommandTests)


if __name__ == "__main__":
    test_main()
