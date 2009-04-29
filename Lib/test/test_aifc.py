from test.test_support import findfile, run_unittest
import unittest

import aifc


class AIFCTest(unittest.TestCase):

    def setUp(self):
        self.sndfilepath = findfile('Sine-1000Hz-300ms.aif')

    def test_skipunknown(self):
        #Issue 2245
        #This file contains chunk types aifc doesn't recognize.
        f = aifc.open(self.sndfilepath)
        f.close()


def test_main():
    run_unittest(AIFCTest)


if __name__ == "__main__":
    unittest.main()
