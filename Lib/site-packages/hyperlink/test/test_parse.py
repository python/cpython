# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from .common import HyperlinkTestCase
from hyperlink import parse, EncodedURL, DecodedURL

BASIC_URL = "http://example.com/#"
TOTAL_URL = (
    "https://%75%73%65%72:%00%00%00%00@xn--bcher-kva.ch:8080"
    "/a/nice%20nice/./path/?zot=23%25&zut#frég"
)
UNDECODABLE_FRAG_URL = TOTAL_URL + "%C3"
# the %C3 above percent-decodes to an unpaired \xc3 byte which makes this
# invalid utf8


class TestURL(HyperlinkTestCase):
    def test_parse(self):
        # type: () -> None
        purl = parse(TOTAL_URL)
        assert isinstance(purl, DecodedURL)
        assert purl.user == "user"
        assert purl.get("zot") == ["23%"]
        assert purl.fragment == "frég"

        purl2 = parse(TOTAL_URL, decoded=False)
        assert isinstance(purl2, EncodedURL)
        assert purl2.get("zot") == ["23%25"]

        with self.assertRaises(UnicodeDecodeError):
            purl3 = parse(UNDECODABLE_FRAG_URL)

        purl3 = parse(UNDECODABLE_FRAG_URL, lazy=True)

        with self.assertRaises(UnicodeDecodeError):
            purl3.fragment
