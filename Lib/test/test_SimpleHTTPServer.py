"""
These tests only check url parsing for now.
We don't want to require the 'network' resource.
"""

import os, unittest
from http.server import SimpleHTTPRequestHandler
from test import support


class SocketlessRequestHandler(SimpleHTTPRequestHandler):
    def __init__(self):
        pass

class SimpleHTTPRequestHandlerTestCase(unittest.TestCase):
    """ Test url parsing """
    def setUp (self):
        self.translated = os.getcwd()
        self.translated = os.path.join(self.translated, 'filename')
        self.handler = SocketlessRequestHandler ()

    def test_queryArguments (self):
        path = self.handler.translate_path ('/filename')
        self.assertEqual (path, self.translated)
        path = self.handler.translate_path ('/filename?foo=bar')
        self.assertEqual (path, self.translated)
        path = self.handler.translate_path ('/filename?a=b&spam=eggs#zot')
        self.assertEqual (path, self.translated)

    def test_startWithDoubleSlash (self):
        path = self.handler.translate_path ('//filename')
        self.assertEqual (path, self.translated)
        path = self.handler.translate_path ('//filename?foo=bar')
        self.assertEqual (path, self.translated)


def test_main():
    support.run_unittest(SimpleHTTPRequestHandlerTestCase)

if __name__ == "__main__":
    test_main()
