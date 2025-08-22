"""Regression tests for what was in Python 2's "urllib" module"""

import urllib.parse
import urllib.request
import urllib.error
import http.client
import email.message
import io
import unittest
from test import support
from test.support import os_helper
from test.support import socket_helper
import os
import socket
try:
    import ssl
except ImportError:
    ssl = None
import sys
import tempfile

import collections


if not socket_helper.has_gethostname:
    raise unittest.SkipTest("test requires gethostname()")


def hexescape(char):
    """Escape char as RFC 2396 specifies"""
    hex_repr = hex(ord(char))[2:].upper()
    if len(hex_repr) == 1:
        hex_repr = "0%s" % hex_repr
    return "%" + hex_repr


def fakehttp(fakedata, mock_close=False):
    class FakeSocket(io.BytesIO):
        io_refs = 1

        def sendall(self, data):
            FakeHTTPConnection.buf = data

        def makefile(self, *args, **kwds):
            self.io_refs += 1
            return self

        def read(self, amt=None):
            if self.closed:
                return b""
            return io.BytesIO.read(self, amt)

        def readline(self, length=None):
            if self.closed:
                return b""
            return io.BytesIO.readline(self, length)

        def close(self):
            self.io_refs -= 1
            if self.io_refs == 0:
                io.BytesIO.close(self)

    class FakeHTTPConnection(http.client.HTTPConnection):

        # buffer to store data for verification in urlopen tests.
        buf = None

        def connect(self):
            self.sock = FakeSocket(self.fakedata)
            type(self).fakesock = self.sock

        if mock_close:
            # bpo-36918: HTTPConnection destructor calls close() which calls
            # flush(). Problem: flush() calls self.fp.flush() which raises
            # "ValueError: I/O operation on closed file" which is logged as an
            # "Exception ignored in". Override close() to silence this error.
            def close(self):
                pass
    FakeHTTPConnection.fakedata = fakedata

    return FakeHTTPConnection


class FakeHTTPMixin(object):
    def fakehttp(self, fakedata, mock_close=False):
        fake_http_class = fakehttp(fakedata, mock_close=mock_close)
        self._connection_class = http.client.HTTPConnection
        http.client.HTTPConnection = fake_http_class

    def unfakehttp(self):
        http.client.HTTPConnection = self._connection_class


class urlopen_FileTests(unittest.TestCase):
    """Test urlopen() opening a temporary file.

    Try to test as much functionality as possible so as to cut down on reliance
    on connecting to the Net for testing.

    """

    def setUp(self):
        # Create a temp file to use for testing
        self.text = bytes("test_urllib: %s\n" % self.__class__.__name__,
                          "ascii")
        f = open(os_helper.TESTFN, 'wb')
        try:
            f.write(self.text)
        finally:
            f.close()
        self.pathname = os_helper.TESTFN
        self.quoted_pathname = urllib.parse.quote(os.fsencode(self.pathname))
        self.returned_obj = urllib.request.urlopen("file:%s" % self.quoted_pathname)

    def tearDown(self):
        """Shut down the open object"""
        self.returned_obj.close()
        os.remove(os_helper.TESTFN)

    def test_interface(self):
        # Make sure object returned by urlopen() has the specified methods
        for attr in ("read", "readline", "readlines", "fileno",
                     "close", "info", "geturl", "getcode", "__iter__"):
            self.assertHasAttr(self.returned_obj, attr)

    def test_read(self):
        self.assertEqual(self.text, self.returned_obj.read())

    def test_readline(self):
        self.assertEqual(self.text, self.returned_obj.readline())
        self.assertEqual(b'', self.returned_obj.readline(),
                         "calling readline() after exhausting the file did not"
                         " return an empty string")

    def test_readlines(self):
        lines_list = self.returned_obj.readlines()
        self.assertEqual(len(lines_list), 1,
                         "readlines() returned the wrong number of lines")
        self.assertEqual(lines_list[0], self.text,
                         "readlines() returned improper text")

    def test_fileno(self):
        file_num = self.returned_obj.fileno()
        self.assertIsInstance(file_num, int, "fileno() did not return an int")
        self.assertEqual(os.read(file_num, len(self.text)), self.text,
                         "Reading on the file descriptor returned by fileno() "
                         "did not return the expected text")

    def test_close(self):
        # Test close() by calling it here and then having it be called again
        # by the tearDown() method for the test
        self.returned_obj.close()

    def test_headers(self):
        self.assertIsInstance(self.returned_obj.headers, email.message.Message)

    def test_url(self):
        self.assertEqual(self.returned_obj.url, "file:" + self.quoted_pathname)

    def test_status(self):
        self.assertIsNone(self.returned_obj.status)

    def test_info(self):
        self.assertIsInstance(self.returned_obj.info(), email.message.Message)

    def test_geturl(self):
        self.assertEqual(self.returned_obj.geturl(), "file:" + self.quoted_pathname)

    def test_getcode(self):
        self.assertIsNone(self.returned_obj.getcode())

    def test_iter(self):
        # Test iterator
        # Don't need to count number of iterations since test would fail the
        # instant it returned anything beyond the first line from the
        # comparison.
        # Use the iterator in the usual implicit way to test for ticket #4608.
        for line in self.returned_obj:
            self.assertEqual(line, self.text)

    def test_relativelocalfile(self):
        self.assertRaises(ValueError,urllib.request.urlopen,'./' + self.pathname)

    def test_remote_authority(self):
        # Test for GH-90812.
        url = 'file://pythontest.net/foo/bar'
        with self.assertRaises(urllib.error.URLError) as e:
            urllib.request.urlopen(url)
        if os.name == 'nt':
            self.assertEqual(e.exception.filename, r'\\pythontest.net\foo\bar')
        else:
            self.assertEqual(e.exception.reason, 'file:// scheme is supported only on localhost')


class ProxyTests(unittest.TestCase):

    def setUp(self):
        # Records changes to env vars
        self.env = self.enterContext(os_helper.EnvironmentVarGuard())
        # Delete all proxy related env vars
        for k in list(os.environ):
            if 'proxy' in k.lower():
                self.env.unset(k)

    def test_getproxies_environment_keep_no_proxies(self):
        self.env.set('NO_PROXY', 'localhost')
        proxies = urllib.request.getproxies_environment()
        # getproxies_environment use lowered case truncated (no '_proxy') keys
        self.assertEqual('localhost', proxies['no'])
        # List of no_proxies with space.
        self.env.set('NO_PROXY', 'localhost, anotherdomain.com, newdomain.com:1234')
        self.assertTrue(urllib.request.proxy_bypass_environment('anotherdomain.com'))
        self.assertTrue(urllib.request.proxy_bypass_environment('anotherdomain.com:8888'))
        self.assertTrue(urllib.request.proxy_bypass_environment('newdomain.com:1234'))

    def test_proxy_cgi_ignore(self):
        try:
            self.env.set('HTTP_PROXY', 'http://somewhere:3128')
            proxies = urllib.request.getproxies_environment()
            self.assertEqual('http://somewhere:3128', proxies['http'])
            self.env.set('REQUEST_METHOD', 'GET')
            proxies = urllib.request.getproxies_environment()
            self.assertNotIn('http', proxies)
        finally:
            self.env.unset('REQUEST_METHOD')
            self.env.unset('HTTP_PROXY')

    def test_proxy_bypass_environment_host_match(self):
        bypass = urllib.request.proxy_bypass_environment
        self.env.set('NO_PROXY',
                     'localhost, anotherdomain.com, newdomain.com:1234, .d.o.t')
        self.assertTrue(bypass('localhost'))
        self.assertTrue(bypass('LocalHost'))                 # MixedCase
        self.assertTrue(bypass('LOCALHOST'))                 # UPPERCASE
        self.assertTrue(bypass('.localhost'))
        self.assertTrue(bypass('newdomain.com:1234'))
        self.assertTrue(bypass('.newdomain.com:1234'))
        self.assertTrue(bypass('foo.d.o.t'))                 # issue 29142
        self.assertTrue(bypass('d.o.t'))
        self.assertTrue(bypass('anotherdomain.com:8888'))
        self.assertTrue(bypass('.anotherdomain.com:8888'))
        self.assertTrue(bypass('www.newdomain.com:1234'))
        self.assertFalse(bypass('prelocalhost'))
        self.assertFalse(bypass('newdomain.com'))            # no port
        self.assertFalse(bypass('newdomain.com:1235'))       # wrong port

    def test_proxy_bypass_environment_always_match(self):
        bypass = urllib.request.proxy_bypass_environment
        self.env.set('NO_PROXY', '*')
        self.assertTrue(bypass('newdomain.com'))
        self.assertTrue(bypass('newdomain.com:1234'))
        self.env.set('NO_PROXY', '*, anotherdomain.com')
        self.assertTrue(bypass('anotherdomain.com'))
        self.assertFalse(bypass('newdomain.com'))
        self.assertFalse(bypass('newdomain.com:1234'))

    def test_proxy_bypass_environment_newline(self):
        bypass = urllib.request.proxy_bypass_environment
        self.env.set('NO_PROXY',
                     'localhost, anotherdomain.com, newdomain.com:1234')
        self.assertFalse(bypass('localhost\n'))
        self.assertFalse(bypass('anotherdomain.com:8888\n'))
        self.assertFalse(bypass('newdomain.com:1234\n'))


