# xml.etree test for cElementTree

from test import test_support

cET = test_support.import_module('xml.etree.cElementTree')


# cElementTree specific tests

def sanity():
    """
    Import sanity.

    >>> from xml.etree import cElementTree
    """


def test_main():
    from test import test_xml_etree, test_xml_etree_c

    # Run the tests specific to the C implementation
    test_support.run_doctest(test_xml_etree_c, verbosity=True)

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
