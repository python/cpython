from test import test_support
import unittest
import codecs
import locale
import sys, StringIO, _testcapi

class Queue(object):
    """
    queue: write bytes at one end, read bytes from the other end
    """
    def __init__(self):
        self._buffer = ""

    def write(self, chars):
        self._buffer += chars

    def read(self, size=-1):
        if size<0:
            s = self._buffer
            self._buffer = ""
            return s
        else:
            s = self._buffer[:size]
            self._buffer = self._buffer[size:]
            return s

class ReadTest(unittest.TestCase):
    def check_partial(self, input, partialresults):
        # get a StreamReader for the encoding and feed the bytestring version
        # of input to the reader byte by byte. Read everything available from
        # the StreamReader and check that the results equal the appropriate
        # entries from partialresults.
        q = Queue()
        r = codecs.getreader(self.encoding)(q)
        result = u""
        for (c, partialresult) in zip(input.encode(self.encoding), partialresults):
            q.write(c)
            result += r.read()
            self.assertEqual(result, partialresult)
        # check that there's nothing left in the buffers
        self.assertEqual(r.read(), u"")
        self.assertEqual(r.bytebuffer, "")
        self.assertEqual(r.charbuffer, u"")

        # do the check again, this time using a incremental decoder
        d = codecs.getincrementaldecoder(self.encoding)()
        result = u""
        for (c, partialresult) in zip(input.encode(self.encoding), partialresults):
            result += d.decode(c)
            self.assertEqual(result, partialresult)
        # check that there's nothing left in the buffers
        self.assertEqual(d.decode("", True), u"")
        self.assertEqual(d.buffer, "")

        # Check whether the reset method works properly
        d.reset()
        result = u""
        for (c, partialresult) in zip(input.encode(self.encoding), partialresults):
            result += d.decode(c)
            self.assertEqual(result, partialresult)
        # check that there's nothing left in the buffers
        self.assertEqual(d.decode("", True), u"")
        self.assertEqual(d.buffer, "")

        # check iterdecode()
        encoded = input.encode(self.encoding)
        self.assertEqual(
            input,
            u"".join(codecs.iterdecode(encoded, self.encoding))
        )

    def test_readline(self):
        def getreader(input):
            stream = StringIO.StringIO(input.encode(self.encoding))
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

        s = u"foo\nbar\r\nbaz\rspam\u2028eggs"
        sexpected = u"foo\n|bar\r\n|baz\r|spam\u2028|eggs"
        sexpectednoends = u"foo|bar|baz|spam|eggs"
        self.assertEqual(readalllines(s, True), sexpected)
        self.assertEqual(readalllines(s, False), sexpectednoends)
        self.assertEqual(readalllines(s, True, 10), sexpected)
        self.assertEqual(readalllines(s, False, 10), sexpectednoends)

        # Test long lines (multiple calls to read() in readline())
        vw = []
        vwo = []
        for (i, lineend) in enumerate(u"\n \r\n \r \u2028".split()):
            vw.append((i*200)*u"\3042" + lineend)
            vwo.append((i*200)*u"\3042")
        self.assertEqual(readalllines("".join(vw), True), "".join(vw))
        self.assertEqual(readalllines("".join(vw), False),"".join(vwo))

        # Test lines where the first read might end with \r, so the
        # reader has to look ahead whether this is a lone \r or a \r\n
        for size in xrange(80):
            for lineend in u"\n \r\n \r \u2028".split():
                s = 10*(size*u"a" + lineend + u"xxx\n")
                reader = getreader(s)
                for i in xrange(10):
                    self.assertEqual(
                        reader.readline(keepends=True),
                        size*u"a" + lineend,
                    )
                reader = getreader(s)
                for i in xrange(10):
                    self.assertEqual(
                        reader.readline(keepends=False),
                        size*u"a",
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
        stream = StringIO.StringIO("".join(s).encode(self.encoding))
        reader = codecs.getreader(self.encoding)(stream)
        for (i, line) in enumerate(reader):
            self.assertEqual(line, s[i])

    def test_readlinequeue(self):
        q = Queue()
        writer = codecs.getwriter(self.encoding)(q)
        reader = codecs.getreader(self.encoding)(q)

        # No lineends
        writer.write(u"foo\r")
        self.assertEqual(reader.readline(keepends=False), u"foo")
        writer.write(u"\nbar\r")
        self.assertEqual(reader.readline(keepends=False), u"")
        self.assertEqual(reader.readline(keepends=False), u"bar")
        writer.write(u"baz")
        self.assertEqual(reader.readline(keepends=False), u"baz")
        self.assertEqual(reader.readline(keepends=False), u"")

        # Lineends
        writer.write(u"foo\r")
        self.assertEqual(reader.readline(keepends=True), u"foo\r")
        writer.write(u"\nbar\r")
        self.assertEqual(reader.readline(keepends=True), u"\n")
        self.assertEqual(reader.readline(keepends=True), u"bar\r")
        writer.write(u"baz")
        self.assertEqual(reader.readline(keepends=True), u"baz")
        self.assertEqual(reader.readline(keepends=True), u"")
        writer.write(u"foo\r\n")
        self.assertEqual(reader.readline(keepends=True), u"foo\r\n")

    def test_bug1098990_a(self):
        s1 = u"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\r\n"
        s2 = u"offending line: ladfj askldfj klasdj fskla dfzaskdj fasklfj laskd fjasklfzzzzaa%whereisthis!!!\r\n"
        s3 = u"next line.\r\n"

        s = (s1+s2+s3).encode(self.encoding)
        stream = StringIO.StringIO(s)
        reader = codecs.getreader(self.encoding)(stream)
        self.assertEqual(reader.readline(), s1)
        self.assertEqual(reader.readline(), s2)
        self.assertEqual(reader.readline(), s3)
        self.assertEqual(reader.readline(), u"")

    def test_bug1098990_b(self):
        s1 = u"aaaaaaaaaaaaaaaaaaaaaaaa\r\n"
        s2 = u"bbbbbbbbbbbbbbbbbbbbbbbb\r\n"
        s3 = u"stillokay:bbbbxx\r\n"
        s4 = u"broken!!!!badbad\r\n"
        s5 = u"againokay.\r\n"

        s = (s1+s2+s3+s4+s5).encode(self.encoding)
        stream = StringIO.StringIO(s)
        reader = codecs.getreader(self.encoding)(stream)
        self.assertEqual(reader.readline(), s1)
        self.assertEqual(reader.readline(), s2)
        self.assertEqual(reader.readline(), s3)
        self.assertEqual(reader.readline(), s4)
        self.assertEqual(reader.readline(), s5)
        self.assertEqual(reader.readline(), u"")

class UTF32Test(ReadTest):
    encoding = "utf-32"

    spamle = ('\xff\xfe\x00\x00'
              's\x00\x00\x00p\x00\x00\x00a\x00\x00\x00m\x00\x00\x00'
              's\x00\x00\x00p\x00\x00\x00a\x00\x00\x00m\x00\x00\x00')
    spambe = ('\x00\x00\xfe\xff'
              '\x00\x00\x00s\x00\x00\x00p\x00\x00\x00a\x00\x00\x00m'
              '\x00\x00\x00s\x00\x00\x00p\x00\x00\x00a\x00\x00\x00m')

    def test_only_one_bom(self):
        _,_,reader,writer = codecs.lookup(self.encoding)
        # encode some stream
        s = StringIO.StringIO()
        f = writer(s)
        f.write(u"spam")
        f.write(u"spam")
        d = s.getvalue()
        # check whether there is exactly one BOM in it
        self.assertTrue(d == self.spamle or d == self.spambe)
        # try to read it back
        s = StringIO.StringIO(d)
        f = reader(s)
        self.assertEqual(f.read(), u"spamspam")

    def test_badbom(self):
        s = StringIO.StringIO(4*"\xff")
        f = codecs.getreader(self.encoding)(s)
        self.assertRaises(UnicodeError, f.read)

        s = StringIO.StringIO(8*"\xff")
        f = codecs.getreader(self.encoding)(s)
        self.assertRaises(UnicodeError, f.read)

    def test_partial(self):
        self.check_partial(
            u"\x00\xff\u0100\uffff\U00010000",
            [
                u"", # first byte of BOM read
                u"", # second byte of BOM read
                u"", # third byte of BOM read
                u"", # fourth byte of BOM read => byteorder known
                u"",
                u"",
                u"",
                u"\x00",
                u"\x00",
                u"\x00",
                u"\x00",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff\U00010000",
            ]
        )

    def test_handlers(self):
        self.assertEqual((u'\ufffd', 1),
                         codecs.utf_32_decode('\x01', 'replace', True))
        self.assertEqual((u'', 1),
                         codecs.utf_32_decode('\x01', 'ignore', True))

    def test_errors(self):
        self.assertRaises(UnicodeDecodeError, codecs.utf_32_decode,
                          "\xff", "strict", True)

    def test_issue8941(self):
        # Issue #8941: insufficient result allocation when decoding into
        # surrogate pairs on UCS-2 builds.
        encoded_le = '\xff\xfe\x00\x00' + '\x00\x00\x01\x00' * 1024
        self.assertEqual(u'\U00010000' * 1024,
                         codecs.utf_32_decode(encoded_le)[0])
        encoded_be = '\x00\x00\xfe\xff' + '\x00\x01\x00\x00' * 1024
        self.assertEqual(u'\U00010000' * 1024,
                         codecs.utf_32_decode(encoded_be)[0])

class UTF32LETest(ReadTest):
    encoding = "utf-32-le"

    def test_partial(self):
        self.check_partial(
            u"\x00\xff\u0100\uffff\U00010000",
            [
                u"",
                u"",
                u"",
                u"\x00",
                u"\x00",
                u"\x00",
                u"\x00",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff\U00010000",
            ]
        )

    def test_simple(self):
        self.assertEqual(u"\U00010203".encode(self.encoding), "\x03\x02\x01\x00")

    def test_errors(self):
        self.assertRaises(UnicodeDecodeError, codecs.utf_32_le_decode,
                          "\xff", "strict", True)

    def test_issue8941(self):
        # Issue #8941: insufficient result allocation when decoding into
        # surrogate pairs on UCS-2 builds.
        encoded = '\x00\x00\x01\x00' * 1024
        self.assertEqual(u'\U00010000' * 1024,
                         codecs.utf_32_le_decode(encoded)[0])

class UTF32BETest(ReadTest):
    encoding = "utf-32-be"

    def test_partial(self):
        self.check_partial(
            u"\x00\xff\u0100\uffff\U00010000",
            [
                u"",
                u"",
                u"",
                u"\x00",
                u"\x00",
                u"\x00",
                u"\x00",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff\U00010000",
            ]
        )

    def test_simple(self):
        self.assertEqual(u"\U00010203".encode(self.encoding), "\x00\x01\x02\x03")

    def test_errors(self):
        self.assertRaises(UnicodeDecodeError, codecs.utf_32_be_decode,
                          "\xff", "strict", True)

    def test_issue8941(self):
        # Issue #8941: insufficient result allocation when decoding into
        # surrogate pairs on UCS-2 builds.
        encoded = '\x00\x01\x00\x00' * 1024
        self.assertEqual(u'\U00010000' * 1024,
                         codecs.utf_32_be_decode(encoded)[0])


class UTF16Test(ReadTest):
    encoding = "utf-16"

    spamle = '\xff\xfes\x00p\x00a\x00m\x00s\x00p\x00a\x00m\x00'
    spambe = '\xfe\xff\x00s\x00p\x00a\x00m\x00s\x00p\x00a\x00m'

    def test_only_one_bom(self):
        _,_,reader,writer = codecs.lookup(self.encoding)
        # encode some stream
        s = StringIO.StringIO()
        f = writer(s)
        f.write(u"spam")
        f.write(u"spam")
        d = s.getvalue()
        # check whether there is exactly one BOM in it
        self.assertTrue(d == self.spamle or d == self.spambe)
        # try to read it back
        s = StringIO.StringIO(d)
        f = reader(s)
        self.assertEqual(f.read(), u"spamspam")

    def test_badbom(self):
        s = StringIO.StringIO("\xff\xff")
        f = codecs.getreader(self.encoding)(s)
        self.assertRaises(UnicodeError, f.read)

        s = StringIO.StringIO("\xff\xff\xff\xff")
        f = codecs.getreader(self.encoding)(s)
        self.assertRaises(UnicodeError, f.read)

    def test_partial(self):
        self.check_partial(
            u"\x00\xff\u0100\uffff\U00010000",
            [
                u"", # first byte of BOM read
                u"", # second byte of BOM read => byteorder known
                u"",
                u"\x00",
                u"\x00",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff\U00010000",
            ]
        )

    def test_handlers(self):
        self.assertEqual((u'\ufffd', 1),
                         codecs.utf_16_decode('\x01', 'replace', True))
        self.assertEqual((u'', 1),
                         codecs.utf_16_decode('\x01', 'ignore', True))

    def test_errors(self):
        self.assertRaises(UnicodeDecodeError, codecs.utf_16_decode, "\xff", "strict", True)

    def test_bug691291(self):
        # Files are always opened in binary mode, even if no binary mode was
        # specified.  This means that no automatic conversion of '\n' is done
        # on reading and writing.
        s1 = u'Hello\r\nworld\r\n'

        s = s1.encode(self.encoding)
        self.addCleanup(test_support.unlink, test_support.TESTFN)
        with open(test_support.TESTFN, 'wb') as fp:
            fp.write(s)
        with codecs.open(test_support.TESTFN, 'U', encoding=self.encoding) as reader:
            self.assertEqual(reader.read(), s1)

class UTF16LETest(ReadTest):
    encoding = "utf-16-le"

    def test_partial(self):
        self.check_partial(
            u"\x00\xff\u0100\uffff\U00010000",
            [
                u"",
                u"\x00",
                u"\x00",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff\U00010000",
            ]
        )

    def test_errors(self):
        tests = [
            (b'\xff', u'\ufffd'),
            (b'A\x00Z', u'A\ufffd'),
            (b'A\x00B\x00C\x00D\x00Z', u'ABCD\ufffd'),
            (b'\x00\xd8', u'\ufffd'),
            (b'\x00\xd8A', u'\ufffd'),
            (b'\x00\xd8A\x00', u'\ufffdA'),
            (b'\x00\xdcA\x00', u'\ufffdA'),
        ]
        for raw, expected in tests:
            self.assertRaises(UnicodeDecodeError, codecs.utf_16_le_decode,
                              raw, 'strict', True)
            self.assertEqual(raw.decode('utf-16le', 'replace'), expected)

class UTF16BETest(ReadTest):
    encoding = "utf-16-be"

    def test_partial(self):
        self.check_partial(
            u"\x00\xff\u0100\uffff\U00010000",
            [
                u"",
                u"\x00",
                u"\x00",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff",
                u"\x00\xff\u0100\uffff\U00010000",
            ]
        )

    def test_errors(self):
        tests = [
            (b'\xff', u'\ufffd'),
            (b'\x00A\xff', u'A\ufffd'),
            (b'\x00A\x00B\x00C\x00DZ', u'ABCD\ufffd'),
            (b'\xd8\x00', u'\ufffd'),
            (b'\xd8\x00\xdc', u'\ufffd'),
            (b'\xd8\x00\x00A', u'\ufffdA'),
            (b'\xdc\x00\x00A', u'\ufffdA'),
        ]
        for raw, expected in tests:
            self.assertRaises(UnicodeDecodeError, codecs.utf_16_be_decode,
                              raw, 'strict', True)
            self.assertEqual(raw.decode('utf-16be', 'replace'), expected)

class UTF8Test(ReadTest):
    encoding = "utf-8"

    def test_partial(self):
        self.check_partial(
            u"\x00\xff\u07ff\u0800\uffff\U00010000",
            [
                u"\x00",
                u"\x00",
                u"\x00\xff",
                u"\x00\xff",
                u"\x00\xff\u07ff",
                u"\x00\xff\u07ff",
                u"\x00\xff\u07ff",
                u"\x00\xff\u07ff\u0800",
                u"\x00\xff\u07ff\u0800",
                u"\x00\xff\u07ff\u0800",
                u"\x00\xff\u07ff\u0800\uffff",
                u"\x00\xff\u07ff\u0800\uffff",
                u"\x00\xff\u07ff\u0800\uffff",
                u"\x00\xff\u07ff\u0800\uffff",
                u"\x00\xff\u07ff\u0800\uffff\U00010000",
            ]
        )

class UTF7Test(ReadTest):
    encoding = "utf-7"

    def test_partial(self):
        self.check_partial(
            u"a+-b",
            [
                u"a",
                u"a",
                u"a+",
                u"a+-",
                u"a+-b",
            ]
        )

class UTF16ExTest(unittest.TestCase):

    def test_errors(self):
        self.assertRaises(UnicodeDecodeError, codecs.utf_16_ex_decode, "\xff", "strict", 0, True)

    def test_bad_args(self):
        self.assertRaises(TypeError, codecs.utf_16_ex_decode)

class ReadBufferTest(unittest.TestCase):

    def test_array(self):
        import array
        self.assertEqual(
            codecs.readbuffer_encode(array.array("c", "spam")),
            ("spam", 4)
        )

    def test_empty(self):
        self.assertEqual(codecs.readbuffer_encode(""), ("", 0))

    def test_bad_args(self):
        self.assertRaises(TypeError, codecs.readbuffer_encode)
        self.assertRaises(TypeError, codecs.readbuffer_encode, 42)

class CharBufferTest(unittest.TestCase):

    def test_string(self):
        self.assertEqual(codecs.charbuffer_encode("spam"), ("spam", 4))

    def test_empty(self):
        self.assertEqual(codecs.charbuffer_encode(""), ("", 0))

    def test_bad_args(self):
        self.assertRaises(TypeError, codecs.charbuffer_encode)
        self.assertRaises(TypeError, codecs.charbuffer_encode, 42)

class UTF8SigTest(ReadTest):
    encoding = "utf-8-sig"

    def test_partial(self):
        self.check_partial(
            u"\ufeff\x00\xff\u07ff\u0800\uffff\U00010000",
            [
                u"",
                u"",
                u"", # First BOM has been read and skipped
                u"",
                u"",
                u"\ufeff", # Second BOM has been read and emitted
                u"\ufeff\x00", # "\x00" read and emitted
                u"\ufeff\x00", # First byte of encoded u"\xff" read
                u"\ufeff\x00\xff", # Second byte of encoded u"\xff" read
                u"\ufeff\x00\xff", # First byte of encoded u"\u07ff" read
                u"\ufeff\x00\xff\u07ff", # Second byte of encoded u"\u07ff" read
                u"\ufeff\x00\xff\u07ff",
                u"\ufeff\x00\xff\u07ff",
                u"\ufeff\x00\xff\u07ff\u0800",
                u"\ufeff\x00\xff\u07ff\u0800",
                u"\ufeff\x00\xff\u07ff\u0800",
                u"\ufeff\x00\xff\u07ff\u0800\uffff",
                u"\ufeff\x00\xff\u07ff\u0800\uffff",
                u"\ufeff\x00\xff\u07ff\u0800\uffff",
                u"\ufeff\x00\xff\u07ff\u0800\uffff",
                u"\ufeff\x00\xff\u07ff\u0800\uffff\U00010000",
            ]
        )

    def test_bug1601501(self):
        # SF bug #1601501: check that the codec works with a buffer
        unicode("\xef\xbb\xbf", "utf-8-sig")

    def test_bom(self):
        d = codecs.getincrementaldecoder("utf-8-sig")()
        s = u"spam"
        self.assertEqual(d.decode(s.encode("utf-8-sig")), s)

    def test_stream_bom(self):
        unistring = u"ABC\u00A1\u2200XYZ"
        bytestring = codecs.BOM_UTF8 + "ABC\xC2\xA1\xE2\x88\x80XYZ"

        reader = codecs.getreader("utf-8-sig")
        for sizehint in [None] + range(1, 11) + \
                        [64, 128, 256, 512, 1024]:
            istream = reader(StringIO.StringIO(bytestring))
            ostream = StringIO.StringIO()
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
        unistring = u"ABC\u00A1\u2200XYZ"
        bytestring = "ABC\xC2\xA1\xE2\x88\x80XYZ"

        reader = codecs.getreader("utf-8-sig")
        for sizehint in [None] + range(1, 11) + \
                        [64, 128, 256, 512, 1024]:
            istream = reader(StringIO.StringIO(bytestring))
            ostream = StringIO.StringIO()
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
        self.assertEqual(codecs.escape_decode(""), ("", 0))

    def test_raw(self):
        for b in ''.join(map(chr, range(256))):
            if b != '\\':
                self.assertEqual(codecs.escape_decode(b + '0'),
                                 (b + '0', 2))

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
        for b in ''.join(map(chr, range(256))):
            if b not in '\n"\'\\abtnvfr01234567x':
                self.assertEqual(codecs.escape_decode('\\' + b),
                                 ('\\' + b, 2))

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
        f = StringIO.StringIO()
        f2 = codecs.EncodedFile(f, "unicode_internal", "utf-8")
        f2.write(u"a")
        f2.close()
        # Python used to crash on this at exit because of a refcount
        # bug in _codecsmodule.c

# From RFC 3492
punycode_testcases = [
    # A Arabic (Egyptian):
    (u"\u0644\u064A\u0647\u0645\u0627\u0628\u062A\u0643\u0644"
     u"\u0645\u0648\u0634\u0639\u0631\u0628\u064A\u061F",
     "egbpdaj6bu4bxfgehfvwxn"),
    # B Chinese (simplified):
    (u"\u4ED6\u4EEC\u4E3A\u4EC0\u4E48\u4E0D\u8BF4\u4E2D\u6587",
     "ihqwcrb4cv8a8dqg056pqjye"),
    # C Chinese (traditional):
    (u"\u4ED6\u5011\u7232\u4EC0\u9EBD\u4E0D\u8AAA\u4E2D\u6587",
     "ihqwctvzc91f659drss3x8bo0yb"),
    # D Czech: Pro<ccaron>prost<ecaron>nemluv<iacute><ccaron>esky
    (u"\u0050\u0072\u006F\u010D\u0070\u0072\u006F\u0073\u0074"
     u"\u011B\u006E\u0065\u006D\u006C\u0075\u0076\u00ED\u010D"
     u"\u0065\u0073\u006B\u0079",
     "Proprostnemluvesky-uyb24dma41a"),
    # E Hebrew:
    (u"\u05DC\u05DE\u05D4\u05D4\u05DD\u05E4\u05E9\u05D5\u05D8"
     u"\u05DC\u05D0\u05DE\u05D3\u05D1\u05E8\u05D9\u05DD\u05E2"
     u"\u05D1\u05E8\u05D9\u05EA",
     "4dbcagdahymbxekheh6e0a7fei0b"),
    # F Hindi (Devanagari):
    (u"\u092F\u0939\u0932\u094B\u0917\u0939\u093F\u0928\u094D"
    u"\u0926\u0940\u0915\u094D\u092F\u094B\u0902\u0928\u0939"
    u"\u0940\u0902\u092C\u094B\u0932\u0938\u0915\u0924\u0947"
    u"\u0939\u0948\u0902",
    "i1baa7eci9glrd9b2ae1bj0hfcgg6iyaf8o0a1dig0cd"),

    #(G) Japanese (kanji and hiragana):
    (u"\u306A\u305C\u307F\u3093\u306A\u65E5\u672C\u8A9E\u3092"
    u"\u8A71\u3057\u3066\u304F\u308C\u306A\u3044\u306E\u304B",
     "n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa"),

    # (H) Korean (Hangul syllables):
    (u"\uC138\uACC4\uC758\uBAA8\uB4E0\uC0AC\uB78C\uB4E4\uC774"
     u"\uD55C\uAD6D\uC5B4\uB97C\uC774\uD574\uD55C\uB2E4\uBA74"
     u"\uC5BC\uB9C8\uB098\uC88B\uC744\uAE4C",
     "989aomsvi5e83db1d2a355cv1e0vak1dwrv93d5xbh15a0dt30a5j"
     "psd879ccm6fea98c"),

    # (I) Russian (Cyrillic):
    (u"\u043F\u043E\u0447\u0435\u043C\u0443\u0436\u0435\u043E"
     u"\u043D\u0438\u043D\u0435\u0433\u043E\u0432\u043E\u0440"
     u"\u044F\u0442\u043F\u043E\u0440\u0443\u0441\u0441\u043A"
     u"\u0438",
     "b1abfaaepdrnnbgefbaDotcwatmq2g4l"),

    # (J) Spanish: Porqu<eacute>nopuedensimplementehablarenEspa<ntilde>ol
    (u"\u0050\u006F\u0072\u0071\u0075\u00E9\u006E\u006F\u0070"
     u"\u0075\u0065\u0064\u0065\u006E\u0073\u0069\u006D\u0070"
     u"\u006C\u0065\u006D\u0065\u006E\u0074\u0065\u0068\u0061"
     u"\u0062\u006C\u0061\u0072\u0065\u006E\u0045\u0073\u0070"
     u"\u0061\u00F1\u006F\u006C",
     "PorqunopuedensimplementehablarenEspaol-fmd56a"),

    # (K) Vietnamese:
    #  T<adotbelow>isaoh<odotbelow>kh<ocirc>ngth<ecirchookabove>ch\
    #   <ihookabove>n<oacute>iti<ecircacute>ngVi<ecircdotbelow>t
    (u"\u0054\u1EA1\u0069\u0073\u0061\u006F\u0068\u1ECD\u006B"
     u"\u0068\u00F4\u006E\u0067\u0074\u0068\u1EC3\u0063\u0068"
     u"\u1EC9\u006E\u00F3\u0069\u0074\u0069\u1EBF\u006E\u0067"
     u"\u0056\u0069\u1EC7\u0074",
     "TisaohkhngthchnitingVit-kjcr8268qyxafd2f1b9g"),

    #(L) 3<nen>B<gumi><kinpachi><sensei>
    (u"\u0033\u5E74\u0042\u7D44\u91D1\u516B\u5148\u751F",
     "3B-ww4c5e180e575a65lsy2b"),

    # (M) <amuro><namie>-with-SUPER-MONKEYS
    (u"\u5B89\u5BA4\u5948\u7F8E\u6075\u002D\u0077\u0069\u0074"
     u"\u0068\u002D\u0053\u0055\u0050\u0045\u0052\u002D\u004D"
     u"\u004F\u004E\u004B\u0045\u0059\u0053",
     "-with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n"),

    # (N) Hello-Another-Way-<sorezore><no><basho>
    (u"\u0048\u0065\u006C\u006C\u006F\u002D\u0041\u006E\u006F"
     u"\u0074\u0068\u0065\u0072\u002D\u0057\u0061\u0079\u002D"
     u"\u305D\u308C\u305E\u308C\u306E\u5834\u6240",
     "Hello-Another-Way--fc4qua05auwb3674vfr0b"),

    # (O) <hitotsu><yane><no><shita>2
    (u"\u3072\u3068\u3064\u5C4B\u6839\u306E\u4E0B\u0032",
     "2-u9tlzr9756bt3uc0v"),

    # (P) Maji<de>Koi<suru>5<byou><mae>
    (u"\u004D\u0061\u006A\u0069\u3067\u004B\u006F\u0069\u3059"
     u"\u308B\u0035\u79D2\u524D",
     "MajiKoi5-783gue6qz075azm5e"),

     # (Q) <pafii>de<runba>
    (u"\u30D1\u30D5\u30A3\u30FC\u0064\u0065\u30EB\u30F3\u30D0",
     "de-jg4avhby1noc0d"),

    # (R) <sono><supiido><de>
    (u"\u305D\u306E\u30B9\u30D4\u30FC\u30C9\u3067",
     "d9juau41awczczp"),

    # (S) -> $1.00 <-
    (u"\u002D\u003E\u0020\u0024\u0031\u002E\u0030\u0030\u0020"
     u"\u003C\u002D",
     "-> $1.00 <--")
    ]

for i in punycode_testcases:
    if len(i)!=2:
        print repr(i)

class PunycodeTest(unittest.TestCase):
    def test_encode(self):
        for uni, puny in punycode_testcases:
            # Need to convert both strings to lower case, since
            # some of the extended encodings use upper case, but our
            # code produces only lower case. Converting just puny to
            # lower is also insufficient, since some of the input characters
            # are upper case.
            self.assertEqual(uni.encode("punycode").lower(), puny.lower())

    def test_decode(self):
        for uni, puny in punycode_testcases:
            self.assertEqual(uni, puny.decode("punycode"))

class UnicodeInternalTest(unittest.TestCase):
    def test_bug1251300(self):
        # Decoding with unicode_internal used to not correctly handle "code
        # points" above 0x10ffff on UCS-4 builds.
        if sys.maxunicode > 0xffff:
            ok = [
                ("\x00\x10\xff\xff", u"\U0010ffff"),
                ("\x00\x00\x01\x01", u"\U00000101"),
                ("", u""),
            ]
            not_ok = [
                "\x7f\xff\xff\xff",
                "\x80\x00\x00\x00",
                "\x81\x00\x00\x00",
                "\x00",
                "\x00\x00\x00\x00\x00",
            ]
            for internal, uni in ok:
                if sys.byteorder == "little":
                    internal = "".join(reversed(internal))
                self.assertEqual(uni, internal.decode("unicode_internal"))
            for internal in not_ok:
                if sys.byteorder == "little":
                    internal = "".join(reversed(internal))
                self.assertRaises(UnicodeDecodeError, internal.decode,
                    "unicode_internal")

    def test_decode_error_attributes(self):
        if sys.maxunicode > 0xffff:
            try:
                "\x00\x00\x00\x00\x00\x11\x11\x00".decode("unicode_internal")
            except UnicodeDecodeError, ex:
                self.assertEqual("unicode_internal", ex.encoding)
                self.assertEqual("\x00\x00\x00\x00\x00\x11\x11\x00", ex.object)
                self.assertEqual(4, ex.start)
                self.assertEqual(8, ex.end)
            else:
                self.fail()

    def test_decode_callback(self):
        if sys.maxunicode > 0xffff:
            codecs.register_error("UnicodeInternalTest", codecs.ignore_errors)
            decoder = codecs.getdecoder("unicode_internal")
            ab = u"ab".encode("unicode_internal")
            ignored = decoder("%s\x22\x22\x22\x22%s" % (ab[:4], ab[4:]),
                "UnicodeInternalTest")
            self.assertEqual((u"ab", 12), ignored)

    def test_encode_length(self):
        # Issue 3739
        encoder = codecs.getencoder("unicode_internal")
        self.assertEqual(encoder(u"a")[1], 1)
        self.assertEqual(encoder(u"\xe9\u0142")[1], 2)

        encoder = codecs.getencoder("string-escape")
        self.assertEqual(encoder(r'\x00')[1], 4)

# From http://www.gnu.org/software/libidn/draft-josefsson-idn-test-vectors.html
nameprep_tests = [
    # 3.1 Map to nothing.
    ('foo\xc2\xad\xcd\x8f\xe1\xa0\x86\xe1\xa0\x8bbar'
     '\xe2\x80\x8b\xe2\x81\xa0baz\xef\xb8\x80\xef\xb8\x88\xef'
     '\xb8\x8f\xef\xbb\xbf',
     'foobarbaz'),
    # 3.2 Case folding ASCII U+0043 U+0041 U+0046 U+0045.
    ('CAFE',
     'cafe'),
    # 3.3 Case folding 8bit U+00DF (german sharp s).
    # The original test case is bogus; it says \xc3\xdf
    ('\xc3\x9f',
     'ss'),
    # 3.4 Case folding U+0130 (turkish capital I with dot).
    ('\xc4\xb0',
     'i\xcc\x87'),
    # 3.5 Case folding multibyte U+0143 U+037A.
    ('\xc5\x83\xcd\xba',
     '\xc5\x84 \xce\xb9'),
    # 3.6 Case folding U+2121 U+33C6 U+1D7BB.
    # XXX: skip this as it fails in UCS-2 mode
    #('\xe2\x84\xa1\xe3\x8f\x86\xf0\x9d\x9e\xbb',
    # 'telc\xe2\x88\x95kg\xcf\x83'),
    (None, None),
    # 3.7 Normalization of U+006a U+030c U+00A0 U+00AA.
    ('j\xcc\x8c\xc2\xa0\xc2\xaa',
     '\xc7\xb0 a'),
    # 3.8 Case folding U+1FB7 and normalization.
    ('\xe1\xbe\xb7',
     '\xe1\xbe\xb6\xce\xb9'),
    # 3.9 Self-reverting case folding U+01F0 and normalization.
    # The original test case is bogus, it says `\xc7\xf0'
    ('\xc7\xb0',
     '\xc7\xb0'),
    # 3.10 Self-reverting case folding U+0390 and normalization.
    ('\xce\x90',
     '\xce\x90'),
    # 3.11 Self-reverting case folding U+03B0 and normalization.
    ('\xce\xb0',
     '\xce\xb0'),
    # 3.12 Self-reverting case folding U+1E96 and normalization.
    ('\xe1\xba\x96',
     '\xe1\xba\x96'),
    # 3.13 Self-reverting case folding U+1F56 and normalization.
    ('\xe1\xbd\x96',
     '\xe1\xbd\x96'),
    # 3.14 ASCII space character U+0020.
    (' ',
     ' '),
    # 3.15 Non-ASCII 8bit space character U+00A0.
    ('\xc2\xa0',
     ' '),
    # 3.16 Non-ASCII multibyte space character U+1680.
    ('\xe1\x9a\x80',
     None),
    # 3.17 Non-ASCII multibyte space character U+2000.
    ('\xe2\x80\x80',
     ' '),
    # 3.18 Zero Width Space U+200b.
    ('\xe2\x80\x8b',
     ''),
    # 3.19 Non-ASCII multibyte space character U+3000.
    ('\xe3\x80\x80',
     ' '),
    # 3.20 ASCII control characters U+0010 U+007F.
    ('\x10\x7f',
     '\x10\x7f'),
    # 3.21 Non-ASCII 8bit control character U+0085.
    ('\xc2\x85',
     None),
    # 3.22 Non-ASCII multibyte control character U+180E.
    ('\xe1\xa0\x8e',
     None),
    # 3.23 Zero Width No-Break Space U+FEFF.
    ('\xef\xbb\xbf',
     ''),
    # 3.24 Non-ASCII control character U+1D175.
    ('\xf0\x9d\x85\xb5',
     None),
    # 3.25 Plane 0 private use character U+F123.
    ('\xef\x84\xa3',
     None),
    # 3.26 Plane 15 private use character U+F1234.
    ('\xf3\xb1\x88\xb4',
     None),
    # 3.27 Plane 16 private use character U+10F234.
    ('\xf4\x8f\x88\xb4',
     None),
    # 3.28 Non-character code point U+8FFFE.
    ('\xf2\x8f\xbf\xbe',
     None),
    # 3.29 Non-character code point U+10FFFF.
    ('\xf4\x8f\xbf\xbf',
     None),
    # 3.30 Surrogate code U+DF42.
    ('\xed\xbd\x82',
     None),
    # 3.31 Non-plain text character U+FFFD.
    ('\xef\xbf\xbd',
     None),
    # 3.32 Ideographic description character U+2FF5.
    ('\xe2\xbf\xb5',
     None),
    # 3.33 Display property character U+0341.
    ('\xcd\x81',
     '\xcc\x81'),
    # 3.34 Left-to-right mark U+200E.
    ('\xe2\x80\x8e',
     None),
    # 3.35 Deprecated U+202A.
    ('\xe2\x80\xaa',
     None),
    # 3.36 Language tagging character U+E0001.
    ('\xf3\xa0\x80\x81',
     None),
    # 3.37 Language tagging character U+E0042.
    ('\xf3\xa0\x81\x82',
     None),
    # 3.38 Bidi: RandALCat character U+05BE and LCat characters.
    ('foo\xd6\xbebar',
     None),
    # 3.39 Bidi: RandALCat character U+FD50 and LCat characters.
    ('foo\xef\xb5\x90bar',
     None),
    # 3.40 Bidi: RandALCat character U+FB38 and LCat characters.
    ('foo\xef\xb9\xb6bar',
     'foo \xd9\x8ebar'),
    # 3.41 Bidi: RandALCat without trailing RandALCat U+0627 U+0031.
    ('\xd8\xa71',
     None),
    # 3.42 Bidi: RandALCat character U+0627 U+0031 U+0628.
    ('\xd8\xa71\xd8\xa8',
     '\xd8\xa71\xd8\xa8'),
    # 3.43 Unassigned code point U+E0002.
    # Skip this test as we allow unassigned
    #('\xf3\xa0\x80\x82',
    # None),
    (None, None),
    # 3.44 Larger test (shrinking).
    # Original test case reads \xc3\xdf
    ('X\xc2\xad\xc3\x9f\xc4\xb0\xe2\x84\xa1j\xcc\x8c\xc2\xa0\xc2'
     '\xaa\xce\xb0\xe2\x80\x80',
     'xssi\xcc\x87tel\xc7\xb0 a\xce\xb0 '),
    # 3.45 Larger test (expanding).
    # Original test case reads \xc3\x9f
    ('X\xc3\x9f\xe3\x8c\x96\xc4\xb0\xe2\x84\xa1\xe2\x92\x9f\xe3\x8c'
     '\x80',
     'xss\xe3\x82\xad\xe3\x83\xad\xe3\x83\xa1\xe3\x83\xbc\xe3'
     '\x83\x88\xe3\x83\xabi\xcc\x87tel\x28d\x29\xe3\x82'
     '\xa2\xe3\x83\x91\xe3\x83\xbc\xe3\x83\x88')
    ]


class NameprepTest(unittest.TestCase):
    def test_nameprep(self):
        from encodings.idna import nameprep
        for pos, (orig, prepped) in enumerate(nameprep_tests):
            if orig is None:
                # Skipped
                continue
            # The Unicode strings are given in UTF-8
            orig = unicode(orig, "utf-8")
            if prepped is None:
                # Input contains prohibited characters
                self.assertRaises(UnicodeError, nameprep, orig)
            else:
                prepped = unicode(prepped, "utf-8")
                try:
                    self.assertEqual(nameprep(orig), prepped)
                except Exception,e:
                    raise test_support.TestFailed("Test 3.%d: %s" % (pos+1, str(e)))

class IDNACodecTest(unittest.TestCase):
    def test_builtin_decode(self):
        self.assertEqual(unicode("python.org", "idna"), u"python.org")
        self.assertEqual(unicode("python.org.", "idna"), u"python.org.")
        self.assertEqual(unicode("xn--pythn-mua.org", "idna"), u"pyth\xf6n.org")
        self.assertEqual(unicode("xn--pythn-mua.org.", "idna"), u"pyth\xf6n.org.")

    def test_builtin_encode(self):
        self.assertEqual(u"python.org".encode("idna"), "python.org")
        self.assertEqual("python.org.".encode("idna"), "python.org.")
        self.assertEqual(u"pyth\xf6n.org".encode("idna"), "xn--pythn-mua.org")
        self.assertEqual(u"pyth\xf6n.org.".encode("idna"), "xn--pythn-mua.org.")

    def test_stream(self):
        import StringIO
        r = codecs.getreader("idna")(StringIO.StringIO("abc"))
        r.read(3)
        self.assertEqual(r.read(), u"")

    def test_incremental_decode(self):
        self.assertEqual(
            "".join(codecs.iterdecode("python.org", "idna")),
            u"python.org"
        )
        self.assertEqual(
            "".join(codecs.iterdecode("python.org.", "idna")),
            u"python.org."
        )
        self.assertEqual(
            "".join(codecs.iterdecode("xn--pythn-mua.org.", "idna")),
            u"pyth\xf6n.org."
        )
        self.assertEqual(
            "".join(codecs.iterdecode("xn--pythn-mua.org.", "idna")),
            u"pyth\xf6n.org."
        )

        decoder = codecs.getincrementaldecoder("idna")()
        self.assertEqual(decoder.decode("xn--xam", ), u"")
        self.assertEqual(decoder.decode("ple-9ta.o", ), u"\xe4xample.")
        self.assertEqual(decoder.decode(u"rg"), u"")
        self.assertEqual(decoder.decode(u"", True), u"org")

        decoder.reset()
        self.assertEqual(decoder.decode("xn--xam", ), u"")
        self.assertEqual(decoder.decode("ple-9ta.o", ), u"\xe4xample.")
        self.assertEqual(decoder.decode("rg."), u"org.")
        self.assertEqual(decoder.decode("", True), u"")

    def test_incremental_encode(self):
        self.assertEqual(
            "".join(codecs.iterencode(u"python.org", "idna")),
            "python.org"
        )
        self.assertEqual(
            "".join(codecs.iterencode(u"python.org.", "idna")),
            "python.org."
        )
        self.assertEqual(
            "".join(codecs.iterencode(u"pyth\xf6n.org.", "idna")),
            "xn--pythn-mua.org."
        )
        self.assertEqual(
            "".join(codecs.iterencode(u"pyth\xf6n.org.", "idna")),
            "xn--pythn-mua.org."
        )

        encoder = codecs.getincrementalencoder("idna")()
        self.assertEqual(encoder.encode(u"\xe4x"), "")
        self.assertEqual(encoder.encode(u"ample.org"), "xn--xample-9ta.")
        self.assertEqual(encoder.encode(u"", True), "org")

        encoder.reset()
        self.assertEqual(encoder.encode(u"\xe4x"), "")
        self.assertEqual(encoder.encode(u"ample.org."), "xn--xample-9ta.org.")
        self.assertEqual(encoder.encode(u"", True), "")

class CodecsModuleTest(unittest.TestCase):

    def test_decode(self):
        self.assertEqual(codecs.decode('\xe4\xf6\xfc', 'latin-1'),
                          u'\xe4\xf6\xfc')
        self.assertRaises(TypeError, codecs.decode)
        self.assertEqual(codecs.decode('abc'), u'abc')
        self.assertRaises(UnicodeDecodeError, codecs.decode, '\xff', 'ascii')

    def test_encode(self):
        self.assertEqual(codecs.encode(u'\xe4\xf6\xfc', 'latin-1'),
                          '\xe4\xf6\xfc')
        self.assertRaises(TypeError, codecs.encode)
        self.assertRaises(LookupError, codecs.encode, "foo", "__spam__")
        self.assertEqual(codecs.encode(u'abc'), 'abc')
        self.assertRaises(UnicodeEncodeError, codecs.encode, u'\xffff', 'ascii')

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
        # because 'I' is lowercased as a dotless "i"
        oldlocale = locale.getlocale(locale.LC_CTYPE)
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
        self.stream = StringIO.StringIO('\xed\x95\x9c\n\xea\xb8\x80')

    def test_readlines(self):
        f = self.reader(self.stream)
        self.assertEqual(f.readlines(), [u'\ud55c\n', u'\uae00'])

class EncodedFileTest(unittest.TestCase):

    def test_basic(self):
        f = StringIO.StringIO('\xed\x95\x9c\n\xea\xb8\x80')
        ef = codecs.EncodedFile(f, 'utf-16-le', 'utf-8')
        self.assertEqual(ef.read(), '\\\xd5\n\x00\x00\xae')

        f = StringIO.StringIO()
        ef = codecs.EncodedFile(f, 'utf-8', 'latin1')
        ef.write('\xc3\xbc')
        self.assertEqual(f.getvalue(), '\xfc')

class Str2StrTest(unittest.TestCase):

    def test_read(self):
        sin = "\x80".encode("base64_codec")
        reader = codecs.getreader("base64_codec")(StringIO.StringIO(sin))
        sout = reader.read()
        self.assertEqual(sout, "\x80")
        self.assertIsInstance(sout, str)

    def test_readline(self):
        sin = "\x80".encode("base64_codec")
        reader = codecs.getreader("base64_codec")(StringIO.StringIO(sin))
        sout = reader.readline()
        self.assertEqual(sout, "\x80")
        self.assertIsInstance(sout, str)

all_unicode_encodings = [
    "ascii",
    "base64_codec",
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
    "hex_codec",
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
    "rot_13",
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

# The following encodings work only with str, not unicode
all_string_encodings = [
    "quopri_codec",
    "string_escape",
    "uu_codec",
]

# The following encoding is not tested, because it's not supposed
# to work:
#    "undefined"

# The following encodings don't work in stateful mode
broken_unicode_with_streams = [
    "base64_codec",
    "hex_codec",
    "punycode",
    "unicode_internal"
]
broken_incremental_coders = broken_unicode_with_streams[:]

# The following encodings only support "strict" mode
only_strict_mode = [
    "idna",
    "zlib_codec",
    "bz2_codec",
]

try:
    import bz2
except ImportError:
    pass
else:
    all_unicode_encodings.append("bz2_codec")
    broken_unicode_with_streams.append("bz2_codec")

try:
    import zlib
except ImportError:
    pass
else:
    all_unicode_encodings.append("zlib_codec")
    broken_unicode_with_streams.append("zlib_codec")

class BasicUnicodeTest(unittest.TestCase):
    def test_basics(self):
        s = u"abc123" # all codecs should be able to encode these
        for encoding in all_unicode_encodings:
            name = codecs.lookup(encoding).name
            if encoding.endswith("_codec"):
                name += "_codec"
            elif encoding == "latin_1":
                name = "latin_1"
            self.assertEqual(encoding.replace("_", "-"), name.replace("_", "-"))
            (bytes, size) = codecs.getencoder(encoding)(s)
            self.assertEqual(size, len(s), "%r != %r (encoding=%r)" % (size, len(s), encoding))
            (chars, size) = codecs.getdecoder(encoding)(bytes)
            self.assertEqual(chars, s, "%r != %r (encoding=%r)" % (chars, s, encoding))

            if encoding not in broken_unicode_with_streams:
                # check stream reader/writer
                q = Queue()
                writer = codecs.getwriter(encoding)(q)
                encodedresult = ""
                for c in s:
                    writer.write(c)
                    encodedresult += q.read()
                q = Queue()
                reader = codecs.getreader(encoding)(q)
                decodedresult = u""
                for c in encodedresult:
                    q.write(c)
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
                    encodedresult = ""
                    for c in s:
                        encodedresult += encoder.encode(c)
                    encodedresult += encoder.encode(u"", True)
                    decoder = codecs.getincrementaldecoder(encoding)()
                    decodedresult = u""
                    for c in encodedresult:
                        decodedresult += decoder.decode(c)
                    decodedresult += decoder.decode("", True)
                    self.assertEqual(decodedresult, s, "%r != %r (encoding=%r)" % (decodedresult, s, encoding))

                    # check C API
                    encodedresult = ""
                    for c in s:
                        encodedresult += cencoder.encode(c)
                    encodedresult += cencoder.encode(u"", True)
                    cdecoder = _testcapi.codec_incrementaldecoder(encoding)
                    decodedresult = u""
                    for c in encodedresult:
                        decodedresult += cdecoder.decode(c)
                    decodedresult += cdecoder.decode("", True)
                    self.assertEqual(decodedresult, s, "%r != %r (encoding=%r)" % (decodedresult, s, encoding))

                    # check iterencode()/iterdecode()
                    result = u"".join(codecs.iterdecode(codecs.iterencode(s, encoding), encoding))
                    self.assertEqual(result, s, "%r != %r (encoding=%r)" % (result, s, encoding))

                    # check iterencode()/iterdecode() with empty string
                    result = u"".join(codecs.iterdecode(codecs.iterencode(u"", encoding), encoding))
                    self.assertEqual(result, u"")

                if encoding not in only_strict_mode:
                    # check incremental decoder/encoder with errors argument
                    try:
                        encoder = codecs.getincrementalencoder(encoding)("ignore")
                        cencoder = _testcapi.codec_incrementalencoder(encoding, "ignore")
                    except LookupError: # no IncrementalEncoder
                        pass
                    else:
                        encodedresult = "".join(encoder.encode(c) for c in s)
                        decoder = codecs.getincrementaldecoder(encoding)("ignore")
                        decodedresult = u"".join(decoder.decode(c) for c in encodedresult)
                        self.assertEqual(decodedresult, s, "%r != %r (encoding=%r)" % (decodedresult, s, encoding))

                        encodedresult = "".join(cencoder.encode(c) for c in s)
                        cdecoder = _testcapi.codec_incrementaldecoder(encoding, "ignore")
                        decodedresult = u"".join(cdecoder.decode(c) for c in encodedresult)
                        self.assertEqual(decodedresult, s, "%r != %r (encoding=%r)" % (decodedresult, s, encoding))

    def test_seek(self):
        # all codecs should be able to encode these
        s = u"%s\n%s\n" % (100*u"abc123", 100*u"def456")
        for encoding in all_unicode_encodings:
            if encoding == "idna": # FIXME: See SF bug #1163178
                continue
            if encoding in broken_unicode_with_streams:
                continue
            reader = codecs.getreader(encoding)(StringIO.StringIO(s.encode(encoding)))
            for t in xrange(5):
                # Test that calling seek resets the internal codec state and buffers
                reader.seek(0, 0)
                line = reader.readline()
                self.assertEqual(s[:len(line)], line)

    def test_bad_decode_args(self):
        for encoding in all_unicode_encodings:
            decoder = codecs.getdecoder(encoding)
            self.assertRaises(TypeError, decoder)
            if encoding not in ("idna", "punycode"):
                self.assertRaises(TypeError, decoder, 42)

    def test_bad_encode_args(self):
        for encoding in all_unicode_encodings:
            encoder = codecs.getencoder(encoding)
            self.assertRaises(TypeError, encoder)

    def test_encoding_map_type_initialized(self):
        from encodings import cp1140
        # This used to crash, we are only verifying there's no crash.
        table_type = type(cp1140.encoding_table)
        self.assertEqual(table_type, table_type)

class BasicStrTest(unittest.TestCase):
    def test_basics(self):
        s = "abc123"
        for encoding in all_string_encodings:
            (bytes, size) = codecs.getencoder(encoding)(s)
            self.assertEqual(size, len(s))
            (chars, size) = codecs.getdecoder(encoding)(bytes)
            self.assertEqual(chars, s, "%r != %r (encoding=%r)" % (chars, s, encoding))

class CharmapTest(unittest.TestCase):
    def test_decode_with_string_map(self):
        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "strict", u"abc"),
            (u"abc", 3)
        )

        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, b"\x00\x01\x02", "strict", u"ab"
        )

        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, "\x00\x01\x02", "strict", u"ab\ufffe"
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "replace", u"ab"),
            (u"ab\ufffd", 3)
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "replace", u"ab\ufffe"),
            (u"ab\ufffd", 3)
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "ignore", u"ab"),
            (u"ab", 3)
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "ignore", u"ab\ufffe"),
            (u"ab", 3)
        )

        allbytes = "".join(chr(i) for i in xrange(256))
        self.assertEqual(
            codecs.charmap_decode(allbytes, "ignore", u""),
            (u"", len(allbytes))
        )

    def test_decode_with_int2str_map(self):
        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "strict",
                                  {0: u'a', 1: u'b', 2: u'c'}),
            (u"abc", 3)
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "strict",
                                  {0: u'Aa', 1: u'Bb', 2: u'Cc'}),
            (u"AaBbCc", 3)
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "strict",
                                  {0: u'\U0010FFFF', 1: u'b', 2: u'c'}),
            (u"\U0010FFFFbc", 3)
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "strict",
                                  {0: u'a', 1: u'b', 2: u''}),
            (u"ab", 3)
        )

        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, "\x00\x01\x02", "strict",
                                   {0: u'a', 1: u'b'}
        )

        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, "\x00\x01\x02", "strict",
                                   {0: u'a', 1: u'b', 2: None}
        )

        # Issue #14850
        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, "\x00\x01\x02", "strict",
                                   {0: u'a', 1: u'b', 2: u'\ufffe'}
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "replace",
                                  {0: u'a', 1: u'b'}),
            (u"ab\ufffd", 3)
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "replace",
                                  {0: u'a', 1: u'b', 2: None}),
            (u"ab\ufffd", 3)
        )

        # Issue #14850
        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "replace",
                                  {0: u'a', 1: u'b', 2: u'\ufffe'}),
            (u"ab\ufffd", 3)
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "ignore",
                                  {0: u'a', 1: u'b'}),
            (u"ab", 3)
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "ignore",
                                  {0: u'a', 1: u'b', 2: None}),
            (u"ab", 3)
        )

        # Issue #14850
        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "ignore",
                                  {0: u'a', 1: u'b', 2: u'\ufffe'}),
            (u"ab", 3)
        )

        allbytes = "".join(chr(i) for i in xrange(256))
        self.assertEqual(
            codecs.charmap_decode(allbytes, "ignore", {}),
            (u"", len(allbytes))
        )

    def test_decode_with_int2int_map(self):
        a = ord(u'a')
        b = ord(u'b')
        c = ord(u'c')

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "strict",
                                  {0: a, 1: b, 2: c}),
            (u"abc", 3)
        )

        # Issue #15379
        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "strict",
                                  {0: 0x10FFFF, 1: b, 2: c}),
            (u"\U0010FFFFbc", 3)
        )

        self.assertRaises(TypeError,
            codecs.charmap_decode, "\x00\x01\x02", "strict",
                                   {0: 0x110000, 1: b, 2: c}
        )

        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, "\x00\x01\x02", "strict",
                                   {0: a, 1: b},
        )

        self.assertRaises(UnicodeDecodeError,
            codecs.charmap_decode, "\x00\x01\x02", "strict",
                                   {0: a, 1: b, 2: 0xFFFE},
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "replace",
                                  {0: a, 1: b}),
            (u"ab\ufffd", 3)
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "replace",
                                  {0: a, 1: b, 2: 0xFFFE}),
            (u"ab\ufffd", 3)
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "ignore",
                                  {0: a, 1: b}),
            (u"ab", 3)
        )

        self.assertEqual(
            codecs.charmap_decode("\x00\x01\x02", "ignore",
                                  {0: a, 1: b, 2: 0xFFFE}),
            (u"ab", 3)
        )


