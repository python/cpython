import unittest

from email.parser import BytesParser, Parser
from email.policy import default
from email._header_value_parser import _get_ptext_to_endchars

class ParserTests(unittest.TestCase):
    def test_check_payload_issue37461(self):
        # expected to not hang
        payload='Content-Type:x;\x1b*="\'G\'\\"\n'
        Parser(policy=default).parsestr(payload)

    def test_get_ptext_to_endchars_notquotes(self):
        ptext,value,escaped = _get_ptext_to_endchars("foo bar", '"')
        self.assertFalse(escaped)
        self.assertEqual(ptext, "foo")
        self.assertEqual(value, " bar")

    def test_get_ptext_to_endchars_quotes(self):
        ptext,value,escaped = _get_ptext_to_endchars("\\foo", '"')
        self.assertFalse(escaped)
        self.assertEqual(ptext, "foo")
        self.assertEqual(value, "")

    def test_get_ptext_to_endchars_value_like_endchars(self):
        ptext,value,escaped = _get_ptext_to_endchars('"', '"')
        self.assertFalse(escaped)
        self.assertEqual(ptext, '"')
        self.assertEqual(value, "")

if __name__ == '__main__':
    unittest.main()