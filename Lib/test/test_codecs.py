import _testcapi
import codecs
import io
import locale
import sys
import unittest
import warnings

from test import support

if sys.platform == 'win32':
    VISTA_OR_LATER = (sys.getwindowsversion().major >= 6)
else:
    VISTA_OR_LATER = False

try:
    import ctypes
except ImportError:
    ctypes = None
    SIZEOF_WCHAR_T = -1
else:
    SIZEOF_WCHAR_T = ctypes.sizeof(ctypes.c_wchar)

def coding_checker(self, coder):
    def check(input, expect):
        self.assertEqual(coder(input), (expect, len(input)))
    return check

class Queue(object):
    """
    queue: write bytes at one end, read bytes from the other end
    """
    def __init__(self, buffer):
        self._buffer = buffer

    def write(self, chars):
        self._buffer += chars

    def read(self, size=-1):
        if size<0:
            s = self._buffer
            self._buffer = self._buffer[:0] # make empty
            return s
        else:
            s = self._buffer[:size]
            self._buffer = self._buffer[size:]
            return s

class MixInCheckStateHandling:
    def check_state_handling_decode(self, encoding, u, s):
        for i in range(len(s)+1):
            d = codecs.getincrementaldecoder(encoding)()
            part1 = d.decode(s[:i])
            state = d.getstate()
            self.assertIsInstance(state[1], int)
            # Check that the condition stated in the documentation for
            # IncrementalDecoder.getstate() holds
            if not state[1]:
                # reset decoder to the default state without anything buffered
                d.setstate((state[0][:0], 0))
                # Feeding the previous input may not produce any output
                self.assertTrue(not d.decode(state[0]))
                # The decoder must return to the same state
                self.assertEqual(state, d.getstate())
            # Create a new decoder and set it to the state
            # we extracted from the old one
            d = codecs.getincrementaldecoder(encoding)()
            d.setstate(state)
            part2 = d.decode(s[i:], True)
            self.assertEqual(u, part1+part2)

    def check_state_handling_encode(self, encoding, u, s):
        for i in range(len(u)+1):
            d = codecs.getincrementalencoder(encoding)()
            part1 = d.encode(u[:i])
            state = d.getstate()
            d = codecs.getincrementalencoder(encoding)()
            d.setstate(state)
            part2 = d.encode(u[i:], True)
            self.assertEqual(s, part1+part2)

class ReadTest(MixInCheckStateHandling):
    def check_partial(self, input, partialresults):
        # get a StreamReader for the encoding and feed the bytestring version
        # of input to the reader byte by byte. Read everything available from
        # the StreamReader and check that the results equal the appropriate
        # entries from partialresults.
        q = Queue(b"")
        r = codecs.getreader(self.encoding)(q)
        result = ""
        for (c, partialresult) in zip(input.encode(self.encoding), partialresults):
            q.write(bytes([c]))
            result += r.read()
            self.assertEqual(result, partialresult)
        # check that there's nothing left in the buffers
        self.assertEqual(r.read(), "")
        self.assertEqual(r.bytebuffer, b"")

        # do the check again, this time using a incremental decoder
        d = codecs.getincrementaldecoder(self.encoding)()
        result = ""
        for (c, partialresult) in zip(input.encode(self.encoding), partialresults):
            result += d.decode(bytes([c]))
            self.assertEqual(result, partialresult)
        # check that there's nothing left in the buffers
        self.assertEqual(d.decode(b"", True), "")
        self.assertEqual(d.buffer, b"")

        # Check whether the reset method works properly
        d.reset()
        result = ""
        for (c, partialresult) in zip(input.encode(self.encoding), partialresults):
            result += d.decode(bytes([c]))
            self.assertEqual(result, partialresult)
        # check that there's nothing left in the buffers
        self.assertEqual(d.decode(b"", True), "")
        self.assertEqual(d.buffer, b"")

        # check iterdecode()
        encoded = input.encode(self.encoding)
        self.assertEqual(
            input,
            "".join(codecs.iterdecode([bytes([c]) for c in encoded], self.encoding))
        )

    def test_readline(self):
        def getreader(input):
            stream = io.BytesIO(input.encode(self.encoding))
            return codecs.getreader(self.encoding)(stream)

        def readalllines(input, keepends=True, size=None):
            reader = getreader(input)
            lines = []
            while True:
                line = reader.readline(size=size, keepends=keepends)
                if not line:
                    break
                lines.append(line)
            return "|".join(lines)

        s = "foo\nbar\r\nbaz\rspam\u2028eggs"
        sexpected = "foo\n|bar\r\n|baz\r|spam\u2028|eggs"
        sexpectednoends = "foo|bar|baz|spam|eggs"
        self.assertEqual(readalllines(s, True), sexpected)
        self.assertEqual(readalllines(s, False), sexpectednoends)
        self.assertEqual(readalllines(s, True, 10), sexpected)
        self.assertEqual(readalllines(s, False, 10), sexpectednoends)

        # Test long lines (multiple calls to read() in readline())
        vw = []
        vwo = []
        for (i, lineend) in enumerate("\n \r\n \r \u2028".split()):
            vw.append((i*200)*"\3042" + lineend)
            vwo.append((i*200)*"\3042")
        self.assertEqual(readalllines("".join(vw), True), "".join(vw))
        self.assertEqual(readalllines("".join(vw), False),"".join(vwo))

        # Test lines where the first read might end with \r, so the
        # reader has to look ahead whether this is a lone \r or a \r\n
        for size in range(80):
            for lineend in "\n \r\n \r \u2028".split():
                s = 10*(size*"a" + lineend + "xxx\n")
                reader = getreader(s)
                for i in range(10):
                    self.assertEqual(
                        reader.readline(keepends=True),
                        size*"a" + lineend,
                    )
                reader = getreader(s)
                for i in range(10):
                    self.assertEqual(
                        reader.readline(keepends=False),
                        size*"a",
                    )

    def test_bug1175396(self):
        s = [
            '<%!--===================================================\r\n',
            '    BLOG index page: show recent articles,\r\n',
            '    today\'s articles, or articles of a specific date.\r\n',
            '========================================================--%>\r\n',
            '<%@inputencoding="ISO-8859-1"%>\r\n',
            '<%@pagetemplate=TEMPLATE.y%>\r\n',
            '<%@import=import frog.util, frog%>\r\n',
            '<%@import=import frog.objects%>\r\n',
            '<%@import=from frog.storageerrors import StorageError%>\r\n',
            '<%\r\n',
            '\r\n',
            'import logging\r\n',
            'log=logging.getLogger("Snakelets.logger")\r\n',
            '\r\n',
            '\r\n',
            'user=self.SessionCtx.user\r\n',
            'storageEngine=self.SessionCtx.storageEngine\r\n',
            '\r\n',
            '\r\n',
            'def readArticlesFromDate(date, count=None):\r\n',
            '    entryids=storageEngine.listBlogEntries(date)\r\n',
            '    entryids.reverse() # descending\r\n',
            '    if count:\r\n',
            '        entryids=entryids[:count]\r\n',
            '    try:\r\n',
            '        return [ frog.objects.BlogEntry.load(storageEngine, date, Id) for Id in entryids ]\r\n',
            '    except StorageError,x:\r\n',
            '        log.error("Error loading articles: "+str(x))\r\n',
            '        self.abort("cannot load articles")\r\n',
            '\r\n',
            'showdate=None\r\n',
            '\r\n',
            'arg=self.Request.getArg()\r\n',
            'if arg=="today":\r\n',
            '    #-------------------- TODAY\'S ARTICLES\r\n',
            '    self.write("<h2>Today\'s articles</h2>")\r\n',
            '    showdate = frog.util.isodatestr() \r\n',
            '    entries = readArticlesFromDate(showdate)\r\n',
            'elif arg=="active":\r\n',
            '    #-------------------- ACTIVE ARTICLES redirect\r\n',
            '    self.Yredirect("active.y")\r\n',
            'elif arg=="login":\r\n',
            '    #-------------------- LOGIN PAGE redirect\r\n',
            '    self.Yredirect("login.y")\r\n',
            'elif arg=="date":\r\n',
            '    #-------------------- ARTICLES OF A SPECIFIC DATE\r\n',
            '    showdate = self.Request.getParameter("date")\r\n',
            '    self.write("<h2>Articles written on %s</h2>"% frog.util.mediumdatestr(showdate))\r\n',
            '    entries = readArticlesFromDate(showdate)\r\n',
            'else:\r\n',
            '    #-------------------- RECENT ARTICLES\r\n',
            '    self.write("<h2>Recent articles</h2>")\r\n',
            '    dates=storageEngine.listBlogEntryDates()\r\n',
            '    if dates:\r\n',
            '        entries=[]\r\n',
            '        SHOWAMOUNT=10\r\n',
            '        for showdate in dates:\r\n',
            '            entries.extend( readArticlesFromDate(showdate, SHOWAMOUNT-len(entries)) )\r\n',
            '            if len(entries)>=SHOWAMOUNT:\r\n',
            '                break\r\n',
            '                \r\n',
        ]
        stream = io.BytesIO("".join(s).encode(self.encoding))
        reader = codecs.getreader(self.encoding)(stream)
        for (i, line) in enumerate(reader):
            self.assertEqual(line, s[i])

    def test_readlinequeue(self):
        q = Queue(b"")
        writer = codecs.getwriter(self.encoding)(q)
        reader = codecs.getreader(self.encoding)(q)

        # No lineends
        writer.write("foo\r")
        self.assertEqual(reader.readline(keepends=False), "foo")
        writer.write("\nbar\r")
        self.assertEqual(reader.readline(keepends=False), "")
        self.assertEqual(reader.readline(keepends=False), "bar")
        writer.write("baz")
        self.assertEqual(reader.readline(keepends=False), "baz")
        self.assertEqual(reader.readline(keepends=False), "")

        # Lineends
        writer.write("foo\r")
        self.assertEqual(reader.readline(keepends=True), "foo\r")
        writer.write("\nbar\r")
        self.assertEqual(reader.readline(keepends=True), "\n")
        self.assertEqual(reader.readline(keepends=True), "bar\r")
        writer.write("baz")
        self.assertEqual(reader.readline(keepends=True), "baz")
        self.assertEqual(reader.readline(keepends=True), "")
        writer.write("foo\r\n")
        self.assertEqual(reader.readline(keepends=True), "foo\r\n")

    def test_bug1098990_a(self):
        s1 = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\r\n"
        s2 = "offending line: ladfj askldfj klasdj fskla dfzaskdj fasklfj laskd fjasklfzzzzaa%whereisthis!!!\r\n"
        s3 = "next line.\r\n"

        s = (s1+s2+s3).encode(self.encoding)
        stream = io.BytesIO(s)
        reader = codecs.getreader(self.encoding)(stream)
        self.assertEqual(reader.readline(), s1)
        self.assertEqual(reader.readline(), s2)
        self.assertEqual(reader.readline(), s3)
        self.assertEqual(reader.readline(), "")

    def test_bug1098990_b(self):
        s1 = "aaaaaaaaaaaaaaaaaaaaaaaa\r\n"
        s2 = "bbbbbbbbbbbbbbbbbbbbbbbb\r\n"
        s3 = "stillokay:bbbbxx\r\n"
        s4 = "broken!!!!badbad\r\n"
        s5 = "againokay.\r\n"

        s = (s1+s2+s3+s4+s5).encode(self.encoding)
        stream = io.BytesIO(s)
        reader = codecs.getreader(self.encoding)(stream)
        self.assertEqual(reader.readline(), s1)
        self.assertEqual(reader.readline(), s2)
        self.assertEqual(reader.readline(), s3)
        self.assertEqual(reader.readline(), s4)
        self.assertEqual(reader.readline(), s5)
        self.assertEqual(reader.readline(), "")

