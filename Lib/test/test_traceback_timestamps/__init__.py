# Test package for traceback timestamps feature


def load_tests(loader, tests, pattern):
    """Load all tests from the package."""
    import unittest
    from . import (
        test_basic,
        test_pickle,
        test_user_exceptions,
        test_timestamp_presence,
    )

    suite = unittest.TestSuite()

    # Add tests from all modules
    suite.addTests(loader.loadTestsFromModule(test_basic))
    suite.addTests(loader.loadTestsFromModule(test_pickle))
    suite.addTests(loader.loadTestsFromModule(test_user_exceptions))
    suite.addTests(loader.loadTestsFromModule(test_timestamp_presence))

    return suite
