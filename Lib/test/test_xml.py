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

    def test_is_valid_text(self):
        is_valid_text = xml.is_valid_text
        self.assertTrue(is_valid_text(''))
        self.assertTrue(is_valid_text('!0Aa_~ \r\n\t\x85\xa0'))
        self.assertTrue(is_valid_text('\ud7ff\ue000\ufffd\U00010000\U0010ffff'))
        self.assertFalse(is_valid_text('\x00'))
        self.assertFalse(is_valid_text('\x01'))
        self.assertFalse(is_valid_text('\x1f'))
        self.assertTrue(is_valid_text('\x7f'))
        self.assertTrue(is_valid_text('\x80'))
        self.assertTrue(is_valid_text('\x9f'))
        self.assertFalse(is_valid_text('\ud800'))
        self.assertFalse(is_valid_text('\udfff'))
        self.assertFalse(is_valid_text('\ufffe'))
        self.assertFalse(is_valid_text('\uffff'))


if __name__ == '__main__':
    unittest.main()