class ProxyTests_withOrderedEnv(unittest.TestCase):

    def setUp(self):
        # We need to test conditions, where variable order _is_ significant
        self._saved_env = os.environ
        # Monkey patch os.environ, start with empty fake environment
        os.environ = collections.OrderedDict()

    def tearDown(self):
        os.environ = self._saved_env

    def test_getproxies_environment_prefer_lowercase(self):
        # Test lowercase preference with removal
        os.environ['no_proxy'] = ''
        os.environ['No_Proxy'] = 'localhost'
        self.assertFalse(urllib.request.proxy_bypass_environment('localhost'))
        self.assertFalse(urllib.request.proxy_bypass_environment('arbitrary'))
        os.environ['http_proxy'] = ''
        os.environ['HTTP_PROXY'] = 'http://somewhere:3128'
        proxies = urllib.request.getproxies_environment()
        self.assertEqual({}, proxies)
        # Test lowercase preference of proxy bypass and correct matching including ports
        os.environ['no_proxy'] = 'localhost, noproxy.com, my.proxy:1234'
        os.environ['No_Proxy'] = 'xyz.com'
        self.assertTrue(urllib.request.proxy_bypass_environment('localhost'))
        self.assertTrue(urllib.request.proxy_bypass_environment('noproxy.com:5678'))
        self.assertTrue(urllib.request.proxy_bypass_environment('my.proxy:1234'))
        self.assertFalse(urllib.request.proxy_bypass_environment('my.proxy'))
        self.assertFalse(urllib.request.proxy_bypass_environment('arbitrary'))
        # Test lowercase preference with replacement
        os.environ['http_proxy'] = 'http://somewhere:3128'
        os.environ['Http_Proxy'] = 'http://somewhereelse:3128'
        proxies = urllib.request.getproxies_environment()
        self.assertEqual('http://somewhere:3128', proxies['http'])


class urlopen_HttpTests(unittest.TestCase, FakeHTTPMixin):
    """Test urlopen() opening a fake http connection."""

    def check_read(self, ver):
        self.fakehttp(b"HTTP/" + ver + b" 200 OK\r\n\r\nHello!")
        try:
            fp = urllib.request.urlopen("http://python.org/")
            self.assertEqual(fp.readline(), b"Hello!")
            self.assertEqual(fp.readline(), b"")
            self.assertEqual(fp.geturl(), 'http://python.org/')
            self.assertEqual(fp.getcode(), 200)
        finally:
            self.unfakehttp()

    def test_url_fragment(self):
        # Issue #11703: geturl() omits fragments in the original URL.
        url = 'http://docs.python.org/library/urllib.html#OK'
        self.fakehttp(b"HTTP/1.1 200 OK\r\n\r\nHello!")
        try:
            fp = urllib.request.urlopen(url)
            self.assertEqual(fp.geturl(), url)
        finally:
            self.unfakehttp()

    def test_willclose(self):
        self.fakehttp(b"HTTP/1.1 200 OK\r\n\r\nHello!")
        try:
            resp = urllib.request.urlopen("http://www.python.org")
            self.assertTrue(resp.will_close)
        finally:
            self.unfakehttp()

    @unittest.skipUnless(ssl, "ssl module required")
    def test_url_path_with_control_char_rejected(self):
        for char_no in list(range(0, 0x21)) + [0x7f]:
            char = chr(char_no)
            schemeless_url = f"//localhost:7777/test{char}/"
            self.fakehttp(b"HTTP/1.1 200 OK\r\n\r\nHello.")
            try:
                # We explicitly test urllib.request.urlopen() instead of the top
                # level 'def urlopen()' function defined in this... (quite ugly)
                # test suite.  They use different url opening codepaths.  Plain
                # urlopen uses FancyURLOpener which goes via a codepath that
                # calls urllib.parse.quote() on the URL which makes all of the
                # above attempts at injection within the url _path_ safe.
                escaped_char_repr = repr(char).replace('\\', r'\\')
                InvalidURL = http.client.InvalidURL
                with self.assertRaisesRegex(
                    InvalidURL, f"contain control.*{escaped_char_repr}"):
                    urllib.request.urlopen(f"http:{schemeless_url}")
                with self.assertRaisesRegex(
                    InvalidURL, f"contain control.*{escaped_char_repr}"):
                    urllib.request.urlopen(f"https:{schemeless_url}")
            finally:
                self.unfakehttp()

    @unittest.skipUnless(ssl, "ssl module required")
    def test_url_path_with_newline_header_injection_rejected(self):
        self.fakehttp(b"HTTP/1.1 200 OK\r\n\r\nHello.")
        host = "localhost:7777?a=1 HTTP/1.1\r\nX-injected: header\r\nTEST: 123"
        schemeless_url = "//" + host + ":8080/test/?test=a"
        try:
            # We explicitly test urllib.request.urlopen() instead of the top
            # level 'def urlopen()' function defined in this... (quite ugly)
            # test suite.  They use different url opening codepaths.  Plain
            # urlopen uses FancyURLOpener which goes via a codepath that
            # calls urllib.parse.quote() on the URL which makes all of the
            # above attempts at injection within the url _path_ safe.
            InvalidURL = http.client.InvalidURL
            with self.assertRaisesRegex(
                InvalidURL, r"contain control.*\\r.*(found at least . .)"):
                urllib.request.urlopen(f"http:{schemeless_url}")
            with self.assertRaisesRegex(InvalidURL, r"contain control.*\\n"):
                urllib.request.urlopen(f"https:{schemeless_url}")
        finally:
            self.unfakehttp()

    @unittest.skipUnless(ssl, "ssl module required")
    def test_url_host_with_control_char_rejected(self):
        for char_no in list(range(0, 0x21)) + [0x7f]:
            char = chr(char_no)
            schemeless_url = f"//localhost{char}/test/"
            self.fakehttp(b"HTTP/1.1 200 OK\r\n\r\nHello.")
            try:
                escaped_char_repr = repr(char).replace('\\', r'\\')
                InvalidURL = http.client.InvalidURL
                with self.assertRaisesRegex(
                    InvalidURL, f"contain control.*{escaped_char_repr}"):
                    urllib.request.urlopen(f"http:{schemeless_url}")
                with self.assertRaisesRegex(InvalidURL, f"contain control.*{escaped_char_repr}"):
                    urllib.request.urlopen(f"https:{schemeless_url}")
            finally:
                self.unfakehttp()

    @unittest.skipUnless(ssl, "ssl module required")
    def test_url_host_with_newline_header_injection_rejected(self):
        self.fakehttp(b"HTTP/1.1 200 OK\r\n\r\nHello.")
        host = "localhost\r\nX-injected: header\r\n"
        schemeless_url = "//" + host + ":8080/test/?test=a"
        try:
            InvalidURL = http.client.InvalidURL
            with self.assertRaisesRegex(
                InvalidURL, r"contain control.*\\r"):
                urllib.request.urlopen(f"http:{schemeless_url}")
            with self.assertRaisesRegex(InvalidURL, r"contain control.*\\n"):
                urllib.request.urlopen(f"https:{schemeless_url}")
        finally:
            self.unfakehttp()

    def test_read_0_9(self):
        # "0.9" response accepted (but not "simple responses" without
        # a status line)
        self.check_read(b"0.9")

    def test_read_1_0(self):
        self.check_read(b"1.0")

    def test_read_1_1(self):
        self.check_read(b"1.1")

    def test_read_bogus(self):
        # urlopen() should raise OSError for many error codes.
        self.fakehttp(b'''HTTP/1.1 401 Authentication Required
Date: Wed, 02 Jan 2008 03:03:54 GMT
Server: Apache/1.3.33 (Debian GNU/Linux) mod_ssl/2.8.22 OpenSSL/0.9.7e
Connection: close
Content-Type: text/html; charset=iso-8859-1
''', mock_close=True)
        try:
            with self.assertRaises(urllib.error.HTTPError) as cm:
                urllib.request.urlopen("http://python.org/")
            cm.exception.close()
        finally:
            self.unfakehttp()

    def test_invalid_redirect(self):
        # urlopen() should raise OSError for many error codes.
        self.fakehttp(b'''HTTP/1.1 302 Found
Date: Wed, 02 Jan 2008 03:03:54 GMT
Server: Apache/1.3.33 (Debian GNU/Linux) mod_ssl/2.8.22 OpenSSL/0.9.7e
Location: file://guidocomputer.athome.com:/python/license
Connection: close
Content-Type: text/html; charset=iso-8859-1
''', mock_close=True)
        try:
            msg = "Redirection to url 'file:"
            with self.assertRaisesRegex(urllib.error.HTTPError, msg) as cm:
                urllib.request.urlopen("http://python.org/")
            cm.exception.close()
        finally:
            self.unfakehttp()

    def test_redirect_limit_independent(self):
        # Ticket #12923: make sure independent requests each use their
        # own retry limit.
        for i in range(urllib.request.HTTPRedirectHandler.max_redirections):
            self.fakehttp(b'''HTTP/1.1 302 Found
Location: file://guidocomputer.athome.com:/python/license
Connection: close
''', mock_close=True)
            try:
                with self.assertRaises(urllib.error.HTTPError) as cm:
                    urllib.request.urlopen("http://something")
                cm.exception.close()
            finally:
                self.unfakehttp()

    def test_empty_socket(self):
        # urlopen() raises OSError if the underlying socket does not send any
        # data. (#1680230)
        self.fakehttp(b'')
        try:
            self.assertRaises(OSError, urllib.request.urlopen, "http://something")
        finally:
            self.unfakehttp()

    def test_missing_localfile(self):
        # Test for #10836
        with self.assertRaises(urllib.error.URLError) as e:
            urllib.request.urlopen('file://localhost/a/file/which/doesnot/exists.py')
        self.assertTrue(e.exception.filename)
        self.assertTrue(e.exception.reason)

    def test_file_notexists(self):
        fd, tmp_file = tempfile.mkstemp()
        tmp_file_canon_url = urllib.request.pathname2url(tmp_file, add_scheme=True)
        parsed = urllib.parse.urlsplit(tmp_file_canon_url)
        tmp_fileurl = parsed._replace(netloc='localhost').geturl()
        try:
            self.assertTrue(os.path.exists(tmp_file))
            with urllib.request.urlopen(tmp_fileurl) as fobj:
                self.assertTrue(fobj)
                self.assertEqual(fobj.url, tmp_file_canon_url)
        finally:
            os.close(fd)
            os.unlink(tmp_file)
        self.assertFalse(os.path.exists(tmp_file))
        with self.assertRaises(urllib.error.URLError):
            urllib.request.urlopen(tmp_fileurl)

    def test_ftp_nohost(self):
        test_ftp_url = 'ftp:///path'
        with self.assertRaises(urllib.error.URLError) as e:
            urllib.request.urlopen(test_ftp_url)
        self.assertFalse(e.exception.filename)
        self.assertTrue(e.exception.reason)

    def test_ftp_nonexisting(self):
        with self.assertRaises(urllib.error.URLError) as e:
            urllib.request.urlopen('ftp://localhost/a/file/which/doesnot/exists.py')
        self.assertFalse(e.exception.filename)
        self.assertTrue(e.exception.reason)


