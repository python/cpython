# xml.etree test for cElementTree
import io
import struct
from test import support
from test.support import threading_helper
from test.support.import_helper import import_fresh_module
import types
import unittest

cET = import_fresh_module('xml.etree.ElementTree',
                          fresh=['_elementtree'])
cET_alias = import_fresh_module('xml.etree.cElementTree',
                                fresh=['_elementtree', 'xml.etree'],
                                deprecated=True)


@unittest.skipUnless(cET, 'requires _elementtree')
class MiscTests(unittest.TestCase):
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

    @support.skip_wasi_stack_overflow()
    @support.skip_emscripten_stack_overflow()
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

    @support.cpython_only
    def test_uninitialized_parser(self):
        # The interpreter shouldn't crash in case of calling methods or
        # accessing attributes of uninitialized XMLParser objects.
        parser = cET.XMLParser.__new__(cET.XMLParser)
        self.assertRaises(ValueError, parser.close)
        self.assertRaises(ValueError, parser.feed, 'foo')
        class MockFile:
            def read(*args):
                return ''
        self.assertRaises(ValueError, parser._parse_whole, MockFile())
        self.assertRaises(ValueError, parser._setevents, None)
        self.assertIsNone(parser.entity)
        self.assertIsNone(parser.target)

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

    def test_iterparse_leaks(self):
        # Test reference leaks in TreeBuilder (issue #35502).
        # The test is written to be executed in the hunting reference leaks
        # mode.
        XML = '<a></a></b>'
        parser = cET.iterparse(io.StringIO(XML))
        next(parser)
        del parser
        support.gc_collect()

    def test_xmlpullparser_leaks(self):
        # Test reference leaks in TreeBuilder (issue #35502).
        # The test is written to be executed in the hunting reference leaks
        # mode.
        XML = '<a></a></b>'
        parser = cET.XMLPullParser()
        parser.feed(XML)
        del parser
        support.gc_collect()

    def test_dict_disappearing_during_get_item(self):
        # test fix for seg fault reported in issue 27946
        class X:
            def __hash__(self):
                e.attrib = {} # this frees e->extra->attrib
                [{i: i} for i in range(1000)] # exhaust the dict keys cache
                return 13

        e = cET.Element("elem", {1: 2})
        r = e.get(X())
        self.assertIsNone(r)

    @support.cpython_only
    def test_immutable_types(self):
        root = cET.fromstring('<a></a>')
        dataset = (
            cET.Element,
            cET.TreeBuilder,
            cET.XMLParser,
            type(root.iter()),
        )
        for tp in dataset:
            with self.subTest(tp=tp):
                support.check_immutable_type(self, tp)

    @support.cpython_only
    def test_disallow_instantiation(self):
        root = cET.fromstring('<a></a>')
        iter_type = type(root.iter())
        support.check_disallow_instantiation(self, iter_type)


@unittest.skipUnless(cET, 'requires _elementtree')
@threading_helper.requires_working_threading()
class ThreadingTests(unittest.TestCase):
    def test_concurrent_slice_deletion(self):
        self.check_concurrent_slice_deletion(slice(1, 65))

    def test_concurrent_extended_slice_deletion(self):
        for index in (slice(1, 65, 3), slice(64, 0, -3), slice(1, None, 3)):
            with self.subTest(index=index):
                self.check_concurrent_slice_deletion(index)

    def check_concurrent_slice_deletion(self, index):
        # gh-149965: Read children while slice deletion shifts the child
        # pointers. Create the elements here so the writer does not own them.
        roots = []
        expected = []
        for _ in range(100):
            root = cET.Element('root')
            children = [cET.SubElement(root, str(i)) for i in range(100)]
            del children[index]
            roots.append(root)
            expected.append(children)

        def delete():
            for root in roots:
                del root[index]

        def read():
            for _ in range(10):
                for root in roots:
                    # Each slice leaves at least two children. Their identity
                    # can change during deletion, but they must remain valid.
                    child = root[1]
                    self.assertIsInstance(child, cET.Element)
                    self.assertIn(child.tag, tags)

        tags = {str(i) for i in range(100)}
        threading_helper.run_concurrently([delete, read, read])
        for root, children in zip(roots, expected):
            self.assertEqual(root[:], children)


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


def install_tests():
    # Test classes should have __module__ referring to this module.
    from test import test_xml_etree
    for name, base in vars(test_xml_etree).items():
        if isinstance(base, type) and issubclass(base, unittest.TestCase):
            class Temp(base):
                pass
            Temp.__name__ = Temp.__qualname__ = name
            Temp.__module__ = __name__
            assert name not in globals()
            globals()[name] = Temp

install_tests()

def setUpModule():
    from test import test_xml_etree
    test_xml_etree.setUpModule(module=cET)


if __name__ == '__main__':
    unittest.main()
