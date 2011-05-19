"""Tests for packaging.extension."""
import os

from packaging.compiler.extension import Extension
from packaging.tests import unittest

class ExtensionTestCase(unittest.TestCase):

    pass

def test_suite():
    return unittest.makeSuite(ExtensionTestCase)

if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
