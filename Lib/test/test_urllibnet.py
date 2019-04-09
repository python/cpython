import unittest
from test import support

import contextlib
import socket
import urllib.parse
import urllib.request
import os
import email.message
import time


support.requires('network')


class URLTimeoutTest(unittest.TestCase):
    # XXX this test doesn't seem to test anything useful.

    TIMEOUT = 30.0

    def setUp(self):
        socket.setdefaulttimeout(self.TIMEOUT)

    def tearDown(self):
        socket.setdefaulttimeout(None)

    def testURLread(self):
        domain = urllib.parse.urlparse(support.TEST_HTTP_URL).netloc
        with support.transient_internet(domain):
            f = urllib.request.urlopen(support.TEST_HTTP_URL)
            f.read()


class urlopenNetworkTests(unittest.TestCase):
    """Tests urllib.request.urlopen using the network.

    These tests are not exhaustive.  Assuming that testing using files does a
    good job overall of some of the basic interface features.  There are no
    tests exercising the optional 'data' and 'proxies' arguments.  No tests
    for transparent redirection have been written.

    setUp is not used for always constructing a connection to
    http://www.pythontest.net/ since there a few tests that don't use that address
    and making a connection is expensive enough to warrant minimizing unneeded
    connections.

    """

    url = 'http://www.pythontest.net/'

    @contextlib.contextmanager
    def urlopen(self, *args, **kwargs):
        resource = args[0]
        with support.transient_internet(resource):
            r = urllib.request.urlopen(*args, **kwargs)
            try:
                yield r
            finally:
                r.close()

    def test_basic(self):
        # Simple test expected to pass.
        with self.urlopen(self.url) as open_url:
            for attr in ("read", "readline", "readlines", "fileno", "close",
                         "info", "geturl"):
                self.assertTrue(hasattr(open_url, attr), "object returned from "
                                "urlopen lacks the %s attribute" % attr)
            self.assertTrue(open_url.read(), "calling 'read' failed")

    def test_readlines(self):
        # Test both readline and readlines.
        with self.urlopen(self.url) as open_url:
            self.assertIsInstance(open_url.readline(), bytes,
                                  "readline did not return a string")
            self.assertIsInstance(open_url.readlines(), list,
                                  "readlines did not return a list")

    def test_info(self):
        # Test 'info'.
        with self.urlopen(self.url) as open_url:
            info_obj = open_url.info()
            self.assertIsInstance(info_obj, email.message.Message,
                                  "object returned by 'info' is not an "
                                  "instance of email.message.Message")
            self.assertEqual(info_obj.get_content_subtype(), "html")

    def test_geturl(self):
        # Make sure same URL as opened is returned by geturl.
        with self.urlopen(self.url) as open_url:
            gotten_url = open_url.geturl()
            self.assertEqual(gotten_url, self.url)

    def test_getcode(self):
        # test getcode() with the fancy opener to get 404 error codes
        URL = self.url + "XXXinvalidXXX"
        with support.transient_internet(URL):
            with self.assertWarns(DeprecationWarning):
                open_url = urllib.request.FancyURLopener().open(URL)
            try:
                code = open_url.getcode()
            finally:
                open_url.close()
            self.assertEqual(code, 404)

    def test_bad_address(self):
        # Make sure proper exception is raised when connecting to a bogus
        # address.

        # Given that both VeriSign and various ISPs have in
        # the past or are presently hijacking various invalid
        # domain name requests in an attempt to boost traffic
        # to their own sites, finding a domain name to use
        # for this test is difficult.  RFC2606 leads one to
        # believe that '.invalid' should work, but experience
        # seemed to indicate otherwise.  Single character
        # TLDs are likely to remain invalid, so this seems to
        # be the best choice. The trailing '.' prevents a
        # related problem: The normal DNS resolver appends
        # the domain names from the search path if there is
        # no '.' the end and, and if one of those domains
        # implements a '*' rule a result is returned.
        # However, none of this will prevent the test from
        # failing if the ISP hijacks all invalid domain
        # requests.  The real solution would be to be able to
        # parameterize the framework with a mock resolver.
        bogus_domain = "sadflkjsasf.i.nvali.d."
        try:
            socket.gethostbyname(bogus_domain)
        except OSError:
            # socket.gaierror is too narrow, since getaddrinfo() may also
            # fail with EAI_SYSTEM and ETIMEDOUT (seen on Ubuntu 13.04),
            # i.e. Python's TimeoutError.
            pass
        else:
            # This happens with some overzealous DNS providers such as OpenDNS
            self.skipTest("%r should not resolve for test to work" % bogus_domain)
        failure_explanation = ('opening an invalid URL did not raise OSError; '
                               'can be caused by a broken DNS server '
                               '(e.g. returns 404 or hijacks page)')
        with self.assertRaises(OSError, msg=failure_explanation):
            urllib.request.urlopen("http://{}/".format(bogus_domain))


