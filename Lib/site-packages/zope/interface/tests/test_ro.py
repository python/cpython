##############################################################################
#
# Copyright (c) 2014 Zope Foundation and Contributors.
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
"""Resolution ordering utility tests"""
import unittest

# pylint:disable=blacklisted-name,protected-access,attribute-defined-outside-init

class Test__mergeOrderings(unittest.TestCase):

    def _callFUT(self, orderings):
        from zope.interface.ro import _legacy_mergeOrderings
        return _legacy_mergeOrderings(orderings)

    def test_empty(self):
        self.assertEqual(self._callFUT([]), [])

    def test_single(self):
        self.assertEqual(self._callFUT(['a', 'b', 'c']), ['a', 'b', 'c'])

    def test_w_duplicates(self):
        self.assertEqual(self._callFUT([['a'], ['b', 'a']]), ['b', 'a'])

    def test_suffix_across_multiple_duplicates(self):
        O1 = ['x', 'y', 'z']
        O2 = ['q', 'z']
        O3 = [1, 3, 5]
        O4 = ['z']
        self.assertEqual(self._callFUT([O1, O2, O3, O4]),
                         ['x', 'y', 'q', 1, 3, 5, 'z'])


class Test__flatten(unittest.TestCase):

    def _callFUT(self, ob):
        from zope.interface.ro import _legacy_flatten
        return _legacy_flatten(ob)

    def test_w_empty_bases(self):
        class Foo(object):
            pass
        foo = Foo()
        foo.__bases__ = ()
        self.assertEqual(self._callFUT(foo), [foo])

    def test_w_single_base(self):
        class Foo(object):
            pass
        self.assertEqual(self._callFUT(Foo), [Foo, object])

    def test_w_bases(self):
        class Foo(object):
            pass
        class Bar(Foo):
            pass
        self.assertEqual(self._callFUT(Bar), [Bar, Foo, object])

    def test_w_diamond(self):
        class Foo(object):
            pass
        class Bar(Foo):
            pass
        class Baz(Foo):
            pass
        class Qux(Bar, Baz):
            pass
        self.assertEqual(self._callFUT(Qux),
                         [Qux, Bar, Foo, object, Baz, Foo, object])


class Test_ro(unittest.TestCase):
    maxDiff = None
    def _callFUT(self, ob, **kwargs):
        from zope.interface.ro import _legacy_ro
        return _legacy_ro(ob, **kwargs)

    def test_w_empty_bases(self):
        class Foo(object):
            pass
        foo = Foo()
        foo.__bases__ = ()
        self.assertEqual(self._callFUT(foo), [foo])

    def test_w_single_base(self):
        class Foo(object):
            pass
        self.assertEqual(self._callFUT(Foo), [Foo, object])

    def test_w_bases(self):
        class Foo(object):
            pass
        class Bar(Foo):
            pass
        self.assertEqual(self._callFUT(Bar), [Bar, Foo, object])

    def test_w_diamond(self):
        class Foo(object):
            pass
        class Bar(Foo):
            pass
        class Baz(Foo):
            pass
        class Qux(Bar, Baz):
            pass
        self.assertEqual(self._callFUT(Qux),
                         [Qux, Bar, Baz, Foo, object])

    def _make_IOErr(self):
        # This can't be done in the standard C3 ordering.
        class Foo(object):
            def __init__(self, name, *bases):
                self.__name__ = name
                self.__bases__ = bases
            def __repr__(self): # pragma: no cover
                return self.__name__

        # Mimic what classImplements(IOError, IIOError)
        # does.
        IEx = Foo('IEx')
        IStdErr = Foo('IStdErr', IEx)
        IEnvErr = Foo('IEnvErr', IStdErr)
        IIOErr = Foo('IIOErr', IEnvErr)
        IOSErr = Foo('IOSErr', IEnvErr)

        IOErr = Foo('IOErr', IEnvErr, IIOErr, IOSErr)
        return IOErr, [IOErr, IIOErr, IOSErr, IEnvErr, IStdErr, IEx]

    def test_non_orderable(self):
        IOErr, bases = self._make_IOErr()

        self.assertEqual(self._callFUT(IOErr), bases)

    def test_mixed_inheritance_and_implementation(self):
        # https://github.com/zopefoundation/zope.interface/issues/8
        # This test should fail, but doesn't, as described in that issue.
        # pylint:disable=inherit-non-class
        from zope.interface import implementer
        from zope.interface import Interface
        from zope.interface import providedBy
        from zope.interface import implementedBy

        class IFoo(Interface):
            pass

        @implementer(IFoo)
        class ImplementsFoo(object):
            pass

        class ExtendsFoo(ImplementsFoo):
            pass

        class ImplementsNothing(object):
            pass

        class ExtendsFooImplementsNothing(ExtendsFoo, ImplementsNothing):
            pass

        self.assertEqual(
            self._callFUT(providedBy(ExtendsFooImplementsNothing())),
            [implementedBy(ExtendsFooImplementsNothing),
             implementedBy(ExtendsFoo),
             implementedBy(ImplementsFoo),
             IFoo,
             Interface,
             implementedBy(ImplementsNothing),
             implementedBy(object)])


