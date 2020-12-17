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
"""Tests for advice

This module was adapted from 'protocols.tests.advice', part of the Python
Enterprise Application Kit (PEAK).  Please notify the PEAK authors
(pje@telecommunity.com and tsarna@sarna.org) if bugs are found or
Zope-specific changes are required, so that the PEAK version of this module
can be kept in sync.

PEAK is a Python application framework that interoperates with (but does
not require) Zope 3 and Twisted.  It provides tools for manipulating UML
models, object-relational persistence, aspect-oriented programming, and more.
Visit the PEAK home page at http://peak.telecommunity.com for more information.
"""

import unittest
import sys

from zope.interface._compat import _skip_under_py2
from zope.interface._compat import _skip_under_py3k


class FrameInfoTest(unittest.TestCase):

    def test_w_module(self):
        from zope.interface.tests import advisory_testing
        (kind, module,
         f_locals, f_globals) = advisory_testing.moduleLevelFrameInfo
        self.assertEqual(kind, "module")
        for d in module.__dict__, f_locals, f_globals:
            self.assertTrue(d is advisory_testing.my_globals)

    @_skip_under_py3k
    def test_w_ClassicClass(self):
        from zope.interface.tests import advisory_testing
        (kind,
         module,
         f_locals,
         f_globals) = advisory_testing.ClassicClass.classLevelFrameInfo
        self.assertEqual(kind, "class")

        self.assertTrue(
            f_locals is advisory_testing.ClassicClass.__dict__)  # ???
        for d in module.__dict__, f_globals:
            self.assertTrue(d is advisory_testing.my_globals)

    def test_w_NewStyleClass(self):
        from zope.interface.tests import advisory_testing
        (kind,
         module,
         f_locals,
         f_globals) = advisory_testing.NewStyleClass.classLevelFrameInfo
        self.assertEqual(kind, "class")

        for d in module.__dict__, f_globals:
            self.assertTrue(d is advisory_testing.my_globals)

    def test_inside_function_call(self):
        from zope.interface.advice import getFrameInfo
        kind, module, f_locals, f_globals = getFrameInfo(sys._getframe())
        self.assertEqual(kind, "function call")
        self.assertTrue(f_locals is locals()) # ???
        for d in module.__dict__, f_globals:
            self.assertTrue(d is globals())

    def test_inside_exec(self):
        from zope.interface.advice import getFrameInfo
        _globals = {'getFrameInfo': getFrameInfo}
        _locals = {}
        exec(_FUNKY_EXEC, _globals, _locals)
        self.assertEqual(_locals['kind'], "exec")
        self.assertTrue(_locals['f_locals'] is _locals)
        self.assertTrue(_locals['module'] is None)
        self.assertTrue(_locals['f_globals'] is _globals)


_FUNKY_EXEC = """\
import sys
kind, module, f_locals, f_globals = getFrameInfo(sys._getframe())
"""

class AdviceTests(unittest.TestCase):

    @_skip_under_py3k
    def test_order(self):
        from zope.interface.tests.advisory_testing import ping
        log = []
        class Foo(object):
            ping(log, 1)
            ping(log, 2)
            ping(log, 3)

        # Strip the list nesting
        for i in 1, 2, 3:
            self.assertTrue(isinstance(Foo, list))
            Foo, = Foo

        self.assertEqual(log, [(1, Foo), (2, [Foo]), (3, [[Foo]])])

    @_skip_under_py3k
    def test_single_explicit_meta(self):
        from zope.interface.tests.advisory_testing import ping

        class Metaclass(type):
            pass

        class Concrete(Metaclass):
            __metaclass__ = Metaclass
            ping([],1)

        Concrete, = Concrete
        self.assertTrue(Concrete.__class__ is Metaclass)


    @_skip_under_py3k
    def test_mixed_metas(self):
        from zope.interface.tests.advisory_testing import ping

        class Metaclass1(type):
            pass

        class Metaclass2(type):
            pass

        class Base1:
            __metaclass__ = Metaclass1

        class Base2:
            __metaclass__ = Metaclass2

        try:
            class Derived(Base1, Base2):
                ping([], 1)
            self.fail("Should have gotten incompatibility error")
        except TypeError:
            pass

        class Metaclass3(Metaclass1, Metaclass2):
            pass

        class Derived(Base1, Base2):
            __metaclass__ = Metaclass3
            ping([], 1)

        self.assertTrue(isinstance(Derived, list))
        Derived, = Derived
        self.assertTrue(isinstance(Derived, Metaclass3))

    @_skip_under_py3k
    def test_meta_no_bases(self):
        from zope.interface.tests.advisory_testing import ping
        from types import ClassType
        class Thing:
            ping([], 1)
        klass, = Thing # unpack list created by pong
        self.assertEqual(type(klass), ClassType)


