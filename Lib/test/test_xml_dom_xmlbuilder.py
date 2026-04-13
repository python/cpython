import io
import unittest
from http import client
from test.test_httplib import FakeSocket
from unittest import mock
from xml.dom import getDOMImplementation, minidom, xmlbuilder

SMALL_SAMPLE = b"""<?xml version="1.0"?>
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:xdc="http://www.xml.com/books">
<!-- A comment -->
<title>Introduction to XSL</title>
<hr/>
<p><xdc:author xdc:attrib="prefixed attribute" attrib="other attrib">A. Namespace</xdc:author></p>
</html>"""


class XMLBuilderTest(unittest.TestCase):
    def test_entity_resolver(self):
        body = (
            b"HTTP/1.1 200 OK\r\nContent-Type: text/xml; charset=utf-8\r\n\r\n"
            + SMALL_SAMPLE
        )

        sock = FakeSocket(body)
        response = client.HTTPResponse(sock)
        response.begin()
        attrs = {"open.return_value": response}
        opener = mock.Mock(**attrs)

        resolver = xmlbuilder.DOMEntityResolver()

        with mock.patch("urllib.request.build_opener") as mock_build:
            mock_build.return_value = opener
            source = resolver.resolveEntity(None, "http://example.com/2000/svg")

        self.assertIsInstance(source, xmlbuilder.DOMInputSource)
        self.assertIsNone(source.publicId)
        self.assertEqual(source.systemId, "http://example.com/2000/svg")
        self.assertEqual(source.baseURI, "http://example.com/2000/")
        self.assertEqual(source.encoding, "utf-8")
        self.assertIs(source.byteStream, response)

        self.assertIsNone(source.characterStream)
        self.assertIsNone(source.stringData)

    def test_builder(self):
        imp = getDOMImplementation()
        self.assertIsInstance(imp, xmlbuilder.DOMImplementationLS)

        builder = imp.createDOMBuilder(imp.MODE_SYNCHRONOUS, None)
        self.assertIsInstance(builder, xmlbuilder.DOMBuilder)

    def test_parse_uri(self):
        body = (
            b"HTTP/1.1 200 OK\r\nContent-Type: text/xml; charset=utf-8\r\n\r\n"
            + SMALL_SAMPLE
        )

        sock = FakeSocket(body)
        response = client.HTTPResponse(sock)
        response.begin()
        attrs = {"open.return_value": response}
        opener = mock.Mock(**attrs)

        with mock.patch("urllib.request.build_opener") as mock_build:
            mock_build.return_value = opener

            imp = getDOMImplementation()
            builder = imp.createDOMBuilder(imp.MODE_SYNCHRONOUS, None)
            document = builder.parseURI("http://example.com/2000/svg")

        self.assertIsInstance(document, minidom.Document)
        self.assertEqual(len(document.childNodes), 1)

    def test_parse_with_systemId(self):
        response = io.BytesIO(SMALL_SAMPLE)

        with mock.patch("urllib.request.urlopen") as mock_open:
            mock_open.return_value = response

            imp = getDOMImplementation()
            source = imp.createDOMInputSource()
            builder = imp.createDOMBuilder(imp.MODE_SYNCHRONOUS, None)
            source.systemId = "http://example.com/2000/svg"
            document = builder.parse(source)

        self.assertIsInstance(document, minidom.Document)
        self.assertEqual(len(document.childNodes), 1)
