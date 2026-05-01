import copy
import pickle
import sys
import unicodedata
import unittest
import urllib.parse
from urllib.parse import urldefrag, urlparse, urlsplit, urlunparse, urlunsplit
from test import support

RFC1808_BASE = "http://a/b/c/d;p?q#f"
RFC2396_BASE = "http://a/b/c/d;p?q"
RFC3986_BASE = 'http://a/b/c/d;p?q'
SIMPLE_BASE  = 'http://a/b/c/d'

# Each parse_qsl testcase is a two-tuple that contains
# a string with the query and a list with the expected result.

parse_qsl_test_cases = [
    ("", []),
    ("&", []),
    ("&&", []),
    ("=", [('', '')]),
    ("=a", [('', 'a')]),
    ("a", [('a', '')]),
    ("a=", [('a', '')]),
    ("a=b=c", [('a', 'b=c')]),
    ("a%3Db=c", [('a=b', 'c')]),
    ("a=b&c=d", [('a', 'b'), ('c', 'd')]),
    ("a=b%26c=d", [('a', 'b&c=d')]),
    ("&a=b", [('a', 'b')]),
    ("a=a+b&b=b+c", [('a', 'a b'), ('b', 'b c')]),
    ("a=1&a=2", [('a', '1'), ('a', '2')]),
    (b"", []),
    (b"&", []),
    (b"&&", []),
    (b"=", [(b'', b'')]),
    (b"=a", [(b'', b'a')]),
    (b"a", [(b'a', b'')]),
    (b"a=", [(b'a', b'')]),
    (b"a=b=c", [(b'a', b'b=c')]),
    (b"a%3Db=c", [(b'a=b', b'c')]),
    (b"a=b&c=d", [(b'a', b'b'), (b'c', b'd')]),
    (b"a=b%26c=d", [(b'a', b'b&c=d')]),
    (b"&a=b", [(b'a', b'b')]),
    (b"a=a+b&b=b+c", [(b'a', b'a b'), (b'b', b'b c')]),
    (b"a=1&a=2", [(b'a', b'1'), (b'a', b'2')]),
    (";a=b", [(';a', 'b')]),
    ("a=a+b;b=b+c", [('a', 'a b;b=b c')]),
    (b";a=b", [(b';a', b'b')]),
    (b"a=a+b;b=b+c", [(b'a', b'a b;b=b c')]),

    ("\u0141=\xE9", [('\u0141', '\xE9')]),
    ("%C5%81=%C3%A9", [('\u0141', '\xE9')]),
    ("%81=%A9", [('\ufffd', '\ufffd')]),
    (b"\xc5\x81=\xc3\xa9", [(b'\xc5\x81', b'\xc3\xa9')]),
    (b"%C5%81=%C3%A9", [(b'\xc5\x81', b'\xc3\xa9')]),
    (b"\x81=\xA9", [(b'\x81', b'\xa9')]),
    (b"%81=%A9", [(b'\x81', b'\xa9')]),
]

# Each parse_qs testcase is a two-tuple that contains
# a string with the query and a dictionary with the expected result.

parse_qs_test_cases = [
    ("", {}),
    ("&", {}),
    ("&&", {}),
    ("=", {'': ['']}),
    ("=a", {'': ['a']}),
    ("a", {'a': ['']}),
    ("a=", {'a': ['']}),
    ("a=b=c", {'a': ['b=c']}),
    ("a%3Db=c", {'a=b': ['c']}),
    ("a=b&c=d", {'a': ['b'], 'c': ['d']}),
    ("a=b%26c=d", {'a': ['b&c=d']}),
    ("&a=b", {'a': ['b']}),
    ("a=a+b&b=b+c", {'a': ['a b'], 'b': ['b c']}),
    ("a=1&a=2", {'a': ['1', '2']}),
    (b"", {}),
    (b"&", {}),
    (b"&&", {}),
    (b"=", {b'': [b'']}),
    (b"=a", {b'': [b'a']}),
    (b"a", {b'a': [b'']}),
    (b"a=", {b'a': [b'']}),
    (b"a=b=c", {b'a': [b'b=c']}),
    (b"a%3Db=c", {b'a=b': [b'c']}),
    (b"a=b&c=d", {b'a': [b'b'], b'c': [b'd']}),
    (b"a=b%26c=d", {b'a': [b'b&c=d']}),
    (b"&a=b", {b'a': [b'b']}),
    (b"a=a+b&b=b+c", {b'a': [b'a b'], b'b': [b'b c']}),
    (b"a=1&a=2", {b'a': [b'1', b'2']}),
    (";a=b", {';a': ['b']}),
    ("a=a+b;b=b+c", {'a': ['a b;b=b c']}),
    (b";a=b", {b';a': [b'b']}),
    (b"a=a+b;b=b+c", {b'a':[ b'a b;b=b c']}),
    (b"a=a%E2%80%99b", {b'a': [b'a\xe2\x80\x99b']}),

    ("\u0141=\xE9", {'\u0141': ['\xE9']}),
    ("%C5%81=%C3%A9", {'\u0141': ['\xE9']}),
    ("%81=%A9", {'\ufffd': ['\ufffd']}),
    (b"\xc5\x81=\xc3\xa9", {b'\xc5\x81': [b'\xc3\xa9']}),
    (b"%C5%81=%C3%A9", {b'\xc5\x81': [b'\xc3\xa9']}),
    (b"\x81=\xA9", {b'\x81': [b'\xa9']}),
    (b"%81=%A9", {b'\x81': [b'\xa9']}),
]

