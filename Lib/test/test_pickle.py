import pickle
import unittest
import io

from test import test_support

from test.pickletester import AbstractPickleTests
from test.pickletester import AbstractPickleModuleTests
from test.pickletester import AbstractPersistentPicklerTests

class PickleTests(AbstractPickleTests, AbstractPickleModuleTests):

    module = pickle
    error = KeyError

    def dumps(self, arg, proto=0, fast=0):
        # Ignore fast
        return pickle.dumps(arg, proto)

    def loads(self, buf):
        # Ignore fast
        return pickle.loads(buf)

class PicklerTests(AbstractPickleTests):

    error = KeyError

    def dumps(self, arg, proto=0, fast=0):
        f = io.BytesIO()
        p = pickle.Pickler(f, proto)
        if fast:
            p.fast = fast
        p.dump(arg)
        f.seek(0)
        return bytes(f.read())

    def loads(self, buf):
        f = io.BytesIO(buf)
        u = pickle.Unpickler(f)
        return u.load()

class PersPicklerTests(AbstractPersistentPicklerTests):

    def dumps(self, arg, proto=0, fast=0):
        class PersPickler(pickle.Pickler):
            def persistent_id(subself, obj):
                return self.persistent_id(obj)
        f = io.BytesIO()
        p = PersPickler(f, proto)
        if fast:
            p.fast = fast
        p.dump(arg)
        f.seek(0)
        return f.read()

    def loads(self, buf):
        class PersUnpickler(pickle.Unpickler):
            def persistent_load(subself, obj):
                return self.persistent_load(obj)
        f = io.BytesIO(buf)
        u = PersUnpickler(f)
        return u.load()

def test_main():
    test_support.run_unittest(
        PickleTests,
        PicklerTests,
        PersPicklerTests
    )
    test_support.run_doctest(pickle)

if __name__ == "__main__":
    test_main()
