import types
import unittest


def global_function():
    def inner_function():
        class LocalClass:
            pass
        global inner_global_function
        def inner_global_function():
            def inner_function2():
                pass
            return inner_function2
        return LocalClass
    return lambda: inner_function


class FuncAttrsTest(unittest.TestCase):
    def setUp(self):
        class F:
            def a(self):
                pass
        def b():
            return 3
        self.fi = F()
        self.F = F
        self.b = b

    def cannot_set_attr(self, obj, name, value, exceptions):
        try:
            setattr(obj, name, value)
        except exceptions:
            pass
        else:
            self.fail("shouldn't be able to set %s to %r" % (name, value))
        try:
            delattr(obj, name)
        except exceptions:
            pass
        else:
            self.fail("shouldn't be able to del %s" % name)


class FunctionPropertiesTest(FuncAttrsTest):
    # Include the external setUp method that is common to all tests
    def test_module(self):
        self.assertEqual(self.b.__module__, __name__)

    def test_dir_includes_correct_attrs(self):
        self.b.known_attr = 7
        self.assertIn('known_attr', dir(self.b),
            "set attributes not in dir listing of method")
        # Test on underlying function object of method
        self.F.a.known_attr = 7
        self.assertIn('known_attr', dir(self.fi.a), "set attribute on function "
                     "implementations, should show up in next dir")

    def test_duplicate_function_equality(self):
        # Body of `duplicate' is the exact same as self.b
        def duplicate():
            'my docstring'
            return 3
        self.assertNotEqual(self.b, duplicate)

    def test_copying___code__(self):
        def test(): pass
        self.assertEqual(test(), None)
        test.__code__ = self.b.__code__
        self.assertEqual(test(), 3) # self.b always returns 3, arbitrarily

    def test___globals__(self):
        self.assertIs(self.b.__globals__, globals())
        self.cannot_set_attr(self.b, '__globals__', 2,
                             (AttributeError, TypeError))

    def test___closure__(self):
        a = 12
        def f(): print(a)
        c = f.__closure__
        self.assertIsInstance(c, tuple)
        self.assertEqual(len(c), 1)
        # don't have a type object handy
        self.assertEqual(c[0].__class__.__name__, "cell")
        self.cannot_set_attr(f, "__closure__", c, AttributeError)

    def test_empty_cell(self):
        def f(): print(a)
        try:
            f.__closure__[0].cell_contents
        except ValueError:
            pass
        else:
            self.fail("shouldn't be able to read an empty cell")
        a = 12

    def test_set_cell(self):
        a = 12
        def f(): return a
        c = f.__closure__
        c[0].cell_contents = 9
        self.assertEqual(c[0].cell_contents, 9)
        self.assertEqual(f(), 9)
        self.assertEqual(a, 9)
        del c[0].cell_contents
        try:
            c[0].cell_contents
        except ValueError:
            pass
        else:
            self.fail("shouldn't be able to read an empty cell")
        with self.assertRaises(NameError):
            f()
        with self.assertRaises(UnboundLocalError):
            print(a)

    def test___name__(self):
        self.assertEqual(self.b.__name__, 'b')
        self.b.__name__ = 'c'
        self.assertEqual(self.b.__name__, 'c')
        self.b.__name__ = 'd'
        self.assertEqual(self.b.__name__, 'd')
        # __name__ and __name__ must be a string
        self.cannot_set_attr(self.b, '__name__', 7, TypeError)
        # __name__ must be available when in restricted mode. Exec will raise
        # AttributeError if __name__ is not available on f.
        s = """def f(): pass\nf.__name__"""
        exec(s, {'__builtins__': {}})
        # Test on methods, too
        self.assertEqual(self.fi.a.__name__, 'a')
        self.cannot_set_attr(self.fi.a, "__name__", 'a', AttributeError)

    def test___qualname__(self):
        # PEP 3155
        self.assertEqual(self.b.__qualname__, 'FuncAttrsTest.setUp.<locals>.b')
        self.assertEqual(FuncAttrsTest.setUp.__qualname__, 'FuncAttrsTest.setUp')
        self.assertEqual(global_function.__qualname__, 'global_function')
        self.assertEqual(global_function().__qualname__,
                         'global_function.<locals>.<lambda>')
        self.assertEqual(global_function()().__qualname__,
                         'global_function.<locals>.inner_function')
        self.assertEqual(global_function()()().__qualname__,
                         'global_function.<locals>.inner_function.<locals>.LocalClass')
        self.assertEqual(inner_global_function.__qualname__, 'inner_global_function')
        self.assertEqual(inner_global_function().__qualname__, 'inner_global_function.<locals>.inner_function2')
        self.b.__qualname__ = 'c'
        self.assertEqual(self.b.__qualname__, 'c')
        self.b.__qualname__ = 'd'
        self.assertEqual(self.b.__qualname__, 'd')
        # __qualname__ must be a string
        self.cannot_set_attr(self.b, '__qualname__', 7, TypeError)

    def test___code__(self):
        num_one, num_two = 7, 8
        def a(): pass
        def b(): return 12
        def c(): return num_one
        def d(): return num_two
        def e(): return num_one, num_two
        for func in [a, b, c, d, e]:
            self.assertEqual(type(func.__code__), types.CodeType)
        self.assertEqual(c(), 7)
        self.assertEqual(d(), 8)
        d.__code__ = c.__code__
        self.assertEqual(c.__code__, d.__code__)
        self.assertEqual(c(), 7)
        # self.assertEqual(d(), 7)
        try:
            b.__code__ = c.__code__
        except ValueError:
            pass
        else:
            self.fail("__code__ with different numbers of free vars should "
                      "not be possible")
        try:
            e.__code__ = d.__code__
        except ValueError:
            pass
        else:
            self.fail("__code__ with different numbers of free vars should "
                      "not be possible")

    def test_blank_func_defaults(self):
        self.assertEqual(self.b.__defaults__, None)
        del self.b.__defaults__
        self.assertEqual(self.b.__defaults__, None)

    def test_func_default_args(self):
        def first_func(a, b):
            return a+b
        def second_func(a=1, b=2):
            return a+b
        self.assertEqual(first_func.__defaults__, None)
        self.assertEqual(second_func.__defaults__, (1, 2))
        first_func.__defaults__ = (1, 2)
        self.assertEqual(first_func.__defaults__, (1, 2))
        self.assertEqual(first_func(), 3)
        self.assertEqual(first_func(3), 5)
        self.assertEqual(first_func(3, 5), 8)
        del second_func.__defaults__
        self.assertEqual(second_func.__defaults__, None)
        try:
            second_func()
        except TypeError:
            pass
        else:
            self.fail("__defaults__ does not update; deleting it does not "
                      "remove requirement")


