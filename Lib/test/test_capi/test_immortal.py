import unittest
from test.support import import_helper

_testcapi = import_helper.import_module('_testcapi')


class TestCAPI(unittest.TestCase):
    def test_immortal_bool(self):
        _testcapi.test_immortal_bool()

    def test_immortal_none(self):
        _testcapi.test_immortal_none()

    def test_immortal_ellipsis(self):
        _testcapi.test_immortal_ellipsis()


if __name__ == "__main__":
    unittest.main()
