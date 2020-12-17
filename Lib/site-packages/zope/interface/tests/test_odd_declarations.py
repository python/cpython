##############################################################################
#
# Copyright (c) 2003 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
#
##############################################################################
"""Test interface declarations against ExtensionClass-like classes.

These tests are to make sure we do something sane in the presence of
classic ExtensionClass classes and instances.
"""
import unittest

from zope.interface.tests import odd
from zope.interface import Interface
from zope.interface import implementer
from zope.interface import directlyProvides
from zope.interface import providedBy
from zope.interface import directlyProvidedBy
from zope.interface import classImplements
from zope.interface import classImplementsOnly
from zope.interface import implementedBy
from zope.interface._compat import _skip_under_py3k

class I1(Interface): pass
class I2(Interface): pass
class I3(Interface): pass
class I31(I3): pass
class I4(Interface): pass
class I5(Interface): pass

class Odd(object):
    pass
Odd = odd.MetaClass('Odd', Odd.__bases__, {})


class B(Odd): __implemented__ = I2


# TODO: We are going to need more magic to make classProvides work with odd
#       classes. This will work in the next iteration. For now, we'll use
#       a different mechanism.

# from zope.interface import classProvides
class A(Odd):
    pass
classImplements(A, I1)

class C(A, B):
    pass
classImplements(C, I31)


class Test(unittest.TestCase):

    def test_ObjectSpecification(self):
        c = C()
        directlyProvides(c, I4)
        self.assertEqual([i.getName() for i in providedBy(c)],
                         ['I4', 'I31', 'I1', 'I2']
                         )
        self.assertEqual([i.getName() for i in providedBy(c).flattened()],
                         ['I4', 'I31', 'I3', 'I1', 'I2', 'Interface']
                         )
        self.assertTrue(I1 in providedBy(c))
        self.assertFalse(I3 in providedBy(c))
        self.assertTrue(providedBy(c).extends(I3))
        self.assertTrue(providedBy(c).extends(I31))
        self.assertFalse(providedBy(c).extends(I5))

        class COnly(A, B):
            pass
        classImplementsOnly(COnly, I31)

        class D(COnly):
            pass
        classImplements(D, I5)

        classImplements(D, I5)

        c = D()
        directlyProvides(c, I4)
        self.assertEqual([i.getName() for i in providedBy(c)],
                         ['I4', 'I5', 'I31'])
        self.assertEqual([i.getName() for i in providedBy(c).flattened()],
                         ['I4', 'I5', 'I31', 'I3', 'Interface'])
        self.assertFalse(I1 in providedBy(c))
        self.assertFalse(I3 in providedBy(c))
        self.assertTrue(providedBy(c).extends(I3))
        self.assertFalse(providedBy(c).extends(I1))
        self.assertTrue(providedBy(c).extends(I31))
        self.assertTrue(providedBy(c).extends(I5))

        class COnly(A, B): __implemented__ = I31
        class D(COnly):
            pass
        classImplements(D, I5)

        classImplements(D, I5)
        c = D()
        directlyProvides(c, I4)
        self.assertEqual([i.getName() for i in providedBy(c)],
                         ['I4', 'I5', 'I31'])
        self.assertEqual([i.getName() for i in providedBy(c).flattened()],
                         ['I4', 'I5', 'I31', 'I3', 'Interface'])
        self.assertFalse(I1 in providedBy(c))
        self.assertFalse(I3 in providedBy(c))
        self.assertTrue(providedBy(c).extends(I3))
        self.assertFalse(providedBy(c).extends(I1))
        self.assertTrue(providedBy(c).extends(I31))
        self.assertTrue(providedBy(c).extends(I5))

    def test_classImplements(self):

        @implementer(I3)
        class A(Odd):
            pass

        @implementer(I4)
        class B(Odd):
            pass

        class C(A, B):
            pass
        classImplements(C, I1, I2)
        self.assertEqual([i.getName() for i in implementedBy(C)],
                         ['I1', 'I2', 'I3', 'I4'])
        classImplements(C, I5)
        self.assertEqual([i.getName() for i in implementedBy(C)],
                         ['I1', 'I2', 'I5', 'I3', 'I4'])

    def test_classImplementsOnly(self):
        @implementer(I3)
        class A(Odd):
            pass

        @implementer(I4)
        class B(Odd):
            pass

        class C(A, B):
            pass
        classImplementsOnly(C, I1, I2)
        self.assertEqual([i.__name__ for i in implementedBy(C)],
                         ['I1', 'I2'])


    def test_directlyProvides(self):
        class IA1(Interface): pass
        class IA2(Interface): pass
        class IB(Interface): pass
        class IC(Interface): pass
        class A(Odd):
            pass
        classImplements(A, IA1, IA2)

        class B(Odd):
            pass
        classImplements(B, IB)

        class C(A, B):
            pass
        classImplements(C, IC)


        ob = C()
        directlyProvides(ob, I1, I2)
        self.assertTrue(I1 in providedBy(ob))
        self.assertTrue(I2 in providedBy(ob))
        self.assertTrue(IA1 in providedBy(ob))
        self.assertTrue(IA2 in providedBy(ob))
        self.assertTrue(IB in providedBy(ob))
        self.assertTrue(IC in providedBy(ob))

        directlyProvides(ob, directlyProvidedBy(ob)-I2)
        self.assertTrue(I1 in providedBy(ob))
        self.assertFalse(I2 in providedBy(ob))
        self.assertFalse(I2 in providedBy(ob))
        directlyProvides(ob, directlyProvidedBy(ob), I2)
        self.assertTrue(I2 in providedBy(ob))

    @_skip_under_py3k
    def test_directlyProvides_fails_for_odd_class(self):
        self.assertRaises(TypeError, directlyProvides, C, I5)

    # see above
    #def TODO_test_classProvides_fails_for_odd_class(self):
    #    try:
    #        class A(Odd):
    #            classProvides(I1)
    #    except TypeError:
    #        pass # Sucess
    #    self.assert_(False,
    #                 "Shouldn't be able to use directlyProvides on odd class."
    #                 )

    def test_implementedBy(self):
        class I2(I1): pass

        class C1(Odd):
            pass
        classImplements(C1, I2)

        class C2(C1):
            pass
        classImplements(C2, I3)

        self.assertEqual([i.getName() for i in implementedBy(C2)],
                         ['I3', 'I2'])

    def test_odd_metaclass_that_doesnt_subclass_type(self):
        # This was originally a doctest in odd.py.
        # It verifies that the metaclass the rest of these tests use
        # works as expected.

        # This is used for testing support for ExtensionClass in new interfaces.

        class A(object):
            a = 1

        A = odd.MetaClass('A', A.__bases__, A.__dict__)

        class B(object):
            b = 1

        B = odd.MetaClass('B', B.__bases__, B.__dict__)

        class C(A, B):
            pass

        self.assertEqual(C.__bases__, (A, B))

        a = A()
        aa = A()
        self.assertEqual(a.a, 1)
        self.assertEqual(aa.a, 1)

        aa.a = 2
        self.assertEqual(a.a, 1)
        self.assertEqual(aa.a, 2)

        c = C()
        self.assertEqual(c.a, 1)
        self.assertEqual(c.b, 1)

        c.b = 2
        self.assertEqual(c.b, 2)

        C.c = 1
        self.assertEqual(c.c, 1)
        c.c

        try:
            from types import ClassType
        except ImportError:
            pass
        else:
            # This test only makes sense under Python 2.x
            assert not isinstance(C, (type, ClassType))

        self.assertIs(C.__class__.__class__, C.__class__)
