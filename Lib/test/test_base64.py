from unittest import TestCase
from test.test_support import vereq, run_unittest
from base64 import encodestring, decodestring

class Base64TestCase(TestCase):

    def test_encodestring(self):
        vereq(encodestring("www.python.org"), "d3d3LnB5dGhvbi5vcmc=\n")
        vereq(encodestring("a"), "YQ==\n")
        vereq(encodestring("ab"), "YWI=\n")
        vereq(encodestring("abc"), "YWJj\n")
        vereq(encodestring(""), "")
        vereq(encodestring("abcdefghijklmnopqrstuvwxyz"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "0123456789!@#0^&*();:<>,. []{}"),
              "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
              "RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
              "Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n")

    def test_decodestring(self):
        vereq(decodestring("d3d3LnB5dGhvbi5vcmc=\n"), "www.python.org")
        vereq(decodestring("YQ==\n"), "a")
        vereq(decodestring("YWI=\n"), "ab")
        vereq(decodestring("YWJj\n"), "abc")
        vereq(decodestring("YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
                           "RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
                           "Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n"),
              "abcdefghijklmnopqrstuvwxyz"
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "0123456789!@#0^&*();:<>,. []{}")
        vereq(decodestring(''), '')

def test_main():
    run_unittest(Base64TestCase)

if __name__ == "__main__":
    test_main()
