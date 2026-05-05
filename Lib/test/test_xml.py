import xml
import unittest


class TestUtils(unittest.TestCase):

    def test_is_valid_name(self):
        is_valid_name = xml.is_valid_name
        self.assertFalse(is_valid_name(''))
        self.assertTrue(is_valid_name('name'))
        self.assertTrue(is_valid_name('NAME'))
        self.assertTrue(is_valid_name('name0:-._·'))
        self.assertTrue(is_valid_name('_'))
        self.assertTrue(is_valid_name(':'))
        self.assertTrue(is_valid_name('Ñàḿĕ'))
        self.assertTrue(is_valid_name('\U000EFFFF'))
        self.assertFalse(is_valid_name('0'))
        self.assertFalse(is_valid_name('-'))
        self.assertFalse(is_valid_name('.'))
        self.assertFalse(is_valid_name('·'))
        self.assertFalse(is_valid_name('na me'))
        for c in '<>/!?=\x00\x01\x7f\ud800\udfff\ufffe\uffff\U000F0000':
            self.assertFalse(is_valid_name('name' + c))


if __name__ == '__main__':
    unittest.main()
