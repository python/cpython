# Test package for traceback timestamps feature


def load_tests(loader, tests, pattern):
    import unittest
    from . import test_basic, test_pickle

    suite = unittest.TestSuite()
    suite.addTests(loader.loadTestsFromModule(test_basic))
    suite.addTests(loader.loadTestsFromModule(test_pickle))
    return suite
