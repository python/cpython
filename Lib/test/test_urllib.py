"""Regresssion tests for urllib"""

import urllib.parse
import urllib.request
import urllib.error
import http.client
import email.message
import io
import unittest
from test import support
import os
import sys
import tempfile

from base64 import b64encode
import collections

def hexescape(char):
    """Escape char as RFC 2396 specifies"""
    hex_repr = hex(ord(char))[2:].upper()
    if len(hex_repr) == 1:
        hex_repr = "0%s" % hex_repr
    return "%" + hex_repr

# Shortcut for testing FancyURLopener
_urlopener = None
def urlopen(url, data=None, proxies=None):
    """urlopen(url [, data]) -> open file-like object"""
    global _urlopener
    if proxies is not None:
        opener = urllib.request.FancyURLopener(proxies=proxies)
    elif not _urlopener:
        opener = urllib.request.FancyURLopener()
        _urlopener = opener
    else:
        opener = _urlopener
    if data is None:
        return opener.open(url)
    else:
        return opener.open(url, data)


class FakeHTTPMixin(object):
    def fakehttp(self, fakedata):
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
                self.sock = FakeSocket(fakedata)

        self._connection_class = http.client.HTTPConnection
        http.client.HTTPConnection = FakeHTTPConnection

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
        f = open(support.TESTFN, 'wb')
        try:
            f.write(self.text)
        finally:
            f.close()
        self.pathname = support.TESTFN
        self.returned_obj = urlopen("file:%s" % self.pathname)

    def tearDown(self):
        """Shut down the open object"""
        self.returned_obj.close()
        os.remove(support.TESTFN)

    def test_interface(self):
        # Make sure object returned by urlopen() has the specified methods
        for attr in ("read", "readline", "readlines", "fileno",
                     "close", "info", "geturl", "getcode", "__iter__"):
            self.assertTrue(hasattr(self.returned_obj, attr),
                         "object returned by urlopen() lacks %s attribute" %
                         attr)

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

    def test_info(self):
        self.assertIsInstance(self.returned_obj.info(), email.message.Message)

    def test_geturl(self):
        self.assertEqual(self.returned_obj.geturl(), self.pathname)

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

class ProxyTests(unittest.TestCase):

    def setUp(self):
        # Records changes to env vars
        self.env = support.EnvironmentVarGuard()
        # Delete all proxy related env vars
        for k in list(os.environ):
            if 'proxy' in k.lower():
                self.env.unset(k)

    def tearDown(self):
        # Restore all proxy related env vars
        self.env.__exit__()
        del self.env

    def test_getproxies_environment_keep_no_proxies(self):
        self.env.set('NO_PROXY', 'localhost')
        proxies = urllib.request.getproxies_environment()
        # getproxies_environment use lowered case truncated (no '_proxy') keys
        self.assertEqual('localhost', proxies['no'])
        # List of no_proxies with space.
        self.env.set('NO_PROXY', 'localhost, anotherdomain.com, newdomain.com')
        self.assertTrue(urllib.request.proxy_bypass_environment('anotherdomain.com'))

