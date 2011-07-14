"""Tests for http/cookiejar.py."""

import os
import re
import test.support
import time
import unittest
import urllib.request

from http.cookiejar import (time2isoz, http2time, time2netscape,
     parse_ns_headers, join_header_words, split_header_words, Cookie,
     CookieJar, DefaultCookiePolicy, LWPCookieJar, MozillaCookieJar,
     LoadError, lwp_cookie_str, DEFAULT_HTTP_PORT, escape_path,
     reach, is_HDN, domain_match, user_domain_match, request_path,
     request_port, request_host)


class DateTimeTests(unittest.TestCase):

    def test_time2isoz(self):
        base = 1019227000
        day = 24*3600
        self.assertEqual(time2isoz(base), "2002-04-19 14:36:40Z")
        self.assertEqual(time2isoz(base+day), "2002-04-20 14:36:40Z")
        self.assertEqual(time2isoz(base+2*day), "2002-04-21 14:36:40Z")
        self.assertEqual(time2isoz(base+3*day), "2002-04-22 14:36:40Z")

        az = time2isoz()
        bz = time2isoz(500000)
        for text in (az, bz):
            self.assertTrue(re.search(r"^\d{4}-\d\d-\d\d \d\d:\d\d:\d\dZ$", text),
                            "bad time2isoz format: %s %s" % (az, bz))

    def test_http2time(self):
        def parse_date(text):
            return time.gmtime(http2time(text))[:6]

        self.assertEqual(parse_date("01 Jan 2001"), (2001, 1, 1, 0, 0, 0.0))

        # this test will break around year 2070
        self.assertEqual(parse_date("03-Feb-20"), (2020, 2, 3, 0, 0, 0.0))

        # this test will break around year 2048
        self.assertEqual(parse_date("03-Feb-98"), (1998, 2, 3, 0, 0, 0.0))

    def test_http2time_formats(self):
        # test http2time for supported dates.  Test cases with 2 digit year
        # will probably break in year 2044.
        tests = [
         'Thu, 03 Feb 1994 00:00:00 GMT',  # proposed new HTTP format
         'Thursday, 03-Feb-94 00:00:00 GMT',  # old rfc850 HTTP format
         'Thursday, 03-Feb-1994 00:00:00 GMT',  # broken rfc850 HTTP format

         '03 Feb 1994 00:00:00 GMT',  # HTTP format (no weekday)
         '03-Feb-94 00:00:00 GMT',  # old rfc850 (no weekday)
         '03-Feb-1994 00:00:00 GMT',  # broken rfc850 (no weekday)
         '03-Feb-1994 00:00 GMT',  # broken rfc850 (no weekday, no seconds)
         '03-Feb-1994 00:00',  # broken rfc850 (no weekday, no seconds, no tz)

         '03-Feb-94',  # old rfc850 HTTP format (no weekday, no time)
         '03-Feb-1994',  # broken rfc850 HTTP format (no weekday, no time)
         '03 Feb 1994',  # proposed new HTTP format (no weekday, no time)

         # A few tests with extra space at various places
         '  03   Feb   1994  0:00  ',
         '  03-Feb-1994  ',
        ]

        test_t = 760233600  # assume broken POSIX counting of seconds
        result = time2isoz(test_t)
        expected = "1994-02-03 00:00:00Z"
        self.assertEqual(result, expected,
                         "%s  =>  '%s' (%s)" % (test_t, result, expected))

        for s in tests:
            t = http2time(s)
            t2 = http2time(s.lower())
            t3 = http2time(s.upper())

            self.assertTrue(t == t2 == t3 == test_t,
                         "'%s'  =>  %s, %s, %s (%s)" % (s, t, t2, t3, test_t))

    def test_http2time_garbage(self):
        for test in [
            '',
            'Garbage',
            'Mandag 16. September 1996',
            '01-00-1980',
            '01-13-1980',
            '00-01-1980',
            '32-01-1980',
            '01-01-1980 25:00:00',
            '01-01-1980 00:61:00',
            '01-01-1980 00:00:62',
            ]:
            self.assertTrue(http2time(test) is None,
                         "http2time(%s) is not None\n"
                         "http2time(test) %s" % (test, http2time(test))
                         )


class HeaderTests(unittest.TestCase):

    def test_parse_ns_headers(self):
        # quotes should be stripped
        expected = [[('foo', 'bar'), ('expires', 2209069412), ('version', '0')]]
        for hdr in [
            'foo=bar; expires=01 Jan 2040 22:23:32 GMT',
            'foo=bar; expires="01 Jan 2040 22:23:32 GMT"',
            ]:
            self.assertEqual(parse_ns_headers([hdr]), expected)

    def test_parse_ns_headers_version(self):

        # quotes should be stripped
        expected = [[('foo', 'bar'), ('version', '1')]]
        for hdr in [
            'foo=bar; version="1"',
            'foo=bar; Version="1"',
            ]:
            self.assertEqual(parse_ns_headers([hdr]), expected)

    def test_parse_ns_headers_special_names(self):
        # names such as 'expires' are not special in first name=value pair
        # of Set-Cookie: header
        # Cookie with name 'expires'
        hdr = 'expires=01 Jan 2040 22:23:32 GMT'
        expected = [[("expires", "01 Jan 2040 22:23:32 GMT"), ("version", "0")]]
        self.assertEqual(parse_ns_headers([hdr]), expected)

    def test_join_header_words(self):
        joined = join_header_words([[("foo", None), ("bar", "baz")]])
        self.assertEqual(joined, "foo; bar=baz")

        self.assertEqual(join_header_words([[]]), "")

    def test_split_header_words(self):
        tests = [
            ("foo", [[("foo", None)]]),
            ("foo=bar", [[("foo", "bar")]]),
            ("   foo   ", [[("foo", None)]]),
            ("   foo=   ", [[("foo", "")]]),
            ("   foo=", [[("foo", "")]]),
            ("   foo=   ; ", [[("foo", "")]]),
            ("   foo=   ; bar= baz ", [[("foo", ""), ("bar", "baz")]]),
            ("foo=bar bar=baz", [[("foo", "bar"), ("bar", "baz")]]),
            # doesn't really matter if this next fails, but it works ATM
            ("foo= bar=baz", [[("foo", "bar=baz")]]),
            ("foo=bar;bar=baz", [[("foo", "bar"), ("bar", "baz")]]),
            ('foo bar baz', [[("foo", None), ("bar", None), ("baz", None)]]),
            ("a, b, c", [[("a", None)], [("b", None)], [("c", None)]]),
            (r'foo; bar=baz, spam=, foo="\,\;\"", bar= ',
             [[("foo", None), ("bar", "baz")],
              [("spam", "")], [("foo", ',;"')], [("bar", "")]]),
            ]

        for arg, expect in tests:
            try:
                result = split_header_words([arg])
            except:
                import traceback, io
                f = io.StringIO()
                traceback.print_exc(None, f)
                result = "(error -- traceback follows)\n\n%s" % f.getvalue()
            self.assertEqual(result,  expect, """
When parsing: '%s'
Expected:     '%s'
Got:          '%s'
""" % (arg, expect, result))

    def test_roundtrip(self):
        tests = [
            ("foo", "foo"),
            ("foo=bar", "foo=bar"),
            ("   foo   ", "foo"),
            ("foo=", 'foo=""'),
            ("foo=bar bar=baz", "foo=bar; bar=baz"),
            ("foo=bar;bar=baz", "foo=bar; bar=baz"),
            ('foo bar baz', "foo; bar; baz"),
            (r'foo="\"" bar="\\"', r'foo="\""; bar="\\"'),
            ('foo,,,bar', 'foo, bar'),
            ('foo=bar,bar=baz', 'foo=bar, bar=baz'),

            ('text/html; charset=iso-8859-1',
             'text/html; charset="iso-8859-1"'),

            ('foo="bar"; port="80,81"; discard, bar=baz',
             'foo=bar; port="80,81"; discard, bar=baz'),

            (r'Basic realm="\"foo\\\\bar\""',
             r'Basic; realm="\"foo\\\\bar\""')
            ]

        for arg, expect in tests:
            input = split_header_words([arg])
            res = join_header_words(input)
            self.assertEqual(res, expect, """
When parsing: '%s'
Expected:     '%s'
Got:          '%s'
Input was:    '%s'
""" % (arg, expect, res, input))


class FakeResponse:
    def __init__(self, headers=[], url=None):
        """
        headers: list of RFC822-style 'Key: value' strings
        """
        import email
        self._headers = email.message_from_string("\n".join(headers))
        self._url = url
    def info(self): return self._headers

def interact_2965(cookiejar, url, *set_cookie_hdrs):
    return _interact(cookiejar, url, set_cookie_hdrs, "Set-Cookie2")

def interact_netscape(cookiejar, url, *set_cookie_hdrs):
    return _interact(cookiejar, url, set_cookie_hdrs, "Set-Cookie")