class UrlParseTestCase(unittest.TestCase):

    def checkRoundtrips(self, url, parsed, split, url2=None):
        if url2 is None:
            url2 = url
        self.checkRoundtrips1(url, parsed, split, missing_as_none=True)
        empty = url[:0]
        parsed = tuple(x or empty for x in parsed)
        split = tuple(x or empty for x in split)
        self.checkRoundtrips1(url, parsed, split, url2, missing_as_none=False)

        result = urlparse(url, missing_as_none=True)
        self.assertEqual(urlunparse(result, keep_empty=False), url2)
        self.assertEqual(urlunparse(tuple(result), keep_empty=False), url2)
        result = urlparse(url, missing_as_none=False)
        with self.assertRaises(ValueError):
            urlunparse(result, keep_empty=True)
        urlunparse(tuple(result), keep_empty=True)

        result = urlsplit(url, missing_as_none=True)
        self.assertEqual(urlunsplit(result, keep_empty=False), url2)
        self.assertEqual(urlunsplit(tuple(result), keep_empty=False), url2)
        result = urlsplit(url, missing_as_none=False)
        with self.assertRaises(ValueError):
            urlunsplit(result, keep_empty=True)
        urlunsplit(tuple(result), keep_empty=True)

    def checkRoundtrips1(self, url, parsed, split, url2=None, *, missing_as_none):
        if url2 is None:
            url2 = url
        result = urlparse(url, missing_as_none=missing_as_none)
        self.assertSequenceEqual(result, parsed)
        t = (result.scheme, result.netloc, result.path,
             result.params, result.query, result.fragment)
        self.assertSequenceEqual(t, parsed)
        # put it back together and it should be the same
        result2 = urlunparse(result)
        self.assertEqual(result2, url2)
        self.assertEqual(result2, result.geturl())
        self.assertEqual(urlunparse(result, keep_empty=missing_as_none), url2)
        self.assertEqual(urlunparse(tuple(result), keep_empty=missing_as_none), result2)

        # the result of geturl() is a fixpoint; we can always parse it
        # again to get the same result:
        result3 = urlparse(result.geturl(), missing_as_none=missing_as_none)
        self.assertEqual(result3.geturl(), result.geturl())
        self.assertSequenceEqual(result3, result)
        self.assertEqual(result3.scheme,   result.scheme)
        self.assertEqual(result3.netloc,   result.netloc)
        self.assertEqual(result3.path,     result.path)
        self.assertEqual(result3.params,   result.params)
        self.assertEqual(result3.query,    result.query)
        self.assertEqual(result3.fragment, result.fragment)
        self.assertEqual(result3.username, result.username)
        self.assertEqual(result3.password, result.password)
        self.assertEqual(result3.hostname, result.hostname)
        self.assertEqual(result3.port,     result.port)

        # check the roundtrip using urlsplit() as well
        result = urlsplit(url, missing_as_none=missing_as_none)
        self.assertSequenceEqual(result, split)
        t = (result.scheme, result.netloc, result.path,
            result.query, result.fragment)
        self.assertSequenceEqual(t, split)
        result2 = urlunsplit(result)
        self.assertEqual(result2, url2)
        self.assertEqual(result2, result.geturl())
        self.assertEqual(urlunsplit(tuple(result), keep_empty=missing_as_none), result2)

        # check the fixpoint property of re-parsing the result of geturl()
        result3 = urlsplit(result.geturl(), missing_as_none=missing_as_none)
        self.assertEqual(result3.geturl(), result.geturl())
        self.assertSequenceEqual(result3, result)
        self.assertEqual(result3.scheme,   result.scheme)
        self.assertEqual(result3.netloc,   result.netloc)
        self.assertEqual(result3.path,     result.path)
        self.assertEqual(result3.query,    result.query)
        self.assertEqual(result3.fragment, result.fragment)
        self.assertEqual(result3.username, result.username)
        self.assertEqual(result3.password, result.password)
        self.assertEqual(result3.hostname, result.hostname)
        self.assertEqual(result3.port,     result.port)

    @support.subTests('orig,expect', parse_qsl_test_cases)
    def test_qsl(self, orig, expect):
        result = urllib.parse.parse_qsl(orig, keep_blank_values=True)
        self.assertEqual(result, expect)
        expect_without_blanks = [v for v in expect if len(v[1])]
        result = urllib.parse.parse_qsl(orig, keep_blank_values=False)
        self.assertEqual(result, expect_without_blanks)

    @support.subTests('orig,expect', parse_qs_test_cases)
    def test_qs(self, orig, expect):
        result = urllib.parse.parse_qs(orig, keep_blank_values=True)
        self.assertEqual(result, expect)
        expect_without_blanks = {v: expect[v]
                                 for v in expect if len(expect[v][0])}
        result = urllib.parse.parse_qs(orig, keep_blank_values=False)
        self.assertEqual(result, expect_without_blanks)

    @support.subTests('bytes', (False, True))
    @support.subTests('url,parsed,split', [
            ('path/to/file',
             (None, None, 'path/to/file', None, None, None),
             (None, None, 'path/to/file', None, None)),
            ('/path/to/file',
             (None, None, '/path/to/file', None, None, None),
             (None, None, '/path/to/file', None, None)),
            ('//path/to/file',
             (None, 'path', '/to/file', None, None, None),
             (None, 'path', '/to/file', None, None)),
            ('////path/to/file',
             (None, '', '//path/to/file', None, None, None),
             (None, '', '//path/to/file', None, None)),
            ('/////path/to/file',
             (None, '', '///path/to/file', None, None, None),
             (None, '', '///path/to/file', None, None)),
            ('scheme:path/to/file',
             ('scheme', None, 'path/to/file', None, None, None),
             ('scheme', None, 'path/to/file', None, None)),
            ('scheme:/path/to/file',
             ('scheme', None, '/path/to/file', None, None, None),
             ('scheme', None, '/path/to/file', None, None)),
            ('scheme://path/to/file',
             ('scheme', 'path', '/to/file', None, None, None),
             ('scheme', 'path', '/to/file', None, None)),
            ('scheme:////path/to/file',
             ('scheme', '', '//path/to/file', None, None, None),
             ('scheme', '', '//path/to/file', None, None)),
            ('scheme://///path/to/file',
             ('scheme', '', '///path/to/file', None, None, None),
             ('scheme', '', '///path/to/file', None, None)),
            ('file:tmp/junk.txt',
             ('file', None, 'tmp/junk.txt', None, None, None),
             ('file', None, 'tmp/junk.txt', None, None)),
            ('file:///tmp/junk.txt',
             ('file', '', '/tmp/junk.txt', None, None, None),
             ('file', '', '/tmp/junk.txt', None, None)),
            ('file:////tmp/junk.txt',
             ('file', '', '//tmp/junk.txt', None, None, None),
             ('file', '', '//tmp/junk.txt', None, None)),
            ('file://///tmp/junk.txt',
             ('file', '', '///tmp/junk.txt', None, None, None),
             ('file', '', '///tmp/junk.txt', None, None)),
            ('http:tmp/junk.txt',
             ('http', None, 'tmp/junk.txt', None, None, None),
             ('http', None, 'tmp/junk.txt', None, None)),
            ('http://example.com/tmp/junk.txt',
             ('http', 'example.com', '/tmp/junk.txt', None, None, None),
             ('http', 'example.com', '/tmp/junk.txt', None, None)),
            ('http:///example.com/tmp/junk.txt',
             ('http', '', '/example.com/tmp/junk.txt', None, None, None),
             ('http', '', '/example.com/tmp/junk.txt', None, None)),
            ('http:////example.com/tmp/junk.txt',
             ('http', '', '//example.com/tmp/junk.txt', None, None, None),
             ('http', '', '//example.com/tmp/junk.txt', None, None)),
            ('imap://mail.python.org/mbox1',
             ('imap', 'mail.python.org', '/mbox1', None, None, None),
             ('imap', 'mail.python.org', '/mbox1', None, None)),
            ('mms://wms.sys.hinet.net/cts/Drama/09006251100.asf',
             ('mms', 'wms.sys.hinet.net', '/cts/Drama/09006251100.asf',
              None, None, None),
             ('mms', 'wms.sys.hinet.net', '/cts/Drama/09006251100.asf',
              None, None)),
            ('nfs://server/path/to/file.txt',
             ('nfs', 'server', '/path/to/file.txt', None, None, None),
             ('nfs', 'server', '/path/to/file.txt', None, None)),
            ('svn+ssh://svn.zope.org/repos/main/ZConfig/trunk/',
             ('svn+ssh', 'svn.zope.org', '/repos/main/ZConfig/trunk/',
              None, None, None),
             ('svn+ssh', 'svn.zope.org', '/repos/main/ZConfig/trunk/',
              None, None)),
            ('git+ssh://git@github.com/user/project.git',
             ('git+ssh', 'git@github.com','/user/project.git',
              None,None,None),
             ('git+ssh', 'git@github.com','/user/project.git',
              None, None)),
            ('itms-services://?action=download-manifest&url=https://example.com/app',
             ('itms-services', '', '', None,
              'action=download-manifest&url=https://example.com/app', None),
             ('itms-services', '', '',
              'action=download-manifest&url=https://example.com/app', None)),
            ('+scheme:path/to/file',
             (None, None, '+scheme:path/to/file', None, None, None),
             (None, None, '+scheme:path/to/file', None, None)),
            ('sch_me:path/to/file',
             (None, None, 'sch_me:path/to/file', None, None, None),
             (None, None, 'sch_me:path/to/file', None, None)),
            ('schème:path/to/file',
             (None, None, 'schème:path/to/file', None, None, None),
             (None, None, 'schème:path/to/file', None, None)),
            ])
    def test_roundtrips(self, bytes, url, parsed, split):
        if bytes:
            if not url.isascii():
                self.skipTest('non-ASCII bytes')
            url = str_encode(url)
            parsed = tuple_encode(parsed)
            split = tuple_encode(split)
        self.checkRoundtrips(url, parsed, split)

    @support.subTests('bytes', (False, True))
    @support.subTests('url,url2,parsed,split', [
            ('///path/to/file',
             '/path/to/file',
             (None, '', '/path/to/file', None, None, None),
             (None, '', '/path/to/file', None, None)),
            ('scheme:///path/to/file',
             'scheme:/path/to/file',
             ('scheme', '', '/path/to/file', None, None, None),
             ('scheme', '', '/path/to/file', None, None)),
            ('file:/tmp/junk.txt',
             'file:///tmp/junk.txt',
             ('file', None, '/tmp/junk.txt', None, None, None),
             ('file', None, '/tmp/junk.txt', None, None)),
            ('http:/tmp/junk.txt',
             'http:///tmp/junk.txt',
             ('http', None, '/tmp/junk.txt', None, None, None),
             ('http', None, '/tmp/junk.txt', None, None)),
            ('https:/tmp/junk.txt',
             'https:///tmp/junk.txt',
             ('https', None, '/tmp/junk.txt', None, None, None),
             ('https', None, '/tmp/junk.txt', None, None)),
        ])
    def test_roundtrips_normalization(self, bytes, url, url2, parsed, split):
        if bytes:
            url = str_encode(url)
            url2 = str_encode(url2)
            parsed = tuple_encode(parsed)
            split = tuple_encode(split)
        self.checkRoundtrips(url, parsed, split, url2)

    @support.subTests('bytes', (False, True))
    @support.subTests('scheme', ('http', 'https'))
    @support.subTests('url,parsed,split', [
            ('://www.python.org',
             ('www.python.org', '', None, None, None),
             ('www.python.org', '', None, None)),
            ('://www.python.org#abc',
             ('www.python.org', '', None, None, 'abc'),
             ('www.python.org', '', None, 'abc')),
            ('://www.python.org?q=abc',
             ('www.python.org', '', None, 'q=abc', None),
             ('www.python.org', '', 'q=abc', None)),
            ('://www.python.org/#abc',
             ('www.python.org', '/', None, None, 'abc'),
             ('www.python.org', '/', None, 'abc')),
            ('://a/b/c/d;p?q#f',
             ('a', '/b/c/d', 'p', 'q', 'f'),
             ('a', '/b/c/d;p', 'q', 'f')),
            ])
    def test_http_roundtrips(self, bytes, scheme, url, parsed, split):
        # urllib.parse.urlsplit treats 'http:' as an optimized special case,
        # so we test both 'http:' and 'https:' in all the following.
        # Three cheers for white box knowledge!
        if bytes:
            scheme = str_encode(scheme)
            url = str_encode(url)
            parsed = tuple_encode(parsed)
            split = tuple_encode(split)
        url = scheme + url
        parsed = (scheme,) + parsed
        split = (scheme,) + split
        self.checkRoundtrips(url, parsed, split)

    def checkJoin(self, base, relurl, expected, *, relroundtrip=True):
        with self.subTest(base=base, relurl=relurl):
            self.assertEqual(urllib.parse.urljoin(base, relurl), expected)
            baseb = str_encode(base)
            relurlb = str_encode(relurl)
            expectedb = str_encode(expected)
            self.assertEqual(urllib.parse.urljoin(baseb, relurlb), expectedb)

            if relroundtrip:
                relurl2 = urlunsplit(urlsplit(relurl))
                self.assertEqual(urllib.parse.urljoin(base, relurl2), expected)
                relurlb2 = urlunsplit(urlsplit(relurlb))
                self.assertEqual(urllib.parse.urljoin(baseb, relurlb2), expectedb)

            relurl3 = urlunsplit(urlsplit(relurl, missing_as_none=True))
            self.assertEqual(urllib.parse.urljoin(base, relurl3), expected)
            relurlb3 = urlunsplit(urlsplit(relurlb, missing_as_none=True))
            self.assertEqual(urllib.parse.urljoin(baseb, relurlb3), expectedb)

    @support.subTests('bytes', (False, True))
    @support.subTests('u', ['Python', './Python','x-newscheme://foo.com/stuff','x://y','x:/y','x:/','/',])
    def test_unparse_parse(self, bytes, u):
        if bytes:
            u = str_encode(u)
        self.assertEqual(urllib.parse.urlunsplit(urllib.parse.urlsplit(u)), u)
        self.assertEqual(urllib.parse.urlunparse(urllib.parse.urlparse(u)), u)

    def test_RFC1808(self):
        # "normal" cases from RFC 1808:
        self.checkJoin(RFC1808_BASE, 'g:h', 'g:h')
        self.checkJoin(RFC1808_BASE, 'g', 'http://a/b/c/g')
        self.checkJoin(RFC1808_BASE, './g', 'http://a/b/c/g')
        self.checkJoin(RFC1808_BASE, 'g/', 'http://a/b/c/g/')
        self.checkJoin(RFC1808_BASE, '/g', 'http://a/g')
        self.checkJoin(RFC1808_BASE, '//g', 'http://g')
        self.checkJoin(RFC1808_BASE, 'g?y', 'http://a/b/c/g?y')
        self.checkJoin(RFC1808_BASE, 'g?y/./x', 'http://a/b/c/g?y/./x')
        self.checkJoin(RFC1808_BASE, '#s', 'http://a/b/c/d;p?q#s')
        self.checkJoin(RFC1808_BASE, 'g#s', 'http://a/b/c/g#s')
        self.checkJoin(RFC1808_BASE, 'g#s/./x', 'http://a/b/c/g#s/./x')
        self.checkJoin(RFC1808_BASE, 'g?y#s', 'http://a/b/c/g?y#s')
        self.checkJoin(RFC1808_BASE, 'g;x', 'http://a/b/c/g;x')
        self.checkJoin(RFC1808_BASE, 'g;x?y#s', 'http://a/b/c/g;x?y#s')
        self.checkJoin(RFC1808_BASE, '.', 'http://a/b/c/')
        self.checkJoin(RFC1808_BASE, './', 'http://a/b/c/')
        self.checkJoin(RFC1808_BASE, '..', 'http://a/b/')
        self.checkJoin(RFC1808_BASE, '../', 'http://a/b/')
        self.checkJoin(RFC1808_BASE, '../g', 'http://a/b/g')
        self.checkJoin(RFC1808_BASE, '../..', 'http://a/')
        self.checkJoin(RFC1808_BASE, '../../', 'http://a/')
        self.checkJoin(RFC1808_BASE, '../../g', 'http://a/g')

        # "abnormal" cases from RFC 1808:
        self.checkJoin(RFC1808_BASE, None, 'http://a/b/c/d;p?q#f')
        self.checkJoin(RFC1808_BASE, 'g.', 'http://a/b/c/g.')
        self.checkJoin(RFC1808_BASE, '.g', 'http://a/b/c/.g')
        self.checkJoin(RFC1808_BASE, 'g..', 'http://a/b/c/g..')
        self.checkJoin(RFC1808_BASE, '..g', 'http://a/b/c/..g')
        self.checkJoin(RFC1808_BASE, './../g', 'http://a/b/g')
        self.checkJoin(RFC1808_BASE, './g/.', 'http://a/b/c/g/')
        self.checkJoin(RFC1808_BASE, 'g/./h', 'http://a/b/c/g/h')
        self.checkJoin(RFC1808_BASE, 'g/../h', 'http://a/b/c/h')

        # RFC 1808 and RFC 1630 disagree on these (according to RFC 1808),
        # so we'll not actually run these tests (which expect 1808 behavior).
        #self.checkJoin(RFC1808_BASE, 'http:g', 'http:g')
        #self.checkJoin(RFC1808_BASE, 'http:', 'http:')

        # XXX: The following tests are no longer compatible with RFC3986
        # self.checkJoin(RFC1808_BASE, '../../../g', 'http://a/../g')
        # self.checkJoin(RFC1808_BASE, '../../../../g', 'http://a/../../g')
        # self.checkJoin(RFC1808_BASE, '/./g', 'http://a/./g')
        # self.checkJoin(RFC1808_BASE, '/../g', 'http://a/../g')


    def test_RFC2368(self):
        # Issue 11467: path that starts with a number is not parsed correctly
        self.assertEqual(urlparse('mailto:1337@example.org'),
                ('mailto', '', '1337@example.org', '', '', ''))
        self.assertEqual(urlparse('mailto:1337@example.org', missing_as_none=True),
                ('mailto', None, '1337@example.org', None, None, None))

    def test_RFC2396(self):
        # cases from RFC 2396

        self.checkJoin(RFC2396_BASE, 'g:h', 'g:h')
        self.checkJoin(RFC2396_BASE, 'g', 'http://a/b/c/g')
        self.checkJoin(RFC2396_BASE, './g', 'http://a/b/c/g')
        self.checkJoin(RFC2396_BASE, 'g/', 'http://a/b/c/g/')
        self.checkJoin(RFC2396_BASE, '/g', 'http://a/g')
        self.checkJoin(RFC2396_BASE, '//g', 'http://g')
        self.checkJoin(RFC2396_BASE, 'g?y', 'http://a/b/c/g?y')
        self.checkJoin(RFC2396_BASE, '#s', 'http://a/b/c/d;p?q#s')
        self.checkJoin(RFC2396_BASE, 'g#s', 'http://a/b/c/g#s')
        self.checkJoin(RFC2396_BASE, 'g?y#s', 'http://a/b/c/g?y#s')
        self.checkJoin(RFC2396_BASE, 'g;x', 'http://a/b/c/g;x')
        self.checkJoin(RFC2396_BASE, 'g;x?y#s', 'http://a/b/c/g;x?y#s')
        self.checkJoin(RFC2396_BASE, '.', 'http://a/b/c/')
        self.checkJoin(RFC2396_BASE, './', 'http://a/b/c/')
        self.checkJoin(RFC2396_BASE, '..', 'http://a/b/')
        self.checkJoin(RFC2396_BASE, '../', 'http://a/b/')
        self.checkJoin(RFC2396_BASE, '../g', 'http://a/b/g')
        self.checkJoin(RFC2396_BASE, '../..', 'http://a/')
        self.checkJoin(RFC2396_BASE, '../../', 'http://a/')
        self.checkJoin(RFC2396_BASE, '../../g', 'http://a/g')
        self.checkJoin(RFC2396_BASE, '', RFC2396_BASE)
        self.checkJoin(RFC2396_BASE, 'g.', 'http://a/b/c/g.')
        self.checkJoin(RFC2396_BASE, '.g', 'http://a/b/c/.g')
        self.checkJoin(RFC2396_BASE, 'g..', 'http://a/b/c/g..')
        self.checkJoin(RFC2396_BASE, '..g', 'http://a/b/c/..g')
        self.checkJoin(RFC2396_BASE, './../g', 'http://a/b/g')
        self.checkJoin(RFC2396_BASE, './g/.', 'http://a/b/c/g/')
        self.checkJoin(RFC2396_BASE, 'g/./h', 'http://a/b/c/g/h')
        self.checkJoin(RFC2396_BASE, 'g/../h', 'http://a/b/c/h')
        self.checkJoin(RFC2396_BASE, 'g;x=1/./y', 'http://a/b/c/g;x=1/y')
        self.checkJoin(RFC2396_BASE, 'g;x=1/../y', 'http://a/b/c/y')
        self.checkJoin(RFC2396_BASE, 'g?y/./x', 'http://a/b/c/g?y/./x')
        self.checkJoin(RFC2396_BASE, 'g?y/../x', 'http://a/b/c/g?y/../x')
        self.checkJoin(RFC2396_BASE, 'g#s/./x', 'http://a/b/c/g#s/./x')
        self.checkJoin(RFC2396_BASE, 'g#s/../x', 'http://a/b/c/g#s/../x')

        # XXX: The following tests are no longer compatible with RFC3986
        # self.checkJoin(RFC2396_BASE, '../../../g', 'http://a/../g')
        # self.checkJoin(RFC2396_BASE, '../../../../g', 'http://a/../../g')
        # self.checkJoin(RFC2396_BASE, '/./g', 'http://a/./g')
        # self.checkJoin(RFC2396_BASE, '/../g', 'http://a/../g')

    def test_RFC3986(self):
        self.checkJoin(RFC3986_BASE, '?y','http://a/b/c/d;p?y')
        self.checkJoin(RFC3986_BASE, ';x', 'http://a/b/c/;x')
        self.checkJoin(RFC3986_BASE, 'g:h','g:h')
        self.checkJoin(RFC3986_BASE, 'g','http://a/b/c/g')
        self.checkJoin(RFC3986_BASE, './g','http://a/b/c/g')
        self.checkJoin(RFC3986_BASE, 'g/','http://a/b/c/g/')
        self.checkJoin(RFC3986_BASE, '/g','http://a/g')
        self.checkJoin(RFC3986_BASE, '//g','http://g')
        self.checkJoin(RFC3986_BASE, '?y','http://a/b/c/d;p?y')
        self.checkJoin(RFC3986_BASE, 'g?y','http://a/b/c/g?y')
        self.checkJoin(RFC3986_BASE, '#s','http://a/b/c/d;p?q#s')
        self.checkJoin(RFC3986_BASE, 'g#s','http://a/b/c/g#s')
        self.checkJoin(RFC3986_BASE, 'g?y#s','http://a/b/c/g?y#s')
        self.checkJoin(RFC3986_BASE, ';x','http://a/b/c/;x')
        self.checkJoin(RFC3986_BASE, 'g;x','http://a/b/c/g;x')
        self.checkJoin(RFC3986_BASE, 'g;x?y#s','http://a/b/c/g;x?y#s')
        self.checkJoin(RFC3986_BASE, '','http://a/b/c/d;p?q')
        self.checkJoin(RFC3986_BASE, '.','http://a/b/c/')
        self.checkJoin(RFC3986_BASE, './','http://a/b/c/')
        self.checkJoin(RFC3986_BASE, '..','http://a/b/')
        self.checkJoin(RFC3986_BASE, '../','http://a/b/')
        self.checkJoin(RFC3986_BASE, '../g','http://a/b/g')
        self.checkJoin(RFC3986_BASE, '../..','http://a/')
        self.checkJoin(RFC3986_BASE, '../../','http://a/')
        self.checkJoin(RFC3986_BASE, '../../g','http://a/g')
        self.checkJoin(RFC3986_BASE, '../../../g', 'http://a/g')

        # Abnormal Examples

        # The 'abnormal scenarios' are incompatible with RFC2986 parsing
        # Tests are here for reference.

        self.checkJoin(RFC3986_BASE, '../../../g','http://a/g')
        self.checkJoin(RFC3986_BASE, '../../../../g','http://a/g')
        self.checkJoin(RFC3986_BASE, '/./g','http://a/g')
        self.checkJoin(RFC3986_BASE, '/../g','http://a/g')
        self.checkJoin(RFC3986_BASE, 'g.','http://a/b/c/g.')
        self.checkJoin(RFC3986_BASE, '.g','http://a/b/c/.g')
        self.checkJoin(RFC3986_BASE, 'g..','http://a/b/c/g..')
        self.checkJoin(RFC3986_BASE, '..g','http://a/b/c/..g')
        self.checkJoin(RFC3986_BASE, './../g','http://a/b/g')
        self.checkJoin(RFC3986_BASE, './g/.','http://a/b/c/g/')
        self.checkJoin(RFC3986_BASE, 'g/./h','http://a/b/c/g/h')
        self.checkJoin(RFC3986_BASE, 'g/../h','http://a/b/c/h')
        self.checkJoin(RFC3986_BASE, 'g;x=1/./y','http://a/b/c/g;x=1/y')
        self.checkJoin(RFC3986_BASE, 'g;x=1/../y','http://a/b/c/y')
        self.checkJoin(RFC3986_BASE, 'g?y/./x','http://a/b/c/g?y/./x')
        self.checkJoin(RFC3986_BASE, 'g?y/../x','http://a/b/c/g?y/../x')
        self.checkJoin(RFC3986_BASE, 'g#s/./x','http://a/b/c/g#s/./x')
        self.checkJoin(RFC3986_BASE, 'g#s/../x','http://a/b/c/g#s/../x')
        #self.checkJoin(RFC3986_BASE, 'http:g','http:g') # strict parser
        self.checkJoin(RFC3986_BASE, 'http:g','http://a/b/c/g') #relaxed parser

        # Test for issue9721
        self.checkJoin('http://a/b/c/de', ';x','http://a/b/c/;x')

    def test_urljoins(self):
        self.checkJoin(SIMPLE_BASE, 'g:h','g:h')
        self.checkJoin(SIMPLE_BASE, 'g','http://a/b/c/g')
        self.checkJoin(SIMPLE_BASE, './g','http://a/b/c/g')
        self.checkJoin(SIMPLE_BASE, 'g/','http://a/b/c/g/')
        self.checkJoin(SIMPLE_BASE, '/g','http://a/g')
        self.checkJoin(SIMPLE_BASE, '//g','http://g')
        self.checkJoin(SIMPLE_BASE, '?y','http://a/b/c/d?y')
        self.checkJoin(SIMPLE_BASE, 'g?y','http://a/b/c/g?y')
        self.checkJoin(SIMPLE_BASE, 'g?y/./x','http://a/b/c/g?y/./x')
        self.checkJoin(SIMPLE_BASE, '.','http://a/b/c/')
        self.checkJoin(SIMPLE_BASE, './','http://a/b/c/')
        self.checkJoin(SIMPLE_BASE, '..','http://a/b/')
        self.checkJoin(SIMPLE_BASE, '../','http://a/b/')
        self.checkJoin(SIMPLE_BASE, '../g','http://a/b/g')
        self.checkJoin(SIMPLE_BASE, '../..','http://a/')
        self.checkJoin(SIMPLE_BASE, '../../g','http://a/g')
        self.checkJoin(SIMPLE_BASE, './../g','http://a/b/g')
        self.checkJoin(SIMPLE_BASE, './g/.','http://a/b/c/g/')
        self.checkJoin(SIMPLE_BASE, 'g/./h','http://a/b/c/g/h')
        self.checkJoin(SIMPLE_BASE, 'g/../h','http://a/b/c/h')
        self.checkJoin(SIMPLE_BASE, 'http:g','http://a/b/c/g')
        self.checkJoin(SIMPLE_BASE, 'http:g?y','http://a/b/c/g?y')
        self.checkJoin(SIMPLE_BASE, 'http:g?y/./x','http://a/b/c/g?y/./x')
        self.checkJoin('http:///', '..','http:///')
        self.checkJoin('', 'http://a/b/c/g?y/./x','http://a/b/c/g?y/./x')
        self.checkJoin('', 'http://a/./g', 'http://a/./g')
        self.checkJoin('svn://pathtorepo/dir1', 'dir2', 'svn://pathtorepo/dir2')
        self.checkJoin('svn+ssh://pathtorepo/dir1', 'dir2', 'svn+ssh://pathtorepo/dir2')
        self.checkJoin('ws://a/b','g','ws://a/g')
        self.checkJoin('wss://a/b','g','wss://a/g')

        # XXX: The following tests are no longer compatible with RFC3986
        # self.checkJoin(SIMPLE_BASE, '../../../g','http://a/../g')
        # self.checkJoin(SIMPLE_BASE, '/./g','http://a/./g')

        # test for issue22118 duplicate slashes
        self.checkJoin(SIMPLE_BASE + '/', 'foo', SIMPLE_BASE + '/foo')

        # Non-RFC-defined tests, covering variations of base and trailing
        # slashes
        self.checkJoin('http://a/b/c/d/e/', '../../f/g/', 'http://a/b/c/f/g/')
        self.checkJoin('http://a/b/c/d/e', '../../f/g/', 'http://a/b/f/g/')
        self.checkJoin('http://a/b/c/d/e/', '/../../f/g/', 'http://a/f/g/')
        self.checkJoin('http://a/b/c/d/e', '/../../f/g/', 'http://a/f/g/')
        self.checkJoin('http://a/b/c/d/e/', '../../f/g', 'http://a/b/c/f/g')
        self.checkJoin('http://a/b/', '../../f/g/', 'http://a/f/g/')

        # issue 23703: don't duplicate filename
        self.checkJoin('a', 'b', 'b')

        # Test with empty (but defined) components.
        self.checkJoin(RFC1808_BASE, '', 'http://a/b/c/d;p?q#f')
        self.checkJoin(RFC1808_BASE, '#', 'http://a/b/c/d;p?q#', relroundtrip=False)
        self.checkJoin(RFC1808_BASE, '#z', 'http://a/b/c/d;p?q#z')
        self.checkJoin(RFC1808_BASE, '?', 'http://a/b/c/d;p?', relroundtrip=False)
        self.checkJoin(RFC1808_BASE, '?#z', 'http://a/b/c/d;p?#z', relroundtrip=False)
        self.checkJoin(RFC1808_BASE, '?y', 'http://a/b/c/d;p?y')
        self.checkJoin(RFC1808_BASE, ';', 'http://a/b/c/;')
        self.checkJoin(RFC1808_BASE, ';?y', 'http://a/b/c/;?y')
        self.checkJoin(RFC1808_BASE, ';#z', 'http://a/b/c/;#z')
        self.checkJoin(RFC1808_BASE, ';x', 'http://a/b/c/;x')
        self.checkJoin(RFC1808_BASE, '/w', 'http://a/w')
        self.checkJoin(RFC1808_BASE, '//', 'http://a/b/c/d;p?q#f')
        self.checkJoin(RFC1808_BASE, '//#z', 'http://a/b/c/d;p?q#z')
        self.checkJoin(RFC1808_BASE, '//?y', 'http://a/b/c/d;p?y')
        self.checkJoin(RFC1808_BASE, '//;x', 'http://;x')
        self.checkJoin(RFC1808_BASE, '///w', 'http://a/w')
        self.checkJoin(RFC1808_BASE, '//v', 'http://v')
        # For backward compatibility with RFC1630, the scheme name is allowed
        # to be present in a relative reference if it is the same as the base
        # URI scheme.
        self.checkJoin(RFC1808_BASE, 'http:', 'http://a/b/c/d;p?q#f')
        self.checkJoin(RFC1808_BASE, 'http:#', 'http://a/b/c/d;p?q#', relroundtrip=False)
        self.checkJoin(RFC1808_BASE, 'http:#z', 'http://a/b/c/d;p?q#z')
        self.checkJoin(RFC1808_BASE, 'http:?', 'http://a/b/c/d;p?', relroundtrip=False)
        self.checkJoin(RFC1808_BASE, 'http:?#z', 'http://a/b/c/d;p?#z', relroundtrip=False)
        self.checkJoin(RFC1808_BASE, 'http:?y', 'http://a/b/c/d;p?y')
        self.checkJoin(RFC1808_BASE, 'http:;', 'http://a/b/c/;')
        self.checkJoin(RFC1808_BASE, 'http:;?y', 'http://a/b/c/;?y')
        self.checkJoin(RFC1808_BASE, 'http:;#z', 'http://a/b/c/;#z')
        self.checkJoin(RFC1808_BASE, 'http:;x', 'http://a/b/c/;x')
        self.checkJoin(RFC1808_BASE, 'http:/w', 'http://a/w')
        self.checkJoin(RFC1808_BASE, 'http://', 'http://a/b/c/d;p?q#f')
        self.checkJoin(RFC1808_BASE, 'http://#z', 'http://a/b/c/d;p?q#z')
        self.checkJoin(RFC1808_BASE, 'http://?y', 'http://a/b/c/d;p?y')
        self.checkJoin(RFC1808_BASE, 'http://;x', 'http://;x')
        self.checkJoin(RFC1808_BASE, 'http:///w', 'http://a/w')
        self.checkJoin(RFC1808_BASE, 'http://v', 'http://v')
        # Different scheme is not ignored.
        self.checkJoin(RFC1808_BASE, 'https:', 'https:', relroundtrip=False)
        self.checkJoin(RFC1808_BASE, 'https:#', 'https:#', relroundtrip=False)
        self.checkJoin(RFC1808_BASE, 'https:#z', 'https:#z', relroundtrip=False)
        self.checkJoin(RFC1808_BASE, 'https:?', 'https:?', relroundtrip=False)
        self.checkJoin(RFC1808_BASE, 'https:?y', 'https:?y', relroundtrip=False)
        self.checkJoin(RFC1808_BASE, 'https:;', 'https:;')
        self.checkJoin(RFC1808_BASE, 'https:;x', 'https:;x')

    def test_urljoins_relative_base(self):
        # According to RFC 3986, Section 5.1, a base URI must conform to
        # the absolute-URI syntax rule (Section 4.3). But urljoin() lacks
        # a context to establish missed components of the relative base URI.
        # It still has to return a sensible result for backwards compatibility.
        # The following tests are figments of the imagination and artifacts
        # of the current implementation that are not based on any standard.
        self.checkJoin('', '', '')
        self.checkJoin('', '//', '//', relroundtrip=False)
        self.checkJoin('', '//v', '//v')
        self.checkJoin('', '//v/w', '//v/w')
        self.checkJoin('', '/w', '/w')
        self.checkJoin('', '///w', '///w', relroundtrip=False)
        self.checkJoin('', 'w', 'w')

        self.checkJoin('//', '', '//')
        self.checkJoin('//', '//', '//')
        self.checkJoin('//', '//v', '//v')
        self.checkJoin('//', '//v/w', '//v/w')
        self.checkJoin('//', '/w', '///w')
        self.checkJoin('//', '///w', '///w')
        self.checkJoin('//', 'w', '///w')

        self.checkJoin('//a', '', '//a')
        self.checkJoin('//a', '//', '//a')
        self.checkJoin('//a', '//v', '//v')
        self.checkJoin('//a', '//v/w', '//v/w')
        self.checkJoin('//a', '/w', '//a/w')
        self.checkJoin('//a', '///w', '//a/w')
        self.checkJoin('//a', 'w', '//a/w')

        for scheme in '', 'http:':
            self.checkJoin('http:', scheme + '', 'http:')
            self.checkJoin('http:', scheme + '//', 'http:')
            self.checkJoin('http:', scheme + '//v', 'http://v')
            self.checkJoin('http:', scheme + '//v/w', 'http://v/w')
            self.checkJoin('http:', scheme + '/w', 'http:/w')
            self.checkJoin('http:', scheme + '///w', 'http:/w')
            self.checkJoin('http:', scheme + 'w', 'http:/w')

            self.checkJoin('http://', scheme + '', 'http://')
            self.checkJoin('http://', scheme + '//', 'http://')
            self.checkJoin('http://', scheme + '//v', 'http://v')
            self.checkJoin('http://', scheme + '//v/w', 'http://v/w')
            self.checkJoin('http://', scheme + '/w', 'http:///w')
            self.checkJoin('http://', scheme + '///w', 'http:///w')
            self.checkJoin('http://', scheme + 'w', 'http:///w')

            self.checkJoin('http://a', scheme + '', 'http://a')
            self.checkJoin('http://a', scheme + '//', 'http://a')
            self.checkJoin('http://a', scheme + '//v', 'http://v')
            self.checkJoin('http://a', scheme + '//v/w', 'http://v/w')
            self.checkJoin('http://a', scheme + '/w', 'http://a/w')
            self.checkJoin('http://a', scheme + '///w', 'http://a/w')
            self.checkJoin('http://a', scheme + 'w', 'http://a/w')

        self.checkJoin('/b/c', '', '/b/c')
        self.checkJoin('/b/c', '//', '/b/c')
        self.checkJoin('/b/c', '//v', '//v')
        self.checkJoin('/b/c', '//v/w', '//v/w')
        self.checkJoin('/b/c', '/w', '/w')
        self.checkJoin('/b/c', '///w', '/w')
        self.checkJoin('/b/c', 'w', '/b/w')

        self.checkJoin('///b/c', '', '///b/c')
        self.checkJoin('///b/c', '//', '///b/c')
        self.checkJoin('///b/c', '//v', '//v')
        self.checkJoin('///b/c', '//v/w', '//v/w')
        self.checkJoin('///b/c', '/w', '///w')
        self.checkJoin('///b/c', '///w', '///w')
        self.checkJoin('///b/c', 'w', '///b/w')

    @support.subTests('bytes', (False, True))
    @support.subTests('url,hostname,port', [
            ('http://Test.python.org:5432/foo/', 'test.python.org', 5432),
            ('http://12.34.56.78:5432/foo/', '12.34.56.78', 5432),
            ('http://[::1]:5432/foo/', '::1', 5432),
            ('http://[dead:beef::1]:5432/foo/', 'dead:beef::1', 5432),
            ('http://[dead:beef::]:5432/foo/', 'dead:beef::', 5432),
            ('http://[dead:beef:cafe:5417:affe:8FA3:deaf:feed]:5432/foo/',
             'dead:beef:cafe:5417:affe:8fa3:deaf:feed', 5432),
            ('http://[::12.34.56.78]:5432/foo/', '::12.34.56.78', 5432),
            ('http://[::ffff:12.34.56.78]:5432/foo/',
             '::ffff:12.34.56.78', 5432),
            ('http://Test.python.org/foo/', 'test.python.org', None),
            ('http://12.34.56.78/foo/', '12.34.56.78', None),
            ('http://[::1]/foo/', '::1', None),
            ('http://[dead:beef::1]/foo/', 'dead:beef::1', None),
            ('http://[dead:beef::]/foo/', 'dead:beef::', None),
            ('http://[dead:beef:cafe:5417:affe:8FA3:deaf:feed]/foo/',
             'dead:beef:cafe:5417:affe:8fa3:deaf:feed', None),
            ('http://[::12.34.56.78]/foo/', '::12.34.56.78', None),
            ('http://[::ffff:12.34.56.78]/foo/',
             '::ffff:12.34.56.78', None),
            ('http://Test.python.org:/foo/', 'test.python.org', None),
            ('http://12.34.56.78:/foo/', '12.34.56.78', None),
            ('http://[::1]:/foo/', '::1', None),
            ('http://[dead:beef::1]:/foo/', 'dead:beef::1', None),
            ('http://[dead:beef::]:/foo/', 'dead:beef::', None),
            ('http://[dead:beef:cafe:5417:affe:8FA3:deaf:feed]:/foo/',
             'dead:beef:cafe:5417:affe:8fa3:deaf:feed', None),
            ('http://[::12.34.56.78]:/foo/', '::12.34.56.78', None),
            ('http://[::ffff:12.34.56.78]:/foo/',
             '::ffff:12.34.56.78', None),
            ])
    def test_RFC2732(self, bytes, url, hostname, port):
        if bytes:
            url = str_encode(url)
            hostname = str_encode(hostname)
        urlparsed = urllib.parse.urlparse(url)
        self.assertEqual((urlparsed.hostname, urlparsed.port), (hostname, port))

    @support.subTests('bytes', (False, True))
    @support.subTests('invalid_url', [
                'http://::12.34.56.78]/',
                'http://[::1/foo/',
                'ftp://[::1/foo/bad]/bad',
                'http://[::1/foo/bad]/bad',
                'http://[::ffff:12.34.56.78'])
    def test_RFC2732_invalid(self, bytes, invalid_url):
        if bytes:
            invalid_url = str_encode(invalid_url)
        self.assertRaises(ValueError, urllib.parse.urlparse, invalid_url)

    @support.subTests('bytes', (False, True))
    @support.subTests('url,defrag,frag', [
            ('http://python.org#frag', 'http://python.org', 'frag'),
            ('http://python.org', 'http://python.org', None),
            ('http://python.org/#frag', 'http://python.org/', 'frag'),
            ('http://python.org/', 'http://python.org/', None),
            ('http://python.org/?q#frag', 'http://python.org/?q', 'frag'),
            ('http://python.org/?q', 'http://python.org/?q', None),
            ('http://python.org/p#frag', 'http://python.org/p', 'frag'),
            ('http://python.org/p?q', 'http://python.org/p?q', None),
            (RFC1808_BASE, 'http://a/b/c/d;p?q', 'f'),
            (RFC2396_BASE, 'http://a/b/c/d;p?q', None),
            ('http://a/b/c;p?q#f', 'http://a/b/c;p?q', 'f'),
            ('http://a/b/c;p?q#', 'http://a/b/c;p?q', ''),
            ('http://a/b/c;p?q', 'http://a/b/c;p?q', None),
            ('http://a/b/c;p?#f', 'http://a/b/c;p?', 'f'),
            ('http://a/b/c;p#f', 'http://a/b/c;p', 'f'),
            ('http://a/b/c;?q#f', 'http://a/b/c;?q', 'f'),
            ('http://a/b/c?q#f', 'http://a/b/c?q', 'f'),
            ('http:///b/c;p?q#f', 'http:///b/c;p?q', 'f'),
            ('http:b/c;p?q#f', 'http:b/c;p?q', 'f'),
            ('http:;?q#f', 'http:;?q', 'f'),
            ('http:?q#f', 'http:?q', 'f'),
            ('//a/b/c;p?q#f', '//a/b/c;p?q', 'f'),
            ('://a/b/c;p?q#f', '://a/b/c;p?q', 'f'),
        ])
    @support.subTests('missing_as_none', (False, True))
    def test_urldefrag(self, bytes, url, defrag, frag, missing_as_none):
        if bytes:
            url = str_encode(url)
            defrag = str_encode(defrag)
            frag = str_encode(frag)
        result = urllib.parse.urldefrag(url, missing_as_none=missing_as_none)
        if not missing_as_none:
            hash = '#' if isinstance(url, str) else b'#'
            url = url.rstrip(hash)
            if frag is None:
                frag = url[:0]
        self.assertEqual(result.geturl(), url)
        self.assertEqual(result, (defrag, frag))
        self.assertEqual(result.url, defrag)
        self.assertEqual(result.fragment, frag)

    def test_urlsplit_scoped_IPv6(self):
        p = urllib.parse.urlsplit('http://[FE80::822a:a8ff:fe49:470c%tESt]:1234')
        self.assertEqual(p.hostname, "fe80::822a:a8ff:fe49:470c%tESt")
        self.assertEqual(p.netloc, '[FE80::822a:a8ff:fe49:470c%tESt]:1234')

        p = urllib.parse.urlsplit(b'http://[FE80::822a:a8ff:fe49:470c%tESt]:1234')
        self.assertEqual(p.hostname, b"fe80::822a:a8ff:fe49:470c%tESt")
        self.assertEqual(p.netloc, b'[FE80::822a:a8ff:fe49:470c%tESt]:1234')

    def test_urlsplit_attributes(self):
        url = "HTTP://WWW.PYTHON.ORG/doc/#frag"
        p = urllib.parse.urlsplit(url)
        self.assertEqual(p.scheme, "http")
        self.assertEqual(p.netloc, "WWW.PYTHON.ORG")
        self.assertEqual(p.path, "/doc/")
        self.assertEqual(p.query, "")
        self.assertEqual(p.fragment, "frag")
        self.assertEqual(p.username, None)
        self.assertEqual(p.password, None)
        self.assertEqual(p.hostname, "www.python.org")
        self.assertEqual(p.port, None)
        # geturl() won't return exactly the original URL in this case
        # since the scheme is always case-normalized
        # We handle this by ignoring the first 4 characters of the URL
        self.assertEqual(p.geturl()[4:], url[4:])

        url = "http://User:Pass@www.python.org:080/doc/?query=yes#frag"
        p = urllib.parse.urlsplit(url)
        self.assertEqual(p.scheme, "http")
        self.assertEqual(p.netloc, "User:Pass@www.python.org:080")
        self.assertEqual(p.path, "/doc/")
        self.assertEqual(p.query, "query=yes")
        self.assertEqual(p.fragment, "frag")
        self.assertEqual(p.username, "User")
        self.assertEqual(p.password, "Pass")
        self.assertEqual(p.hostname, "www.python.org")
        self.assertEqual(p.port, 80)
        self.assertEqual(p.geturl(), url)

        # Addressing issue1698, which suggests Username can contain
        # "@" characters.  Though not RFC compliant, many ftp sites allow
        # and request email addresses as usernames.

        url = "http://User@example.com:Pass@www.python.org:080/doc/?query=yes#frag"
        p = urllib.parse.urlsplit(url)
        self.assertEqual(p.scheme, "http")
        self.assertEqual(p.netloc, "User@example.com:Pass@www.python.org:080")
        self.assertEqual(p.path, "/doc/")
        self.assertEqual(p.query, "query=yes")
        self.assertEqual(p.fragment, "frag")
        self.assertEqual(p.username, "User@example.com")
        self.assertEqual(p.password, "Pass")
        self.assertEqual(p.hostname, "www.python.org")
        self.assertEqual(p.port, 80)
        self.assertEqual(p.geturl(), url)

        # And check them all again, only with bytes this time
        url = b"HTTP://WWW.PYTHON.ORG/doc/#frag"
        p = urllib.parse.urlsplit(url)
        self.assertEqual(p.scheme, b"http")
        self.assertEqual(p.netloc, b"WWW.PYTHON.ORG")
        self.assertEqual(p.path, b"/doc/")
        self.assertEqual(p.query, b"")
        self.assertEqual(p.fragment, b"frag")
        self.assertEqual(p.username, None)
        self.assertEqual(p.password, None)
        self.assertEqual(p.hostname, b"www.python.org")
        self.assertEqual(p.port, None)
        self.assertEqual(p.geturl()[4:], url[4:])

        url = b"http://User:Pass@www.python.org:080/doc/?query=yes#frag"
        p = urllib.parse.urlsplit(url)
        self.assertEqual(p.scheme, b"http")
        self.assertEqual(p.netloc, b"User:Pass@www.python.org:080")
        self.assertEqual(p.path, b"/doc/")
        self.assertEqual(p.query, b"query=yes")
        self.assertEqual(p.fragment, b"frag")
        self.assertEqual(p.username, b"User")
        self.assertEqual(p.password, b"Pass")
        self.assertEqual(p.hostname, b"www.python.org")
        self.assertEqual(p.port, 80)
        self.assertEqual(p.geturl(), url)

        url = b"http://User@example.com:Pass@www.python.org:080/doc/?query=yes#frag"
        p = urllib.parse.urlsplit(url)
        self.assertEqual(p.scheme, b"http")
        self.assertEqual(p.netloc, b"User@example.com:Pass@www.python.org:080")
        self.assertEqual(p.path, b"/doc/")
        self.assertEqual(p.query, b"query=yes")
        self.assertEqual(p.fragment, b"frag")
        self.assertEqual(p.username, b"User@example.com")
        self.assertEqual(p.password, b"Pass")
        self.assertEqual(p.hostname, b"www.python.org")
        self.assertEqual(p.port, 80)
        self.assertEqual(p.geturl(), url)

        # Verify an illegal port raises ValueError
        url = b"HTTP://WWW.PYTHON.ORG:65536/doc/#frag"
        p = urllib.parse.urlsplit(url)
        with self.assertRaisesRegex(ValueError, "out of range"):
            p.port

    def test_urlsplit_remove_unsafe_bytes(self):
        # Remove ASCII tabs and newlines from input
        url = "http\t://www.python\n.org\t/java\nscript:\talert('msg\r\n')/?query\n=\tsomething#frag\nment"
        p = urllib.parse.urlsplit(url)
        self.assertEqual(p.scheme, "http")
        self.assertEqual(p.netloc, "www.python.org")
        self.assertEqual(p.path, "/javascript:alert('msg')/")
        self.assertEqual(p.query, "query=something")
        self.assertEqual(p.fragment, "fragment")
        self.assertEqual(p.username, None)
        self.assertEqual(p.password, None)
        self.assertEqual(p.hostname, "www.python.org")
        self.assertEqual(p.port, None)
        self.assertEqual(p.geturl(), "http://www.python.org/javascript:alert('msg')/?query=something#fragment")

        # Remove ASCII tabs and newlines from input as bytes.
        url = b"http\t://www.python\n.org\t/java\nscript:\talert('msg\r\n')/?query\n=\tsomething#frag\nment"
        p = urllib.parse.urlsplit(url)
        self.assertEqual(p.scheme, b"http")
        self.assertEqual(p.netloc, b"www.python.org")
        self.assertEqual(p.path, b"/javascript:alert('msg')/")
        self.assertEqual(p.query, b"query=something")
        self.assertEqual(p.fragment, b"fragment")
        self.assertEqual(p.username, None)
        self.assertEqual(p.password, None)
        self.assertEqual(p.hostname, b"www.python.org")
        self.assertEqual(p.port, None)
        self.assertEqual(p.geturl(), b"http://www.python.org/javascript:alert('msg')/?query=something#fragment")

        # with scheme as cache-key
        url = "http://www.python.org/java\nscript:\talert('msg\r\n')/?query\n=\tsomething#frag\nment"
        scheme = "ht\ntp"
        for _ in range(2):
            p = urllib.parse.urlsplit(url, scheme=scheme)
            self.assertEqual(p.scheme, "http")
            self.assertEqual(p.geturl(), "http://www.python.org/javascript:alert('msg')/?query=something#fragment")

    def test_urlsplit_strip_url(self):
        noise = bytes(range(0, 0x20 + 1))
        base_url = "http://User:Pass@www.python.org:080/doc/?query=yes#frag"

        url = noise.decode("utf-8") + base_url
        p = urllib.parse.urlsplit(url)
        self.assertEqual(p.scheme, "http")
        self.assertEqual(p.netloc, "User:Pass@www.python.org:080")
        self.assertEqual(p.path, "/doc/")
        self.assertEqual(p.query, "query=yes")
        self.assertEqual(p.fragment, "frag")
        self.assertEqual(p.username, "User")
        self.assertEqual(p.password, "Pass")
        self.assertEqual(p.hostname, "www.python.org")
        self.assertEqual(p.port, 80)
        self.assertEqual(p.geturl(), base_url)

        url = noise + base_url.encode("utf-8")
        p = urllib.parse.urlsplit(url)
        self.assertEqual(p.scheme, b"http")
        self.assertEqual(p.netloc, b"User:Pass@www.python.org:080")
        self.assertEqual(p.path, b"/doc/")
        self.assertEqual(p.query, b"query=yes")
        self.assertEqual(p.fragment, b"frag")
        self.assertEqual(p.username, b"User")
        self.assertEqual(p.password, b"Pass")
        self.assertEqual(p.hostname, b"www.python.org")
        self.assertEqual(p.port, 80)
        self.assertEqual(p.geturl(), base_url.encode("utf-8"))

        # Test that trailing space is preserved as some applications rely on
        # this within query strings.
        query_spaces_url = "https://www.python.org:88/doc/?query=    "
        p = urllib.parse.urlsplit(noise.decode("utf-8") + query_spaces_url)
        self.assertEqual(p.scheme, "https")
        self.assertEqual(p.netloc, "www.python.org:88")
        self.assertEqual(p.path, "/doc/")
        self.assertEqual(p.query, "query=    ")
        self.assertEqual(p.port, 88)
        self.assertEqual(p.geturl(), query_spaces_url)

        p = urllib.parse.urlsplit("www.pypi.org ")
        # That "hostname" gets considered a "path" due to the
        # trailing space and our existing logic...  YUCK...
        # and re-assembles via geturl aka unurlsplit into the original.
        # django.core.validators.URLValidator (at least through v3.2) relies on
        # this, for better or worse, to catch it in a ValidationError via its
        # regular expressions.
        # Here we test the basic round trip concept of such a trailing space.
        self.assertEqual(urllib.parse.urlunsplit(p), "www.pypi.org ")

        # with scheme as cache-key
        url = "//www.python.org/"
        scheme = noise.decode("utf-8") + "https" + noise.decode("utf-8")
        for _ in range(2):
            p = urllib.parse.urlsplit(url, scheme=scheme)
            self.assertEqual(p.scheme, "https")
            self.assertEqual(p.geturl(), "https://www.python.org/")

    @support.subTests('bytes', (False, True))
    @support.subTests('parse', (urllib.parse.urlsplit, urllib.parse.urlparse))
    @support.subTests('port', ("foo", "1.5", "-1", "0x10", "-0", "1_1", " 1", "1 ", "६"))
    def test_attributes_bad_port(self, bytes, parse, port):
        """Check handling of invalid ports."""
        netloc = "www.example.net:" + port
        url = "http://" + netloc + "/"
        if bytes:
            if not (netloc.isascii() and port.isascii()):
                self.skipTest('non-ASCII bytes')
            netloc = str_encode(netloc)
            url = str_encode(url)
        p = parse(url)
        self.assertEqual(p.netloc, netloc)
        with self.assertRaises(ValueError):
            p.port

    @support.subTests('bytes', (False, True))
    @support.subTests('parse', (urllib.parse.urlsplit, urllib.parse.urlparse))
    @support.subTests('scheme', (".", "+", "-", "0", "http&", "६http"))
    def test_attributes_bad_scheme(self, bytes, parse, scheme):
        """Check handling of invalid schemes."""
        url = scheme + "://www.example.net"
        if bytes:
            if not url.isascii():
                self.skipTest('non-ASCII bytes')
            url = url.encode("ascii")
        p = parse(url, missing_as_none=True)
        self.assertIsNone(p.scheme)

    @support.subTests('missing_as_none', (False, True))
    def test_attributes_without_netloc(self, missing_as_none):
        # This example is straight from RFC 3261.  It looks like it
        # should allow the username, hostname, and port to be filled
        # in, but doesn't.  Since it's a URI and doesn't use the
        # scheme://netloc syntax, the netloc and related attributes
        # should be left empty.
        uri = "sip:alice@atlanta.com;maddr=239.255.255.1;ttl=15"
        p = urllib.parse.urlsplit(uri, missing_as_none=missing_as_none)
        self.assertEqual(p.netloc, None if missing_as_none else "")
        self.assertEqual(p.username, None)
        self.assertEqual(p.password, None)
        self.assertEqual(p.hostname, None)
        self.assertEqual(p.port, None)
        self.assertEqual(p.geturl(), uri)

        p = urllib.parse.urlparse(uri, missing_as_none=missing_as_none)
        self.assertEqual(p.netloc, None if missing_as_none else "")
        self.assertEqual(p.username, None)
        self.assertEqual(p.password, None)
        self.assertEqual(p.hostname, None)
        self.assertEqual(p.port, None)
        self.assertEqual(p.geturl(), uri)

        # You guessed it, repeating the test with bytes input
        uri = b"sip:alice@atlanta.com;maddr=239.255.255.1;ttl=15"
        p = urllib.parse.urlsplit(uri, missing_as_none=missing_as_none)
        self.assertEqual(p.netloc, None if missing_as_none else b"")
        self.assertEqual(p.username, None)
        self.assertEqual(p.password, None)
        self.assertEqual(p.hostname, None)
        self.assertEqual(p.port, None)
        self.assertEqual(p.geturl(), uri)

        p = urllib.parse.urlparse(uri, missing_as_none=missing_as_none)
        self.assertEqual(p.netloc, None if missing_as_none else b"")
        self.assertEqual(p.username, None)
        self.assertEqual(p.password, None)
        self.assertEqual(p.hostname, None)
        self.assertEqual(p.port, None)
        self.assertEqual(p.geturl(), uri)

    def test_noslash(self):
        # Issue 1637: http://foo.com?query is legal
        self.assertEqual(urllib.parse.urlparse("http://example.com?blahblah=/foo"),
                         ('http', 'example.com', '', '', 'blahblah=/foo', ''))
        self.assertEqual(urllib.parse.urlparse(b"http://example.com?blahblah=/foo"),
                         (b'http', b'example.com', b'', b'', b'blahblah=/foo', b''))

    @support.subTests('missing_as_none', (False, True))
    def test_withoutscheme(self, missing_as_none):
        # Test urlparse without scheme
        # Issue 754016: urlparse goes wrong with IP:port without scheme
        # RFC 1808 specifies that netloc should start with //, urlparse expects
        # the same, otherwise it classifies the portion of url as path.
        none = None if missing_as_none else ''
        self.assertEqual(urlparse("path", missing_as_none=missing_as_none),
                (none, none, 'path', none, none, none))
        self.assertEqual(urlparse("//www.python.org:80", missing_as_none=missing_as_none),
                (none, 'www.python.org:80', '', none, none, none))
        self.assertEqual(urlparse("http://www.python.org:80", missing_as_none=missing_as_none),
                ('http', 'www.python.org:80', '', none, none, none))
        # Repeat for bytes input
        none = None if missing_as_none else b''
        self.assertEqual(urlparse(b"path", missing_as_none=missing_as_none),
                (none, none, b'path', none, none, none))
        self.assertEqual(urlparse(b"//www.python.org:80", missing_as_none=missing_as_none),
                (none, b'www.python.org:80', b'', none, none, none))
        self.assertEqual(urlparse(b"http://www.python.org:80", missing_as_none=missing_as_none),
                (b'http', b'www.python.org:80', b'', none, none, none))

    @support.subTests('missing_as_none', (False, True))
    def test_portseparator(self, missing_as_none):
        # Issue 754016 makes changes for port separator ':' from scheme separator
        none = None if missing_as_none else ''
        self.assertEqual(urlparse("http:80", missing_as_none=missing_as_none),
                ('http', none, '80', none, none, none))
        self.assertEqual(urlparse("https:80", missing_as_none=missing_as_none),
                ('https', none, '80', none, none, none))
        self.assertEqual(urlparse("path:80", missing_as_none=missing_as_none),
                ('path', none, '80', none, none, none))
        self.assertEqual(urlparse("http:", missing_as_none=missing_as_none),
                ('http', none, '', none, none, none))
        self.assertEqual(urlparse("https:", missing_as_none=missing_as_none),
                ('https', none, '', none, none, none))
        self.assertEqual(urlparse("http://www.python.org:80", missing_as_none=missing_as_none),
                ('http', 'www.python.org:80', '', none, none, none))
        # As usual, need to check bytes input as well
        none = None if missing_as_none else b''
        self.assertEqual(urlparse(b"http:80", missing_as_none=missing_as_none),
                (b'http', none, b'80', none, none, none))
        self.assertEqual(urlparse(b"https:80", missing_as_none=missing_as_none),
                (b'https', none, b'80', none, none, none))
        self.assertEqual(urlparse(b"path:80", missing_as_none=missing_as_none),
                (b'path', none, b'80', none, none, none))
        self.assertEqual(urlparse(b"http:", missing_as_none=missing_as_none),
                (b'http', none, b'', none, none, none))
        self.assertEqual(urlparse(b"https:", missing_as_none=missing_as_none),
                (b'https', none, b'', none, none, none))
        self.assertEqual(urlparse(b"http://www.python.org:80", missing_as_none=missing_as_none),
                (b'http', b'www.python.org:80', b'', none, none, none))

    def test_usingsys(self):
        # Issue 3314: sys module is used in the error
        self.assertRaises(TypeError, urllib.parse.urlencode, "foo")

    @support.subTests('missing_as_none', (False, True))
    def test_anyscheme(self, missing_as_none):
        # Issue 7904: s3://foo.com/stuff has netloc "foo.com".
        none = None if missing_as_none else ''
        self.assertEqual(urlparse("s3://foo.com/stuff", missing_as_none=missing_as_none),
                         ('s3', 'foo.com', '/stuff', none, none, none))
        self.assertEqual(urlparse("x-newscheme://foo.com/stuff", missing_as_none=missing_as_none),
                         ('x-newscheme', 'foo.com', '/stuff', none, none, none))
        self.assertEqual(urlparse("x-newscheme://foo.com/stuff?query#fragment", missing_as_none=missing_as_none),
                         ('x-newscheme', 'foo.com', '/stuff', none, 'query', 'fragment'))
        self.assertEqual(urlparse("x-newscheme://foo.com/stuff?query", missing_as_none=missing_as_none),
                         ('x-newscheme', 'foo.com', '/stuff', none, 'query', none))

        # And for bytes...
        none = None if missing_as_none else b''
        self.assertEqual(urlparse(b"s3://foo.com/stuff", missing_as_none=missing_as_none),
                         (b's3', b'foo.com', b'/stuff', none, none, none))
        self.assertEqual(urlparse(b"x-newscheme://foo.com/stuff", missing_as_none=missing_as_none),
                         (b'x-newscheme', b'foo.com', b'/stuff', none, none, none))
        self.assertEqual(urlparse(b"x-newscheme://foo.com/stuff?query#fragment", missing_as_none=missing_as_none),
                         (b'x-newscheme', b'foo.com', b'/stuff', none, b'query', b'fragment'))
        self.assertEqual(urlparse(b"x-newscheme://foo.com/stuff?query", missing_as_none=missing_as_none),
                         (b'x-newscheme', b'foo.com', b'/stuff', none, b'query', none))

    @support.subTests('func', (urllib.parse.urlparse, urllib.parse.urlsplit))
    def test_default_scheme(self, func):
        # Exercise the scheme parameter of urlparse() and urlsplit()
        result = func("http://example.net/", "ftp")
        self.assertEqual(result.scheme, "http")
        result = func(b"http://example.net/", b"ftp")
        self.assertEqual(result.scheme, b"http")
        self.assertEqual(func("path", "ftp").scheme, "ftp")
        self.assertEqual(func("path", scheme="ftp").scheme, "ftp")
        self.assertEqual(func(b"path", scheme=b"ftp").scheme, b"ftp")
        self.assertEqual(func("path").scheme, "")
        self.assertEqual(func("path", missing_as_none=True).scheme, None)
        self.assertEqual(func(b"path").scheme, b"")
        self.assertEqual(func(b"path", missing_as_none=True).scheme, None)
        self.assertEqual(func(b"path", "").scheme, b"")
        self.assertEqual(func(b"path", "", missing_as_none=True).scheme, b"")

    @support.subTests('url,attr,expected_frag', (
            ("http:#frag", "path", "frag"),
            ("//example.net#frag", "path", "frag"),
            ("index.html#frag", "path", "frag"),
            (";a=b#frag", "params", "frag"),
            ("?a=b#frag", "query", "frag"),
            ("#frag", "path", "frag"),
            ("abc#@frag", "path", "@frag"),
            ("//abc#@frag", "path", "@frag"),
            ("//abc:80#@frag", "path", "@frag"),
            ("//abc#@frag:80", "path", "@frag:80"),
        ))
    @support.subTests('func', (urllib.parse.urlparse, urllib.parse.urlsplit))
    def test_parse_fragments(self, url, attr, expected_frag, func):
        # Exercise the allow_fragments parameter of urlparse() and urlsplit()
        if attr == "params" and func is urllib.parse.urlsplit:
            attr = "path"
        result = func(url, allow_fragments=False)
        self.assertEqual(result.fragment, "")
        self.assertEndsWith(getattr(result, attr),
                            "#" + expected_frag)
        self.assertEqual(func(url, "", False).fragment, "")

        result = func(url, allow_fragments=False, missing_as_none=True)
        self.assertIsNone(result.fragment)
        self.assertTrue(
                getattr(result, attr).endswith("#" + expected_frag))
        self.assertIsNone(func(url, "", False, missing_as_none=True).fragment)

        result = func(url, allow_fragments=True)
        self.assertEqual(result.fragment, expected_frag)
        self.assertFalse(
                getattr(result, attr).endswith(expected_frag))
        self.assertEqual(func(url, "", True).fragment,
                            expected_frag)
        self.assertEqual(func(url).fragment, expected_frag)

    def test_mixed_types_rejected(self):
        # Several functions that process either strings or ASCII encoded bytes
        # accept multiple arguments. Check they reject mixed type input
        with self.assertRaisesRegex(TypeError, "Cannot mix str"):
            urllib.parse.urlparse("www.python.org", b"http")
        with self.assertRaisesRegex(TypeError, "Cannot mix str"):
            urllib.parse.urlparse(b"www.python.org", "http")
        with self.assertRaisesRegex(TypeError, "Cannot mix str"):
            urllib.parse.urlsplit("www.python.org", b"http")
        with self.assertRaisesRegex(TypeError, "Cannot mix str"):
            urllib.parse.urlsplit(b"www.python.org", "http")
        with self.assertRaisesRegex(TypeError, "Cannot mix str"):
            urllib.parse.urlunparse(( b"http", "www.python.org","","","",""))
        with self.assertRaisesRegex(TypeError, "Cannot mix str"):
            urllib.parse.urlunparse(("http", b"www.python.org","","","",""))
        with self.assertRaisesRegex(TypeError, "Cannot mix str"):
            urllib.parse.urlunsplit((b"http", "www.python.org","","",""))
        with self.assertRaisesRegex(TypeError, "Cannot mix str"):
            urllib.parse.urlunsplit(("http", b"www.python.org","","",""))
        with self.assertRaisesRegex(TypeError, "Cannot mix str"):
            urllib.parse.urljoin("http://python.org", b"http://python.org")
        with self.assertRaisesRegex(TypeError, "Cannot mix str"):
            urllib.parse.urljoin(b"http://python.org", "http://python.org")

    def _check_result_type(self, str_type, str_args):
        bytes_type = str_type._encoded_counterpart
        self.assertIs(bytes_type._decoded_counterpart, str_type)
        bytes_args = tuple_encode(str_args)
        str_result = str_type(*str_args)
        bytes_result = bytes_type(*bytes_args)
        encoding = 'ascii'
        errors = 'strict'
        self.assertEqual(str_result, str_args)
        self.assertEqual(bytes_result.decode(), str_args)
        self.assertEqual(bytes_result.decode(), str_result)
        self.assertEqual(bytes_result.decode(encoding), str_args)
        self.assertEqual(bytes_result.decode(encoding), str_result)
        self.assertEqual(bytes_result.decode(encoding, errors), str_args)
        self.assertEqual(bytes_result.decode(encoding, errors), str_result)
        self.assertEqual(bytes_result, bytes_args)
        self.assertEqual(str_result.encode(), bytes_args)
        self.assertEqual(str_result.encode(), bytes_result)
        self.assertEqual(str_result.encode(encoding), bytes_args)
        self.assertEqual(str_result.encode(encoding), bytes_result)
        self.assertEqual(str_result.encode(encoding, errors), bytes_args)
        self.assertEqual(str_result.encode(encoding, errors), bytes_result)
        for result in str_result, bytes_result:
            self.assertEqual(copy.copy(result), result)
            self.assertEqual(copy.deepcopy(result), result)
            self.assertEqual(copy.replace(result), result)
            self.assertEqual(result._replace(), result)

    def test_result_pairs__(self):
        # Check encoding and decoding between result pairs
        self._check_result_type(urllib.parse.DefragResult, ('', ''))
        self._check_result_type(urllib.parse.DefragResult, ('', None))
        self._check_result_type(urllib.parse.SplitResult, ('', '', '', '', ''))
        self._check_result_type(urllib.parse.SplitResult, (None, None, '', None, None))
        self._check_result_type(urllib.parse.ParseResult, ('', '', '', '', '', ''))
        self._check_result_type(urllib.parse.ParseResult, (None, None, '', None, None, None))

    def test_result_encoding_decoding(self):
        def check(str_result, bytes_result):
            self.assertEqual(str_result.encode(), bytes_result)
            self.assertEqual(str_result.encode().geturl(), bytes_result.geturl())
            self.assertEqual(bytes_result.decode(), str_result)
            self.assertEqual(bytes_result.decode().geturl(), str_result.geturl())

        url = 'http://example.com/?#'
        burl = url.encode()
        for func in urldefrag, urlsplit, urlparse:
            check(func(url, missing_as_none=True), func(burl, missing_as_none=True))
            check(func(url), func(burl))

    def test_result_copying(self):
        def check(result):
            result2 = copy.copy(result)
            self.assertEqual(result2, result)
            self.assertEqual(result2.geturl(), result.geturl())
            result2 = copy.deepcopy(result)
            self.assertEqual(result2, result)
            self.assertEqual(result2.geturl(), result.geturl())
            result2 = copy.replace(result)
            self.assertEqual(result2, result)
            self.assertEqual(result2.geturl(), result.geturl())
            result2 = result._replace()
            self.assertEqual(result2, result)
            self.assertEqual(result2.geturl(), result.geturl())

        url = 'http://example.com/?#'
        burl = url.encode()
        for func in urldefrag, urlsplit, urlparse:
            check(func(url))
            check(func(url, missing_as_none=True))
            check(func(burl))
            check(func(burl, missing_as_none=True))

    def test_result_pickling(self):
        def check(result):
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                pickled = pickle.dumps(result, proto)
                result2 = pickle.loads(pickled)
                self.assertEqual(result2, result)
                self.assertEqual(result2.geturl(), result.geturl())

        url = 'http://example.com/?#'
        burl = url.encode()
        for func in urldefrag, urlsplit, urlparse:
            check(func(url))
            check(func(url, missing_as_none=True))
            check(func(burl))
            check(func(burl, missing_as_none=True))

    def test_result_compat_unpickling(self):
        def check(result, pickles):
            for pickled in pickles:
                result2 = pickle.loads(pickled)
                self.assertEqual(result2, result)
                self.assertEqual(result2.geturl(), result.geturl())

        url = 'http://example.com/?#'
        burl = url.encode()
        # Pre-3.15 data.
        check(urldefrag(url), (
            b'ccopy_reg\n_reconstructor\n(curlparse\nDefragResult\nc__builtin__\ntuple\n(Vhttp://example.com/?\nV\nttR.',
            b'ccopy_reg\n_reconstructor\n(curlparse\nDefragResult\nc__builtin__\ntuple\n(X\x14\x00\x00\x00http://example.com/?X\x00\x00\x00\x00ttR.',
            b'\x80\x02curlparse\nDefragResult\nX\x14\x00\x00\x00http://example.com/?X\x00\x00\x00\x00\x86\x81.',
            b'\x80\x03curllib.parse\nDefragResult\nX\x14\x00\x00\x00http://example.com/?X\x00\x00\x00\x00\x86\x81.',
            b'\x80\x04\x958\x00\x00\x00\x00\x00\x00\x00\x8c\x0curllib.parse\x8c\x0cDefragResult\x93\x8c\x14http://example.com/?\x8c\x00\x86\x81.',
        ))
        check(urldefrag(burl), (
            b'ccopy_reg\n_reconstructor\n(curlparse\nDefragResultBytes\nc__builtin__\ntuple\n(c_codecs\nencode\n(Vhttp://example.com/?\nVlatin1\ntRc__builtin__\nbytes\n(tRttR.',
            b'ccopy_reg\n_reconstructor\n(curlparse\nDefragResultBytes\nc__builtin__\ntuple\n(c_codecs\nencode\n(X\x14\x00\x00\x00http://example.com/?X\x06\x00\x00\x00latin1tRc__builtin__\nbytes\n)RttR.',
            b'\x80\x02curlparse\nDefragResultBytes\nc_codecs\nencode\nX\x14\x00\x00\x00http://example.com/?X\x06\x00\x00\x00latin1\x86Rc__builtin__\nbytes\n)R\x86\x81.',
            b'\x80\x03curllib.parse\nDefragResultBytes\nC\x14http://example.com/?C\x00\x86\x81.',
            b'\x80\x04\x95=\x00\x00\x00\x00\x00\x00\x00\x8c\x0curllib.parse\x8c\x11DefragResultBytes\x93C\x14http://example.com/?C\x00\x86\x81.',
        ))
        check(urlsplit(url), (
            b'ccopy_reg\n_reconstructor\n(curlparse\nSplitResult\nc__builtin__\ntuple\n(Vhttp\nVexample.com\nV/\nV\np0\ng0\nttR.',
            b'ccopy_reg\n_reconstructor\n(curlparse\nSplitResult\nc__builtin__\ntuple\n(X\x04\x00\x00\x00httpX\x0b\x00\x00\x00example.comX\x01\x00\x00\x00/X\x00\x00\x00\x00q\x00h\x00ttR.',
            b'\x80\x02curlparse\nSplitResult\n(X\x04\x00\x00\x00httpX\x0b\x00\x00\x00example.comX\x01\x00\x00\x00/X\x00\x00\x00\x00q\x00h\x00t\x81.',
            b'\x80\x03curllib.parse\nSplitResult\n(X\x04\x00\x00\x00httpX\x0b\x00\x00\x00example.comX\x01\x00\x00\x00/X\x00\x00\x00\x00q\x00h\x00t\x81.',
            b'\x80\x04\x95;\x00\x00\x00\x00\x00\x00\x00\x8c\x0curllib.parse\x8c\x0bSplitResult\x93(\x8c\x04http\x8c\x0bexample.com\x8c\x01/\x8c\x00\x94h\x00t\x81.',
        ))
        check(urlsplit(burl), (
            b'ccopy_reg\n_reconstructor\n(curlparse\nSplitResultBytes\nc__builtin__\ntuple\n(c_codecs\nencode\np0\n(Vhttp\nVlatin1\np1\ntRg0\n(Vexample.com\ng1\ntRg0\n(V/\ng1\ntRc__builtin__\nbytes\n(tRp2\ng2\nttR.',
            b'ccopy_reg\n_reconstructor\n(curlparse\nSplitResultBytes\nc__builtin__\ntuple\n(c_codecs\nencode\nq\x00(X\x04\x00\x00\x00httpX\x06\x00\x00\x00latin1q\x01tRh\x00(X\x0b\x00\x00\x00example.comh\x01tRh\x00(X\x01\x00\x00\x00/h\x01tRc__builtin__\nbytes\n)Rq\x02h\x02ttR.',
            b'\x80\x02curlparse\nSplitResultBytes\n(c_codecs\nencode\nq\x00X\x04\x00\x00\x00httpX\x06\x00\x00\x00latin1q\x01\x86Rh\x00X\x0b\x00\x00\x00example.comh\x01\x86Rh\x00X\x01\x00\x00\x00/h\x01\x86Rc__builtin__\nbytes\n)Rq\x02h\x02t\x81.',
            b'\x80\x03curllib.parse\nSplitResultBytes\n(C\x04httpC\x0bexample.comC\x01/C\x00q\x00h\x00t\x81.',
            b'\x80\x04\x95@\x00\x00\x00\x00\x00\x00\x00\x8c\x0curllib.parse\x8c\x10SplitResultBytes\x93(C\x04httpC\x0bexample.comC\x01/C\x00\x94h\x00t\x81.',
        ))
        check(urlparse(url), (
            b'ccopy_reg\n_reconstructor\n(curlparse\nParseResult\nc__builtin__\ntuple\n(Vhttp\nVexample.com\nV/\nV\np0\ng0\ng0\nttR.',
            b'ccopy_reg\n_reconstructor\n(curlparse\nParseResult\nc__builtin__\ntuple\n(X\x04\x00\x00\x00httpX\x0b\x00\x00\x00example.comX\x01\x00\x00\x00/X\x00\x00\x00\x00q\x00h\x00h\x00ttR.',
            b'\x80\x02curlparse\nParseResult\n(X\x04\x00\x00\x00httpX\x0b\x00\x00\x00example.comX\x01\x00\x00\x00/X\x00\x00\x00\x00q\x00h\x00h\x00t\x81.',
            b'\x80\x03curllib.parse\nParseResult\n(X\x04\x00\x00\x00httpX\x0b\x00\x00\x00example.comX\x01\x00\x00\x00/X\x00\x00\x00\x00q\x00h\x00h\x00t\x81.',
            b'\x80\x04\x95=\x00\x00\x00\x00\x00\x00\x00\x8c\x0curllib.parse\x8c\x0bParseResult\x93(\x8c\x04http\x8c\x0bexample.com\x8c\x01/\x8c\x00\x94h\x00h\x00t\x81.',
        ))
        check(urlparse(burl), (
            b'ccopy_reg\n_reconstructor\n(curlparse\nParseResultBytes\nc__builtin__\ntuple\n(c_codecs\nencode\np0\n(Vhttp\nVlatin1\np1\ntRg0\n(Vexample.com\ng1\ntRg0\n(V/\ng1\ntRc__builtin__\nbytes\n(tRp2\ng2\ng2\nttR.',
            b'ccopy_reg\n_reconstructor\n(curlparse\nParseResultBytes\nc__builtin__\ntuple\n(c_codecs\nencode\nq\x00(X\x04\x00\x00\x00httpX\x06\x00\x00\x00latin1q\x01tRh\x00(X\x0b\x00\x00\x00example.comh\x01tRh\x00(X\x01\x00\x00\x00/h\x01tRc__builtin__\nbytes\n)Rq\x02h\x02h\x02ttR.',
            b'\x80\x02curlparse\nParseResultBytes\n(c_codecs\nencode\nq\x00X\x04\x00\x00\x00httpX\x06\x00\x00\x00latin1q\x01\x86Rh\x00X\x0b\x00\x00\x00example.comh\x01\x86Rh\x00X\x01\x00\x00\x00/h\x01\x86Rc__builtin__\nbytes\n)Rq\x02h\x02h\x02t\x81.',
            b'\x80\x03curllib.parse\nParseResultBytes\n(C\x04httpC\x0bexample.comC\x01/C\x00q\x00h\x00h\x00t\x81.',
            b'\x80\x04\x95B\x00\x00\x00\x00\x00\x00\x00\x8c\x0curllib.parse\x8c\x10ParseResultBytes\x93(C\x04httpC\x0bexample.comC\x01/C\x00\x94h\x00h\x00t\x81.',
        ))

        # 3.15 data with missing_as_none=True.
        check(urldefrag(url, missing_as_none=True), (
            b'ccopy_reg\n_reconstructor\n(curlparse\nDefragResult\nc__builtin__\ntuple\n(Vhttp://example.com/?\nV\nttR(N(dV_keep_empty\nI01\nstb.',
            b'ccopy_reg\n_reconstructor\n(curlparse\nDefragResult\nc__builtin__\ntuple\n(X\x14\x00\x00\x00http://example.com/?X\x00\x00\x00\x00ttR(N}X\x0b\x00\x00\x00_keep_emptyI01\nstb.',
            b'\x80\x02curlparse\nDefragResult\nX\x14\x00\x00\x00http://example.com/?X\x00\x00\x00\x00\x86\x81N}X\x0b\x00\x00\x00_keep_empty\x88s\x86b.',
            b'\x80\x03curllib.parse\nDefragResult\nX\x14\x00\x00\x00http://example.com/?X\x00\x00\x00\x00\x86\x81N}X\x0b\x00\x00\x00_keep_empty\x88s\x86b.',
            b'\x80\x04\x95K\x00\x00\x00\x00\x00\x00\x00\x8c\x0curllib.parse\x8c\x0cDefragResult\x93\x8c\x14http://example.com/?\x8c\x00\x86\x81N}\x8c\x0b_keep_empty\x88s\x86b.',
        ))
        check(urldefrag(burl, missing_as_none=True), (
            b'ccopy_reg\n_reconstructor\n(curlparse\nDefragResultBytes\nc__builtin__\ntuple\n(c_codecs\nencode\n(Vhttp://example.com/?\nVlatin1\ntRc__builtin__\nbytes\n(tRttR(N(dV_keep_empty\nI01\nstb.',
            b'ccopy_reg\n_reconstructor\n(curlparse\nDefragResultBytes\nc__builtin__\ntuple\n(c_codecs\nencode\n(X\x14\x00\x00\x00http://example.com/?X\x06\x00\x00\x00latin1tRc__builtin__\nbytes\n)RttR(N}X\x0b\x00\x00\x00_keep_emptyI01\nstb.',
            b'\x80\x02curlparse\nDefragResultBytes\nc_codecs\nencode\nX\x14\x00\x00\x00http://example.com/?X\x06\x00\x00\x00latin1\x86Rc__builtin__\nbytes\n)R\x86\x81N}X\x0b\x00\x00\x00_keep_empty\x88s\x86b.',
            b'\x80\x03curllib.parse\nDefragResultBytes\nC\x14http://example.com/?C\x00\x86\x81N}X\x0b\x00\x00\x00_keep_empty\x88s\x86b.',
            b'\x80\x04\x95P\x00\x00\x00\x00\x00\x00\x00\x8c\x0curllib.parse\x8c\x11DefragResultBytes\x93C\x14http://example.com/?C\x00\x86\x81N}\x8c\x0b_keep_empty\x88s\x86b.',
        ))
        check(urlsplit(url, missing_as_none=True), (
            b'ccopy_reg\n_reconstructor\n(curlparse\nSplitResult\nc__builtin__\ntuple\n(Vhttp\nVexample.com\nV/\nV\np0\ng0\nttR(N(dV_keep_empty\nI01\nstb.',
            b'ccopy_reg\n_reconstructor\n(curlparse\nSplitResult\nc__builtin__\ntuple\n(X\x04\x00\x00\x00httpX\x0b\x00\x00\x00example.comX\x01\x00\x00\x00/X\x00\x00\x00\x00q\x00h\x00ttR(N}X\x0b\x00\x00\x00_keep_emptyI01\nstb.',
            b'\x80\x02curlparse\nSplitResult\n(X\x04\x00\x00\x00httpX\x0b\x00\x00\x00example.comX\x01\x00\x00\x00/X\x00\x00\x00\x00q\x00h\x00t\x81N}X\x0b\x00\x00\x00_keep_empty\x88s\x86b.',
            b'\x80\x03curllib.parse\nSplitResult\n(X\x04\x00\x00\x00httpX\x0b\x00\x00\x00example.comX\x01\x00\x00\x00/X\x00\x00\x00\x00q\x00h\x00t\x81N}X\x0b\x00\x00\x00_keep_empty\x88s\x86b.',
            b'\x80\x04\x95N\x00\x00\x00\x00\x00\x00\x00\x8c\x0curllib.parse\x8c\x0bSplitResult\x93(\x8c\x04http\x8c\x0bexample.com\x8c\x01/\x8c\x00\x94h\x00t\x81N}\x8c\x0b_keep_empty\x88s\x86b.',
        ))
        check(urlsplit(burl, missing_as_none=True), (
            b'ccopy_reg\n_reconstructor\n(curlparse\nSplitResultBytes\nc__builtin__\ntuple\n(c_codecs\nencode\np0\n(Vhttp\nVlatin1\np1\ntRg0\n(Vexample.com\ng1\ntRg0\n(V/\ng1\ntRc__builtin__\nbytes\n(tRp2\ng2\nttR(N(dV_keep_empty\nI01\nstb.',
            b'ccopy_reg\n_reconstructor\n(curlparse\nSplitResultBytes\nc__builtin__\ntuple\n(c_codecs\nencode\nq\x00(X\x04\x00\x00\x00httpX\x06\x00\x00\x00latin1q\x01tRh\x00(X\x0b\x00\x00\x00example.comh\x01tRh\x00(X\x01\x00\x00\x00/h\x01tRc__builtin__\nbytes\n)Rq\x02h\x02ttR(N}X\x0b\x00\x00\x00_keep_emptyI01\nstb.',
            b'\x80\x02curlparse\nSplitResultBytes\n(c_codecs\nencode\nq\x00X\x04\x00\x00\x00httpX\x06\x00\x00\x00latin1q\x01\x86Rh\x00X\x0b\x00\x00\x00example.comh\x01\x86Rh\x00X\x01\x00\x00\x00/h\x01\x86Rc__builtin__\nbytes\n)Rq\x02h\x02t\x81N}X\x0b\x00\x00\x00_keep_empty\x88s\x86b.',
            b'\x80\x03curllib.parse\nSplitResultBytes\n(C\x04httpC\x0bexample.comC\x01/C\x00q\x00h\x00t\x81N}X\x0b\x00\x00\x00_keep_empty\x88s\x86b.',
            b'\x80\x04\x95S\x00\x00\x00\x00\x00\x00\x00\x8c\x0curllib.parse\x8c\x10SplitResultBytes\x93(C\x04httpC\x0bexample.comC\x01/C\x00\x94h\x00t\x81N}\x8c\x0b_keep_empty\x88s\x86b.',
        ))
        check(urlparse(url, missing_as_none=True), (
            b'ccopy_reg\n_reconstructor\n(curlparse\nParseResult\nc__builtin__\ntuple\n(Vhttp\nVexample.com\nV/\nNV\np0\ng0\nttR(N(dV_keep_empty\nI01\nstb.',
            b'ccopy_reg\n_reconstructor\n(curlparse\nParseResult\nc__builtin__\ntuple\n(X\x04\x00\x00\x00httpX\x0b\x00\x00\x00example.comX\x01\x00\x00\x00/NX\x00\x00\x00\x00q\x00h\x00ttR(N}X\x0b\x00\x00\x00_keep_emptyI01\nstb.',
            b'\x80\x02curlparse\nParseResult\n(X\x04\x00\x00\x00httpX\x0b\x00\x00\x00example.comX\x01\x00\x00\x00/NX\x00\x00\x00\x00q\x00h\x00t\x81N}X\x0b\x00\x00\x00_keep_empty\x88s\x86b.',
            b'\x80\x03curllib.parse\nParseResult\n(X\x04\x00\x00\x00httpX\x0b\x00\x00\x00example.comX\x01\x00\x00\x00/NX\x00\x00\x00\x00q\x00h\x00t\x81N}X\x0b\x00\x00\x00_keep_empty\x88s\x86b.',
            b'\x80\x04\x95O\x00\x00\x00\x00\x00\x00\x00\x8c\x0curllib.parse\x8c\x0bParseResult\x93(\x8c\x04http\x8c\x0bexample.com\x8c\x01/N\x8c\x00\x94h\x00t\x81N}\x8c\x0b_keep_empty\x88s\x86b.',
        ))
        check(urlparse(burl, missing_as_none=True), (
            b'ccopy_reg\n_reconstructor\n(curlparse\nParseResultBytes\nc__builtin__\ntuple\n(c_codecs\nencode\np0\n(Vhttp\nVlatin1\np1\ntRg0\n(Vexample.com\ng1\ntRg0\n(V/\ng1\ntRNc__builtin__\nbytes\n(tRp2\ng2\nttR(N(dV_keep_empty\nI01\nstb.',
            b'ccopy_reg\n_reconstructor\n(curlparse\nParseResultBytes\nc__builtin__\ntuple\n(c_codecs\nencode\nq\x00(X\x04\x00\x00\x00httpX\x06\x00\x00\x00latin1q\x01tRh\x00(X\x0b\x00\x00\x00example.comh\x01tRh\x00(X\x01\x00\x00\x00/h\x01tRNc__builtin__\nbytes\n)Rq\x02h\x02ttR(N}X\x0b\x00\x00\x00_keep_emptyI01\nstb.',
            b'\x80\x02curlparse\nParseResultBytes\n(c_codecs\nencode\nq\x00X\x04\x00\x00\x00httpX\x06\x00\x00\x00latin1q\x01\x86Rh\x00X\x0b\x00\x00\x00example.comh\x01\x86Rh\x00X\x01\x00\x00\x00/h\x01\x86RNc__builtin__\nbytes\n)Rq\x02h\x02t\x81N}X\x0b\x00\x00\x00_keep_empty\x88s\x86b.',
            b'\x80\x03curllib.parse\nParseResultBytes\n(C\x04httpC\x0bexample.comC\x01/NC\x00q\x00h\x00t\x81N}X\x0b\x00\x00\x00_keep_empty\x88s\x86b.',
            b'\x80\x04\x95T\x00\x00\x00\x00\x00\x00\x00\x8c\x0curllib.parse\x8c\x10ParseResultBytes\x93(C\x04httpC\x0bexample.comC\x01/NC\x00\x94h\x00t\x81N}\x8c\x0b_keep_empty\x88s\x86b.',
        ))

    def test_parse_qs_encoding(self):
        result = urllib.parse.parse_qs("key=\u0141%E9", encoding="latin-1")
        self.assertEqual(result, {'key': ['\u0141\xE9']})
        result = urllib.parse.parse_qs("key=\u0141%C3%A9", encoding="utf-8")
        self.assertEqual(result, {'key': ['\u0141\xE9']})
        result = urllib.parse.parse_qs("key=\u0141%C3%A9", encoding="ascii")
        self.assertEqual(result, {'key': ['\u0141\ufffd\ufffd']})
        result = urllib.parse.parse_qs("key=\u0141%E9-", encoding="ascii")
        self.assertEqual(result, {'key': ['\u0141\ufffd-']})
        result = urllib.parse.parse_qs("key=\u0141%E9-", encoding="ascii",
                                                          errors="ignore")
        self.assertEqual(result, {'key': ['\u0141-']})

    def test_parse_qsl_encoding(self):
        result = urllib.parse.parse_qsl("key=\u0141%E9", encoding="latin-1")
        self.assertEqual(result, [('key', '\u0141\xE9')])
        result = urllib.parse.parse_qsl("key=\u0141%C3%A9", encoding="utf-8")
        self.assertEqual(result, [('key', '\u0141\xE9')])
        result = urllib.parse.parse_qsl("key=\u0141%C3%A9", encoding="ascii")
        self.assertEqual(result, [('key', '\u0141\ufffd\ufffd')])
        result = urllib.parse.parse_qsl("key=\u0141%E9-", encoding="ascii")
        self.assertEqual(result, [('key', '\u0141\ufffd-')])
        result = urllib.parse.parse_qsl("key=\u0141%E9-", encoding="ascii",
                                                          errors="ignore")
        self.assertEqual(result, [('key', '\u0141-')])

    def test_parse_qsl_max_num_fields(self):
        with self.assertRaises(ValueError):
            urllib.parse.parse_qsl('&'.join(['a=a']*11), max_num_fields=10)
        urllib.parse.parse_qsl('&'.join(['a=a']*10), max_num_fields=10)

    @support.subTests('orig,expect', [
            (";", {}),
            (";;", {}),
            (";a=b", {'a': ['b']}),
            ("a=a+b;b=b+c", {'a': ['a b'], 'b': ['b c']}),
            ("a=1;a=2", {'a': ['1', '2']}),
            (b";", {}),
            (b";;", {}),
            (b";a=b", {b'a': [b'b']}),
            (b"a=a+b;b=b+c", {b'a': [b'a b'], b'b': [b'b c']}),
            (b"a=1;a=2", {b'a': [b'1', b'2']}),
        ])
    def test_parse_qs_separator(self, orig, expect):
        result = urllib.parse.parse_qs(orig, separator=';')
        self.assertEqual(result, expect)
        result_bytes = urllib.parse.parse_qs(orig, separator=b';')
        self.assertEqual(result_bytes, expect)

    @support.subTests('orig,expect', [
            (";", []),
            (";;", []),
            (";a=b", [('a', 'b')]),
            ("a=a+b;b=b+c", [('a', 'a b'), ('b', 'b c')]),
            ("a=1;a=2", [('a', '1'), ('a', '2')]),
            (b";", []),
            (b";;", []),
            (b";a=b", [(b'a', b'b')]),
            (b"a=a+b;b=b+c", [(b'a', b'a b'), (b'b', b'b c')]),
            (b"a=1;a=2", [(b'a', b'1'), (b'a', b'2')]),
        ])
    def test_parse_qsl_separator(self, orig, expect):
        result = urllib.parse.parse_qsl(orig, separator=';')
        self.assertEqual(result, expect)
        result_bytes = urllib.parse.parse_qsl(orig, separator=b';')
        self.assertEqual(result_bytes, expect)

    def test_parse_qsl_bytes(self):
        self.assertEqual(urllib.parse.parse_qsl(b'a=b'), [(b'a', b'b')])
        self.assertEqual(urllib.parse.parse_qsl(bytearray(b'a=b')), [(b'a', b'b')])
        self.assertEqual(urllib.parse.parse_qsl(memoryview(b'a=b')), [(b'a', b'b')])

    def test_parse_qsl_false_value(self):
        kwargs = dict(keep_blank_values=True, strict_parsing=True)
        for x in '', b'', None, memoryview(b''):
            self.assertEqual(urllib.parse.parse_qsl(x, **kwargs), [])
            self.assertRaises(ValueError, urllib.parse.parse_qsl, x, separator=1)
        for x in 0, 0.0, [], {}:
            with self.assertWarns(DeprecationWarning) as cm:
                self.assertEqual(urllib.parse.parse_qsl(x, **kwargs), [])
            self.assertEqual(cm.filename, __file__)
            with self.assertWarns(DeprecationWarning) as cm:
                self.assertEqual(urllib.parse.parse_qs(x, **kwargs), {})
            self.assertEqual(cm.filename, __file__)
            self.assertRaises(ValueError, urllib.parse.parse_qsl, x, separator=1)

    def test_parse_qsl_errors(self):
        self.assertRaises(TypeError, urllib.parse.parse_qsl, list(b'a=b'))
        self.assertRaises(TypeError, urllib.parse.parse_qsl, iter(b'a=b'))
        self.assertRaises(TypeError, urllib.parse.parse_qsl, 1)
        self.assertRaises(TypeError, urllib.parse.parse_qsl, object())

        for separator in '', b'', None, 0, 1, 0.0, 1.5:
            with self.assertRaises(ValueError):
                urllib.parse.parse_qsl('a=b', separator=separator)
        with self.assertRaises(UnicodeEncodeError):
            urllib.parse.parse_qsl(b'a=b', separator='\xa6')
        with self.assertRaises(UnicodeDecodeError):
            urllib.parse.parse_qsl('a=b', separator=b'\xa6')

    def test_urlencode_sequences(self):
        # Other tests incidentally urlencode things; test non-covered cases:
        # Sequence and object values.
        result = urllib.parse.urlencode({'a': [1, 2], 'b': (3, 4, 5)}, True)
        # we cannot rely on ordering here
        assert set(result.split('&')) == {'a=1', 'a=2', 'b=3', 'b=4', 'b=5'}

        class Trivial:
            def __str__(self):
                return 'trivial'

        result = urllib.parse.urlencode({'a': Trivial()}, True)
        self.assertEqual(result, 'a=trivial')

    def test_urlencode_quote_via(self):
        result = urllib.parse.urlencode({'a': 'some value'})
        self.assertEqual(result, "a=some+value")
        result = urllib.parse.urlencode({'a': 'some value/another'},
                                        quote_via=urllib.parse.quote)
        self.assertEqual(result, "a=some%20value%2Fanother")
        result = urllib.parse.urlencode({'a': 'some value/another'},
                                        safe='/', quote_via=urllib.parse.quote)
        self.assertEqual(result, "a=some%20value/another")

    def test_quote_from_bytes(self):
        self.assertRaises(TypeError, urllib.parse.quote_from_bytes, 'foo')
        result = urllib.parse.quote_from_bytes(b'archaeological arcana')
        self.assertEqual(result, 'archaeological%20arcana')
        result = urllib.parse.quote_from_bytes(b'')
        self.assertEqual(result, '')
        result = urllib.parse.quote_from_bytes(b'A'*10_000)
        self.assertEqual(result, 'A'*10_000)
        result = urllib.parse.quote_from_bytes(b'z\x01/ '*253_183)
        self.assertEqual(result, 'z%01/%20'*253_183)

    def test_unquote_to_bytes(self):
        result = urllib.parse.unquote_to_bytes('abc%20def')
        self.assertEqual(result, b'abc def')
        result = urllib.parse.unquote_to_bytes('')
        self.assertEqual(result, b'')

    def test_quote_errors(self):
        self.assertRaises(TypeError, urllib.parse.quote, b'foo',
                          encoding='utf-8')
        self.assertRaises(TypeError, urllib.parse.quote, b'foo', errors='strict')

    def test_issue14072(self):
        p1 = urllib.parse.urlsplit('tel:+31-641044153')
        self.assertEqual(p1.scheme, 'tel')
        self.assertEqual(p1.path, '+31-641044153')
        p2 = urllib.parse.urlsplit('tel:+31641044153')
        self.assertEqual(p2.scheme, 'tel')
        self.assertEqual(p2.path, '+31641044153')
        # assert the behavior for urlparse
        p1 = urllib.parse.urlparse('tel:+31-641044153')
        self.assertEqual(p1.scheme, 'tel')
        self.assertEqual(p1.path, '+31-641044153')
        p2 = urllib.parse.urlparse('tel:+31641044153')
        self.assertEqual(p2.scheme, 'tel')
        self.assertEqual(p2.path, '+31641044153')

    def test_invalid_bracketed_hosts(self):
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'Scheme://user@[192.0.2.146]/Path?Query')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'Scheme://user@[important.com:8000]/Path?Query')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'Scheme://user@[v123r.IP]/Path?Query')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'Scheme://user@[v12ae]/Path?Query')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'Scheme://user@[v.IP]/Path?Query')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'Scheme://user@[v123.]/Path?Query')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'Scheme://user@[v]/Path?Query')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'Scheme://user@[0439:23af::2309::fae7:1234]/Path?Query')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'Scheme://user@[0439:23af:2309::fae7:1234:2342:438e:192.0.2.146]/Path?Query')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'Scheme://user@]v6a.ip[/Path')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix.[v6a.ip]')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://[v6a.ip].suffix')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix.[v6a.ip]/')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://[v6a.ip].suffix/')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix.[v6a.ip]?')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://[v6a.ip].suffix?')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix.[::1]')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://[::1].suffix')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix.[::1]/')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://[::1].suffix/')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix.[::1]?')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://[::1].suffix?')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix.[::1]:a')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://[::1].suffix:a')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix.[::1]:a1')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://[::1].suffix:a1')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix.[::1]:1a')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://[::1].suffix:1a')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix.[::1]:')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://[::1].suffix:/')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix.[::1]:?')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://user@prefix.[v6a.ip]')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://user@[v6a.ip].suffix')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://[v6a.ip')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://v6a.ip]')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://]v6a.ip[')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://]v6a.ip')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://v6a.ip[')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix.[v6a.ip')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://v6a.ip].suffix')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix]v6a.ip[suffix')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://prefix]v6a.ip')
        self.assertRaises(ValueError, urllib.parse.urlsplit, 'scheme://v6a.ip[suffix')

    def test_splitting_bracketed_hosts(self):
        p1 = urllib.parse.urlsplit('scheme://user@[v6a.ip]:1234/path?query')
        self.assertEqual(p1.hostname, 'v6a.ip')
        self.assertEqual(p1.username, 'user')
        self.assertEqual(p1.path, '/path')
        self.assertEqual(p1.port, 1234)
        p2 = urllib.parse.urlsplit('scheme://user@[0439:23af:2309::fae7%test]/path?query')
        self.assertEqual(p2.hostname, '0439:23af:2309::fae7%test')
        self.assertEqual(p2.username, 'user')
        self.assertEqual(p2.path, '/path')
        self.assertIs(p2.port, None)
        p3 = urllib.parse.urlsplit('scheme://user@[0439:23af:2309::fae7:1234:192.0.2.146%test]/path?query')
        self.assertEqual(p3.hostname, '0439:23af:2309::fae7:1234:192.0.2.146%test')
        self.assertEqual(p3.username, 'user')
        self.assertEqual(p3.path, '/path')

    def test_port_casting_failure_message(self):
        message = "Port could not be cast to integer value as 'oracle'"
        p1 = urllib.parse.urlparse('http://Server=sde; Service=sde:oracle')
        with self.assertRaisesRegex(ValueError, message):
            p1.port

        p2 = urllib.parse.urlsplit('http://Server=sde; Service=sde:oracle')
        with self.assertRaisesRegex(ValueError, message):
            p2.port

    def test_telurl_params(self):
        p1 = urllib.parse.urlparse('tel:123-4;phone-context=+1-650-516')
        self.assertEqual(p1.scheme, 'tel')
        self.assertEqual(p1.path, '123-4')
        self.assertEqual(p1.params, 'phone-context=+1-650-516')

        p1 = urllib.parse.urlparse('tel:+1-201-555-0123')
        self.assertEqual(p1.scheme, 'tel')
        self.assertEqual(p1.path, '+1-201-555-0123')
        self.assertEqual(p1.params, '')

        p1 = urllib.parse.urlparse('tel:+1-201-555-0123', missing_as_none=True)
        self.assertEqual(p1.scheme, 'tel')
        self.assertEqual(p1.path, '+1-201-555-0123')
        self.assertEqual(p1.params, None)

        p1 = urllib.parse.urlparse('tel:7042;phone-context=example.com')
        self.assertEqual(p1.scheme, 'tel')
        self.assertEqual(p1.path, '7042')
        self.assertEqual(p1.params, 'phone-context=example.com')

        p1 = urllib.parse.urlparse('tel:863-1234;phone-context=+1-914-555')
        self.assertEqual(p1.scheme, 'tel')
        self.assertEqual(p1.path, '863-1234')
        self.assertEqual(p1.params, 'phone-context=+1-914-555')

    def test_Quoter_repr(self):
        quoter = urllib.parse._Quoter(urllib.parse._ALWAYS_SAFE)
        self.assertIn('Quoter', repr(quoter))

    def test_clear_cache_for_code_coverage(self):
        urllib.parse.clear_cache()

    def test_urllib_parse_getattr_failure(self):
        """Test that urllib.parse.__getattr__() fails correctly."""
        with self.assertRaises(AttributeError):
            unused = urllib.parse.this_does_not_exist

    def test_all(self):
        expected = []
        undocumented = {
            'splitattr', 'splithost', 'splitnport', 'splitpasswd',
            'splitport', 'splitquery', 'splittag', 'splittype', 'splituser',
            'splitvalue',
            'ResultBase', 'clear_cache', 'to_bytes', 'unwrap',
        }
        for name in dir(urllib.parse):
            if name.startswith('_') or name in undocumented:
                continue
            object = getattr(urllib.parse, name)
            if getattr(object, '__module__', None) == 'urllib.parse':
                expected.append(name)
        self.assertCountEqual(urllib.parse.__all__, expected)

    def test_urlsplit_normalization(self):
        # Certain characters should never occur in the netloc,
        # including under normalization.
        # Ensure that ALL of them are detected and cause an error
        illegal_chars = '/:#?@'
        hex_chars = {'{:04X}'.format(ord(c)) for c in illegal_chars}
        denorm_chars = [
            c for c in map(chr, range(128, sys.maxunicode))
            if unicodedata.decomposition(c)
            and (hex_chars & set(unicodedata.decomposition(c).split()))
            and c not in illegal_chars
        ]
        # Sanity check that we found at least one such character
        self.assertIn('\u2100', denorm_chars)
        self.assertIn('\uFF03', denorm_chars)

        # bpo-36742: Verify port separators are ignored when they
        # existed prior to decomposition
        urllib.parse.urlsplit('http://\u30d5\u309a:80')
        with self.assertRaises(ValueError):
            urllib.parse.urlsplit('http://\u30d5\u309a\ufe1380')

        for scheme in ["http", "https", "ftp"]:
            for netloc in ["netloc{}false.netloc", "n{}user@netloc"]:
                for c in denorm_chars:
                    url = "{}://{}/path".format(scheme, netloc.format(c))
                    with self.subTest(url=url, char='{:04X}'.format(ord(c))):
                        with self.assertRaises(ValueError):
                            urllib.parse.urlsplit(url)

