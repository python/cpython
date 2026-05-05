"""Smoke tests for lxml built-in on NanVix."""

import sys
import unittest
from test.support import is_nanvix
from test.support.import_helper import import_module

if not is_nanvix:
    raise unittest.SkipTest("lxml built-in is Nanvix-specific")

etree = import_module("lxml.etree")


class NanvixLxmlTests(unittest.TestCase):

    def test_import_lxml_etree(self):
        self.assertTrue(hasattr(etree, "fromstring"))
        self.assertTrue(hasattr(etree, "_Element"))

    def test_parse_xml(self):
        root = etree.fromstring(b'<root><child key="val">text</child></root>')
        self.assertEqual(root.tag, "root")
        child = root.find("child")
        self.assertIsNotNone(child)
        self.assertEqual(child.text, "text")
        self.assertEqual(child.get("key"), "val")

    def test_element_creation(self):
        root = etree.Element("doc")
        etree.SubElement(root, "item").text = "hello"
        xml = etree.tostring(root, encoding="unicode")
        self.assertIn("<item>hello</item>", xml)

    def test_elementpath(self):
        root = etree.fromstring(b"<a><b>1</b><b>2</b></a>")
        results = root.findall("b")
        self.assertEqual(len(results), 2)
        self.assertEqual(results[0].text, "1")

    def test_xmlfile_available(self):
        # xmlfile must be importable from lxml.etree for openpyxl compat.
        from lxml.etree import xmlfile

        self.assertTrue(callable(xmlfile))

    def test_htmlfile_available(self):
        from lxml.etree import htmlfile

        self.assertTrue(callable(htmlfile))


if __name__ == "__main__":
    unittest.main()