class C3Setting(object):

    def __init__(self, setting, value):
        self._setting = setting
        self._value = value

    def __enter__(self):
        from zope.interface import ro
        setattr(ro.C3, self._setting.__name__, self._value)

    def __exit__(self, t, v, tb):
        from zope.interface import ro
        setattr(ro.C3, self._setting.__name__, self._setting)

class Test_c3_ro(Test_ro):

    def setUp(self):
        Test_ro.setUp(self)
        from zope.testing.loggingsupport import InstalledHandler
        self.log_handler = handler = InstalledHandler('zope.interface.ro')
        self.addCleanup(handler.uninstall)

    def _callFUT(self, ob, **kwargs):
        from zope.interface.ro import ro
        return ro(ob, **kwargs)

    def test_complex_diamond(self, base=object):
        # https://github.com/zopefoundation/zope.interface/issues/21
        O = base
        class F(O):
            pass
        class E(O):
            pass
        class D(O):
            pass
        class C(D, F):
            pass
        class B(D, E):
            pass
        class A(B, C):
            pass

        if hasattr(A, 'mro'):
            self.assertEqual(A.mro(), self._callFUT(A))

        return A

    def test_complex_diamond_interface(self):
        from zope.interface import Interface

        IA = self.test_complex_diamond(Interface)

        self.assertEqual(
            [x.__name__ for x in IA.__iro__],
            ['A', 'B', 'C', 'D', 'E', 'F', 'Interface']
        )

    def test_complex_diamond_use_legacy_argument(self):
        from zope.interface import Interface

        A = self.test_complex_diamond(Interface)
        legacy_A_iro = self._callFUT(A, use_legacy_ro=True)
        self.assertNotEqual(A.__iro__, legacy_A_iro)

        # And logging happened as a side-effect.
        self._check_handler_complex_diamond()

    def test_complex_diamond_compare_legacy_argument(self):
        from zope.interface import Interface

        A = self.test_complex_diamond(Interface)
        computed_A_iro = self._callFUT(A, log_changed_ro=True)
        # It matches, of course, but we did log a warning.
        self.assertEqual(tuple(computed_A_iro), A.__iro__)
        self._check_handler_complex_diamond()

    def _check_handler_complex_diamond(self):
        handler = self.log_handler
        self.assertEqual(1, len(handler.records))
        record = handler.records[0]

        self.assertEqual('\n'.join(l.rstrip() for l in record.getMessage().splitlines()), """\
Object <InterfaceClass zope.interface.tests.test_ro.A> has different legacy and C3 MROs:
  Legacy RO (len=7)                                  C3 RO (len=7; inconsistent=no)
  ====================================================================================================
    <InterfaceClass zope.interface.tests.test_ro.A>    <InterfaceClass zope.interface.tests.test_ro.A>
    <InterfaceClass zope.interface.tests.test_ro.B>    <InterfaceClass zope.interface.tests.test_ro.B>
  - <InterfaceClass zope.interface.tests.test_ro.E>
    <InterfaceClass zope.interface.tests.test_ro.C>    <InterfaceClass zope.interface.tests.test_ro.C>
    <InterfaceClass zope.interface.tests.test_ro.D>    <InterfaceClass zope.interface.tests.test_ro.D>
                                                     + <InterfaceClass zope.interface.tests.test_ro.E>
    <InterfaceClass zope.interface.tests.test_ro.F>    <InterfaceClass zope.interface.tests.test_ro.F>
    <InterfaceClass zope.interface.Interface>          <InterfaceClass zope.interface.Interface>""")

    def test_ExtendedPathIndex_implement_thing_implementedby_super(self):
        # See https://github.com/zopefoundation/zope.interface/pull/182#issuecomment-598754056
        from zope.interface import ro
        # pylint:disable=inherit-non-class
        class _Based(object):
            __bases__ = ()

            def __init__(self, name, bases=(), attrs=None):
                self.__name__ = name
                self.__bases__ = bases

            def __repr__(self):
                return self.__name__

        Interface = _Based('Interface', (), {})

        class IPluggableIndex(Interface):
            pass

        class ILimitedResultIndex(IPluggableIndex):
            pass

        class IQueryIndex(IPluggableIndex):
            pass

        class IPathIndex(Interface):
            pass

        # A parent class who implements two distinct interfaces whose
        # only common ancestor is Interface. An easy case.
        # @implementer(IPathIndex, IQueryIndex)
        # class PathIndex(object):
        #     pass
        obj = _Based('object')
        PathIndex = _Based('PathIndex', (IPathIndex, IQueryIndex, obj))

        # Child class that tries to put an interface the parent declares
        # later ahead of the parent.
        # @implementer(ILimitedResultIndex, IQueryIndex)
        # class ExtendedPathIndex(PathIndex):
        #     pass
        ExtendedPathIndex = _Based('ExtendedPathIndex',
                                   (ILimitedResultIndex, IQueryIndex, PathIndex))

        # We were able to resolve it, and in exactly the same way as
        # the legacy RO did, even though it is inconsistent.
        result = self._callFUT(ExtendedPathIndex, log_changed_ro=True, strict=False)
        self.assertEqual(result, [
            ExtendedPathIndex,
            ILimitedResultIndex,
            PathIndex,
            IPathIndex,
            IQueryIndex,
            IPluggableIndex,
            Interface,
            obj])

        record, = self.log_handler.records
        self.assertIn('used the legacy', record.getMessage())

        with self.assertRaises(ro.InconsistentResolutionOrderError):
            self._callFUT(ExtendedPathIndex, strict=True)

    def test_OSError_IOError(self):
        if OSError is not IOError:
            # Python 2
            self.skipTest("Requires Python 3 IOError == OSError")
        from zope.interface.common import interfaces
        from zope.interface import providedBy

        self.assertEqual(
            list(providedBy(OSError()).flattened()),
            [
                interfaces.IOSError,
                interfaces.IIOError,
                interfaces.IEnvironmentError,
                interfaces.IStandardError,
                interfaces.IException,
                interfaces.Interface,
            ])

    def test_non_orderable(self):
        import warnings
        from zope.interface import ro
        try:
            # If we've already warned, we must reset that state.
            del ro.__warningregistry__
        except AttributeError:
            pass

        with warnings.catch_warnings():
            warnings.simplefilter('error')
            with C3Setting(ro.C3.WARN_BAD_IRO, True), C3Setting(ro.C3.STRICT_IRO, False):
                with self.assertRaises(ro.InconsistentResolutionOrderWarning):
                    super(Test_c3_ro, self).test_non_orderable()

        IOErr, _ = self._make_IOErr()
        with self.assertRaises(ro.InconsistentResolutionOrderError):
            self._callFUT(IOErr, strict=True)

        with C3Setting(ro.C3.TRACK_BAD_IRO, True), C3Setting(ro.C3.STRICT_IRO, False):
            with warnings.catch_warnings():
                warnings.simplefilter('ignore')
                self._callFUT(IOErr)
            self.assertIn(IOErr, ro.C3.BAD_IROS)

        iro = self._callFUT(IOErr, strict=False)
        legacy_iro = self._callFUT(IOErr, use_legacy_ro=True, strict=False)
        self.assertEqual(iro, legacy_iro)


