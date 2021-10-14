"Test constants and functions found in idlelib.util"

import unittest
from idlelib import util


class ExtensionTest(unittest.TestCase):
    def test_extensions(self):
        self.assertIn('.pyi', util.PYTHON_EXTENSIONS)
        self.assertIn('.py', util.PYTHON_EXTENSIONS)
        self.assertIn('.pyw', util.PYTHON_EXTENSIONS)


if __name__ == '__main__':
    unittest.main(verbosity=2)
