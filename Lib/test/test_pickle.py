from _compat_pickle import (IMPORT_MAPPING, REVERSE_IMPORT_MAPPING,
                            NAME_MAPPING, REVERSE_NAME_MAPPING)
import builtins
import collections
import contextlib
import io
import pickle
import struct
import sys
import tempfile
import warnings
import weakref
from textwrap import dedent

import doctest
import unittest
from test import support
from test.support import import_helper, os_helper

from test.pickletester import AbstractHookTests
from test.pickletester import AbstractUnpickleTests
from test.pickletester import AbstractPicklingErrorTests
from test.pickletester import AbstractPickleTests
from test.pickletester import AbstractPickleModuleTests
from test.pickletester import AbstractPersistentPicklerTests
from test.pickletester import AbstractIdentityPersistentPicklerTests
from test.pickletester import AbstractPicklerUnpicklerObjectTests
from test.pickletester import AbstractDispatchTableTests
from test.pickletester import AbstractCustomPicklerClass
from test.pickletester import BigmemPickleTests

try:
    import _pickle
    has_c_implementation = True
except ImportError:
    has_c_implementation = False


class PyPickleTests(AbstractPickleModuleTests, unittest.TestCase):
    dump = staticmethod(pickle._dump)
    dumps = staticmethod(pickle._dumps)
    load = staticmethod(pickle._load)
    loads = staticmethod(pickle._loads)
    Pickler = pickle._Pickler
    Unpickler = pickle._Unpickler


class PyUnpicklerTests(AbstractUnpickleTests, unittest.TestCase):

    unpickler = pickle._Unpickler
    bad_stack_errors = (IndexError,)
    truncated_errors = (pickle.UnpicklingError, EOFError,
                        AttributeError, ValueError,
                        struct.error, IndexError, ImportError)

    def loads(self, buf, **kwds):
        f = io.BytesIO(buf)
        u = self.unpickler(f, **kwds)
        return u.load()


class PyPicklingErrorTests(AbstractPicklingErrorTests, unittest.TestCase):

    pickler = pickle._Pickler

    def dumps(self, arg, proto=None, **kwargs):
        f = io.BytesIO()
        p = self.pickler(f, proto, **kwargs)
        p.dump(arg)
        f.seek(0)
        return bytes(f.read())


class PyPicklerTests(AbstractPickleTests, unittest.TestCase):

    pickler = pickle._Pickler
    unpickler = pickle._Unpickler

    def dumps(self, arg, proto=None, **kwargs):
        f = io.BytesIO()
        p = self.pickler(f, proto, **kwargs)
        p.dump(arg)
        f.seek(0)
        return bytes(f.read())

    def loads(self, buf, **kwds):
        f = io.BytesIO(buf)
        u = self.unpickler(f, **kwds)
        return u.load()


class InMemoryPickleTests(AbstractPickleTests, AbstractUnpickleTests,
                          BigmemPickleTests, unittest.TestCase):

    bad_stack_errors = (pickle.UnpicklingError, IndexError)
    truncated_errors = (pickle.UnpicklingError, EOFError,
                        AttributeError, ValueError,
                        struct.error, IndexError, ImportError)

    def dumps(self, arg, protocol=None, **kwargs):
        return pickle.dumps(arg, protocol, **kwargs)

    def loads(self, buf, **kwds):
        return pickle.loads(buf, **kwds)

    test_framed_write_sizes_with_delayed_writer = None
    test_find_class = None
    test_custom_find_class = None


class PersistentPicklerUnpicklerMixin(object):

    def dumps(self, arg, proto=None):
        class PersPickler(self.pickler):
            def persistent_id(subself, obj):
                return self.persistent_id(obj)
        f = io.BytesIO()
        p = PersPickler(f, proto)
        p.dump(arg)
        return f.getvalue()

    def loads(self, buf, **kwds):
        class PersUnpickler(self.unpickler):
            def persistent_load(subself, obj):
                return self.persistent_load(obj)
        f = io.BytesIO(buf)
        u = PersUnpickler(f, **kwds)
        return u.load()


