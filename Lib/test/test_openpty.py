# Test to see if openpty works. (But don't worry if it isn't available.)

import os, unittest
from test.support import run_unittest

if not hasattr(os, "openpty"):
    raise unittest.SkipTest("No openpty() available.")


class OpenptyTest(unittest.TestCase):
    def test(self):
        master, slave = os.openpty()
        if not os.isatty(slave):
            self.fail("Slave-end of pty is not a terminal.")

        os.write(slave, b'Ping!')
        self.assertEqual(os.read(master, 1024), b'Ping!')

def test_main():
    run_unittest(OpenptyTest)

if __name__ == '__main__':
    test_main()
