# xml.etree test for cElementTree
import sys, struct
from test import support
from test.support import import_fresh_module
import unittest

cET = import_fresh_module('xml.etree.ElementTree', fresh=['_elementtree'])
cET_alias = import_fresh_module('xml.etree.cElementTree', fresh=['_elementtree', 'xml.etree'])


class MiscTests(unittest.TestCase):
    # Issue #8651.
    @support.bigmemtest(size=support._2G + 100, memuse=1)
    def test_length_overflow(self, size):
        if size < support._2G + 100:
            self.skipTest("not enough free memory, need at least 2 GB")
        data = b'x' * size
        parser = cET.XMLParser()
        try:
            self.assertRaises(OverflowError, parser.feed, data)
        finally:
            data = None


@unittest.skipUnless(cET, 'requires _elementtree')
class TestAliasWorking(unittest.TestCase):
    # Test that the cET alias module is alive
    def test_alias_working(self):
        e = cET_alias.Element('foo')
        self.assertEqual(e.tag, 'foo')


@unittest.skipUnless(cET, 'requires _elementtree')
class TestAcceleratorImported(unittest.TestCase):
    # Test that the C accelerator was imported, as expected
    def test_correct_import_cET(self):
        self.assertEqual(cET.SubElement.__module__, '_elementtree')

    def test_correct_import_cET_alias(self):
        self.assertEqual(cET_alias.SubElement.__module__, '_elementtree')


@unittest.skipUnless(cET, 'requires _elementtree')
class SizeofTest(unittest.TestCase):
    def setUp(self):
        import _testcapi
        gc_headsize = _testcapi.SIZEOF_PYGC_HEAD
        # object header
        header = 'PP'
        if hasattr(sys, "gettotalrefcount"):
            # debug header
            header = 'PP' + header
        # fields
        element = header + '5P'
        self.elementsize = gc_headsize + struct.calcsize(element)
        # extra
        self.extra = struct.calcsize('PiiP4P')

    def test_element(self):
        e = cET.Element('a')
        self.assertEqual(sys.getsizeof(e), self.elementsize)

    def test_element_with_attrib(self):
        e = cET.Element('a', href='about:')
        self.assertEqual(sys.getsizeof(e),
                         self.elementsize + self.extra)

    def test_element_with_children(self):
        e = cET.Element('a')
        for i in range(5):
            cET.SubElement(e, 'span')
        # should have space for 8 children now
        self.assertEqual(sys.getsizeof(e),
                         self.elementsize + self.extra +
                         struct.calcsize('8P'))

def test_main():
    from test import test_xml_etree, test_xml_etree_c

    # Run the tests specific to the C implementation
    support.run_unittest(
        MiscTests,
        TestAliasWorking,
        TestAcceleratorImported,
        SizeofTest,
        )

    # Run the same test suite as the Python module
    test_xml_etree.test_main(module=cET)


if __name__ == '__main__':
    test_main()