def _interact(cookiejar, url, set_cookie_hdrs, hdr_name):
    """Perform a single request / response cycle, returning Cookie: header."""
    req = urllib.request.Request(url)
    cookiejar.add_cookie_header(req)
    cookie_hdr = req.get_header("Cookie", "")
    headers = []
    for hdr in set_cookie_hdrs:
        headers.append("%s: %s" % (hdr_name, hdr))
    res = FakeResponse(headers, url)
    cookiejar.extract_cookies(res, req)
    return cookie_hdr


class FileCookieJarTests(unittest.TestCase):
    def test_lwp_valueless_cookie(self):
        # cookies with no value should be saved and loaded consistently
        filename = test.support.TESTFN
        c = LWPCookieJar()
        interact_netscape(c, "http://www.acme.com/", 'boo')
        self.assertEqual(c._cookies["www.acme.com"]["/"]["boo"].value, None)
        try:
            c.save(filename, ignore_discard=True)
            c = LWPCookieJar()
            c.load(filename, ignore_discard=True)
        finally:
            try: os.unlink(filename)
            except OSError: pass
        self.assertEqual(c._cookies["www.acme.com"]["/"]["boo"].value, None)

    def test_bad_magic(self):
        # IOErrors (eg. file doesn't exist) are allowed to propagate
        filename = test.support.TESTFN
        for cookiejar_class in LWPCookieJar, MozillaCookieJar:
            c = cookiejar_class()
            try:
                c.load(filename="for this test to work, a file with this "
                                "filename should not exist")
            except IOError as exc:
                # exactly IOError, not LoadError
                self.assertIs(exc.__class__, IOError)
            else:
                self.fail("expected IOError for invalid filename")
        # Invalid contents of cookies file (eg. bad magic string)
        # causes a LoadError.
        try:
            with open(filename, "w") as f:
                f.write("oops\n")
                for cookiejar_class in LWPCookieJar, MozillaCookieJar:
                    c = cookiejar_class()
                    self.assertRaises(LoadError, c.load, filename)
        finally:
            try: os.unlink(filename)
            except OSError: pass

class CookieTests(unittest.TestCase):
    # XXX
    # Get rid of string comparisons where not actually testing str / repr.
    # .clear() etc.
    # IP addresses like 50 (single number, no dot) and domain-matching
    #  functions (and is_HDN)?  See draft RFC 2965 errata.
    # Strictness switches
    # is_third_party()
    # unverifiability / third-party blocking
    # Netscape cookies work the same as RFC 2965 with regard to port.
    # Set-Cookie with negative max age.
    # If turn RFC 2965 handling off, Set-Cookie2 cookies should not clobber
    #  Set-Cookie cookies.
    # Cookie2 should be sent if *any* cookies are not V1 (ie. V0 OR V2 etc.).
    # Cookies (V1 and V0) with no expiry date should be set to be discarded.
    # RFC 2965 Quoting:
    #  Should accept unquoted cookie-attribute values?  check errata draft.
    #   Which are required on the way in and out?
    #  Should always return quoted cookie-attribute values?
    # Proper testing of when RFC 2965 clobbers Netscape (waiting for errata).
    # Path-match on return (same for V0 and V1).
    # RFC 2965 acceptance and returning rules
    #  Set-Cookie2 without version attribute is rejected.

    # Netscape peculiarities list from Ronald Tschalar.
    # The first two still need tests, the rest are covered.