class InstancemethodAttrTest(FuncAttrsTest):

    def test___class__(self):
        self.assertEqual(self.fi.a.__self__.__class__, self.F)
        self.cannot_set_attr(self.fi.a, "__class__", self.F, TypeError)

    def test___func__(self):
        self.assertEqual(self.fi.a.__func__, self.F.a)
        self.cannot_set_attr(self.fi.a, "__func__", self.F.a, AttributeError)

    def test___self__(self):
        self.assertEqual(self.fi.a.__self__, self.fi)
        self.cannot_set_attr(self.fi.a, "__self__", self.fi, AttributeError)

    def test___func___non_method(self):
        # Behavior should be the same when a method is added via an attr
        # assignment
        self.fi.id = types.MethodType(id, self.fi)
        self.assertEqual(self.fi.id(), id(self.fi))
        # Test usage
        try:
            self.fi.id.unknown_attr
        except AttributeError:
            pass
        else:
            self.fail("using unknown attributes should raise AttributeError")
        # Test assignment and deletion
        self.cannot_set_attr(self.fi.id, 'unknown_attr', 2, AttributeError)


class ArbitraryFunctionAttrTest(FuncAttrsTest):
    def test_set_attr(self):
        self.b.known_attr = 7
        self.assertEqual(self.b.known_attr, 7)
        try:
            self.fi.a.known_attr = 7
        except AttributeError:
            pass
        else:
            self.fail("setting attributes on methods should raise error")

    def test_delete_unknown_attr(self):
        try:
            del self.b.unknown_attr
        except AttributeError:
            pass
        else:
            self.fail("deleting unknown attribute should raise TypeError")

    def test_unset_attr(self):
        for func in [self.b, self.fi.a]:
            try:
                func.non_existent_attr
            except AttributeError:
                pass
            else:
                self.fail("using unknown attributes should raise "
                          "AttributeError")


