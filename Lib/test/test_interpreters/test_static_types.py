import unittest

from test.support import import_helper
# Raise SkipTest if subinterpreters not supported.
import_helper.import_module('_interpreters')
from concurrent import interpreters
from .utils import TestBase


xxsubtype = import_helper.import_module('xxsubtype')


class TestStaticTypeCache(TestBase):

    def test_subinterp_no_crash(self):
        # gh-158203: Static extension types registered via PyType_Ready()
        # only have a globally shared _tp_cache field. Two interpreters
        # writing to the same cache slot caused a use-after-free (SIGSEGV).
        obj = xxsubtype.spamlist()
        obj.append(1)
        for name in dir(xxsubtype.spamlist):
            getattr(xxsubtype.spamlist, name, None)

        interp = interpreters.create()
        try:
            interp.exec("""
import xxsubtype
obj = xxsubtype.spamlist()
obj.append(42)
for name in dir(xxsubtype.spamlist):
    getattr(xxsubtype.spamlist, name, None)
""")
        finally:
            interp.close()

    def test_multiple_subinterps_no_crash(self):
        # Same as above but with several subinterpreters concurrently
        # reading and populating the cache.
        xxsubtype.spamlist().append(0)

        interps = [interpreters.create() for _ in range(3)]
        try:
            for interp in interps:
                interp.exec("""
import xxsubtype
for name in dir(xxsubtype.spamlist):
    getattr(xxsubtype.spamlist, name, None)
""")
        finally:
            for interp in interps:
                interp.close()


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()