class urlopen_DataTests(unittest.TestCase):
    """Test urlopen() opening a data URL."""

    def setUp(self):
        # clear _opener global variable
        self.addCleanup(urllib.request.urlcleanup)

        # text containing URL special- and unicode-characters
        self.text = "test data URLs :;,%=& \u00f6 \u00c4 "
        # 2x1 pixel RGB PNG image with one black and one white pixel
        self.image = (
            b'\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x02\x00\x00\x00'
            b'\x01\x08\x02\x00\x00\x00{@\xe8\xdd\x00\x00\x00\x01sRGB\x00\xae'
            b'\xce\x1c\xe9\x00\x00\x00\x0fIDAT\x08\xd7c```\xf8\xff\xff?\x00'
            b'\x06\x01\x02\xfe\no/\x1e\x00\x00\x00\x00IEND\xaeB`\x82')

        self.text_url = (
            "data:text/plain;charset=UTF-8,test%20data%20URLs%20%3A%3B%2C%25%3"
            "D%26%20%C3%B6%20%C3%84%20")
        self.text_url_base64 = (
            "data:text/plain;charset=ISO-8859-1;base64,dGVzdCBkYXRhIFVSTHMgOjs"
            "sJT0mIPYgxCA%3D")
        # base64 encoded data URL that contains ignorable spaces,
        # such as "\n", " ", "%0A", and "%20".
        self.image_url = (
            "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAABCAIAAAB7\n"
            "QOjdAAAAAXNSR0IArs4c6QAAAA9JREFUCNdj%0AYGBg%2BP//PwAGAQL%2BCm8 "
            "vHgAAAABJRU5ErkJggg%3D%3D%0A%20")

        self.text_url_resp = self.enterContext(
            urllib.request.urlopen(self.text_url))
        self.text_url_base64_resp = self.enterContext(
            urllib.request.urlopen(self.text_url_base64))
        self.image_url_resp = self.enterContext(urllib.request.urlopen(self.image_url))

    def test_interface(self):
        # Make sure object returned by urlopen() has the specified methods
        for attr in ("read", "readline", "readlines",
                     "close", "info", "geturl", "getcode", "__iter__"):
            self.assertHasAttr(self.text_url_resp, attr)

    def test_info(self):
        self.assertIsInstance(self.text_url_resp.info(), email.message.Message)
        self.assertEqual(self.text_url_base64_resp.info().get_params(),
            [('text/plain', ''), ('charset', 'ISO-8859-1')])
        self.assertEqual(self.image_url_resp.info()['content-length'],
            str(len(self.image)))
        r = urllib.request.urlopen("data:,")
        self.assertEqual(r.info().get_params(),
            [('text/plain', ''), ('charset', 'US-ASCII')])
        r.close()

    def test_geturl(self):
        self.assertEqual(self.text_url_resp.geturl(), self.text_url)
        self.assertEqual(self.text_url_base64_resp.geturl(),
            self.text_url_base64)
        self.assertEqual(self.image_url_resp.geturl(), self.image_url)

    def test_read_text(self):
        self.assertEqual(self.text_url_resp.read().decode(
            dict(self.text_url_resp.info().get_params())['charset']), self.text)

    def test_read_text_base64(self):
        self.assertEqual(self.text_url_base64_resp.read().decode(
            dict(self.text_url_base64_resp.info().get_params())['charset']),
            self.text)

    def test_read_image(self):
        self.assertEqual(self.image_url_resp.read(), self.image)

    def test_missing_comma(self):
        self.assertRaises(ValueError,urllib.request.urlopen,'data:text/plain')

    def test_invalid_base64_data(self):
        # missing padding character
        self.assertRaises(ValueError,urllib.request.urlopen,'data:;base64,Cg=')


class urlretrieve_FileTests(unittest.TestCase):
    """Test urllib.urlretrieve() on local files"""

    def setUp(self):
        # clear _opener global variable
        self.addCleanup(urllib.request.urlcleanup)

        # Create a list of temporary files. Each item in the list is a file
        # name (absolute path or relative to the current working directory).
        # All files in this list will be deleted in the tearDown method. Note,
        # this only helps to makes sure temporary files get deleted, but it
        # does nothing about trying to close files that may still be open. It
        # is the responsibility of the developer to properly close files even
        # when exceptional conditions occur.
        self.tempFiles = []

        # Create a temporary file.
        self.registerFileForCleanUp(os_helper.TESTFN)
        self.text = b'testing urllib.urlretrieve'
        try:
            FILE = open(os_helper.TESTFN, 'wb')
            FILE.write(self.text)
            FILE.close()
        finally:
            try: FILE.close()
            except: pass

    def tearDown(self):
        # Delete the temporary files.
        for each in self.tempFiles:
            try: os.remove(each)
            except: pass

    def constructLocalFileUrl(self, filePath):
        filePath = os.path.abspath(filePath)
        return urllib.request.pathname2url(filePath, add_scheme=True)

    def createNewTempFile(self, data=b""):
        """Creates a new temporary file containing the specified data,
        registers the file for deletion during the test fixture tear down, and
        returns the absolute path of the file."""

        newFd, newFilePath = tempfile.mkstemp()
        try:
            self.registerFileForCleanUp(newFilePath)
            newFile = os.fdopen(newFd, "wb")
            newFile.write(data)
            newFile.close()
        finally:
            try: newFile.close()
            except: pass
        return newFilePath

    def registerFileForCleanUp(self, fileName):
        self.tempFiles.append(fileName)

    def test_basic(self):
        # Make sure that a local file just gets its own location returned and
        # a headers value is returned.
        result = urllib.request.urlretrieve("file:%s" % os_helper.TESTFN)
        self.assertEqual(result[0], os_helper.TESTFN)
        self.assertIsInstance(result[1], email.message.Message,
                              "did not get an email.message.Message instance "
                              "as second returned value")

    def test_copy(self):
        # Test that setting the filename argument works.
        second_temp = "%s.2" % os_helper.TESTFN
        self.registerFileForCleanUp(second_temp)
        result = urllib.request.urlretrieve(self.constructLocalFileUrl(
            os_helper.TESTFN), second_temp)
        self.assertEqual(second_temp, result[0])
        self.assertTrue(os.path.exists(second_temp), "copy of the file was not "
                                                  "made")
        FILE = open(second_temp, 'rb')
        try:
            text = FILE.read()
            FILE.close()
        finally:
            try: FILE.close()
            except: pass
        self.assertEqual(self.text, text)

    def test_reporthook(self):
        # Make sure that the reporthook works.
        def hooktester(block_count, block_read_size, file_size, count_holder=[0]):
            self.assertIsInstance(block_count, int)
            self.assertIsInstance(block_read_size, int)
            self.assertIsInstance(file_size, int)
            self.assertEqual(block_count, count_holder[0])
            count_holder[0] = count_holder[0] + 1
        second_temp = "%s.2" % os_helper.TESTFN
        self.registerFileForCleanUp(second_temp)
        urllib.request.urlretrieve(
            self.constructLocalFileUrl(os_helper.TESTFN),
            second_temp, hooktester)

    def test_reporthook_0_bytes(self):
        # Test on zero length file. Should call reporthook only 1 time.
        report = []
        def hooktester(block_count, block_read_size, file_size, _report=report):
            _report.append((block_count, block_read_size, file_size))
        srcFileName = self.createNewTempFile()
        urllib.request.urlretrieve(self.constructLocalFileUrl(srcFileName),
            os_helper.TESTFN, hooktester)
        self.assertEqual(len(report), 1)
        self.assertEqual(report[0][2], 0)

    def test_reporthook_5_bytes(self):
        # Test on 5 byte file. Should call reporthook only 2 times (once when
        # the "network connection" is established and once when the block is
        # read).
        report = []
        def hooktester(block_count, block_read_size, file_size, _report=report):
            _report.append((block_count, block_read_size, file_size))
        srcFileName = self.createNewTempFile(b"x" * 5)
        urllib.request.urlretrieve(self.constructLocalFileUrl(srcFileName),
            os_helper.TESTFN, hooktester)
        self.assertEqual(len(report), 2)
        self.assertEqual(report[0][2], 5)
        self.assertEqual(report[1][2], 5)

    def test_reporthook_8193_bytes(self):
        # Test on 8193 byte file. Should call reporthook only 3 times (once
        # when the "network connection" is established, once for the next 8192
        # bytes, and once for the last byte).
        report = []
        def hooktester(block_count, block_read_size, file_size, _report=report):
            _report.append((block_count, block_read_size, file_size))
        srcFileName = self.createNewTempFile(b"x" * 8193)
        urllib.request.urlretrieve(self.constructLocalFileUrl(srcFileName),
            os_helper.TESTFN, hooktester)
        self.assertEqual(len(report), 3)
        self.assertEqual(report[0][2], 8193)
        self.assertEqual(report[0][1], 8192)
        self.assertEqual(report[1][1], 8192)
        self.assertEqual(report[2][1], 8192)


