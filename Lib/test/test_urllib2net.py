#!/usr/bin/env python

import unittest
from test import test_support

import socket
import urllib2
import sys
import os
import mimetools

class URLTimeoutTest(unittest.TestCase):

    TIMEOUT = 10.0

    def setUp(self):
        socket.setdefaulttimeout(self.TIMEOUT)

    def tearDown(self):
        socket.setdefaulttimeout(None)

    def testURLread(self):
        f = urllib2.urlopen("http://www.python.org/")
        x = f.read()

class urlopenNetworkTests(unittest.TestCase):
    """Tests urllib2.urlopen using the network.

    These tests are not exhaustive.  Assuming that testing using files does a
    good job overall of some of the basic interface features.  There are no
    tests exercising the optional 'data' and 'proxies' arguments.  No tests
    for transparent redirection have been written.

    setUp is not used for always constructing a connection to
    http://www.python.org/ since there a few tests that don't use that address
    and making a connection is expensive enough to warrant minimizing unneeded
    connections.

    """

    def test_basic(self):
        # Simple test expected to pass.
        open_url = urllib2.urlopen("http://www.python.org/")
        for attr in ("read", "close", "info", "geturl"):
            self.assert_(hasattr(open_url, attr), "object returned from "
                            "urlopen lacks the %s attribute" % attr)
        try:
            self.assert_(open_url.read(), "calling 'read' failed")
        finally:
            open_url.close()

    def test_info(self):
        # Test 'info'.
        open_url = urllib2.urlopen("http://www.python.org/")
        try:
            info_obj = open_url.info()
        finally:
            open_url.close()
            self.assert_(isinstance(info_obj, mimetools.Message),
                         "object returned by 'info' is not an instance of "
                         "mimetools.Message")
            self.assertEqual(info_obj.getsubtype(), "html")

    def test_geturl(self):
        # Make sure same URL as opened is returned by geturl.
        URL = "http://www.python.org/"
        open_url = urllib2.urlopen(URL)
        try:
            gotten_url = open_url.geturl()
        finally:
            open_url.close()
        self.assertEqual(gotten_url, URL)

    def test_bad_address(self):
        # Make sure proper exception is raised when connecting to a bogus
        # address.
        self.assertRaises(IOError,
                          # SF patch 809915:  In Sep 2003, VeriSign started
                          # highjacking invalid .com and .net addresses to
                          # boost traffic to their own site.  This test
                          # started failing then.  One hopes the .invalid
                          # domain will be spared to serve its defined
                          # purpose.
                          # urllib2.urlopen, "http://www.sadflkjsasadf.com/")
                          urllib2.urlopen, "http://www.python.invalid/")

def test_main():
    test_support.requires("network")
    test_support.run_unittest(URLTimeoutTest, urlopenNetworkTests)

if __name__ == "__main__":
    test_main()
