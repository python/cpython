import pickle
from cStringIO import StringIO
from pickletester import AbstractPickleTests, AbstractPickleModuleTests
from test_support import run_unittest

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

if __name__ == "__main__":
    run_unittest(PickleTests)
    run_unittest(PicklerTests)
