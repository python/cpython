'''
   Tests for commands module
   Nick Mathewson
'''
import unittest
import os, tempfile, re

from test.test_support import TestSkipped, run_unittest, reap_children
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

        # we use mkdtemp in the next line to create an empty directory
        # under our exclusive control; from that, we can invent a pathname
        # that we _know_ won't exist.  This is guaranteed to fail.
        dir = None
        try:
            dir = tempfile.mkdtemp()
            name = os.path.join(dir, "foo")

            status, output = getstatusoutput('cat ' + name)
            self.assertNotEquals(status, 0)
        finally:
            if dir is not None:
                os.rmdir(dir)

    def test_getstatus(self):
        # This pattern should match 'ls -ld /.' on any posix
        # system, however perversely configured.  Even on systems
        # (e.g., Cygwin) where user and group names can have spaces:
        #     drwxr-xr-x   15 Administ Domain U     4096 Aug 12 12:50 /
        #     drwxr-xr-x   15 Joe User My Group     4096 Aug 12 12:50 /
        # Note that the first case above has a space in the group name
        # while the second one has a space in both names.
        pat = r'''d.........   # It is a directory.
                  \+?          # It may have ACLs.
                  \s+\d+       # It has some number of links.
                  [^/]*        # Skip user, group, size, and date.
                  /\.          # and end with the name of the file.
               '''

        self.assert_(re.match(pat, getstatus("/."), re.VERBOSE))


def test_main():
    run_unittest(CommandTests)
    reap_children()


if __name__ == "__main__":
    test_main()
