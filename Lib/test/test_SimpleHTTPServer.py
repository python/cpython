"""
These tests only check url parsing for now.
We don't want to require the 'network' resource.
"""

import os, unittest
from SimpleHTTPServer import SimpleHTTPRequestHandler
from test import test_support


class SocketlessRequestHandler (SimpleHTTPRequestHandler):
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
        self.assertEquals (path, self.translated)
        path = self.handler.translate_path ('/filename?foo=bar')
        self.assertEquals (path, self.translated)
        path = self.handler.translate_path ('/filename?a=b&spam=eggs#zot')
        self.assertEquals (path, self.translated)

    def test_startWithDoubleSlash (self):
        path = self.handler.translate_path ('//filename')
        self.assertEquals (path, self.translated)
        path = self.handler.translate_path ('//filename?foo=bar')
        self.assertEquals (path, self.translated)


def test_main():
    test_support.run_unittest(SimpleHTTPRequestHandlerTestCase)

if __name__ == "__main__":
    test_main()
