# -*- coding: utf-8 -*-

# Copyright (c) Twisted Matrix Laboratories.
# See LICENSE for details.

from __future__ import unicode_literals

import sys
import socket
from typing import Any, Iterable, Optional, Text, Tuple, cast

from .common import HyperlinkTestCase
from .. import URL, URLParseError
from .._url import inet_pton, SCHEME_PORT_MAP


PY2 = sys.version_info[0] == 2
unicode = type("")


BASIC_URL = "http://www.foo.com/a/nice/path/?zot=23&zut"

# Examples from RFC 3986 section 5.4, Reference Resolution Examples
relativeLinkBaseForRFC3986 = "http://a/b/c/d;p?q"
relativeLinkTestsForRFC3986 = [
    # "Normal"
    # ('g:h', 'g:h'),  # can't click on a scheme-having url without an abs path
    ("g", "http://a/b/c/g"),
    ("./g", "http://a/b/c/g"),
    ("g/", "http://a/b/c/g/"),
    ("/g", "http://a/g"),
    ("//g", "http://g"),
    ("?y", "http://a/b/c/d;p?y"),
    ("g?y", "http://a/b/c/g?y"),
    ("#s", "http://a/b/c/d;p?q#s"),
    ("g#s", "http://a/b/c/g#s"),
    ("g?y#s", "http://a/b/c/g?y#s"),
    (";x", "http://a/b/c/;x"),
    ("g;x", "http://a/b/c/g;x"),
    ("g;x?y#s", "http://a/b/c/g;x?y#s"),
    ("", "http://a/b/c/d;p?q"),
    (".", "http://a/b/c/"),
    ("./", "http://a/b/c/"),
    ("..", "http://a/b/"),
    ("../", "http://a/b/"),
    ("../g", "http://a/b/g"),
    ("../..", "http://a/"),
    ("../../", "http://a/"),
    ("../../g", "http://a/g"),
    # Abnormal examples
    # ".." cannot be used to change the authority component of a URI.
    ("../../../g", "http://a/g"),
    ("../../../../g", "http://a/g"),
    # Only include "." and ".." when they are only part of a larger segment,
    # not by themselves.
    ("/./g", "http://a/g"),
    ("/../g", "http://a/g"),
    ("g.", "http://a/b/c/g."),
    (".g", "http://a/b/c/.g"),
    ("g..", "http://a/b/c/g.."),
    ("..g", "http://a/b/c/..g"),
    # Unnecessary or nonsensical forms of "." and "..".
    ("./../g", "http://a/b/g"),
    ("./g/.", "http://a/b/c/g/"),
    ("g/./h", "http://a/b/c/g/h"),
    ("g/../h", "http://a/b/c/h"),
    ("g;x=1/./y", "http://a/b/c/g;x=1/y"),
    ("g;x=1/../y", "http://a/b/c/y"),
    # Separating the reference's query and fragment components from the path.
    ("g?y/./x", "http://a/b/c/g?y/./x"),
    ("g?y/../x", "http://a/b/c/g?y/../x"),
    ("g#s/./x", "http://a/b/c/g#s/./x"),
    ("g#s/../x", "http://a/b/c/g#s/../x"),
]


ROUNDTRIP_TESTS = (
    "http://localhost",
    "http://localhost/",
    "http://127.0.0.1/",
    "http://[::127.0.0.1]/",
    "http://[::1]/",
    "http://localhost/foo",
    "http://localhost/foo/",
    "http://localhost/foo!!bar/",
    "http://localhost/foo%20bar/",
    "http://localhost/foo%2Fbar/",
    "http://localhost/foo?n",
    "http://localhost/foo?n=v",
    "http://localhost/foo?n=/a/b",
    "http://example.com/foo!@$bar?b!@z=123",
    "http://localhost/asd?a=asd%20sdf/345",
    "http://(%2525)/(%2525)?(%2525)&(%2525)=(%2525)#(%2525)",
    "http://(%C3%A9)/(%C3%A9)?(%C3%A9)&(%C3%A9)=(%C3%A9)#(%C3%A9)",
    "?sslrootcert=/Users/glyph/Downloads/rds-ca-2015-root.pem&sslmode=verify",
    # from boltons.urlutils' tests
    "http://googlewebsite.com/e-shops.aspx",
    "http://example.com:8080/search?q=123&business=Nothing%20Special",
    "http://hatnote.com:9000/?arg=1&arg=2&arg=3",
    "https://xn--bcher-kva.ch",
    "http://xn--ggbla1c4e.xn--ngbc5azd/",
    "http://tools.ietf.org/html/rfc3986#section-3.4",
    # 'http://wiki:pedia@hatnote.com',
    "ftp://ftp.rfc-editor.org/in-notes/tar/RFCs0001-0500.tar.gz",
    "http://[1080:0:0:0:8:800:200C:417A]/index.html",
    "ssh://192.0.2.16:2222/",
    "https://[::101.45.75.219]:80/?hi=bye",
    "ldap://[::192.9.5.5]/dc=example,dc=com??sub?(sn=Jensen)",
    "mailto:me@example.com?to=me@example.com&body=hi%20http://wikipedia.org",
    "news:alt.rec.motorcycle",
    "tel:+1-800-867-5309",
    "urn:oasis:member:A00024:x",
    (
        "magnet:?xt=urn:btih:1a42b9e04e122b97a5254e3df77ab3c4b7da725f&dn=Puppy%"
        "20Linux%20precise-5.7.1.iso&tr=udp://tracker.openbittorrent.com:80&"
        "tr=udp://tracker.publicbt.com:80&tr=udp://tracker.istole.it:6969&"
        "tr=udp://tracker.ccc.de:80&tr=udp://open.demonii.com:1337"
    ),
    # percent-encoded delimiters in percent-encodable fields
    "https://%3A@example.com/",  # colon in username
    "https://%40@example.com/",  # at sign in username
    "https://%2f@example.com/",  # slash in username
    "https://a:%3a@example.com/",  # colon in password
    "https://a:%40@example.com/",  # at sign in password
    "https://a:%2f@example.com/",  # slash in password
    "https://a:%3f@example.com/",  # question mark in password
    "https://example.com/%2F/",  # slash in path
    "https://example.com/%3F/",  # question mark in path
    "https://example.com/%23/",  # hash in path
    "https://example.com/?%23=b",  # hash in query param name
    "https://example.com/?%3D=b",  # equals in query param name
    "https://example.com/?%26=b",  # ampersand in query param name
    "https://example.com/?a=%23",  # hash in query param value
    "https://example.com/?a=%26",  # ampersand in query param value
    "https://example.com/?a=%3D",  # equals in query param value
    # double-encoded percent sign in all percent-encodable positions:
    "http://(%2525):(%2525)@example.com/(%2525)/?(%2525)=(%2525)#(%2525)",
    # colon in first part of schemeless relative url
    "first_seg_rel_path__colon%3Anotok/second_seg__colon%3Aok",
)