## - Quoting: only quotes around the expires value are recognized as such
##   (and yes, some folks quote the expires value); quotes around any other
##   value are treated as part of the value.
## - White space: white space around names and values is ignored
## - Default path: if no path parameter is given, the path defaults to the
##   path in the request-uri up to, but not including, the last '/'. Note
##   that this is entirely different from what the spec says.
## - Commas and other delimiters: Netscape just parses until the next ';'.
##   This means it will allow commas etc inside values (and yes, both
##   commas and equals are commonly appear in the cookie value). This also
##   means that if you fold multiple Set-Cookie header fields into one,
##   comma-separated list, it'll be a headache to parse (at least my head
##   starts hurting everytime I think of that code).
## - Expires: You'll get all sorts of date formats in the expires,
##   including emtpy expires attributes ("expires="). Be as flexible as you
##   can, and certainly don't expect the weekday to be there; if you can't
##   parse it, just ignore it and pretend it's a session cookie.
## - Domain-matching: Netscape uses the 2-dot rule for _all_ domains, not
##   just the 7 special TLD's listed in their spec. And folks rely on
##   that...

    def test_domain_return_ok(self):
        # test optimization: .domain_return_ok() should filter out most
        # domains in the CookieJar before we try to access them (because that
        # may require disk access -- in particular, with MSIECookieJar)
        # This is only a rough check for performance reasons, so it's not too
        # critical as long as it's sufficiently liberal.
        pol = DefaultCookiePolicy()
        for url, domain, ok in [
            ("http://foo.bar.com/", "blah.com", False),
            ("http://foo.bar.com/", "rhubarb.blah.com", False),
            ("http://foo.bar.com/", "rhubarb.foo.bar.com", False),
            ("http://foo.bar.com/", ".foo.bar.com", True),
            ("http://foo.bar.com/", "foo.bar.com", True),
            ("http://foo.bar.com/", ".bar.com", True),
            ("http://foo.bar.com/", "com", True),
            ("http://foo.com/", "rhubarb.foo.com", False),
            ("http://foo.com/", ".foo.com", True),
            ("http://foo.com/", "foo.com", True),
            ("http://foo.com/", "com", True),
            ("http://foo/", "rhubarb.foo", False),
            ("http://foo/", ".foo", True),
            ("http://foo/", "foo", True),
            ("http://foo/", "foo.local", True),
            ("http://foo/", ".local", True),
            ]:
            request = urllib.request.Request(url)
            r = pol.domain_return_ok(domain, request)
            if ok: self.assertTrue(r)
            else: self.assertTrue(not r)

    def test_missing_value(self):
        # missing = sign in Cookie: header is regarded by Mozilla as a missing
        # name, and by http.cookiejar as a missing value
        filename = test.support.TESTFN
        c = MozillaCookieJar(filename)
        interact_netscape(c, "http://www.acme.com/", 'eggs')
        interact_netscape(c, "http://www.acme.com/", '"spam"; path=/foo/')
        cookie = c._cookies["www.acme.com"]["/"]["eggs"]
        self.assertTrue(cookie.value is None)
        self.assertEqual(cookie.name, "eggs")
        cookie = c._cookies["www.acme.com"]['/foo/']['"spam"']
        self.assertTrue(cookie.value is None)
        self.assertEqual(cookie.name, '"spam"')
        self.assertEqual(lwp_cookie_str(cookie), (
            r'"spam"; path="/foo/"; domain="www.acme.com"; '
            'path_spec; discard; version=0'))
        old_str = repr(c)
        c.save(ignore_expires=True, ignore_discard=True)
        try:
            c = MozillaCookieJar(filename)
            c.revert(ignore_expires=True, ignore_discard=True)
        finally:
            os.unlink(c.filename)
        # cookies unchanged apart from lost info re. whether path was specified
        self.assertEqual(
            repr(c),
            re.sub("path_specified=%s" % True, "path_specified=%s" % False,
                   old_str)
            )
        self.assertEqual(interact_netscape(c, "http://www.acme.com/foo/"),
                         '"spam"; eggs')

    def test_rfc2109_handling(self):
        # RFC 2109 cookies are handled as RFC 2965 or Netscape cookies,
        # dependent on policy settings
        for rfc2109_as_netscape, rfc2965, version in [
            # default according to rfc2965 if not explicitly specified
            (None, False, 0),
            (None, True, 1),
            # explicit rfc2109_as_netscape
            (False, False, None),  # version None here means no cookie stored
            (False, True, 1),
            (True, False, 0),
            (True, True, 0),
            ]:
            policy = DefaultCookiePolicy(
                rfc2109_as_netscape=rfc2109_as_netscape,
                rfc2965=rfc2965)
            c = CookieJar(policy)
            interact_netscape(c, "http://www.example.com/", "ni=ni; Version=1")
            try:
                cookie = c._cookies["www.example.com"]["/"]["ni"]
            except KeyError:
                self.assertTrue(version is None)  # didn't expect a stored cookie
            else:
                self.assertEqual(cookie.version, version)
                # 2965 cookies are unaffected
                interact_2965(c, "http://www.example.com/",
                              "foo=bar; Version=1")
                if rfc2965:
                    cookie2965 = c._cookies["www.example.com"]["/"]["foo"]
                    self.assertEqual(cookie2965.version, 1)

    def test_ns_parser(self):
        c = CookieJar()
        interact_netscape(c, "http://www.acme.com/",
                          'spam=eggs; DoMain=.acme.com; port; blArgh="feep"')
        interact_netscape(c, "http://www.acme.com/", 'ni=ni; port=80,8080')
        interact_netscape(c, "http://www.acme.com:80/", 'nini=ni')
        interact_netscape(c, "http://www.acme.com:80/", 'foo=bar; expires=')
        interact_netscape(c, "http://www.acme.com:80/", 'spam=eggs; '
                          'expires="Foo Bar 25 33:22:11 3022"')

        cookie = c._cookies[".acme.com"]["/"]["spam"]
        self.assertEqual(cookie.domain, ".acme.com")
        self.assertTrue(cookie.domain_specified)
        self.assertEqual(cookie.port, DEFAULT_HTTP_PORT)
        self.assertTrue(not cookie.port_specified)
        # case is preserved
        self.assertTrue(cookie.has_nonstandard_attr("blArgh") and
                     not cookie.has_nonstandard_attr("blargh"))

        cookie = c._cookies["www.acme.com"]["/"]["ni"]
        self.assertEqual(cookie.domain, "www.acme.com")
        self.assertTrue(not cookie.domain_specified)
        self.assertEqual(cookie.port, "80,8080")
        self.assertTrue(cookie.port_specified)

        cookie = c._cookies["www.acme.com"]["/"]["nini"]
        self.assertTrue(cookie.port is None)
        self.assertTrue(not cookie.port_specified)

        # invalid expires should not cause cookie to be dropped
        foo = c._cookies["www.acme.com"]["/"]["foo"]
        spam = c._cookies["www.acme.com"]["/"]["foo"]
        self.assertTrue(foo.expires is None)
        self.assertTrue(spam.expires is None)

    def test_ns_parser_special_names(self):
        # names such as 'expires' are not special in first name=value pair
        # of Set-Cookie: header
        c = CookieJar()
        interact_netscape(c, "http://www.acme.com/", 'expires=eggs')
        interact_netscape(c, "http://www.acme.com/", 'version=eggs; spam=eggs')

        cookies = c._cookies["www.acme.com"]["/"]
        self.assertIn('expires', cookies)
        self.assertIn('version', cookies)

    def test_expires(self):
        # if expires is in future, keep cookie...
        c = CookieJar()
        future = time2netscape(time.time()+3600)
        interact_netscape(c, "http://www.acme.com/", 'spam="bar"; expires=%s' %
                          future)
        self.assertEqual(len(c), 1)
        now = time2netscape(time.time()-1)
        # ... and if in past or present, discard it
        interact_netscape(c, "http://www.acme.com/", 'foo="eggs"; expires=%s' %
                          now)
        h = interact_netscape(c, "http://www.acme.com/")
        self.assertEqual(len(c), 1)
        self.assertIn('spam="bar"', h)
        self.assertNotIn("foo", h)

        # max-age takes precedence over expires, and zero max-age is request to
        # delete both new cookie and any old matching cookie
        interact_netscape(c, "http://www.acme.com/", 'eggs="bar"; expires=%s' %
                          future)
        interact_netscape(c, "http://www.acme.com/", 'bar="bar"; expires=%s' %
                          future)
        self.assertEqual(len(c), 3)
        interact_netscape(c, "http://www.acme.com/", 'eggs="bar"; '
                          'expires=%s; max-age=0' % future)
        interact_netscape(c, "http://www.acme.com/", 'bar="bar"; '
                          'max-age=0; expires=%s' % future)
        h = interact_netscape(c, "http://www.acme.com/")
        self.assertEqual(len(c), 1)

        # test expiry at end of session for cookies with no expires attribute
        interact_netscape(c, "http://www.rhubarb.net/", 'whum="fizz"')
        self.assertEqual(len(c), 2)
        c.clear_session_cookies()
        self.assertEqual(len(c), 1)
        self.assertIn('spam="bar"', h)

        # XXX RFC 2965 expiry rules (some apply to V0 too)

    def test_default_path(self):
        # RFC 2965
        pol = DefaultCookiePolicy(rfc2965=True)

        c = CookieJar(pol)
        interact_2965(c, "http://www.acme.com/", 'spam="bar"; Version="1"')
        self.assertIn("/", c._cookies["www.acme.com"])

        c = CookieJar(pol)
        interact_2965(c, "http://www.acme.com/blah", 'eggs="bar"; Version="1"')
        self.assertIn("/", c._cookies["www.acme.com"])

        c = CookieJar(pol)
        interact_2965(c, "http://www.acme.com/blah/rhubarb",
                      'eggs="bar"; Version="1"')
        self.assertIn("/blah/", c._cookies["www.acme.com"])

        c = CookieJar(pol)
        interact_2965(c, "http://www.acme.com/blah/rhubarb/",
                      'eggs="bar"; Version="1"')
        self.assertIn("/blah/rhubarb/", c._cookies["www.acme.com"])

        # Netscape

        c = CookieJar()
        interact_netscape(c, "http://www.acme.com/", 'spam="bar"')
        self.assertIn("/", c._cookies["www.acme.com"])

        c = CookieJar()
        interact_netscape(c, "http://www.acme.com/blah", 'eggs="bar"')
        self.assertIn("/", c._cookies["www.acme.com"])

        c = CookieJar()
        interact_netscape(c, "http://www.acme.com/blah/rhubarb", 'eggs="bar"')
        self.assertIn("/blah", c._cookies["www.acme.com"])

        c = CookieJar()
        interact_netscape(c, "http://www.acme.com/blah/rhubarb/", 'eggs="bar"')
        self.assertIn("/blah/rhubarb", c._cookies["www.acme.com"])

    def test_default_path_with_query(self):
        cj = CookieJar()
        uri = "http://example.com/?spam/eggs"
        value = 'eggs="bar"'
        interact_netscape(cj, uri, value)
        # Default path does not include query, so is "/", not "/?spam".
        self.assertIn("/", cj._cookies["example.com"])
        # Cookie is sent back to the same URI.
        self.assertEqual(interact_netscape(cj, uri), value)

    def test_escape_path(self):
        cases = [
            # quoted safe
            ("/foo%2f/bar", "/foo%2F/bar"),
            ("/foo%2F/bar", "/foo%2F/bar"),
            # quoted %
            ("/foo%%/bar", "/foo%%/bar"),
            # quoted unsafe
            ("/fo%19o/bar", "/fo%19o/bar"),
            ("/fo%7do/bar", "/fo%7Do/bar"),
            # unquoted safe
            ("/foo/bar&", "/foo/bar&"),
            ("/foo//bar", "/foo//bar"),
            ("\176/foo/bar", "\176/foo/bar"),
            # unquoted unsafe
            ("/foo\031/bar", "/foo%19/bar"),
            ("/\175foo/bar", "/%7Dfoo/bar"),
            # unicode, latin-1 range
            ("/foo/bar\u00fc", "/foo/bar%C3%BC"),     # UTF-8 encoded
            # unicode
            ("/foo/bar\uabcd", "/foo/bar%EA%AF%8D"),  # UTF-8 encoded
            ]
        for arg, result in cases:
            self.assertEqual(escape_path(arg), result)

    def test_request_path(self):
        # with parameters
        req = urllib.request.Request(
            "http://www.example.com/rheum/rhaponticum;"
            "foo=bar;sing=song?apples=pears&spam=eggs#ni")
        self.assertEqual(request_path(req),
                         "/rheum/rhaponticum;foo=bar;sing=song")
        # without parameters
        req = urllib.request.Request(
            "http://www.example.com/rheum/rhaponticum?"
            "apples=pears&spam=eggs#ni")
        self.assertEqual(request_path(req), "/rheum/rhaponticum")
        # missing final slash
        req = urllib.request.Request("http://www.example.com")
        self.assertEqual(request_path(req), "/")

    def test_request_port(self):
        req = urllib.request.Request("http://www.acme.com:1234/",
                                     headers={"Host": "www.acme.com:4321"})
        self.assertEqual(request_port(req), "1234")
        req = urllib.request.Request("http://www.acme.com/",
                                     headers={"Host": "www.acme.com:4321"})
        self.assertEqual(request_port(req), DEFAULT_HTTP_PORT)

    def test_request_host(self):
        # this request is illegal (RFC2616, 14.2.3)
        req = urllib.request.Request("http://1.1.1.1/",
                                     headers={"Host": "www.acme.com:80"})
        # libwww-perl wants this response, but that seems wrong (RFC 2616,
        # section 5.2, point 1., and RFC 2965 section 1, paragraph 3)
        #self.assertEqual(request_host(req), "www.acme.com")
        self.assertEqual(request_host(req), "1.1.1.1")
        req = urllib.request.Request("http://www.acme.com/",
                                     headers={"Host": "irrelevant.com"})
        self.assertEqual(request_host(req), "www.acme.com")
        # port shouldn't be in request-host
        req = urllib.request.Request("http://www.acme.com:2345/resource.html",
                                     headers={"Host": "www.acme.com:5432"})
        self.assertEqual(request_host(req), "www.acme.com")

    def test_is_HDN(self):
        self.assertTrue(is_HDN("foo.bar.com"))
        self.assertTrue(is_HDN("1foo2.3bar4.5com"))
        self.assertTrue(not is_HDN("192.168.1.1"))
        self.assertTrue(not is_HDN(""))
        self.assertTrue(not is_HDN("."))
        self.assertTrue(not is_HDN(".foo.bar.com"))
        self.assertTrue(not is_HDN("..foo"))
        self.assertTrue(not is_HDN("foo."))

    def test_reach(self):
        self.assertEqual(reach("www.acme.com"), ".acme.com")
        self.assertEqual(reach("acme.com"), "acme.com")
        self.assertEqual(reach("acme.local"), ".local")
        self.assertEqual(reach(".local"), ".local")
        self.assertEqual(reach(".com"), ".com")
        self.assertEqual(reach("."), ".")
        self.assertEqual(reach(""), "")
        self.assertEqual(reach("192.168.0.1"), "192.168.0.1")

    def test_domain_match(self):
        self.assertTrue(domain_match("192.168.1.1", "192.168.1.1"))
        self.assertTrue(not domain_match("192.168.1.1", ".168.1.1"))
        self.assertTrue(domain_match("x.y.com", "x.Y.com"))
        self.assertTrue(domain_match("x.y.com", ".Y.com"))
        self.assertTrue(not domain_match("x.y.com", "Y.com"))
        self.assertTrue(domain_match("a.b.c.com", ".c.com"))
        self.assertTrue(not domain_match(".c.com", "a.b.c.com"))
        self.assertTrue(domain_match("example.local", ".local"))
        self.assertTrue(not domain_match("blah.blah", ""))
        self.assertTrue(not domain_match("", ".rhubarb.rhubarb"))
        self.assertTrue(domain_match("", ""))

        self.assertTrue(user_domain_match("acme.com", "acme.com"))
        self.assertTrue(not user_domain_match("acme.com", ".acme.com"))
        self.assertTrue(user_domain_match("rhubarb.acme.com", ".acme.com"))
        self.assertTrue(user_domain_match("www.rhubarb.acme.com", ".acme.com"))
        self.assertTrue(user_domain_match("x.y.com", "x.Y.com"))
        self.assertTrue(user_domain_match("x.y.com", ".Y.com"))
        self.assertTrue(not user_domain_match("x.y.com", "Y.com"))
        self.assertTrue(user_domain_match("y.com", "Y.com"))
        self.assertTrue(not user_domain_match(".y.com", "Y.com"))
        self.assertTrue(user_domain_match(".y.com", ".Y.com"))
        self.assertTrue(user_domain_match("x.y.com", ".com"))
        self.assertTrue(not user_domain_match("x.y.com", "com"))
        self.assertTrue(not user_domain_match("x.y.com", "m"))
        self.assertTrue(not user_domain_match("x.y.com", ".m"))
        self.assertTrue(not user_domain_match("x.y.com", ""))
        self.assertTrue(not user_domain_match("x.y.com", "."))
        self.assertTrue(user_domain_match("192.168.1.1", "192.168.1.1"))
        # not both HDNs, so must string-compare equal to match
        self.assertTrue(not user_domain_match("192.168.1.1", ".168.1.1"))
        self.assertTrue(not user_domain_match("192.168.1.1", "."))
        # empty string is a special case
        self.assertTrue(not user_domain_match("192.168.1.1", ""))

    def test_wrong_domain(self):
        # Cookies whose effective request-host name does not domain-match the
        # domain are rejected.

        # XXX far from complete
        c = CookieJar()
        interact_2965(c, "http://www.nasty.com/",
                      'foo=bar; domain=friendly.org; Version="1"')
        self.assertEqual(len(c), 0)

    def test_strict_domain(self):
        # Cookies whose domain is a country-code tld like .co.uk should
        # not be set if CookiePolicy.strict_domain is true.
        cp = DefaultCookiePolicy(strict_domain=True)
        cj = CookieJar(policy=cp)
        interact_netscape(cj, "http://example.co.uk/", 'no=problemo')
        interact_netscape(cj, "http://example.co.uk/",
                          'okey=dokey; Domain=.example.co.uk')
        self.assertEqual(len(cj), 2)
        for pseudo_tld in [".co.uk", ".org.za", ".tx.us", ".name.us"]:
            interact_netscape(cj, "http://example.%s/" % pseudo_tld,
                              'spam=eggs; Domain=.co.uk')
            self.assertEqual(len(cj), 2)

    def test_two_component_domain_ns(self):
        # Netscape: .www.bar.com, www.bar.com, .bar.com, bar.com, no domain
        # should all get accepted, as should .acme.com, acme.com and no domain
        # for 2-component domains like acme.com.
        c = CookieJar()

        # two-component V0 domain is OK
        interact_netscape(c, "http://foo.net/", 'ns=bar')
        self.assertEqual(len(c), 1)
        self.assertEqual(c._cookies["foo.net"]["/"]["ns"].value, "bar")
        self.assertEqual(interact_netscape(c, "http://foo.net/"), "ns=bar")
        # *will* be returned to any other domain (unlike RFC 2965)...
        self.assertEqual(interact_netscape(c, "http://www.foo.net/"),
                         "ns=bar")
        # ...unless requested otherwise
        pol = DefaultCookiePolicy(
            strict_ns_domain=DefaultCookiePolicy.DomainStrictNonDomain)
        c.set_policy(pol)
        self.assertEqual(interact_netscape(c, "http://www.foo.net/"), "")

        # unlike RFC 2965, even explicit two-component domain is OK,
        # because .foo.net matches foo.net
        interact_netscape(c, "http://foo.net/foo/",
                          'spam1=eggs; domain=foo.net')
        # even if starts with a dot -- in NS rules, .foo.net matches foo.net!
        interact_netscape(c, "http://foo.net/foo/bar/",
                          'spam2=eggs; domain=.foo.net')
        self.assertEqual(len(c), 3)
        self.assertEqual(c._cookies[".foo.net"]["/foo"]["spam1"].value,
                         "eggs")
        self.assertEqual(c._cookies[".foo.net"]["/foo/bar"]["spam2"].value,
                         "eggs")
        self.assertEqual(interact_netscape(c, "http://foo.net/foo/bar/"),
                         "spam2=eggs; spam1=eggs; ns=bar")

        # top-level domain is too general
        interact_netscape(c, "http://foo.net/", 'nini="ni"; domain=.net')
        self.assertEqual(len(c), 3)

