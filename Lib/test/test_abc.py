# Copyright 2007 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Unit tests for abc.py."""

import sys
import unittest
from test import test_support

import abc


class TestABC(unittest.TestCase):

    def test_abstractmethod_basics(self):
        @abc.abstractmethod
        def foo(self): pass
        self.assertEqual(foo.__isabstractmethod__, True)
        def bar(self): pass
        self.assertEqual(hasattr(bar, "__isabstractmethod__"), False)

    def test_abstractproperty_basics(self):
        @abc.abstractproperty
        def foo(self): pass
        self.assertEqual(foo.__isabstractmethod__, True)
        def bar(self): pass
        self.assertEqual(hasattr(bar, "__isabstractmethod__"), False)

        class C:
            __metaclass__ = abc.ABCMeta
            @abc.abstractproperty
            def foo(self): return 3
        class D(C):
            @property
            def foo(self): return super(D, self).foo
        self.assertEqual(D().foo, 3)

    def test_abstractmethod_integration(self):
        for abstractthing in [abc.abstractmethod, abc.abstractproperty]:
            class C:
                __metaclass__ = abc.ABCMeta
                @abstractthing
                def foo(self): pass  # abstract
                def bar(self): pass  # concrete
            self.assertEqual(C.__abstractmethods__, set(["foo"]))
            self.assertRaises(TypeError, C)  # because foo is abstract
            class D(C):
                def bar(self): pass  # concrete override of concrete
            self.assertEqual(D.__abstractmethods__, set(["foo"]))
            self.assertRaises(TypeError, D)  # because foo is still abstract
            class E(D):
                def foo(self): pass
            self.assertEqual(E.__abstractmethods__, set())
            E()  # now foo is concrete, too
            class F(E):
                @abstractthing
                def bar(self): pass  # abstract override of concrete
            self.assertEqual(F.__abstractmethods__, set(["bar"]))
            self.assertRaises(TypeError, F)  # because bar is abstract now

    def test_subclass_oldstyle_class(self):
        class A:
            __metaclass__ = abc.ABCMeta
        class OldstyleClass:
            pass
        self.assertFalse(issubclass(OldstyleClass, A))
        self.assertFalse(issubclass(A, OldstyleClass))

    def test_registration_basics(self):
        class A:
            __metaclass__ = abc.ABCMeta
        class B(object):
            pass
        b = B()
        self.assertEqual(issubclass(B, A), False)
        self.assertEqual(isinstance(b, A), False)
        A.register(B)
        self.assertEqual(issubclass(B, A), True)
        self.assertEqual(isinstance(b, A), True)
        class C(B):
            pass
        c = C()
        self.assertEqual(issubclass(C, A), True)
        self.assertEqual(isinstance(c, A), True)

    def test_registration_builtins(self):
        class A:
            __metaclass__ = abc.ABCMeta
        A.register(int)
        self.assertEqual(isinstance(42, A), True)
        self.assertEqual(issubclass(int, A), True)
        class B(A):
            pass
        B.register(basestring)
        self.assertEqual(isinstance("", A), True)
        self.assertEqual(issubclass(str, A), True)

    def test_registration_edge_cases(self):
        class A:
            __metaclass__ = abc.ABCMeta
        A.register(A)  # should pass silently
        class A1(A):
            pass
        self.assertRaises(RuntimeError, A1.register, A)  # cycles not allowed
        class B(object):
            pass
        A1.register(B)  # ok
        A1.register(B)  # should pass silently
        class C(A):
            pass
        A.register(C)  # should pass silently
        self.assertRaises(RuntimeError, C.register, A)  # cycles not allowed
        C.register(B)  # ok

    def test_registration_transitiveness(self):
        class A:
            __metaclass__ = abc.ABCMeta
        self.failUnless(issubclass(A, A))
        class B:
            __metaclass__ = abc.ABCMeta
        self.failIf(issubclass(A, B))
        self.failIf(issubclass(B, A))
        class C:
            __metaclass__ = abc.ABCMeta
        A.register(B)
        class B1(B):
            pass
        self.failUnless(issubclass(B1, A))
        class C1(C):
            pass
        B1.register(C1)
        self.failIf(issubclass(C, B))
        self.failIf(issubclass(C, B1))
        self.failUnless(issubclass(C1, A))
        self.failUnless(issubclass(C1, B))
        self.failUnless(issubclass(C1, B1))
        C1.register(int)
        class MyInt(int):
            pass
        self.failUnless(issubclass(MyInt, A))
        self.failUnless(isinstance(42, A))

    def test_all_new_methods_are_called(self):
        class A:
            __metaclass__ = abc.ABCMeta
        class B(object):
            counter = 0
            def __new__(cls):
                B.counter += 1
                return super(B, cls).__new__(cls)
        class C(A, B):
            pass
        self.assertEqual(B.counter, 0)
        C()
        self.assertEqual(B.counter, 1)


def test_main():
    test_support.run_unittest(TestABC)


if __name__ == "__main__":
    unittest.main()
