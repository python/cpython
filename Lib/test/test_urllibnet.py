import unittest
from test import test_support

import socket
import urllib
import sys
import os
import time

try:
    import ssl
except ImportError:
    ssl = None

here = os.path.dirname(__file__)
# Self-signed cert file for self-signed.pythontest.net
CERT_selfsigned_pythontestdotnet = os.path.join(here, 'selfsigned_pythontestdotnet.pem')

mimetools = test_support.import_module("mimetools", deprecated=True)


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
        f = _open_with_retry(urllib.urlopen, "http://www.example.com/")
        x = f.read()

class urlopenNetworkTests(unittest.TestCase):
    """Tests urllib.urlopen using the network.

    These tests are not exhaustive.  Assuming that testing using files does a
    good job overall of some of the basic interface features.  There are no
    tests exercising the optional 'data' and 'proxies' arguments.  No tests
    for transparent redirection have been written.

    setUp is not used for always constructing a connection to
    http://www.example.com/ since there a few tests that don't use that address
    and making a connection is expensive enough to warrant minimizing unneeded
    connections.

    """

    def urlopen(self, *args):
        return _open_with_retry(urllib.urlopen, *args)

    def test_basic(self):
        # Simple test expected to pass.
        open_url = self.urlopen("http://www.example.com/")
        for attr in ("read", "readline", "readlines", "fileno", "close",
                     "info", "geturl"):
            self.assertTrue(hasattr(open_url, attr), "object returned from "
                            "urlopen lacks the %s attribute" % attr)
        try:
            self.assertTrue(open_url.read(), "calling 'read' failed")
        finally:
            open_url.close()

    def test_readlines(self):
        # Test both readline and readlines.
        open_url = self.urlopen("http://www.example.com/")
        try:
            self.assertIsInstance(open_url.readline(), basestring,
                                  "readline did not return a string")
            self.assertIsInstance(open_url.readlines(), list,
                                  "readlines did not return a list")
        finally:
            open_url.close()

    def test_info(self):
        # Test 'info'.
        open_url = self.urlopen("http://www.example.com/")
        try:
            info_obj = open_url.info()
        finally:
            open_url.close()
            self.assertIsInstance(info_obj, mimetools.Message,
                                  "object returned by 'info' is not an "
                                  "instance of mimetools.Message")
            self.assertEqual(info_obj.getsubtype(), "html")

    def test_geturl(self):
        # Make sure same URL as opened is returned by geturl.
        URL = "http://www.example.com/"
        open_url = self.urlopen(URL)
        try:
            gotten_url = open_url.geturl()
        finally:
            open_url.close()
        self.assertEqual(gotten_url, URL)

    def test_getcode(self):
        # test getcode() with the fancy opener to get 404 error codes
        URL = "http://www.pythontest.net/XXXinvalidXXX"
        open_url = urllib.FancyURLopener().open(URL)
        try:
            code = open_url.getcode()
        finally:
            open_url.close()
        self.assertEqual(code, 404)

    @unittest.skipIf(sys.platform in ('win32',), 'not appropriate for Windows')
    @unittest.skipUnless(hasattr(os, 'fdopen'), 'os.fdopen not available')
    def test_fileno(self):
        # Make sure fd returned by fileno is valid.
        open_url = self.urlopen("http://www.example.com/")
        fd = open_url.fileno()
        FILE = os.fdopen(fd)
        try:
            self.assertTrue(FILE.read(), "reading from file created using fd "
                                      "returned by fileno failed")
        finally:
            FILE.close()

    def test_bad_address(self):
        # Make sure proper exception is raised when connecting to a bogus
        # address.
        bogus_domain = "sadflkjsasf.i.nvali.d"
        try:
            socket.gethostbyname(bogus_domain)
        except socket.gaierror:
            pass
        else:
            # This happens with some overzealous DNS providers such as OpenDNS
            self.skipTest("%r should not resolve for test to work" % bogus_domain)
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
        file_location,info = self.urlretrieve("http://www.example.com/")
        self.assertTrue(os.path.exists(file_location), "file location returned by"
                        " urlretrieve is not a valid path")
        FILE = file(file_location)
        try:
            self.assertTrue(FILE.read(), "reading from the file location returned"
                         " by urlretrieve failed")
        finally:
            FILE.close()
            os.unlink(file_location)

    def test_specified_path(self):
        # Make sure that specifying the location of the file to write to works.
        file_location,info = self.urlretrieve("http://www.example.com/",
                                              test_support.TESTFN)
        self.assertEqual(file_location, test_support.TESTFN)
        self.assertTrue(os.path.exists(file_location))
        FILE = file(file_location)
        try:
            self.assertTrue(FILE.read(), "reading from temporary file failed")
        finally:
            FILE.close()
            os.unlink(file_location)

    def test_header(self):
        # Make sure header returned as 2nd value from urlretrieve is good.
        file_location, header = self.urlretrieve("http://www.example.com/")
        os.unlink(file_location)
        self.assertIsInstance(header, mimetools.Message,
                              "header is not an instance of mimetools.Message")

    def test_data_header(self):
        logo = "http://www.example.com/"
        file_location, fileheaders = self.urlretrieve(logo)
        os.unlink(file_location)
        datevalue = fileheaders.getheader('Date')
        dateformat = '%a, %d %b %Y %H:%M:%S GMT'
        try:
            time.strptime(datevalue, dateformat)
        except ValueError:
            self.fail('Date value not in %r format', dateformat)


@unittest.skipIf(ssl is None, "requires ssl")
class urlopen_HttpsTests(unittest.TestCase):

    def test_context_argument(self):
        context = ssl.create_default_context(cafile=CERT_selfsigned_pythontestdotnet)
        response = urllib.urlopen("https://self-signed.pythontest.net", context=context)
        self.assertIn("Python", response.read())


def test_main():
    test_support.requires('network')
    with test_support.check_py3k_warnings(
            ("urllib.urlopen.. has been removed", DeprecationWarning)):
        test_support.run_unittest(URLTimeoutTest,
                                  urlopenNetworkTests,
                                  urlretrieveNetworkTests,
                                  urlopen_HttpsTests)

if __name__ == "__main__":
    test_main()
