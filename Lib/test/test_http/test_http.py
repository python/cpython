import unittest
import re

class TestHTTP(unittest.TestCase):

    def test_http_parse_request(self):
        self.assertEqual(re.sub(r'^/+', '/', '//test.com'), '/test.com', '//test.com should be converted to a proper relative path')
        self.assertEqual(re.sub(r'^/+', '/', '///test.com'), '/test.com', '///test.com should be converted to a proper relative path')

if __name__ == '__main__':
    unittest.main()