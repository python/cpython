import pickle
import struct
from cStringIO import StringIO

from test import test_support

from test.pickletester import (AbstractUnpickleTests,
                               AbstractPickleTests,
                               AbstractPickleModuleTests,
                               AbstractPersistentPicklerTests,
                               AbstractPicklerUnpicklerObjectTests,
                               BigmemPickleTests)

class PickleTests(AbstractUnpickleTests, AbstractPickleTests,
                  AbstractPickleModuleTests):

    def dumps(self, arg, proto=0, fast=0):
        # Ignore fast
        return pickle.dumps(arg, proto)

    def loads(self, buf):
        # Ignore fast
        return pickle.loads(buf)

    module = pickle
    error = KeyError
    bad_stack_errors = (IndexError,)
    bad_mark_errors = (IndexError, pickle.UnpicklingError,
                       TypeError, AttributeError, EOFError)
    truncated_errors = (pickle.UnpicklingError, EOFError,
                        AttributeError, ValueError,
                        struct.error, IndexError, ImportError,
                        TypeError, KeyError)

class UnpicklerTests(AbstractUnpickleTests):

    error = KeyError
    bad_stack_errors = (IndexError,)
    bad_mark_errors = (IndexError, pickle.UnpicklingError,
                       TypeError, AttributeError, EOFError)
    truncated_errors = (pickle.UnpicklingError, EOFError,
                        AttributeError, ValueError,
                        struct.error, IndexError, ImportError,
                        TypeError, KeyError)

    def loads(self, buf):
        f = StringIO(buf)
        u = pickle.Unpickler(f)
        return u.load()

class PicklerTests(AbstractPickleTests):

    def dumps(self, arg, proto=0, fast=0):
        f = StringIO()
        p = pickle.Pickler(f, proto)
        if fast:
            p.fast = fast
        p.dump(arg)
        f.seek(0)
        return f.read()

    def loads(self, buf):
        f = StringIO(buf)
        u = pickle.Unpickler(f)
        return u.load()

class PersPicklerTests(AbstractPersistentPicklerTests):

    def dumps(self, arg, proto=0, fast=0):
        class PersPickler(pickle.Pickler):
            def persistent_id(subself, obj):
                return self.persistent_id(obj)
        f = StringIO()
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
        f = StringIO(buf)
        u = PersUnpickler(f)
        return u.load()

class PicklerUnpicklerObjectTests(AbstractPicklerUnpicklerObjectTests):

    pickler_class = pickle.Pickler
    unpickler_class = pickle.Unpickler

class PickleBigmemPickleTests(BigmemPickleTests):

    def dumps(self, arg, proto=0, fast=0):
        # Ignore fast
        return pickle.dumps(arg, proto)

    def loads(self, buf):
        # Ignore fast
        return pickle.loads(buf)


def test_main():
    test_support.run_unittest(
        PickleTests,
        UnpicklerTests,
        PicklerTests,
        PersPicklerTests,
        PicklerUnpicklerObjectTests,
        PickleBigmemPickleTests,
    )
    test_support.run_doctest(pickle)

if __name__ == "__main__":
    test_main()
