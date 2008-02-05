from test import test_support
import types
import unittest

class FuncAttrsTest(unittest.TestCase):
    def setUp(self):
        class F:
            def a(self):
                pass
        def b():
            return 3
        self.f = F
        self.fi = F()
        self.b = b

    def cannot_set_attr(self,obj, name, value, exceptions):
        # This method is not called as a test (name doesn't start with 'test'),
        # but may be used by other tests.
        try: setattr(obj, name, value)
        except exceptions: pass
        else: self.fail("shouldn't be able to set %s to %r" % (name, value))
        try: delattr(obj, name)
        except exceptions: pass
        else: self.fail("shouldn't be able to del %s" % name)


class FunctionPropertiesTest(FuncAttrsTest):
    # Include the external setUp method that is common to all tests
    def test_module(self):
        self.assertEqual(self.b.__module__, __name__)

    def test_dir_includes_correct_attrs(self):
        self.b.known_attr = 7
        self.assert_('known_attr' in dir(self.b),
            "set attributes not in dir listing of method")
        # Test on underlying function object of method
        self.f.a.im_func.known_attr = 7
        self.assert_('known_attr' in dir(self.f.a),
            "set attribute on unbound method implementation in class not in "
                     "dir")
        self.assert_('known_attr' in dir(self.fi.a),
            "set attribute on unbound method implementations, should show up"
                     " in next dir")

    def test_duplicate_function_equality(self):
        # Body of `duplicate' is the exact same as self.b
        def duplicate():
            'my docstring'
            return 3
        self.assertNotEqual(self.b, duplicate)

    def test_copying_func_code(self):
        def test(): pass
        self.assertEqual(test(), None)
        test.func_code = self.b.func_code
        self.assertEqual(test(), 3) # self.b always returns 3, arbitrarily

    def test_func_globals(self):
        self.assertEqual(self.b.func_globals, globals())
        self.cannot_set_attr(self.b, 'func_globals', 2, TypeError)

    def test_func_name(self):
        self.assertEqual(self.b.__name__, 'b')
        self.assertEqual(self.b.func_name, 'b')
        self.b.__name__ = 'c'
        self.assertEqual(self.b.__name__, 'c')
        self.assertEqual(self.b.func_name, 'c')
        self.b.func_name = 'd'
        self.assertEqual(self.b.__name__, 'd')
        self.assertEqual(self.b.func_name, 'd')
        # __name__ and func_name must be a string
        self.cannot_set_attr(self.b, '__name__', 7, TypeError)
        self.cannot_set_attr(self.b, 'func_name', 7, TypeError)
        # __name__ must be available when in restricted mode. Exec will raise
        # AttributeError if __name__ is not available on f.
        s = """def f(): pass\nf.__name__"""
        exec s in {'__builtins__': {}}
        # Test on methods, too
        self.assertEqual(self.f.a.__name__, 'a')
        self.assertEqual(self.fi.a.__name__, 'a')
        self.cannot_set_attr(self.f.a, "__name__", 'a', AttributeError)
        self.cannot_set_attr(self.fi.a, "__name__", 'a', AttributeError)

    def test_func_code(self):
        num_one, num_two = 7, 8
        def a(): pass
        def b(): return 12
        def c(): return num_one
        def d(): return num_two
        def e(): return num_one, num_two
        for func in [a, b, c, d, e]:
            self.assertEqual(type(func.func_code), types.CodeType)
        self.assertEqual(c(), 7)
        self.assertEqual(d(), 8)
        d.func_code = c.func_code
        self.assertEqual(c.func_code, d.func_code)
        self.assertEqual(c(), 7)
        # self.assertEqual(d(), 7)
        try: b.func_code = c.func_code
        except ValueError: pass
        else: self.fail(
            "func_code with different numbers of free vars should not be "
            "possible")
        try: e.func_code = d.func_code
        except ValueError: pass
        else: self.fail(
            "func_code with different numbers of free vars should not be "
            "possible")

    def test_blank_func_defaults(self):
        self.assertEqual(self.b.func_defaults, None)
        del self.b.func_defaults
        self.assertEqual(self.b.func_defaults, None)

    def test_func_default_args(self):
        def first_func(a, b):
            return a+b
        def second_func(a=1, b=2):
            return a+b
        self.assertEqual(first_func.func_defaults, None)
        self.assertEqual(second_func.func_defaults, (1, 2))
        first_func.func_defaults = (1, 2)
        self.assertEqual(first_func.func_defaults, (1, 2))
        self.assertEqual(first_func(), 3)
        self.assertEqual(first_func(3), 5)
        self.assertEqual(first_func(3, 5), 8)
        del second_func.func_defaults
        self.assertEqual(second_func.func_defaults, None)
        try: second_func()
        except TypeError: pass
        else: self.fail(
            "func_defaults does not update; deleting it does not remove "
            "requirement")