class urlretrieve_HttpTests(unittest.TestCase, FakeHTTPMixin):
    """Test urllib.urlretrieve() using fake http connections"""

    def test_short_content_raises_ContentTooShortError(self):
        self.addCleanup(urllib.request.urlcleanup)

        self.fakehttp(b'''HTTP/1.1 200 OK
Date: Wed, 02 Jan 2008 03:03:54 GMT
Server: Apache/1.3.33 (Debian GNU/Linux) mod_ssl/2.8.22 OpenSSL/0.9.7e
Connection: close
Content-Length: 100
Content-Type: text/html; charset=iso-8859-1

FF
''')

        def _reporthook(par1, par2, par3):
            pass

        with self.assertRaises(urllib.error.ContentTooShortError):
            try:
                urllib.request.urlretrieve(support.TEST_HTTP_URL,
                                           reporthook=_reporthook)
            finally:
                self.unfakehttp()

    def test_short_content_raises_ContentTooShortError_without_reporthook(self):
        self.addCleanup(urllib.request.urlcleanup)

        self.fakehttp(b'''HTTP/1.1 200 OK
Date: Wed, 02 Jan 2008 03:03:54 GMT
Server: Apache/1.3.33 (Debian GNU/Linux) mod_ssl/2.8.22 OpenSSL/0.9.7e
Connection: close
Content-Length: 100
Content-Type: text/html; charset=iso-8859-1

FF
''')
        with self.assertRaises(urllib.error.ContentTooShortError):
            try:
                urllib.request.urlretrieve(support.TEST_HTTP_URL)
            finally:
                self.unfakehttp()


class QuotingTests(unittest.TestCase):
    r"""Tests for urllib.quote() and urllib.quote_plus()

    According to RFC 3986 (Uniform Resource Identifiers), to escape a
    character you write it as '%' + <2 character US-ASCII hex value>.
    The Python code of ``'%' + hex(ord(<character>))[2:]`` escapes a
    character properly. Case does not matter on the hex letters.

    The various character sets specified are:

    Reserved characters : ";/?:@&=+$,"
        Have special meaning in URIs and must be escaped if not being used for
        their special meaning
    Data characters : letters, digits, and "-_.!~*'()"
        Unreserved and do not need to be escaped; can be, though, if desired
    Control characters : 0x00 - 0x1F, 0x7F
        Have no use in URIs so must be escaped
    space : 0x20
        Must be escaped
    Delimiters : '<>#%"'
        Must be escaped
    Unwise : "{}|\^[]`"
        Must be escaped

    """

    def test_never_quote(self):
        # Make sure quote() does not quote letters, digits, and "_,.-"
        do_not_quote = '' .join(["ABCDEFGHIJKLMNOPQRSTUVWXYZ",
                                 "abcdefghijklmnopqrstuvwxyz",
                                 "0123456789",
                                 "_.-~"])
        result = urllib.parse.quote(do_not_quote)
        self.assertEqual(do_not_quote, result,
                         "using quote(): %r != %r" % (do_not_quote, result))
        result = urllib.parse.quote_plus(do_not_quote)
        self.assertEqual(do_not_quote, result,
                        "using quote_plus(): %r != %r" % (do_not_quote, result))

    def test_default_safe(self):
        # Test '/' is default value for 'safe' parameter
        self.assertEqual(urllib.parse.quote.__defaults__[0], '/')

    def test_safe(self):
        # Test setting 'safe' parameter does what it should do
        quote_by_default = "<>"
        result = urllib.parse.quote(quote_by_default, safe=quote_by_default)
        self.assertEqual(quote_by_default, result,
                         "using quote(): %r != %r" % (quote_by_default, result))
        result = urllib.parse.quote_plus(quote_by_default,
                                         safe=quote_by_default)
        self.assertEqual(quote_by_default, result,
                         "using quote_plus(): %r != %r" %
                         (quote_by_default, result))
        # Safe expressed as bytes rather than str
        result = urllib.parse.quote(quote_by_default, safe=b"<>")
        self.assertEqual(quote_by_default, result,
                         "using quote(): %r != %r" % (quote_by_default, result))
        # "Safe" non-ASCII characters should have no effect
        # (Since URIs are not allowed to have non-ASCII characters)
        result = urllib.parse.quote("a\xfcb", encoding="latin-1", safe="\xfc")
        expect = urllib.parse.quote("a\xfcb", encoding="latin-1", safe="")
        self.assertEqual(expect, result,
                         "using quote(): %r != %r" %
                         (expect, result))
        # Same as above, but using a bytes rather than str
        result = urllib.parse.quote("a\xfcb", encoding="latin-1", safe=b"\xfc")
        expect = urllib.parse.quote("a\xfcb", encoding="latin-1", safe="")
        self.assertEqual(expect, result,
                         "using quote(): %r != %r" %
                         (expect, result))

    def test_default_quoting(self):
        # Make sure all characters that should be quoted are by default sans
        # space (separate test for that).
        should_quote = [chr(num) for num in range(32)] # For 0x00 - 0x1F
        should_quote.append(r'<>#%"{}|\^[]`')
        should_quote.append(chr(127)) # For 0x7F
        should_quote = ''.join(should_quote)
        for char in should_quote:
            result = urllib.parse.quote(char)
            self.assertEqual(hexescape(char), result,
                             "using quote(): "
                             "%s should be escaped to %s, not %s" %
                             (char, hexescape(char), result))
            result = urllib.parse.quote_plus(char)
            self.assertEqual(hexescape(char), result,
                             "using quote_plus(): "
                             "%s should be escapes to %s, not %s" %
                             (char, hexescape(char), result))
        del should_quote
        partial_quote = "ab[]cd"
        expected = "ab%5B%5Dcd"
        result = urllib.parse.quote(partial_quote)
        self.assertEqual(expected, result,
                         "using quote(): %r != %r" % (expected, result))
        result = urllib.parse.quote_plus(partial_quote)
        self.assertEqual(expected, result,
                         "using quote_plus(): %r != %r" % (expected, result))

    def test_quoting_space(self):
        # Make sure quote() and quote_plus() handle spaces as specified in
        # their unique way
        result = urllib.parse.quote(' ')
        self.assertEqual(result, hexescape(' '),
                         "using quote(): %r != %r" % (result, hexescape(' ')))
        result = urllib.parse.quote_plus(' ')
        self.assertEqual(result, '+',
                         "using quote_plus(): %r != +" % result)
        given = "a b cd e f"
        expect = given.replace(' ', hexescape(' '))
        result = urllib.parse.quote(given)
        self.assertEqual(expect, result,
                         "using quote(): %r != %r" % (expect, result))
        expect = given.replace(' ', '+')
        result = urllib.parse.quote_plus(given)
        self.assertEqual(expect, result,
                         "using quote_plus(): %r != %r" % (expect, result))

    def test_quoting_plus(self):
        self.assertEqual(urllib.parse.quote_plus('alpha+beta gamma'),
                         'alpha%2Bbeta+gamma')
        self.assertEqual(urllib.parse.quote_plus('alpha+beta gamma', '+'),
                         'alpha+beta+gamma')
        # Test with bytes
        self.assertEqual(urllib.parse.quote_plus(b'alpha+beta gamma'),
                         'alpha%2Bbeta+gamma')
        # Test with safe bytes
        self.assertEqual(urllib.parse.quote_plus('alpha+beta gamma', b'+'),
                         'alpha+beta+gamma')

    def test_quote_bytes(self):
        # Bytes should quote directly to percent-encoded values
        given = b"\xa2\xd8ab\xff"
        expect = "%A2%D8ab%FF"
        result = urllib.parse.quote(given)
        self.assertEqual(expect, result,
                         "using quote(): %r != %r" % (expect, result))
        # Encoding argument should raise type error on bytes input
        self.assertRaises(TypeError, urllib.parse.quote, given,
                            encoding="latin-1")
        # quote_from_bytes should work the same
        result = urllib.parse.quote_from_bytes(given)
        self.assertEqual(expect, result,
                         "using quote_from_bytes(): %r != %r"
                         % (expect, result))

    def test_quote_with_unicode(self):
        # Characters in Latin-1 range, encoded by default in UTF-8
        given = "\xa2\xd8ab\xff"
        expect = "%C2%A2%C3%98ab%C3%BF"
        result = urllib.parse.quote(given)
        self.assertEqual(expect, result,
                         "using quote(): %r != %r" % (expect, result))
        # Characters in Latin-1 range, encoded by with None (default)
        result = urllib.parse.quote(given, encoding=None, errors=None)
        self.assertEqual(expect, result,
                         "using quote(): %r != %r" % (expect, result))
        # Characters in Latin-1 range, encoded with Latin-1
        given = "\xa2\xd8ab\xff"
        expect = "%A2%D8ab%FF"
        result = urllib.parse.quote(given, encoding="latin-1")
        self.assertEqual(expect, result,
                         "using quote(): %r != %r" % (expect, result))
        # Characters in BMP, encoded by default in UTF-8
        given = "\u6f22\u5b57"              # "Kanji"
        expect = "%E6%BC%A2%E5%AD%97"
        result = urllib.parse.quote(given)
        self.assertEqual(expect, result,
                         "using quote(): %r != %r" % (expect, result))
        # Characters in BMP, encoded with Latin-1
        given = "\u6f22\u5b57"
        self.assertRaises(UnicodeEncodeError, urllib.parse.quote, given,
                                    encoding="latin-1")
        # Characters in BMP, encoded with Latin-1, with replace error handling
        given = "\u6f22\u5b57"
        expect = "%3F%3F"                   # "??"
        result = urllib.parse.quote(given, encoding="latin-1",
                                    errors="replace")
        self.assertEqual(expect, result,
                         "using quote(): %r != %r" % (expect, result))
        # Characters in BMP, Latin-1, with xmlcharref error handling
        given = "\u6f22\u5b57"
        expect = "%26%2328450%3B%26%2323383%3B"     # "&#28450;&#23383;"
        result = urllib.parse.quote(given, encoding="latin-1",
                                    errors="xmlcharrefreplace")
        self.assertEqual(expect, result,
                         "using quote(): %r != %r" % (expect, result))

    def test_quote_plus_with_unicode(self):
        # Encoding (latin-1) test for quote_plus
        given = "\xa2\xd8 \xff"
        expect = "%A2%D8+%FF"
        result = urllib.parse.quote_plus(given, encoding="latin-1")
        self.assertEqual(expect, result,
                         "using quote_plus(): %r != %r" % (expect, result))
        # Errors test for quote_plus
        given = "ab\u6f22\u5b57 cd"
        expect = "ab%3F%3F+cd"
        result = urllib.parse.quote_plus(given, encoding="latin-1",
                                         errors="replace")
        self.assertEqual(expect, result,
                         "using quote_plus(): %r != %r" % (expect, result))