class Utility_Tests(unittest.TestCase):
    """Testcase to test the various utility functions in the urllib."""
    # In Python 2 this test class was in test_urllib.

    def test_splittype(self):
        splittype = urllib.parse._splittype
        self.assertEqual(splittype('type:opaquestring'), ('type', 'opaquestring'))
        self.assertEqual(splittype('opaquestring'), (None, 'opaquestring'))
        self.assertEqual(splittype(':opaquestring'), (None, ':opaquestring'))
        self.assertEqual(splittype('type:'), ('type', ''))
        self.assertEqual(splittype('type:opaque:string'), ('type', 'opaque:string'))

    def test_splithost(self):
        splithost = urllib.parse._splithost
        self.assertEqual(splithost('//www.example.org:80/foo/bar/baz.html'),
                         ('www.example.org:80', '/foo/bar/baz.html'))
        self.assertEqual(splithost('//www.example.org:80'),
                         ('www.example.org:80', ''))
        self.assertEqual(splithost('/foo/bar/baz.html'),
                         (None, '/foo/bar/baz.html'))

        # bpo-30500: # starts a fragment.
        self.assertEqual(splithost('//127.0.0.1#@host.com'),
                         ('127.0.0.1', '/#@host.com'))
        self.assertEqual(splithost('//127.0.0.1#@host.com:80'),
                         ('127.0.0.1', '/#@host.com:80'))
        self.assertEqual(splithost('//127.0.0.1:80#@host.com'),
                         ('127.0.0.1:80', '/#@host.com'))

        # Empty host is returned as empty string.
        self.assertEqual(splithost("///file"),
                         ('', '/file'))

        # Trailing semicolon, question mark and hash symbol are kept.
        self.assertEqual(splithost("//example.net/file;"),
                         ('example.net', '/file;'))
        self.assertEqual(splithost("//example.net/file?"),
                         ('example.net', '/file?'))
        self.assertEqual(splithost("//example.net/file#"),
                         ('example.net', '/file#'))

    def test_splituser(self):
        splituser = urllib.parse._splituser
        self.assertEqual(splituser('User:Pass@www.python.org:080'),
                         ('User:Pass', 'www.python.org:080'))
        self.assertEqual(splituser('@www.python.org:080'),
                         ('', 'www.python.org:080'))
        self.assertEqual(splituser('www.python.org:080'),
                         (None, 'www.python.org:080'))
        self.assertEqual(splituser('User:Pass@'),
                         ('User:Pass', ''))
        self.assertEqual(splituser('User@example.com:Pass@www.python.org:080'),
                         ('User@example.com:Pass', 'www.python.org:080'))

    def test_splitpasswd(self):
        # Some of the password examples are not sensible, but it is added to
        # confirming to RFC2617 and addressing issue4675.
        splitpasswd = urllib.parse._splitpasswd
        self.assertEqual(splitpasswd('user:ab'), ('user', 'ab'))
        self.assertEqual(splitpasswd('user:a\nb'), ('user', 'a\nb'))
        self.assertEqual(splitpasswd('user:a\tb'), ('user', 'a\tb'))
        self.assertEqual(splitpasswd('user:a\rb'), ('user', 'a\rb'))
        self.assertEqual(splitpasswd('user:a\fb'), ('user', 'a\fb'))
        self.assertEqual(splitpasswd('user:a\vb'), ('user', 'a\vb'))
        self.assertEqual(splitpasswd('user:a:b'), ('user', 'a:b'))
        self.assertEqual(splitpasswd('user:a b'), ('user', 'a b'))
        self.assertEqual(splitpasswd('user 2:ab'), ('user 2', 'ab'))
        self.assertEqual(splitpasswd('user+1:a+b'), ('user+1', 'a+b'))
        self.assertEqual(splitpasswd('user:'), ('user', ''))
        self.assertEqual(splitpasswd('user'), ('user', None))
        self.assertEqual(splitpasswd(':ab'), ('', 'ab'))

    def test_splitport(self):
        splitport = urllib.parse._splitport
        self.assertEqual(splitport('parrot:88'), ('parrot', '88'))
        self.assertEqual(splitport('parrot'), ('parrot', None))
        self.assertEqual(splitport('parrot:'), ('parrot', None))
        self.assertEqual(splitport('127.0.0.1'), ('127.0.0.1', None))
        self.assertEqual(splitport('parrot:cheese'), ('parrot:cheese', None))
        self.assertEqual(splitport('[::1]:88'), ('[::1]', '88'))
        self.assertEqual(splitport('[::1]'), ('[::1]', None))
        self.assertEqual(splitport(':88'), ('', '88'))

    def test_splitnport(self):
        splitnport = urllib.parse._splitnport
        self.assertEqual(splitnport('parrot:88'), ('parrot', 88))
        self.assertEqual(splitnport('parrot'), ('parrot', -1))
        self.assertEqual(splitnport('parrot', 55), ('parrot', 55))
        self.assertEqual(splitnport('parrot:'), ('parrot', -1))
        self.assertEqual(splitnport('parrot:', 55), ('parrot', 55))
        self.assertEqual(splitnport('127.0.0.1'), ('127.0.0.1', -1))
        self.assertEqual(splitnport('127.0.0.1', 55), ('127.0.0.1', 55))
        self.assertEqual(splitnport('parrot:cheese'), ('parrot', None))
        self.assertEqual(splitnport('parrot:cheese', 55), ('parrot', None))
        self.assertEqual(splitnport('parrot: +1_0 '), ('parrot', None))

    def test_splitquery(self):
        # Normal cases are exercised by other tests; ensure that we also
        # catch cases with no port specified (testcase ensuring coverage)
        splitquery = urllib.parse._splitquery
        self.assertEqual(splitquery('http://python.org/fake?foo=bar'),
                         ('http://python.org/fake', 'foo=bar'))
        self.assertEqual(splitquery('http://python.org/fake?foo=bar?'),
                         ('http://python.org/fake?foo=bar', ''))
        self.assertEqual(splitquery('http://python.org/fake'),
                         ('http://python.org/fake', None))
        self.assertEqual(splitquery('?foo=bar'), ('', 'foo=bar'))

    def test_splittag(self):
        splittag = urllib.parse._splittag
        self.assertEqual(splittag('http://example.com?foo=bar#baz'),
                         ('http://example.com?foo=bar', 'baz'))
        self.assertEqual(splittag('http://example.com?foo=bar#'),
                         ('http://example.com?foo=bar', ''))
        self.assertEqual(splittag('#baz'), ('', 'baz'))
        self.assertEqual(splittag('http://example.com?foo=bar'),
                         ('http://example.com?foo=bar', None))
        self.assertEqual(splittag('http://example.com?foo=bar#baz#boo'),
                         ('http://example.com?foo=bar#baz', 'boo'))

    def test_splitattr(self):
        splitattr = urllib.parse._splitattr
        self.assertEqual(splitattr('/path;attr1=value1;attr2=value2'),
                         ('/path', ['attr1=value1', 'attr2=value2']))
        self.assertEqual(splitattr('/path;'), ('/path', ['']))
        self.assertEqual(splitattr(';attr1=value1;attr2=value2'),
                         ('', ['attr1=value1', 'attr2=value2']))
        self.assertEqual(splitattr('/path'), ('/path', []))

    def test_splitvalue(self):
        # Normal cases are exercised by other tests; test pathological cases
        # with no key/value pairs. (testcase ensuring coverage)
        splitvalue = urllib.parse._splitvalue
        self.assertEqual(splitvalue('foo=bar'), ('foo', 'bar'))
        self.assertEqual(splitvalue('foo='), ('foo', ''))
        self.assertEqual(splitvalue('=bar'), ('', 'bar'))
        self.assertEqual(splitvalue('foobar'), ('foobar', None))
        self.assertEqual(splitvalue('foo=bar=baz'), ('foo', 'bar=baz'))

    def test_to_bytes(self):
        result = urllib.parse._to_bytes('http://www.python.org')
        self.assertEqual(result, 'http://www.python.org')
        self.assertRaises(UnicodeError, urllib.parse._to_bytes,
                          'http://www.python.org/medi\u00e6val')

    @support.subTests('wrapped_url',
                          ('<URL:scheme://host/path>', '<scheme://host/path>',
                           'URL:scheme://host/path', 'scheme://host/path'))
    def test_unwrap(self, wrapped_url):
        url = urllib.parse.unwrap(wrapped_url)
        self.assertEqual(url, 'scheme://host/path')