##         # Netscape protocol doesn't allow non-special top level domains (such
##         # as co.uk) in the domain attribute unless there are at least three
##         # dots in it.
        # Oh yes it does!  Real implementations don't check this, and real
        # cookies (of course) rely on that behaviour.
        interact_netscape(c, "http://foo.co.uk", 'nasty=trick; domain=.co.uk')
##         self.assertEqual(len(c), 2)
        self.assertEqual(len(c), 4)

    def test_two_component_domain_rfc2965(self):
        pol = DefaultCookiePolicy(rfc2965=True)
        c = CookieJar(pol)

        # two-component V1 domain is OK
        interact_2965(c, "http://foo.net/", 'foo=bar; Version="1"')
        self.assertEqual(len(c), 1)
        self.assertEqual(c._cookies["foo.net"]["/"]["foo"].value, "bar")
        self.assertEqual(interact_2965(c, "http://foo.net/"),
                         "$Version=1; foo=bar")
        # won't be returned to any other domain (because domain was implied)
        self.assertEqual(interact_2965(c, "http://www.foo.net/"), "")

        # unless domain is given explicitly, because then it must be
        # rewritten to start with a dot: foo.net --> .foo.net, which does
        # not domain-match foo.net
        interact_2965(c, "http://foo.net/foo",
                      'spam=eggs; domain=foo.net; path=/foo; Version="1"')
        self.assertEqual(len(c), 1)
        self.assertEqual(interact_2965(c, "http://foo.net/foo"),
                         "$Version=1; foo=bar")

        # explicit foo.net from three-component domain www.foo.net *does* get
        # set, because .foo.net domain-matches .foo.net
        interact_2965(c, "http://www.foo.net/foo/",
                      'spam=eggs; domain=foo.net; Version="1"')
        self.assertEqual(c._cookies[".foo.net"]["/foo/"]["spam"].value,
                         "eggs")
        self.assertEqual(len(c), 2)
        self.assertEqual(interact_2965(c, "http://foo.net/foo/"),
                         "$Version=1; foo=bar")
        self.assertEqual(interact_2965(c, "http://www.foo.net/foo/"),
                         '$Version=1; spam=eggs; $Domain="foo.net"')

        # top-level domain is too general
        interact_2965(c, "http://foo.net/",
                      'ni="ni"; domain=".net"; Version="1"')
        self.assertEqual(len(c), 2)

        # RFC 2965 doesn't require blocking this
        interact_2965(c, "http://foo.co.uk/",
                      'nasty=trick; domain=.co.uk; Version="1"')
        self.assertEqual(len(c), 3)

    def test_domain_allow(self):
        c = CookieJar(policy=DefaultCookiePolicy(
            blocked_domains=["acme.com"],
            allowed_domains=["www.acme.com"]))

        req = urllib.request.Request("http://acme.com/")
        headers = ["Set-Cookie: CUSTOMER=WILE_E_COYOTE; path=/"]
        res = FakeResponse(headers, "http://acme.com/")
        c.extract_cookies(res, req)
        self.assertEqual(len(c), 0)

        req = urllib.request.Request("http://www.acme.com/")
        res = FakeResponse(headers, "http://www.acme.com/")
        c.extract_cookies(res, req)
        self.assertEqual(len(c), 1)

        req = urllib.request.Request("http://www.coyote.com/")
        res = FakeResponse(headers, "http://www.coyote.com/")
        c.extract_cookies(res, req)
        self.assertEqual(len(c), 1)

        # set a cookie with non-allowed domain...
        req = urllib.request.Request("http://www.coyote.com/")
        res = FakeResponse(headers, "http://www.coyote.com/")
        cookies = c.make_cookies(res, req)
        c.set_cookie(cookies[0])
        self.assertEqual(len(c), 2)
        # ... and check is doesn't get returned
        c.add_cookie_header(req)
        self.assertTrue(not req.has_header("Cookie"))

    def test_domain_block(self):
        pol = DefaultCookiePolicy(
            rfc2965=True, blocked_domains=[".acme.com"])
        c = CookieJar(policy=pol)
        headers = ["Set-Cookie: CUSTOMER=WILE_E_COYOTE; path=/"]

        req = urllib.request.Request("http://www.acme.com/")
        res = FakeResponse(headers, "http://www.acme.com/")
        c.extract_cookies(res, req)
        self.assertEqual(len(c), 0)

        p = pol.set_blocked_domains(["acme.com"])
        c.extract_cookies(res, req)
        self.assertEqual(len(c), 1)

        c.clear()
        req = urllib.request.Request("http://www.roadrunner.net/")
        res = FakeResponse(headers, "http://www.roadrunner.net/")
        c.extract_cookies(res, req)
        self.assertEqual(len(c), 1)
        req = urllib.request.Request("http://www.roadrunner.net/")
        c.add_cookie_header(req)
        self.assertTrue((req.has_header("Cookie") and
                      req.has_header("Cookie2")))

        c.clear()
        pol.set_blocked_domains([".acme.com"])
        c.extract_cookies(res, req)
        self.assertEqual(len(c), 1)

        # set a cookie with blocked domain...
        req = urllib.request.Request("http://www.acme.com/")
        res = FakeResponse(headers, "http://www.acme.com/")
        cookies = c.make_cookies(res, req)
        c.set_cookie(cookies[0])
        self.assertEqual(len(c), 2)
        # ... and check is doesn't get returned
        c.add_cookie_header(req)
        self.assertTrue(not req.has_header("Cookie"))

    def test_secure(self):
        for ns in True, False:
            for whitespace in " ", "":
                c = CookieJar()
                if ns:
                    pol = DefaultCookiePolicy(rfc2965=False)
                    int = interact_netscape
                    vs = ""
                else:
                    pol = DefaultCookiePolicy(rfc2965=True)
                    int = interact_2965
                    vs = "; Version=1"
                c.set_policy(pol)
                url = "http://www.acme.com/"
                int(c, url, "foo1=bar%s%s" % (vs, whitespace))
                int(c, url, "foo2=bar%s; secure%s" %  (vs, whitespace))
                self.assertTrue(
                    not c._cookies["www.acme.com"]["/"]["foo1"].secure,
                    "non-secure cookie registered secure")
                self.assertTrue(
                    c._cookies["www.acme.com"]["/"]["foo2"].secure,
                    "secure cookie registered non-secure")

    def test_quote_cookie_value(self):
        c = CookieJar(policy=DefaultCookiePolicy(rfc2965=True))
        interact_2965(c, "http://www.acme.com/", r'foo=\b"a"r; Version=1')
        h = interact_2965(c, "http://www.acme.com/")
        self.assertEqual(h, r'$Version=1; foo=\\b\"a\"r')

    def test_missing_final_slash(self):
        # Missing slash from request URL's abs_path should be assumed present.
        url = "http://www.acme.com"
        c = CookieJar(DefaultCookiePolicy(rfc2965=True))
        interact_2965(c, url, "foo=bar; Version=1")
        req = urllib.request.Request(url)
        self.assertEqual(len(c), 1)
        c.add_cookie_header(req)
        self.assertTrue(req.has_header("Cookie"))

    def test_domain_mirror(self):
        pol = DefaultCookiePolicy(rfc2965=True)

        c = CookieJar(pol)
        url = "http://foo.bar.com/"
        interact_2965(c, url, "spam=eggs; Version=1")
        h = interact_2965(c, url)
        self.assertNotIn("Domain", h,
                     "absent domain returned with domain present")

        c = CookieJar(pol)
        url = "http://foo.bar.com/"
        interact_2965(c, url, 'spam=eggs; Version=1; Domain=.bar.com')
        h = interact_2965(c, url)
        self.assertIn('$Domain=".bar.com"', h, "domain not returned")

        c = CookieJar(pol)
        url = "http://foo.bar.com/"
        # note missing initial dot in Domain
        interact_2965(c, url, 'spam=eggs; Version=1; Domain=bar.com')
        h = interact_2965(c, url)
        self.assertIn('$Domain="bar.com"', h, "domain not returned")

    def test_path_mirror(self):
        pol = DefaultCookiePolicy(rfc2965=True)

        c = CookieJar(pol)
        url = "http://foo.bar.com/"
        interact_2965(c, url, "spam=eggs; Version=1")
        h = interact_2965(c, url)
        self.assertNotIn("Path", h, "absent path returned with path present")

        c = CookieJar(pol)
        url = "http://foo.bar.com/"
        interact_2965(c, url, 'spam=eggs; Version=1; Path=/')
        h = interact_2965(c, url)
        self.assertIn('$Path="/"', h, "path not returned")

    def test_port_mirror(self):
        pol = DefaultCookiePolicy(rfc2965=True)

        c = CookieJar(pol)
        url = "http://foo.bar.com/"
        interact_2965(c, url, "spam=eggs; Version=1")
        h = interact_2965(c, url)
        self.assertNotIn("Port", h, "absent port returned with port present")

        c = CookieJar(pol)
        url = "http://foo.bar.com/"
        interact_2965(c, url, "spam=eggs; Version=1; Port")
        h = interact_2965(c, url)
        self.assertTrue(re.search("\$Port([^=]|$)", h),
                     "port with no value not returned with no value")

        c = CookieJar(pol)
        url = "http://foo.bar.com/"
        interact_2965(c, url, 'spam=eggs; Version=1; Port="80"')
        h = interact_2965(c, url)
        self.assertIn('$Port="80"', h,
                      "port with single value not returned with single value")

        c = CookieJar(pol)
        url = "http://foo.bar.com/"
        interact_2965(c, url, 'spam=eggs; Version=1; Port="80,8080"')
        h = interact_2965(c, url)
        self.assertIn('$Port="80,8080"', h,
                      "port with multiple values not returned with multiple "
                      "values")

    def test_no_return_comment(self):
        c = CookieJar(DefaultCookiePolicy(rfc2965=True))
        url = "http://foo.bar.com/"
        interact_2965(c, url, 'spam=eggs; Version=1; '
                      'Comment="does anybody read these?"; '
                      'CommentURL="http://foo.bar.net/comment.html"')
        h = interact_2965(c, url)
        self.assertTrue(
            "Comment" not in h,
            "Comment or CommentURL cookie-attributes returned to server")

    def test_Cookie_iterator(self):
        cs = CookieJar(DefaultCookiePolicy(rfc2965=True))
        # add some random cookies
        interact_2965(cs, "http://blah.spam.org/", 'foo=eggs; Version=1; '
                      'Comment="does anybody read these?"; '
                      'CommentURL="http://foo.bar.net/comment.html"')
        interact_netscape(cs, "http://www.acme.com/blah/", "spam=bar; secure")
        interact_2965(cs, "http://www.acme.com/blah/",
                      "foo=bar; secure; Version=1")
        interact_2965(cs, "http://www.acme.com/blah/",
                      "foo=bar; path=/; Version=1")
        interact_2965(cs, "http://www.sol.no",
                      r'bang=wallop; version=1; domain=".sol.no"; '
                      r'port="90,100, 80,8080"; '
                      r'max-age=100; Comment = "Just kidding! (\"|\\\\) "')

        versions = [1, 1, 1, 0, 1]
        names = ["bang", "foo", "foo", "spam", "foo"]
        domains = [".sol.no", "blah.spam.org", "www.acme.com",
                   "www.acme.com", "www.acme.com"]
        paths = ["/", "/", "/", "/blah", "/blah/"]

        for i in range(4):
            i = 0
            for c in cs:
                self.assertTrue(isinstance(c, Cookie))
                self.assertEqual(c.version, versions[i])
                self.assertEqual(c.name, names[i])
                self.assertEqual(c.domain, domains[i])
                self.assertEqual(c.path, paths[i])
                i = i + 1

    def test_parse_ns_headers(self):
        # missing domain value (invalid cookie)
        self.assertEqual(
            parse_ns_headers(["foo=bar; path=/; domain"]),
            [[("foo", "bar"),
              ("path", "/"), ("domain", None), ("version", "0")]]
            )
        # invalid expires value
        self.assertEqual(
            parse_ns_headers(["foo=bar; expires=Foo Bar 12 33:22:11 2000"]),
            [[("foo", "bar"), ("expires", None), ("version", "0")]]
            )
        # missing cookie value (valid cookie)
        self.assertEqual(
            parse_ns_headers(["foo"]),
            [[("foo", None), ("version", "0")]]
            )
        # shouldn't add version if header is empty
        self.assertEqual(parse_ns_headers([""]), [])

    def test_bad_cookie_header(self):

        def cookiejar_from_cookie_headers(headers):
            c = CookieJar()
            req = urllib.request.Request("http://www.example.com/")
            r = FakeResponse(headers, "http://www.example.com/")
            c.extract_cookies(r, req)
            return c

        # none of these bad headers should cause an exception to be raised
        for headers in [
            ["Set-Cookie: "],  # actually, nothing wrong with this
            ["Set-Cookie2: "],  # ditto
            # missing domain value
            ["Set-Cookie2: a=foo; path=/; Version=1; domain"],
            # bad max-age
            ["Set-Cookie: b=foo; max-age=oops"],
            # bad version
            ["Set-Cookie: b=foo; version=spam"],
            ]:
            c = cookiejar_from_cookie_headers(headers)
            # these bad cookies shouldn't be set
            self.assertEqual(len(c), 0)

        # cookie with invalid expires is treated as session cookie
        headers = ["Set-Cookie: c=foo; expires=Foo Bar 12 33:22:11 2000"]
        c = cookiejar_from_cookie_headers(headers)
        cookie = c._cookies["www.example.com"]["/"]["c"]
        self.assertTrue(cookie.expires is None)