class UTF32Test(ReadTest, unittest.TestCase):
    encoding = "utf-32"

    spamle = (b'\xff\xfe\x00\x00'
              b's\x00\x00\x00p\x00\x00\x00a\x00\x00\x00m\x00\x00\x00'
              b's\x00\x00\x00p\x00\x00\x00a\x00\x00\x00m\x00\x00\x00')
    spambe = (b'\x00\x00\xfe\xff'
              b'\x00\x00\x00s\x00\x00\x00p\x00\x00\x00a\x00\x00\x00m'
              b'\x00\x00\x00s\x00\x00\x00p\x00\x00\x00a\x00\x00\x00m')

    def test_only_one_bom(self):
        _,_,reader,writer = codecs.lookup(self.encoding)
        # encode some stream
        s = io.BytesIO()
        f = writer(s)
        f.write("spam")
        f.write("spam")
        d = s.getvalue()
        # check whether there is exactly one BOM in it
        self.assertTrue(d == self.spamle or d == self.spambe)
        # try to read it back
        s = io.BytesIO(d)
        f = reader(s)
        self.assertEqual(f.read(), "spamspam")

    def test_badbom(self):
        s = io.BytesIO(4*b"\xff")
        f = codecs.getreader(self.encoding)(s)
        self.assertRaises(UnicodeError, f.read)

        s = io.BytesIO(8*b"\xff")
        f = codecs.getreader(self.encoding)(s)
        self.assertRaises(UnicodeError, f.read)

    def test_partial(self):
        self.check_partial(
            "\x00\xff\u0100\uffff\U00010000",
            [
                "", # first byte of BOM read
                "", # second byte of BOM read
                "", # third byte of BOM read
                "", # fourth byte of BOM read => byteorder known
                "",
                "",
                "",
                "\x00",
                "\x00",
                "\x00",
                "\x00",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff\u0100",
                "\x00\xff\u0100",
                "\x00\xff\u0100",
                "\x00\xff\u0100",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff\U00010000",
            ]
        )

    def test_handlers(self):
        self.assertEqual(('\ufffd', 1),
                         codecs.utf_32_decode(b'\x01', 'replace', True))
        self.assertEqual(('', 1),
                         codecs.utf_32_decode(b'\x01', 'ignore', True))

    def test_errors(self):
        self.assertRaises(UnicodeDecodeError, codecs.utf_32_decode,
                          b"\xff", "strict", True)

    def test_decoder_state(self):
        self.check_state_handling_decode(self.encoding,
                                         "spamspam", self.spamle)
        self.check_state_handling_decode(self.encoding,
                                         "spamspam", self.spambe)

    def test_issue8941(self):
        # Issue #8941: insufficient result allocation when decoding into
        # surrogate pairs on UCS-2 builds.
        encoded_le = b'\xff\xfe\x00\x00' + b'\x00\x00\x01\x00' * 1024
        self.assertEqual('\U00010000' * 1024,
                         codecs.utf_32_decode(encoded_le)[0])
        encoded_be = b'\x00\x00\xfe\xff' + b'\x00\x01\x00\x00' * 1024
        self.assertEqual('\U00010000' * 1024,
                         codecs.utf_32_decode(encoded_be)[0])

class UTF32LETest(ReadTest, unittest.TestCase):
    encoding = "utf-32-le"

    def test_partial(self):
        self.check_partial(
            "\x00\xff\u0100\uffff\U00010000",
            [
                "",
                "",
                "",
                "\x00",
                "\x00",
                "\x00",
                "\x00",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff\u0100",
                "\x00\xff\u0100",
                "\x00\xff\u0100",
                "\x00\xff\u0100",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff\U00010000",
            ]
        )

    def test_simple(self):
        self.assertEqual("\U00010203".encode(self.encoding), b"\x03\x02\x01\x00")

    def test_errors(self):
        self.assertRaises(UnicodeDecodeError, codecs.utf_32_le_decode,
                          b"\xff", "strict", True)

    def test_issue8941(self):
        # Issue #8941: insufficient result allocation when decoding into
        # surrogate pairs on UCS-2 builds.
        encoded = b'\x00\x00\x01\x00' * 1024
        self.assertEqual('\U00010000' * 1024,
                         codecs.utf_32_le_decode(encoded)[0])

class UTF32BETest(ReadTest, unittest.TestCase):
    encoding = "utf-32-be"

    def test_partial(self):
        self.check_partial(
            "\x00\xff\u0100\uffff\U00010000",
            [
                "",
                "",
                "",
                "\x00",
                "\x00",
                "\x00",
                "\x00",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff\u0100",
                "\x00\xff\u0100",
                "\x00\xff\u0100",
                "\x00\xff\u0100",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff\U00010000",
            ]
        )

    def test_simple(self):
        self.assertEqual("\U00010203".encode(self.encoding), b"\x00\x01\x02\x03")

    def test_errors(self):
        self.assertRaises(UnicodeDecodeError, codecs.utf_32_be_decode,
                          b"\xff", "strict", True)

    def test_issue8941(self):
        # Issue #8941: insufficient result allocation when decoding into
        # surrogate pairs on UCS-2 builds.
        encoded = b'\x00\x01\x00\x00' * 1024
        self.assertEqual('\U00010000' * 1024,
                         codecs.utf_32_be_decode(encoded)[0])


class UTF16Test(ReadTest, unittest.TestCase):
    encoding = "utf-16"

    spamle = b'\xff\xfes\x00p\x00a\x00m\x00s\x00p\x00a\x00m\x00'
    spambe = b'\xfe\xff\x00s\x00p\x00a\x00m\x00s\x00p\x00a\x00m'

    def test_only_one_bom(self):
        _,_,reader,writer = codecs.lookup(self.encoding)
        # encode some stream
        s = io.BytesIO()
        f = writer(s)
        f.write("spam")
        f.write("spam")
        d = s.getvalue()
        # check whether there is exactly one BOM in it
        self.assertTrue(d == self.spamle or d == self.spambe)
        # try to read it back
        s = io.BytesIO(d)
        f = reader(s)
        self.assertEqual(f.read(), "spamspam")

    def test_badbom(self):
        s = io.BytesIO(b"\xff\xff")
        f = codecs.getreader(self.encoding)(s)
        self.assertRaises(UnicodeError, f.read)

        s = io.BytesIO(b"\xff\xff\xff\xff")
        f = codecs.getreader(self.encoding)(s)
        self.assertRaises(UnicodeError, f.read)

    def test_partial(self):
        self.check_partial(
            "\x00\xff\u0100\uffff\U00010000",
            [
                "", # first byte of BOM read
                "", # second byte of BOM read => byteorder known
                "",
                "\x00",
                "\x00",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff\u0100",
                "\x00\xff\u0100",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff\U00010000",
            ]
        )

    def test_handlers(self):
        self.assertEqual(('\ufffd', 1),
                         codecs.utf_16_decode(b'\x01', 'replace', True))
        self.assertEqual(('', 1),
                         codecs.utf_16_decode(b'\x01', 'ignore', True))

    def test_errors(self):
        self.assertRaises(UnicodeDecodeError, codecs.utf_16_decode,
                          b"\xff", "strict", True)

    def test_decoder_state(self):
        self.check_state_handling_decode(self.encoding,
                                         "spamspam", self.spamle)
        self.check_state_handling_decode(self.encoding,
                                         "spamspam", self.spambe)

    def test_bug691291(self):
        # Files are always opened in binary mode, even if no binary mode was
        # specified.  This means that no automatic conversion of '\n' is done
        # on reading and writing.
        s1 = 'Hello\r\nworld\r\n'

        s = s1.encode(self.encoding)
        self.addCleanup(support.unlink, support.TESTFN)
        with open(support.TESTFN, 'wb') as fp:
            fp.write(s)
        with codecs.open(support.TESTFN, 'U', encoding=self.encoding) as reader:
            self.assertEqual(reader.read(), s1)

class UTF16LETest(ReadTest, unittest.TestCase):
    encoding = "utf-16-le"

    def test_partial(self):
        self.check_partial(
            "\x00\xff\u0100\uffff\U00010000",
            [
                "",
                "\x00",
                "\x00",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff\u0100",
                "\x00\xff\u0100",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff\U00010000",
            ]
        )

    def test_errors(self):
        tests = [
            (b'\xff', '\ufffd'),
            (b'A\x00Z', 'A\ufffd'),
            (b'A\x00B\x00C\x00D\x00Z', 'ABCD\ufffd'),
            (b'\x00\xd8', '\ufffd'),
            (b'\x00\xd8A', '\ufffd'),
            (b'\x00\xd8A\x00', '\ufffdA'),
            (b'\x00\xdcA\x00', '\ufffdA'),
        ]
        for raw, expected in tests:
            self.assertRaises(UnicodeDecodeError, codecs.utf_16_le_decode,
                              raw, 'strict', True)
            self.assertEqual(raw.decode('utf-16le', 'replace'), expected)

    def test_nonbmp(self):
        self.assertEqual("\U00010203".encode(self.encoding),
                         b'\x00\xd8\x03\xde')
        self.assertEqual(b'\x00\xd8\x03\xde'.decode(self.encoding),
                         "\U00010203")

class UTF16BETest(ReadTest, unittest.TestCase):
    encoding = "utf-16-be"

    def test_partial(self):
        self.check_partial(
            "\x00\xff\u0100\uffff\U00010000",
            [
                "",
                "\x00",
                "\x00",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff\u0100",
                "\x00\xff\u0100",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff",
                "\x00\xff\u0100\uffff\U00010000",
            ]
        )

    def test_errors(self):
        tests = [
            (b'\xff', '\ufffd'),
            (b'\x00A\xff', 'A\ufffd'),
            (b'\x00A\x00B\x00C\x00DZ', 'ABCD\ufffd'),
            (b'\xd8\x00', '\ufffd'),
            (b'\xd8\x00\xdc', '\ufffd'),
            (b'\xd8\x00\x00A', '\ufffdA'),
            (b'\xdc\x00\x00A', '\ufffdA'),
        ]
        for raw, expected in tests:
            self.assertRaises(UnicodeDecodeError, codecs.utf_16_be_decode,
                              raw, 'strict', True)
            self.assertEqual(raw.decode('utf-16be', 'replace'), expected)

    def test_nonbmp(self):
        self.assertEqual("\U00010203".encode(self.encoding),
                         b'\xd8\x00\xde\x03')
        self.assertEqual(b'\xd8\x00\xde\x03'.decode(self.encoding),
                         "\U00010203")