class PyPersPicklerTests(AbstractPersistentPicklerTests,
                         PersistentPicklerUnpicklerMixin, unittest.TestCase):

    pickler = pickle._Pickler
    unpickler = pickle._Unpickler


class PyIdPersPicklerTests(AbstractIdentityPersistentPicklerTests,
                           PersistentPicklerUnpicklerMixin, unittest.TestCase):

    pickler = pickle._Pickler
    unpickler = pickle._Unpickler
    persistent_load_error = pickle.UnpicklingError

    @support.cpython_only
    def test_pickler_reference_cycle(self):
        def check(Pickler):
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                f = io.BytesIO()
                pickler = Pickler(f, proto)
                pickler.dump('abc')
                self.assertEqual(self.loads(f.getvalue()), 'abc')
            pickler = Pickler(io.BytesIO())
            self.assertEqual(pickler.persistent_id('def'), 'def')
            r = weakref.ref(pickler)
            del pickler
            self.assertIsNone(r())

        class PersPickler(self.pickler):
            def persistent_id(subself, obj):
                return obj
        check(PersPickler)

        class PersPickler(self.pickler):
            @classmethod
            def persistent_id(cls, obj):
                return obj
        check(PersPickler)

        class PersPickler(self.pickler):
            @staticmethod
            def persistent_id(obj):
                return obj
        check(PersPickler)

    @support.cpython_only
    def test_custom_pickler_dispatch_table_memleak(self):
        # See https://github.com/python/cpython/issues/89988

        class Pickler(self.pickler):
            def __init__(self, *args, **kwargs):
                self.dispatch_table = table
                super().__init__(*args, **kwargs)

        class DispatchTable:
            pass

        table = DispatchTable()
        pickler = Pickler(io.BytesIO())
        self.assertIs(pickler.dispatch_table, table)
        table_ref = weakref.ref(table)
        self.assertIsNotNone(table_ref())
        del pickler
        del table
        support.gc_collect()
        self.assertIsNone(table_ref())

    @support.cpython_only
    def test_unpickler_reference_cycle(self):
        def check(Unpickler):
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                unpickler = Unpickler(io.BytesIO(self.dumps('abc', proto)))
                self.assertEqual(unpickler.load(), 'abc')
            unpickler = Unpickler(io.BytesIO())
            self.assertEqual(unpickler.persistent_load('def'), 'def')
            r = weakref.ref(unpickler)
            del unpickler
            self.assertIsNone(r())

        class PersUnpickler(self.unpickler):
            def persistent_load(subself, pid):
                return pid
        check(PersUnpickler)

        class PersUnpickler(self.unpickler):
            @classmethod
            def persistent_load(cls, pid):
                return pid
        check(PersUnpickler)

        class PersUnpickler(self.unpickler):
            @staticmethod
            def persistent_load(pid):
                return pid
        check(PersUnpickler)

    def test_pickler_super(self):
        class PersPickler(self.pickler):
            def persistent_id(subself, obj):
                called.append(obj)
                self.assertIsNone(super().persistent_id(obj))
                return obj

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            f = io.BytesIO()
            pickler = PersPickler(f, proto)
            called = []
            pickler.dump('abc')
            self.assertEqual(called, ['abc'])
            self.assertEqual(self.loads(f.getvalue()), 'abc')

    def test_unpickler_super(self):
        class PersUnpickler(self.unpickler):
            def persistent_load(subself, pid):
                called.append(pid)
                with self.assertRaises(self.persistent_load_error):
                    super().persistent_load(pid)
                return pid

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            unpickler = PersUnpickler(io.BytesIO(self.dumps('abc', proto)))
            called = []
            self.assertEqual(unpickler.load(), 'abc')
            self.assertEqual(called, ['abc'])

    def test_pickler_instance_attribute(self):
        def persistent_id(obj):
            called.append(obj)
            return obj

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            f = io.BytesIO()
            pickler = self.pickler(f, proto)
            called = []
            old_persistent_id = pickler.persistent_id
            pickler.persistent_id = persistent_id
            self.assertEqual(pickler.persistent_id, persistent_id)
            pickler.dump('abc')
            self.assertEqual(called, ['abc'])
            self.assertEqual(self.loads(f.getvalue()), 'abc')
            del pickler.persistent_id
            self.assertEqual(pickler.persistent_id, old_persistent_id)

    def test_unpickler_instance_attribute(self):
        def persistent_load(pid):
            called.append(pid)
            return pid

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            unpickler = self.unpickler(io.BytesIO(self.dumps('abc', proto)))
            called = []
            old_persistent_load = unpickler.persistent_load
            unpickler.persistent_load = persistent_load
            self.assertEqual(unpickler.persistent_load, persistent_load)
            self.assertEqual(unpickler.load(), 'abc')
            self.assertEqual(called, ['abc'])
            del unpickler.persistent_load
            self.assertEqual(unpickler.persistent_load, old_persistent_load)

    def test_pickler_super_instance_attribute(self):
        class PersPickler(self.pickler):
            def persistent_id(subself, obj):
                raise AssertionError('should never be called')
            def _persistent_id(subself, obj):
                called.append(obj)
                self.assertIsNone(super().persistent_id(obj))
                return obj

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            f = io.BytesIO()
            pickler = PersPickler(f, proto)
            called = []
            old_persistent_id = pickler.persistent_id
            pickler.persistent_id = pickler._persistent_id
            self.assertEqual(pickler.persistent_id, pickler._persistent_id)
            pickler.dump('abc')
            self.assertEqual(called, ['abc'])
            self.assertEqual(self.loads(f.getvalue()), 'abc')
            del pickler.persistent_id
            self.assertEqual(pickler.persistent_id, old_persistent_id)

    def test_unpickler_super_instance_attribute(self):
        class PersUnpickler(self.unpickler):
            def persistent_load(subself, pid):
                raise AssertionError('should never be called')
            def _persistent_load(subself, pid):
                called.append(pid)
                with self.assertRaises(self.persistent_load_error):
                    super().persistent_load(pid)
                return pid

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            unpickler = PersUnpickler(io.BytesIO(self.dumps('abc', proto)))
            called = []
            old_persistent_load = unpickler.persistent_load
            unpickler.persistent_load = unpickler._persistent_load
            self.assertEqual(unpickler.persistent_load, unpickler._persistent_load)
            self.assertEqual(unpickler.load(), 'abc')
            self.assertEqual(called, ['abc'])
            del unpickler.persistent_load
            self.assertEqual(unpickler.persistent_load, old_persistent_load)


