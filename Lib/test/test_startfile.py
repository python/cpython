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
from test.support import os_helper
import os
import platform
import sys
from os import path

startfile = support.get_attribute(os, 'startfile')


@unittest.skipIf(platform.win32_is_iot(), "starting files is not supported on Windows IoT Core or nanoserver")
class TestCase(unittest.TestCase):
    def test_nonexisting(self):
        self.assertRaises(OSError, startfile, "nonexisting.vbs")

    def test_empty(self):
        # We need to make sure the child process starts in a directory
        # we're not about to delete. If we're running under -j, that
        # means the test harness provided directory isn't a safe option.
        # See http://bugs.python.org/issue15526 for more details
        with os_helper.change_cwd(path.dirname(sys.executable)):
            empty = path.join(path.dirname(__file__), "empty.vbs")
            startfile(empty)
            startfile(empty, "open")
        startfile(empty, cwd=path.dirname(sys.executable))

    def test_python(self):
        # Passing "-V" ensures that it closes quickly, though still not
        # quickly enough that we can run in the test directory
        cwd, name = path.split(sys.executable)
        startfile(name, arguments="-V", cwd=cwd)
        startfile(name, arguments="-V", cwd=cwd, show_cmd=0)

if __name__ == "__main__":
    unittest.main()