class WithStmtTest(unittest.TestCase):
    def test_encodedfile(self):
        f = StringIO.StringIO("\xc3\xbc")
        with codecs.EncodedFile(f, "latin-1", "utf-8") as ef:
            self.assertEqual(ef.read(), "\xfc")

    def test_streamreaderwriter(self):
        f = StringIO.StringIO("\xc3\xbc")
        info = codecs.lookup("utf-8")
        with codecs.StreamReaderWriter(f, info.streamreader,
                                       info.streamwriter, 'strict') as srw:
            self.assertEqual(srw.read(), u"\xfc")


class BomTest(unittest.TestCase):
    def test_seek0(self):
        data = u"1234567890"
        tests = ("utf-16",
                 "utf-16-le",
                 "utf-16-be",
                 "utf-32",
                 "utf-32-le",
                 "utf-32-be")
        self.addCleanup(test_support.unlink, test_support.TESTFN)
        for encoding in tests:
            # Check if the BOM is written only once
            with codecs.open(test_support.TESTFN, 'w+', encoding=encoding) as f:
                f.write(data)
                f.write(data)
                f.seek(0)
                self.assertEqual(f.read(), data * 2)
                f.seek(0)
                self.assertEqual(f.read(), data * 2)

            # Check that the BOM is written after a seek(0)
            with codecs.open(test_support.TESTFN, 'w+', encoding=encoding) as f:
                f.write(data[0])
                self.assertNotEqual(f.tell(), 0)
                f.seek(0)
                f.write(data)
                f.seek(0)
                self.assertEqual(f.read(), data)

            # (StreamWriter) Check that the BOM is written after a seek(0)
            with codecs.open(test_support.TESTFN, 'w+', encoding=encoding) as f:
                f.writer.write(data[0])
                self.assertNotEqual(f.writer.tell(), 0)
                f.writer.seek(0)
                f.writer.write(data)
                f.seek(0)
                self.assertEqual(f.read(), data)

            # Check that the BOM is not written after a seek() at a position
            # different than the start
            with codecs.open(test_support.TESTFN, 'w+', encoding=encoding) as f:
                f.write(data)
                f.seek(f.tell())
                f.write(data)
                f.seek(0)
                self.assertEqual(f.read(), data * 2)

            # (StreamWriter) Check that the BOM is not written after a seek()
            # at a position different than the start
            with codecs.open(test_support.TESTFN, 'w+', encoding=encoding) as f:
                f.writer.write(data)
                f.writer.seek(f.writer.tell())
                f.writer.write(data)
                f.seek(0)
                self.assertEqual(f.read(), data * 2)


def test_main():
    test_support.run_unittest(
        UTF32Test,
        UTF32LETest,
        UTF32BETest,
        UTF16Test,
        UTF16LETest,
        UTF16BETest,
        UTF8Test,
        UTF8SigTest,
        UTF7Test,
        UTF16ExTest,
        ReadBufferTest,
        CharBufferTest,
        EscapeDecodeTest,
        RecodingTest,
        PunycodeTest,
        UnicodeInternalTest,
        NameprepTest,
        IDNACodecTest,
        CodecsModuleTest,
        StreamReaderTest,
        EncodedFileTest,
        Str2StrTest,
        BasicUnicodeTest,
        BasicStrTest,
        CharmapTest,
        WithStmtTest,
        BomTest,
    )


if __name__ == "__main__":
    test_main()
