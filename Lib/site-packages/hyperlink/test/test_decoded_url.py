# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from typing import Dict, Union
from .. import DecodedURL, URL
from .._url import _percent_decode
from .common import HyperlinkTestCase

BASIC_URL = "http://example.com/#"
TOTAL_URL = (
    "https://%75%73%65%72:%00%00%00%00@xn--bcher-kva.ch:8080/"
    "a/nice%20nice/./path/?zot=23%25&zut#frég"
)


class TestURL(HyperlinkTestCase):
    def test_durl_basic(self):
        # type: () -> None
        bdurl = DecodedURL.from_text(BASIC_URL)
        assert bdurl.scheme == "http"
        assert bdurl.host == "example.com"
        assert bdurl.port == 80
        assert bdurl.path == ("",)
        assert bdurl.fragment == ""

        durl = DecodedURL.from_text(TOTAL_URL)

        assert durl.scheme == "https"
        assert durl.host == "bücher.ch"
        assert durl.port == 8080
        assert durl.path == ("a", "nice nice", ".", "path", "")
        assert durl.fragment == "frég"
        assert durl.get("zot") == ["23%"]

        assert durl.user == "user"
        assert durl.userinfo == ("user", "\0\0\0\0")

    def test_passthroughs(self):
        # type: () -> None

        # just basic tests for the methods that more or less pass straight
        # through to the underlying URL

        durl = DecodedURL.from_text(TOTAL_URL)
        assert durl.sibling("te%t").path[-1] == "te%t"
        assert durl.child("../test2%").path[-1] == "../test2%"
        assert durl.child() == durl
        assert durl.child() is durl
        assert durl.click("/").path[-1] == ""
        assert durl.user == "user"

        assert "." in durl.path
        assert "." not in durl.normalize().path

        assert durl.to_uri().fragment == "fr%C3%A9g"
        assert " " in durl.to_iri().path[1]

        assert durl.to_text(with_password=True) == TOTAL_URL

        assert durl.absolute
        assert durl.rooted

        assert durl == durl.encoded_url.get_decoded_url()

        durl2 = DecodedURL.from_text(TOTAL_URL, lazy=True)
        assert durl2 == durl2.encoded_url.get_decoded_url(lazy=True)

        assert (
            str(DecodedURL.from_text(BASIC_URL).child(" "))
            == "http://example.com/%20"
        )

        assert not (durl == 1)
        assert durl != 1

    def test_repr(self):
        # type: () -> None
        durl = DecodedURL.from_text(TOTAL_URL)
        assert repr(durl) == "DecodedURL(url=" + repr(durl._url) + ")"

    def test_query_manipulation(self):
        # type: () -> None
        durl = DecodedURL.from_text(TOTAL_URL)

        assert durl.get("zot") == ["23%"]
        durl = durl.add(" ", "space")
        assert durl.get(" ") == ["space"]
        durl = durl.set(" ", "spa%ed")
        assert durl.get(" ") == ["spa%ed"]

        durl = DecodedURL(url=durl.to_uri())
        assert durl.get(" ") == ["spa%ed"]
        durl = durl.remove(" ")
        assert durl.get(" ") == []

        durl = DecodedURL.from_text("/?%61rg=b&arg=c")
        assert durl.get("arg") == ["b", "c"]

        assert durl.set("arg", "d").get("arg") == ["d"]

        durl = DecodedURL.from_text(
            "https://example.com/a/b/?fóó=1&bar=2&fóó=3"
        )
        assert durl.remove("fóó") == DecodedURL.from_text(
            "https://example.com/a/b/?bar=2"
        )
        assert durl.remove("fóó", value="1") == DecodedURL.from_text(
            "https://example.com/a/b/?bar=2&fóó=3"
        )
        assert durl.remove("fóó", limit=1) == DecodedURL.from_text(
            "https://example.com/a/b/?bar=2&fóó=3"
        )
        assert durl.remove("fóó", value="1", limit=0) == DecodedURL.from_text(
            "https://example.com/a/b/?fóó=1&bar=2&fóó=3"
        )

    def test_equality_and_hashability(self):
        # type: () -> None
        durl = DecodedURL.from_text(TOTAL_URL)
        durl2 = DecodedURL.from_text(TOTAL_URL)
        burl = DecodedURL.from_text(BASIC_URL)
        durl_uri = durl.to_uri()

        assert durl == durl
        assert durl == durl2
        assert durl != burl
        assert durl is not None
        assert durl != durl._url

        AnyURL = Union[URL, DecodedURL]

        durl_map = {}  # type: Dict[AnyURL, AnyURL]
        durl_map[durl] = durl
        durl_map[durl2] = durl2

        assert len(durl_map) == 1

        durl_map[burl] = burl

        assert len(durl_map) == 2

        durl_map[durl_uri] = durl_uri

        assert len(durl_map) == 3

    def test_replace_roundtrip(self):
        # type: () -> None
        durl = DecodedURL.from_text(TOTAL_URL)

        durl2 = durl.replace(
            scheme=durl.scheme,
            host=durl.host,
            path=durl.path,
            query=durl.query,
            fragment=durl.fragment,
            port=durl.port,
            rooted=durl.rooted,
            userinfo=durl.userinfo,
            uses_netloc=durl.uses_netloc,
        )

        assert durl == durl2

    def test_replace_userinfo(self):
        # type: () -> None
        durl = DecodedURL.from_text(TOTAL_URL)
        with self.assertRaises(ValueError):
            durl.replace(
                userinfo=(  # type: ignore[arg-type]
                    "user",
                    "pw",
                    "thiswillcauseafailure",
                )
            )
        return

    def test_twisted_compat(self):
        # type: () -> None
        durl = DecodedURL.from_text(TOTAL_URL)

        assert durl == DecodedURL.fromText(TOTAL_URL)
        assert "to_text" in dir(durl)
        assert "asText" not in dir(durl)
        assert durl.to_text() == durl.asText()

    def test_percent_decode_mixed(self):
        # type: () -> None

        # See https://github.com/python-hyper/hyperlink/pull/59 for a
        # nice discussion of the possibilities
        assert _percent_decode("abcdé%C3%A9éfg") == "abcdéééfg"

        # still allow percent encoding in the case of an error
        assert _percent_decode("abcdé%C3éfg") == "abcdé%C3éfg"

        # ...unless explicitly told otherwise
        with self.assertRaises(UnicodeDecodeError):
            _percent_decode("abcdé%C3éfg", raise_subencoding_exc=True)

        # when not encodable as subencoding
        assert _percent_decode("é%25é", subencoding="ascii") == "é%25é"

    def test_click_decoded_url(self):
        # type: () -> None
        durl = DecodedURL.from_text(TOTAL_URL)
        durl_dest = DecodedURL.from_text("/tëst")

        clicked = durl.click(durl_dest)
        assert clicked.host == durl.host
        assert clicked.path == durl_dest.path
        assert clicked.path == ("tëst",)
