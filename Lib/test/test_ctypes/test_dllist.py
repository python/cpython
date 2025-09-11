import os
import sys
import unittest
from ctypes import CDLL
import ctypes.util
from test.support import import_helper


WINDOWS = os.name == "nt"
APPLE = sys.platform in {"darwin", "ios", "tvos", "watchos"}

if WINDOWS:
    KNOWN_LIBRARIES = ["KERNEL32.DLL"]
elif APPLE:
    KNOWN_LIBRARIES = ["libSystem.B.dylib"]
else:
    # trickier than it seems, because libc may not be present
    # on musl systems, and sometimes goes by different names.
    # However, ctypes itself loads libffi
    KNOWN_LIBRARIES = ["libc.so", "libffi.so"]


@unittest.skipUnless(
    hasattr(ctypes.util, "dllist"),
    "ctypes.util.dllist is not available on this platform",
)
class ListSharedLibraries(unittest.TestCase):

    def test_lists_system(self):
        dlls = ctypes.util.dllist()

        self.assertGreater(len(dlls), 0, f"loaded={dlls}")
        self.assertTrue(
            any(lib in dll for dll in dlls for lib in KNOWN_LIBRARIES), f"loaded={dlls}"
        )

    def test_lists_updates(self):
        dlls = ctypes.util.dllist()

        # this test relies on being able to import a library which is
        # not already loaded.
        # If it is (e.g. by a previous test in the same process), we skip
        if any("_ctypes_test" in dll for dll in dlls):
            self.skipTest("Test library is already loaded")

        _ctypes_test = import_helper.import_module("_ctypes_test")
        test_module = CDLL(_ctypes_test.__file__)
        dlls2 = ctypes.util.dllist()
        self.assertIsNotNone(dlls2)

        dlls1 = set(dlls)
        dlls2 = set(dlls2)

        self.assertGreater(dlls2, dlls1, f"newly loaded libraries: {dlls2 - dlls1}")
        self.assertTrue(any("_ctypes_test" in dll for dll in dlls2), f"loaded={dlls2}")


if __name__ == "__main__":
    unittest.main()
