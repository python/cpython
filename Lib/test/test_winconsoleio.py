'''Tests for WindowsConsoleIO

Unfortunately, most testing requires interactive use, since we have no
API to read back from a real console, and this class is only for use
with real consoles.

Instead, we validate that basic functionality such as opening, closing
and in particular fileno() work, but are forced to leave real testing
to real people with real keyborads.
'''

import io
import unittest
import sys

if sys.platform != 'win32':
    raise unittest.SkipTest("test only relevant on win32")

ConIO = io._WindowsConsoleIO

class WindowsConsoleIOTests(unittest.TestCase):
    def test_abc(self):
        self.assertTrue(issubclass(ConIO, io.RawIOBase))
        self.assertFalse(issubclass(ConIO, io.BufferedIOBase))
        self.assertFalse(issubclass(ConIO, io.TextIOBase))

    def test_open_fd(self):
        try:
            f = ConIO(0)
        except ValueError:
            # cannot open console because it's not a real console
            pass
        else:
            self.assertTrue(f.readable())
            self.assertFalse(f.writable())
            self.assertEqual(0, f.fileno())
            f.close()   # multiple close should not crash
            f.close()

        try:
            f = ConIO(1, 'w')
        except ValueError:
            # cannot open console because it's not a real console
            pass
        else:
            self.assertFalse(f.readable())
            self.assertTrue(f.writable())
            self.assertEqual(1, f.fileno())
            f.close()
            f.close()

        try:
            f = ConIO(2, 'w')
        except ValueError:
            # cannot open console because it's not a real console
            pass
        else:
            self.assertFalse(f.readable())
            self.assertTrue(f.writable())
            self.assertEqual(2, f.fileno())
            f.close()
            f.close()

    def test_open_name(self):
        f = ConIO("CON")
        self.assertTrue(f.readable())
        self.assertFalse(f.writable())
        self.assertIsNotNone(f.fileno())
        f.close()   # multiple close should not crash
        f.close()

        f = ConIO('CONIN$')
        self.assertTrue(f.readable())
        self.assertFalse(f.writable())
        self.assertIsNotNone(f.fileno())
        f.close()
        f.close()

        f = ConIO('CONOUT$', 'w')
        self.assertFalse(f.readable())
        self.assertTrue(f.writable())
        self.assertIsNotNone(f.fileno())
        f.close()
        f.close()

if __name__ == "__main__":
    unittest.main()