class PyPicklerUnpicklerObjectTests(AbstractPicklerUnpicklerObjectTests, unittest.TestCase):

    pickler_class = pickle._Pickler
    unpickler_class = pickle._Unpickler


class PyDispatchTableTests(AbstractDispatchTableTests, unittest.TestCase):

    pickler_class = pickle._Pickler

    def get_dispatch_table(self):
        return pickle.dispatch_table.copy()


class PyChainDispatchTableTests(AbstractDispatchTableTests, unittest.TestCase):

    pickler_class = pickle._Pickler

    def get_dispatch_table(self):
        return collections.ChainMap({}, pickle.dispatch_table)


class PyPicklerHookTests(AbstractHookTests, unittest.TestCase):
    class CustomPyPicklerClass(pickle._Pickler,
                               AbstractCustomPicklerClass):
        pass
    pickler_class = CustomPyPicklerClass


if has_c_implementation:
    class CPickleTests(AbstractPickleModuleTests, unittest.TestCase):
        from _pickle import dump, dumps, load, loads, Pickler, Unpickler

    class CUnpicklerTests(PyUnpicklerTests):
        unpickler = _pickle.Unpickler
        bad_stack_errors = (pickle.UnpicklingError,)
        truncated_errors = (pickle.UnpicklingError,)

    class CPicklingErrorTests(PyPicklingErrorTests):
        pickler = _pickle.Pickler

    class CPicklerTests(PyPicklerTests):
        pickler = _pickle.Pickler
        unpickler = _pickle.Unpickler

    class CPersPicklerTests(PyPersPicklerTests):
        pickler = _pickle.Pickler
        unpickler = _pickle.Unpickler

    class CIdPersPicklerTests(PyIdPersPicklerTests):
        pickler = _pickle.Pickler
        unpickler = _pickle.Unpickler
        persistent_load_error = _pickle.UnpicklingError

    class CDumpPickle_LoadPickle(PyPicklerTests):
        pickler = _pickle.Pickler
        unpickler = pickle._Unpickler

    class DumpPickle_CLoadPickle(PyPicklerTests):
        pickler = pickle._Pickler
        unpickler = _pickle.Unpickler

    class CPicklerUnpicklerObjectTests(AbstractPicklerUnpicklerObjectTests, unittest.TestCase):
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

    class CDispatchTableTests(AbstractDispatchTableTests, unittest.TestCase):
        pickler_class = pickle.Pickler
        def get_dispatch_table(self):
            return pickle.dispatch_table.copy()

    class CChainDispatchTableTests(AbstractDispatchTableTests, unittest.TestCase):
        pickler_class = pickle.Pickler
        def get_dispatch_table(self):
            return collections.ChainMap({}, pickle.dispatch_table)

    class CPicklerHookTests(AbstractHookTests, unittest.TestCase):
        class CustomCPicklerClass(_pickle.Pickler, AbstractCustomPicklerClass):
            pass
        pickler_class = CustomCPicklerClass

    @support.cpython_only
    class HeapTypesTests(unittest.TestCase):
        def setUp(self):
            pickler = _pickle.Pickler(io.BytesIO())
            unpickler = _pickle.Unpickler(io.BytesIO())

            self._types = (
                _pickle.Pickler,
                _pickle.Unpickler,
                type(pickler.memo),
                type(unpickler.memo),

                # We cannot test the _pickle.Pdata;
                # there's no way to get to it.
            )

        def test_have_gc(self):
            import gc
            for tp in self._types:
                with self.subTest(tp=tp):
                    self.assertTrue(gc.is_tracked(tp))

        def test_immutable(self):
            for tp in self._types:
                with self.subTest(tp=tp):
                    with self.assertRaisesRegex(TypeError, "immutable"):
                        tp.foo = "bar"

    @support.cpython_only
    class SizeofTests(unittest.TestCase):
        check_sizeof = support.check_sizeof

        def test_pickler(self):
            basesize = support.calcobjsize('7P2n3i2n3i2P')
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
            basesize = support.calcobjsize('2P2n2P 2P2n2i5P 2P3n8P2n2i')
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
            check_unpickler(recurse(20), 32, 20)
            check_unpickler(recurse(50), 64, 60)
            if not (support.is_wasi and support.Py_DEBUG):
                # stack depth too shallow in pydebug WASI.
                check_unpickler(recurse(100), 128, 140)

            u = unpickler(io.BytesIO(pickle.dumps('a', 0)),
                          encoding='ASCII', errors='strict')
            u.load()
            check(u, stdsize + 32 * P + 2 + 1)