class Test_isClassAdvisor(unittest.TestCase):

    def _callFUT(self, *args, **kw):
        from zope.interface.advice import isClassAdvisor
        return isClassAdvisor(*args, **kw)

    def test_w_non_function(self):
        self.assertEqual(self._callFUT(self), False)

    def test_w_normal_function(self):
        def foo():
            raise NotImplementedError()
        self.assertEqual(self._callFUT(foo), False)

    def test_w_advisor_function(self):
        def bar():
            raise NotImplementedError()
        bar.previousMetaclass = object()
        self.assertEqual(self._callFUT(bar), True)


class Test_determineMetaclass(unittest.TestCase):

    def _callFUT(self, *args, **kw):
        from zope.interface.advice import determineMetaclass
        return determineMetaclass(*args, **kw)

    @_skip_under_py3k
    def test_empty(self):
        from types import ClassType
        self.assertEqual(self._callFUT(()), ClassType)

    def test_empty_w_explicit_metatype(self):
        class Meta(type):
            pass
        self.assertEqual(self._callFUT((), Meta), Meta)

    def test_single(self):
        class Meta(type):
            pass
        self.assertEqual(self._callFUT((Meta,)), type)

    @_skip_under_py3k
    def test_meta_of_class(self):
        class Metameta(type):
            pass

        class Meta(type):
            __metaclass__ = Metameta

        self.assertEqual(self._callFUT((Meta, type)), Metameta)

    @_skip_under_py2
    def test_meta_of_class_py3k(self):
        # Work around SyntaxError under Python2.
        EXEC = '\n'.join([
        'class Metameta(type):',
        '    pass',
        'class Meta(type, metaclass=Metameta):',
        '    pass',
        ])
        globs = {}
        exec(EXEC, globs)
        Meta = globs['Meta']
        Metameta = globs['Metameta']

        self.assertEqual(self._callFUT((Meta, type)), Metameta)

    @_skip_under_py3k
    def test_multiple_in_hierarchy(self):
        class Meta_A(type):
            pass
        class Meta_B(Meta_A):
            pass
        class A(type):
            __metaclass__ = Meta_A
        class B(type):
            __metaclass__ = Meta_B
        self.assertEqual(self._callFUT((A, B,)), Meta_B)

    @_skip_under_py2
    def test_multiple_in_hierarchy_py3k(self):
        # Work around SyntaxError under Python2.
        EXEC = '\n'.join([
        'class Meta_A(type):',
        '    pass',
        'class Meta_B(Meta_A):',
        '    pass',
        'class A(type, metaclass=Meta_A):',
        '    pass',
        'class B(type, metaclass=Meta_B):',
        '    pass',
        ])
        globs = {}
        exec(EXEC, globs)
        Meta_A = globs['Meta_A']
        Meta_B = globs['Meta_B']
        A = globs['A']
        B = globs['B']
        self.assertEqual(self._callFUT((A, B)), Meta_B)

    @_skip_under_py3k
    def test_multiple_not_in_hierarchy(self):
        class Meta_A(type):
            pass
        class Meta_B(type):
            pass
        class A(type):
            __metaclass__ = Meta_A
        class B(type):
            __metaclass__ = Meta_B
        self.assertRaises(TypeError, self._callFUT, (A, B,))

    @_skip_under_py2
    def test_multiple_not_in_hierarchy_py3k(self):
        # Work around SyntaxError under Python2.
        EXEC = '\n'.join([
        'class Meta_A(type):',
        '    pass',
        'class Meta_B(type):',
        '    pass',
        'class A(type, metaclass=Meta_A):',
        '    pass',
        'class B(type, metaclass=Meta_B):',
        '    pass',
        ])
        globs = {}
        exec(EXEC, globs)
        Meta_A = globs['Meta_A']
        Meta_B = globs['Meta_B']
        A = globs['A']
        B = globs['B']
        self.assertRaises(TypeError, self._callFUT, (A, B))


class Test_minimalBases(unittest.TestCase):

    def _callFUT(self, klasses):
        from zope.interface.advice import minimalBases
        return minimalBases(klasses)

    def test_empty(self):
        self.assertEqual(self._callFUT([]), [])

    @_skip_under_py3k
    def test_w_oldstyle_meta(self):
        class C:
            pass
        self.assertEqual(self._callFUT([type(C)]), [])

    @_skip_under_py3k
    def test_w_oldstyle_class(self):
        class C:
            pass
        self.assertEqual(self._callFUT([C]), [C])

    def test_w_newstyle_meta(self):
        self.assertEqual(self._callFUT([type]), [type])

    def test_w_newstyle_class(self):
        class C(object):
            pass
        self.assertEqual(self._callFUT([C]), [C])

    def test_simple_hierarchy_skips_implied(self):
        class A(object):
            pass
        class B(A):
            pass
        class C(B):
            pass
        class D(object):
            pass
        self.assertEqual(self._callFUT([A, B, C]), [C])
        self.assertEqual(self._callFUT([A, C]), [C])
        self.assertEqual(self._callFUT([B, C]), [C])
        self.assertEqual(self._callFUT([A, B]), [B])
        self.assertEqual(self._callFUT([D, B, D]), [B, D])

    def test_repeats_kicked_to_end_of_queue(self):
        class A(object):
            pass
        class B(object):
            pass
        self.assertEqual(self._callFUT([A, B, A]), [B, A])