class TestC3(unittest.TestCase):
    def _makeOne(self, C, strict=False, base_mros=None):
        from zope.interface.ro import C3
        return C3.resolver(C, strict, base_mros)

    def test_base_mros_given(self):
        c3 = self._makeOne(type(self), base_mros={unittest.TestCase: unittest.TestCase.__mro__})
        memo = c3.memo
        self.assertIn(unittest.TestCase, memo)
        # We used the StaticMRO class
        self.assertIsNone(memo[unittest.TestCase].had_inconsistency)

    def test_one_base_optimization(self):
        c3 = self._makeOne(type(self))
        # Even though we didn't call .mro() yet, the MRO has been
        # computed.
        self.assertIsNotNone(c3._C3__mro) # pylint:disable=no-member
        c3._merge = None
        self.assertEqual(c3.mro(), list(type(self).__mro__))


class Test_ROComparison(unittest.TestCase):

    class MockC3(object):
        direct_inconsistency = False
        bases_had_inconsistency = False

    def _makeOne(self, c3=None, c3_ro=(), legacy_ro=()):
        from zope.interface.ro import _ROComparison
        return _ROComparison(c3 or self.MockC3(), c3_ro, legacy_ro)

    def test_inconsistent_label(self):
        comp = self._makeOne()
        self.assertEqual('no', comp._inconsistent_label)

        comp.c3.direct_inconsistency = True
        self.assertEqual("direct", comp._inconsistent_label)

        comp.c3.bases_had_inconsistency = True
        self.assertEqual("direct+bases", comp._inconsistent_label)

        comp.c3.direct_inconsistency = False
        self.assertEqual('bases', comp._inconsistent_label)