class UTF8Test(ReadTest, unittest.TestCase):
    encoding = "utf-8"

    def test_partial(self):
        self.check_partial(
            "\x00\xff\u07ff\u0800\uffff\U00010000",
            [
                "\x00",
                "\x00",
                "\x00\xff",
                "\x00\xff",
                "\x00\xff\u07ff",
                "\x00\xff\u07ff",
                "\x00\xff\u07ff",
                "\x00\xff\u07ff\u0800",
                "\x00\xff\u07ff\u0800",
                "\x00\xff\u07ff\u0800",
                "\x00\xff\u07ff\u0800\uffff",
                "\x00\xff\u07ff\u0800\uffff",
                "\x00\xff\u07ff\u0800\uffff",
                "\x00\xff\u07ff\u0800\uffff",
                "\x00\xff\u07ff\u0800\uffff\U00010000",
            ]
        )

    def test_decoder_state(self):
        u = "\x00\x7f\x80\xff\u0100\u07ff\u0800\uffff\U0010ffff"
        self.check_state_handling_decode(self.encoding,
                                         u, u.encode(self.encoding))

    def test_lone_surrogates(self):
        self.assertRaises(UnicodeEncodeError, "\ud800".encode, "utf-8")
        self.assertRaises(UnicodeDecodeError, b"\xed\xa0\x80".decode, "utf-8")
        self.assertEqual("[\uDC80]".encode("utf-8", "backslashreplace"),
                         b'[\\udc80]')
        self.assertEqual("[\uDC80]".encode("utf-8", "xmlcharrefreplace"),
                         b'[&#56448;]')
        self.assertEqual("[\uDC80]".encode("utf-8", "surrogateescape"),
                         b'[\x80]')
        self.assertEqual("[\uDC80]".encode("utf-8", "ignore"),
                         b'[]')
        self.assertEqual("[\uDC80]".encode("utf-8", "replace"),
                         b'[?]')

    def test_surrogatepass_handler(self):
        self.assertEqual("abc\ud800def".encode("utf-8", "surrogatepass"),
                         b"abc\xed\xa0\x80def")
        self.assertEqual(b"abc\xed\xa0\x80def".decode("utf-8", "surrogatepass"),
                         "abc\ud800def")
        self.assertEqual("\U00010fff\uD800".encode("utf-8", "surrogatepass"),
                         b"\xf0\x90\xbf\xbf\xed\xa0\x80")
        self.assertEqual(b"\xf0\x90\xbf\xbf\xed\xa0\x80".decode("utf-8", "surrogatepass"),
                         "\U00010fff\uD800")
        self.assertTrue(codecs.lookup_error("surrogatepass"))
        with self.assertRaises(UnicodeDecodeError):
            b"abc\xed\xa0".decode("utf-8", "surrogatepass")
        with self.assertRaises(UnicodeDecodeError):
            b"abc\xed\xa0z".decode("utf-8", "surrogatepass")

@unittest.skipUnless(sys.platform == 'win32',
                     'cp65001 is a Windows-only codec')
class CP65001Test(ReadTest, unittest.TestCase):
    encoding = "cp65001"

    def test_encode(self):
        tests = [
            ('abc', 'strict', b'abc'),
            ('\xe9\u20ac', 'strict',  b'\xc3\xa9\xe2\x82\xac'),
            ('\U0010ffff', 'strict', b'\xf4\x8f\xbf\xbf'),
        ]
        if VISTA_OR_LATER:
            tests.extend((
                ('\udc80', 'strict', None),
                ('\udc80', 'ignore', b''),
                ('\udc80', 'replace', b'?'),
                ('\udc80', 'backslashreplace', b'\\udc80'),
                ('\udc80', 'surrogatepass', b'\xed\xb2\x80'),
            ))
        else:
            tests.append(('\udc80', 'strict', b'\xed\xb2\x80'))
        for text, errors, expected in tests:
            if expected is not None:
                try:
                    encoded = text.encode('cp65001', errors)
                except UnicodeEncodeError as err:
                    self.fail('Unable to encode %a to cp65001 with '
                              'errors=%r: %s' % (text, errors, err))
                self.assertEqual(encoded, expected,
                    '%a.encode("cp65001", %r)=%a != %a'
                    % (text, errors, encoded, expected))
            else:
                self.assertRaises(UnicodeEncodeError,
                    text.encode, "cp65001", errors)

    def test_decode(self):
        tests = [
            (b'abc', 'strict', 'abc'),
            (b'\xc3\xa9\xe2\x82\xac', 'strict', '\xe9\u20ac'),
            (b'\xf4\x8f\xbf\xbf', 'strict', '\U0010ffff'),
            (b'\xef\xbf\xbd', 'strict', '\ufffd'),
            (b'[\xc3\xa9]', 'strict', '[\xe9]'),
            # invalid bytes
            (b'[\xff]', 'strict', None),
            (b'[\xff]', 'ignore', '[]'),
            (b'[\xff]', 'replace', '[\ufffd]'),
            (b'[\xff]', 'surrogateescape', '[\udcff]'),
        ]
        if VISTA_OR_LATER:
            tests.extend((
                (b'[\xed\xb2\x80]', 'strict', None),
                (b'[\xed\xb2\x80]', 'ignore', '[]'),
                (b'[\xed\xb2\x80]', 'replace', '[\ufffd\ufffd\ufffd]'),
            ))
        else:
            tests.extend((
                (b'[\xed\xb2\x80]', 'strict', '[\udc80]'),
            ))
        for raw, errors, expected in tests:
            if expected is not None:
                try:
                    decoded = raw.decode('cp65001', errors)
                except UnicodeDecodeError as err:
                    self.fail('Unable to decode %a from cp65001 with '
                              'errors=%r: %s' % (raw, errors, err))
                self.assertEqual(decoded, expected,
                    '%a.decode("cp65001", %r)=%a != %a'
                    % (raw, errors, decoded, expected))
            else:
                self.assertRaises(UnicodeDecodeError,
                    raw.decode, 'cp65001', errors)

    @unittest.skipUnless(VISTA_OR_LATER, 'require Windows Vista or later')
    def test_lone_surrogates(self):
        self.assertRaises(UnicodeEncodeError, "\ud800".encode, "cp65001")
        self.assertRaises(UnicodeDecodeError, b"\xed\xa0\x80".decode, "cp65001")
        self.assertEqual("[\uDC80]".encode("cp65001", "backslashreplace"),
                         b'[\\udc80]')
        self.assertEqual("[\uDC80]".encode("cp65001", "xmlcharrefreplace"),
                         b'[&#56448;]')
        self.assertEqual("[\uDC80]".encode("cp65001", "surrogateescape"),
                         b'[\x80]')
        self.assertEqual("[\uDC80]".encode("cp65001", "ignore"),
                         b'[]')
        self.assertEqual("[\uDC80]".encode("cp65001", "replace"),
                         b'[?]')

    @unittest.skipUnless(VISTA_OR_LATER, 'require Windows Vista or later')
    def test_surrogatepass_handler(self):
        self.assertEqual("abc\ud800def".encode("cp65001", "surrogatepass"),
                         b"abc\xed\xa0\x80def")
        self.assertEqual(b"abc\xed\xa0\x80def".decode("cp65001", "surrogatepass"),
                         "abc\ud800def")
        self.assertEqual("\U00010fff\uD800".encode("cp65001", "surrogatepass"),
                         b"\xf0\x90\xbf\xbf\xed\xa0\x80")
        self.assertEqual(b"\xf0\x90\xbf\xbf\xed\xa0\x80".decode("cp65001", "surrogatepass"),
                         "\U00010fff\uD800")
        self.assertTrue(codecs.lookup_error("surrogatepass"))



class UTF7Test(ReadTest, unittest.TestCase):
    encoding = "utf-7"

    def test_partial(self):
        self.check_partial(
            "a+-b",
            [
                "a",
                "a",
                "a+",
                "a+-",
                "a+-b",
            ]
        )

class UTF16ExTest(unittest.TestCase):

    def test_errors(self):
        self.assertRaises(UnicodeDecodeError, codecs.utf_16_ex_decode, b"\xff", "strict", 0, True)

    def test_bad_args(self):
        self.assertRaises(TypeError, codecs.utf_16_ex_decode)

class ReadBufferTest(unittest.TestCase):

    def test_array(self):
        import array
        self.assertEqual(
            codecs.readbuffer_encode(array.array("b", b"spam")),
            (b"spam", 4)
        )

    def test_empty(self):
        self.assertEqual(codecs.readbuffer_encode(""), (b"", 0))

    def test_bad_args(self):
        self.assertRaises(TypeError, codecs.readbuffer_encode)
        self.assertRaises(TypeError, codecs.readbuffer_encode, 42)

class UTF8SigTest(ReadTest, unittest.TestCase):
    encoding = "utf-8-sig"

    def test_partial(self):
        self.check_partial(
            "\ufeff\x00\xff\u07ff\u0800\uffff\U00010000",
            [
                "",
                "",
                "", # First BOM has been read and skipped
                "",
                "",
                "\ufeff", # Second BOM has been read and emitted
                "\ufeff\x00", # "\x00" read and emitted
                "\ufeff\x00", # First byte of encoded "\xff" read
                "\ufeff\x00\xff", # Second byte of encoded "\xff" read
                "\ufeff\x00\xff", # First byte of encoded "\u07ff" read
                "\ufeff\x00\xff\u07ff", # Second byte of encoded "\u07ff" read
                "\ufeff\x00\xff\u07ff",
                "\ufeff\x00\xff\u07ff",
                "\ufeff\x00\xff\u07ff\u0800",
                "\ufeff\x00\xff\u07ff\u0800",
                "\ufeff\x00\xff\u07ff\u0800",
                "\ufeff\x00\xff\u07ff\u0800\uffff",
                "\ufeff\x00\xff\u07ff\u0800\uffff",
                "\ufeff\x00\xff\u07ff\u0800\uffff",
                "\ufeff\x00\xff\u07ff\u0800\uffff",
                "\ufeff\x00\xff\u07ff\u0800\uffff\U00010000",
            ]
        )

    def test_bug1601501(self):
        # SF bug #1601501: check that the codec works with a buffer
        self.assertEqual(str(b"\xef\xbb\xbf", "utf-8-sig"), "")

    def test_bom(self):
        d = codecs.getincrementaldecoder("utf-8-sig")()
        s = "spam"
        self.assertEqual(d.decode(s.encode("utf-8-sig")), s)

    def test_stream_bom(self):
        unistring = "ABC\u00A1\u2200XYZ"
        bytestring = codecs.BOM_UTF8 + b"ABC\xC2\xA1\xE2\x88\x80XYZ"

        reader = codecs.getreader("utf-8-sig")
        for sizehint in [None] + list(range(1, 11)) + \
                        [64, 128, 256, 512, 1024]:
            istream = reader(io.BytesIO(bytestring))
            ostream = io.StringIO()
            while 1:
                if sizehint is not None:
                    data = istream.read(sizehint)
                else:
                    data = istream.read()

                if not data:
                    break
                ostream.write(data)

            got = ostream.getvalue()
            self.assertEqual(got, unistring)

    def test_stream_bare(self):
        unistring = "ABC\u00A1\u2200XYZ"
        bytestring = b"ABC\xC2\xA1\xE2\x88\x80XYZ"

        reader = codecs.getreader("utf-8-sig")
        for sizehint in [None] + list(range(1, 11)) + \
                        [64, 128, 256, 512, 1024]:
            istream = reader(io.BytesIO(bytestring))
            ostream = io.StringIO()
            while 1:
                if sizehint is not None:
                    data = istream.read(sizehint)
                else:
                    data = istream.read()

                if not data:
                    break
                ostream.write(data)

            got = ostream.getvalue()
            self.assertEqual(got, unistring)

