# xml.etree test for cElementTree

from test import support
from test.support import precisionbigmemtest, _2G
import unittest

cET = support.import_module('xml.etree.cElementTree')


# cElementTree specific tests

def sanity():
    r"""
    Import sanity.

    >>> from xml.etree import cElementTree

    Issue #6697.

    >>> e = cElementTree.Element('a')
    >>> getattr(e, '\uD800')           # doctest: +ELLIPSIS
    Traceback (most recent call last):
      ...
    UnicodeEncodeError: ...

    >>> p = cElementTree.XMLParser()
    >>> p.version.split()[0]
    'Expat'
    >>> getattr(p, '\uD800')
    Traceback (most recent call last):
     ...
    AttributeError: 'XMLParser' object has no attribute '\ud800'
    """


class MiscTests(unittest.TestCase):
    # Issue #8651.
    @support.precisionbigmemtest(size=support._2G + 100, memuse=1)
    def test_length_overflow(self, size):
        if size < support._2G + 100:
            self.skipTest("not enough free memory, need at least 2 GB")
        data = b'x' * size
        parser = cET.XMLParser()
        try:
            self.assertRaises(OverflowError, parser.feed, data)
        finally:
            data = None


def test_main():
    from test import test_xml_etree, test_xml_etree_c

    # Run the tests specific to the C implementation
    support.run_doctest(test_xml_etree_c, verbosity=True)

    support.run_unittest(MiscTests)

    # Assign the C implementation before running the doctests
    # Patch the __name__, to prevent confusion with the pure Python test
    pyET = test_xml_etree.ET
    py__name__ = test_xml_etree.__name__
    test_xml_etree.ET = cET
    if __name__ != '__main__':
        test_xml_etree.__name__ = __name__
    try:
        # Run the same test suite as xml.etree.ElementTree
        test_xml_etree.test_main(module_name='xml.etree.cElementTree')
    finally:
        test_xml_etree.ET = pyET
        test_xml_etree.__name__ = py__name__

if __name__ == '__main__':
    test_main()
