import unittest

from xml.sax.expatreader import create_parser
from xml.sax import handler


class feature_namespace_prefixes(unittest.TestCase):
    def setUp(self):
        self.parser = create_parser()
        self.parser.setFeature(handler.feature_namespaces, 1)
        self.parser.setFeature(handler.feature_namespace_prefixes, 1)

    def test_prefix_given(self):
        class Handler(handler.ContentHandler):
            def startElementNS(self, name, qname, attrs):
                self.qname = qname

        h = Handler()

        self.parser.setContentHandler(h)
        self.parser.feed("<Q:E xmlns:Q='http://example.org/testuri'/>")
        self.parser.close()
        print("self.assertEqual")
        self.assertFalse(h.qname is None)