class EscapeDecodeTest(unittest.TestCase):
    def test_empty(self):
        self.assertEqual(codecs.escape_decode(""), (b"", 0))

    def test_raw(self):
        for b in range(256):
            if b != b'\\'[0]:
                self.assertEqual(codecs.escape_decode(bytes([b]) + b'0'),
                                 (bytes([b]) + b'0', 2))

    def test_escape(self):
        self.assertEqual(codecs.escape_decode(b"[\\\n]"), (b"[]", 4))
        self.assertEqual(codecs.escape_decode(br'[\"]'), (b'["]', 4))
        self.assertEqual(codecs.escape_decode(br"[\']"), (b"[']", 4))
        self.assertEqual(codecs.escape_decode(br"[\\]"), (br"[\]", 4))
        self.assertEqual(codecs.escape_decode(br"[\a]"), (b"[\x07]", 4))
        self.assertEqual(codecs.escape_decode(br"[\b]"), (b"[\x08]", 4))
        self.assertEqual(codecs.escape_decode(br"[\t]"), (b"[\x09]", 4))
        self.assertEqual(codecs.escape_decode(br"[\n]"), (b"[\x0a]", 4))
        self.assertEqual(codecs.escape_decode(br"[\v]"), (b"[\x0b]", 4))
        self.assertEqual(codecs.escape_decode(br"[\f]"), (b"[\x0c]", 4))
        self.assertEqual(codecs.escape_decode(br"[\r]"), (b"[\x0d]", 4))
        self.assertEqual(codecs.escape_decode(br"[\7]"), (b"[\x07]", 4))
        self.assertEqual(codecs.escape_decode(br"[\8]"), (br"[\8]", 4))
        self.assertEqual(codecs.escape_decode(br"[\78]"), (b"[\x078]", 5))
        self.assertEqual(codecs.escape_decode(br"[\41]"), (b"[!]", 5))
        self.assertEqual(codecs.escape_decode(br"[\418]"), (b"[!8]", 6))
        self.assertEqual(codecs.escape_decode(br"[\101]"), (b"[A]", 6))
        self.assertEqual(codecs.escape_decode(br"[\1010]"), (b"[A0]", 7))
        self.assertEqual(codecs.escape_decode(br"[\501]"), (b"[A]", 6))
        self.assertEqual(codecs.escape_decode(br"[\x41]"), (b"[A]", 6))
        self.assertEqual(codecs.escape_decode(br"[\X41]"), (br"[\X41]", 6))
        self.assertEqual(codecs.escape_decode(br"[\x410]"), (b"[A0]", 7))
        for b in range(256):
            if b not in b'\n"\'\\abtnvfr01234567x':
                self.assertEqual(codecs.escape_decode(b'\\' + bytes([b])),
                                 (b'\\' + bytes([b]), 2))

    def test_errors(self):
        self.assertRaises(ValueError, codecs.escape_decode, br"\x")
        self.assertRaises(ValueError, codecs.escape_decode, br"[\x]")
        self.assertEqual(codecs.escape_decode(br"[\x]\x", "ignore"), (b"[]", 6))
        self.assertEqual(codecs.escape_decode(br"[\x]\x", "replace"), (b"[?]?", 6))
        self.assertRaises(ValueError, codecs.escape_decode, br"\x0")
        self.assertRaises(ValueError, codecs.escape_decode, br"[\x0]")
        self.assertEqual(codecs.escape_decode(br"[\x0]\x0", "ignore"), (b"[]", 8))
        self.assertEqual(codecs.escape_decode(br"[\x0]\x0", "replace"), (b"[?]?", 8))

class RecodingTest(unittest.TestCase):
    def test_recoding(self):
        f = io.BytesIO()
        f2 = codecs.EncodedFile(f, "unicode_internal", "utf-8")
        f2.write("a")
        f2.close()
        # Python used to crash on this at exit because of a refcount
        # bug in _codecsmodule.c

# From RFC 3492
punycode_testcases = [
    # A Arabic (Egyptian):
    ("\u0644\u064A\u0647\u0645\u0627\u0628\u062A\u0643\u0644"
     "\u0645\u0648\u0634\u0639\u0631\u0628\u064A\u061F",
     b"egbpdaj6bu4bxfgehfvwxn"),
    # B Chinese (simplified):
    ("\u4ED6\u4EEC\u4E3A\u4EC0\u4E48\u4E0D\u8BF4\u4E2D\u6587",
     b"ihqwcrb4cv8a8dqg056pqjye"),
    # C Chinese (traditional):
    ("\u4ED6\u5011\u7232\u4EC0\u9EBD\u4E0D\u8AAA\u4E2D\u6587",
     b"ihqwctvzc91f659drss3x8bo0yb"),
    # D Czech: Pro<ccaron>prost<ecaron>nemluv<iacute><ccaron>esky
    ("\u0050\u0072\u006F\u010D\u0070\u0072\u006F\u0073\u0074"
     "\u011B\u006E\u0065\u006D\u006C\u0075\u0076\u00ED\u010D"
     "\u0065\u0073\u006B\u0079",
     b"Proprostnemluvesky-uyb24dma41a"),
    # E Hebrew:
    ("\u05DC\u05DE\u05D4\u05D4\u05DD\u05E4\u05E9\u05D5\u05D8"
     "\u05DC\u05D0\u05DE\u05D3\u05D1\u05E8\u05D9\u05DD\u05E2"
     "\u05D1\u05E8\u05D9\u05EA",
     b"4dbcagdahymbxekheh6e0a7fei0b"),
    # F Hindi (Devanagari):
    ("\u092F\u0939\u0932\u094B\u0917\u0939\u093F\u0928\u094D"
     "\u0926\u0940\u0915\u094D\u092F\u094B\u0902\u0928\u0939"
     "\u0940\u0902\u092C\u094B\u0932\u0938\u0915\u0924\u0947"
     "\u0939\u0948\u0902",
     b"i1baa7eci9glrd9b2ae1bj0hfcgg6iyaf8o0a1dig0cd"),

    #(G) Japanese (kanji and hiragana):
    ("\u306A\u305C\u307F\u3093\u306A\u65E5\u672C\u8A9E\u3092"
     "\u8A71\u3057\u3066\u304F\u308C\u306A\u3044\u306E\u304B",
     b"n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa"),

    # (H) Korean (Hangul syllables):
    ("\uC138\uACC4\uC758\uBAA8\uB4E0\uC0AC\uB78C\uB4E4\uC774"
     "\uD55C\uAD6D\uC5B4\uB97C\uC774\uD574\uD55C\uB2E4\uBA74"
     "\uC5BC\uB9C8\uB098\uC88B\uC744\uAE4C",
     b"989aomsvi5e83db1d2a355cv1e0vak1dwrv93d5xbh15a0dt30a5j"
     b"psd879ccm6fea98c"),

    # (I) Russian (Cyrillic):
    ("\u043F\u043E\u0447\u0435\u043C\u0443\u0436\u0435\u043E"
     "\u043D\u0438\u043D\u0435\u0433\u043E\u0432\u043E\u0440"
     "\u044F\u0442\u043F\u043E\u0440\u0443\u0441\u0441\u043A"
     "\u0438",
     b"b1abfaaepdrnnbgefbaDotcwatmq2g4l"),

    # (J) Spanish: Porqu<eacute>nopuedensimplementehablarenEspa<ntilde>ol
    ("\u0050\u006F\u0072\u0071\u0075\u00E9\u006E\u006F\u0070"
     "\u0075\u0065\u0064\u0065\u006E\u0073\u0069\u006D\u0070"
     "\u006C\u0065\u006D\u0065\u006E\u0074\u0065\u0068\u0061"
     "\u0062\u006C\u0061\u0072\u0065\u006E\u0045\u0073\u0070"
     "\u0061\u00F1\u006F\u006C",
     b"PorqunopuedensimplementehablarenEspaol-fmd56a"),

    # (K) Vietnamese:
    #  T<adotbelow>isaoh<odotbelow>kh<ocirc>ngth<ecirchookabove>ch\
    #   <ihookabove>n<oacute>iti<ecircacute>ngVi<ecircdotbelow>t
    ("\u0054\u1EA1\u0069\u0073\u0061\u006F\u0068\u1ECD\u006B"
     "\u0068\u00F4\u006E\u0067\u0074\u0068\u1EC3\u0063\u0068"
     "\u1EC9\u006E\u00F3\u0069\u0074\u0069\u1EBF\u006E\u0067"
     "\u0056\u0069\u1EC7\u0074",
     b"TisaohkhngthchnitingVit-kjcr8268qyxafd2f1b9g"),

    #(L) 3<nen>B<gumi><kinpachi><sensei>
    ("\u0033\u5E74\u0042\u7D44\u91D1\u516B\u5148\u751F",
     b"3B-ww4c5e180e575a65lsy2b"),

    # (M) <amuro><namie>-with-SUPER-MONKEYS
    ("\u5B89\u5BA4\u5948\u7F8E\u6075\u002D\u0077\u0069\u0074"
     "\u0068\u002D\u0053\u0055\u0050\u0045\u0052\u002D\u004D"
     "\u004F\u004E\u004B\u0045\u0059\u0053",
     b"-with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n"),

    # (N) Hello-Another-Way-<sorezore><no><basho>
    ("\u0048\u0065\u006C\u006C\u006F\u002D\u0041\u006E\u006F"
     "\u0074\u0068\u0065\u0072\u002D\u0057\u0061\u0079\u002D"
     "\u305D\u308C\u305E\u308C\u306E\u5834\u6240",
     b"Hello-Another-Way--fc4qua05auwb3674vfr0b"),

    # (O) <hitotsu><yane><no><shita>2
    ("\u3072\u3068\u3064\u5C4B\u6839\u306E\u4E0B\u0032",
     b"2-u9tlzr9756bt3uc0v"),

    # (P) Maji<de>Koi<suru>5<byou><mae>
    ("\u004D\u0061\u006A\u0069\u3067\u004B\u006F\u0069\u3059"
     "\u308B\u0035\u79D2\u524D",
     b"MajiKoi5-783gue6qz075azm5e"),

     # (Q) <pafii>de<runba>
    ("\u30D1\u30D5\u30A3\u30FC\u0064\u0065\u30EB\u30F3\u30D0",
     b"de-jg4avhby1noc0d"),

    # (R) <sono><supiido><de>
    ("\u305D\u306E\u30B9\u30D4\u30FC\u30C9\u3067",
     b"d9juau41awczczp"),

    # (S) -> $1.00 <-
    ("\u002D\u003E\u0020\u0024\u0031\u002E\u0030\u0030\u0020"
     "\u003C\u002D",
     b"-> $1.00 <--")
    ]

for i in punycode_testcases:
    if len(i)!=2:
        print(repr(i))

class PunycodeTest(unittest.TestCase):
    def test_encode(self):
        for uni, puny in punycode_testcases:
            # Need to convert both strings to lower case, since
            # some of the extended encodings use upper case, but our
            # code produces only lower case. Converting just puny to
            # lower is also insufficient, since some of the input characters
            # are upper case.
            self.assertEqual(
                str(uni.encode("punycode"), "ascii").lower(),
                str(puny, "ascii").lower()
            )

    def test_decode(self):
        for uni, puny in punycode_testcases:
            self.assertEqual(uni, puny.decode("punycode"))
            puny = puny.decode("ascii").encode("ascii")
            self.assertEqual(uni, puny.decode("punycode"))