class urlretrieveNetworkTests(unittest.TestCase):
    """Tests urllib.request.urlretrieve using the network."""

    @contextlib.contextmanager
    def urlretrieve(self, *args, **kwargs):
        resource = args[0]
        with support.transient_internet(resource):
            file_location, info = urllib.request.urlretrieve(*args, **kwargs)
            try:
                yield file_location, info
            finally:
                support.unlink(file_location)

    def test_basic(self):
        # Test basic functionality.
        with self.urlretrieve(self.logo) as (file_location, info):
            self.assertTrue(os.path.exists(file_location), "file location returned by"
                            " urlretrieve is not a valid path")
            with open(file_location, 'rb') as f:
                self.assertTrue(f.read(), "reading from the file location returned"
                                " by urlretrieve failed")

    def test_specified_path(self):
        # Make sure that specifying the location of the file to write to works.
        with self.urlretrieve(self.logo,
                              support.TESTFN) as (file_location, info):
            self.assertEqual(file_location, support.TESTFN)
            self.assertTrue(os.path.exists(file_location))
            with open(file_location, 'rb') as f:
                self.assertTrue(f.read(), "reading from temporary file failed")

    def test_header(self):
        # Make sure header returned as 2nd value from urlretrieve is good.
        with self.urlretrieve(self.logo) as (file_location, info):
            self.assertIsInstance(info, email.message.Message,
                                  "info is not an instance of email.message.Message")

    logo = "http://www.pythontest.net/"

    def test_data_header(self):
        with self.urlretrieve(self.logo) as (file_location, fileheaders):
            datevalue = fileheaders.get('Date')
            dateformat = '%a, %d %b %Y %H:%M:%S GMT'
            try:
                time.strptime(datevalue, dateformat)
            except ValueError:
                self.fail('Date value not in %r format' % dateformat)

    def test_reporthook(self):
        records = []

        def recording_reporthook(blocks, block_size, total_size):
            records.append((blocks, block_size, total_size))

        with self.urlretrieve(self.logo, reporthook=recording_reporthook) as (
                file_location, fileheaders):
            expected_size = int(fileheaders['Content-Length'])

        records_repr = repr(records)  # For use in error messages.
        self.assertGreater(len(records), 1, msg="There should always be two "
                           "calls; the first one before the transfer starts.")
        self.assertEqual(records[0][0], 0)
        self.assertGreater(records[0][1], 0,
                           msg="block size can't be 0 in %s" % records_repr)
        self.assertEqual(records[0][2], expected_size)
        self.assertEqual(records[-1][2], expected_size)

        block_sizes = {block_size for _, block_size, _ in records}
        self.assertEqual({records[0][1]}, block_sizes,
                         msg="block sizes in %s must be equal" % records_repr)
        self.assertGreaterEqual(records[-1][0]*records[0][1], expected_size,
                                msg="number of blocks * block size must be"
                                " >= total size in %s" % records_repr)


if __name__ == "__main__":
    unittest.main()
