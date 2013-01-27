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
        # Switch to an existing, but safe, working directory to let the
        # cleanup code do its thing without permission errors.
        with support.temp_cwd(path=path.dirname(sys.executable)):
            empty = path.join(path.dirname(__file__), "empty.vbs")
            startfile(empty)
            startfile(empty, "open")

def test_main():
    support.run_unittest(TestCase)

if __name__=="__main__":
    test_main()