class LWPCookieTests(unittest.TestCase):
    # Tests taken from libwww-perl, with a few modifications and additions.

    def test_netscape_example_1(self):
        #-------------------------------------------------------------------
        # First we check that it works for the original example at
        # http://www.netscape.com/newsref/std/cookie_spec.html

        # Client requests a document, and receives in the response:
        #
        #       Set-Cookie: CUSTOMER=WILE_E_COYOTE; path=/; expires=Wednesday, 09-Nov-99 23:12:40 GMT
        #
        # When client requests a URL in path "/" on this server, it sends:
        #
        #       Cookie: CUSTOMER=WILE_E_COYOTE
        #
        # Client requests a document, and receives in the response:
        #
        #       Set-Cookie: PART_NUMBER=ROCKET_LAUNCHER_0001; path=/
        #
        # When client requests a URL in path "/" on this server, it sends:
        #
        #       Cookie: CUSTOMER=WILE_E_COYOTE; PART_NUMBER=ROCKET_LAUNCHER_0001
        #
        # Client receives:
        #
        #       Set-Cookie: SHIPPING=FEDEX; path=/fo
        #
        # When client requests a URL in path "/" on this server, it sends:
        #
        #       Cookie: CUSTOMER=WILE_E_COYOTE; PART_NUMBER=ROCKET_LAUNCHER_0001
        #
        # When client requests a URL in path "/foo" on this server, it sends:
        #
        #       Cookie: CUSTOMER=WILE_E_COYOTE; PART_NUMBER=ROCKET_LAUNCHER_0001; SHIPPING=FEDEX
        #
        # The last Cookie is buggy, because both specifications say that the
        # most specific cookie must be sent first.  SHIPPING=FEDEX is the
        # most specific and should thus be first.

        year_plus_one = time.localtime()[0] + 1

        headers = []

        c = CookieJar(DefaultCookiePolicy(rfc2965 = True))

        #req = urllib.request.Request("http://1.1.1.1/",
        #              headers={"Host": "www.acme.com:80"})
        req = urllib.request.Request("http://www.acme.com:80/",
                      headers={"Host": "www.acme.com:80"})

        headers.append(
            "Set-Cookie: CUSTOMER=WILE_E_COYOTE; path=/ ; "
            "expires=Wednesday, 09-Nov-%d 23:12:40 GMT" % year_plus_one)
        res = FakeResponse(headers, "http://www.acme.com/")
        c.extract_cookies(res, req)

        req = urllib.request.Request("http://www.acme.com/")
        c.add_cookie_header(req)

        self.assertEqual(req.get_header("Cookie"), "CUSTOMER=WILE_E_COYOTE")
        self.assertEqual(req.get_header("Cookie2"), '$Version="1"')

        headers.append("Set-Cookie: PART_NUMBER=ROCKET_LAUNCHER_0001; path=/")
        res = FakeResponse(headers, "http://www.acme.com/")
        c.extract_cookies(res, req)

        req = urllib.request.Request("http://www.acme.com/foo/bar")
        c.add_cookie_header(req)

        h = req.get_header("Cookie")
        self.assertIn("PART_NUMBER=ROCKET_LAUNCHER_0001", h)
        self.assertIn("CUSTOMER=WILE_E_COYOTE", h)

        headers.append('Set-Cookie: SHIPPING=FEDEX; path=/foo')
        res = FakeResponse(headers, "http://www.acme.com")
        c.extract_cookies(res, req)

        req = urllib.request.Request("http://www.acme.com/")
        c.add_cookie_header(req)

        h = req.get_header("Cookie")
        self.assertIn("PART_NUMBER=ROCKET_LAUNCHER_0001", h)
        self.assertIn("CUSTOMER=WILE_E_COYOTE", h)
        self.assertNotIn("SHIPPING=FEDEX", h)

        req = urllib.request.Request("http://www.acme.com/foo/")
        c.add_cookie_header(req)

        h = req.get_header("Cookie")
        self.assertIn("PART_NUMBER=ROCKET_LAUNCHER_0001", h)
        self.assertIn("CUSTOMER=WILE_E_COYOTE", h)
        self.assertTrue(h.startswith("SHIPPING=FEDEX;"))

    def test_netscape_example_2(self):
        # Second Example transaction sequence:
        #
        # Assume all mappings from above have been cleared.
        #
        # Client receives:
        #
        #       Set-Cookie: PART_NUMBER=ROCKET_LAUNCHER_0001; path=/
        #
        # When client requests a URL in path "/" on this server, it sends:
        #
        #       Cookie: PART_NUMBER=ROCKET_LAUNCHER_0001
        #
        # Client receives:
        #
        #       Set-Cookie: PART_NUMBER=RIDING_ROCKET_0023; path=/ammo
        #
        # When client requests a URL in path "/ammo" on this server, it sends:
        #
        #       Cookie: PART_NUMBER=RIDING_ROCKET_0023; PART_NUMBER=ROCKET_LAUNCHER_0001
        #
        #       NOTE: There are two name/value pairs named "PART_NUMBER" due to
        #       the inheritance of the "/" mapping in addition to the "/ammo" mapping.

        c = CookieJar()
        headers = []

        req = urllib.request.Request("http://www.acme.com/")
        headers.append("Set-Cookie: PART_NUMBER=ROCKET_LAUNCHER_0001; path=/")
        res = FakeResponse(headers, "http://www.acme.com/")

        c.extract_cookies(res, req)

        req = urllib.request.Request("http://www.acme.com/")
        c.add_cookie_header(req)

        self.assertEqual(req.get_header("Cookie"),
                         "PART_NUMBER=ROCKET_LAUNCHER_0001")

        headers.append(
            "Set-Cookie: PART_NUMBER=RIDING_ROCKET_0023; path=/ammo")
        res = FakeResponse(headers, "http://www.acme.com/")
        c.extract_cookies(res, req)

        req = urllib.request.Request("http://www.acme.com/ammo")
        c.add_cookie_header(req)

        self.assertTrue(re.search(r"PART_NUMBER=RIDING_ROCKET_0023;\s*"
                               "PART_NUMBER=ROCKET_LAUNCHER_0001",
                               req.get_header("Cookie")))

    def test_ietf_example_1(self):
        #-------------------------------------------------------------------
        # Then we test with the examples from draft-ietf-http-state-man-mec-03.txt
        #
        # 5.  EXAMPLES

        c = CookieJar(DefaultCookiePolicy(rfc2965=True))

        #
        # 5.1  Example 1
        #
        # Most detail of request and response headers has been omitted.  Assume
        # the user agent has no stored cookies.
        #
        #   1.  User Agent -> Server
        #
        #       POST /acme/login HTTP/1.1
        #       [form data]
        #
        #       User identifies self via a form.
        #
        #   2.  Server -> User Agent
        #
        #       HTTP/1.1 200 OK
        #       Set-Cookie2: Customer="WILE_E_COYOTE"; Version="1"; Path="/acme"
        #
        #       Cookie reflects user's identity.

        cookie = interact_2965(
            c, 'http://www.acme.com/acme/login',
            'Customer="WILE_E_COYOTE"; Version="1"; Path="/acme"')
        self.assertTrue(not cookie)

        #
        #   3.  User Agent -> Server
        #
        #       POST /acme/pickitem HTTP/1.1
        #       Cookie: $Version="1"; Customer="WILE_E_COYOTE"; $Path="/acme"
        #       [form data]
        #
        #       User selects an item for ``shopping basket.''
        #
        #   4.  Server -> User Agent
        #
        #       HTTP/1.1 200 OK
        #       Set-Cookie2: Part_Number="Rocket_Launcher_0001"; Version="1";
        #               Path="/acme"
        #
        #       Shopping basket contains an item.

        cookie = interact_2965(c, 'http://www.acme.com/acme/pickitem',
                               'Part_Number="Rocket_Launcher_0001"; '
                               'Version="1"; Path="/acme"');
        self.assertTrue(re.search(
            r'^\$Version="?1"?; Customer="?WILE_E_COYOTE"?; \$Path="/acme"$',
            cookie))

        #
        #   5.  User Agent -> Server
        #
        #       POST /acme/shipping HTTP/1.1
        #       Cookie: $Version="1";
        #               Customer="WILE_E_COYOTE"; $Path="/acme";
        #               Part_Number="Rocket_Launcher_0001"; $Path="/acme"
        #       [form data]
        #
        #       User selects shipping method from form.
        #
        #   6.  Server -> User Agent
        #
        #       HTTP/1.1 200 OK
        #       Set-Cookie2: Shipping="FedEx"; Version="1"; Path="/acme"
        #
        #       New cookie reflects shipping method.

        cookie = interact_2965(c, "http://www.acme.com/acme/shipping",
                               'Shipping="FedEx"; Version="1"; Path="/acme"')

        self.assertTrue(re.search(r'^\$Version="?1"?;', cookie))
        self.assertTrue(re.search(r'Part_Number="?Rocket_Launcher_0001"?;'
                               '\s*\$Path="\/acme"', cookie))
        self.assertTrue(re.search(r'Customer="?WILE_E_COYOTE"?;\s*\$Path="\/acme"',
                               cookie))

        #
        #   7.  User Agent -> Server
        #
        #       POST /acme/process HTTP/1.1
        #       Cookie: $Version="1";
        #               Customer="WILE_E_COYOTE"; $Path="/acme";
        #               Part_Number="Rocket_Launcher_0001"; $Path="/acme";
        #               Shipping="FedEx"; $Path="/acme"
        #       [form data]
        #
        #       User chooses to process order.
        #
        #   8.  Server -> User Agent
        #
        #       HTTP/1.1 200 OK
        #
        #       Transaction is complete.

        cookie = interact_2965(c, "http://www.acme.com/acme/process")
        self.assertTrue(
            re.search(r'Shipping="?FedEx"?;\s*\$Path="\/acme"', cookie) and
            "WILE_E_COYOTE" in cookie)

        #
        # The user agent makes a series of requests on the origin server, after
        # each of which it receives a new cookie.  All the cookies have the same
        # Path attribute and (default) domain.  Because the request URLs all have
        # /acme as a prefix, and that matches the Path attribute, each request
        # contains all the cookies received so far.

    def test_ietf_example_2(self):
        # 5.2  Example 2
        #
        # This example illustrates the effect of the Path attribute.  All detail
        # of request and response headers has been omitted.  Assume the user agent
        # has no stored cookies.

        c = CookieJar(DefaultCookiePolicy(rfc2965=True))

        # Imagine the user agent has received, in response to earlier requests,
        # the response headers
        #
        # Set-Cookie2: Part_Number="Rocket_Launcher_0001"; Version="1";
        #         Path="/acme"
        #
        # and
        #
        # Set-Cookie2: Part_Number="Riding_Rocket_0023"; Version="1";
        #         Path="/acme/ammo"

        interact_2965(
            c, "http://www.acme.com/acme/ammo/specific",
            'Part_Number="Rocket_Launcher_0001"; Version="1"; Path="/acme"',
            'Part_Number="Riding_Rocket_0023"; Version="1"; Path="/acme/ammo"')

        # A subsequent request by the user agent to the (same) server for URLs of
        # the form /acme/ammo/...  would include the following request header:
        #
        # Cookie: $Version="1";
        #         Part_Number="Riding_Rocket_0023"; $Path="/acme/ammo";
        #         Part_Number="Rocket_Launcher_0001"; $Path="/acme"
        #
        # Note that the NAME=VALUE pair for the cookie with the more specific Path
        # attribute, /acme/ammo, comes before the one with the less specific Path
        # attribute, /acme.  Further note that the same cookie name appears more
        # than once.

        cookie = interact_2965(c, "http://www.acme.com/acme/ammo/...")
        self.assertTrue(
            re.search(r"Riding_Rocket_0023.*Rocket_Launcher_0001", cookie))

        # A subsequent request by the user agent to the (same) server for a URL of
        # the form /acme/parts/ would include the following request header:
        #
        # Cookie: $Version="1"; Part_Number="Rocket_Launcher_0001"; $Path="/acme"
        #
        # Here, the second cookie's Path attribute /acme/ammo is not a prefix of
        # the request URL, /acme/parts/, so the cookie does not get forwarded to
        # the server.

        cookie = interact_2965(c, "http://www.acme.com/acme/parts/")
        self.assertIn("Rocket_Launcher_0001", cookie)
        self.assertNotIn("Riding_Rocket_0023", cookie)

    def test_rejection(self):
        # Test rejection of Set-Cookie2 responses based on domain, path, port.
        pol = DefaultCookiePolicy(rfc2965=True)

        c = LWPCookieJar(policy=pol)

        max_age = "max-age=3600"

        # illegal domain (no embedded dots)
        cookie = interact_2965(c, "http://www.acme.com",
                               'foo=bar; domain=".com"; version=1')
        self.assertTrue(not c)

        # legal domain
        cookie = interact_2965(c, "http://www.acme.com",
                               'ping=pong; domain="acme.com"; version=1')
        self.assertEqual(len(c), 1)

        # illegal domain (host prefix "www.a" contains a dot)
        cookie = interact_2965(c, "http://www.a.acme.com",
                               'whiz=bang; domain="acme.com"; version=1')
        self.assertEqual(len(c), 1)

        # legal domain
        cookie = interact_2965(c, "http://www.a.acme.com",
                               'wow=flutter; domain=".a.acme.com"; version=1')
        self.assertEqual(len(c), 2)

        # can't partially match an IP-address
        cookie = interact_2965(c, "http://125.125.125.125",
                               'zzzz=ping; domain="125.125.125"; version=1')
        self.assertEqual(len(c), 2)

        # illegal path (must be prefix of request path)
        cookie = interact_2965(c, "http://www.sol.no",
                               'blah=rhubarb; domain=".sol.no"; path="/foo"; '
                               'version=1')
        self.assertEqual(len(c), 2)

        # legal path
        cookie = interact_2965(c, "http://www.sol.no/foo/bar",
                               'bing=bong; domain=".sol.no"; path="/foo"; '
                               'version=1')
        self.assertEqual(len(c), 3)

        # illegal port (request-port not in list)
        cookie = interact_2965(c, "http://www.sol.no",
                               'whiz=ffft; domain=".sol.no"; port="90,100"; '
                               'version=1')
        self.assertEqual(len(c), 3)

        # legal port
        cookie = interact_2965(
            c, "http://www.sol.no",
            r'bang=wallop; version=1; domain=".sol.no"; '
            r'port="90,100, 80,8080"; '
            r'max-age=100; Comment = "Just kidding! (\"|\\\\) "')
        self.assertEqual(len(c), 4)

        # port attribute without any value (current port)
        cookie = interact_2965(c, "http://www.sol.no",
                               'foo9=bar; version=1; domain=".sol.no"; port; '
                               'max-age=100;')
        self.assertEqual(len(c), 5)

        # encoded path
        # LWP has this test, but unescaping allowed path characters seems
        # like a bad idea, so I think this should fail:
