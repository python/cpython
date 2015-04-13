"""Unit tests for new super() implementation."""

import sys
import unittest


class A:
    def f(self):
        return 'A'
    @classmethod
    def cm(cls):
        return (cls, 'A')

class B(A):
    def f(self):
        return super().f() + 'B'
    @classmethod
    def cm(cls):
        return (cls, super().cm(), 'B')

class C(A):
    def f(self):
        return super().f() + 'C'
    @classmethod
    def cm(cls):
        return (cls, super().cm(), 'C')

class D(C, B):
    def f(self):
        return super().f() + 'D'
    def cm(cls):
        return (cls, super().cm(), 'D')

class E(D):
    pass

class F(E):
    f = E.f

class G(A):
    pass


class TestSuper(unittest.TestCase):

    def tearDown(self):
        # This fixes the damage that test_various___class___pathologies does.
        nonlocal __class__
        __class__ = TestSuper

    def test_basics_working(self):
        self.assertEqual(D().f(), 'ABCD')

    def test_class_getattr_working(self):
        self.assertEqual(D.f(D()), 'ABCD')

    def test_subclass_no_override_working(self):
        self.assertEqual(E().f(), 'ABCD')
        self.assertEqual(E.f(E()), 'ABCD')

    def test_unbound_method_transfer_working(self):
        self.assertEqual(F().f(), 'ABCD')
        self.assertEqual(F.f(F()), 'ABCD')

    def test_class_methods_still_working(self):
        self.assertEqual(A.cm(), (A, 'A'))
        self.assertEqual(A().cm(), (A, 'A'))
        self.assertEqual(G.cm(), (G, 'A'))
        self.assertEqual(G().cm(), (G, 'A'))

    def test_super_in_class_methods_working(self):
        d = D()
        self.assertEqual(d.cm(), (d, (D, (D, (D, 'A'), 'B'), 'C'), 'D'))
        e = E()
        self.assertEqual(e.cm(), (e, (E, (E, (E, 'A'), 'B'), 'C'), 'D'))

    def test_super_with_closure(self):
        # Issue4360: super() did not work in a function that
        # contains a closure
        class E(A):
            def f(self):
                def nested():
                    self
                return super().f() + 'E'

        self.assertEqual(E().f(), 'AE')

    def test_various___class___pathologies(self):
        # See issue #12370
        class X(A):
            def f(self):
                return super().f()
            __class__ = 413
        x = X()
        self.assertEqual(x.f(), 'A')
        self.assertEqual(x.__class__, 413)
        class X:
            x = __class__
            def f():
                __class__
        self.assertIs(X.x, type(self))
        with self.assertRaises(NameError) as e:
            exec("""class X:
                __class__
                def f():
                    __class__""", globals(), {})
        self.assertIs(type(e.exception), NameError) # Not UnboundLocalError
        class X:
            global __class__
            __class__ = 42
            def f():
                __class__
        self.assertEqual(globals()["__class__"], 42)
        del globals()["__class__"]
        self.assertNotIn("__class__", X.__dict__)
        class X:
            nonlocal __class__
            __class__ = 42
            def f():
                __class__
        self.assertEqual(__class__, 42)

    def test___class___instancemethod(self):
        # See issue #14857
        class X:
            def f(self):
                return __class__
        self.assertIs(X().f(), X)

    def test___class___classmethod(self):
        # See issue #14857
        class X:
            @classmethod
            def f(cls):
                return __class__
        self.assertIs(X.f(), X)

    def test___class___staticmethod(self):
        # See issue #14857
        class X:
            @staticmethod
            def f():
                return __class__
        self.assertIs(X.f(), X)

    def test_obscure_super_errors(self):
        def f():
            super()
        self.assertRaises(RuntimeError, f)
        def f(x):
            del x
            super()
        self.assertRaises(RuntimeError, f, None)
        class X:
            def f(x):
                nonlocal __class__
                del __class__
                super()
        self.assertRaises(RuntimeError, X().f)

    def test_cell_as_self(self):
        class X:
            def meth(self):
                super()

        def f():
            k = X()
            def g():
                return k
            return g
        c = f().__closure__[0]
        self.assertRaises(TypeError, X.meth, c)


if __name__ == "__main__":
    unittest.main()
