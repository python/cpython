#!/usr/bin/env python

import unittest
from test import test_support

import socket
import urllib2
import sys

class URLTimeoutTest(unittest.TestCase):

    TIMEOUT = 10.0

    def setUp(self):
        socket.setdefaulttimeout(self.TIMEOUT)

    def tearDown(self):
        socket.setdefaulttimeout(None)

    def testURLread(self):
        f = urllib2.urlopen("http://www.python.org/")
        x = f.read()

def test_main():
    test_support.requires('network')

    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(URLTimeoutTest))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
