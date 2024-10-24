import os
import sys
import test.support
import unittest
from ctypes import CDLL
from ctypes.util import dllist
from test.support import import_helper


WINDOWS = os.name == 'nt'
APPLE = sys.platform in {"darwin", "ios", "tvos", "watchos"}
AIX = sys.platform.startswith("aix")

if WINDOWS:
    SYSTEM_LIBRARY = 'KERNEL32.DLL'
elif APPLE:
    SYSTEM_LIBRARY = 'libSystem.B.dylib'
elif AIX:
    SYSTEM_LIBRARY = None
else:
    SYSTEM_LIBRARY = 'libc.so'

class ListSharedLibraries(unittest.TestCase):

    def test_lists_system(self):
        dlls = dllist()

        if SYSTEM_LIBRARY is not None:
            self.assertIsNotNone(dlls)
            self.assertGreater(len(dlls), 0, f'loaded={dlls}')
            self.assertTrue(any(SYSTEM_LIBRARY in dll for dll in dlls), f'loaded={dlls}')
        else:
            # unsupported platform
            self.assertIsNone(dlls)

    def test_lists_updates(self):
        dlls = dllist()

        if dlls is not None:
            if any("_ctypes_test" in dll for dll in dlls):
                self.skipTest("Test library is already loaded")

            _ctypes_test = import_helper.import_module("_ctypes_test")
            test_module = CDLL(_ctypes_test.__file__)
            dlls2 = dllist()
            self.assertIsNotNone(dlls2)

            dlls1 = set(dlls)
            dlls2 = set(dlls2)
            if test.support.verbose:
                print("Newly loaded shared libraries:")
                for dll in (dlls2 - dlls1):
                    print("\t", dll)

            self.assertGreater(dlls2, dlls1)
            self.assertTrue(any("_ctypes_test" in dll for dll in dlls2))



if __name__ == "__main__":
    unittest.main()
