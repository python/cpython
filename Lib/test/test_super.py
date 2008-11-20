"""Unit tests for new super() implementation."""

import sys
import unittest
from test import support


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

    def testBasicsWorking(self):
        self.assertEqual(D().f(), 'ABCD')

    def testClassGetattrWorking(self):
        self.assertEqual(D.f(D()), 'ABCD')

    def testSubclassNoOverrideWorking(self):
        self.assertEqual(E().f(), 'ABCD')
        self.assertEqual(E.f(E()), 'ABCD')

    def testUnboundMethodTransferWorking(self):
        self.assertEqual(F().f(), 'ABCD')
        self.assertEqual(F.f(F()), 'ABCD')

    def testClassMethodsStillWorking(self):
        self.assertEqual(A.cm(), (A, 'A'))
        self.assertEqual(A().cm(), (A, 'A'))
        self.assertEqual(G.cm(), (G, 'A'))
        self.assertEqual(G().cm(), (G, 'A'))

    def testSuperInClassMethodsWorking(self):
        d = D()
        self.assertEqual(d.cm(), (d, (D, (D, (D, 'A'), 'B'), 'C'), 'D'))
        e = E()
        self.assertEqual(e.cm(), (e, (E, (E, (E, 'A'), 'B'), 'C'), 'D'))

    def testSuperWithClosure(self):
        # Issue4360: super() did not work in a function that
        # contains a closure
        class E(A):
            def f(self):
                def nested():
                    self
                return super().f() + 'E'

        self.assertEqual(E().f(), 'AE')


def test_main():
    support.run_unittest(TestSuper)


if __name__ == "__main__":
    unittest.main()
