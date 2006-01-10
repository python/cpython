#
# This file is for everybody to add tests for crashes that aren't
# fixed yet. Please add a test case and appropriate description.
#
# When you fix one of the crashes, please move the test to the correct
# test_ module.
#

import unittest
from test import test_support


# Bug 1377858
#
# mwh's description:
# The problem is obvious if you read typeobject.c around line 660: the weakref
# list is cleared before __del__ is called, so any weakrefs added during the 
# execution of __del__ are never informed of the object's death.

import weakref
ref = None

class TestBug1377858(unittest.TestCase):
    class Target(object):
        def __del__(self):
            global ref
            ref = weakref.ref(self)
    
    def testBug1377858(self):
        w = self.__class__.Target()
        w = None
        print ref()

def test_main():
    test_support.run_unittest(TestBug1377858)

if __name__ == "__main__":
    test_main()
