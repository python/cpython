# xml.etree test for cElementTree
import struct
from test import support
from test.support import import_fresh_module
import types
import unittest

cET = import_fresh_module('xml.etree.ElementTree',
                          fresh=['_elementtree'])
cET_alias = import_fresh_module('xml.etree.cElementTree',
                                fresh=['_elementtree', 'xml.etree'])


@unittest.skipUnless(cET, 'requires _elementtree')
class MiscTests(unittest.TestCase):
    # Issue #8651.
    @support.bigmemtest(size=support._2G + 100, memuse=1, dry_run=False)
    def test_length_overflow(self, size):
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

    def test_trashcan(self):
        # If this test fails, it will most likely die via segfault.
        e = root = cET.Element('root')
        for i in range(200000):
            e = cET.SubElement(e, 'x')
        del e
        del root
        support.gc_collect()

    def test_parser_ref_cycle(self):
        # bpo-31499: xmlparser_dealloc() crashed with a segmentation fault when
        # xmlparser_gc_clear() was called previously by the garbage collector,
        # when the parser was part of a reference cycle.

        def parser_ref_cycle():
            parser = cET.XMLParser()
            # Create a reference cycle using an exception to keep the frame
            # alive, so the parser will be destroyed by the garbage collector
            try:
                raise ValueError
            except ValueError as exc:
                err = exc

        # Create a parser part of reference cycle
        parser_ref_cycle()
        # Trigger an explicit garbage collection to break the reference cycle
        # and so destroy the parser
        support.gc_collect()

    def test_bpo_31728(self):
        # A crash or an assertion failure shouldn't happen, in case garbage
        # collection triggers a call to clear() or a reading of text or tail,
        # while a setter or clear() or __setstate__() is already running.
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

        elem.text = X()
        elem.__setstate__({'tag': 42})  # shouldn't cause an assertion failure
        elem.clear()

        elem.tail = X()
        elem.__setstate__({'tag': 42})  # shouldn't cause an assertion failure

    def test_setstate_leaks(self):
        # Test reference leaks
        elem = cET.Element.__new__(cET.Element)
        for i in range(100):
            elem.__setstate__({'tag': 'foo', 'attrib': {'bar': 42},
                               '_children': [cET.Element('child')],
                               'text': 'text goes here',
                               'tail': 'opposite of head'})

        self.assertEqual(elem.tag, 'foo')
        self.assertEqual(elem.text, 'text goes here')
        self.assertEqual(elem.tail, 'opposite of head')
        self.assertEqual(list(elem.attrib.items()), [('bar', 42)])
        self.assertEqual(len(elem), 1)
        self.assertEqual(elem[0].tag, 'child')


@unittest.skipUnless(cET, 'requires _elementtree')
class TestAliasWorking(unittest.TestCase):
    # Test that the cET alias module is alive
    def test_alias_working(self):
        e = cET_alias.Element('foo')
        self.assertEqual(e.tag, 'foo')


@unittest.skipUnless(cET, 'requires _elementtree')
@support.cpython_only
class TestAcceleratorImported(unittest.TestCase):
    # Test that the C accelerator was imported, as expected
    def test_correct_import_cET(self):
        # SubElement is a function so it retains _elementtree as its module.
        self.assertEqual(cET.SubElement.__module__, '_elementtree')

    def test_correct_import_cET_alias(self):
        self.assertEqual(cET_alias.SubElement.__module__, '_elementtree')

    def test_parser_comes_from_C(self):
        # The type of methods defined in Python code is types.FunctionType,
        # while the type of methods defined inside _elementtree is
        # <class 'wrapper_descriptor'>
        self.assertNotIsInstance(cET.Element.__init__, types.FunctionType)


@unittest.skipUnless(cET, 'requires _elementtree')
@support.cpython_only
class SizeofTest(unittest.TestCase):
    def setUp(self):
        self.elementsize = support.calcobjsize('5P')
        # extra
        self.extra = struct.calcsize('PnnP4P')

    check_sizeof = support.check_sizeof

    def test_element(self):
        e = cET.Element('a')
        self.check_sizeof(e, self.elementsize)

    def test_element_with_attrib(self):
        e = cET.Element('a', href='about:')
        self.check_sizeof(e, self.elementsize + self.extra)

    def test_element_with_children(self):
        e = cET.Element('a')
        for i in range(5):
            cET.SubElement(e, 'span')
        # should have space for 8 children now
        self.check_sizeof(e, self.elementsize + self.extra +
                             struct.calcsize('8P'))

def test_main():
    from test import test_xml_etree

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
