import os
from test import support
import unittest

# Skip tests if we don't have concurrent.futures.
support.import_module('concurrent.futures')


def load_tests(loader, _, pattern):
    pkg_dir = os.path.dirname(__file__)
    suite = AsyncioTestSuite()
    return support.load_package_tests(pkg_dir, loader, suite, pattern)


class AsyncioTestSuite(unittest.TestSuite):
    """A custom test suite that also runs setup/teardown for the whole package.

    Normally unittest only runs setUpModule() and tearDownModule() within each
    test module part of the test suite. Copying those functions to each file
    would be tedious, let's run this once and for all.
    """
    def run(self, result, debug=False):
        ignore = support.ignore_deprecations_from
        tokens = {
            ignore("asyncio.base_events", like=r".*loop argument.*"),
            ignore("asyncio.unix_events", like=r".*loop argument.*"),
            ignore("asyncio.futures", like=r".*loop argument.*"),
            ignore("asyncio.runners", like=r".*loop argument.*"),
            ignore("asyncio.subprocess", like=r".*loop argument.*"),
            ignore("asyncio.tasks", like=r".*loop argument.*"),
            ignore("test.test_asyncio.test_events", like=r".*loop argument.*"),
            ignore("test.test_asyncio.test_queues", like=r".*loop argument.*"),
            ignore("test.test_asyncio.test_tasks", like=r".*loop argument.*"),
        }
        try:
            super().run(result, debug=debug)
        finally:
            support.clear_ignored_deprecations(*tokens)