ALT_IMPORT_MAPPING = {
    ('_elementtree', 'xml.etree.ElementTree'),
    ('cPickle', 'pickle'),
    ('StringIO', 'io'),
    ('cStringIO', 'io'),
}

ALT_NAME_MAPPING = {
    ('__builtin__', 'basestring', 'builtins', 'str'),
    ('exceptions', 'StandardError', 'builtins', 'Exception'),
    ('UserDict', 'UserDict', 'collections', 'UserDict'),
    ('socket', '_socketobject', 'socket', 'SocketType'),
}

def mapping(module, name):
    if (module, name) in NAME_MAPPING:
        module, name = NAME_MAPPING[(module, name)]
    elif module in IMPORT_MAPPING:
        module = IMPORT_MAPPING[module]
    return module, name

def reverse_mapping(module, name):
    if (module, name) in REVERSE_NAME_MAPPING:
        module, name = REVERSE_NAME_MAPPING[(module, name)]
    elif module in REVERSE_IMPORT_MAPPING:
        module = REVERSE_IMPORT_MAPPING[module]
    return module, name

def getmodule(module):
    try:
        return sys.modules[module]
    except KeyError:
        try:
            with warnings.catch_warnings():
                action = 'always' if support.verbose else 'ignore'
                warnings.simplefilter(action, DeprecationWarning)
                __import__(module)
        except AttributeError as exc:
            if support.verbose:
                print("Can't import module %r: %s" % (module, exc))
            raise ImportError
        except ImportError as exc:
            if support.verbose:
                print(exc)
            raise
        return sys.modules[module]

