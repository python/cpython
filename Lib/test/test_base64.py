import unittest
from test import test_support
import base64

class Base64TestCase(unittest.TestCase):

    def test_encodestring(self):
        self.assertEqual(base64.encodestring("www.python.org"), "d3d3LnB5dGhvbi5vcmc=\n")
        self.assertEqual(base64.encodestring("a"), "YQ==\n")
        self.assertEqual(base64.encodestring("ab"), "YWI=\n")
        self.assertEqual(base64.encodestring("abc"), "YWJj\n")
        self.assertEqual(base64.encodestring(""), "")
        self.assertEqual(base64.encodestring("abcdefghijklmnopqrstuvwxyz"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "0123456789!@#0^&*();:<>,. []{}"),
              "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
              "RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
              "Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n")

    def test_decodestring(self):
        self.assertEqual(base64.decodestring("d3d3LnB5dGhvbi5vcmc=\n"), "www.python.org")
        self.assertEqual(base64.decodestring("YQ==\n"), "a")
        self.assertEqual(base64.decodestring("YWI=\n"), "ab")
        self.assertEqual(base64.decodestring("YWJj\n"), "abc")
        self.assertEqual(base64.decodestring("YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
                           "RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
                           "Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n"),
              "abcdefghijklmnopqrstuvwxyz"
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "0123456789!@#0^&*();:<>,. []{}")
        self.assertEqual(base64.decodestring(''), '')

def test_main():
    test_support.run_unittest(Base64TestCase)

if __name__ == "__main__":
    test_main()
