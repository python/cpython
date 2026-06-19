import time
import unittest

from contextlib import timeout

class TimeOutTest(unittest.TestCase):
    def test_py(self):
        with timeout(0.2):
            time.sleep(0.1)

        with self.assertRaises(TimeoutError):
            with timeout(0.2):
                while True:
                    pass

    def test_c_module_re(self):
        import re
        with self.assertRaises(TimeoutError):
            with timeout(0.2):
                # A catastrophic case for re match
                re.match(r"(a+)+b", "a" * 30 + "c")