class urlopen_HttpTests(unittest.TestCase, FakeHTTPMixin):
    """Test urlopen() opening a fake http connection."""

    def check_read(self, ver):
        self.fakehttp(b"HTTP/" + ver + b" 200 OK\r\n\r\nHello!")
        try:
            fp = urlopen("http://python.org/")
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
            resp = urlopen("http://www.python.org")
            self.assertTrue(resp.fp.will_close)
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
        # urlopen() should raise IOError for many error codes.
        self.fakehttp(b'''HTTP/1.1 401 Authentication Required
Date: Wed, 02 Jan 2008 03:03:54 GMT
Server: Apache/1.3.33 (Debian GNU/Linux) mod_ssl/2.8.22 OpenSSL/0.9.7e
Connection: close
Content-Type: text/html; charset=iso-8859-1
''')
        try:
            self.assertRaises(IOError, urlopen, "http://python.org/")
        finally:
            self.unfakehttp()

    def test_invalid_redirect(self):
        # urlopen() should raise IOError for many error codes.
        self.fakehttp(b'''HTTP/1.1 302 Found
Date: Wed, 02 Jan 2008 03:03:54 GMT
Server: Apache/1.3.33 (Debian GNU/Linux) mod_ssl/2.8.22 OpenSSL/0.9.7e
Location: file://guidocomputer.athome.com:/python/license
Connection: close
Content-Type: text/html; charset=iso-8859-1
''')
        try:
            self.assertRaises(urllib.error.HTTPError, urlopen,
                              "http://python.org/")
        finally:
            self.unfakehttp()

    def test_empty_socket(self):
        # urlopen() raises IOError if the underlying socket does not send any
        # data. (#1680230)
        self.fakehttp(b'')
        try:
            self.assertRaises(IOError, urlopen, "http://something")
        finally:
            self.unfakehttp()

    def test_userpass_inurl(self):
        self.fakehttp(b"HTTP/1.0 200 OK\r\n\r\nHello!")
        try:
            fp = urlopen("http://user:pass@python.org/")
            self.assertEqual(fp.readline(), b"Hello!")
            self.assertEqual(fp.readline(), b"")
            self.assertEqual(fp.geturl(), 'http://user:pass@python.org/')
            self.assertEqual(fp.getcode(), 200)
        finally:
            self.unfakehttp()

    def test_userpass_inurl_w_spaces(self):
        self.fakehttp(b"HTTP/1.0 200 OK\r\n\r\nHello!")
        try:
            userpass = "a b:c d"
            url = "http://{}@python.org/".format(userpass)
            fakehttp_wrapper = http.client.HTTPConnection
            authorization = ("Authorization: Basic %s\r\n" %
                             b64encode(userpass.encode("ASCII")).decode("ASCII"))
            fp = urlopen(url)
            # The authorization header must be in place
            self.assertIn(authorization, fakehttp_wrapper.buf.decode("UTF-8"))
            self.assertEqual(fp.readline(), b"Hello!")
            self.assertEqual(fp.readline(), b"")
            # the spaces are quoted in URL so no match
            self.assertNotEqual(fp.geturl(), url)
            self.assertEqual(fp.getcode(), 200)
        finally:
            self.unfakehttp()

    def test_URLopener_deprecation(self):
        with support.check_warnings(('',DeprecationWarning)):
            warn = urllib.request.URLopener()

class urlretrieve_FileTests(unittest.TestCase):
    """Test urllib.urlretrieve() on local files"""

    def setUp(self):
        # Create a list of temporary files. Each item in the list is a file
        # name (absolute path or relative to the current working directory).
        # All files in this list will be deleted in the tearDown method. Note,
        # this only helps to makes sure temporary files get deleted, but it
        # does nothing about trying to close files that may still be open. It
        # is the responsibility of the developer to properly close files even
        # when exceptional conditions occur.
        self.tempFiles = []

        # Create a temporary file.
        self.registerFileForCleanUp(support.TESTFN)
        self.text = b'testing urllib.urlretrieve'
        try:
            FILE = open(support.TESTFN, 'wb')
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
        try:
            filePath.encode("utf-8")
        except UnicodeEncodeError:
            raise unittest.SkipTest("filePath is not encodable to utf8")
        return "file://%s" % urllib.request.pathname2url(filePath)

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
        result = urllib.request.urlretrieve("file:%s" % support.TESTFN)
        self.assertEqual(result[0], support.TESTFN)
        self.assertIsInstance(result[1], email.message.Message,
                              "did not get a email.message.Message instance "
                              "as second returned value")

    def test_copy(self):
        # Test that setting the filename argument works.
        second_temp = "%s.2" % support.TESTFN
        self.registerFileForCleanUp(second_temp)
        result = urllib.request.urlretrieve(self.constructLocalFileUrl(
            support.TESTFN), second_temp)
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
        second_temp = "%s.2" % support.TESTFN
        self.registerFileForCleanUp(second_temp)
        urllib.request.urlretrieve(
            self.constructLocalFileUrl(support.TESTFN),
            second_temp, hooktester)

    def test_reporthook_0_bytes(self):
        # Test on zero length file. Should call reporthook only 1 time.
        report = []
        def hooktester(block_count, block_read_size, file_size, _report=report):
            _report.append((block_count, block_read_size, file_size))
        srcFileName = self.createNewTempFile()
        urllib.request.urlretrieve(self.constructLocalFileUrl(srcFileName),
            support.TESTFN, hooktester)
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
            support.TESTFN, hooktester)
        self.assertEqual(len(report), 2)
        self.assertEqual(report[0][1], 0)
        self.assertEqual(report[1][1], 5)

    def test_reporthook_8193_bytes(self):
        # Test on 8193 byte file. Should call reporthook only 3 times (once
        # when the "network connection" is established, once for the next 8192
        # bytes, and once for the last byte).
        report = []
        def hooktester(block_count, block_read_size, file_size, _report=report):
            _report.append((block_count, block_read_size, file_size))
        srcFileName = self.createNewTempFile(b"x" * 8193)
        urllib.request.urlretrieve(self.constructLocalFileUrl(srcFileName),
            support.TESTFN, hooktester)
        self.assertEqual(len(report), 3)
        self.assertEqual(report[0][1], 0)
        self.assertEqual(report[1][1], 8192)
        self.assertEqual(report[2][1], 1)


