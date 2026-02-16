"""Unit tests for code in urllib.response."""

import socket
import tempfile
import urllib.response
import unittest
from test import support

if support.is_wasi:
    raise unittest.SkipTest("Cannot create socket on WASI")


class TestResponse(unittest.TestCase):

    def setUp(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.fp = self.sock.makefile('rb')
        self.test_headers = {"Host": "www.python.org",
                             "Connection": "close"}

    def test_with(self):
        addbase = urllib.response.addbase(self.fp)

        self.assertIsInstance(addbase, tempfile._TemporaryFileWrapper)

        def f():
            with addbase as spam:
                pass
        self.assertFalse(self.fp.closed)
        f()
        self.assertTrue(self.fp.closed)
        self.assertRaises(ValueError, f)

    def test_addclosehook(self):
        closehook_called = False

        def closehook():
            nonlocal closehook_called
            closehook_called = True

        closehook = urllib.response.addclosehook(self.fp, closehook)
        closehook.close()

        self.assertTrue(self.fp.closed)
        self.assertTrue(closehook_called)

    def test_addinfo(self):
        info = urllib.response.addinfo(self.fp, self.test_headers)
        self.assertEqual(info.info(), self.test_headers)
        self.assertEqual(info.headers, self.test_headers)
        info.close()

    def test_addinfourl(self):
        url = "http://www.python.org"
        code = 200
        infourl = urllib.response.addinfourl(self.fp, self.test_headers,
                                             url, code)
        self.assertEqual(infourl.info(), self.test_headers)
        self.assertEqual(infourl.geturl(), url)
        self.assertEqual(infourl.getcode(), code)
        self.assertEqual(infourl.headers, self.test_headers)
        self.assertEqual(infourl.url, url)
        self.assertEqual(infourl.status, code)
        infourl.close()

    def tearDown(self):
        self.sock.close()

if __name__ == '__main__':
    unittest.main()