class UnicodeInternalTest(unittest.TestCase):
    @unittest.skipUnless(SIZEOF_WCHAR_T == 4, 'specific to 32-bit wchar_t')
    def test_bug1251300(self):
        # Decoding with unicode_internal used to not correctly handle "code
        # points" above 0x10ffff on UCS-4 builds.
        ok = [
            (b"\x00\x10\xff\xff", "\U0010ffff"),
            (b"\x00\x00\x01\x01", "\U00000101"),
            (b"", ""),
        ]
        not_ok = [
            b"\x7f\xff\xff\xff",
            b"\x80\x00\x00\x00",
            b"\x81\x00\x00\x00",
            b"\x00",
            b"\x00\x00\x00\x00\x00",
        ]
        for internal, uni in ok:
            if sys.byteorder == "little":
                internal = bytes(reversed(internal))
            with support.check_warnings():
                self.assertEqual(uni, internal.decode("unicode_internal"))
        for internal in not_ok:
            if sys.byteorder == "little":
                internal = bytes(reversed(internal))
            with support.check_warnings(('unicode_internal codec has been '
                                         'deprecated', DeprecationWarning)):
                self.assertRaises(UnicodeDecodeError, internal.decode,
                                  "unicode_internal")
        if sys.byteorder == "little":
            invalid = b"\x00\x00\x11\x00"
        else:
            invalid = b"\x00\x11\x00\x00"
        with support.check_warnings():
            self.assertRaises(UnicodeDecodeError,
                              invalid.decode, "unicode_internal")
        with support.check_warnings():
            self.assertEqual(invalid.decode("unicode_internal", "replace"),
                             '\ufffd')

    @unittest.skipUnless(SIZEOF_WCHAR_T == 4, 'specific to 32-bit wchar_t')
    def test_decode_error_attributes(self):
        try:
            with support.check_warnings(('unicode_internal codec has been '
                                         'deprecated', DeprecationWarning)):
                b"\x00\x00\x00\x00\x00\x11\x11\x00".decode("unicode_internal")
        except UnicodeDecodeError as ex:
            self.assertEqual("unicode_internal", ex.encoding)
            self.assertEqual(b"\x00\x00\x00\x00\x00\x11\x11\x00", ex.object)
            self.assertEqual(4, ex.start)
            self.assertEqual(8, ex.end)
        else:
            self.fail()

    @unittest.skipUnless(SIZEOF_WCHAR_T == 4, 'specific to 32-bit wchar_t')
    def test_decode_callback(self):
        codecs.register_error("UnicodeInternalTest", codecs.ignore_errors)
        decoder = codecs.getdecoder("unicode_internal")
        with support.check_warnings(('unicode_internal codec has been '
                                     'deprecated', DeprecationWarning)):
            ab = "ab".encode("unicode_internal").decode()
            ignored = decoder(bytes("%s\x22\x22\x22\x22%s" % (ab[:4], ab[4:]),
                                    "ascii"),
                              "UnicodeInternalTest")
        self.assertEqual(("ab", 12), ignored)

    def test_encode_length(self):
        with support.check_warnings(('unicode_internal codec has been '
                                     'deprecated', DeprecationWarning)):
            # Issue 3739
            encoder = codecs.getencoder("unicode_internal")
            self.assertEqual(encoder("a")[1], 1)
            self.assertEqual(encoder("\xe9\u0142")[1], 2)

            self.assertEqual(codecs.escape_encode(br'\x00')[1], 4)

# From http://www.gnu.org/software/libidn/draft-josefsson-idn-test-vectors.html
nameprep_tests = [
    # 3.1 Map to nothing.
    (b'foo\xc2\xad\xcd\x8f\xe1\xa0\x86\xe1\xa0\x8bbar'
     b'\xe2\x80\x8b\xe2\x81\xa0baz\xef\xb8\x80\xef\xb8\x88\xef'
     b'\xb8\x8f\xef\xbb\xbf',
     b'foobarbaz'),
    # 3.2 Case folding ASCII U+0043 U+0041 U+0046 U+0045.
    (b'CAFE',
     b'cafe'),
    # 3.3 Case folding 8bit U+00DF (german sharp s).
    # The original test case is bogus; it says \xc3\xdf
    (b'\xc3\x9f',
     b'ss'),
    # 3.4 Case folding U+0130 (turkish capital I with dot).
    (b'\xc4\xb0',
     b'i\xcc\x87'),
    # 3.5 Case folding multibyte U+0143 U+037A.
    (b'\xc5\x83\xcd\xba',
     b'\xc5\x84 \xce\xb9'),
    # 3.6 Case folding U+2121 U+33C6 U+1D7BB.
    # XXX: skip this as it fails in UCS-2 mode
    #('\xe2\x84\xa1\xe3\x8f\x86\xf0\x9d\x9e\xbb',
    # 'telc\xe2\x88\x95kg\xcf\x83'),
    (None, None),
    # 3.7 Normalization of U+006a U+030c U+00A0 U+00AA.
    (b'j\xcc\x8c\xc2\xa0\xc2\xaa',
     b'\xc7\xb0 a'),
    # 3.8 Case folding U+1FB7 and normalization.
    (b'\xe1\xbe\xb7',
     b'\xe1\xbe\xb6\xce\xb9'),
    # 3.9 Self-reverting case folding U+01F0 and normalization.
    # The original test case is bogus, it says `\xc7\xf0'
    (b'\xc7\xb0',
     b'\xc7\xb0'),
    # 3.10 Self-reverting case folding U+0390 and normalization.
    (b'\xce\x90',
     b'\xce\x90'),
    # 3.11 Self-reverting case folding U+03B0 and normalization.
    (b'\xce\xb0',
     b'\xce\xb0'),
    # 3.12 Self-reverting case folding U+1E96 and normalization.
    (b'\xe1\xba\x96',
     b'\xe1\xba\x96'),
    # 3.13 Self-reverting case folding U+1F56 and normalization.
    (b'\xe1\xbd\x96',
     b'\xe1\xbd\x96'),
    # 3.14 ASCII space character U+0020.
    (b' ',
     b' '),
    # 3.15 Non-ASCII 8bit space character U+00A0.
    (b'\xc2\xa0',
     b' '),
    # 3.16 Non-ASCII multibyte space character U+1680.
    (b'\xe1\x9a\x80',
     None),
    # 3.17 Non-ASCII multibyte space character U+2000.
    (b'\xe2\x80\x80',
     b' '),
    # 3.18 Zero Width Space U+200b.
    (b'\xe2\x80\x8b',
     b''),
    # 3.19 Non-ASCII multibyte space character U+3000.
    (b'\xe3\x80\x80',
     b' '),
    # 3.20 ASCII control characters U+0010 U+007F.
    (b'\x10\x7f',
     b'\x10\x7f'),
    # 3.21 Non-ASCII 8bit control character U+0085.
    (b'\xc2\x85',
     None),
    # 3.22 Non-ASCII multibyte control character U+180E.
    (b'\xe1\xa0\x8e',
     None),
    # 3.23 Zero Width No-Break Space U+FEFF.
    (b'\xef\xbb\xbf',
     b''),
    # 3.24 Non-ASCII control character U+1D175.
    (b'\xf0\x9d\x85\xb5',
     None),
    # 3.25 Plane 0 private use character U+F123.
    (b'\xef\x84\xa3',
     None),
    # 3.26 Plane 15 private use character U+F1234.
    (b'\xf3\xb1\x88\xb4',
     None),
    # 3.27 Plane 16 private use character U+10F234.
    (b'\xf4\x8f\x88\xb4',
     None),
    # 3.28 Non-character code point U+8FFFE.
    (b'\xf2\x8f\xbf\xbe',
     None),
    # 3.29 Non-character code point U+10FFFF.
    (b'\xf4\x8f\xbf\xbf',
     None),
    # 3.30 Surrogate code U+DF42.
    (b'\xed\xbd\x82',
     None),
    # 3.31 Non-plain text character U+FFFD.
    (b'\xef\xbf\xbd',
     None),
    # 3.32 Ideographic description character U+2FF5.
    (b'\xe2\xbf\xb5',
     None),
    # 3.33 Display property character U+0341.
    (b'\xcd\x81',
     b'\xcc\x81'),
    # 3.34 Left-to-right mark U+200E.
    (b'\xe2\x80\x8e',
     None),
    # 3.35 Deprecated U+202A.
    (b'\xe2\x80\xaa',
     None),
    # 3.36 Language tagging character U+E0001.
    (b'\xf3\xa0\x80\x81',
     None),
    # 3.37 Language tagging character U+E0042.
    (b'\xf3\xa0\x81\x82',
     None),
    # 3.38 Bidi: RandALCat character U+05BE and LCat characters.
    (b'foo\xd6\xbebar',
     None),
    # 3.39 Bidi: RandALCat character U+FD50 and LCat characters.
    (b'foo\xef\xb5\x90bar',
     None),
    # 3.40 Bidi: RandALCat character U+FB38 and LCat characters.
    (b'foo\xef\xb9\xb6bar',
     b'foo \xd9\x8ebar'),
    # 3.41 Bidi: RandALCat without trailing RandALCat U+0627 U+0031.
    (b'\xd8\xa71',
     None),
    # 3.42 Bidi: RandALCat character U+0627 U+0031 U+0628.
    (b'\xd8\xa71\xd8\xa8',
     b'\xd8\xa71\xd8\xa8'),
    # 3.43 Unassigned code point U+E0002.
    # Skip this test as we allow unassigned
    #(b'\xf3\xa0\x80\x82',
    # None),
    (None, None),
    # 3.44 Larger test (shrinking).
    # Original test case reads \xc3\xdf
    (b'X\xc2\xad\xc3\x9f\xc4\xb0\xe2\x84\xa1j\xcc\x8c\xc2\xa0\xc2'
     b'\xaa\xce\xb0\xe2\x80\x80',
     b'xssi\xcc\x87tel\xc7\xb0 a\xce\xb0 '),
    # 3.45 Larger test (expanding).
    # Original test case reads \xc3\x9f
    (b'X\xc3\x9f\xe3\x8c\x96\xc4\xb0\xe2\x84\xa1\xe2\x92\x9f\xe3\x8c'
     b'\x80',
     b'xss\xe3\x82\xad\xe3\x83\xad\xe3\x83\xa1\xe3\x83\xbc\xe3'
     b'\x83\x88\xe3\x83\xabi\xcc\x87tel\x28d\x29\xe3\x82'
     b'\xa2\xe3\x83\x91\xe3\x83\xbc\xe3\x83\x88')
    ]


class NameprepTest(unittest.TestCase):
    def test_nameprep(self):
        from encodings.idna import nameprep
        for pos, (orig, prepped) in enumerate(nameprep_tests):
            if orig is None:
                # Skipped
                continue
            # The Unicode strings are given in UTF-8
            orig = str(orig, "utf-8", "surrogatepass")
            if prepped is None:
                # Input contains prohibited characters
                self.assertRaises(UnicodeError, nameprep, orig)
            else:
                prepped = str(prepped, "utf-8", "surrogatepass")
                try:
                    self.assertEqual(nameprep(orig), prepped)
                except Exception as e:
                    raise support.TestFailed("Test 3.%d: %s" % (pos+1, str(e)))