class UnquotingTests(unittest.TestCase):
    """Tests for unquote() and unquote_plus()

    See the doc string for quoting_Tests for details on quoting and such.

    """

    def test_unquoting(self):
        # Make sure unquoting of all ASCII values works
        escape_list = []
        for num in range(128):
            given = hexescape(chr(num))
            expect = chr(num)
            result = urllib.parse.unquote(given)
            self.assertEqual(expect, result,
                             "using unquote(): %r != %r" % (expect, result))
            result = urllib.parse.unquote_plus(given)
            self.assertEqual(expect, result,
                             "using unquote_plus(): %r != %r" %
                             (expect, result))
            escape_list.append(given)
        escape_string = ''.join(escape_list)
        del escape_list
        result = urllib.parse.unquote(escape_string)
        self.assertEqual(result.count('%'), 1,
                         "using unquote(): not all characters escaped: "
                         "%s" % result)

    def test_unquote_rejects_none_and_tuple(self):
        self.assertRaises((TypeError, AttributeError), urllib.parse.unquote, None)
        self.assertRaises((TypeError, AttributeError), urllib.parse.unquote, ())

    def test_unquoting_badpercent(self):
        # Test unquoting on bad percent-escapes
        given = '%xab'
        expect = given
        result = urllib.parse.unquote(given)
        self.assertEqual(expect, result, "using unquote(): %r != %r"
                         % (expect, result))
        given = '%x'
        expect = given
        result = urllib.parse.unquote(given)
        self.assertEqual(expect, result, "using unquote(): %r != %r"
                         % (expect, result))
        given = '%'
        expect = given
        result = urllib.parse.unquote(given)
        self.assertEqual(expect, result, "using unquote(): %r != %r"
                         % (expect, result))
        # unquote_to_bytes
        given = '%xab'
        expect = bytes(given, 'ascii')
        result = urllib.parse.unquote_to_bytes(given)
        self.assertEqual(expect, result, "using unquote_to_bytes(): %r != %r"
                         % (expect, result))
        given = '%x'
        expect = bytes(given, 'ascii')
        result = urllib.parse.unquote_to_bytes(given)
        self.assertEqual(expect, result, "using unquote_to_bytes(): %r != %r"
                         % (expect, result))
        given = '%'
        expect = bytes(given, 'ascii')
        result = urllib.parse.unquote_to_bytes(given)
        self.assertEqual(expect, result, "using unquote_to_bytes(): %r != %r"
                         % (expect, result))
        self.assertRaises((TypeError, AttributeError), urllib.parse.unquote_to_bytes, None)
        self.assertRaises((TypeError, AttributeError), urllib.parse.unquote_to_bytes, ())

    def test_unquoting_mixed_case(self):
        # Test unquoting on mixed-case hex digits in the percent-escapes
        given = '%Ab%eA'
        expect = b'\xab\xea'
        result = urllib.parse.unquote_to_bytes(given)
        self.assertEqual(expect, result,
                         "using unquote_to_bytes(): %r != %r"
                         % (expect, result))

    def test_unquoting_parts(self):
        # Make sure unquoting works when have non-quoted characters
        # interspersed
        given = 'ab%sd' % hexescape('c')
        expect = "abcd"
        result = urllib.parse.unquote(given)
        self.assertEqual(expect, result,
                         "using quote(): %r != %r" % (expect, result))
        result = urllib.parse.unquote_plus(given)
        self.assertEqual(expect, result,
                         "using unquote_plus(): %r != %r" % (expect, result))

    def test_unquoting_plus(self):
        # Test difference between unquote() and unquote_plus()
        given = "are+there+spaces..."
        expect = given
        result = urllib.parse.unquote(given)
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))
        expect = given.replace('+', ' ')
        result = urllib.parse.unquote_plus(given)
        self.assertEqual(expect, result,
                         "using unquote_plus(): %r != %r" % (expect, result))

    def test_unquote_to_bytes(self):
        given = 'br%C3%BCckner_sapporo_20050930.doc'
        expect = b'br\xc3\xbcckner_sapporo_20050930.doc'
        result = urllib.parse.unquote_to_bytes(given)
        self.assertEqual(expect, result,
                         "using unquote_to_bytes(): %r != %r"
                         % (expect, result))
        # Test on a string with unescaped non-ASCII characters
        # (Technically an invalid URI; expect those characters to be UTF-8
        # encoded).
        result = urllib.parse.unquote_to_bytes("\u6f22%C3%BC")
        expect = b'\xe6\xbc\xa2\xc3\xbc'    # UTF-8 for "\u6f22\u00fc"
        self.assertEqual(expect, result,
                         "using unquote_to_bytes(): %r != %r"
                         % (expect, result))
        # Test with a bytes as input
        given = b'%A2%D8ab%FF'
        expect = b'\xa2\xd8ab\xff'
        result = urllib.parse.unquote_to_bytes(given)
        self.assertEqual(expect, result,
                         "using unquote_to_bytes(): %r != %r"
                         % (expect, result))
        # Test with a bytes as input, with unescaped non-ASCII bytes
        # (Technically an invalid URI; expect those bytes to be preserved)
        given = b'%A2\xd8ab%FF'
        expect = b'\xa2\xd8ab\xff'
        result = urllib.parse.unquote_to_bytes(given)
        self.assertEqual(expect, result,
                         "using unquote_to_bytes(): %r != %r"
                         % (expect, result))

    def test_unquote_with_unicode(self):
        # Characters in the Latin-1 range, encoded with UTF-8
        given = 'br%C3%BCckner_sapporo_20050930.doc'
        expect = 'br\u00fcckner_sapporo_20050930.doc'
        result = urllib.parse.unquote(given)
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))
        # Characters in the Latin-1 range, encoded with None (default)
        result = urllib.parse.unquote(given, encoding=None, errors=None)
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))

        # Characters in the Latin-1 range, encoded with Latin-1
        result = urllib.parse.unquote('br%FCckner_sapporo_20050930.doc',
                                      encoding="latin-1")
        expect = 'br\u00fcckner_sapporo_20050930.doc'
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))

        # Characters in BMP, encoded with UTF-8
        given = "%E6%BC%A2%E5%AD%97"
        expect = "\u6f22\u5b57"             # "Kanji"
        result = urllib.parse.unquote(given)
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))

        # Decode with UTF-8, invalid sequence
        given = "%F3%B1"
        expect = "\ufffd"                   # Replacement character
        result = urllib.parse.unquote(given)
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))

        # Decode with UTF-8, invalid sequence, replace errors
        result = urllib.parse.unquote(given, errors="replace")
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))

        # Decode with UTF-8, invalid sequence, ignoring errors
        given = "%F3%B1"
        expect = ""
        result = urllib.parse.unquote(given, errors="ignore")
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))

        # A mix of non-ASCII and percent-encoded characters, UTF-8
        result = urllib.parse.unquote("\u6f22%C3%BC")
        expect = '\u6f22\u00fc'
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))

        # A mix of non-ASCII and percent-encoded characters, Latin-1
        # (Note, the string contains non-Latin-1-representable characters)
        result = urllib.parse.unquote("\u6f22%FC", encoding="latin-1")
        expect = '\u6f22\u00fc'
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))

    def test_unquoting_with_bytes_input(self):
        # ASCII characters decoded to a string
        given = b'blueberryjam'
        expect = 'blueberryjam'
        result = urllib.parse.unquote(given)
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))

        # A mix of non-ASCII hex-encoded characters and ASCII characters
        given = b'bl\xc3\xa5b\xc3\xa6rsyltet\xc3\xb8y'
        expect = 'bl\u00e5b\u00e6rsyltet\u00f8y'
        result = urllib.parse.unquote(given)
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))

        # A mix of non-ASCII percent-encoded characters and ASCII characters
        given = b'bl%c3%a5b%c3%a6rsyltet%c3%b8j'
        expect = 'bl\u00e5b\u00e6rsyltet\u00f8j'
        result = urllib.parse.unquote(given)
        self.assertEqual(expect, result,
                         "using unquote(): %r != %r" % (expect, result))