class DeprecationTest(unittest.TestCase):
    def test_splittype_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            urllib.parse.splittype('')
        self.assertEqual(str(cm.warning),
                         'urllib.parse.splittype() is deprecated as of 3.8, '
                         'use urllib.parse.urlsplit() instead')

    def test_splithost_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            urllib.parse.splithost('')
        self.assertEqual(str(cm.warning),
                         'urllib.parse.splithost() is deprecated as of 3.8, '
                         'use urllib.parse.urlsplit() instead')

    def test_splituser_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            urllib.parse.splituser('')
        self.assertEqual(str(cm.warning),
                         'urllib.parse.splituser() is deprecated as of 3.8, '
                         'use urllib.parse.urlsplit() instead')

    def test_splitpasswd_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            urllib.parse.splitpasswd('')
        self.assertEqual(str(cm.warning),
                         'urllib.parse.splitpasswd() is deprecated as of 3.8, '
                         'use urllib.parse.urlsplit() instead')

    def test_splitport_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            urllib.parse.splitport('')
        self.assertEqual(str(cm.warning),
                         'urllib.parse.splitport() is deprecated as of 3.8, '
                         'use urllib.parse.urlsplit() instead')

    def test_splitnport_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            urllib.parse.splitnport('')
        self.assertEqual(str(cm.warning),
                         'urllib.parse.splitnport() is deprecated as of 3.8, '
                         'use urllib.parse.urlsplit() instead')

    def test_splitquery_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            urllib.parse.splitquery('')
        self.assertEqual(str(cm.warning),
                         'urllib.parse.splitquery() is deprecated as of 3.8, '
                         'use urllib.parse.urlsplit() instead')

    def test_splittag_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            urllib.parse.splittag('')
        self.assertEqual(str(cm.warning),
                         'urllib.parse.splittag() is deprecated as of 3.8, '
                         'use urllib.parse.urlsplit() instead')

    def test_splitattr_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            urllib.parse.splitattr('')
        self.assertEqual(str(cm.warning),
                         'urllib.parse.splitattr() is deprecated as of 3.8, '
                         'use urllib.parse.urlsplit() instead')

    def test_splitvalue_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            urllib.parse.splitvalue('')
        self.assertEqual(str(cm.warning),
                         'urllib.parse.splitvalue() is deprecated as of 3.8, '
                         'use urllib.parse.parse_qsl() instead')

    def test_to_bytes_deprecation(self):
        with self.assertWarns(DeprecationWarning) as cm:
            urllib.parse.to_bytes('')
        self.assertEqual(str(cm.warning),
                         'urllib.parse.to_bytes() is deprecated as of 3.8')


def str_encode(s):
    if s is None:
        return None
    return s.encode('ascii')

def tuple_encode(t):
    return tuple(str_encode(x) for x in t)

if __name__ == "__main__":
    unittest.main()