def getattribute(module, name):
    obj = getmodule(module)
    for n in name.split('.'):
        obj = getattr(obj, n)
    return obj

def get_exceptions(mod):
    for name in dir(mod):
        attr = getattr(mod, name)
        if isinstance(attr, type) and issubclass(attr, BaseException):
            yield name, attr

class CompatPickleTests(unittest.TestCase):
    def test_import(self):
        modules = set(IMPORT_MAPPING.values())
        modules |= set(REVERSE_IMPORT_MAPPING)
        modules |= {module for module, name in REVERSE_NAME_MAPPING}
        modules |= {module for module, name in NAME_MAPPING.values()}
        for module in modules:
            try:
                getmodule(module)
            except ImportError:
                pass

    def test_import_mapping(self):
        for module3, module2 in REVERSE_IMPORT_MAPPING.items():
            with self.subTest((module3, module2)):
                try:
                    getmodule(module3)
                except ImportError:
                    pass
                if module3[:1] != '_':
                    self.assertIn(module2, IMPORT_MAPPING)
                    self.assertEqual(IMPORT_MAPPING[module2], module3)

    def test_name_mapping(self):
        for (module3, name3), (module2, name2) in REVERSE_NAME_MAPPING.items():
            with self.subTest(((module3, name3), (module2, name2))):
                if (module2, name2) == ('exceptions', 'OSError'):
                    attr = getattribute(module3, name3)
                    self.assertTrue(issubclass(attr, OSError))
                elif (module2, name2) == ('exceptions', 'ImportError'):
                    attr = getattribute(module3, name3)
                    self.assertTrue(issubclass(attr, ImportError))
                else:
                    module, name = mapping(module2, name2)
                    if module3[:1] != '_':
                        self.assertEqual((module, name), (module3, name3))
                    try:
                        attr = getattribute(module3, name3)
                    except ImportError:
                        pass
                    else:
                        self.assertEqual(getattribute(module, name), attr)

    def test_reverse_import_mapping(self):
        for module2, module3 in IMPORT_MAPPING.items():
            with self.subTest((module2, module3)):
                try:
                    getmodule(module3)
                except ImportError as exc:
                    if support.verbose:
                        print(exc)
                if ((module2, module3) not in ALT_IMPORT_MAPPING and
                    REVERSE_IMPORT_MAPPING.get(module3, None) != module2):
                    for (m3, n3), (m2, n2) in REVERSE_NAME_MAPPING.items():
                        if (module3, module2) == (m3, m2):
                            break
                    else:
                        self.fail('No reverse mapping from %r to %r' %
                                  (module3, module2))
                module = REVERSE_IMPORT_MAPPING.get(module3, module3)
                module = IMPORT_MAPPING.get(module, module)
                self.assertEqual(module, module3)

    def test_reverse_name_mapping(self):
        for (module2, name2), (module3, name3) in NAME_MAPPING.items():
            with self.subTest(((module2, name2), (module3, name3))):
                try:
                    attr = getattribute(module3, name3)
                except ImportError:
                    pass
                module, name = reverse_mapping(module3, name3)
                if (module2, name2, module3, name3) not in ALT_NAME_MAPPING:
                    self.assertEqual((module, name), (module2, name2))
                module, name = mapping(module, name)
                self.assertEqual((module, name), (module3, name3))

    def test_exceptions(self):
        self.assertEqual(mapping('exceptions', 'StandardError'),
                         ('builtins', 'Exception'))
        self.assertEqual(mapping('exceptions', 'Exception'),
                         ('builtins', 'Exception'))
        self.assertEqual(reverse_mapping('builtins', 'Exception'),
                         ('exceptions', 'Exception'))
        self.assertEqual(mapping('exceptions', 'OSError'),
                         ('builtins', 'OSError'))
        self.assertEqual(reverse_mapping('builtins', 'OSError'),
                         ('exceptions', 'OSError'))

        for name, exc in get_exceptions(builtins):
            with self.subTest(name):
                if exc in (BlockingIOError,
                           ResourceWarning,
                           StopAsyncIteration,
                           PythonFinalizationError,
                           RecursionError,
                           EncodingWarning,
                           BaseExceptionGroup,
                           ExceptionGroup,
                           _IncompleteInputError):
                    continue
                if exc is not OSError and issubclass(exc, OSError):
                    self.assertEqual(reverse_mapping('builtins', name),
                                     ('exceptions', 'OSError'))
                elif exc is not ImportError and issubclass(exc, ImportError):
                    self.assertEqual(reverse_mapping('builtins', name),
                                     ('exceptions', 'ImportError'))
                    self.assertEqual(mapping('exceptions', name),
                                     ('exceptions', name))
                else:
                    self.assertEqual(reverse_mapping('builtins', name),
                                     ('exceptions', name))
                    self.assertEqual(mapping('exceptions', name),
                                     ('builtins', name))

    def test_multiprocessing_exceptions(self):
        module = import_helper.import_module('multiprocessing.context')
        for name, exc in get_exceptions(module):
            if issubclass(exc, Warning):
                continue
            with self.subTest(name):
                self.assertEqual(reverse_mapping('multiprocessing.context', name),
                                 ('multiprocessing', name))
                self.assertEqual(mapping('multiprocessing', name),
                                 ('multiprocessing.context', name))


