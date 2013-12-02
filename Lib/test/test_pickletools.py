import pickle
import pickletools
from test import support
from test.pickletester import AbstractPickleTests
from test.pickletester import AbstractPickleModuleTests

class OptimizedPickleTests(AbstractPickleTests, AbstractPickleModuleTests):

    def dumps(self, arg, proto=None):
        return pickletools.optimize(pickle.dumps(arg, proto))

    def loads(self, buf, **kwds):
        return pickle.loads(buf, **kwds)

    # Test relies on precise output of dumps()
    test_pickle_to_2x = None


def test_main():
    support.run_unittest(OptimizedPickleTests)
    support.run_doctest(pickletools)


if __name__ == "__main__":
    test_main()