class FunctionDictsTest(FuncAttrsTest):
    def test_setting_dict_to_invalid(self):
        self.cannot_set_attr(self.b, '__dict__', None, TypeError)
        from collections import UserDict
        d = UserDict({'known_attr': 7})
        self.cannot_set_attr(self.fi.a.__func__, '__dict__', d, TypeError)

    def test_setting_dict_to_valid(self):
        d = {'known_attr': 7}
        self.b.__dict__ = d
        # Test assignment
        self.assertIs(d, self.b.__dict__)
        # ... and on all the different ways of referencing the method's func
        self.F.a.__dict__ = d
        self.assertIs(d, self.fi.a.__func__.__dict__)
        self.assertIs(d, self.fi.a.__dict__)
        # Test value
        self.assertEqual(self.b.known_attr, 7)
        self.assertEqual(self.b.__dict__['known_attr'], 7)
        # ... and again, on all the different method's names
        self.assertEqual(self.fi.a.__func__.known_attr, 7)
        self.assertEqual(self.fi.a.known_attr, 7)

    def test_delete___dict__(self):
        try:
            del self.b.__dict__
        except TypeError:
            pass
        else:
            self.fail("deleting function dictionary should raise TypeError")

    def test_unassigned_dict(self):
        self.assertEqual(self.b.__dict__, {})

    def test_func_as_dict_key(self):
        value = "Some string"
        d = {}
        d[self.b] = value
        self.assertEqual(d[self.b], value)


class FunctionDocstringTest(FuncAttrsTest):
    def test_set_docstring_attr(self):
        self.assertEqual(self.b.__doc__, None)
        docstr = "A test method that does nothing"
        self.b.__doc__ = docstr
        self.F.a.__doc__ = docstr
        self.assertEqual(self.b.__doc__, docstr)
        self.assertEqual(self.fi.a.__doc__, docstr)
        self.cannot_set_attr(self.fi.a, "__doc__", docstr, AttributeError)

    def test_delete_docstring(self):
        self.b.__doc__ = "The docstring"
        del self.b.__doc__
        self.assertEqual(self.b.__doc__, None)


def cell(value):
    """Create a cell containing the given value."""
    def f():
        print(a)
    a = value
    return f.__closure__[0]

def empty_cell(empty=True):
    """Create an empty cell."""
    def f():
        print(a)
    # the intent of the following line is simply "if False:";  it's
    # spelt this way to avoid the danger that a future optimization
    # might simply remove an "if False:" code block.
    if not empty:
        a = 1729
    return f.__closure__[0]


class CellTest(unittest.TestCase):
    def test_comparison(self):
        # These tests are here simply to exercise the comparison code;
        # their presence should not be interpreted as providing any
        # guarantees about the semantics (or even existence) of cell
        # comparisons in future versions of CPython.
        self.assertTrue(cell(2) < cell(3))
        self.assertTrue(empty_cell() < cell('saturday'))
        self.assertTrue(empty_cell() == empty_cell())
        self.assertTrue(cell(-36) == cell(-36.0))
        self.assertTrue(cell(True) > empty_cell())


class StaticMethodAttrsTest(unittest.TestCase):
    def test_func_attribute(self):
        def f():
            pass

        c = classmethod(f)
        self.assertTrue(c.__func__ is f)

        s = staticmethod(f)
        self.assertTrue(s.__func__ is f)


class BuiltinFunctionPropertiesTest(unittest.TestCase):
    # XXX Not sure where this should really go since I can't find a
    # test module specifically for builtin_function_or_method.

    def test_builtin__qualname__(self):
        import time

        # builtin function:
        self.assertEqual(len.__qualname__, 'len')
        self.assertEqual(time.time.__qualname__, 'time')

        # builtin classmethod:
        self.assertEqual(dict.fromkeys.__qualname__, 'dict.fromkeys')
        self.assertEqual(float.__getformat__.__qualname__,
                         'float.__getformat__')

        # builtin staticmethod:
        self.assertEqual(str.maketrans.__qualname__, 'str.maketrans')
        self.assertEqual(bytes.maketrans.__qualname__, 'bytes.maketrans')

        # builtin bound instance method:
        self.assertEqual([1, 2, 3].append.__qualname__, 'list.append')
        self.assertEqual({'foo': 'bar'}.pop.__qualname__, 'dict.pop')


if __name__ == "__main__":
    unittest.main()
