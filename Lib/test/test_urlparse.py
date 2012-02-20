#! /usr/bin/env python3

from test import support
import unittest
import urllib.parse

RFC1808_BASE = "http://a/b/c/d;p?q#f"
RFC2396_BASE = "http://a/b/c/d;p?q"
RFC3986_BASE = 'http://a/b/c/d;p?q'
SIMPLE_BASE  = 'http://a/b/c/d'

# A list of test cases.  Each test case is a two-tuple that contains
# a string with the query and a dictionary with the expected result.

parse_qsl_test_cases = [
    ("", []),
    ("&", []),
    ("&&", []),
    ("=", [('', '')]),
    ("=a", [('', 'a')]),
    ("a", [('a', '')]),
    ("a=", [('a', '')]),
    ("a=", [('a', '')]),
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
    (b"a=", [(b'a', b'')]),
    (b"&a=b", [(b'a', b'b')]),
    (b"a=a+b&b=b+c", [(b'a', b'a b'), (b'b', b'b c')]),
    (b"a=1&a=2", [(b'a', b'1'), (b'a', b'2')]),
]

class UrlParseTestCase(unittest.TestCase):

    def checkRoundtrips(self, url, parsed, split):
        result = urllib.parse.urlparse(url)
        self.assertEqual(result, parsed)
        t = (result.scheme, result.netloc, result.path,
             result.params, result.query, result.fragment)
        self.assertEqual(t, parsed)
        # put it back together and it should be the same
        result2 = urllib.parse.urlunparse(result)
        self.assertEqual(result2, url)
        self.assertEqual(result2, result.geturl())

        # the result of geturl() is a fixpoint; we can always parse it
        # again to get the same result:
        result3 = urllib.parse.urlparse(result.geturl())
        self.assertEqual(result3.geturl(), result.geturl())
        self.assertEqual(result3,          result)
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
        result = urllib.parse.urlsplit(url)
        self.assertEqual(result, split)
        t = (result.scheme, result.netloc, result.path,
             result.query, result.fragment)
        self.assertEqual(t, split)
        result2 = urllib.parse.urlunsplit(result)
        self.assertEqual(result2, url)
        self.assertEqual(result2, result.geturl())

        # check the fixpoint property of re-parsing the result of geturl()
        result3 = urllib.parse.urlsplit(result.geturl())
        self.assertEqual(result3.geturl(), result.geturl())
        self.assertEqual(result3,          result)
        self.assertEqual(result3.scheme,   result.scheme)
        self.assertEqual(result3.netloc,   result.netloc)
        self.assertEqual(result3.path,     result.path)
        self.assertEqual(result3.query,    result.query)
        self.assertEqual(result3.fragment, result.fragment)
        self.assertEqual(result3.username, result.username)
        self.assertEqual(result3.password, result.password)
        self.assertEqual(result3.hostname, result.hostname)
        self.assertEqual(result3.port,     result.port)

    def test_qsl(self):
        for orig, expect in parse_qsl_test_cases:
            result = urllib.parse.parse_qsl(orig, keep_blank_values=True)
            self.assertEqual(result, expect, "Error parsing %r" % orig)
            expect_without_blanks = [v for v in expect if len(v[1])]
            result = urllib.parse.parse_qsl(orig, keep_blank_values=False)
            self.assertEqual(result, expect_without_blanks,
                            "Error parsing %r" % orig)

    def test_roundtrips(self):
        str_cases = [
            ('file:///tmp/junk.txt',
             ('file', '', '/tmp/junk.txt', '', '', ''),
             ('file', '', '/tmp/junk.txt', '', '')),
            ('imap://mail.python.org/mbox1',
             ('imap', 'mail.python.org', '/mbox1', '', '', ''),
             ('imap', 'mail.python.org', '/mbox1', '', '')),
            ('mms://wms.sys.hinet.net/cts/Drama/09006251100.asf',
             ('mms', 'wms.sys.hinet.net', '/cts/Drama/09006251100.asf',
              '', '', ''),
             ('mms', 'wms.sys.hinet.net', '/cts/Drama/09006251100.asf',
              '', '')),
            ('nfs://server/path/to/file.txt',
             ('nfs', 'server', '/path/to/file.txt', '', '', ''),
             ('nfs', 'server', '/path/to/file.txt', '', '')),
            ('svn+ssh://svn.zope.org/repos/main/ZConfig/trunk/',
             ('svn+ssh', 'svn.zope.org', '/repos/main/ZConfig/trunk/',
              '', '', ''),
             ('svn+ssh', 'svn.zope.org', '/repos/main/ZConfig/trunk/',
              '', '')),
            ('git+ssh://git@github.com/user/project.git',
            ('git+ssh', 'git@github.com','/user/project.git',
             '','',''),
            ('git+ssh', 'git@github.com','/user/project.git',
             '', '')),
            ]
        def _encode(t):
            return (t[0].encode('ascii'),
                    tuple(x.encode('ascii') for x in t[1]),
                    tuple(x.encode('ascii') for x in t[2]))
        bytes_cases = [_encode(x) for x in str_cases]
        for url, parsed, split in str_cases + bytes_cases:
            self.checkRoundtrips(url, parsed, split)

    def test_http_roundtrips(self):
        # urllib.parse.urlsplit treats 'http:' as an optimized special case,
        # so we test both 'http:' and 'https:' in all the following.
        # Three cheers for white box knowledge!
        str_cases = [
            ('://www.python.org',
             ('www.python.org', '', '', '', ''),
             ('www.python.org', '', '', '')),
            ('://www.python.org#abc',
             ('www.python.org', '', '', '', 'abc'),
             ('www.python.org', '', '', 'abc')),
            ('://www.python.org?q=abc',
             ('www.python.org', '', '', 'q=abc', ''),
             ('www.python.org', '', 'q=abc', '')),
            ('://www.python.org/#abc',
             ('www.python.org', '/', '', '', 'abc'),
             ('www.python.org', '/', '', 'abc')),
            ('://a/b/c/d;p?q#f',
             ('a', '/b/c/d', 'p', 'q', 'f'),
             ('a', '/b/c/d;p', 'q', 'f')),
            ]
        def _encode(t):
            return (t[0].encode('ascii'),
                    tuple(x.encode('ascii') for x in t[1]),
                    tuple(x.encode('ascii') for x in t[2]))
        bytes_cases = [_encode(x) for x in str_cases]
        str_schemes = ('http', 'https')
        bytes_schemes = (b'http', b'https')
        str_tests = str_schemes, str_cases
        bytes_tests = bytes_schemes, bytes_cases
        for schemes, test_cases in (str_tests, bytes_tests):
            for scheme in schemes:
                for url, parsed, split in test_cases:
                    url = scheme + url
                    parsed = (scheme,) + parsed
                    split = (scheme,) + split
                    self.checkRoundtrips(url, parsed, split)

    def checkJoin(self, base, relurl, expected):
        str_components = (base, relurl, expected)
        self.assertEqual(urllib.parse.urljoin(base, relurl), expected)
        bytes_components = baseb, relurlb, expectedb = [
                            x.encode('ascii') for x in str_components]
        self.assertEqual(urllib.parse.urljoin(baseb, relurlb), expectedb)

    def test_unparse_parse(self):
        str_cases = ['Python', './Python','x-newscheme://foo.com/stuff','x://y','x:/y','x:/','/',]
        bytes_cases = [x.encode('ascii') for x in str_cases]
        for u in str_cases + bytes_cases:
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
        self.checkJoin(RFC1808_BASE, '', 'http://a/b/c/d;p?q#f')
        self.checkJoin(RFC1808_BASE, '../../../g', 'http://a/../g')
        self.checkJoin(RFC1808_BASE, '../../../../g', 'http://a/../../g')
        self.checkJoin(RFC1808_BASE, '/./g', 'http://a/./g')
        self.checkJoin(RFC1808_BASE, '/../g', 'http://a/../g')
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

    def test_RFC2368(self):
        # Issue 11467: path that starts with a number is not parsed correctly
        self.assertEqual(urllib.parse.urlparse('mailto:1337@example.org'),
                ('mailto', '', '1337@example.org', '', '', ''))

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
        self.checkJoin(RFC2396_BASE, '../../../g', 'http://a/../g')
        self.checkJoin(RFC2396_BASE, '../../../../g', 'http://a/../../g')
        self.checkJoin(RFC2396_BASE, '/./g', 'http://a/./g')
        self.checkJoin(RFC2396_BASE, '/../g', 'http://a/../g')
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

    def test_RFC3986(self):
        # Test cases from RFC3986
        self.checkJoin(RFC3986_BASE, '?y','http://a/b/c/d;p?y')
        self.checkJoin(RFC2396_BASE, ';x', 'http://a/b/c/;x')
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

        #Abnormal Examples

        # The 'abnormal scenarios' are incompatible with RFC2986 parsing
        # Tests are here for reference.

        #self.checkJoin(RFC3986_BASE, '../../../g','http://a/g')
        #self.checkJoin(RFC3986_BASE, '../../../../g','http://a/g')
        #self.checkJoin(RFC3986_BASE, '/./g','http://a/g')
        #self.checkJoin(RFC3986_BASE, '/../g','http://a/g')

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
        self.checkJoin(SIMPLE_BASE, 'http:g','http://a/b/c/g')
        self.checkJoin(SIMPLE_BASE, 'http:','http://a/b/c/d')
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
        self.checkJoin(SIMPLE_BASE, '../../../g','http://a/../g')
        self.checkJoin(SIMPLE_BASE, './../g','http://a/b/g')
        self.checkJoin(SIMPLE_BASE, './g/.','http://a/b/c/g/')
        self.checkJoin(SIMPLE_BASE, '/./g','http://a/./g')
        self.checkJoin(SIMPLE_BASE, 'g/./h','http://a/b/c/g/h')
        self.checkJoin(SIMPLE_BASE, 'g/../h','http://a/b/c/h')
        self.checkJoin(SIMPLE_BASE, 'http:g','http://a/b/c/g')
        self.checkJoin(SIMPLE_BASE, 'http:','http://a/b/c/d')
        self.checkJoin(SIMPLE_BASE, 'http:?y','http://a/b/c/d?y')
        self.checkJoin(SIMPLE_BASE, 'http:g?y','http://a/b/c/g?y')
        self.checkJoin(SIMPLE_BASE, 'http:g?y/./x','http://a/b/c/g?y/./x')
        self.checkJoin('http:///', '..','http:///')
        self.checkJoin('', 'http://a/b/c/g?y/./x','http://a/b/c/g?y/./x')
        self.checkJoin('', 'http://a/./g', 'http://a/./g')
        self.checkJoin('svn://pathtorepo/dir1', 'dir2', 'svn://pathtorepo/dir2')
        self.checkJoin('svn+ssh://pathtorepo/dir1', 'dir2', 'svn+ssh://pathtorepo/dir2')

    def test_RFC2732(self):
        str_cases = [
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
            ]
        def _encode(t):
            return t[0].encode('ascii'), t[1].encode('ascii'), t[2]
        bytes_cases = [_encode(x) for x in str_cases]
        for url, hostname, port in str_cases + bytes_cases:
            urlparsed = urllib.parse.urlparse(url)
            self.assertEqual((urlparsed.hostname, urlparsed.port) , (hostname, port))

        str_cases = [
                'http://::12.34.56.78]/',
                'http://[::1/foo/',
                'ftp://[::1/foo/bad]/bad',
                'http://[::1/foo/bad]/bad',
                'http://[::ffff:12.34.56.78']
        bytes_cases = [x.encode('ascii') for x in str_cases]
        for invalid_url in str_cases + bytes_cases:
            self.assertRaises(ValueError, urllib.parse.urlparse, invalid_url)

    def test_urldefrag(self):
        str_cases = [
            ('http://python.org#frag', 'http://python.org', 'frag'),
            ('http://python.org', 'http://python.org', ''),
            ('http://python.org/#frag', 'http://python.org/', 'frag'),
            ('http://python.org/', 'http://python.org/', ''),
            ('http://python.org/?q#frag', 'http://python.org/?q', 'frag'),
            ('http://python.org/?q', 'http://python.org/?q', ''),
            ('http://python.org/p#frag', 'http://python.org/p', 'frag'),
            ('http://python.org/p?q', 'http://python.org/p?q', ''),
            (RFC1808_BASE, 'http://a/b/c/d;p?q', 'f'),
            (RFC2396_BASE, 'http://a/b/c/d;p?q', ''),
        ]
        def _encode(t):
            return type(t)(x.encode('ascii') for x in t)
        bytes_cases = [_encode(x) for x in str_cases]
        for url, defrag, frag in str_cases + bytes_cases:
            result = urllib.parse.urldefrag(url)
            self.assertEqual(result.geturl(), url)
            self.assertEqual(result, (defrag, frag))
            self.assertEqual(result.url, defrag)
            self.assertEqual(result.fragment, frag)

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

    def test_attributes_bad_port(self):
        """Check handling of non-integer ports."""
        p = urllib.parse.urlsplit("http://www.example.net:foo")
        self.assertEqual(p.netloc, "www.example.net:foo")
        self.assertRaises(ValueError, lambda: p.port)

        p = urllib.parse.urlparse("http://www.example.net:foo")
        self.assertEqual(p.netloc, "www.example.net:foo")
        self.assertRaises(ValueError, lambda: p.port)

        # Once again, repeat ourselves to test bytes
        p = urllib.parse.urlsplit(b"http://www.example.net:foo")
        self.assertEqual(p.netloc, b"www.example.net:foo")
        self.assertRaises(ValueError, lambda: p.port)

        p = urllib.parse.urlparse(b"http://www.example.net:foo")
        self.assertEqual(p.netloc, b"www.example.net:foo")
        self.assertRaises(ValueError, lambda: p.port)

    def test_attributes_without_netloc(self):
        # This example is straight from RFC 3261.  It looks like it
        # should allow the username, hostname, and port to be filled
        # in, but doesn't.  Since it's a URI and doesn't use the
        # scheme://netloc syntax, the netloc and related attributes
        # should be left empty.
        uri = "sip:alice@atlanta.com;maddr=239.255.255.1;ttl=15"
        p = urllib.parse.urlsplit(uri)
        self.assertEqual(p.netloc, "")
        self.assertEqual(p.username, None)
        self.assertEqual(p.password, None)
        self.assertEqual(p.hostname, None)
        self.assertEqual(p.port, None)
        self.assertEqual(p.geturl(), uri)

        p = urllib.parse.urlparse(uri)
        self.assertEqual(p.netloc, "")
        self.assertEqual(p.username, None)
        self.assertEqual(p.password, None)
        self.assertEqual(p.hostname, None)
        self.assertEqual(p.port, None)
        self.assertEqual(p.geturl(), uri)

        # You guessed it, repeating the test with bytes input
        uri = b"sip:alice@atlanta.com;maddr=239.255.255.1;ttl=15"
        p = urllib.parse.urlsplit(uri)
        self.assertEqual(p.netloc, b"")
        self.assertEqual(p.username, None)
        self.assertEqual(p.password, None)
        self.assertEqual(p.hostname, None)
        self.assertEqual(p.port, None)
        self.assertEqual(p.geturl(), uri)

        p = urllib.parse.urlparse(uri)
        self.assertEqual(p.netloc, b"")
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

    def test_withoutscheme(self):
        # Test urlparse without scheme
        # Issue 754016: urlparse goes wrong with IP:port without scheme
        # RFC 1808 specifies that netloc should start with //, urlparse expects
        # the same, otherwise it classifies the portion of url as path.
        self.assertEqual(urllib.parse.urlparse("path"),
                ('','','path','','',''))
        self.assertEqual(urllib.parse.urlparse("//www.python.org:80"),
                ('','www.python.org:80','','','',''))
        self.assertEqual(urllib.parse.urlparse("http://www.python.org:80"),
                ('http','www.python.org:80','','','',''))
        # Repeat for bytes input
        self.assertEqual(urllib.parse.urlparse(b"path"),
                (b'',b'',b'path',b'',b'',b''))
        self.assertEqual(urllib.parse.urlparse(b"//www.python.org:80"),
                (b'',b'www.python.org:80',b'',b'',b'',b''))
        self.assertEqual(urllib.parse.urlparse(b"http://www.python.org:80"),
                (b'http',b'www.python.org:80',b'',b'',b'',b''))

    def test_portseparator(self):
        # Issue 754016 makes changes for port separator ':' from scheme separator
        self.assertEqual(urllib.parse.urlparse("path:80"),
                ('','','path:80','','',''))
        self.assertEqual(urllib.parse.urlparse("http:"),('http','','','','',''))
        self.assertEqual(urllib.parse.urlparse("https:"),('https','','','','',''))
        self.assertEqual(urllib.parse.urlparse("http://www.python.org:80"),
                ('http','www.python.org:80','','','',''))
        # As usual, need to check bytes input as well
        self.assertEqual(urllib.parse.urlparse(b"path:80"),
                (b'',b'',b'path:80',b'',b'',b''))
        self.assertEqual(urllib.parse.urlparse(b"http:"),(b'http',b'',b'',b'',b'',b''))
        self.assertEqual(urllib.parse.urlparse(b"https:"),(b'https',b'',b'',b'',b'',b''))
        self.assertEqual(urllib.parse.urlparse(b"http://www.python.org:80"),
                (b'http',b'www.python.org:80',b'',b'',b'',b''))

    def test_usingsys(self):
        # Issue 3314: sys module is used in the error
        self.assertRaises(TypeError, urllib.parse.urlencode, "foo")

    def test_anyscheme(self):
        # Issue 7904: s3://foo.com/stuff has netloc "foo.com".
        self.assertEqual(urllib.parse.urlparse("s3://foo.com/stuff"),
                         ('s3', 'foo.com', '/stuff', '', '', ''))
        self.assertEqual(urllib.parse.urlparse("x-newscheme://foo.com/stuff"),
                         ('x-newscheme', 'foo.com', '/stuff', '', '', ''))
        # And for bytes...
        self.assertEqual(urllib.parse.urlparse(b"s3://foo.com/stuff"),
                         (b's3', b'foo.com', b'/stuff', b'', b'', b''))
        self.assertEqual(urllib.parse.urlparse(b"x-newscheme://foo.com/stuff"),
                         (b'x-newscheme', b'foo.com', b'/stuff', b'', b'', b''))

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

    def _check_result_type(self, str_type):
        num_args = len(str_type._fields)
        bytes_type = str_type._encoded_counterpart
        self.assertIs(bytes_type._decoded_counterpart, str_type)
        str_args = ('',) * num_args
        bytes_args = (b'',) * num_args
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

    def test_result_pairs(self):
        # Check encoding and decoding between result pairs
        result_types = [
          urllib.parse.DefragResult,
          urllib.parse.SplitResult,
          urllib.parse.ParseResult,
        ]
        for result_type in result_types:
            self._check_result_type(result_type)

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

    def test_splitnport(self):
        # Normal cases are exercised by other tests; ensure that we also
        # catch cases with no port specified. (testcase ensuring coverage)
        result = urllib.parse.splitnport('parrot:88')
        self.assertEqual(result, ('parrot', 88))
        result = urllib.parse.splitnport('parrot')
        self.assertEqual(result, ('parrot', -1))
        result = urllib.parse.splitnport('parrot', 55)
        self.assertEqual(result, ('parrot', 55))
        result = urllib.parse.splitnport('parrot:')
        self.assertEqual(result, ('parrot', None))

    def test_splitquery(self):
        # Normal cases are exercised by other tests; ensure that we also
        # catch cases with no port specified (testcase ensuring coverage)
        result = urllib.parse.splitquery('http://python.org/fake?foo=bar')
        self.assertEqual(result, ('http://python.org/fake', 'foo=bar'))
        result = urllib.parse.splitquery('http://python.org/fake?foo=bar?')
        self.assertEqual(result, ('http://python.org/fake?foo=bar', ''))
        result = urllib.parse.splitquery('http://python.org/fake')
        self.assertEqual(result, ('http://python.org/fake', None))

    def test_splitvalue(self):
        # Normal cases are exercised by other tests; test pathological cases
        # with no key/value pairs. (testcase ensuring coverage)
        result = urllib.parse.splitvalue('foo=bar')
        self.assertEqual(result, ('foo', 'bar'))
        result = urllib.parse.splitvalue('foo=')
        self.assertEqual(result, ('foo', ''))
        result = urllib.parse.splitvalue('foobar')
        self.assertEqual(result, ('foobar', None))

    def test_to_bytes(self):
        result = urllib.parse.to_bytes('http://www.python.org')
        self.assertEqual(result, 'http://www.python.org')
        self.assertRaises(UnicodeError, urllib.parse.to_bytes,
                          'http://www.python.org/medi\u00e6val')

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

    def test_quote_from_bytes(self):
        self.assertRaises(TypeError, urllib.parse.quote_from_bytes, 'foo')
        result = urllib.parse.quote_from_bytes(b'archaeological arcana')
        self.assertEqual(result, 'archaeological%20arcana')
        result = urllib.parse.quote_from_bytes(b'')
        self.assertEqual(result, '')

    def test_unquote_to_bytes(self):
        result = urllib.parse.unquote_to_bytes('abc%20def')
        self.assertEqual(result, b'abc def')
        result = urllib.parse.unquote_to_bytes('')
        self.assertEqual(result, b'')

    def test_quote_errors(self):
        self.assertRaises(TypeError, urllib.parse.quote, b'foo',
                          encoding='utf-8')
        self.assertRaises(TypeError, urllib.parse.quote, b'foo', errors='strict')


def test_main():
    support.run_unittest(UrlParseTestCase)

if __name__ == "__main__":
    test_main()
