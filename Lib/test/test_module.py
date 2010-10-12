# Test the module type
import unittest
from test.support import run_unittest, gc_collect

import sys
ModuleType = type(sys)

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
        self.assertEqual(foo.__dict__, {"__name__": "foo", "__doc__": None})

    def test_ascii_docstring(self):
        # ASCII docstring
        foo = ModuleType("foo", "foodoc")
        self.assertEqual(foo.__name__, "foo")
        self.assertEqual(foo.__doc__, "foodoc")
        self.assertEqual(foo.__dict__,
                         {"__name__": "foo", "__doc__": "foodoc"})

    def test_unicode_docstring(self):
        # Unicode docstring
        foo = ModuleType("foo", "foodoc\u1234")
        self.assertEqual(foo.__name__, "foo")
        self.assertEqual(foo.__doc__, "foodoc\u1234")
        self.assertEqual(foo.__dict__,
                         {"__name__": "foo", "__doc__": "foodoc\u1234"})

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
              {"__name__": "foo", "__doc__": "foodoc", "bar": 42})
        self.assertTrue(foo.__dict__ is d)

    @unittest.expectedFailure
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
    def __del__(self):
        destroyed.append(1)
a = A()"""
        exec(s, m.__dict__)
        del m
        gc_collect()
        self.assertEqual(destroyed, [1])

def test_main():
    run_unittest(ModuleTests)

if __name__ == '__main__':
    test_main()