class urlretrieve_HttpTests(unittest.TestCase, FakeHTTPMixin):
    """Test urllib.urlretrieve() using fake http connections"""

    def test_short_content_raises_ContentTooShortError(self):
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
                urllib.request.urlretrieve('http://example.com/',
                                           reporthook=_reporthook)
            finally:
                self.unfakehttp()

    def test_short_content_raises_ContentTooShortError_without_reporthook(self):
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
                urllib.request.urlretrieve('http://example.com/')
            finally:
                self.unfakehttp()


class QuotingTests(unittest.TestCase):
    """Tests for urllib.quote() and urllib.quote_plus()

    According to RFC 2396 (Uniform Resource Identifiers), to escape a
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
                                 "_.-"])
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
        should_quote.append('<>#%"{}|\^[]`')
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
        self.assertRaises((TypeError, AttributeError), urllib.parse.unquote, None)
        self.assertRaises((TypeError, AttributeError), urllib.parse.unquote, ())
        with support.check_warnings(('', BytesWarning), quiet=True):
            self.assertRaises((TypeError, AttributeError), urllib.parse.unquote, b'')

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

    @unittest.skipUnless(sys.platform == 'win32',
                         'test specific to the urllib.url2path function.')
    def test_ntpath(self):
        given = ('/C:/', '///C:/', '/C|//')
        expect = 'C:\\'
        for url in given:
            result = urllib.request.url2pathname(url)
            self.assertEqual(expect, result,
                             'urllib.request..url2pathname() failed; %s != %s' %
                             (expect, result))
        given = '///C|/path'
        expect = 'C:\\path'
        result = urllib.request.url2pathname(given)
        self.assertEqual(expect, result,
                         'urllib.request.url2pathname() failed; %s != %s' %
                         (expect, result))

class Utility_Tests(unittest.TestCase):
    """Testcase to test the various utility functions in the urllib."""

    def test_splitpasswd(self):
        """Some of password examples are not sensible, but it is added to
        confirming to RFC2617 and addressing issue4675.
        """
        self.assertEqual(('user', 'ab'),urllib.parse.splitpasswd('user:ab'))
        self.assertEqual(('user', 'a\nb'),urllib.parse.splitpasswd('user:a\nb'))
        self.assertEqual(('user', 'a\tb'),urllib.parse.splitpasswd('user:a\tb'))
        self.assertEqual(('user', 'a\rb'),urllib.parse.splitpasswd('user:a\rb'))
        self.assertEqual(('user', 'a\fb'),urllib.parse.splitpasswd('user:a\fb'))
        self.assertEqual(('user', 'a\vb'),urllib.parse.splitpasswd('user:a\vb'))
        self.assertEqual(('user', 'a:b'),urllib.parse.splitpasswd('user:a:b'))
        self.assertEqual(('user', 'a b'),urllib.parse.splitpasswd('user:a b'))
        self.assertEqual(('user 2', 'ab'),urllib.parse.splitpasswd('user 2:ab'))
        self.assertEqual(('user+1', 'a+b'),urllib.parse.splitpasswd('user+1:a+b'))

    def test_thishost(self):
        """Test the urllib.request.thishost utility function returns a tuple"""
        self.assertIsInstance(urllib.request.thishost(), tuple)


class URLopener_Tests(unittest.TestCase):
    """Testcase to test the open method of URLopener class."""

    def test_quoted_open(self):
        class DummyURLopener(urllib.request.URLopener):
            def open_spam(self, url):
                return url

        self.assertEqual(DummyURLopener().open(
            'spam://example/ /'),'//example/%20/')

        # test the safe characters are not quoted by urlopen
        self.assertEqual(DummyURLopener().open(
            "spam://c:|windows%/:=&?~#+!$,;'@()*[]|/path/"),
            "//c:|windows%/:=&?~#+!$,;'@()*[]|/path/")

# Just commented them out.
# Can't really tell why keep failing in windows and sparc.
# Everywhere else they work ok, but on those machines, sometimes
# fail in one of the tests, sometimes in other. I have a linux, and
# the tests go ok.
# If anybody has one of the problematic enviroments, please help!
# .   Facundo
#
# def server(evt):
#     import socket, time
#     serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#     serv.settimeout(3)
#     serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
#     serv.bind(("", 9093))
#     serv.listen(5)
#     try:
#         conn, addr = serv.accept()
#         conn.send("1 Hola mundo\n")
#         cantdata = 0
#         while cantdata < 13:
#             data = conn.recv(13-cantdata)
#             cantdata += len(data)
#             time.sleep(.3)
#         conn.send("2 No more lines\n")
#         conn.close()
#     except socket.timeout:
#         pass
#     finally:
#         serv.close()
#         evt.set()
#
# class FTPWrapperTests(unittest.TestCase):
#
#     def setUp(self):
#         import ftplib, time, threading
#         ftplib.FTP.port = 9093
#         self.evt = threading.Event()
#         threading.Thread(target=server, args=(self.evt,)).start()
#         time.sleep(.1)
#
#     def tearDown(self):
#         self.evt.wait()
#
#     def testBasic(self):
#         # connects
#         ftp = urllib.ftpwrapper("myuser", "mypass", "localhost", 9093, [])
#         ftp.close()
#
#     def testTimeoutNone(self):
#         # global default timeout is ignored
#         import socket
#         self.assertTrue(socket.getdefaulttimeout() is None)
#         socket.setdefaulttimeout(30)
#         try:
#             ftp = urllib.ftpwrapper("myuser", "mypass", "localhost", 9093, [])
#         finally:
#             socket.setdefaulttimeout(None)
#         self.assertEqual(ftp.ftp.sock.gettimeout(), 30)
#         ftp.close()
#
#     def testTimeoutDefault(self):
#         # global default timeout is used
#         import socket
#         self.assertTrue(socket.getdefaulttimeout() is None)
#         socket.setdefaulttimeout(30)
#         try:
#             ftp = urllib.ftpwrapper("myuser", "mypass", "localhost", 9093, [])
#         finally:
#             socket.setdefaulttimeout(None)
#         self.assertEqual(ftp.ftp.sock.gettimeout(), 30)
#         ftp.close()
#
#     def testTimeoutValue(self):
#         ftp = urllib.ftpwrapper("myuser", "mypass", "localhost", 9093, [],
#                                 timeout=30)
#         self.assertEqual(ftp.ftp.sock.gettimeout(), 30)
#         ftp.close()

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

    def test_quote_url(self):
        Request = urllib.request.Request
        request = Request("http://www.python.org/foo bar")
        self.assertEqual(request.full_url, "http://www.python.org/foo%20bar")


def test_main():
    support.run_unittest(
        urlopen_FileTests,
        urlopen_HttpTests,
        urlretrieve_FileTests,
        urlretrieve_HttpTests,
        ProxyTests,
        QuotingTests,
        UnquotingTests,
        urlencode_Tests,
        Pathname_Tests,
        Utility_Tests,
        URLopener_Tests,
        #FTPWrapperTests,
        RequestTests,
    )



if __name__ == '__main__':
    test_main()