##         cookie = interact_2965(c, "http://www.sol.no/foo/",
##                           r'foo8=bar; version=1; path="/%66oo"')
        # but this is OK, because '<' is not an allowed HTTP URL path
        # character:
        cookie = interact_2965(c, "http://www.sol.no/<oo/",
                               r'foo8=bar; version=1; path="/%3coo"')
        self.assertEqual(len(c), 6)

        # save and restore
        filename = test.support.TESTFN

        try:
            c.save(filename, ignore_discard=True)
            old = repr(c)

            c = LWPCookieJar(policy=pol)
            c.load(filename, ignore_discard=True)
        finally:
            try: os.unlink(filename)
            except OSError: pass

        self.assertEqual(old, repr(c))

    def test_url_encoding(self):
        # Try some URL encodings of the PATHs.
        # (the behaviour here has changed from libwww-perl)
        c = CookieJar(DefaultCookiePolicy(rfc2965=True))
        interact_2965(c, "http://www.acme.com/foo%2f%25/"
                         "%3c%3c%0Anew%C3%A5/%C3%A5",
                      "foo  =   bar; version    =   1")

        cookie = interact_2965(
            c, "http://www.acme.com/foo%2f%25/<<%0anew\345/\346\370\345",
            'bar=baz; path="/foo/"; version=1');
        version_re = re.compile(r'^\$version=\"?1\"?', re.I)
        self.assertIn("foo=bar", cookie)
        self.assertTrue(version_re.search(cookie))

        cookie = interact_2965(
            c, "http://www.acme.com/foo/%25/<<%0anew\345/\346\370\345")
        self.assertTrue(not cookie)

        # unicode URL doesn't raise exception
        cookie = interact_2965(c, "http://www.acme.com/\xfc")

    def test_mozilla(self):
        # Save / load Mozilla/Netscape cookie file format.
        year_plus_one = time.localtime()[0] + 1

        filename = test.support.TESTFN

        c = MozillaCookieJar(filename,
                             policy=DefaultCookiePolicy(rfc2965=True))
        interact_2965(c, "http://www.acme.com/",
                      "foo1=bar; max-age=100; Version=1")
        interact_2965(c, "http://www.acme.com/",
                      'foo2=bar; port="80"; max-age=100; Discard; Version=1')
        interact_2965(c, "http://www.acme.com/", "foo3=bar; secure; Version=1")

        expires = "expires=09-Nov-%d 23:12:40 GMT" % (year_plus_one,)
        interact_netscape(c, "http://www.foo.com/",
                          "fooa=bar; %s" % expires)
        interact_netscape(c, "http://www.foo.com/",
                          "foob=bar; Domain=.foo.com; %s" % expires)
        interact_netscape(c, "http://www.foo.com/",
                          "fooc=bar; Domain=www.foo.com; %s" % expires)

        def save_and_restore(cj, ignore_discard):
            try:
                cj.save(ignore_discard=ignore_discard)
                new_c = MozillaCookieJar(filename,
                                         DefaultCookiePolicy(rfc2965=True))
                new_c.load(ignore_discard=ignore_discard)
            finally:
                try: os.unlink(filename)
                except OSError: pass
            return new_c

        new_c = save_and_restore(c, True)
        self.assertEqual(len(new_c), 6)  # none discarded
        self.assertIn("name='foo1', value='bar'", repr(new_c))

        new_c = save_and_restore(c, False)
        self.assertEqual(len(new_c), 4)  # 2 of them discarded on save
        self.assertIn("name='foo1', value='bar'", repr(new_c))

    def test_netscape_misc(self):
        # Some additional Netscape cookies tests.
        c = CookieJar()
        headers = []
        req = urllib.request.Request("http://foo.bar.acme.com/foo")

        # Netscape allows a host part that contains dots
        headers.append("Set-Cookie: Customer=WILE_E_COYOTE; domain=.acme.com")
        res = FakeResponse(headers, "http://www.acme.com/foo")
        c.extract_cookies(res, req)

        # and that the domain is the same as the host without adding a leading
        # dot to the domain.  Should not quote even if strange chars are used
        # in the cookie value.
        headers.append("Set-Cookie: PART_NUMBER=3,4; domain=foo.bar.acme.com")
        res = FakeResponse(headers, "http://www.acme.com/foo")
        c.extract_cookies(res, req)

        req = urllib.request.Request("http://foo.bar.acme.com/foo")
        c.add_cookie_header(req)
        self.assertIn("PART_NUMBER=3,4", req.get_header("Cookie"))
        self.assertIn("Customer=WILE_E_COYOTE",req.get_header("Cookie"))

    def test_intranet_domains_2965(self):
        # Test handling of local intranet hostnames without a dot.
        c = CookieJar(DefaultCookiePolicy(rfc2965=True))
        interact_2965(c, "http://example/",
                      "foo1=bar; PORT; Discard; Version=1;")
        cookie = interact_2965(c, "http://example/",
                               'foo2=bar; domain=".local"; Version=1')
        self.assertIn("foo1=bar", cookie)

        interact_2965(c, "http://example/", 'foo3=bar; Version=1')
        cookie = interact_2965(c, "http://example/")
        self.assertIn("foo2=bar", cookie)
        self.assertEqual(len(c), 3)

    def test_intranet_domains_ns(self):
        c = CookieJar(DefaultCookiePolicy(rfc2965 = False))
        interact_netscape(c, "http://example/", "foo1=bar")
        cookie = interact_netscape(c, "http://example/",
                                   'foo2=bar; domain=.local')
        self.assertEqual(len(c), 2)
        self.assertIn("foo1=bar", cookie)

        cookie = interact_netscape(c, "http://example/")
        self.assertIn("foo2=bar", cookie)
        self.assertEqual(len(c), 2)

    def test_empty_path(self):
        # Test for empty path
        # Broken web-server ORION/1.3.38 returns to the client response like
        #
        #       Set-Cookie: JSESSIONID=ABCDERANDOM123; Path=
        #
        # ie. with Path set to nothing.
        # In this case, extract_cookies() must set cookie to / (root)
        c = CookieJar(DefaultCookiePolicy(rfc2965 = True))
        headers = []

        req = urllib.request.Request("http://www.ants.com/")
        headers.append("Set-Cookie: JSESSIONID=ABCDERANDOM123; Path=")
        res = FakeResponse(headers, "http://www.ants.com/")
        c.extract_cookies(res, req)

        req = urllib.request.Request("http://www.ants.com/")
        c.add_cookie_header(req)

        self.assertEqual(req.get_header("Cookie"),
                         "JSESSIONID=ABCDERANDOM123")
        self.assertEqual(req.get_header("Cookie2"), '$Version="1"')

        # missing path in the request URI
        req = urllib.request.Request("http://www.ants.com:8080")
        c.add_cookie_header(req)

        self.assertEqual(req.get_header("Cookie"),
                         "JSESSIONID=ABCDERANDOM123")
        self.assertEqual(req.get_header("Cookie2"), '$Version="1"')

    def test_session_cookies(self):
        year_plus_one = time.localtime()[0] + 1

        # Check session cookies are deleted properly by
        # CookieJar.clear_session_cookies method

        req = urllib.request.Request('http://www.perlmeister.com/scripts')
        headers = []
        headers.append("Set-Cookie: s1=session;Path=/scripts")
        headers.append("Set-Cookie: p1=perm; Domain=.perlmeister.com;"
                       "Path=/;expires=Fri, 02-Feb-%d 23:24:20 GMT" %
                       year_plus_one)
        headers.append("Set-Cookie: p2=perm;Path=/;expires=Fri, "
                       "02-Feb-%d 23:24:20 GMT" % year_plus_one)
        headers.append("Set-Cookie: s2=session;Path=/scripts;"
                       "Domain=.perlmeister.com")
        headers.append('Set-Cookie2: s3=session;Version=1;Discard;Path="/"')
        res = FakeResponse(headers, 'http://www.perlmeister.com/scripts')

        c = CookieJar()
        c.extract_cookies(res, req)
        # How many session/permanent cookies do we have?
        counter = {"session_after": 0,
                   "perm_after": 0,
                   "session_before": 0,
                   "perm_before": 0}
        for cookie in c:
            key = "%s_before" % cookie.value
            counter[key] = counter[key] + 1
        c.clear_session_cookies()
        # How many now?
        for cookie in c:
            key = "%s_after" % cookie.value
            counter[key] = counter[key] + 1

        self.assertTrue(not (
            # a permanent cookie got lost accidently
            counter["perm_after"] != counter["perm_before"] or
            # a session cookie hasn't been cleared
            counter["session_after"] != 0 or
            # we didn't have session cookies in the first place
            counter["session_before"] == 0))


def test_main(verbose=None):
    test.support.run_unittest(
        DateTimeTests,
        HeaderTests,
        CookieTests,
        FileCookieJarTests,
        LWPCookieTests,
        )

if __name__ == "__main__":
    test_main(verbose=True)
