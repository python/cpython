import unittest

from test.support import requires_subprocess
from test.support.script_helper import assert_python_ok


@requires_subprocess()
class CommandLineTest(unittest.TestCase):
    def test_this_module(self):
        rc, stdout, stderr = assert_python_ok('-m', 'this')
        self.assertEqual(rc, 0)
        self.assertIn(b'The Zen of Python', stdout)


if __name__ == "__main__":
    unittest.main()
