# xml.etree test for cElementTree

from test import support
import unittest

from xml.etree import ElementTree as cET, cElementTree as cET_alias


# cElementTree specific tests

def sanity():
    r"""
    Import sanity.

    Issue #6697.

    >>> cElementTree = cET
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


def test_main():
    from test import test_xml_etree, test_xml_etree_c

    # Run the tests specific to the C implementation
    support.run_doctest(test_xml_etree_c, verbosity=True)

    support.run_unittest(MiscTests)

    # Run the same test suite as the Python module
    test_xml_etree.test_main(module=cET)
    # Exercise the deprecated alias
    test_xml_etree.test_main(module=cET_alias)

if __name__ == '__main__':
    test_main()
