import unittest
from test.support import import_helper


_testinternalcapi = import_helper.import_module("_testinternalcapi")


class TokenizerTests(unittest.TestCase):
    def test_source(self):
        _testinternalcapi.test_tokenizer_source()

    def test_cursor(self):
        _testinternalcapi.test_tokenizer_cursor()


if __name__ == "__main__":
    unittest.main()