class urlencode_Tests(unittest.TestCase):
    """Tests for urlencode()"""

    def help_inputtype(self, given, test_type):
        """Helper method for testing different input types.

        'given' must lead to only the pairs:
            * 1st, 1
            * 2nd, 2
            * 3rd, 3

        Test cannot assume anything about order.  Docs make no guarantee and
        have possible dictionary input.

        """
        expect_somewhere = ["1st=1", "2nd=2", "3rd=3"]
        result = urllib.parse.urlencode(given)
        for expected in expect_somewhere:
            self.assertIn(expected, result,
                         "testing %s: %s not found in %s" %
                         (test_type, expected, result))
        self.assertEqual(result.count('&'), 2,
                         "testing %s: expected 2 '&'s; got %s" %
                         (test_type, result.count('&')))
        amp_location = result.index('&')
        on_amp_left = result[amp_location - 1]
        on_amp_right = result[amp_location + 1]
        self.assertTrue(on_amp_left.isdigit() and on_amp_right.isdigit(),
                     "testing %s: '&' not located in proper place in %s" %
                     (test_type, result))
        self.assertEqual(len(result), (5 * 3) + 2, #5 chars per thing and amps
                         "testing %s: "
                         "unexpected number of characters: %s != %s" %
                         (test_type, len(result), (5 * 3) + 2))

    def test_using_mapping(self):
        # Test passing in a mapping object as an argument.
        self.help_inputtype({"1st":'1', "2nd":'2', "3rd":'3'},
                            "using dict as input type")

    def test_using_sequence(self):
        # Test passing in a sequence of two-item sequences as an argument.
        self.help_inputtype([('1st', '1'), ('2nd', '2'), ('3rd', '3')],
                            "using sequence of two-item tuples as input")

    def test_quoting(self):
        # Make sure keys and values are quoted using quote_plus()
        given = {"&":"="}
        expect = "%s=%s" % (hexescape('&'), hexescape('='))
        result = urllib.parse.urlencode(given)
        self.assertEqual(expect, result)
        given = {"key name":"A bunch of pluses"}
        expect = "key+name=A+bunch+of+pluses"
        result = urllib.parse.urlencode(given)
        self.assertEqual(expect, result)

    def test_doseq(self):
        # Test that passing True for 'doseq' parameter works correctly
        given = {'sequence':['1', '2', '3']}
        expect = "sequence=%s" % urllib.parse.quote_plus(str(['1', '2', '3']))
        result = urllib.parse.urlencode(given)
        self.assertEqual(expect, result)
        result = urllib.parse.urlencode(given, True)
        for value in given["sequence"]:
            expect = "sequence=%s" % value
            self.assertIn(expect, result)
        self.assertEqual(result.count('&'), 2,
                         "Expected 2 '&'s, got %s" % result.count('&'))

    def test_empty_sequence(self):
        self.assertEqual("", urllib.parse.urlencode({}))
        self.assertEqual("", urllib.parse.urlencode([]))

    def test_nonstring_values(self):
        self.assertEqual("a=1", urllib.parse.urlencode({"a": 1}))
        self.assertEqual("a=None", urllib.parse.urlencode({"a": None}))

    def test_nonstring_seq_values(self):
        self.assertEqual("a=1&a=2", urllib.parse.urlencode({"a": [1, 2]}, True))
        self.assertEqual("a=None&a=a",
                         urllib.parse.urlencode({"a": [None, "a"]}, True))
        data = collections.OrderedDict([("a", 1), ("b", 1)])
        self.assertEqual("a=a&a=b",
                         urllib.parse.urlencode({"a": data}, True))

    def test_urlencode_encoding(self):
        # ASCII encoding. Expect %3F with errors="replace'
        given = (('\u00a0', '\u00c1'),)
        expect = '%3F=%3F'
        result = urllib.parse.urlencode(given, encoding="ASCII", errors="replace")
        self.assertEqual(expect, result)

        # Default is UTF-8 encoding.
        given = (('\u00a0', '\u00c1'),)
        expect = '%C2%A0=%C3%81'
        result = urllib.parse.urlencode(given)
        self.assertEqual(expect, result)

        # Latin-1 encoding.
        given = (('\u00a0', '\u00c1'),)
        expect = '%A0=%C1'
        result = urllib.parse.urlencode(given, encoding="latin-1")
        self.assertEqual(expect, result)

    def test_urlencode_encoding_doseq(self):
        # ASCII Encoding. Expect %3F with errors="replace'
        given = (('\u00a0', '\u00c1'),)
        expect = '%3F=%3F'
        result = urllib.parse.urlencode(given, doseq=True,
                                        encoding="ASCII", errors="replace")
        self.assertEqual(expect, result)

        # ASCII Encoding. On a sequence of values.
        given = (("\u00a0", (1, "\u00c1")),)
        expect = '%3F=1&%3F=%3F'
        result = urllib.parse.urlencode(given, True,
                                        encoding="ASCII", errors="replace")
        self.assertEqual(expect, result)

        # Utf-8
        given = (("\u00a0", "\u00c1"),)
        expect = '%C2%A0=%C3%81'
        result = urllib.parse.urlencode(given, True)
        self.assertEqual(expect, result)

        given = (("\u00a0", (42, "\u00c1")),)
        expect = '%C2%A0=42&%C2%A0=%C3%81'
        result = urllib.parse.urlencode(given, True)
        self.assertEqual(expect, result)

        # latin-1
        given = (("\u00a0", "\u00c1"),)
        expect = '%A0=%C1'
        result = urllib.parse.urlencode(given, True, encoding="latin-1")
        self.assertEqual(expect, result)

        given = (("\u00a0", (42, "\u00c1")),)
        expect = '%A0=42&%A0=%C1'
        result = urllib.parse.urlencode(given, True, encoding="latin-1")
        self.assertEqual(expect, result)

    def test_urlencode_bytes(self):
        given = ((b'\xa0\x24', b'\xc1\x24'),)
        expect = '%A0%24=%C1%24'
        result = urllib.parse.urlencode(given)
        self.assertEqual(expect, result)
        result = urllib.parse.urlencode(given, True)
        self.assertEqual(expect, result)

        # Sequence of values
        given = ((b'\xa0\x24', (42, b'\xc1\x24')),)
        expect = '%A0%24=42&%A0%24=%C1%24'
        result = urllib.parse.urlencode(given, True)
        self.assertEqual(expect, result)

    def test_urlencode_encoding_safe_parameter(self):

        # Send '$' (\x24) as safe character
        # Default utf-8 encoding

        given = ((b'\xa0\x24', b'\xc1\x24'),)
        result = urllib.parse.urlencode(given, safe=":$")
        expect = '%A0$=%C1$'
        self.assertEqual(expect, result)

        given = ((b'\xa0\x24', b'\xc1\x24'),)
        result = urllib.parse.urlencode(given, doseq=True, safe=":$")
        expect = '%A0$=%C1$'
        self.assertEqual(expect, result)

        # Safe parameter in sequence
        given = ((b'\xa0\x24', (b'\xc1\x24', 0xd, 42)),)
        expect = '%A0$=%C1$&%A0$=13&%A0$=42'
        result = urllib.parse.urlencode(given, True, safe=":$")
        self.assertEqual(expect, result)

        # Test all above in latin-1 encoding

        given = ((b'\xa0\x24', b'\xc1\x24'),)
        result = urllib.parse.urlencode(given, safe=":$",
                                        encoding="latin-1")
        expect = '%A0$=%C1$'
        self.assertEqual(expect, result)

        given = ((b'\xa0\x24', b'\xc1\x24'),)
        expect = '%A0$=%C1$'
        result = urllib.parse.urlencode(given, doseq=True, safe=":$",
                                        encoding="latin-1")

        given = ((b'\xa0\x24', (b'\xc1\x24', 0xd, 42)),)
        expect = '%A0$=%C1$&%A0$=13&%A0$=42'
        result = urllib.parse.urlencode(given, True, safe=":$",
                                        encoding="latin-1")
        self.assertEqual(expect, result)

