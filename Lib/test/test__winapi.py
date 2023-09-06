import _winapi
import msvcrt
import unittest

from test.support import os_helper
from test.support.os_helper import TESTFN


class LockFileTest(unittest.TestCase):
    def test_lockfile(self):
        with open(TESTFN, "w") as f:
            self.addCleanup(os_helper.unlink, TESTFN)

            h = msvcrt.get_osfhandle(f.fileno())

            _winapi.LockFile(h, 0, 0, 1, 0)
            self.assertRaises(PermissionError, _winapi.LockFile, h, 0, 0, 1, 0)

    def test_unlockfile(self):
        with open(TESTFN, "w") as f:
            self.addCleanup(os_helper.unlink, TESTFN)

            h = msvcrt.get_osfhandle(f.fileno())

            _winapi.LockFile(h, 0, 0, 1, 0)
            _winapi.UnlockFile(h, 0, 0, 1, 0)
            _winapi.LockFile(h, 0, 0, 1, 0)




if __name__ == "__main__":
    unittest.main()