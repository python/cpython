import unittest
import test_support
import base64
from binascii import Error as binascii_error

class Base64TestCase(unittest.TestCase):
    def test_encode_string(self):
        """Testing encode string"""
        test_support.verify(base64.encodestring("www.python.org") ==
            "d3d3LnB5dGhvbi5vcmc=\n",
            reason="www.python.org encodestring failed")
        test_support.verify(base64.encodestring("a") ==
            "YQ==\n",
            reason="a encodestring failed")
        test_support.verify(base64.encodestring("ab") ==
            "YWI=\n",
            reason="ab encodestring failed")
        test_support.verify(base64.encodestring("abc") ==
            "YWJj\n",
            reason="abc encodestring failed")
        test_support.verify(base64.encodestring("") ==
            "",
            reason="null encodestring failed")
        test_support.verify(base64.encodestring(
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#0^&*();:<>,. []{}") ==
            "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNTY3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n",
            reason = "long encodestring failed")

    def test_decode_string(self):
        """Testing decode string"""
        test_support.verify(base64.decodestring("d3d3LnB5dGhvbi5vcmc=\n") ==
            "www.python.org",
            reason="www.python.org decodestring failed")
        test_support.verify(base64.decodestring("YQ==\n") ==
            "a",
            reason="a decodestring failed")
        test_support.verify(base64.decodestring("YWI=\n") ==
            "ab",
            reason="ab decodestring failed")
        test_support.verify(base64.decodestring("YWJj\n") ==
            "abc",
            reason="abc decodestring failed")
        test_support.verify(base64.decodestring(
            "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNTY3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n") ==
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#0^&*();:<>,. []{}",
            reason = "long decodestring failed")
        test_support.verify(base64.decodestring('') == '')

def test_main():
    test_support.run_unittest(Base64TestCase)

if __name__ == "__main__":
    test_main()
