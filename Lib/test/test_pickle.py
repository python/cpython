import pickle
import test_support
import unittest
from cStringIO import StringIO
from pickletester import AbstractPickleTests, AbstractPickleModuleTests

class PickleTests(AbstractPickleTests, AbstractPickleModuleTests):

    def setUp(self):
        self.dumps = pickle.dumps
        self.loads = pickle.loads

    module = pickle
    error = KeyError

class PicklerTests(AbstractPickleTests):

    error = KeyError

    def dumps(self, arg, bin=0):
        f = StringIO()
        p = pickle.Pickler(f, bin)
        p.dump(arg)
        f.seek(0)
        return f.read()

    def loads(self, buf):
        f = StringIO(buf)
        u = pickle.Unpickler(f)
        return u.load()

def test_main():
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    suite.addTest(loader.loadTestsFromTestCase(PickleTests))
    suite.addTest(loader.loadTestsFromTestCase(PicklerTests))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
