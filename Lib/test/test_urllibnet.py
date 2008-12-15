#!/usr/bin/env python

import unittest
from test import test_support

import socket
import urllib
import sys
import os
import mimetools


def _open_with_retry(func, host, *args, **kwargs):
    # Connecting to remote hosts is flaky.  Make it more robust
    # by retrying the connection several times.
    for i in range(3):
        try:
            return func(host, *args, **kwargs)
        except IOError, last_exc:
            continue
        except:
            raise
    raise last_exc


class URLTimeoutTest(unittest.TestCase):

    TIMEOUT = 10.0

    def setUp(self):
        socket.setdefaulttimeout(self.TIMEOUT)

    def tearDown(self):
        socket.setdefaulttimeout(None)

    def testURLread(self):
        f = _open_with_retry(urllib.urlopen, "http://www.python.org/")
        x = f.read()

class urlopenNetworkTests(unittest.TestCase):
    """Tests urllib.urlopen using the network.

    These tests are not exhaustive.  Assuming that testing using files does a
    good job overall of some of the basic interface features.  There are no
    tests exercising the optional 'data' and 'proxies' arguments.  No tests
    for transparent redirection have been written.

    setUp is not used for always constructing a connection to
    http://www.python.org/ since there a few tests that don't use that address
    and making a connection is expensive enough to warrant minimizing unneeded
    connections.

    """

    def urlopen(self, *args):
        return _open_with_retry(urllib.urlopen, *args)

    def test_basic(self):
        # Simple test expected to pass.
        open_url = self.urlopen("http://www.python.org/")
        for attr in ("read", "readline", "readlines", "fileno", "close",
                     "info", "geturl"):
            self.assert_(hasattr(open_url, attr), "object returned from "
                            "urlopen lacks the %s attribute" % attr)
        try:
            self.assert_(open_url.read(), "calling 'read' failed")
        finally:
            open_url.close()

    def test_readlines(self):
        # Test both readline and readlines.
        open_url = self.urlopen("http://www.python.org/")
        try:
            self.assert_(isinstance(open_url.readline(), basestring),
                         "readline did not return a string")
            self.assert_(isinstance(open_url.readlines(), list),
                         "readlines did not return a list")
        finally:
            open_url.close()

    def test_info(self):
        # Test 'info'.
        open_url = self.urlopen("http://www.python.org/")
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
        open_url = self.urlopen(URL)
        try:
            gotten_url = open_url.geturl()
        finally:
            open_url.close()
        self.assertEqual(gotten_url, URL)

    def test_getcode(self):
        # test getcode() with the fancy opener to get 404 error codes
        URL = "http://www.python.org/XXXinvalidXXX"
        open_url = urllib.FancyURLopener().open(URL)
        try:
            code = open_url.getcode()
        finally:
            open_url.close()
        self.assertEqual(code, 404)

    def test_fileno(self):
        if (sys.platform in ('win32',) or
                not hasattr(os, 'fdopen')):
            # On Windows, socket handles are not file descriptors; this
            # test can't pass on Windows.
            return
        # Make sure fd returned by fileno is valid.
        open_url = self.urlopen("http://www.python.org/")
        fd = open_url.fileno()
        FILE = os.fdopen(fd)
        try:
            self.assert_(FILE.read(), "reading from file created using fd "
                                      "returned by fileno failed")
        finally:
            FILE.close()

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
                          # urllib.urlopen, "http://www.sadflkjsasadf.com/")
                          urllib.urlopen, "http://sadflkjsasf.i.nvali.d/")

class urlretrieveNetworkTests(unittest.TestCase):
    """Tests urllib.urlretrieve using the network."""

    def urlretrieve(self, *args):
        return _open_with_retry(urllib.urlretrieve, *args)

    def test_basic(self):
        # Test basic functionality.
        file_location,info = self.urlretrieve("http://www.python.org/")
        self.assert_(os.path.exists(file_location), "file location returned by"
                        " urlretrieve is not a valid path")
        FILE = file(file_location)
        try:
            self.assert_(FILE.read(), "reading from the file location returned"
                         " by urlretrieve failed")
        finally:
            FILE.close()
            os.unlink(file_location)

    def test_specified_path(self):
        # Make sure that specifying the location of the file to write to works.
        file_location,info = self.urlretrieve("http://www.python.org/",
                                              test_support.TESTFN)
        self.assertEqual(file_location, test_support.TESTFN)
        self.assert_(os.path.exists(file_location))
        FILE = file(file_location)
        try:
            self.assert_(FILE.read(), "reading from temporary file failed")
        finally:
            FILE.close()
            os.unlink(file_location)

    def test_header(self):
        # Make sure header returned as 2nd value from urlretrieve is good.
        file_location, header = self.urlretrieve("http://www.python.org/")
        os.unlink(file_location)
        self.assert_(isinstance(header, mimetools.Message),
                     "header is not an instance of mimetools.Message")



def test_main():
    test_support.requires('network')
    from warnings import filterwarnings, catch_warnings
    with catch_warnings():
        filterwarnings('ignore', '.*urllib\.urlopen.*Python 3.0',
                        DeprecationWarning)
        test_support.run_unittest(URLTimeoutTest,
                                  urlopenNetworkTests,
                                  urlretrieveNetworkTests)

if __name__ == "__main__":
    test_main()
