"""Regresssion tests for urllib"""

import urllib
import httplib
import unittest
from test import test_support
import os
import mimetools
import tempfile
import StringIO

def hexescape(char):
    """Escape char as RFC 2396 specifies"""
    hex_repr = hex(ord(char))[2:].upper()
    if len(hex_repr) == 1:
        hex_repr = "0%s" % hex_repr
    return "%" + hex_repr

class urlopen_FileTests(unittest.TestCase):
    """Test urlopen() opening a temporary file.

    Try to test as much functionality as possible so as to cut down on reliance
    on connecting to the Net for testing.

    """

    def setUp(self):
        """Setup of a temp file to use for testing"""
        self.text = "test_urllib: %s\n" % self.__class__.__name__
        FILE = file(test_support.TESTFN, 'wb')
        try:
            FILE.write(self.text)
        finally:
            FILE.close()
        self.pathname = test_support.TESTFN
        self.returned_obj = urllib.urlopen("file:%s" % self.pathname)

    def tearDown(self):
        """Shut down the open object"""
        self.returned_obj.close()
        os.remove(test_support.TESTFN)

    def test_interface(self):
        # Make sure object returned by urlopen() has the specified methods
        for attr in ("read", "readline", "readlines", "fileno",
                     "close", "info", "geturl", "getcode", "__iter__"):
            self.assert_(hasattr(self.returned_obj, attr),
                         "object returned by urlopen() lacks %s attribute" %
                         attr)

    def test_read(self):
        self.assertEqual(self.text, self.returned_obj.read())

    def test_readline(self):
        self.assertEqual(self.text, self.returned_obj.readline())
        self.assertEqual('', self.returned_obj.readline(),
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
        self.assert_(isinstance(file_num, int),
                     "fileno() did not return an int")
        self.assertEqual(os.read(file_num, len(self.text)), self.text,
                         "Reading on the file descriptor returned by fileno() "
                         "did not return the expected text")

    def test_close(self):
        # Test close() by calling it hear and then having it be called again
        # by the tearDown() method for the test
        self.returned_obj.close()

    def test_info(self):
        self.assert_(isinstance(self.returned_obj.info(), mimetools.Message))

    def test_geturl(self):
        self.assertEqual(self.returned_obj.geturl(), self.pathname)

    def test_getcode(self):
        self.assertEqual(self.returned_obj.getcode(), None)

    def test_iter(self):
        # Test iterator
        # Don't need to count number of iterations since test would fail the
        # instant it returned anything beyond the first line from the
        # comparison
        for line in self.returned_obj.__iter__():
            self.assertEqual(line, self.text)

class urlopen_HttpTests(unittest.TestCase):
    """Test urlopen() opening a fake http connection."""

    def fakehttp(self, fakedata):
        class FakeSocket(StringIO.StringIO):
            def sendall(self, str): pass
            def makefile(self, mode, name): return self
            def read(self, amt=None):
                if self.closed: return ''
                return StringIO.StringIO.read(self, amt)
            def readline(self, length=None):
                if self.closed: return ''
                return StringIO.StringIO.readline(self, length)
        class FakeHTTPConnection(httplib.HTTPConnection):
            def connect(self):
                self.sock = FakeSocket(fakedata)
        assert httplib.HTTP._connection_class == httplib.HTTPConnection
        httplib.HTTP._connection_class = FakeHTTPConnection

    def unfakehttp(self):
        httplib.HTTP._connection_class = httplib.HTTPConnection

    def test_read(self):
        self.fakehttp('Hello!')
        try:
            fp = urllib.urlopen("http://python.org/")
            self.assertEqual(fp.readline(), 'Hello!')
            self.assertEqual(fp.readline(), '')
            self.assertEqual(fp.geturl(), 'http://python.org/')
            self.assertEqual(fp.getcode(), 200)
        finally:
            self.unfakehttp()

    def test_read_bogus(self):
        # urlopen() should raise IOError for many error codes.
        self.fakehttp('''HTTP/1.1 401 Authentication Required
Date: Wed, 02 Jan 2008 03:03:54 GMT
Server: Apache/1.3.33 (Debian GNU/Linux) mod_ssl/2.8.22 OpenSSL/0.9.7e
Connection: close
Content-Type: text/html; charset=iso-8859-1
''')
        try:
            self.assertRaises(IOError, urllib.urlopen, "http://python.org/")
        finally:
            self.unfakehttp()

    def test_empty_socket(self):
        # urlopen() raises IOError if the underlying socket does not send any
        # data. (#1680230)
        self.fakehttp('')
        try:
            self.assertRaises(IOError, urllib.urlopen, 'http://something')
        finally:
            self.unfakehttp()

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
        self.registerFileForCleanUp(test_support.TESTFN)
        self.text = 'testing urllib.urlretrieve'
        try:
            FILE = file(test_support.TESTFN, 'wb')
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
        return "file://%s" % urllib.pathname2url(os.path.abspath(filePath))

    def createNewTempFile(self, data=""):
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
        result = urllib.urlretrieve("file:%s" % test_support.TESTFN)
        self.assertEqual(result[0], test_support.TESTFN)
        self.assert_(isinstance(result[1], mimetools.Message),
                     "did not get a mimetools.Message instance as second "
                     "returned value")

    def test_copy(self):
        # Test that setting the filename argument works.
        second_temp = "%s.2" % test_support.TESTFN
        self.registerFileForCleanUp(second_temp)
        result = urllib.urlretrieve(self.constructLocalFileUrl(
            test_support.TESTFN), second_temp)
        self.assertEqual(second_temp, result[0])
        self.assert_(os.path.exists(second_temp), "copy of the file was not "
                                                  "made")
        FILE = file(second_temp, 'rb')
        try:
            text = FILE.read()
            FILE.close()
        finally:
            try: FILE.close()
            except: pass
        self.assertEqual(self.text, text)

    def test_reporthook(self):
        # Make sure that the reporthook works.
        def hooktester(count, block_size, total_size, count_holder=[0]):
            self.assert_(isinstance(count, int))
            self.assert_(isinstance(block_size, int))
            self.assert_(isinstance(total_size, int))
            self.assertEqual(count, count_holder[0])
            count_holder[0] = count_holder[0] + 1
        second_temp = "%s.2" % test_support.TESTFN
        self.registerFileForCleanUp(second_temp)
        urllib.urlretrieve(self.constructLocalFileUrl(test_support.TESTFN),
            second_temp, hooktester)

    def test_reporthook_0_bytes(self):
        # Test on zero length file. Should call reporthook only 1 time.
        report = []
        def hooktester(count, block_size, total_size, _report=report):
            _report.append((count, block_size, total_size))
        srcFileName = self.createNewTempFile()
        urllib.urlretrieve(self.constructLocalFileUrl(srcFileName),
            test_support.TESTFN, hooktester)
        self.assertEqual(len(report), 1)
        self.assertEqual(report[0][2], 0)

    def test_reporthook_5_bytes(self):
        # Test on 5 byte file. Should call reporthook only 2 times (once when
        # the "network connection" is established and once when the block is
        # read). Since the block size is 8192 bytes, only one block read is
        # required to read the entire file.
        report = []
        def hooktester(count, block_size, total_size, _report=report):
            _report.append((count, block_size, total_size))
        srcFileName = self.createNewTempFile("x" * 5)
        urllib.urlretrieve(self.constructLocalFileUrl(srcFileName),
            test_support.TESTFN, hooktester)
        self.assertEqual(len(report), 2)
        self.assertEqual(report[0][1], 8192)
        self.assertEqual(report[0][2], 5)

    def test_reporthook_8193_bytes(self):
        # Test on 8193 byte file. Should call reporthook only 3 times (once
        # when the "network connection" is established, once for the next 8192
        # bytes, and once for the last byte).
        report = []
        def hooktester(count, block_size, total_size, _report=report):
            _report.append((count, block_size, total_size))
        srcFileName = self.createNewTempFile("x" * 8193)
        urllib.urlretrieve(self.constructLocalFileUrl(srcFileName),
            test_support.TESTFN, hooktester)
        self.assertEqual(len(report), 3)
        self.assertEqual(report[0][1], 8192)
        self.assertEqual(report[0][2], 8193)

class QuotingTests(unittest.TestCase):
    """Tests for urllib.quote() and urllib.quote_plus()

    According to RFC 2396 ("Uniform Resource Identifiers), to escape a
    character you write it as '%' + <2 character US-ASCII hex value>.  The Python
    code of ``'%' + hex(ord(<character>))[2:]`` escapes a character properly.
    Case does not matter on the hex letters.

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
        result = urllib.quote(do_not_quote)
        self.assertEqual(do_not_quote, result,
                         "using quote(): %s != %s" % (do_not_quote, result))
        result = urllib.quote_plus(do_not_quote)
        self.assertEqual(do_not_quote, result,
                        "using quote_plus(): %s != %s" % (do_not_quote, result))

    def test_default_safe(self):
        # Test '/' is default value for 'safe' parameter
        self.assertEqual(urllib.quote.func_defaults[0], '/')

    def test_safe(self):
        # Test setting 'safe' parameter does what it should do
        quote_by_default = "<>"
        result = urllib.quote(quote_by_default, safe=quote_by_default)
        self.assertEqual(quote_by_default, result,
                         "using quote(): %s != %s" % (quote_by_default, result))
        result = urllib.quote_plus(quote_by_default, safe=quote_by_default)
        self.assertEqual(quote_by_default, result,
                         "using quote_plus(): %s != %s" %
                         (quote_by_default, result))

    def test_default_quoting(self):
        # Make sure all characters that should be quoted are by default sans
        # space (separate test for that).
        should_quote = [chr(num) for num in range(32)] # For 0x00 - 0x1F
        should_quote.append('<>#%"{}|\^[]`')
        should_quote.append(chr(127)) # For 0x7F
        should_quote = ''.join(should_quote)
        for char in should_quote:
            result = urllib.quote(char)
            self.assertEqual(hexescape(char), result,
                             "using quote(): %s should be escaped to %s, not %s" %
                             (char, hexescape(char), result))
            result = urllib.quote_plus(char)
            self.assertEqual(hexescape(char), result,
                             "using quote_plus(): "
                             "%s should be escapes to %s, not %s" %
                             (char, hexescape(char), result))
        del should_quote
        partial_quote = "ab[]cd"
        expected = "ab%5B%5Dcd"
        result = urllib.quote(partial_quote)
        self.assertEqual(expected, result,
                         "using quote(): %s != %s" % (expected, result))
        self.assertEqual(expected, result,
                         "using quote_plus(): %s != %s" % (expected, result))

    def test_quoting_space(self):
        # Make sure quote() and quote_plus() handle spaces as specified in
        # their unique way
        result = urllib.quote(' ')
        self.assertEqual(result, hexescape(' '),
                         "using quote(): %s != %s" % (result, hexescape(' ')))
        result = urllib.quote_plus(' ')
        self.assertEqual(result, '+',
                         "using quote_plus(): %s != +" % result)
        given = "a b cd e f"
        expect = given.replace(' ', hexescape(' '))
        result = urllib.quote(given)
        self.assertEqual(expect, result,
                         "using quote(): %s != %s" % (expect, result))
        expect = given.replace(' ', '+')
        result = urllib.quote_plus(given)
        self.assertEqual(expect, result,
                         "using quote_plus(): %s != %s" % (expect, result))

    def test_quoting_plus(self):
        self.assertEqual(urllib.quote_plus('alpha+beta gamma'),
                         'alpha%2Bbeta+gamma')
        self.assertEqual(urllib.quote_plus('alpha+beta gamma', '+'),
                         'alpha+beta+gamma')

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
            result = urllib.unquote(given)
            self.assertEqual(expect, result,
                             "using unquote(): %s != %s" % (expect, result))
            result = urllib.unquote_plus(given)
            self.assertEqual(expect, result,
                             "using unquote_plus(): %s != %s" %
                             (expect, result))
            escape_list.append(given)
        escape_string = ''.join(escape_list)
        del escape_list
        result = urllib.unquote(escape_string)
        self.assertEqual(result.count('%'), 1,
                         "using quote(): not all characters escaped; %s" %
                         result)
        result = urllib.unquote(escape_string)
        self.assertEqual(result.count('%'), 1,
                         "using unquote(): not all characters escaped: "
                         "%s" % result)

    def test_unquoting_parts(self):
        # Make sure unquoting works when have non-quoted characters
        # interspersed
        given = 'ab%sd' % hexescape('c')
        expect = "abcd"
        result = urllib.unquote(given)
        self.assertEqual(expect, result,
                         "using quote(): %s != %s" % (expect, result))
        result = urllib.unquote_plus(given)
        self.assertEqual(expect, result,
                         "using unquote_plus(): %s != %s" % (expect, result))

    def test_unquoting_plus(self):
        # Test difference between unquote() and unquote_plus()
        given = "are+there+spaces..."
        expect = given
        result = urllib.unquote(given)
        self.assertEqual(expect, result,
                         "using unquote(): %s != %s" % (expect, result))
        expect = given.replace('+', ' ')
        result = urllib.unquote_plus(given)
        self.assertEqual(expect, result,
                         "using unquote_plus(): %s != %s" % (expect, result))

    def test_unquote_with_unicode(self):
        r = urllib.unquote(u'br%C3%BCckner_sapporo_20050930.doc')
        self.assertEqual(r, u'br\xc3\xbcckner_sapporo_20050930.doc')

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
        result = urllib.urlencode(given)
        for expected in expect_somewhere:
            self.assert_(expected in result,
                         "testing %s: %s not found in %s" %
                         (test_type, expected, result))
        self.assertEqual(result.count('&'), 2,
                         "testing %s: expected 2 '&'s; got %s" %
                         (test_type, result.count('&')))
        amp_location = result.index('&')
        on_amp_left = result[amp_location - 1]
        on_amp_right = result[amp_location + 1]
        self.assert_(on_amp_left.isdigit() and on_amp_right.isdigit(),
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
        result = urllib.urlencode(given)
        self.assertEqual(expect, result)
        given = {"key name":"A bunch of pluses"}
        expect = "key+name=A+bunch+of+pluses"
        result = urllib.urlencode(given)
        self.assertEqual(expect, result)

    def test_doseq(self):
        # Test that passing True for 'doseq' parameter works correctly
        given = {'sequence':['1', '2', '3']}
        expect = "sequence=%s" % urllib.quote_plus(str(['1', '2', '3']))
        result = urllib.urlencode(given)
        self.assertEqual(expect, result)
        result = urllib.urlencode(given, True)
        for value in given["sequence"]:
            expect = "sequence=%s" % value
            self.assert_(expect in result,
                         "%s not found in %s" % (expect, result))
        self.assertEqual(result.count('&'), 2,
                         "Expected 2 '&'s, got %s" % result.count('&'))

class Pathname_Tests(unittest.TestCase):
    """Test pathname2url() and url2pathname()"""

    def test_basic(self):
        # Make sure simple tests pass
        expected_path = os.path.join("parts", "of", "a", "path")
        expected_url = "parts/of/a/path"
        result = urllib.pathname2url(expected_path)
        self.assertEqual(expected_url, result,
                         "pathname2url() failed; %s != %s" %
                         (result, expected_url))
        result = urllib.url2pathname(expected_url)
        self.assertEqual(expected_path, result,
                         "url2pathame() failed; %s != %s" %
                         (result, expected_path))

    def test_quoting(self):
        # Test automatic quoting and unquoting works for pathnam2url() and
        # url2pathname() respectively
        given = os.path.join("needs", "quot=ing", "here")
        expect = "needs/%s/here" % urllib.quote("quot=ing")
        result = urllib.pathname2url(given)
        self.assertEqual(expect, result,
                         "pathname2url() failed; %s != %s" %
                         (expect, result))
        expect = given
        result = urllib.url2pathname(result)
        self.assertEqual(expect, result,
                         "url2pathname() failed; %s != %s" %
                         (expect, result))
        given = os.path.join("make sure", "using_quote")
        expect = "%s/using_quote" % urllib.quote("make sure")
        result = urllib.pathname2url(given)
        self.assertEqual(expect, result,
                         "pathname2url() failed; %s != %s" %
                         (expect, result))
        given = "make+sure/using_unquote"
        expect = os.path.join("make+sure", "using_unquote")
        result = urllib.url2pathname(given)
        self.assertEqual(expect, result,
                         "url2pathname() failed; %s != %s" %
                         (expect, result))

# Just commented them out.
# Can't really tell why keep failing in windows and sparc.
# Everywhere else they work ok, but on those machines, someteimes
# fail in one of the tests, sometimes in other. I have a linux, and
# the tests go ok.
# If anybody has one of the problematic enviroments, please help!
# .   Facundo
#
# def server(evt):
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
#         ftp.ftp.sock.close()
#
#     def testTimeoutDefault(self):
#         # default
#         ftp = urllib.ftpwrapper("myuser", "mypass", "localhost", 9093, [])
#         self.assertTrue(ftp.ftp.sock.gettimeout() is None)
#         ftp.ftp.sock.close()
#
#     def testTimeoutValue(self):
#         # a value
#         ftp = urllib.ftpwrapper("myuser", "mypass", "localhost", 9093, [], timeout=30)
#         self.assertEqual(ftp.ftp.sock.gettimeout(), 30)
#         ftp.ftp.sock.close()
#
#     def testTimeoutNone(self):
#         # None, having other default
#         previous = socket.getdefaulttimeout()
#         socket.setdefaulttimeout(30)
#         try:
#             ftp = urllib.ftpwrapper("myuser", "mypass", "localhost", 9093, [])
#         finally:
#             socket.setdefaulttimeout(previous)
#         self.assertEqual(ftp.ftp.sock.gettimeout(), 30)
#         ftp.ftp.close()
#



def test_main():
    test_support.run_unittest(
        urlopen_FileTests,
        urlopen_HttpTests,
        urlretrieve_FileTests,
        QuotingTests,
        UnquotingTests,
        urlencode_Tests,
        Pathname_Tests,
        #FTPWrapperTests,
    )



if __name__ == '__main__':
    test_main()