class IDNACodecTest(unittest.TestCase):
    def test_builtin_decode(self):
        self.assertEqual(str(b"python.org", "idna"), "python.org")
        self.assertEqual(str(b"python.org.", "idna"), "python.org.")
        self.assertEqual(str(b"xn--pythn-mua.org", "idna"), "pyth\xf6n.org")
        self.assertEqual(str(b"xn--pythn-mua.org.", "idna"), "pyth\xf6n.org.")

    def test_builtin_encode(self):
        self.assertEqual("python.org".encode("idna"), b"python.org")
        self.assertEqual("python.org.".encode("idna"), b"python.org.")
        self.assertEqual("pyth\xf6n.org".encode("idna"), b"xn--pythn-mua.org")
        self.assertEqual("pyth\xf6n.org.".encode("idna"), b"xn--pythn-mua.org.")

    def test_stream(self):
        r = codecs.getreader("idna")(io.BytesIO(b"abc"))
        r.read(3)
        self.assertEqual(r.read(), "")

    def test_incremental_decode(self):
        self.assertEqual(
            "".join(codecs.iterdecode((bytes([c]) for c in b"python.org"), "idna")),
            "python.org"
        )
        self.assertEqual(
            "".join(codecs.iterdecode((bytes([c]) for c in b"python.org."), "idna")),
            "python.org."
        )
        self.assertEqual(
            "".join(codecs.iterdecode((bytes([c]) for c in b"xn--pythn-mua.org."), "idna")),
            "pyth\xf6n.org."
        )
        self.assertEqual(
            "".join(codecs.iterdecode((bytes([c]) for c in b"xn--pythn-mua.org."), "idna")),
            "pyth\xf6n.org."
        )

        decoder = codecs.getincrementaldecoder("idna")()
        self.assertEqual(decoder.decode(b"xn--xam", ), "")
        self.assertEqual(decoder.decode(b"ple-9ta.o", ), "\xe4xample.")
        self.assertEqual(decoder.decode(b"rg"), "")
        self.assertEqual(decoder.decode(b"", True), "org")

        decoder.reset()
        self.assertEqual(decoder.decode(b"xn--xam", ), "")
        self.assertEqual(decoder.decode(b"ple-9ta.o", ), "\xe4xample.")
        self.assertEqual(decoder.decode(b"rg."), "org.")
        self.assertEqual(decoder.decode(b"", True), "")

    def test_incremental_encode(self):
        self.assertEqual(
            b"".join(codecs.iterencode("python.org", "idna")),
            b"python.org"
        )
        self.assertEqual(
            b"".join(codecs.iterencode("python.org.", "idna")),
            b"python.org."
        )
        self.assertEqual(
            b"".join(codecs.iterencode("pyth\xf6n.org.", "idna")),
            b"xn--pythn-mua.org."
        )
        self.assertEqual(
            b"".join(codecs.iterencode("pyth\xf6n.org.", "idna")),
            b"xn--pythn-mua.org."
        )

        encoder = codecs.getincrementalencoder("idna")()
        self.assertEqual(encoder.encode("\xe4x"), b"")
        self.assertEqual(encoder.encode("ample.org"), b"xn--xample-9ta.")
        self.assertEqual(encoder.encode("", True), b"org")

        encoder.reset()
        self.assertEqual(encoder.encode("\xe4x"), b"")
        self.assertEqual(encoder.encode("ample.org."), b"xn--xample-9ta.org.")
        self.assertEqual(encoder.encode("", True), b"")

class CodecsModuleTest(unittest.TestCase):

    def test_decode(self):
        self.assertEqual(codecs.decode(b'\xe4\xf6\xfc', 'latin-1'),
                         '\xe4\xf6\xfc')
        self.assertRaises(TypeError, codecs.decode)
        self.assertEqual(codecs.decode(b'abc'), 'abc')
        self.assertRaises(UnicodeDecodeError, codecs.decode, b'\xff', 'ascii')

    def test_encode(self):
        self.assertEqual(codecs.encode('\xe4\xf6\xfc', 'latin-1'),
                         b'\xe4\xf6\xfc')
        self.assertRaises(TypeError, codecs.encode)
        self.assertRaises(LookupError, codecs.encode, "foo", "__spam__")
        self.assertEqual(codecs.encode('abc'), b'abc')
        self.assertRaises(UnicodeEncodeError, codecs.encode, '\xffff', 'ascii')

    def test_register(self):
        self.assertRaises(TypeError, codecs.register)
        self.assertRaises(TypeError, codecs.register, 42)

    def test_lookup(self):
        self.assertRaises(TypeError, codecs.lookup)
        self.assertRaises(LookupError, codecs.lookup, "__spam__")
        self.assertRaises(LookupError, codecs.lookup, " ")

    def test_getencoder(self):
        self.assertRaises(TypeError, codecs.getencoder)
        self.assertRaises(LookupError, codecs.getencoder, "__spam__")

    def test_getdecoder(self):
        self.assertRaises(TypeError, codecs.getdecoder)
        self.assertRaises(LookupError, codecs.getdecoder, "__spam__")

    def test_getreader(self):
        self.assertRaises(TypeError, codecs.getreader)
        self.assertRaises(LookupError, codecs.getreader, "__spam__")

    def test_getwriter(self):
        self.assertRaises(TypeError, codecs.getwriter)
        self.assertRaises(LookupError, codecs.getwriter, "__spam__")

    def test_lookup_issue1813(self):
        # Issue #1813: under Turkish locales, lookup of some codecs failed
        # because 'I' is lowercased as "ı" (dotless i)
        oldlocale = locale.setlocale(locale.LC_CTYPE)
        self.addCleanup(locale.setlocale, locale.LC_CTYPE, oldlocale)
        try:
            locale.setlocale(locale.LC_CTYPE, 'tr_TR')
        except locale.Error:
            # Unsupported locale on this system
            self.skipTest('test needs Turkish locale')
        c = codecs.lookup('ASCII')
        self.assertEqual(c.name, 'ascii')

class StreamReaderTest(unittest.TestCase):

    def setUp(self):
        self.reader = codecs.getreader('utf-8')
        self.stream = io.BytesIO(b'\xed\x95\x9c\n\xea\xb8\x80')

    def test_readlines(self):
        f = self.reader(self.stream)
        self.assertEqual(f.readlines(), ['\ud55c\n', '\uae00'])

class EncodedFileTest(unittest.TestCase):

    def test_basic(self):
        f = io.BytesIO(b'\xed\x95\x9c\n\xea\xb8\x80')
        ef = codecs.EncodedFile(f, 'utf-16-le', 'utf-8')
        self.assertEqual(ef.read(), b'\\\xd5\n\x00\x00\xae')

        f = io.BytesIO()
        ef = codecs.EncodedFile(f, 'utf-8', 'latin-1')
        ef.write(b'\xc3\xbc')
        self.assertEqual(f.getvalue(), b'\xfc')

all_unicode_encodings = [
    "ascii",
    "big5",
    "big5hkscs",
    "charmap",
    "cp037",
    "cp1006",
    "cp1026",
    "cp1140",
    "cp1250",
    "cp1251",
    "cp1252",
    "cp1253",
    "cp1254",
    "cp1255",
    "cp1256",
    "cp1257",
    "cp1258",
    "cp424",
    "cp437",
    "cp500",
    "cp720",
    "cp737",
    "cp775",
    "cp850",
    "cp852",
    "cp855",
    "cp856",
    "cp857",
    "cp858",
    "cp860",
    "cp861",
    "cp862",
    "cp863",
    "cp864",
    "cp865",
    "cp866",
    "cp869",
    "cp874",
    "cp875",
    "cp932",
    "cp949",
    "cp950",
    "euc_jis_2004",
    "euc_jisx0213",
    "euc_jp",
    "euc_kr",
    "gb18030",
    "gb2312",
    "gbk",
    "hp_roman8",
    "hz",
    "idna",
    "iso2022_jp",
    "iso2022_jp_1",
    "iso2022_jp_2",
    "iso2022_jp_2004",
    "iso2022_jp_3",
    "iso2022_jp_ext",
    "iso2022_kr",
    "iso8859_1",
    "iso8859_10",
    "iso8859_11",
    "iso8859_13",
    "iso8859_14",
    "iso8859_15",
    "iso8859_16",
    "iso8859_2",
    "iso8859_3",
    "iso8859_4",
    "iso8859_5",
    "iso8859_6",
    "iso8859_7",
    "iso8859_8",
    "iso8859_9",
    "johab",
    "koi8_r",
    "koi8_u",
    "latin_1",
    "mac_cyrillic",
    "mac_greek",
    "mac_iceland",
    "mac_latin2",
    "mac_roman",
    "mac_turkish",
    "palmos",
    "ptcp154",
    "punycode",
    "raw_unicode_escape",
    "shift_jis",
    "shift_jis_2004",
    "shift_jisx0213",
    "tis_620",
    "unicode_escape",
    "unicode_internal",
    "utf_16",
    "utf_16_be",
    "utf_16_le",
    "utf_7",
    "utf_8",
]

if hasattr(codecs, "mbcs_encode"):
    all_unicode_encodings.append("mbcs")

# The following encoding is not tested, because it's not supposed
# to work:
#    "undefined"

# The following encodings don't work in stateful mode
broken_unicode_with_streams = [
    "punycode",
    "unicode_internal"
]
broken_incremental_coders = broken_unicode_with_streams + [
    "idna",
]

class BasicUnicodeTest(unittest.TestCase, MixInCheckStateHandling):
    def test_basics(self):
        s = "abc123" # all codecs should be able to encode these
        for encoding in all_unicode_encodings:
            name = codecs.lookup(encoding).name
            if encoding.endswith("_codec"):
                name += "_codec"
            elif encoding == "latin_1":
                name = "latin_1"
            self.assertEqual(encoding.replace("_", "-"), name.replace("_", "-"))

            with support.check_warnings():
                # unicode-internal has been deprecated
                (b, size) = codecs.getencoder(encoding)(s)
                self.assertEqual(size, len(s), "%r != %r (encoding=%r)" % (size, len(s), encoding))
                (chars, size) = codecs.getdecoder(encoding)(b)
                self.assertEqual(chars, s, "%r != %r (encoding=%r)" % (chars, s, encoding))

            if encoding not in broken_unicode_with_streams:
                # check stream reader/writer
                q = Queue(b"")
                writer = codecs.getwriter(encoding)(q)
                encodedresult = b""
                for c in s:
                    writer.write(c)
                    chunk = q.read()
                    self.assertTrue(type(chunk) is bytes, type(chunk))
                    encodedresult += chunk
                q = Queue(b"")
                reader = codecs.getreader(encoding)(q)
                decodedresult = ""
                for c in encodedresult:
                    q.write(bytes([c]))
                    decodedresult += reader.read()
                self.assertEqual(decodedresult, s, "%r != %r (encoding=%r)" % (decodedresult, s, encoding))

            if encoding not in broken_incremental_coders:
                # check incremental decoder/encoder (fetched via the Python
                # and C API) and iterencode()/iterdecode()
                try:
                    encoder = codecs.getincrementalencoder(encoding)()
                    cencoder = _testcapi.codec_incrementalencoder(encoding)
                except LookupError: # no IncrementalEncoder
                    pass
                else:
                    # check incremental decoder/encoder
                    encodedresult = b""
                    for c in s:
                        encodedresult += encoder.encode(c)
                    encodedresult += encoder.encode("", True)
                    decoder = codecs.getincrementaldecoder(encoding)()
                    decodedresult = ""
                    for c in encodedresult:
                        decodedresult += decoder.decode(bytes([c]))
                    decodedresult += decoder.decode(b"", True)
                    self.assertEqual(decodedresult, s, "%r != %r (encoding=%r)" % (decodedresult, s, encoding))

                    # check C API
                    encodedresult = b""
                    for c in s:
                        encodedresult += cencoder.encode(c)
                    encodedresult += cencoder.encode("", True)
                    cdecoder = _testcapi.codec_incrementaldecoder(encoding)
                    decodedresult = ""
                    for c in encodedresult:
                        decodedresult += cdecoder.decode(bytes([c]))
                    decodedresult += cdecoder.decode(b"", True)
                    self.assertEqual(decodedresult, s, "%r != %r (encoding=%r)" % (decodedresult, s, encoding))

                    # check iterencode()/iterdecode()
                    result = "".join(codecs.iterdecode(codecs.iterencode(s, encoding), encoding))
                    self.assertEqual(result, s, "%r != %r (encoding=%r)" % (result, s, encoding))

                    # check iterencode()/iterdecode() with empty string
                    result = "".join(codecs.iterdecode(codecs.iterencode("", encoding), encoding))
                    self.assertEqual(result, "")

                if encoding not in ("idna", "mbcs"):
                    # check incremental decoder/encoder with errors argument
                    try:
                        encoder = codecs.getincrementalencoder(encoding)("ignore")
                        cencoder = _testcapi.codec_incrementalencoder(encoding, "ignore")
                    except LookupError: # no IncrementalEncoder
                        pass
                    else:
                        encodedresult = b"".join(encoder.encode(c) for c in s)
                        decoder = codecs.getincrementaldecoder(encoding)("ignore")
                        decodedresult = "".join(decoder.decode(bytes([c])) for c in encodedresult)
                        self.assertEqual(decodedresult, s, "%r != %r (encoding=%r)" % (decodedresult, s, encoding))

                        encodedresult = b"".join(cencoder.encode(c) for c in s)
                        cdecoder = _testcapi.codec_incrementaldecoder(encoding, "ignore")
                        decodedresult = "".join(cdecoder.decode(bytes([c])) for c in encodedresult)
                        self.assertEqual(decodedresult, s, "%r != %r (encoding=%r)" % (decodedresult, s, encoding))

    def test_seek(self):
        # all codecs should be able to encode these
        s = "%s\n%s\n" % (100*"abc123", 100*"def456")
        for encoding in all_unicode_encodings:
            if encoding == "idna": # FIXME: See SF bug #1163178
                continue
            if encoding in broken_unicode_with_streams:
                continue
            reader = codecs.getreader(encoding)(io.BytesIO(s.encode(encoding)))
            for t in range(5):
                # Test that calling seek resets the internal codec state and buffers
                reader.seek(0, 0)
                data = reader.read()
                self.assertEqual(s, data)

    def test_bad_decode_args(self):
        for encoding in all_unicode_encodings:
            decoder = codecs.getdecoder(encoding)
            self.assertRaises(TypeError, decoder)
            if encoding not in ("idna", "punycode"):
                self.assertRaises(TypeError, decoder, 42)

    def test_bad_encode_args(self):
        for encoding in all_unicode_encodings:
            encoder = codecs.getencoder(encoding)
            with support.check_warnings():
                # unicode-internal has been deprecated
                self.assertRaises(TypeError, encoder)

    def test_encoding_map_type_initialized(self):
        from encodings import cp1140
        # This used to crash, we are only verifying there's no crash.
        table_type = type(cp1140.encoding_table)
        self.assertEqual(table_type, table_type)

    def test_decoder_state(self):
        # Check that getstate() and setstate() handle the state properly
        u = "abc123"
        for encoding in all_unicode_encodings:
            if encoding not in broken_incremental_coders:
                self.check_state_handling_decode(encoding, u, u.encode(encoding))
                self.check_state_handling_encode(encoding, u, u.encode(encoding))

