# Test to see if openpty works. (But don't worry if it isn't available.)

import os, unittest

if not hasattr(os, "openpty"):
    raise unittest.SkipTest("os.openpty() not available.")


class OpenptyTest(unittest.TestCase):
    def test(self):
        master, slave = os.openpty()
        self.addCleanup(os.close, master)
        self.addCleanup(os.close, slave)
        if not os.isatty(slave):
            self.fail("Slave-end of pty is not a terminal.")

        os.write(slave, b'Ping!')
        self.assertEqual(os.read(master, 1024), b'Ping!')

if __name__ == '__main__':
    unittest.main()
