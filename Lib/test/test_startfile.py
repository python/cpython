# Ridiculously simple test of the os.startfile function for Windows.
#
# empty.vbs is an empty file (except for a comment), which does
# nothing when run with cscript or wscript.
#
# A possible improvement would be to have empty.vbs do something that
# we can detect here, to make sure that not only the os.startfile()
# call succeeded, but also the script actually has run.

import unittest
from test import support
import os
import sys
from os import path

startfile = support.get_attribute(os, 'startfile')


class TestCase(unittest.TestCase):
    def test_nonexisting(self):
        self.assertRaises(OSError, startfile, "nonexisting.vbs")

    def test_empty(self):
        # We need to make sure the child process starts in a directory
        # we're not about to delete. If we're running under -j, that
        # means the test harness provided directory isn't a safe option.
        # See http://bugs.python.org/issue15526 for more details
        with support.change_cwd(path.dirname(sys.executable)):
            empty = path.join(path.dirname(__file__), "empty.vbs")
            startfile(empty)
            startfile(empty, "open")

def test_main():
    support.run_unittest(TestCase)

if __name__=="__main__":
    test_main()