class ImplicitReferencesTest(FuncAttrsTest):
    def test_im_class(self):
        self.assertEqual(self.f.a.im_class, self.f)
        self.assertEqual(self.fi.a.im_class, self.f)
        self.cannot_set_attr(self.f.a, "im_class", self.f, TypeError)
        self.cannot_set_attr(self.fi.a, "im_class", self.f, TypeError)

    def test_im_func(self):
        self.f.b = self.b
        self.assertEqual(self.f.b.im_func, self.b)
        self.assertEqual(self.fi.b.im_func, self.b)
        self.cannot_set_attr(self.f.b, "im_func", self.b, TypeError)
        self.cannot_set_attr(self.fi.b, "im_func", self.b, TypeError)

    def test_im_self(self):
        self.assertEqual(self.f.a.im_self, None)
        self.assertEqual(self.fi.a.im_self, self.fi)
        self.cannot_set_attr(self.f.a, "im_self", None, TypeError)
        self.cannot_set_attr(self.fi.a, "im_self", self.fi, TypeError)

    def test_im_func_non_method(self):
        # Behavior should be the same when a method is added via an attr
        # assignment
        self.f.id = types.MethodType(id, None, self.f)
        self.assertEqual(self.fi.id(), id(self.fi))
        self.assertNotEqual(self.fi.id(), id(self.f))
        # Test usage
        try: self.f.id.unknown_attr
        except AttributeError: pass
        else: self.fail("using unknown attributes should raise AttributeError")
        # Test assignment and deletion
        self.cannot_set_attr(self.f.id, 'unknown_attr', 2, AttributeError)
        self.cannot_set_attr(self.fi.id, 'unknown_attr', 2, AttributeError)

    def test_implicit_method_properties(self):
        self.f.a.im_func.known_attr = 7
        self.assertEqual(self.f.a.known_attr, 7)
        self.assertEqual(self.fi.a.known_attr, 7)

class ArbitraryFunctionAttrTest(FuncAttrsTest):
    def test_set_attr(self):
        self.b.known_attr = 7
        self.assertEqual(self.b.known_attr, 7)
        for func in [self.f.a, self.fi.a]:
            try: func.known_attr = 7
            except AttributeError: pass
            else: self.fail("setting attributes on methods should raise error")

    def test_delete_unknown_attr(self):
        try: del self.b.unknown_attr
        except AttributeError: pass
        else: self.fail("deleting unknown attribute should raise TypeError")

    def test_setting_attrs_duplicates(self):
        try: self.f.a.klass = self.f
        except AttributeError: pass
        else: self.fail("setting arbitrary attribute in unbound function "
                        " should raise AttributeError")
        self.f.a.im_func.klass = self.f
        for method in [self.f.a, self.fi.a, self.fi.a.im_func]:
            self.assertEqual(method.klass, self.f)

    def test_unset_attr(self):
        for func in [self.b, self.f.a, self.fi.a]:
            try:  func.non_existant_attr
            except AttributeError: pass
            else: self.fail("using unknown attributes should raise "
                            "AttributeError")

class FunctionDictsTest(FuncAttrsTest):
    def test_setting_dict_to_invalid(self):
        self.cannot_set_attr(self.b, '__dict__', None, TypeError)
        self.cannot_set_attr(self.b, 'func_dict', None, TypeError)
        from UserDict import UserDict
        d = UserDict({'known_attr': 7})
        self.cannot_set_attr(self.f.a.im_func, '__dict__', d, TypeError)
        self.cannot_set_attr(self.fi.a.im_func, '__dict__', d, TypeError)

    def test_setting_dict_to_valid(self):
        d = {'known_attr': 7}
        self.b.__dict__ = d
        # Setting dict is only possible on the underlying function objects
        self.f.a.im_func.__dict__ = d
        # Test assignment
        self.assertEqual(d, self.b.__dict__)
        self.assertEqual(d, self.b.func_dict)
        # ... and on all the different ways of referencing the method's func
        self.assertEqual(d, self.f.a.im_func.__dict__)
        self.assertEqual(d, self.f.a.__dict__)
        self.assertEqual(d, self.fi.a.im_func.__dict__)
        self.assertEqual(d, self.fi.a.__dict__)
        # Test value
        self.assertEqual(self.b.known_attr, 7)
        self.assertEqual(self.b.__dict__['known_attr'], 7)
        self.assertEqual(self.b.func_dict['known_attr'], 7)
        # ... and again, on all the different method's names
        self.assertEqual(self.f.a.im_func.known_attr, 7)
        self.assertEqual(self.f.a.known_attr, 7)
        self.assertEqual(self.fi.a.im_func.known_attr, 7)
        self.assertEqual(self.fi.a.known_attr, 7)

    def test_delete_func_dict(self):
        try: del self.b.__dict__
        except TypeError: pass
        else: self.fail("deleting function dictionary should raise TypeError")
        try: del self.b.func_dict
        except TypeError: pass
        else: self.fail("deleting function dictionary should raise TypeError")

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
        self.assertEqual(self.b.func_doc, None)
        docstr = "A test method that does nothing"
        self.b.__doc__ = self.f.a.im_func.__doc__ = docstr
        self.assertEqual(self.b.__doc__, docstr)
        self.assertEqual(self.b.func_doc, docstr)
        self.assertEqual(self.f.a.__doc__, docstr)
        self.assertEqual(self.fi.a.__doc__, docstr)
        self.cannot_set_attr(self.f.a, "__doc__", docstr, AttributeError)
        self.cannot_set_attr(self.fi.a, "__doc__", docstr, AttributeError)

    def test_delete_docstring(self):
        self.b.__doc__ = "The docstring"
        del self.b.__doc__
        self.assertEqual(self.b.__doc__, None)
        self.assertEqual(self.b.func_doc, None)
        self.b.func_doc = "The docstring"
        del self.b.func_doc
        self.assertEqual(self.b.__doc__, None)
        self.assertEqual(self.b.func_doc, None)

def test_main():
    test_support.run_unittest(FunctionPropertiesTest, ImplicitReferencesTest,
                              ArbitraryFunctionAttrTest, FunctionDictsTest,
                              FunctionDocstringTest)

if __name__ == "__main__":
    test_main()