class CharmapTest(unittest.TestCase):
    def test_decode_with_string_map(self):
        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "strict", "abc"),
            ("abc", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "strict", "\U0010FFFFbc"),
            ("\U0010FFFFbc", 3)
        )

        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, b"\x00\x01\x02", "strict", "ab"
        )

        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, b"\x00\x01\x02", "strict", "ab\ufffe"
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "replace", "ab"),
            ("ab\ufffd", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "replace", "ab\ufffe"),
            ("ab\ufffd", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "ignore", "ab"),
            ("ab", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "ignore", "ab\ufffe"),
            ("ab", 3)
        )

        allbytes = bytes(range(256))
        self.assertEqual(
            codecs.charmap_decode(allbytes, "ignore", ""),
            ("", len(allbytes))
        )

    def test_decode_with_int2str_map(self):
        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "strict",
                                  {0: 'a', 1: 'b', 2: 'c'}),
            ("abc", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "strict",
                                  {0: 'Aa', 1: 'Bb', 2: 'Cc'}),
            ("AaBbCc", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "strict",
                                  {0: '\U0010FFFF', 1: 'b', 2: 'c'}),
            ("\U0010FFFFbc", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "strict",
                                  {0: 'a', 1: 'b', 2: ''}),
            ("ab", 3)
        )

        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, b"\x00\x01\x02", "strict",
                                   {0: 'a', 1: 'b'}
        )

        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, b"\x00\x01\x02", "strict",
                                   {0: 'a', 1: 'b', 2: None}
        )

        # Issue #14850
        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, b"\x00\x01\x02", "strict",
                                   {0: 'a', 1: 'b', 2: '\ufffe'}
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "replace",
                                  {0: 'a', 1: 'b'}),
            ("ab\ufffd", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "replace",
                                  {0: 'a', 1: 'b', 2: None}),
            ("ab\ufffd", 3)
        )

        # Issue #14850
        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "replace",
                                  {0: 'a', 1: 'b', 2: '\ufffe'}),
            ("ab\ufffd", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "ignore",
                                  {0: 'a', 1: 'b'}),
            ("ab", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "ignore",
                                  {0: 'a', 1: 'b', 2: None}),
            ("ab", 3)
        )

        # Issue #14850
        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "ignore",
                                  {0: 'a', 1: 'b', 2: '\ufffe'}),
            ("ab", 3)
        )

        allbytes = bytes(range(256))
        self.assertEqual(
            codecs.charmap_decode(allbytes, "ignore", {}),
            ("", len(allbytes))
        )

    def test_decode_with_int2int_map(self):
        a = ord('a')
        b = ord('b')
        c = ord('c')

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "strict",
                                  {0: a, 1: b, 2: c}),
            ("abc", 3)
        )

        # Issue #15379
        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "strict",
                                  {0: 0x10FFFF, 1: b, 2: c}),
            ("\U0010FFFFbc", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "strict",
                                  {0: sys.maxunicode, 1: b, 2: c}),
            (chr(sys.maxunicode) + "bc", 3)
        )

        self.assertRaises(TypeError,
            codecs.charmap_decode, b"\x00\x01\x02", "strict",
                                   {0: sys.maxunicode + 1, 1: b, 2: c}
        )

        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, b"\x00\x01\x02", "strict",
                                   {0: a, 1: b},
        )

        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, b"\x00\x01\x02", "strict",
                                   {0: a, 1: b, 2: 0xFFFE},
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "replace",
                                  {0: a, 1: b}),
            ("ab\ufffd", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "replace",
                                  {0: a, 1: b, 2: 0xFFFE}),
            ("ab\ufffd", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "ignore",
                                  {0: a, 1: b}),
            ("ab", 3)
        )

        self.assertEqual(
            codecs.charmap_decode(b"\x00\x01\x02", "ignore",
                                  {0: a, 1: b, 2: 0xFFFE}),
            ("ab", 3)
        )


class WithStmtTest(unittest.TestCase):
    def test_encodedfile(self):
        f = io.BytesIO(b"\xc3\xbc")
        with codecs.EncodedFile(f, "latin-1", "utf-8") as ef:
            self.assertEqual(ef.read(), b"\xfc")

    def test_streamreaderwriter(self):
        f = io.BytesIO(b"\xc3\xbc")
        info = codecs.lookup("utf-8")
        with codecs.StreamReaderWriter(f, info.streamreader,
                                       info.streamwriter, 'strict') as srw:
            self.assertEqual(srw.read(), "\xfc")

class TypesTest(unittest.TestCase):
    def test_decode_unicode(self):
        # Most decoders don't accept unicode input
        decoders = [
            codecs.utf_7_decode,
            codecs.utf_8_decode,
            codecs.utf_16_le_decode,
            codecs.utf_16_be_decode,
            codecs.utf_16_ex_decode,
            codecs.utf_32_decode,
            codecs.utf_32_le_decode,
            codecs.utf_32_be_decode,
            codecs.utf_32_ex_decode,
            codecs.latin_1_decode,
            codecs.ascii_decode,
            codecs.charmap_decode,
        ]
        if hasattr(codecs, "mbcs_decode"):
            decoders.append(codecs.mbcs_decode)
        for decoder in decoders:
            self.assertRaises(TypeError, decoder, "xxx")

    def test_unicode_escape(self):
        # Escape-decoding an unicode string is supported ang gives the same
        # result as decoding the equivalent ASCII bytes string.
        self.assertEqual(codecs.unicode_escape_decode(r"\u1234"), ("\u1234", 6))
        self.assertEqual(codecs.unicode_escape_decode(br"\u1234"), ("\u1234", 6))
        self.assertEqual(codecs.raw_unicode_escape_decode(r"\u1234"), ("\u1234", 6))
        self.assertEqual(codecs.raw_unicode_escape_decode(br"\u1234"), ("\u1234", 6))

        self.assertRaises(UnicodeDecodeError, codecs.unicode_escape_decode, br"\U00110000")
        self.assertEqual(codecs.unicode_escape_decode(r"\U00110000", "replace"), ("\ufffd", 10))

        self.assertRaises(UnicodeDecodeError, codecs.raw_unicode_escape_decode, br"\U00110000")
        self.assertEqual(codecs.raw_unicode_escape_decode(r"\U00110000", "replace"), ("\ufffd", 10))


class UnicodeEscapeTest(unittest.TestCase):
    def test_empty(self):
        self.assertEqual(codecs.unicode_escape_encode(""), (b"", 0))
        self.assertEqual(codecs.unicode_escape_decode(b""), ("", 0))

    def test_raw_encode(self):
        encode = codecs.unicode_escape_encode
        for b in range(32, 127):
            if b != b'\\'[0]:
                self.assertEqual(encode(chr(b)), (bytes([b]), 1))

    def test_raw_decode(self):
        decode = codecs.unicode_escape_decode
        for b in range(256):
            if b != b'\\'[0]:
                self.assertEqual(decode(bytes([b]) + b'0'), (chr(b) + '0', 2))

    def test_escape_encode(self):
        encode = codecs.unicode_escape_encode
        check = coding_checker(self, encode)
        check('\t', br'\t')
        check('\n', br'\n')
        check('\r', br'\r')
        check('\\', br'\\')
        for b in range(32):
            if chr(b) not in '\t\n\r':
                check(chr(b), ('\\x%02x' % b).encode())
        for b in range(127, 256):
            check(chr(b), ('\\x%02x' % b).encode())
        check('\u20ac', br'\u20ac')
        check('\U0001d120', br'\U0001d120')

    def test_escape_decode(self):
        decode = codecs.unicode_escape_decode
        check = coding_checker(self, decode)
        check(b"[\\\n]", "[]")
        check(br'[\"]', '["]')
        check(br"[\']", "[']")
        check(br"[\\]", r"[\]")
        check(br"[\a]", "[\x07]")
        check(br"[\b]", "[\x08]")
        check(br"[\t]", "[\x09]")
        check(br"[\n]", "[\x0a]")
        check(br"[\v]", "[\x0b]")
        check(br"[\f]", "[\x0c]")
        check(br"[\r]", "[\x0d]")
        check(br"[\7]", "[\x07]")
        check(br"[\8]", r"[\8]")
        check(br"[\78]", "[\x078]")
        check(br"[\41]", "[!]")
        check(br"[\418]", "[!8]")
        check(br"[\101]", "[A]")
        check(br"[\1010]", "[A0]")
        check(br"[\x41]", "[A]")
        check(br"[\x410]", "[A0]")
        check(br"\u20ac", "\u20ac")
        check(br"\U0001d120", "\U0001d120")
        for b in range(256):
            if b not in b'\n"\'\\abtnvfr01234567xuUN':
                check(b'\\' + bytes([b]), '\\' + chr(b))

    def test_decode_errors(self):
        decode = codecs.unicode_escape_decode
        for c, d in (b'x', 2), (b'u', 4), (b'U', 4):
            for i in range(d):
                self.assertRaises(UnicodeDecodeError, decode,
                                  b"\\" + c + b"0"*i)
                self.assertRaises(UnicodeDecodeError, decode,
                                  b"[\\" + c + b"0"*i + b"]")
                data = b"[\\" + c + b"0"*i + b"]\\" + c + b"0"*i
                self.assertEqual(decode(data, "ignore"), ("[]", len(data)))
                self.assertEqual(decode(data, "replace"),
                                 ("[\ufffd]\ufffd", len(data)))
        self.assertRaises(UnicodeDecodeError, decode, br"\U00110000")
        self.assertEqual(decode(br"\U00110000", "ignore"), ("", 10))
        self.assertEqual(decode(br"\U00110000", "replace"), ("\ufffd", 10))


