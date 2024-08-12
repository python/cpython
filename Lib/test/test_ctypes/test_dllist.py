import os
import sys
import test.support
import unittest
from ctypes import CDLL
from ctypes.util import dllist


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

        if test.support.verbose:
            print("Loaded shared libraries:")
            if dlls is not None:
                for dll in dlls:
                    print("\t", dll)
            else:
                print("\tFunction returned None")

        if SYSTEM_LIBRARY is not None:
            self.assertIsNotNone(dlls)
            self.assertGreater(len(dlls), 0)
            self.assertTrue(any(SYSTEM_LIBRARY in dll for dll in dlls))
        else:
            # unsupported platform
            self.assertIsNone(dlls)

    def test_lists_updates(self):
        dlls = dllist()

        if dlls is not None:
            from test.support import import_helper
            _ctypes_test = import_helper.import_module("_ctypes_test")
            test_module = CDLL(_ctypes_test.__file__)


            if any("_ctypes_test" in dll for dll in dlls):
                # The test module is already loaded
                if test.support.verbose:
                    print("The test module is already loaded")
                return

            dlls2 = dllist()
            if test.support.verbose:
                print("Newly loaded shared libraries:")
                diff = set(dlls2) - set(dlls)
                for dll in diff:
                    print("\t", dll)

            self.assertIsNotNone(dlls2)
            self.assertGreater(len(dlls2), len(dlls))
            self.assertTrue(any("_ctypes_test" in dll for dll in dlls2))



if __name__ == "__main__":
    unittest.main()
