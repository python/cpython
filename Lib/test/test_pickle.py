import pickle
import io

from test import support

from test.pickletester import AbstractPickleTests
from test.pickletester import AbstractPickleModuleTests
from test.pickletester import AbstractPersistentPicklerTests

try:
    import _pickle
    has_c_implementation = True
except ImportError:
    has_c_implementation = False


class PickleTests(AbstractPickleModuleTests):
    pass


class PyPicklerTests(AbstractPickleTests):

    pickler = pickle._Pickler
    unpickler = pickle._Unpickler

    def dumps(self, arg, proto=None):
        f = io.BytesIO()
        p = self.pickler(f, proto)
        p.dump(arg)
        f.seek(0)
        return bytes(f.read())

    def loads(self, buf):
        f = io.BytesIO(buf)
        u = self.unpickler(f)
        return u.load()


class PyPersPicklerTests(AbstractPersistentPicklerTests):

    pickler = pickle._Pickler
    unpickler = pickle._Unpickler

    def dumps(self, arg, proto=None):
        class PersPickler(self.pickler):
            def persistent_id(subself, obj):
                return self.persistent_id(obj)
        f = io.BytesIO()
        p = PersPickler(f, proto)
        p.dump(arg)
        f.seek(0)
        return f.read()

    def loads(self, buf):
        class PersUnpickler(self.unpickler):
            def persistent_load(subself, obj):
                return self.persistent_load(obj)
        f = io.BytesIO(buf)
        u = PersUnpickler(f)
        return u.load()


if has_c_implementation:
    class CPicklerTests(PyPicklerTests):
        pickler = _pickle.Pickler
        unpickler = _pickle.Unpickler

    class CPersPicklerTests(PyPersPicklerTests):
        pickler = _pickle.Pickler
        unpickler = _pickle.Unpickler


def test_main():
    tests = [PickleTests, PyPicklerTests, PyPersPicklerTests]
    if has_c_implementation:
        tests.extend([CPicklerTests, CPersPicklerTests])
    support.run_unittest(*tests)
    support.run_doctest(pickle)

if __name__ == "__main__":
    test_main()
