import sys
import unittest

class FooTest(unittest.TestCase):
    def test_foo(self):
        # use 2.6 syntax to demonstrate conversion
        print 'In test_foo, using Python %s...' % (sys.version_info,)
        self.assertTrue(False)
