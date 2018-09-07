# Test to see if openpty works. (But don't worry if it isn't available.)

import os, unittest

if not hasattr(os, "openpty"):
    raise unittest.SkipTest("os.openpty() not available.")


class OpenptyTest(unittest.TestCase):
    def test(self):
        parent, child = os.openpty()
        self.addCleanup(os.close, parent)
        self.addCleanup(os.close, child)
        if not os.isatty(child):
            self.fail("Child-end of pty is not a terminal.")

        os.write(child, b'Ping!')
        self.assertEqual(os.read(parent, 1024), b'Ping!')

if __name__ == '__main__':
    unittest.main()
