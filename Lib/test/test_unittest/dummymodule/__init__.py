"""dummymodule
This module is simply used to test that unittest's discover method
uses a pattern correctly when discovering tests to run.
"""

import unittest


class Test_Dummy(unittest.TestCase):
    def test_dummy(self):
        pass
