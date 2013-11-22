# Test the module type
import unittest
import weakref
from test.support import run_unittest, gc_collect
from test.script_helper import assert_python_ok

import sys
ModuleType = type(sys)

class FullLoader:
    @classmethod
    def module_repr(cls, m):
        return "<module '{}' (crafted)>".format(m.__name__)

class BareLoader:
    pass


class ModuleTests(unittest.TestCase):
    def test_uninitialized(self):
        # An uninitialized module has no __dict__ or __name__,
        # and __doc__ is None
        foo = ModuleType.__new__(ModuleType)
        self.assertTrue(foo.__dict__ is None)
        self.assertRaises(SystemError, dir, foo)
        try:
            s = foo.__name__
            self.fail("__name__ = %s" % repr(s))
        except AttributeError:
            pass
        self.assertEqual(foo.__doc__, ModuleType.__doc__)

    def test_no_docstring(self):
        # Regularly initialized module, no docstring
        foo = ModuleType("foo")
        self.assertEqual(foo.__name__, "foo")
        self.assertEqual(foo.__doc__, None)
        self.assertIs(foo.__loader__, None)
        self.assertIs(foo.__package__, None)
        self.assertIs(foo.__spec__, None)
        self.assertEqual(foo.__dict__, {"__name__": "foo", "__doc__": None,
                                        "__loader__": None, "__package__": None,
                                        "__spec__": None})

    def test_ascii_docstring(self):
        # ASCII docstring
        foo = ModuleType("foo", "foodoc")
        self.assertEqual(foo.__name__, "foo")
        self.assertEqual(foo.__doc__, "foodoc")
        self.assertEqual(foo.__dict__,
                         {"__name__": "foo", "__doc__": "foodoc",
                          "__loader__": None, "__package__": None,
                          "__spec__": None})

    def test_unicode_docstring(self):
        # Unicode docstring
        foo = ModuleType("foo", "foodoc\u1234")
        self.assertEqual(foo.__name__, "foo")
        self.assertEqual(foo.__doc__, "foodoc\u1234")
        self.assertEqual(foo.__dict__,
                         {"__name__": "foo", "__doc__": "foodoc\u1234",
                          "__loader__": None, "__package__": None,
                          "__spec__": None})

    def test_reinit(self):
        # Reinitialization should not replace the __dict__
        foo = ModuleType("foo", "foodoc\u1234")
        foo.bar = 42
        d = foo.__dict__
        foo.__init__("foo", "foodoc")
        self.assertEqual(foo.__name__, "foo")
        self.assertEqual(foo.__doc__, "foodoc")
        self.assertEqual(foo.bar, 42)
        self.assertEqual(foo.__dict__,
              {"__name__": "foo", "__doc__": "foodoc", "bar": 42,
               "__loader__": None, "__package__": None, "__spec__": None})
        self.assertTrue(foo.__dict__ is d)

    def test_dont_clear_dict(self):
        # See issue 7140.
        def f():
            foo = ModuleType("foo")
            foo.bar = 4
            return foo
        gc_collect()
        self.assertEqual(f().__dict__["bar"], 4)

    def test_clear_dict_in_ref_cycle(self):
        destroyed = []
        m = ModuleType("foo")
        m.destroyed = destroyed
        s = """class A:
    def __init__(self, l):
        self.l = l
    def __del__(self):
        self.l.append(1)
a = A(destroyed)"""
        exec(s, m.__dict__)
        del m
        gc_collect()
        self.assertEqual(destroyed, [1])

    def test_weakref(self):
        m = ModuleType("foo")
        wr = weakref.ref(m)
        self.assertIs(wr(), m)
        del m
        gc_collect()
        self.assertIs(wr(), None)

    def test_module_repr_minimal(self):
        # reprs when modules have no __file__, __name__, or __loader__
        m = ModuleType('foo')
        del m.__name__
        self.assertEqual(repr(m), "<module '?'>")

    def test_module_repr_with_name(self):
        m = ModuleType('foo')
        self.assertEqual(repr(m), "<module 'foo'>")

    def test_module_repr_with_name_and_filename(self):
        m = ModuleType('foo')
        m.__file__ = '/tmp/foo.py'
        self.assertEqual(repr(m), "<module 'foo' from '/tmp/foo.py'>")

    def test_module_repr_with_filename_only(self):
        m = ModuleType('foo')
        del m.__name__
        m.__file__ = '/tmp/foo.py'
        self.assertEqual(repr(m), "<module '?' from '/tmp/foo.py'>")

    def test_module_repr_with_loader_as_None(self):
        m = ModuleType('foo')
        assert m.__loader__ is None
        self.assertEqual(repr(m), "<module 'foo'>")

    def test_module_repr_with_bare_loader_but_no_name(self):
        m = ModuleType('foo')
        del m.__name__
        # Yes, a class not an instance.
        m.__loader__ = BareLoader
        loader_repr = repr(BareLoader)
        self.assertEqual(
            repr(m), "<module '?' ({})>".format(loader_repr))

    def test_module_repr_with_full_loader_but_no_name(self):
        # m.__loader__.module_repr() will fail because the module has no
        # m.__name__.  This exception will get suppressed and instead the
        # loader's repr will be used.
        m = ModuleType('foo')
        del m.__name__
        # Yes, a class not an instance.
        m.__loader__ = FullLoader
        loader_repr = repr(FullLoader)
        self.assertEqual(
            repr(m), "<module '?' ({})>".format(loader_repr))

    def test_module_repr_with_bare_loader(self):
        m = ModuleType('foo')
        # Yes, a class not an instance.
        m.__loader__ = BareLoader
        module_repr = repr(BareLoader)
        self.assertEqual(
            repr(m), "<module 'foo' ({})>".format(module_repr))

    def test_module_repr_with_full_loader(self):
        m = ModuleType('foo')
        # Yes, a class not an instance.
        m.__loader__ = FullLoader
        self.assertEqual(
            repr(m), "<module 'foo' (crafted)>")

    def test_module_repr_with_bare_loader_and_filename(self):
        # Because the loader has no module_repr(), use the file name.
        m = ModuleType('foo')
        # Yes, a class not an instance.
        m.__loader__ = BareLoader
        m.__file__ = '/tmp/foo.py'
        self.assertEqual(repr(m), "<module 'foo' from '/tmp/foo.py'>")

    def test_module_repr_with_full_loader_and_filename(self):
        # Even though the module has an __file__, use __loader__.module_repr()
        m = ModuleType('foo')
        # Yes, a class not an instance.
        m.__loader__ = FullLoader
        m.__file__ = '/tmp/foo.py'
        self.assertEqual(repr(m), "<module 'foo' (crafted)>")

    def test_module_repr_builtin(self):
        self.assertEqual(repr(sys), "<module 'sys' (built-in)>")

    def test_module_repr_source(self):
        r = repr(unittest)
        starts_with = "<module 'unittest' from '"
        ends_with = "__init__.py'>"
        self.assertEqual(r[:len(starts_with)], starts_with,
                         '{!r} does not start with {!r}'.format(r, starts_with))
        self.assertEqual(r[-len(ends_with):], ends_with,
                         '{!r} does not end with {!r}'.format(r, ends_with))

    def test_module_finalization_at_shutdown(self):
        # Module globals and builtins should still be available during shutdown
        rc, out, err = assert_python_ok("-c", "from test import final_a")
        self.assertFalse(err)
        lines = out.splitlines()
        self.assertEqual(set(lines), {
            b"x = a",
            b"x = b",
            b"final_a.x = a",
            b"final_b.x = b",
            b"len = len",
            b"shutil.rmtree = rmtree"})

    # frozen and namespace module reprs are tested in importlib.


def test_main():
    run_unittest(ModuleTests)


if __name__ == '__main__':
    test_main()