class Pathname_Tests(unittest.TestCase):
    """Test pathname2url() and url2pathname()"""

    def test_basic(self):
        # Make sure simple tests pass
        expected_path = os.path.join("parts", "of", "a", "path")
        expected_url = "parts/of/a/path"
        result = urllib.request.pathname2url(expected_path)
        self.assertEqual(expected_url, result,
                         "pathname2url() failed; %s != %s" %
                         (result, expected_url))
        result = urllib.request.url2pathname(expected_url)
        self.assertEqual(expected_path, result,
                         "url2pathame() failed; %s != %s" %
                         (result, expected_path))

    def test_quoting(self):
        # Test automatic quoting and unquoting works for pathnam2url() and
        # url2pathname() respectively
        given = os.path.join("needs", "quot=ing", "here")
        expect = "needs/%s/here" % urllib.parse.quote("quot=ing")
        result = urllib.request.pathname2url(given)
        self.assertEqual(expect, result,
                         "pathname2url() failed; %s != %s" %
                         (expect, result))
        expect = given
        result = urllib.request.url2pathname(result)
        self.assertEqual(expect, result,
                         "url2pathname() failed; %s != %s" %
                         (expect, result))
        given = os.path.join("make sure", "using_quote")
        expect = "%s/using_quote" % urllib.parse.quote("make sure")
        result = urllib.request.pathname2url(given)
        self.assertEqual(expect, result,
                         "pathname2url() failed; %s != %s" %
                         (expect, result))
        given = "make+sure/using_unquote"
        expect = os.path.join("make+sure", "using_unquote")
        result = urllib.request.url2pathname(given)
        self.assertEqual(expect, result,
                         "url2pathname() failed; %s != %s" %
                         (expect, result))

    def test_pathname2url(self):
        # Test cases common to Windows and POSIX.
        fn = urllib.request.pathname2url
        sep = os.path.sep
        self.assertEqual(fn(''), '')
        self.assertEqual(fn(sep), '///')
        self.assertEqual(fn('a'), 'a')
        self.assertEqual(fn(f'a{sep}b.c'), 'a/b.c')
        self.assertEqual(fn(f'{sep}a{sep}b.c'), '///a/b.c')
        self.assertEqual(fn(f'{sep}a{sep}b%#c'), '///a/b%25%23c')

    def test_pathname2url_add_scheme(self):
        sep = os.path.sep
        subtests = [
            ('', 'file:'),
            (sep, 'file:///'),
            ('a', 'file:a'),
            (f'a{sep}b.c', 'file:a/b.c'),
            (f'{sep}a{sep}b.c', 'file:///a/b.c'),
            (f'{sep}a{sep}b%#c', 'file:///a/b%25%23c'),
        ]
        for path, expected_url in subtests:
            with self.subTest(path=path):
                self.assertEqual(
                    urllib.request.pathname2url(path, add_scheme=True), expected_url)

    @unittest.skipUnless(sys.platform == 'win32',
                         'test specific to Windows pathnames.')
    def test_pathname2url_win(self):
        # Test special prefixes are correctly handled in pathname2url()
        fn = urllib.request.pathname2url
        self.assertEqual(fn('\\\\?\\C:\\dir'), '///C:/dir')
        self.assertEqual(fn('\\\\?\\unc\\server\\share\\dir'), '//server/share/dir')
        self.assertEqual(fn("C:"), '///C:')
        self.assertEqual(fn("C:\\"), '///C:/')
        self.assertEqual(fn('c:\\a\\b.c'), '///c:/a/b.c')
        self.assertEqual(fn('C:\\a\\b.c'), '///C:/a/b.c')
        self.assertEqual(fn('C:\\a\\b.c\\'), '///C:/a/b.c/')
        self.assertEqual(fn('C:\\a\\\\b.c'), '///C:/a//b.c')
        self.assertEqual(fn('C:\\a\\b%#c'), '///C:/a/b%25%23c')
        self.assertEqual(fn('C:\\a\\b\xe9'), '///C:/a/b%C3%A9')
        self.assertEqual(fn('C:\\foo\\bar\\spam.foo'), "///C:/foo/bar/spam.foo")
        # NTFS alternate data streams
        self.assertEqual(fn('C:\\foo:bar'), '///C:/foo%3Abar')
        self.assertEqual(fn('foo:bar'), 'foo%3Abar')
        # No drive letter
        self.assertEqual(fn("\\folder\\test\\"), '///folder/test/')
        self.assertEqual(fn("\\\\folder\\test\\"), '//folder/test/')
        self.assertEqual(fn("\\\\\\folder\\test\\"), '///folder/test/')
        self.assertEqual(fn('\\\\some\\share\\'), '//some/share/')
        self.assertEqual(fn('\\\\some\\share\\a\\b.c'), '//some/share/a/b.c')
        self.assertEqual(fn('\\\\some\\share\\a\\b%#c\xe9'), '//some/share/a/b%25%23c%C3%A9')
        # Alternate path separator
        self.assertEqual(fn('C:/a/b.c'), '///C:/a/b.c')
        self.assertEqual(fn('//some/share/a/b.c'), '//some/share/a/b.c')
        self.assertEqual(fn('//?/C:/dir'), '///C:/dir')
        self.assertEqual(fn('//?/unc/server/share/dir'), '//server/share/dir')
        # Round-tripping
        urls = ['///C:',
                '///folder/test/',
                '///C:/foo/bar/spam.foo']
        for url in urls:
            self.assertEqual(fn(urllib.request.url2pathname(url)), url)

    @unittest.skipIf(sys.platform == 'win32',
                     'test specific to POSIX pathnames')
    def test_pathname2url_posix(self):
        fn = urllib.request.pathname2url
        self.assertEqual(fn('//a/b.c'), '////a/b.c')
        self.assertEqual(fn('///a/b.c'), '/////a/b.c')
        self.assertEqual(fn('////a/b.c'), '//////a/b.c')

    @unittest.skipUnless(os_helper.FS_NONASCII, 'need os_helper.FS_NONASCII')
    def test_pathname2url_nonascii(self):
        encoding = sys.getfilesystemencoding()
        errors = sys.getfilesystemencodeerrors()
        url = urllib.parse.quote(os_helper.FS_NONASCII, encoding=encoding, errors=errors)
        self.assertEqual(urllib.request.pathname2url(os_helper.FS_NONASCII), url)

    def test_url2pathname(self):
        # Test cases common to Windows and POSIX.
        fn = urllib.request.url2pathname
        sep = os.path.sep
        self.assertEqual(fn(''), '')
        self.assertEqual(fn('/'), f'{sep}')
        self.assertEqual(fn('///'), f'{sep}')
        self.assertEqual(fn('////'), f'{sep}{sep}')
        self.assertEqual(fn('foo'), 'foo')
        self.assertEqual(fn('foo/bar'), f'foo{sep}bar')
        self.assertEqual(fn('/foo/bar'), f'{sep}foo{sep}bar')
        self.assertEqual(fn('//localhost/foo/bar'), f'{sep}foo{sep}bar')
        self.assertEqual(fn('///foo/bar'), f'{sep}foo{sep}bar')
        self.assertEqual(fn('////foo/bar'), f'{sep}{sep}foo{sep}bar')
        self.assertEqual(fn('data:blah'), 'data:blah')
        self.assertEqual(fn('data://blah'), f'data:{sep}{sep}blah')
        self.assertEqual(fn('foo?bar'), 'foo')
        self.assertEqual(fn('foo#bar'), 'foo')
        self.assertEqual(fn('foo?bar=baz'), 'foo')
        self.assertEqual(fn('foo?bar#baz'), 'foo')
        self.assertEqual(fn('foo%3Fbar'), 'foo?bar')
        self.assertEqual(fn('foo%23bar'), 'foo#bar')
        self.assertEqual(fn('foo%3Fbar%3Dbaz'), 'foo?bar=baz')
        self.assertEqual(fn('foo%3Fbar%23baz'), 'foo?bar#baz')

    def test_url2pathname_require_scheme(self):
        sep = os.path.sep
        subtests = [
            ('file:', ''),
            ('FILE:', ''),
            ('FiLe:', ''),
            ('file:/', f'{sep}'),
            ('file:///', f'{sep}'),
            ('file:////', f'{sep}{sep}'),
            ('file:foo', 'foo'),
            ('file:foo/bar', f'foo{sep}bar'),
            ('file:/foo/bar', f'{sep}foo{sep}bar'),
            ('file://localhost/foo/bar', f'{sep}foo{sep}bar'),
            ('file:///foo/bar', f'{sep}foo{sep}bar'),
            ('file:////foo/bar', f'{sep}{sep}foo{sep}bar'),
            ('file:data:blah', 'data:blah'),
            ('file:data://blah', f'data:{sep}{sep}blah'),
        ]
        for url, expected_path in subtests:
            with self.subTest(url=url):
                self.assertEqual(
                    urllib.request.url2pathname(url, require_scheme=True),
                    expected_path)

    def test_url2pathname_require_scheme_errors(self):
        subtests = [
            '',
            ':',
            'foo',
            'http:foo',
            'localfile:foo',
            'data:foo',
            'data:file:foo',
            'data:file://foo',
        ]
        for url in subtests:
            with self.subTest(url=url):
                self.assertRaises(
                    urllib.error.URLError,
                    urllib.request.url2pathname,
                    url, require_scheme=True)

    @unittest.skipIf(support.is_emscripten, "Fixed by https://github.com/emscripten-core/emscripten/pull/24593")
    def test_url2pathname_resolve_host(self):
        fn = urllib.request.url2pathname
        sep = os.path.sep
        self.assertEqual(fn('//127.0.0.1/foo/bar', resolve_host=True), f'{sep}foo{sep}bar')
        self.assertEqual(fn(f'//{socket.gethostname()}/foo/bar'), f'{sep}foo{sep}bar')
        self.assertEqual(fn(f'//{socket.gethostname()}/foo/bar', resolve_host=True), f'{sep}foo{sep}bar')

    @unittest.skipUnless(sys.platform == 'win32',
                         'test specific to Windows pathnames.')
    def test_url2pathname_win(self):
        fn = urllib.request.url2pathname
        self.assertEqual(fn('/C:/'), 'C:\\')
        self.assertEqual(fn('//C:'), 'C:')
        self.assertEqual(fn('//C:/'), 'C:\\')
        self.assertEqual(fn('//C:\\'), 'C:\\')
        self.assertEqual(fn('//C:80/'), 'C:80\\')
        self.assertEqual(fn("///C|"), 'C:')
        self.assertEqual(fn("///C:"), 'C:')
        self.assertEqual(fn('///C:/'), 'C:\\')
        self.assertEqual(fn('/C|//'), 'C:\\\\')
        self.assertEqual(fn('///C|/path'), 'C:\\path')
        # No DOS drive
        self.assertEqual(fn("///C/test/"), '\\C\\test\\')
        self.assertEqual(fn("////C/test/"), '\\\\C\\test\\')
        # DOS drive paths
        self.assertEqual(fn('c:/path/to/file'), 'c:\\path\\to\\file')
        self.assertEqual(fn('C:/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('C:/path/to/file/'), 'C:\\path\\to\\file\\')
        self.assertEqual(fn('C:/path/to//file'), 'C:\\path\\to\\\\file')
        self.assertEqual(fn('C|/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('/C|/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('///C|/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn("///C|/foo/bar/spam.foo"), 'C:\\foo\\bar\\spam.foo')
        # Colons in URI
        self.assertEqual(fn('///\u00e8|/'), '\u00e8:\\')
        self.assertEqual(fn('//host/share/spam.txt:eggs'), '\\\\host\\share\\spam.txt:eggs')
        self.assertEqual(fn('///c:/spam.txt:eggs'), 'c:\\spam.txt:eggs')
        # UNC paths
        self.assertEqual(fn('//server/path/to/file'), '\\\\server\\path\\to\\file')
        self.assertEqual(fn('////server/path/to/file'), '\\\\server\\path\\to\\file')
        self.assertEqual(fn('/////server/path/to/file'), '\\\\server\\path\\to\\file')
        self.assertEqual(fn('//127.0.0.1/path/to/file'), '\\\\127.0.0.1\\path\\to\\file')
        # Localhost paths
        self.assertEqual(fn('//localhost/C:/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('//localhost/C|/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('//localhost/path/to/file'), '\\path\\to\\file')
        self.assertEqual(fn('//localhost//server/path/to/file'), '\\\\server\\path\\to\\file')
        # Percent-encoded forward slashes are preserved for backwards compatibility
        self.assertEqual(fn('C:/foo%2fbar'), 'C:\\foo/bar')
        self.assertEqual(fn('//server/share/foo%2fbar'), '\\\\server\\share\\foo/bar')
        # Round-tripping
        paths = ['C:',
                 r'\C\test\\',
                 r'C:\foo\bar\spam.foo']
        for path in paths:
            self.assertEqual(fn(urllib.request.pathname2url(path)), path)

    @unittest.skipIf(sys.platform == 'win32',
                     'test specific to POSIX pathnames')
    def test_url2pathname_posix(self):
        fn = urllib.request.url2pathname
        self.assertRaises(urllib.error.URLError, fn, '//foo/bar')
        self.assertRaises(urllib.error.URLError, fn, '//localhost:/foo/bar')
        self.assertRaises(urllib.error.URLError, fn, '//:80/foo/bar')
        self.assertRaises(urllib.error.URLError, fn, '//:/foo/bar')
        self.assertRaises(urllib.error.URLError, fn, '//c:80/foo/bar')
        self.assertRaises(urllib.error.URLError, fn, '//127.0.0.1/foo/bar')

    @unittest.skipUnless(os_helper.FS_NONASCII, 'need os_helper.FS_NONASCII')
    def test_url2pathname_nonascii(self):
        encoding = sys.getfilesystemencoding()
        errors = sys.getfilesystemencodeerrors()
        url = os_helper.FS_NONASCII
        self.assertEqual(urllib.request.url2pathname(url), os_helper.FS_NONASCII)
        url = urllib.parse.quote(url, encoding=encoding, errors=errors)
        self.assertEqual(urllib.request.url2pathname(url), os_helper.FS_NONASCII)

class Utility_Tests(unittest.TestCase):
    """Testcase to test the various utility functions in the urllib."""

    def test_thishost(self):
        """Test the urllib.request.thishost utility function returns a tuple"""
        self.assertIsInstance(urllib.request.thishost(), tuple)


class RequestTests(unittest.TestCase):
    """Unit tests for urllib.request.Request."""

    def test_default_values(self):
        Request = urllib.request.Request
        request = Request("http://www.python.org")
        self.assertEqual(request.get_method(), 'GET')
        request = Request("http://www.python.org", {})
        self.assertEqual(request.get_method(), 'POST')

    def test_with_method_arg(self):
        Request = urllib.request.Request
        request = Request("http://www.python.org", method='HEAD')
        self.assertEqual(request.method, 'HEAD')
        self.assertEqual(request.get_method(), 'HEAD')
        request = Request("http://www.python.org", {}, method='HEAD')
        self.assertEqual(request.method, 'HEAD')
        self.assertEqual(request.get_method(), 'HEAD')
        request = Request("http://www.python.org", method='GET')
        self.assertEqual(request.get_method(), 'GET')
        request.method = 'HEAD'
        self.assertEqual(request.get_method(), 'HEAD')


if __name__ == '__main__':
    unittest.main()