class TestURL(HyperlinkTestCase):
    """
    Tests for L{URL}.
    """

    def assertUnicoded(self, u):
        # type: (URL) -> None
        """
        The given L{URL}'s components should be L{unicode}.

        @param u: The L{URL} to test.
        """
        self.assertTrue(
            isinstance(u.scheme, unicode) or u.scheme is None, repr(u)
        )
        self.assertTrue(isinstance(u.host, unicode) or u.host is None, repr(u))
        for seg in u.path:
            self.assertEqual(type(seg), unicode, repr(u))
        for (_k, v) in u.query:
            self.assertEqual(type(seg), unicode, repr(u))
            self.assertTrue(v is None or isinstance(v, unicode), repr(u))
        self.assertEqual(type(u.fragment), unicode, repr(u))

    def assertURL(
        self,
        u,  # type: URL
        scheme,  # type: Text
        host,  # type: Text
        path,  # type: Iterable[Text]
        query,  # type: Iterable[Tuple[Text, Optional[Text]]]
        fragment,  # type: Text
        port,  # type: Optional[int]
        userinfo="",  # type: Text
    ):
        # type: (...) -> None
        """
        The given L{URL} should have the given components.

        @param u: The actual L{URL} to examine.

        @param scheme: The expected scheme.

        @param host: The expected host.

        @param path: The expected path.

        @param query: The expected query.

        @param fragment: The expected fragment.

        @param port: The expected port.

        @param userinfo: The expected userinfo.
        """
        actual = (
            u.scheme,
            u.host,
            u.path,
            u.query,
            u.fragment,
            u.port,
            u.userinfo,
        )
        expected = (
            scheme,
            host,
            tuple(path),
            tuple(query),
            fragment,
            port,
            u.userinfo,
        )
        self.assertEqual(actual, expected)

    def test_initDefaults(self):
        # type: () -> None
        """
        L{URL} should have appropriate default values.
        """

        def check(u):
            # type: (URL) -> None
            self.assertUnicoded(u)
            self.assertURL(u, "http", "", [], [], "", 80, "")

        check(URL("http", ""))
        check(URL("http", "", [], []))
        check(URL("http", "", [], [], ""))

    def test_init(self):
        # type: () -> None
        """
        L{URL} should accept L{unicode} parameters.
        """
        u = URL("s", "h", ["p"], [("k", "v"), ("k", None)], "f")
        self.assertUnicoded(u)
        self.assertURL(u, "s", "h", ["p"], [("k", "v"), ("k", None)], "f", None)

        self.assertURL(
            URL("http", "\xe0", ["\xe9"], [("\u03bb", "\u03c0")], "\u22a5"),
            "http",
            "\xe0",
            ["\xe9"],
            [("\u03bb", "\u03c0")],
            "\u22a5",
            80,
        )

    def test_initPercent(self):
        # type: () -> None
        """
        L{URL} should accept (and not interpret) percent characters.
        """
        u = URL("s", "%68", ["%70"], [("%6B", "%76"), ("%6B", None)], "%66")
        self.assertUnicoded(u)
        self.assertURL(
            u, "s", "%68", ["%70"], [("%6B", "%76"), ("%6B", None)], "%66", None
        )

    def test_repr(self):
        # type: () -> None
        """
        L{URL.__repr__} will display the canonical form of the URL, wrapped in
        a L{URL.from_text} invocation, so that it is C{eval}-able but still
        easy to read.
        """
        self.assertEqual(
            repr(
                URL(
                    scheme="http",
                    host="foo",
                    path=["bar"],
                    query=[("baz", None), ("k", "v")],
                    fragment="frob",
                )
            ),
            "URL.from_text(%s)" % (repr("http://foo/bar?baz&k=v#frob"),),
        )

    def test_from_text(self):
        # type: () -> None
        """
        Round-tripping L{URL.from_text} with C{str} results in an equivalent
        URL.
        """
        urlpath = URL.from_text(BASIC_URL)
        self.assertEqual(BASIC_URL, urlpath.to_text())

    def test_roundtrip(self):
        # type: () -> None
        """
        L{URL.to_text} should invert L{URL.from_text}.
        """
        for test in ROUNDTRIP_TESTS:
            result = URL.from_text(test).to_text(with_password=True)
            self.assertEqual(test, result)

    def test_roundtrip_double_iri(self):
        # type: () -> None
        for test in ROUNDTRIP_TESTS:
            url = URL.from_text(test)
            iri = url.to_iri()
            double_iri = iri.to_iri()
            assert iri == double_iri

            iri_text = iri.to_text(with_password=True)
            double_iri_text = double_iri.to_text(with_password=True)
            assert iri_text == double_iri_text
        return

    def test_equality(self):
        # type: () -> None
        """
        Two URLs decoded using L{URL.from_text} will be equal (C{==}) if they
        decoded same URL string, and unequal (C{!=}) if they decoded different
        strings.
        """
        urlpath = URL.from_text(BASIC_URL)
        self.assertEqual(urlpath, URL.from_text(BASIC_URL))
        self.assertNotEqual(
            urlpath,
            URL.from_text(
                "ftp://www.anotherinvaliddomain.com/" "foo/bar/baz/?zot=21&zut"
            ),
        )

    def test_fragmentEquality(self):
        # type: () -> None
        """
        An URL created with the empty string for a fragment compares equal
        to an URL created with an unspecified fragment.
        """
        self.assertEqual(URL(fragment=""), URL())
        self.assertEqual(
            URL.from_text("http://localhost/#"),
            URL.from_text("http://localhost/"),
        )

    def test_child(self):
        # type: () -> None
        """
        L{URL.child} appends a new path segment, but does not affect the query
        or fragment.
        """
        urlpath = URL.from_text(BASIC_URL)
        self.assertEqual(
            "http://www.foo.com/a/nice/path/gong?zot=23&zut",
            urlpath.child("gong").to_text(),
        )
        self.assertEqual(
            "http://www.foo.com/a/nice/path/gong%2F?zot=23&zut",
            urlpath.child("gong/").to_text(),
        )
        self.assertEqual(
            "http://www.foo.com/a/nice/path/gong%2Fdouble?zot=23&zut",
            urlpath.child("gong/double").to_text(),
        )
        self.assertEqual(
            "http://www.foo.com/a/nice/path/gong%2Fdouble%2F?zot=23&zut",
            urlpath.child("gong/double/").to_text(),
        )

    def test_multiChild(self):
        # type: () -> None
        """
        L{URL.child} receives multiple segments as C{*args} and appends each in
        turn.
        """
        url = URL.from_text("http://example.com/a/b")
        self.assertEqual(
            url.child("c", "d", "e").to_text(), "http://example.com/a/b/c/d/e"
        )

    def test_childInitRoot(self):
        # type: () -> None
        """
        L{URL.child} of a L{URL} without a path produces a L{URL} with a single
        path segment.
        """
        childURL = URL(host="www.foo.com").child("c")
        self.assertTrue(childURL.rooted)
        self.assertEqual("http://www.foo.com/c", childURL.to_text())

    def test_emptyChild(self):
        # type: () -> None
        """
        L{URL.child} without any new segments returns the original L{URL}.
        """
        url = URL(host="www.foo.com")
        self.assertEqual(url.child(), url)

    def test_sibling(self):
        # type: () -> None
        """
        L{URL.sibling} of a L{URL} replaces the last path segment, but does not
        affect the query or fragment.
        """
        urlpath = URL.from_text(BASIC_URL)
        self.assertEqual(
            "http://www.foo.com/a/nice/path/sister?zot=23&zut",
            urlpath.sibling("sister").to_text(),
        )
        # Use an url without trailing '/' to check child removal.
        url_text = "http://www.foo.com/a/nice/path?zot=23&zut"
        urlpath = URL.from_text(url_text)
        self.assertEqual(
            "http://www.foo.com/a/nice/sister?zot=23&zut",
            urlpath.sibling("sister").to_text(),
        )

    def test_click(self):
        # type: () -> None
        """
        L{URL.click} interprets the given string as a relative URI-reference
        and returns a new L{URL} interpreting C{self} as the base absolute URI.
        """
        urlpath = URL.from_text(BASIC_URL)
        # A null uri should be valid (return here).
        self.assertEqual(
            "http://www.foo.com/a/nice/path/?zot=23&zut",
            urlpath.click("").to_text(),
        )
        # A simple relative path remove the query.
        self.assertEqual(
            "http://www.foo.com/a/nice/path/click",
            urlpath.click("click").to_text(),
        )
        # An absolute path replace path and query.
        self.assertEqual(
            "http://www.foo.com/click", urlpath.click("/click").to_text()
        )
        # Replace just the query.
        self.assertEqual(
            "http://www.foo.com/a/nice/path/?burp",
            urlpath.click("?burp").to_text(),
        )
        # One full url to another should not generate '//' between authority.
        # and path
        self.assertTrue(
            "//foobar"
            not in urlpath.click("http://www.foo.com/foobar").to_text()
        )

        # From a url with no query clicking a url with a query, the query
        # should be handled properly.
        u = URL.from_text("http://www.foo.com/me/noquery")
        self.assertEqual(
            "http://www.foo.com/me/17?spam=158",
            u.click("/me/17?spam=158").to_text(),
        )

        # Check that everything from the path onward is removed when the click
        # link has no path.
        u = URL.from_text("http://localhost/foo?abc=def")
        self.assertEqual(
            u.click("http://www.python.org").to_text(), "http://www.python.org"
        )

        # https://twistedmatrix.com/trac/ticket/8184
        u = URL.from_text("http://hatnote.com/a/b/../c/./d/e/..")
        res = "http://hatnote.com/a/c/d/"
        self.assertEqual(u.click("").to_text(), res)

        # test click default arg is same as empty string above
        self.assertEqual(u.click().to_text(), res)

        # test click on a URL instance
        u = URL.fromText("http://localhost/foo/?abc=def")
        u2 = URL.from_text("bar")
        u3 = u.click(u2)
        self.assertEqual(u3.to_text(), "http://localhost/foo/bar")

    def test_clickRFC3986(self):
        # type: () -> None
        """
        L{URL.click} should correctly resolve the examples in RFC 3986.
        """
        base = URL.from_text(relativeLinkBaseForRFC3986)
        for (ref, expected) in relativeLinkTestsForRFC3986:
            self.assertEqual(base.click(ref).to_text(), expected)

    def test_clickSchemeRelPath(self):
        # type: () -> None
        """
        L{URL.click} should not accept schemes with relative paths.
        """
        base = URL.from_text(relativeLinkBaseForRFC3986)
        self.assertRaises(NotImplementedError, base.click, "g:h")
        self.assertRaises(NotImplementedError, base.click, "http:h")

    def test_cloneUnchanged(self):
        # type: () -> None
        """
        Verify that L{URL.replace} doesn't change any of the arguments it
        is passed.
        """
        urlpath = URL.from_text("https://x:1/y?z=1#A")
        self.assertEqual(
            urlpath.replace(
                urlpath.scheme,
                urlpath.host,
                urlpath.path,
                urlpath.query,
                urlpath.fragment,
                urlpath.port,
            ),
            urlpath,
        )
        self.assertEqual(urlpath.replace(), urlpath)

    def test_clickCollapse(self):
        # type: () -> None
        """
        L{URL.click} collapses C{.} and C{..} according to RFC 3986 section
        5.2.4.
        """
        tests = [
            ["http://localhost/", ".", "http://localhost/"],
            ["http://localhost/", "..", "http://localhost/"],
            ["http://localhost/a/b/c", ".", "http://localhost/a/b/"],
            ["http://localhost/a/b/c", "..", "http://localhost/a/"],
            ["http://localhost/a/b/c", "./d/e", "http://localhost/a/b/d/e"],
            ["http://localhost/a/b/c", "../d/e", "http://localhost/a/d/e"],
            ["http://localhost/a/b/c", "/./d/e", "http://localhost/d/e"],
            ["http://localhost/a/b/c", "/../d/e", "http://localhost/d/e"],
            [
                "http://localhost/a/b/c/",
                "../../d/e/",
                "http://localhost/a/d/e/",
            ],
            ["http://localhost/a/./c", "../d/e", "http://localhost/d/e"],
            ["http://localhost/a/./c/", "../d/e", "http://localhost/a/d/e"],
            [
                "http://localhost/a/b/c/d",
                "./e/../f/../g",
                "http://localhost/a/b/c/g",
            ],
            ["http://localhost/a/b/c", "d//e", "http://localhost/a/b/d//e"],
        ]
        for start, click, expected in tests:
            actual = URL.from_text(start).click(click).to_text()
            self.assertEqual(
                actual,
                expected,
                "{start}.click({click}) => {actual} not {expected}".format(
                    start=start,
                    click=repr(click),
                    actual=actual,
                    expected=expected,
                ),
            )

    def test_queryAdd(self):
        # type: () -> None
        """
        L{URL.add} adds query parameters.
        """
        self.assertEqual(
            "http://www.foo.com/a/nice/path/?foo=bar",
            URL.from_text("http://www.foo.com/a/nice/path/")
            .add("foo", "bar")
            .to_text(),
        )
        self.assertEqual(
            "http://www.foo.com/?foo=bar",
            URL(host="www.foo.com").add("foo", "bar").to_text(),
        )
        urlpath = URL.from_text(BASIC_URL)
        self.assertEqual(
            "http://www.foo.com/a/nice/path/?zot=23&zut&burp",
            urlpath.add("burp").to_text(),
        )
        self.assertEqual(
            "http://www.foo.com/a/nice/path/?zot=23&zut&burp=xxx",
            urlpath.add("burp", "xxx").to_text(),
        )
        self.assertEqual(
            "http://www.foo.com/a/nice/path/?zot=23&zut&burp=xxx&zing",
            urlpath.add("burp", "xxx").add("zing").to_text(),
        )
        # Note the inversion!
        self.assertEqual(
            "http://www.foo.com/a/nice/path/?zot=23&zut&zing&burp=xxx",
            urlpath.add("zing").add("burp", "xxx").to_text(),
        )
        # Note the two values for the same name.
        self.assertEqual(
            "http://www.foo.com/a/nice/path/?zot=23&zut&burp=xxx&zot=32",
            urlpath.add("burp", "xxx").add("zot", "32").to_text(),
        )

    def test_querySet(self):
        # type: () -> None
        """
        L{URL.set} replaces query parameters by name.
        """
        urlpath = URL.from_text(BASIC_URL)
        self.assertEqual(
            "http://www.foo.com/a/nice/path/?zot=32&zut",
            urlpath.set("zot", "32").to_text(),
        )
        # Replace name without value with name/value and vice-versa.
        self.assertEqual(
            "http://www.foo.com/a/nice/path/?zot&zut=itworked",
            urlpath.set("zot").set("zut", "itworked").to_text(),
        )
        # Q: what happens when the query has two values and we replace?
        # A: we replace both values with a single one
        self.assertEqual(
            "http://www.foo.com/a/nice/path/?zot=32&zut",
            urlpath.add("zot", "xxx").set("zot", "32").to_text(),
        )

    def test_queryRemove(self):
        # type: () -> None
        """
        L{URL.remove} removes instances of a query parameter.
        """
        url = URL.from_text("https://example.com/a/b/?foo=1&bar=2&foo=3")
        self.assertEqual(
            url.remove("foo"), URL.from_text("https://example.com/a/b/?bar=2")
        )

        self.assertEqual(
            url.remove(name="foo", value="1"),
            URL.from_text("https://example.com/a/b/?bar=2&foo=3"),
        )

        self.assertEqual(
            url.remove(name="foo", limit=1),
            URL.from_text("https://example.com/a/b/?bar=2&foo=3"),
        )

        self.assertEqual(
            url.remove(name="foo", value="1", limit=0),
            URL.from_text("https://example.com/a/b/?foo=1&bar=2&foo=3"),
        )

    def test_parseEqualSignInParamValue(self):
        # type: () -> None
        """
        Every C{=}-sign after the first in a query parameter is simply included
        in the value of the parameter.
        """
        u = URL.from_text("http://localhost/?=x=x=x")
        self.assertEqual(u.get(""), ["x=x=x"])
        self.assertEqual(u.to_text(), "http://localhost/?=x=x=x")
        u = URL.from_text("http://localhost/?foo=x=x=x&bar=y")
        self.assertEqual(u.query, (("foo", "x=x=x"), ("bar", "y")))
        self.assertEqual(u.to_text(), "http://localhost/?foo=x=x=x&bar=y")

        u = URL.from_text(
            "https://example.com/?argument=3&argument=4&operator=%3D"
        )
        iri = u.to_iri()
        self.assertEqual(iri.get("operator"), ["="])
        # assert that the equals is not unnecessarily escaped
        self.assertEqual(iri.to_uri().get("operator"), ["="])

    def test_empty(self):
        # type: () -> None
        """
        An empty L{URL} should serialize as the empty string.
        """
        self.assertEqual(URL().to_text(), "")

    def test_justQueryText(self):
        # type: () -> None
        """
        An L{URL} with query text should serialize as just query text.
        """
        u = URL(query=[("hello", "world")])
        self.assertEqual(u.to_text(), "?hello=world")

    def test_identicalEqual(self):
        # type: () -> None
        """
        L{URL} compares equal to itself.
        """
        u = URL.from_text("http://localhost/")
        self.assertEqual(u, u)

    def test_similarEqual(self):
        # type: () -> None
        """
        URLs with equivalent components should compare equal.
        """
        u1 = URL.from_text("http://u@localhost:8080/p/a/t/h?q=p#f")
        u2 = URL.from_text("http://u@localhost:8080/p/a/t/h?q=p#f")
        self.assertEqual(u1, u2)

    def test_differentNotEqual(self):
        # type: () -> None
        """
        L{URL}s that refer to different resources are both unequal (C{!=}) and
        also not equal (not C{==}).
        """
        u1 = URL.from_text("http://localhost/a")
        u2 = URL.from_text("http://localhost/b")
        self.assertFalse(u1 == u2, "%r != %r" % (u1, u2))
        self.assertNotEqual(u1, u2)

    def test_otherTypesNotEqual(self):
        # type: () -> None
        """
        L{URL} is not equal (C{==}) to other types.
        """
        u = URL.from_text("http://localhost/")
        self.assertFalse(u == 42, "URL must not equal a number.")
        self.assertFalse(u == object(), "URL must not equal an object.")
        self.assertNotEqual(u, 42)
        self.assertNotEqual(u, object())

    def test_identicalNotUnequal(self):
        # type: () -> None
        """
        Identical L{URL}s are not unequal (C{!=}) to each other.
        """
        u = URL.from_text("http://u@localhost:8080/p/a/t/h?q=p#f")
        self.assertFalse(u != u, "%r == itself" % u)

    def test_similarNotUnequal(self):
        # type: () -> None
        """
        Structurally similar L{URL}s are not unequal (C{!=}) to each other.
        """
        u1 = URL.from_text("http://u@localhost:8080/p/a/t/h?q=p#f")
        u2 = URL.from_text("http://u@localhost:8080/p/a/t/h?q=p#f")
        self.assertFalse(u1 != u2, "%r == %r" % (u1, u2))

    def test_differentUnequal(self):
        # type: () -> None
        """
        Structurally different L{URL}s are unequal (C{!=}) to each other.
        """
        u1 = URL.from_text("http://localhost/a")
        u2 = URL.from_text("http://localhost/b")
        self.assertTrue(u1 != u2, "%r == %r" % (u1, u2))

    def test_otherTypesUnequal(self):
        # type: () -> None
        """
        L{URL} is unequal (C{!=}) to other types.
        """
        u = URL.from_text("http://localhost/")
        self.assertTrue(u != 42, "URL must differ from a number.")
        self.assertTrue(u != object(), "URL must be differ from an object.")

    def test_asURI(self):
        # type: () -> None
        """
        L{URL.asURI} produces an URI which converts any URI unicode encoding
        into pure US-ASCII and returns a new L{URL}.
        """
        unicodey = (
            "http://\N{LATIN SMALL LETTER E WITH ACUTE}.com/"
            "\N{LATIN SMALL LETTER E}\N{COMBINING ACUTE ACCENT}"
            "?\N{LATIN SMALL LETTER A}\N{COMBINING ACUTE ACCENT}="
            "\N{LATIN SMALL LETTER I}\N{COMBINING ACUTE ACCENT}"
            "#\N{LATIN SMALL LETTER U}\N{COMBINING ACUTE ACCENT}"
        )
        iri = URL.from_text(unicodey)
        uri = iri.asURI()
        self.assertEqual(iri.host, "\N{LATIN SMALL LETTER E WITH ACUTE}.com")
        self.assertEqual(
            iri.path[0], "\N{LATIN SMALL LETTER E}\N{COMBINING ACUTE ACCENT}"
        )
        self.assertEqual(iri.to_text(), unicodey)
        expectedURI = "http://xn--9ca.com/%C3%A9?%C3%A1=%C3%AD#%C3%BA"
        actualURI = uri.to_text()
        self.assertEqual(
            actualURI, expectedURI, "%r != %r" % (actualURI, expectedURI)
        )

    def test_asIRI(self):
        # type: () -> None
        """
        L{URL.asIRI} decodes any percent-encoded text in the URI, making it
        more suitable for reading by humans, and returns a new L{URL}.
        """
        asciiish = "http://xn--9ca.com/%C3%A9?%C3%A1=%C3%AD#%C3%BA"
        uri = URL.from_text(asciiish)
        iri = uri.asIRI()
        self.assertEqual(uri.host, "xn--9ca.com")
        self.assertEqual(uri.path[0], "%C3%A9")
        self.assertEqual(uri.to_text(), asciiish)
        expectedIRI = (
            "http://\N{LATIN SMALL LETTER E WITH ACUTE}.com/"
            "\N{LATIN SMALL LETTER E WITH ACUTE}"
            "?\N{LATIN SMALL LETTER A WITH ACUTE}="
            "\N{LATIN SMALL LETTER I WITH ACUTE}"
            "#\N{LATIN SMALL LETTER U WITH ACUTE}"
        )
        actualIRI = iri.to_text()
        self.assertEqual(
            actualIRI, expectedIRI, "%r != %r" % (actualIRI, expectedIRI)
        )

    def test_badUTF8AsIRI(self):
        # type: () -> None
        """
        Bad UTF-8 in a path segment, query parameter, or fragment results in
        that portion of the URI remaining percent-encoded in the IRI.
        """
        urlWithBinary = "http://xn--9ca.com/%00%FF/%C3%A9"
        uri = URL.from_text(urlWithBinary)
        iri = uri.asIRI()
        expectedIRI = (
            "http://\N{LATIN SMALL LETTER E WITH ACUTE}.com/"
            "%00%FF/"
            "\N{LATIN SMALL LETTER E WITH ACUTE}"
        )
        actualIRI = iri.to_text()
        self.assertEqual(
            actualIRI, expectedIRI, "%r != %r" % (actualIRI, expectedIRI)
        )

    def test_alreadyIRIAsIRI(self):
        # type: () -> None
        """
        A L{URL} composed of non-ASCII text will result in non-ASCII text.
        """
        unicodey = (
            "http://\N{LATIN SMALL LETTER E WITH ACUTE}.com/"
            "\N{LATIN SMALL LETTER E}\N{COMBINING ACUTE ACCENT}"
            "?\N{LATIN SMALL LETTER A}\N{COMBINING ACUTE ACCENT}="
            "\N{LATIN SMALL LETTER I}\N{COMBINING ACUTE ACCENT}"
            "#\N{LATIN SMALL LETTER U}\N{COMBINING ACUTE ACCENT}"
        )
        iri = URL.from_text(unicodey)
        alsoIRI = iri.asIRI()
        self.assertEqual(alsoIRI.to_text(), unicodey)

    def test_alreadyURIAsURI(self):
        # type: () -> None
        """
        A L{URL} composed of encoded text will remain encoded.
        """
        expectedURI = "http://xn--9ca.com/%C3%A9?%C3%A1=%C3%AD#%C3%BA"
        uri = URL.from_text(expectedURI)
        actualURI = uri.asURI().to_text()
        self.assertEqual(actualURI, expectedURI)

    def test_userinfo(self):
        # type: () -> None
        """
        L{URL.from_text} will parse the C{userinfo} portion of the URI
        separately from the host and port.
        """
        url = URL.from_text(
            "http://someuser:somepassword@example.com/some-segment@ignore"
        )
        self.assertEqual(
            url.authority(True), "someuser:somepassword@example.com"
        )
        self.assertEqual(url.authority(False), "someuser:@example.com")
        self.assertEqual(url.userinfo, "someuser:somepassword")
        self.assertEqual(url.user, "someuser")
        self.assertEqual(
            url.to_text(), "http://someuser:@example.com/some-segment@ignore"
        )
        self.assertEqual(
            url.replace(userinfo="someuser").to_text(),
            "http://someuser@example.com/some-segment@ignore",
        )

    def test_portText(self):
        # type: () -> None
        """
        L{URL.from_text} parses custom port numbers as integers.
        """
        portURL = URL.from_text("http://www.example.com:8080/")
        self.assertEqual(portURL.port, 8080)
        self.assertEqual(portURL.to_text(), "http://www.example.com:8080/")

    def test_mailto(self):
        # type: () -> None
        """
        Although L{URL} instances are mainly for dealing with HTTP, other
        schemes (such as C{mailto:}) should work as well.  For example,
        L{URL.from_text}/L{URL.to_text} round-trips cleanly for a C{mailto:}
        URL representing an email address.
        """
        self.assertEqual(
            URL.from_text("mailto:user@example.com").to_text(),
            "mailto:user@example.com",
        )

    def test_httpWithoutHost(self):
        # type: () -> None
        """
        An HTTP URL without a hostname, but with a path, should also round-trip
        cleanly.
        """
        without_host = URL.from_text("http:relative-path")
        self.assertEqual(without_host.host, "")
        self.assertEqual(without_host.path, ("relative-path",))
        self.assertEqual(without_host.uses_netloc, False)
        self.assertEqual(without_host.to_text(), "http:relative-path")

    def test_queryIterable(self):
        # type: () -> None
        """
        When a L{URL} is created with a C{query} argument, the C{query}
        argument is converted into an N-tuple of 2-tuples, sensibly
        handling dictionaries.
        """
        expected = (("alpha", "beta"),)
        url = URL(query=[("alpha", "beta")])
        self.assertEqual(url.query, expected)
        url = URL(query={"alpha": "beta"})
        self.assertEqual(url.query, expected)

    def test_pathIterable(self):
        # type: () -> None
        """
        When a L{URL} is created with a C{path} argument, the C{path} is
        converted into a tuple.
        """
        url = URL(path=["hello", "world"])
        self.assertEqual(url.path, ("hello", "world"))

    def test_invalidArguments(self):
        # type: () -> None
        """
        Passing an argument of the wrong type to any of the constructor
        arguments of L{URL} will raise a descriptive L{TypeError}.

        L{URL} typechecks very aggressively to ensure that its constitutent
        parts are all properly immutable and to prevent confusing errors when
        bad data crops up in a method call long after the code that called the
        constructor is off the stack.
        """

        class Unexpected(object):
            def __str__(self):
                # type: () -> str
                return "wrong"

            def __repr__(self):
                # type: () -> str
                return "<unexpected>"

        defaultExpectation = "unicode" if bytes is str else "str"

        def assertRaised(raised, expectation, name):
            # type: (Any, Text, Text) -> None
            self.assertEqual(
                str(raised.exception),
                "expected {0} for {1}, got {2}".format(
                    expectation, name, "<unexpected>"
                ),
            )

        def check(param, expectation=defaultExpectation):
            # type: (Any, str) -> None
            with self.assertRaises(TypeError) as raised:
                URL(**{param: Unexpected()})  # type: ignore[arg-type]

            assertRaised(raised, expectation, param)

        check("scheme")
        check("host")
        check("fragment")
        check("rooted", "bool")
        check("userinfo")
        check("port", "int or NoneType")

        with self.assertRaises(TypeError) as raised:
            URL(path=[cast(Text, Unexpected())])

        assertRaised(raised, defaultExpectation, "path segment")

        with self.assertRaises(TypeError) as raised:
            URL(query=[("name", cast(Text, Unexpected()))])

        assertRaised(
            raised, defaultExpectation + " or NoneType", "query parameter value"
        )

        with self.assertRaises(TypeError) as raised:
            URL(query=[(cast(Text, Unexpected()), "value")])

        assertRaised(raised, defaultExpectation, "query parameter name")
        # No custom error message for this one, just want to make sure
        # non-2-tuples don't get through.

        with self.assertRaises(TypeError):
            URL(query=[cast(Tuple[Text, Text], Unexpected())])

        with self.assertRaises(ValueError):
            URL(query=[cast(Tuple[Text, Text], ("k", "v", "vv"))])

        with self.assertRaises(ValueError):
            URL(query=[cast(Tuple[Text, Text], ("k",))])

        url = URL.from_text("https://valid.example.com/")
        with self.assertRaises(TypeError) as raised:
            url.child(cast(Text, Unexpected()))
        assertRaised(raised, defaultExpectation, "path segment")
        with self.assertRaises(TypeError) as raised:
            url.sibling(cast(Text, Unexpected()))
        assertRaised(raised, defaultExpectation, "path segment")
        with self.assertRaises(TypeError) as raised:
            url.click(cast(Text, Unexpected()))
        assertRaised(raised, defaultExpectation, "relative URL")

    def test_technicallyTextIsIterableBut(self):
        # type: () -> None
        """
        Technically, L{str} (or L{unicode}, as appropriate) is iterable, but
        C{URL(path="foo")} resulting in C{URL.from_text("f/o/o")} is never what
        you want.
        """
        with self.assertRaises(TypeError) as raised:
            URL(path="foo")
        self.assertEqual(
            str(raised.exception),
            "expected iterable of text for path, not: {0}".format(repr("foo")),
        )

    def test_netloc(self):
        # type: () -> None
        url = URL(scheme="https")
        self.assertEqual(url.uses_netloc, True)
        self.assertEqual(url.to_text(), "https://")
        # scheme, no host, no path, no netloc hack
        self.assertEqual(URL.from_text("https:").uses_netloc, False)
        # scheme, no host, absolute path, no netloc hack
        self.assertEqual(URL.from_text("https:/").uses_netloc, False)
        # scheme, no host, no path, netloc hack to indicate :// syntax
        self.assertEqual(URL.from_text("https://").uses_netloc, True)

        url = URL(scheme="https", uses_netloc=False)
        self.assertEqual(url.uses_netloc, False)
        self.assertEqual(url.to_text(), "https:")

        url = URL(scheme="git+https")
        self.assertEqual(url.uses_netloc, True)
        self.assertEqual(url.to_text(), "git+https://")

        url = URL(scheme="mailto")
        self.assertEqual(url.uses_netloc, False)
        self.assertEqual(url.to_text(), "mailto:")

        url = URL(scheme="ztp")
        self.assertEqual(url.uses_netloc, None)
        self.assertEqual(url.to_text(), "ztp:")

        url = URL.from_text("ztp://test.com")
        self.assertEqual(url.uses_netloc, True)

        url = URL.from_text("ztp:test:com")
        self.assertEqual(url.uses_netloc, False)

    def test_ipv6_with_port(self):
        # type: () -> None
        t = "https://[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:80/"
        url = URL.from_text(t)
        assert url.host == "2001:0db8:85a3:0000:0000:8a2e:0370:7334"
        assert url.port == 80
        assert SCHEME_PORT_MAP[url.scheme] != url.port

    def test_basic(self):
        # type: () -> None
        text = "https://user:pass@example.com/path/to/here?k=v#nice"
        url = URL.from_text(text)
        assert url.scheme == "https"
        assert url.userinfo == "user:pass"
        assert url.host == "example.com"
        assert url.path == ("path", "to", "here")
        assert url.fragment == "nice"

        text = "https://user:pass@127.0.0.1/path/to/here?k=v#nice"
        url = URL.from_text(text)
        assert url.scheme == "https"
        assert url.userinfo == "user:pass"
        assert url.host == "127.0.0.1"
        assert url.path == ("path", "to", "here")

        text = "https://user:pass@[::1]/path/to/here?k=v#nice"
        url = URL.from_text(text)
        assert url.scheme == "https"
        assert url.userinfo == "user:pass"
        assert url.host == "::1"
        assert url.path == ("path", "to", "here")

    def test_invalid_url(self):
        # type: () -> None
        self.assertRaises(URLParseError, URL.from_text, "#\n\n")

    def test_invalid_authority_url(self):
        # type: () -> None
        self.assertRaises(URLParseError, URL.from_text, "http://abc:\n\n/#")

    def test_invalid_ipv6(self):
        # type: () -> None
        invalid_ipv6_ips = [
            "2001::0234:C1ab::A0:aabc:003F",
            "2001::1::3F",
            ":",
            "::::",
            "::256.0.0.1",
        ]
        for ip in invalid_ipv6_ips:
            url_text = "http://[" + ip + "]"
            self.assertRaises(socket.error, inet_pton, socket.AF_INET6, ip)
            self.assertRaises(URLParseError, URL.from_text, url_text)

    def test_invalid_port(self):
        # type: () -> None
        self.assertRaises(URLParseError, URL.from_text, "ftp://portmouth:smash")
        self.assertRaises(
            ValueError,
            URL.from_text,
            "http://reader.googlewebsite.com:neverforget",
        )

    def test_idna(self):
        # type: () -> None
        u1 = URL.from_text("http://bücher.ch")
        self.assertEqual(u1.host, "bücher.ch")
        self.assertEqual(u1.to_text(), "http://bücher.ch")
        self.assertEqual(u1.to_uri().to_text(), "http://xn--bcher-kva.ch")

        u2 = URL.from_text("https://xn--bcher-kva.ch")
        self.assertEqual(u2.host, "xn--bcher-kva.ch")
        self.assertEqual(u2.to_text(), "https://xn--bcher-kva.ch")
        self.assertEqual(u2.to_iri().to_text(), "https://bücher.ch")

    def test_netloc_slashes(self):
        # type: () -> None

        # basic sanity checks
        url = URL.from_text("mailto:mahmoud@hatnote.com")
        self.assertEqual(url.scheme, "mailto")
        self.assertEqual(url.to_text(), "mailto:mahmoud@hatnote.com")

        url = URL.from_text("http://hatnote.com")
        self.assertEqual(url.scheme, "http")
        self.assertEqual(url.to_text(), "http://hatnote.com")

        # test that unrecognized schemes stay consistent with '//'
        url = URL.from_text("newscheme:a:b:c")
        self.assertEqual(url.scheme, "newscheme")
        self.assertEqual(url.to_text(), "newscheme:a:b:c")

        url = URL.from_text("newerscheme://a/b/c")
        self.assertEqual(url.scheme, "newerscheme")
        self.assertEqual(url.to_text(), "newerscheme://a/b/c")

        # test that reasonable guesses are made
        url = URL.from_text("git+ftp://gitstub.biz/glyph/lefkowitz")
        self.assertEqual(url.scheme, "git+ftp")
        self.assertEqual(url.to_text(), "git+ftp://gitstub.biz/glyph/lefkowitz")

        url = URL.from_text("what+mailto:freerealestate@enotuniq.org")
        self.assertEqual(url.scheme, "what+mailto")
        self.assertEqual(
            url.to_text(), "what+mailto:freerealestate@enotuniq.org"
        )

        url = URL(scheme="ztp", path=("x", "y", "z"), rooted=True)
        self.assertEqual(url.to_text(), "ztp:/x/y/z")

        # also works when the input doesn't include '//'
        url = URL(
            scheme="git+ftp",
            path=("x", "y", "z", ""),
            rooted=True,
            uses_netloc=True,
        )
        # broken bc urlunsplit
        self.assertEqual(url.to_text(), "git+ftp:///x/y/z/")

        # really why would this ever come up but ok
        url = URL.from_text("file:///path/to/heck")
        url2 = url.replace(scheme="mailto")
        self.assertEqual(url2.to_text(), "mailto:/path/to/heck")

        url_text = "unregisteredscheme:///a/b/c"
        url = URL.from_text(url_text)
        no_netloc_url = url.replace(uses_netloc=False)
        self.assertEqual(no_netloc_url.to_text(), "unregisteredscheme:/a/b/c")
        netloc_url = url.replace(uses_netloc=True)
        self.assertEqual(netloc_url.to_text(), url_text)

        return

    def test_rooted_to_relative(self):
        # type: () -> None
        """
        On host-relative URLs, the C{rooted} flag can be updated to indicate
        that the path should no longer be treated as absolute.
        """
        a = URL(path=["hello"])
        self.assertEqual(a.to_text(), "hello")
        b = a.replace(rooted=True)
        self.assertEqual(b.to_text(), "/hello")
        self.assertNotEqual(a, b)

    def test_autorooted(self):
        # type: () -> None
        """
        The C{rooted} flag can be updated in some cases, but it cannot be made
        to conflict with other facts surrounding the URL; for example, all URLs
        involving an authority (host) are inherently rooted because it is not
        syntactically possible to express otherwise; also, once an unrooted URL
        gains a path that starts with an empty string, that empty string is
        elided and it becomes rooted, because these cases are syntactically
        indistinguisable in real URL text.
        """
        relative_path_rooted = URL(path=["", "foo"], rooted=False)
        self.assertEqual(relative_path_rooted.rooted, True)
        relative_flag_rooted = URL(path=["foo"], rooted=True)
        self.assertEqual(relative_flag_rooted.rooted, True)
        self.assertEqual(relative_path_rooted, relative_flag_rooted)

        attempt_unrooted_absolute = URL(host="foo", path=["bar"], rooted=False)
        normal_absolute = URL(host="foo", path=["bar"])
        self.assertEqual(attempt_unrooted_absolute, normal_absolute)
        self.assertEqual(normal_absolute.rooted, True)
        self.assertEqual(attempt_unrooted_absolute.rooted, True)

    def test_rooted_with_port_but_no_host(self):
        # type: () -> None
        """
        URLs which include a ``://`` netloc-separator for any reason are
        inherently rooted, regardless of the value or presence of the
        ``rooted`` constructor argument.

        They may include a netloc-separator because their constructor was
        directly invoked with an explicit host or port, or because they were
        parsed from a string which included the literal ``://`` separator.
        """
        directly_constructed = URL(scheme="udp", port=4900, rooted=False)
        directly_constructed_implict = URL(scheme="udp", port=4900)
        directly_constructed_rooted = URL(scheme="udp", port=4900, rooted=True)
        self.assertEqual(directly_constructed.rooted, True)
        self.assertEqual(directly_constructed_implict.rooted, True)
        self.assertEqual(directly_constructed_rooted.rooted, True)
        parsed = URL.from_text("udp://:4900")
        self.assertEqual(str(directly_constructed), str(parsed))
        self.assertEqual(str(directly_constructed_implict), str(parsed))
        self.assertEqual(directly_constructed.asText(), parsed.asText())
        self.assertEqual(directly_constructed, parsed)
        self.assertEqual(directly_constructed, directly_constructed_implict)
        self.assertEqual(directly_constructed, directly_constructed_rooted)
        self.assertEqual(directly_constructed_implict, parsed)
        self.assertEqual(directly_constructed_rooted, parsed)

    def test_wrong_constructor(self):
        # type: () -> None
        with self.assertRaises(ValueError):
            # whole URL not allowed
            URL(BASIC_URL)
        with self.assertRaises(ValueError):
            # explicitly bad scheme not allowed
            URL("HTTP_____more_like_imHoTTeP")

    def test_encoded_userinfo(self):
        # type: () -> None
        url = URL.from_text("http://user:pass@example.com")
        assert url.userinfo == "user:pass"
        url = url.replace(userinfo="us%20her:pass")
        iri = url.to_iri()
        assert (
            iri.to_text(with_password=True) == "http://us her:pass@example.com"
        )
        assert iri.to_text(with_password=False) == "http://us her:@example.com"
        assert (
            iri.to_uri().to_text(with_password=True)
            == "http://us%20her:pass@example.com"
        )

    def test_hash(self):
        # type: () -> None
        url_map = {}
        url1 = URL.from_text("http://blog.hatnote.com/ask?utm_source=geocity")
        assert hash(url1) == hash(url1)  # sanity

        url_map[url1] = 1

        url2 = URL.from_text("http://blog.hatnote.com/ask")
        url2 = url2.set("utm_source", "geocity")

        url_map[url2] = 2

        assert len(url_map) == 1
        assert list(url_map.values()) == [2]

        assert hash(URL()) == hash(URL())  # slightly more sanity

    def test_dir(self):
        # type: () -> None
        url = URL()
        res = dir(url)

        assert len(res) > 15
        # twisted compat
        assert "fromText" not in res
        assert "asText" not in res
        assert "asURI" not in res
        assert "asIRI" not in res

    def test_twisted_compat(self):
        # type: () -> None
        url = URL.fromText("http://example.com/a%20té%C3%A9st")
        assert url.asText() == "http://example.com/a%20té%C3%A9st"
        assert url.asURI().asText() == "http://example.com/a%20t%C3%A9%C3%A9st"
        # TODO: assert url.asIRI().asText() == u'http://example.com/a%20téést'

    def test_set_ordering(self):
        # type: () -> None

        # TODO
        url = URL.from_text("http://example.com/?a=b&c")
        url = url.set("x", "x")
        url = url.add("x", "y")
        assert url.to_text() == "http://example.com/?a=b&x=x&c&x=y"
        # Would expect:
        # assert url.to_text() == u'http://example.com/?a=b&c&x=x&x=y'

    def test_schemeless_path(self):
        # type: () -> None
        "See issue #4"
        u1 = URL.from_text("urn%3Aietf%3Awg%3Aoauth%3A2.0%3Aoob")
        u2 = URL.from_text(u1.to_text())
        assert u1 == u2  # sanity testing roundtripping

        u3 = URL.from_text(u1.to_iri().to_text())
        assert u1 == u3
        assert u2 == u3

        # test that colons are ok past the first segment
        u4 = URL.from_text("first-segment/urn%3Aietf%3Awg%3Aoauth%3A2.0%3Aoob")
        u5 = u4.to_iri()
        assert u5.to_text() == "first-segment/urn:ietf:wg:oauth:2.0:oob"

        u6 = URL.from_text(u5.to_text()).to_uri()
        assert u5 == u6  # colons stay decoded bc they're not in the first seg

    def test_emoji_domain(self):
        # type: () -> None
        "See issue #7, affecting only narrow builds (2.6-3.3)"
        url = URL.from_text("https://xn--vi8hiv.ws")
        iri = url.to_iri()
        iri.to_text()
        # as long as we don't get ValueErrors, we're good

    def test_delim_in_param(self):
        # type: () -> None
        "Per issue #6 and #8"
        self.assertRaises(ValueError, URL, scheme="http", host="a/c")
        self.assertRaises(ValueError, URL, path=("?",))
        self.assertRaises(ValueError, URL, path=("#",))
        self.assertRaises(ValueError, URL, query=(("&", "test")))

    def test_empty_paths_eq(self):
        # type: () -> None
        u1 = URL.from_text("http://example.com/")
        u2 = URL.from_text("http://example.com")

        assert u1 == u2

        u1 = URL.from_text("http://example.com")
        u2 = URL.from_text("http://example.com")

        assert u1 == u2

        u1 = URL.from_text("http://example.com")
        u2 = URL.from_text("http://example.com/")

        assert u1 == u2

        u1 = URL.from_text("http://example.com/")
        u2 = URL.from_text("http://example.com/")

        assert u1 == u2

    def test_from_text_type(self):
        # type: () -> None
        assert URL.from_text("#ok").fragment == "ok"  # sanity
        self.assertRaises(TypeError, URL.from_text, b"bytes://x.y.z")
        self.assertRaises(TypeError, URL.from_text, object())

    def test_from_text_bad_authority(self):
        # type: () -> None

        # bad ipv6 brackets
        self.assertRaises(URLParseError, URL.from_text, "http://[::1/")
        self.assertRaises(URLParseError, URL.from_text, "http://::1]/")
        self.assertRaises(URLParseError, URL.from_text, "http://[[::1]/")
        self.assertRaises(URLParseError, URL.from_text, "http://[::1]]/")

        # empty port
        self.assertRaises(URLParseError, URL.from_text, "http://127.0.0.1:")
        # non-integer port
        self.assertRaises(URLParseError, URL.from_text, "http://127.0.0.1:hi")
        # extra port colon (makes for an invalid host)
        self.assertRaises(URLParseError, URL.from_text, "http://127.0.0.1::80")

    def test_normalize(self):
        # type: () -> None
        url = URL.from_text("HTTP://Example.com/A%61/./../A%61?B%62=C%63#D%64")
        assert url.get("Bb") == []
        assert url.get("B%62") == ["C%63"]
        assert len(url.path) == 4

        # test that most expected normalizations happen
        norm_url = url.normalize()

        assert norm_url.scheme == "http"
        assert norm_url.host == "example.com"
        assert norm_url.path == ("Aa",)
        assert norm_url.get("Bb") == ["Cc"]
        assert norm_url.fragment == "Dd"
        assert norm_url.to_text() == "http://example.com/Aa?Bb=Cc#Dd"

        # test that flags work
        noop_norm_url = url.normalize(
            scheme=False, host=False, path=False, query=False, fragment=False
        )
        assert noop_norm_url == url

        # test that empty paths get at least one slash
        slashless_url = URL.from_text("http://example.io")
        slashful_url = slashless_url.normalize()
        assert slashful_url.to_text() == "http://example.io/"

        # test case normalization for percent encoding
        delimited_url = URL.from_text("/a%2fb/cd%3f?k%3d=v%23#test")
        norm_delimited_url = delimited_url.normalize()
        assert norm_delimited_url.to_text() == "/a%2Fb/cd%3F?k%3D=v%23#test"

        # test invalid percent encoding during normalize
        assert (
            URL(path=("", "%te%sts")).normalize(percents=False).to_text()
            == "/%te%sts"
        )
        assert URL(path=("", "%te%sts")).normalize().to_text() == "/%25te%25sts"

        percenty_url = URL(
            scheme="ftp",
            path=["%%%", "%a%b"],
            query=[("%", "%%")],
            fragment="%",
            userinfo="%:%",
        )

        assert (
            percenty_url.to_text(with_password=True)
            == "ftp://%:%@/%%%/%a%b?%=%%#%"
        )
        assert (
            percenty_url.normalize().to_text(with_password=True)
            == "ftp://%25:%25@/%25%25%25/%25a%25b?%25=%25%25#%25"
        )

    def test_str(self):
        # type: () -> None

        # see also issue #49
        text = "http://example.com/á/y%20a%20y/?b=%25"
        url = URL.from_text(text)
        assert unicode(url) == text
        assert bytes(url) == b"http://example.com/%C3%A1/y%20a%20y/?b=%25"

        if PY2:
            assert isinstance(str(url), bytes)
            assert isinstance(unicode(url), unicode)
        else:
            assert isinstance(str(url), unicode)
            assert isinstance(bytes(url), bytes)

    def test_idna_corners(self):
        # type: () -> None
        url = URL.from_text("http://abé.com/")
        assert url.to_iri().host == "abé.com"
        assert url.to_uri().host == "xn--ab-cja.com"

        url = URL.from_text("http://ドメイン.テスト.co.jp#test")
        assert url.to_iri().host == "ドメイン.テスト.co.jp"
        assert url.to_uri().host == "xn--eckwd4c7c.xn--zckzah.co.jp"

        assert url.to_uri().get_decoded_url().host == "ドメイン.テスト.co.jp"

        text = "http://Example.com"
        assert (
            URL.from_text(text).to_uri().get_decoded_url().host == "example.com"
        )
