# test_pickle dumps and loads pickles via pickle.py.
# test_cpickle does the same, but via the cPickle module.
# This test covers the other two cases, making pickles with one module and
# loading them via the other.

import pickle
import cPickle
import unittest

from test import test_support
from test.pickletester import AbstractPickleTests

class DumpCPickle_LoadPickle(AbstractPickleTests):

    error = KeyError

    def dumps(self, arg, proto=0, fast=0):
        # Ignore fast
        return cPickle.dumps(arg, proto)

    def loads(self, buf):
        # Ignore fast
        return pickle.loads(buf)

class DumpPickle_LoadCPickle(AbstractPickleTests):

    error = cPickle.BadPickleGet

    def dumps(self, arg, proto=0, fast=0):
        # Ignore fast
        return pickle.dumps(arg, proto)

    def loads(self, buf):
        # Ignore fast
        return cPickle.loads(buf)

def test_main():
    test_support.run_unittest(
        DumpCPickle_LoadPickle,
        DumpPickle_LoadCPickle
    )

if __name__ == "__main__":
    test_main()
