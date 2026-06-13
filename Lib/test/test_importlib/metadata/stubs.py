import unittest


class fake_filesystem_unittest:
    """
    Stubbed version of the pyfakefs module
    """
    class TestCase(unittest.TestCase):
        def setUpPyfakefs(self):
            self.skipTest("pyfakefs not available")
