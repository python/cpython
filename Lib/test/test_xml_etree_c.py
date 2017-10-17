# xml.etree test for cElementTree

from test import test_support
from test.test_support import precisionbigmemtest, _2G
import unittest

cET = test_support.import_module('xml.etree.cElementTree')


@unittest.skipUnless(cET, 'requires _elementtree')
class MiscTests(unittest.TestCase):
    # Issue #8651.
    @precisionbigmemtest(size=_2G + 100, memuse=1)
    def test_length_overflow(self, size):
        if size < _2G + 100:
            self.skipTest("not enough free memory, need at least 2 GB")
        data = b'x' * size
        parser = cET.XMLParser()
        try:
            self.assertRaises(OverflowError, parser.feed, data)
        finally:
            data = None

    def test_del_attribute(self):
        element = cET.Element('tag')

        element.tag = 'TAG'
        with self.assertRaises(AttributeError):
            del element.tag
        self.assertEqual(element.tag, 'TAG')

        with self.assertRaises(AttributeError):
            del element.text
        self.assertIsNone(element.text)
        element.text = 'TEXT'
        with self.assertRaises(AttributeError):
            del element.text
        self.assertEqual(element.text, 'TEXT')

        with self.assertRaises(AttributeError):
            del element.tail
        self.assertIsNone(element.tail)
        element.tail = 'TAIL'
        with self.assertRaises(AttributeError):
            del element.tail
        self.assertEqual(element.tail, 'TAIL')

        with self.assertRaises(AttributeError):
            del element.attrib
        self.assertEqual(element.attrib, {})
        element.attrib = {'A': 'B', 'C': 'D'}
        with self.assertRaises(AttributeError):
            del element.attrib
        self.assertEqual(element.attrib, {'A': 'B', 'C': 'D'})

    def test_bpo_31728(self):
        # A crash shouldn't happen in case garbage collection triggers a call
        # to clear() or a reading of text or tail, while a setter or clear()
        # is already running.
        elem = cET.Element('elem')
        class X:
            def __del__(self):
                elem.text
                elem.tail
                elem.clear()

        elem.text = X()
        elem.clear()  # shouldn't crash

        elem.tail = X()
        elem.clear()  # shouldn't crash

        elem.text = X()
        elem.text = X()  # shouldn't crash
        elem.clear()

        elem.tail = X()
        elem.tail = X()  # shouldn't crash
        elem.clear()


def test_main():
    from test import test_xml_etree, test_xml_etree_c

    # Run the tests specific to the C implementation
    test_support.run_unittest(MiscTests)

    # Run the same test suite as the Python module
    test_xml_etree.test_main(module=cET)


if __name__ == '__main__':
    test_main()