class CommandLineTest(unittest.TestCase):
    def setUp(self):
        self.filename = tempfile.mktemp()
        self.addCleanup(os_helper.unlink, self.filename)

    @staticmethod
    def text_normalize(string):
        """Dedent *string* and strip it from its surrounding whitespaces.

        This method is used by the other utility functions so that any
        string to write or to match against can be freely indented.
        """
        return dedent(string).strip()

    def set_pickle_data(self, data):
        with open(self.filename, 'wb') as f:
            pickle.dump(data, f)

    def invoke_pickle(self, *flags):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            pickle._main(args=[*flags, self.filename])
        return self.text_normalize(output.getvalue())

    def test_invocation(self):
        # test 'python -m pickle pickle_file'
        data = {
            'a': [1, 2.0, 3+4j],
            'b': ('character string', b'byte string'),
            'c': 'string'
        }
        expect = '''
            {'a': [1, 2.0, (3+4j)],
             'b': ('character string', b'byte string'),
             'c': 'string'}
        '''
        self.set_pickle_data(data)

        with self.subTest(data=data):
            res = self.invoke_pickle()
            expect = self.text_normalize(expect)
            self.assertListEqual(res.splitlines(), expect.splitlines())

    def test_unknown_flag(self):
        stderr = io.StringIO()
        with self.assertRaises(SystemExit):
            # check that the parser help is shown
            with contextlib.redirect_stderr(stderr):
                _ = self.invoke_pickle('--unknown')
        self.assertStartsWith(stderr.getvalue(), 'usage: ')


def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite(pickle))
    return tests


if __name__ == "__main__":
    unittest.main()