class SurrogateEscapeTest(unittest.TestCase):

    def test_utf8(self):
        # Bad byte
        self.assertEqual(b"foo\x80bar".decode("utf-8", "surrogateescape"),
                         "foo\udc80bar")
        self.assertEqual("foo\udc80bar".encode("utf-8", "surrogateescape"),
                         b"foo\x80bar")
        # bad-utf-8 encoded surrogate
        self.assertEqual(b"\xed\xb0\x80".decode("utf-8", "surrogateescape"),
                         "\udced\udcb0\udc80")
        self.assertEqual("\udced\udcb0\udc80".encode("utf-8", "surrogateescape"),
                         b"\xed\xb0\x80")

    def test_ascii(self):
        # bad byte
        self.assertEqual(b"foo\x80bar".decode("ascii", "surrogateescape"),
                         "foo\udc80bar")
        self.assertEqual("foo\udc80bar".encode("ascii", "surrogateescape"),
                         b"foo\x80bar")

    def test_charmap(self):
        # bad byte: \xa5 is unmapped in iso-8859-3
        self.assertEqual(b"foo\xa5bar".decode("iso-8859-3", "surrogateescape"),
                         "foo\udca5bar")
        self.assertEqual("foo\udca5bar".encode("iso-8859-3", "surrogateescape"),
                         b"foo\xa5bar")

    def test_latin1(self):
        # Issue6373
        self.assertEqual("\udce4\udceb\udcef\udcf6\udcfc".encode("latin-1", "surrogateescape"),
                         b"\xe4\xeb\xef\xf6\xfc")


class BomTest(unittest.TestCase):
    def test_seek0(self):
        data = "1234567890"
        tests = ("utf-16",
                 "utf-16-le",
                 "utf-16-be",
                 "utf-32",
                 "utf-32-le",
                 "utf-32-be")
        self.addCleanup(support.unlink, support.TESTFN)
        for encoding in tests:
            # Check if the BOM is written only once
            with codecs.open(support.TESTFN, 'w+', encoding=encoding) as f:
                f.write(data)
                f.write(data)
                f.seek(0)
                self.assertEqual(f.read(), data * 2)
                f.seek(0)
                self.assertEqual(f.read(), data * 2)

            # Check that the BOM is written after a seek(0)
            with codecs.open(support.TESTFN, 'w+', encoding=encoding) as f:
                f.write(data[0])
                self.assertNotEqual(f.tell(), 0)
                f.seek(0)
                f.write(data)
                f.seek(0)
                self.assertEqual(f.read(), data)

            # (StreamWriter) Check that the BOM is written after a seek(0)
            with codecs.open(support.TESTFN, 'w+', encoding=encoding) as f:
                f.writer.write(data[0])
                self.assertNotEqual(f.writer.tell(), 0)
                f.writer.seek(0)
                f.writer.write(data)
                f.seek(0)
                self.assertEqual(f.read(), data)

            # Check that the BOM is not written after a seek() at a position
            # different than the start
            with codecs.open(support.TESTFN, 'w+', encoding=encoding) as f:
                f.write(data)
                f.seek(f.tell())
                f.write(data)
                f.seek(0)
                self.assertEqual(f.read(), data * 2)

            # (StreamWriter) Check that the BOM is not written after a seek()
            # at a position different than the start
            with codecs.open(support.TESTFN, 'w+', encoding=encoding) as f:
                f.writer.write(data)
                f.writer.seek(f.writer.tell())
                f.writer.write(data)
                f.seek(0)
                self.assertEqual(f.read(), data * 2)


bytes_transform_encodings = [
    "base64_codec",
    "uu_codec",
    "quopri_codec",
    "hex_codec",
]
try:
    import zlib
except ImportError:
    pass
else:
    bytes_transform_encodings.append("zlib_codec")
try:
    import bz2
except ImportError:
    pass
else:
    bytes_transform_encodings.append("bz2_codec")

class TransformCodecTest(unittest.TestCase):

    def test_basics(self):
        binput = bytes(range(256))
        for encoding in bytes_transform_encodings:
            # generic codecs interface
            (o, size) = codecs.getencoder(encoding)(binput)
            self.assertEqual(size, len(binput))
            (i, size) = codecs.getdecoder(encoding)(o)
            self.assertEqual(size, len(o))
            self.assertEqual(i, binput)

    def test_read(self):
        for encoding in bytes_transform_encodings:
            sin = codecs.encode(b"\x80", encoding)
            reader = codecs.getreader(encoding)(io.BytesIO(sin))
            sout = reader.read()
            self.assertEqual(sout, b"\x80")

    def test_readline(self):
        for encoding in bytes_transform_encodings:
            if encoding in ['uu_codec', 'zlib_codec']:
                continue
            sin = codecs.encode(b"\x80", encoding)
            reader = codecs.getreader(encoding)(io.BytesIO(sin))
            sout = reader.readline()
            self.assertEqual(sout, b"\x80")


@unittest.skipUnless(sys.platform == 'win32',
                     'code pages are specific to Windows')
class CodePageTest(unittest.TestCase):
    # CP_UTF8 is already tested by CP65001Test
    CP_UTF8 = 65001

    def test_invalid_code_page(self):
        self.assertRaises(ValueError, codecs.code_page_encode, -1, 'a')
        self.assertRaises(ValueError, codecs.code_page_decode, -1, b'a')
        self.assertRaises(WindowsError, codecs.code_page_encode, 123, 'a')
        self.assertRaises(WindowsError, codecs.code_page_decode, 123, b'a')

    def test_code_page_name(self):
        self.assertRaisesRegex(UnicodeEncodeError, 'cp932',
            codecs.code_page_encode, 932, '\xff')
        self.assertRaisesRegex(UnicodeDecodeError, 'cp932',
            codecs.code_page_decode, 932, b'\x81\x00')
        self.assertRaisesRegex(UnicodeDecodeError, 'CP_UTF8',
            codecs.code_page_decode, self.CP_UTF8, b'\xff')

    def check_decode(self, cp, tests):
        for raw, errors, expected in tests:
            if expected is not None:
                try:
                    decoded = codecs.code_page_decode(cp, raw, errors)
                except UnicodeDecodeError as err:
                    self.fail('Unable to decode %a from "cp%s" with '
                              'errors=%r: %s' % (raw, cp, errors, err))
                self.assertEqual(decoded[0], expected,
                    '%a.decode("cp%s", %r)=%a != %a'
                    % (raw, cp, errors, decoded[0], expected))
                # assert 0 <= decoded[1] <= len(raw)
                self.assertGreaterEqual(decoded[1], 0)
                self.assertLessEqual(decoded[1], len(raw))
            else:
                self.assertRaises(UnicodeDecodeError,
                    codecs.code_page_decode, cp, raw, errors)

    def check_encode(self, cp, tests):
        for text, errors, expected in tests:
            if expected is not None:
                try:
                    encoded = codecs.code_page_encode(cp, text, errors)
                except UnicodeEncodeError as err:
                    self.fail('Unable to encode %a to "cp%s" with '
                              'errors=%r: %s' % (text, cp, errors, err))
                self.assertEqual(encoded[0], expected,
                    '%a.encode("cp%s", %r)=%a != %a'
                    % (text, cp, errors, encoded[0], expected))
                self.assertEqual(encoded[1], len(text))
            else:
                self.assertRaises(UnicodeEncodeError,
                    codecs.code_page_encode, cp, text, errors)

    def test_cp932(self):
        self.check_encode(932, (
            ('abc', 'strict', b'abc'),
            ('\uff44\u9a3e', 'strict', b'\x82\x84\xe9\x80'),
            # test error handlers
            ('\xff', 'strict', None),
            ('[\xff]', 'ignore', b'[]'),
            ('[\xff]', 'replace', b'[y]'),
            ('[\u20ac]', 'replace', b'[?]'),
            ('[\xff]', 'backslashreplace', b'[\\xff]'),
            ('[\xff]', 'xmlcharrefreplace', b'[&#255;]'),
        ))
        self.check_decode(932, (
            (b'abc', 'strict', 'abc'),
            (b'\x82\x84\xe9\x80', 'strict', '\uff44\u9a3e'),
            # invalid bytes
            (b'[\xff]', 'strict', None),
            (b'[\xff]', 'ignore', '[]'),
            (b'[\xff]', 'replace', '[\ufffd]'),
            (b'[\xff]', 'surrogateescape', '[\udcff]'),
            (b'\x81\x00abc', 'strict', None),
            (b'\x81\x00abc', 'ignore', '\x00abc'),
            (b'\x81\x00abc', 'replace', '\ufffd\x00abc'),
        ))

    def test_cp1252(self):
        self.check_encode(1252, (
            ('abc', 'strict', b'abc'),
            ('\xe9\u20ac', 'strict',  b'\xe9\x80'),
            ('\xff', 'strict', b'\xff'),
            ('\u0141', 'strict', None),
            ('\u0141', 'ignore', b''),
            ('\u0141', 'replace', b'L'),
        ))
        self.check_decode(1252, (
            (b'abc', 'strict', 'abc'),
            (b'\xe9\x80', 'strict', '\xe9\u20ac'),
            (b'\xff', 'strict', '\xff'),
        ))

    def test_cp_utf7(self):
        cp = 65000
        self.check_encode(cp, (
            ('abc', 'strict', b'abc'),
            ('\xe9\u20ac', 'strict',  b'+AOkgrA-'),
            ('\U0010ffff', 'strict',  b'+2//f/w-'),
            ('\udc80', 'strict', b'+3IA-'),
            ('\ufffd', 'strict', b'+//0-'),
        ))
        self.check_decode(cp, (
            (b'abc', 'strict', 'abc'),
            (b'+AOkgrA-', 'strict', '\xe9\u20ac'),
            (b'+2//f/w-', 'strict', '\U0010ffff'),
            (b'+3IA-', 'strict', '\udc80'),
            (b'+//0-', 'strict', '\ufffd'),
            # invalid bytes
            (b'[+/]', 'strict', '[]'),
            (b'[\xff]', 'strict', '[\xff]'),
        ))

    def test_multibyte_encoding(self):
        self.check_decode(932, (
            (b'\x84\xe9\x80', 'ignore', '\u9a3e'),
            (b'\x84\xe9\x80', 'replace', '\ufffd\u9a3e'),
        ))
        self.check_decode(self.CP_UTF8, (
            (b'\xff\xf4\x8f\xbf\xbf', 'ignore', '\U0010ffff'),
            (b'\xff\xf4\x8f\xbf\xbf', 'replace', '\ufffd\U0010ffff'),
        ))
        if VISTA_OR_LATER:
            self.check_encode(self.CP_UTF8, (
                ('[\U0010ffff\uDC80]', 'ignore', b'[\xf4\x8f\xbf\xbf]'),
                ('[\U0010ffff\uDC80]', 'replace', b'[\xf4\x8f\xbf\xbf?]'),
            ))

    def test_incremental(self):
        decoded = codecs.code_page_decode(932, b'\x82', 'strict', False)
        self.assertEqual(decoded, ('', 0))

        decoded = codecs.code_page_decode(932,
                                          b'\xe9\x80\xe9', 'strict',
                                          False)
        self.assertEqual(decoded, ('\u9a3e', 2))

        decoded = codecs.code_page_decode(932,
                                          b'\xe9\x80\xe9\x80', 'strict',
                                          False)
        self.assertEqual(decoded, ('\u9a3e\u9a3e', 4))

        decoded = codecs.code_page_decode(932,
                                          b'abc', 'strict',
                                          False)
        self.assertEqual(decoded, ('abc', 3))


if __name__ == "__main__":
    unittest.main()
