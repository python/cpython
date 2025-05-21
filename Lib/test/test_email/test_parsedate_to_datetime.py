# Test to see if parsedate_to_datetime returns the correct year for different digit numbers, adhering to the RFC2822 spec

import unittest
from email.utils import parsedate_to_datetime

class ParsedateToDatetimeTest(unittest.TestCase):
    def test(self):
        expectations = {
            "Sat, 15 Aug 0001 23:12:09 +0500": "0001",
            "Thu, 1 Sep 1 23:12:09 +0800": "0001",
            "Thu, 7 Oct 123 23:12:09 +0500": "0123",
        }
        for input_string, output_string in expectations.items():
            self.assertEqual(str(parsedate_to_datetime(input_string))[:4], output_string)

if __name__ == '__main__':
    unittest.main()
