import pickle
import io
import collections
import struct
import sys

import unittest
from test import support

from test.pickletester import AbstractPickleTests
from test.pickletester import AbstractPickleModuleTests
from test.pickletester import AbstractPersistentPicklerTests
from test.pickletester import AbstractPicklerUnpicklerObjectTests
from test.pickletester import AbstractDispatchTableTests
from test.pickletester import BigmemPickleTests

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

    def loads(self, buf, **kwds):
        f = io.BytesIO(buf)
        u = self.unpickler(f, **kwds)
        return u.load()


class InMemoryPickleTests(AbstractPickleTests, BigmemPickleTests):

    pickler = pickle._Pickler
    unpickler = pickle._Unpickler

    def dumps(self, arg, protocol=None):
        return pickle.dumps(arg, protocol)

    def loads(self, buf, **kwds):
        return pickle.loads(buf, **kwds)


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

    def loads(self, buf, **kwds):
        class PersUnpickler(self.unpickler):
            def persistent_load(subself, obj):
                return self.persistent_load(obj)
        f = io.BytesIO(buf)
        u = PersUnpickler(f, **kwds)
        return u.load()


class PyPicklerUnpicklerObjectTests(AbstractPicklerUnpicklerObjectTests):

    pickler_class = pickle._Pickler
    unpickler_class = pickle._Unpickler


class PyDispatchTableTests(AbstractDispatchTableTests):

    pickler_class = pickle._Pickler

    def get_dispatch_table(self):
        return pickle.dispatch_table.copy()


class PyChainDispatchTableTests(AbstractDispatchTableTests):

    pickler_class = pickle._Pickler

    def get_dispatch_table(self):
        return collections.ChainMap({}, pickle.dispatch_table)


if has_c_implementation:
    class CPicklerTests(PyPicklerTests):
        pickler = _pickle.Pickler
        unpickler = _pickle.Unpickler

    class CPersPicklerTests(PyPersPicklerTests):
        pickler = _pickle.Pickler
        unpickler = _pickle.Unpickler

    class CDumpPickle_LoadPickle(PyPicklerTests):
        pickler = _pickle.Pickler
        unpickler = pickle._Unpickler

    class DumpPickle_CLoadPickle(PyPicklerTests):
        pickler = pickle._Pickler
        unpickler = _pickle.Unpickler

    class CPicklerUnpicklerObjectTests(AbstractPicklerUnpicklerObjectTests):
        pickler_class = _pickle.Pickler
        unpickler_class = _pickle.Unpickler

        def test_issue18339(self):
            unpickler = self.unpickler_class(io.BytesIO())
            with self.assertRaises(TypeError):
                unpickler.memo = object
            # used to cause a segfault
            with self.assertRaises(ValueError):
                unpickler.memo = {-1: None}
            unpickler.memo = {1: None}

    class CDispatchTableTests(AbstractDispatchTableTests):
        pickler_class = pickle.Pickler
        def get_dispatch_table(self):
            return pickle.dispatch_table.copy()

    class CChainDispatchTableTests(AbstractDispatchTableTests):
        pickler_class = pickle.Pickler
        def get_dispatch_table(self):
            return collections.ChainMap({}, pickle.dispatch_table)

    @support.cpython_only
    class SizeofTests(unittest.TestCase):
        check_sizeof = support.check_sizeof

        def test_pickler(self):
            basesize = support.calcobjsize('5P2n3i2n3iP')
            p = _pickle.Pickler(io.BytesIO())
            self.assertEqual(object.__sizeof__(p), basesize)
            MT_size = struct.calcsize('3nP0n')
            ME_size = struct.calcsize('Pn0P')
            check = self.check_sizeof
            check(p, basesize +
                MT_size + 8 * ME_size +  # Minimal memo table size.
                sys.getsizeof(b'x'*4096))  # Minimal write buffer size.
            for i in range(6):
                p.dump(chr(i))
            check(p, basesize +
                MT_size + 32 * ME_size +  # Size of memo table required to
                                          # save references to 6 objects.
                0)  # Write buffer is cleared after every dump().

        def test_unpickler(self):
            basesize = support.calcobjsize('2Pn2P 2P2n2i5P 2P3n6P2n2i')
            unpickler = _pickle.Unpickler
            P = struct.calcsize('P')  # Size of memo table entry.
            n = struct.calcsize('n')  # Size of mark table entry.
            check = self.check_sizeof
            for encoding in 'ASCII', 'UTF-16', 'latin-1':
                for errors in 'strict', 'replace':
                    u = unpickler(io.BytesIO(),
                                  encoding=encoding, errors=errors)
                    self.assertEqual(object.__sizeof__(u), basesize)
                    check(u, basesize +
                             32 * P +  # Minimal memo table size.
                             len(encoding) + 1 + len(errors) + 1)

            stdsize = basesize + len('ASCII') + 1 + len('strict') + 1
            def check_unpickler(data, memo_size, marks_size):
                dump = pickle.dumps(data)
                u = unpickler(io.BytesIO(dump),
                              encoding='ASCII', errors='strict')
                u.load()
                check(u, stdsize + memo_size * P + marks_size * n)

            check_unpickler(0, 32, 0)
            # 20 is minimal non-empty mark stack size.
            check_unpickler([0] * 100, 32, 20)
            # 128 is memo table size required to save references to 100 objects.
            check_unpickler([chr(i) for i in range(100)], 128, 20)
            def recurse(deep):
                data = 0
                for i in range(deep):
                    data = [data, data]
                return data
            check_unpickler(recurse(0), 32, 0)
            check_unpickler(recurse(1), 32, 20)
            check_unpickler(recurse(20), 32, 58)
            check_unpickler(recurse(50), 64, 58)
            check_unpickler(recurse(100), 128, 134)

            u = unpickler(io.BytesIO(pickle.dumps('a', 0)),
                          encoding='ASCII', errors='strict')
            u.load()
            check(u, stdsize + 32 * P + 2 + 1)


def test_main():
    tests = [PickleTests, PyPicklerTests, PyPersPicklerTests,
             PyDispatchTableTests, PyChainDispatchTableTests]
    if has_c_implementation:
        tests.extend([CPicklerTests, CPersPicklerTests,
                      CDumpPickle_LoadPickle, DumpPickle_CLoadPickle,
                      PyPicklerUnpicklerObjectTests,
                      CPicklerUnpicklerObjectTests,
                      CDispatchTableTests, CChainDispatchTableTests,
                      InMemoryPickleTests, SizeofTests])
    support.run_unittest(*tests)
    support.run_doctest(pickle)

if __name__ == "__main__":
    test_main()
