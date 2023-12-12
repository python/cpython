import threading
import unittest

from test import support
from test.support import import_helper
from test.support import threading_helper
# Raise SkipTest if subinterpreters not supported.
import_helper.import_module('_xxsubinterpreters')
from test.support import interpreters
from .utils import TestBase


class StressTests(TestBase):

    # In these tests we generally want a lot of interpreters,
    # but not so many that any test takes too long.

    @support.requires_resource('cpu')
    def test_create_many_sequential(self):
        alive = []
        for _ in range(100):
            interp = interpreters.create()
            alive.append(interp)

    @support.requires_resource('cpu')
    def test_create_many_threaded(self):
        alive = []
        def task():
            interp = interpreters.create()
            alive.append(interp)
        threads = (threading.Thread(target=task) for _ in range(200))
        with threading_helper.start_threads(threads):
            pass


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()
