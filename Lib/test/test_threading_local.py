import unittest
from doctest import DocTestSuite
from test import test_support

def test_main():
    suite = DocTestSuite('_threading_local')

    try:
        from thread import _local
    except ImportError:
        pass
    else:
        import _threading_local
        local_orig = _threading_local.local
        def setUp(test):
            _threading_local.local = _local
        def tearDown(test):
            _threading_local.local = local_orig
        suite.addTest(DocTestSuite('_threading_local',
                                   setUp=setUp, tearDown=tearDown)
                      )

    test_support.run_suite(suite)

if __name__ == '__main__':
    test_main()
