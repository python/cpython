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
"""Test the new API for making and checking interface declarations
"""
import unittest

from zope.interface._compat import _skip_under_py3k
from zope.interface._compat import PYTHON3
from zope.interface.tests import OptimizationTestMixin
from zope.interface.tests import MissingSomeAttrs
from zope.interface.tests.test_interface import NameAndModuleComparisonTestsMixin

# pylint:disable=inherit-non-class,too-many-lines,protected-access
# pylint:disable=blacklisted-name,attribute-defined-outside-init

class _Py3ClassAdvice(object):

    def _run_generated_code(self, code, globs, locs,
                            fails_under_py3k=True,
                           ):
        # pylint:disable=exec-used,no-member
        import warnings
        with warnings.catch_warnings(record=True) as log:
            warnings.resetwarnings()
            if not PYTHON3:
                exec(code, globs, locs)
                self.assertEqual(len(log), 0) # no longer warn
                return True

            try:
                exec(code, globs, locs)
            except TypeError:
                return False
            else:
                if fails_under_py3k:
                    self.fail("Didn't raise TypeError")
            return None


class NamedTests(unittest.TestCase):

    def test_class(self):
        from zope.interface.declarations import named

        @named(u'foo')
        class Foo(object):
            pass

        self.assertEqual(Foo.__component_name__, u'foo') # pylint:disable=no-member

    def test_function(self):
        from zope.interface.declarations import named

        @named(u'foo')
        def doFoo(o):
            raise NotImplementedError()

        self.assertEqual(doFoo.__component_name__, u'foo')

    def test_instance(self):
        from zope.interface.declarations import named

        class Foo(object):
            pass
        foo = Foo()
        named(u'foo')(foo)

        self.assertEqual(foo.__component_name__, u'foo') # pylint:disable=no-member


class EmptyDeclarationTests(unittest.TestCase):
    # Tests that should pass for all objects that are empty
    # declarations. This includes a Declaration explicitly created
    # that way, and the empty ImmutableDeclaration.
    def _getEmpty(self):
        from zope.interface.declarations import Declaration
        return Declaration()

    def test___iter___empty(self):
        decl = self._getEmpty()
        self.assertEqual(list(decl), [])

    def test_flattened_empty(self):
        from zope.interface.interface import Interface
        decl = self._getEmpty()
        self.assertEqual(list(decl.flattened()), [Interface])

    def test___contains___empty(self):
        from zope.interface.interface import Interface
        decl = self._getEmpty()
        self.assertNotIn(Interface, decl)

    def test_extends_empty(self):
        from zope.interface.interface import Interface
        decl = self._getEmpty()
        self.assertTrue(decl.extends(Interface))
        self.assertTrue(decl.extends(Interface, strict=True))

    def test_interfaces_empty(self):
        decl = self._getEmpty()
        l = list(decl.interfaces())
        self.assertEqual(l, [])

    def test___sro___(self):
        from zope.interface.interface import Interface
        decl = self._getEmpty()
        self.assertEqual(decl.__sro__, (decl, Interface,))

    def test___iro___(self):
        from zope.interface.interface import Interface
        decl = self._getEmpty()
        self.assertEqual(decl.__iro__, (Interface,))

    def test_get(self):
        decl = self._getEmpty()
        self.assertIsNone(decl.get('attr'))
        self.assertEqual(decl.get('abc', 'def'), 'def')
        # It's a positive cache only (when it even exists)
        # so this added nothing.
        self.assertFalse(decl._v_attrs)

    def test_changed_w_existing__v_attrs(self):
        decl = self._getEmpty()
        decl._v_attrs = object()
        decl.changed(decl)
        self.assertFalse(decl._v_attrs)


class DeclarationTests(EmptyDeclarationTests):

    def _getTargetClass(self):
        from zope.interface.declarations import Declaration
        return Declaration

    def _makeOne(self, *args, **kw):
        return self._getTargetClass()(*args, **kw)

    def test_ctor_no_bases(self):
        decl = self._makeOne()
        self.assertEqual(list(decl.__bases__), [])

    def test_ctor_w_interface_in_bases(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        decl = self._makeOne(IFoo)
        self.assertEqual(list(decl.__bases__), [IFoo])

    def test_ctor_w_implements_in_bases(self):
        from zope.interface.declarations import Implements
        impl = Implements()
        decl = self._makeOne(impl)
        self.assertEqual(list(decl.__bases__), [impl])

    def test_changed_wo_existing__v_attrs(self):
        decl = self._makeOne()
        decl.changed(decl) # doesn't raise
        self.assertIsNone(decl._v_attrs)

    def test___contains__w_self(self):
        decl = self._makeOne()
        self.assertNotIn(decl, decl)

    def test___contains__w_unrelated_iface(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        decl = self._makeOne()
        self.assertNotIn(IFoo, decl)

    def test___contains__w_base_interface(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        decl = self._makeOne(IFoo)
        self.assertIn(IFoo, decl)

    def test___iter___single_base(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        decl = self._makeOne(IFoo)
        self.assertEqual(list(decl), [IFoo])

    def test___iter___multiple_bases(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar')
        decl = self._makeOne(IFoo, IBar)
        self.assertEqual(list(decl), [IFoo, IBar])

    def test___iter___inheritance(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        decl = self._makeOne(IBar)
        self.assertEqual(list(decl), [IBar]) #IBar.interfaces() omits bases

    def test___iter___w_nested_sequence_overlap(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar')
        decl = self._makeOne(IBar, (IFoo, IBar))
        self.assertEqual(list(decl), [IBar, IFoo])

    def test_flattened_single_base(self):
        from zope.interface.interface import Interface
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        decl = self._makeOne(IFoo)
        self.assertEqual(list(decl.flattened()), [IFoo, Interface])

    def test_flattened_multiple_bases(self):
        from zope.interface.interface import Interface
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar')
        decl = self._makeOne(IFoo, IBar)
        self.assertEqual(list(decl.flattened()), [IFoo, IBar, Interface])

    def test_flattened_inheritance(self):
        from zope.interface.interface import Interface
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        decl = self._makeOne(IBar)
        self.assertEqual(list(decl.flattened()), [IBar, IFoo, Interface])

    def test_flattened_w_nested_sequence_overlap(self):
        from zope.interface.interface import Interface
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar')
        # This is the same as calling ``Declaration(IBar, IFoo, IBar)``
        # which doesn't make much sense, but here it is. In older
        # versions of zope.interface, the __iro__ would have been
        # IFoo, IBar, Interface, which especially makes no sense.
        decl = self._makeOne(IBar, (IFoo, IBar))
        # Note that decl.__iro__ has IFoo first.
        self.assertEqual(list(decl.flattened()), [IBar, IFoo, Interface])

    def test___sub___unrelated_interface(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar')
        before = self._makeOne(IFoo)
        after = before - IBar
        self.assertIsInstance(after, self._getTargetClass())
        self.assertEqual(list(after), [IFoo])

    def test___sub___related_interface(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        before = self._makeOne(IFoo)
        after = before - IFoo
        self.assertEqual(list(after), [])

    def test___sub___related_interface_by_inheritance(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        before = self._makeOne(IBar)
        after = before - IBar
        self.assertEqual(list(after), [])

    def test___add___unrelated_interface(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar')
        before = self._makeOne(IFoo)
        after = before + IBar
        self.assertIsInstance(after, self._getTargetClass())
        self.assertEqual(list(after), [IFoo, IBar])

    def test___add___related_interface(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar')
        IBaz = InterfaceClass('IBaz')
        before = self._makeOne(IFoo, IBar)
        other = self._makeOne(IBar, IBaz)
        after = before + other
        self.assertEqual(list(after), [IFoo, IBar, IBaz])


class TestImmutableDeclaration(EmptyDeclarationTests):

    def _getTargetClass(self):
        from zope.interface.declarations import _ImmutableDeclaration
        return _ImmutableDeclaration

    def _getEmpty(self):
        from zope.interface.declarations import _empty
        return _empty

    def test_pickle(self):
        import pickle
        copied = pickle.loads(pickle.dumps(self._getEmpty()))
        self.assertIs(copied, self._getEmpty())

    def test_singleton(self):
        self.assertIs(
            self._getTargetClass()(),
            self._getEmpty()
        )

    def test__bases__(self):
        self.assertEqual(self._getEmpty().__bases__, ())

    def test_change__bases__(self):
        empty = self._getEmpty()
        empty.__bases__ = ()
        self.assertEqual(self._getEmpty().__bases__, ())

        with self.assertRaises(TypeError):
            empty.__bases__ = (1,)

    def test_dependents(self):
        empty = self._getEmpty()
        deps = empty.dependents
        self.assertEqual({}, deps)
        # Doesn't change the return.
        deps[1] = 2
        self.assertEqual({}, empty.dependents)

    def test_changed(self):
        # Does nothing, has no visible side-effects
        self._getEmpty().changed(None)

    def test_extends_always_false(self):
        self.assertFalse(self._getEmpty().extends(self))
        self.assertFalse(self._getEmpty().extends(self, strict=True))
        self.assertFalse(self._getEmpty().extends(self, strict=False))

    def test_get_always_default(self):
        self.assertIsNone(self._getEmpty().get('name'))
        self.assertEqual(self._getEmpty().get('name', 42), 42)

    def test_v_attrs(self):
        decl = self._getEmpty()
        self.assertEqual(decl._v_attrs, {})

        decl._v_attrs['attr'] = 42
        self.assertEqual(decl._v_attrs, {})
        self.assertIsNone(decl.get('attr'))

        attrs = decl._v_attrs = {}
        attrs['attr'] = 42
        self.assertEqual(decl._v_attrs, {})
        self.assertIsNone(decl.get('attr'))


class TestImplements(NameAndModuleComparisonTestsMixin,
                     unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.declarations import Implements
        return Implements

    def _makeOne(self, *args, **kw):
        return self._getTargetClass()(*args, **kw)

    def _makeOneToCompare(self):
        from zope.interface.declarations import implementedBy
        class A(object):
            pass

        return implementedBy(A)

    def test_ctor_no_bases(self):
        impl = self._makeOne()
        self.assertEqual(impl.inherit, None)
        self.assertEqual(impl.declared, ())
        self.assertEqual(impl.__name__, '?')
        self.assertEqual(list(impl.__bases__), [])

    def test___repr__(self):
        impl = self._makeOne()
        impl.__name__ = 'Testing'
        self.assertEqual(repr(impl), '<implementedBy Testing>')

    def test___reduce__(self):
        from zope.interface.declarations import implementedBy
        impl = self._makeOne()
        self.assertEqual(impl.__reduce__(), (implementedBy, (None,)))

    def test_sort(self):
        from zope.interface.declarations import implementedBy
        class A(object):
            pass
        class B(object):
            pass
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')

        self.assertEqual(implementedBy(A), implementedBy(A))
        self.assertEqual(hash(implementedBy(A)), hash(implementedBy(A)))
        self.assertTrue(implementedBy(A) < None)
        self.assertTrue(None > implementedBy(A)) # pylint:disable=misplaced-comparison-constant
        self.assertTrue(implementedBy(A) < implementedBy(B))
        self.assertTrue(implementedBy(A) > IFoo)
        self.assertTrue(implementedBy(A) <= implementedBy(B))
        self.assertTrue(implementedBy(A) >= IFoo)
        self.assertTrue(implementedBy(A) != IFoo)

    def test_proxy_equality(self):
        # https://github.com/zopefoundation/zope.interface/issues/55
        class Proxy(object):
            def __init__(self, wrapped):
                self._wrapped = wrapped

            def __getattr__(self, name):
                raise NotImplementedError()

            def __eq__(self, other):
                return self._wrapped == other

            def __ne__(self, other):
                return self._wrapped != other

        from zope.interface.declarations import implementedBy
        class A(object):
            pass

        class B(object):
            pass

        implementedByA = implementedBy(A)
        implementedByB = implementedBy(B)
        proxy = Proxy(implementedByA)

        # The order of arguments to the operators matters,
        # test both
        self.assertTrue(implementedByA == implementedByA) # pylint:disable=comparison-with-itself
        self.assertTrue(implementedByA != implementedByB)
        self.assertTrue(implementedByB != implementedByA)

        self.assertTrue(proxy == implementedByA)
        self.assertTrue(implementedByA == proxy)
        self.assertFalse(proxy != implementedByA)
        self.assertFalse(implementedByA != proxy)

        self.assertTrue(proxy != implementedByB)
        self.assertTrue(implementedByB != proxy)

    def test_changed_deletes_super_cache(self):
        impl = self._makeOne()
        self.assertIsNone(impl._super_cache)
        self.assertNotIn('_super_cache', impl.__dict__)

        impl._super_cache = 42
        self.assertIn('_super_cache', impl.__dict__)

        impl.changed(None)
        self.assertIsNone(impl._super_cache)
        self.assertNotIn('_super_cache', impl.__dict__)

    def test_changed_does_not_add_super_cache(self):
        impl = self._makeOne()
        self.assertIsNone(impl._super_cache)
        self.assertNotIn('_super_cache', impl.__dict__)

        impl.changed(None)
        self.assertIsNone(impl._super_cache)
        self.assertNotIn('_super_cache', impl.__dict__)


class Test_implementedByFallback(unittest.TestCase):

    def _getTargetClass(self):
        # pylint:disable=no-name-in-module
        from zope.interface.declarations import implementedByFallback
        return implementedByFallback

    _getFallbackClass = _getTargetClass

    def _callFUT(self, *args, **kw):
        return self._getTargetClass()(*args, **kw)

    def test_dictless_wo_existing_Implements_wo_registrations(self):
        class Foo(object):
            __slots__ = ('__implemented__',)
        foo = Foo()
        foo.__implemented__ = None
        self.assertEqual(list(self._callFUT(foo)), [])

    def test_dictless_wo_existing_Implements_cant_assign___implemented__(self):
        class Foo(object):
            def _get_impl(self):
                raise NotImplementedError()
            def _set_impl(self, val):
                raise TypeError
            __implemented__ = property(_get_impl, _set_impl)
            def __call__(self):
                # act like a factory
                raise NotImplementedError()
        foo = Foo()
        self.assertRaises(TypeError, self._callFUT, foo)

    def test_dictless_wo_existing_Implements_w_registrations(self):
        from zope.interface import declarations
        class Foo(object):
            __slots__ = ('__implemented__',)
        foo = Foo()
        foo.__implemented__ = None
        reg = object()
        with _MonkeyDict(declarations,
                         'BuiltinImplementationSpecifications') as specs:
            specs[foo] = reg
            self.assertTrue(self._callFUT(foo) is reg)

    def test_dictless_w_existing_Implements(self):
        from zope.interface.declarations import Implements
        impl = Implements()
        class Foo(object):
            __slots__ = ('__implemented__',)
        foo = Foo()
        foo.__implemented__ = impl
        self.assertTrue(self._callFUT(foo) is impl)

    def test_dictless_w_existing_not_Implements(self):
        from zope.interface.interface import InterfaceClass
        class Foo(object):
            __slots__ = ('__implemented__',)
        foo = Foo()
        IFoo = InterfaceClass('IFoo')
        foo.__implemented__ = (IFoo,)
        self.assertEqual(list(self._callFUT(foo)), [IFoo])

    def test_w_existing_attr_as_Implements(self):
        from zope.interface.declarations import Implements
        impl = Implements()
        class Foo(object):
            __implemented__ = impl
        self.assertTrue(self._callFUT(Foo) is impl)

    def test_builtins_added_to_cache(self):
        from zope.interface import declarations
        from zope.interface.declarations import Implements
        from zope.interface._compat import _BUILTINS
        with _MonkeyDict(declarations,
                         'BuiltinImplementationSpecifications') as specs:
            self.assertEqual(list(self._callFUT(tuple)), [])
            self.assertEqual(list(self._callFUT(list)), [])
            self.assertEqual(list(self._callFUT(dict)), [])
            for typ in (tuple, list, dict):
                spec = specs[typ]
                self.assertIsInstance(spec, Implements)
                self.assertEqual(repr(spec),
                                 '<implementedBy %s.%s>'
                                 % (_BUILTINS, typ.__name__))

    def test_builtins_w_existing_cache(self):
        from zope.interface import declarations
        t_spec, l_spec, d_spec = object(), object(), object()
        with _MonkeyDict(declarations,
                         'BuiltinImplementationSpecifications') as specs:
            specs[tuple] = t_spec
            specs[list] = l_spec
            specs[dict] = d_spec
            self.assertTrue(self._callFUT(tuple) is t_spec)
            self.assertTrue(self._callFUT(list) is l_spec)
            self.assertTrue(self._callFUT(dict) is d_spec)

    def test_oldstyle_class_no_assertions(self):
        # TODO: Figure out P3 story
        class Foo:
            pass
        self.assertEqual(list(self._callFUT(Foo)), [])

    def test_no_assertions(self):
        # TODO: Figure out P3 story
        class Foo(object):
            pass
        self.assertEqual(list(self._callFUT(Foo)), [])

    def test_w_None_no_bases_not_factory(self):
        class Foo(object):
            __implemented__ = None
        foo = Foo()
        self.assertRaises(TypeError, self._callFUT, foo)

    def test_w_None_no_bases_w_factory(self):
        from zope.interface.declarations import objectSpecificationDescriptor
        class Foo(object):
            __implemented__ = None
            def __call__(self):
                raise NotImplementedError()

        foo = Foo()
        foo.__name__ = 'foo'
        spec = self._callFUT(foo)
        self.assertEqual(spec.__name__,
                         'zope.interface.tests.test_declarations.foo')
        self.assertIs(spec.inherit, foo)
        self.assertIs(foo.__implemented__, spec)
        self.assertIs(foo.__providedBy__, objectSpecificationDescriptor) # pylint:disable=no-member
        self.assertNotIn('__provides__', foo.__dict__)

    def test_w_None_no_bases_w_class(self):
        from zope.interface.declarations import ClassProvides
        class Foo(object):
            __implemented__ = None
        spec = self._callFUT(Foo)
        self.assertEqual(spec.__name__,
                         'zope.interface.tests.test_declarations.Foo')
        self.assertIs(spec.inherit, Foo)
        self.assertIs(Foo.__implemented__, spec)
        self.assertIsInstance(Foo.__providedBy__, ClassProvides) # pylint:disable=no-member
        self.assertIsInstance(Foo.__provides__, ClassProvides) # pylint:disable=no-member
        self.assertEqual(Foo.__provides__, Foo.__providedBy__) # pylint:disable=no-member

    def test_w_existing_Implements(self):
        from zope.interface.declarations import Implements
        impl = Implements()
        class Foo(object):
            __implemented__ = impl
        self.assertTrue(self._callFUT(Foo) is impl)

    def test_super_when_base_implements_interface(self):
        from zope.interface import Interface
        from zope.interface.declarations import implementer

        class IBase(Interface):
            pass

        class IDerived(IBase):
            pass

        @implementer(IBase)
        class Base(object):
            pass

        @implementer(IDerived)
        class Derived(Base):
            pass

        self.assertEqual(list(self._callFUT(Derived)), [IDerived, IBase])
        sup = super(Derived, Derived)
        self.assertEqual(list(self._callFUT(sup)), [IBase])

    def test_super_when_base_implements_interface_diamond(self):
        from zope.interface import Interface
        from zope.interface.declarations import implementer

        class IBase(Interface):
            pass

        class IDerived(IBase):
            pass

        @implementer(IBase)
        class Base(object):
            pass

        class Child1(Base):
            pass

        class Child2(Base):
            pass

        @implementer(IDerived)
        class Derived(Child1, Child2):
            pass

        self.assertEqual(list(self._callFUT(Derived)), [IDerived, IBase])
        sup = super(Derived, Derived)
        self.assertEqual(list(self._callFUT(sup)), [IBase])

    def test_super_when_parent_implements_interface_diamond(self):
        from zope.interface import Interface
        from zope.interface.declarations import implementer

        class IBase(Interface):
            pass

        class IDerived(IBase):
            pass


        class Base(object):
            pass

        class Child1(Base):
            pass

        @implementer(IBase)
        class Child2(Base):
            pass

        @implementer(IDerived)
        class Derived(Child1, Child2):
            pass

        self.assertEqual(Derived.__mro__, (Derived, Child1, Child2, Base, object))
        self.assertEqual(list(self._callFUT(Derived)), [IDerived, IBase])
        sup = super(Derived, Derived)
        fut = self._callFUT(sup)
        self.assertEqual(list(fut), [IBase])
        self.assertIsNone(fut._dependents)

    def test_super_when_base_doesnt_implement_interface(self):
        from zope.interface import Interface
        from zope.interface.declarations import implementer

        class IBase(Interface):
            pass

        class IDerived(IBase):
            pass

        class Base(object):
            pass

        @implementer(IDerived)
        class Derived(Base):
            pass

        self.assertEqual(list(self._callFUT(Derived)), [IDerived])

        sup = super(Derived, Derived)
        self.assertEqual(list(self._callFUT(sup)), [])

    def test_super_when_base_is_object(self):
        from zope.interface import Interface
        from zope.interface.declarations import implementer

        class IBase(Interface):
            pass

        class IDerived(IBase):
            pass

        @implementer(IDerived)
        class Derived(object):
            pass

        self.assertEqual(list(self._callFUT(Derived)), [IDerived])

        sup = super(Derived, Derived)
        self.assertEqual(list(self._callFUT(sup)), [])
    def test_super_multi_level_multi_inheritance(self):
        from zope.interface.declarations import implementer
        from zope.interface import Interface

        class IBase(Interface):
            pass

        class IM1(Interface):
            pass

        class IM2(Interface):
            pass

        class IDerived(IBase):
            pass

        class IUnrelated(Interface):
            pass

        @implementer(IBase)
        class Base(object):
            pass

        @implementer(IM1)
        class M1(Base):
            pass

        @implementer(IM2)
        class M2(Base):
            pass

        @implementer(IDerived, IUnrelated)
        class Derived(M1, M2):
            pass

        d = Derived
        sd = super(Derived, Derived)
        sm1 = super(M1, Derived)
        sm2 = super(M2, Derived)

        self.assertEqual(list(self._callFUT(d)),
                         [IDerived, IUnrelated, IM1, IBase, IM2])
        self.assertEqual(list(self._callFUT(sd)),
                         [IM1, IBase, IM2])
        self.assertEqual(list(self._callFUT(sm1)),
                         [IM2, IBase])
        self.assertEqual(list(self._callFUT(sm2)),
                         [IBase])


class Test_implementedBy(Test_implementedByFallback,
                         OptimizationTestMixin):
    # Repeat tests for C optimizations

    def _getTargetClass(self):
        from zope.interface.declarations import implementedBy
        return implementedBy


class _ImplementsTestMixin(object):
    FUT_SETS_PROVIDED_BY = True

    def _callFUT(self, cls, iface):
        # Declare that *cls* implements *iface*; return *cls*
        raise NotImplementedError

    def _check_implementer(self, Foo,
                           orig_spec=None,
                           spec_name=__name__ + '.Foo',
                           inherit="not given"):
        from zope.interface.declarations import ClassProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')

        returned = self._callFUT(Foo, IFoo)

        self.assertIs(returned, Foo)
        spec = Foo.__implemented__
        if orig_spec is not None:
            self.assertIs(spec, orig_spec)

        self.assertEqual(spec.__name__,
                         spec_name)
        inherit = Foo if inherit == "not given" else inherit
        self.assertIs(spec.inherit, inherit)
        self.assertIs(Foo.__implemented__, spec)
        if self.FUT_SETS_PROVIDED_BY:
            self.assertIsInstance(Foo.__providedBy__, ClassProvides)
            self.assertIsInstance(Foo.__provides__, ClassProvides)
            self.assertEqual(Foo.__provides__, Foo.__providedBy__)

        return Foo, IFoo

    def test_oldstyle_class(self):
        # This only matters on Python 2
        class Foo:
            pass
        self._check_implementer(Foo)

    def test_newstyle_class(self):
        class Foo(object):
            pass
        self._check_implementer(Foo)

class Test_classImplementsOnly(_ImplementsTestMixin, unittest.TestCase):
    FUT_SETS_PROVIDED_BY = False

    def _callFUT(self, cls, iface):
        from zope.interface.declarations import classImplementsOnly
        classImplementsOnly(cls, iface)
        return cls

    def test_w_existing_Implements(self):
        from zope.interface.declarations import Implements
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar')
        impl = Implements(IFoo)
        impl.declared = (IFoo,)
        class Foo(object):
            __implemented__ = impl
        impl.inherit = Foo
        self._callFUT(Foo, IBar)
        # Same spec, now different values
        self.assertTrue(Foo.__implemented__ is impl)
        self.assertEqual(impl.inherit, None)
        self.assertEqual(impl.declared, (IBar,))

    def test_oldstyle_class(self):
        from zope.interface.declarations import Implements
        from zope.interface.interface import InterfaceClass
        IBar = InterfaceClass('IBar')
        old_spec = Implements(IBar)

        class Foo:
            __implemented__ = old_spec
        self._check_implementer(Foo, old_spec, '?', inherit=None)

    def test_newstyle_class(self):
        from zope.interface.declarations import Implements
        from zope.interface.interface import InterfaceClass
        IBar = InterfaceClass('IBar')
        old_spec = Implements(IBar)

        class Foo(object):
            __implemented__ = old_spec
        self._check_implementer(Foo, old_spec, '?', inherit=None)


    def test_redundant_with_super_still_implements(self):
        Base, IBase = self._check_implementer(
            type('Foo', (object,), {}),
            inherit=None,
        )

        class Child(Base):
            pass

        self._callFUT(Child, IBase)
        self.assertTrue(IBase.implementedBy(Child))


class Test_classImplements(_ImplementsTestMixin, unittest.TestCase):

    def _callFUT(self, cls, iface):
        from zope.interface.declarations import classImplements
        result = classImplements(cls, iface) # pylint:disable=assignment-from-no-return
        self.assertIsNone(result)
        return cls

    def __check_implementer_redundant(self, Base):
        # If we @implementer exactly what was already present, we write
        # no declared attributes on the parent (we still set everything, though)
        Base, IBase = self._check_implementer(Base)

        class Child(Base):
            pass

        returned = self._callFUT(Child, IBase)
        self.assertIn('__implemented__', returned.__dict__)
        self.assertNotIn('__providedBy__', returned.__dict__)
        self.assertIn('__provides__', returned.__dict__)

        spec = Child.__implemented__
        self.assertEqual(spec.declared, ())
        self.assertEqual(spec.inherit, Child)

        self.assertTrue(IBase.providedBy(Child()))

    def test_redundant_implementer_empty_class_declarations_newstyle(self):
        self.__check_implementer_redundant(type('Foo', (object,), {}))

    def test_redundant_implementer_empty_class_declarations_oldstyle(self):
        # This only matters on Python 2
        class Foo:
            pass
        self.__check_implementer_redundant(Foo)

    def test_redundant_implementer_Interface(self):
        from zope.interface import Interface
        from zope.interface import implementedBy
        from zope.interface import ro
        from zope.interface.tests.test_ro import C3Setting

        class Foo(object):
            pass

        with C3Setting(ro.C3.STRICT_IRO, False):
            self._callFUT(Foo, Interface)
            self.assertEqual(list(implementedBy(Foo)), [Interface])

            class Baz(Foo):
                pass

            self._callFUT(Baz, Interface)
            self.assertEqual(list(implementedBy(Baz)), [Interface])

    def _order_for_two(self, applied_first, applied_second):
        return (applied_first, applied_second)

    def test_w_existing_Implements(self):
        from zope.interface.declarations import Implements
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar')
        impl = Implements(IFoo)
        impl.declared = (IFoo,)
        class Foo(object):
            __implemented__ = impl
        impl.inherit = Foo
        self._callFUT(Foo, IBar)
        # Same spec, now different values
        self.assertIs(Foo.__implemented__, impl)
        self.assertEqual(impl.inherit, Foo)
        self.assertEqual(impl.declared,
                         self._order_for_two(IFoo, IBar))

    def test_w_existing_Implements_w_bases(self):
        from zope.interface.declarations import Implements
        from zope.interface.interface import InterfaceClass
        IRoot = InterfaceClass('IRoot')
        ISecondRoot = InterfaceClass('ISecondRoot')
        IExtendsRoot = InterfaceClass('IExtendsRoot', (IRoot,))

        impl_root = Implements.named('Root', IRoot)
        impl_root.declared = (IRoot,)

        class Root1(object):
            __implemented__ = impl_root
        class Root2(object):
            __implemented__ = impl_root

        impl_extends_root = Implements.named('ExtendsRoot1', IExtendsRoot)
        impl_extends_root.declared = (IExtendsRoot,)
        class ExtendsRoot(Root1, Root2):
            __implemented__ = impl_extends_root
        impl_extends_root.inherit = ExtendsRoot

        self._callFUT(ExtendsRoot, ISecondRoot)
        # Same spec, now different values
        self.assertIs(ExtendsRoot.__implemented__, impl_extends_root)
        self.assertEqual(impl_extends_root.inherit, ExtendsRoot)
        self.assertEqual(impl_extends_root.declared,
                         self._order_for_two(IExtendsRoot, ISecondRoot,))
        self.assertEqual(impl_extends_root.__bases__,
                         self._order_for_two(IExtendsRoot, ISecondRoot) + (impl_root,))


class Test_classImplementsFirst(Test_classImplements):

    def _callFUT(self, cls, iface):
        from zope.interface.declarations import classImplementsFirst
        result = classImplementsFirst(cls, iface) # pylint:disable=assignment-from-no-return
        self.assertIsNone(result)
        return cls

    def _order_for_two(self, applied_first, applied_second):
        return (applied_second, applied_first)


class Test__implements_advice(unittest.TestCase):

    def _callFUT(self, *args, **kw):
        from zope.interface.declarations import _implements_advice
        return _implements_advice(*args, **kw)

    def test_no_existing_implements(self):
        from zope.interface.declarations import classImplements
        from zope.interface.declarations import Implements
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        class Foo(object):
            __implements_advice_data__ = ((IFoo,), classImplements)
        self._callFUT(Foo)
        self.assertNotIn('__implements_advice_data__', Foo.__dict__)
        self.assertIsInstance(Foo.__implemented__, Implements) # pylint:disable=no-member
        self.assertEqual(list(Foo.__implemented__), [IFoo]) # pylint:disable=no-member


class Test_implementer(Test_classImplements):

    def _getTargetClass(self):
        from zope.interface.declarations import implementer
        return implementer

    def _makeOne(self, *args, **kw):
        return self._getTargetClass()(*args, **kw)

    def _callFUT(self, cls, *ifaces):
        decorator = self._makeOne(*ifaces)
        return decorator(cls)

    def test_nonclass_cannot_assign_attr(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        decorator = self._makeOne(IFoo)
        self.assertRaises(TypeError, decorator, object())

    def test_nonclass_can_assign_attr(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        class Foo(object):
            pass
        foo = Foo()
        decorator = self._makeOne(IFoo)
        returned = decorator(foo)
        self.assertTrue(returned is foo)
        spec = foo.__implemented__ # pylint:disable=no-member
        self.assertEqual(spec.__name__, 'zope.interface.tests.test_declarations.?')
        self.assertIsNone(spec.inherit,)
        self.assertIs(foo.__implemented__, spec) # pylint:disable=no-member

    def test_does_not_leak_on_unique_classes(self):
        # Make sure nothing is hanging on to the class or Implements
        # object after they go out of scope. There was briefly a bug
        # in 5.x that caused SpecificationBase._bases (in C) to not be
        # traversed or cleared.
        # https://github.com/zopefoundation/zope.interface/issues/216
        import gc
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')

        begin_count = len(gc.get_objects())

        for _ in range(1900):
            class TestClass(object):
                pass

            self._callFUT(TestClass, IFoo)

        gc.collect()

        end_count = len(gc.get_objects())

        # How many new objects might still be around? In all currently
        # tested interpreters, there aren't any, so our counts should
        # match exactly. When the bug existed, in a steady state, the loop
        # would grow by two objects each iteration
        fudge_factor = 0
        self.assertLessEqual(end_count, begin_count + fudge_factor)



class Test_implementer_only(Test_classImplementsOnly):

    def _getTargetClass(self):
        from zope.interface.declarations import implementer_only
        return implementer_only

    def _makeOne(self, *args, **kw):
        return self._getTargetClass()(*args, **kw)

    def _callFUT(self, cls, iface):
        decorator = self._makeOne(iface)
        return decorator(cls)

    def test_function(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        decorator = self._makeOne(IFoo)
        def _function():
            raise NotImplementedError()
        self.assertRaises(ValueError, decorator, _function)

    def test_method(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        decorator = self._makeOne(IFoo)
        class Bar:
            def _method(self):
                raise NotImplementedError()
        self.assertRaises(ValueError, decorator, Bar._method)



# Test '_implements' by way of 'implements{,Only}', its only callers.

class Test_implementsOnly(unittest.TestCase, _Py3ClassAdvice):

    def test_simple(self):
        import warnings
        from zope.interface.declarations import implementsOnly
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        globs = {'implementsOnly': implementsOnly,
                 'IFoo': IFoo,
                }
        locs = {}
        CODE = "\n".join([
            'class Foo(object):'
            '    implementsOnly(IFoo)',
            ])
        with warnings.catch_warnings(record=True) as log:
            warnings.resetwarnings()
            try:
                exec(CODE, globs, locs)  # pylint:disable=exec-used
            except TypeError:
                self.assertTrue(PYTHON3, "Must be Python 3")
            else:
                if PYTHON3:
                    self.fail("Didn't raise TypeError")
                Foo = locs['Foo']
                spec = Foo.__implemented__
                self.assertEqual(list(spec), [IFoo])
                self.assertEqual(len(log), 0) # no longer warn

    def test_called_once_from_class_w_bases(self):
        from zope.interface.declarations import implements
        from zope.interface.declarations import implementsOnly
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        IBar = InterfaceClass("IBar")
        globs = {'implements': implements,
                 'implementsOnly': implementsOnly,
                 'IFoo': IFoo,
                 'IBar': IBar,
                }
        locs = {}
        CODE = "\n".join([
            'class Foo(object):',
            '    implements(IFoo)',
            'class Bar(Foo):'
            '    implementsOnly(IBar)',
            ])
        if self._run_generated_code(CODE, globs, locs):
            Bar = locs['Bar']
            spec = Bar.__implemented__
            self.assertEqual(list(spec), [IBar])


class Test_implements(unittest.TestCase, _Py3ClassAdvice):

    def test_called_from_function(self):
        import warnings
        from zope.interface.declarations import implements
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        globs = {'implements': implements, 'IFoo': IFoo}
        locs = {}
        CODE = "\n".join([
            'def foo():',
            '    implements(IFoo)'
            ])
        if self._run_generated_code(CODE, globs, locs, False):
            foo = locs['foo']
            with warnings.catch_warnings(record=True) as log:
                warnings.resetwarnings()
                self.assertRaises(TypeError, foo)
                self.assertEqual(len(log), 0) # no longer warn

    def test_called_twice_from_class(self):
        import warnings
        from zope.interface.declarations import implements
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        IBar = InterfaceClass("IBar")
        globs = {'implements': implements, 'IFoo': IFoo, 'IBar': IBar}
        locs = {}
        CODE = "\n".join([
            'class Foo(object):',
            '    implements(IFoo)',
            '    implements(IBar)',
            ])
        with warnings.catch_warnings(record=True) as log:
            warnings.resetwarnings()
            try:
                exec(CODE, globs, locs)  # pylint:disable=exec-used
            except TypeError:
                if not PYTHON3:
                    self.assertEqual(len(log), 0) # no longer warn
            else:
                self.fail("Didn't raise TypeError")

    def test_called_once_from_class(self):
        from zope.interface.declarations import implements
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        globs = {'implements': implements, 'IFoo': IFoo}
        locs = {}
        CODE = "\n".join([
            'class Foo(object):',
            '    implements(IFoo)',
            ])
        if self._run_generated_code(CODE, globs, locs):
            Foo = locs['Foo']
            spec = Foo.__implemented__
            self.assertEqual(list(spec), [IFoo])


class ProvidesClassTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.declarations import ProvidesClass
        return ProvidesClass

    def _makeOne(self, *args, **kw):
        return self._getTargetClass()(*args, **kw)

    def test_simple_class_one_interface(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        spec = self._makeOne(Foo, IFoo)
        self.assertEqual(list(spec), [IFoo])

    def test___reduce__(self):
        from zope.interface.declarations import Provides # the function
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        spec = self._makeOne(Foo, IFoo)
        klass, args = spec.__reduce__()
        self.assertTrue(klass is Provides)
        self.assertEqual(args, (Foo, IFoo))

    def test___get___class(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        spec = self._makeOne(Foo, IFoo)
        Foo.__provides__ = spec
        self.assertTrue(Foo.__provides__ is spec)

    def test___get___instance(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        spec = self._makeOne(Foo, IFoo)
        Foo.__provides__ = spec
        def _test():
            foo = Foo()
            return foo.__provides__
        self.assertRaises(AttributeError, _test)

    def test__repr__(self):
        inst = self._makeOne(type(self))
        self.assertEqual(
            repr(inst),
            "<zope.interface.Provides for %r>"  % type(self)
        )


class Test_Provides(unittest.TestCase):

    def _callFUT(self, *args, **kw):
        from zope.interface.declarations import Provides
        return Provides(*args, **kw)

    def test_no_cached_spec(self):
        from zope.interface import declarations
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        cache = {}
        class Foo(object):
            pass
        with _Monkey(declarations, InstanceDeclarations=cache):
            spec = self._callFUT(Foo, IFoo)
        self.assertEqual(list(spec), [IFoo])
        self.assertTrue(cache[(Foo, IFoo)] is spec)

    def test_w_cached_spec(self):
        from zope.interface import declarations
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        prior = object()
        class Foo(object):
            pass
        cache = {(Foo, IFoo): prior}
        with _Monkey(declarations, InstanceDeclarations=cache):
            spec = self._callFUT(Foo, IFoo)
        self.assertTrue(spec is prior)


class Test_directlyProvides(unittest.TestCase):

    def _callFUT(self, *args, **kw):
        from zope.interface.declarations import directlyProvides
        return directlyProvides(*args, **kw)

    def test_w_normal_object(self):
        from zope.interface.declarations import ProvidesClass
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        obj = Foo()
        self._callFUT(obj, IFoo)
        self.assertIsInstance(obj.__provides__, ProvidesClass) # pylint:disable=no-member
        self.assertEqual(list(obj.__provides__), [IFoo]) # pylint:disable=no-member

    def test_w_class(self):
        from zope.interface.declarations import ClassProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        self._callFUT(Foo, IFoo)
        self.assertIsInstance(Foo.__provides__, ClassProvides) # pylint:disable=no-member
        self.assertEqual(list(Foo.__provides__), [IFoo]) # pylint:disable=no-member

    @_skip_under_py3k
    def test_w_non_descriptor_aware_metaclass(self):
        # There are no non-descriptor-aware types in Py3k
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class MetaClass(type):
            def __getattribute__(cls, name):
                # Emulate metaclass whose base is not the type object.
                if name == '__class__':
                    return cls
                # Under certain circumstances, the implementedByFallback
                # can get here for __dict__
                return type.__getattribute__(cls, name) # pragma: no cover

        class Foo(object):
            __metaclass__ = MetaClass
        obj = Foo()
        self.assertRaises(TypeError, self._callFUT, obj, IFoo)

    def test_w_classless_object(self):
        from zope.interface.declarations import ProvidesClass
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        the_dict = {}
        class Foo(object):
            def __getattribute__(self, name):
                # Emulate object w/o any class
                if name == '__class__':
                    return None
                raise NotImplementedError(name)
            def __setattr__(self, name, value):
                the_dict[name] = value
        obj = Foo()
        self._callFUT(obj, IFoo)
        self.assertIsInstance(the_dict['__provides__'], ProvidesClass)
        self.assertEqual(list(the_dict['__provides__']), [IFoo])


class Test_alsoProvides(unittest.TestCase):

    def _callFUT(self, *args, **kw):
        from zope.interface.declarations import alsoProvides
        return alsoProvides(*args, **kw)

    def test_wo_existing_provides(self):
        from zope.interface.declarations import ProvidesClass
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        obj = Foo()
        self._callFUT(obj, IFoo)
        self.assertIsInstance(obj.__provides__, ProvidesClass) # pylint:disable=no-member
        self.assertEqual(list(obj.__provides__), [IFoo]) # pylint:disable=no-member

    def test_w_existing_provides(self):
        from zope.interface.declarations import directlyProvides
        from zope.interface.declarations import ProvidesClass
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        IBar = InterfaceClass("IBar")
        class Foo(object):
            pass
        obj = Foo()
        directlyProvides(obj, IFoo)
        self._callFUT(obj, IBar)
        self.assertIsInstance(obj.__provides__, ProvidesClass) # pylint:disable=no-member
        self.assertEqual(list(obj.__provides__), [IFoo, IBar]) # pylint:disable=no-member


class Test_noLongerProvides(unittest.TestCase):

    def _callFUT(self, *args, **kw):
        from zope.interface.declarations import noLongerProvides
        return noLongerProvides(*args, **kw)

    def test_wo_existing_provides(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        obj = Foo()
        self._callFUT(obj, IFoo)
        self.assertEqual(list(obj.__provides__), []) # pylint:disable=no-member

    def test_w_existing_provides_hit(self):
        from zope.interface.declarations import directlyProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        obj = Foo()
        directlyProvides(obj, IFoo)
        self._callFUT(obj, IFoo)
        self.assertEqual(list(obj.__provides__), []) # pylint:disable=no-member

    def test_w_existing_provides_miss(self):
        from zope.interface.declarations import directlyProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        IBar = InterfaceClass("IBar")
        class Foo(object):
            pass
        obj = Foo()
        directlyProvides(obj, IFoo)
        self._callFUT(obj, IBar)
        self.assertEqual(list(obj.__provides__), [IFoo]) # pylint:disable=no-member

    def test_w_iface_implemented_by_class(self):
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        @implementer(IFoo)
        class Foo(object):
            pass
        obj = Foo()
        self.assertRaises(ValueError, self._callFUT, obj, IFoo)


class ClassProvidesBaseFallbackTests(unittest.TestCase):

    def _getTargetClass(self):
        # pylint:disable=no-name-in-module
        from zope.interface.declarations import ClassProvidesBaseFallback
        return ClassProvidesBaseFallback

    def _makeOne(self, klass, implements):
        # Don't instantiate directly:  the C version can't have attributes
        # assigned.
        class Derived(self._getTargetClass()):
            def __init__(self, k, i):
                self._cls = k
                self._implements = i
        return Derived(klass, implements)

    def test_w_same_class_via_class(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        cpbp = Foo.__provides__ = self._makeOne(Foo, IFoo)
        self.assertTrue(Foo.__provides__ is cpbp)

    def test_w_same_class_via_instance(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        foo = Foo()
        Foo.__provides__ = self._makeOne(Foo, IFoo)
        self.assertIs(foo.__provides__, IFoo)

    def test_w_different_class(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        class Bar(Foo):
            pass
        bar = Bar()
        Foo.__provides__ = self._makeOne(Foo, IFoo)
        self.assertRaises(AttributeError, getattr, Bar, '__provides__')
        self.assertRaises(AttributeError, getattr, bar, '__provides__')


class ClassProvidesBaseTests(OptimizationTestMixin,
                             ClassProvidesBaseFallbackTests):
    # Repeat tests for C optimizations

    def _getTargetClass(self):
        from zope.interface.declarations import ClassProvidesBase
        return ClassProvidesBase

    def _getFallbackClass(self):
        # pylint:disable=no-name-in-module
        from zope.interface.declarations import ClassProvidesBaseFallback
        return ClassProvidesBaseFallback


class ClassProvidesTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.declarations import ClassProvides
        return ClassProvides

    def _makeOne(self, *args, **kw):
        return self._getTargetClass()(*args, **kw)

    def test_w_simple_metaclass(self):
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        IBar = InterfaceClass("IBar")
        @implementer(IFoo)
        class Foo(object):
            pass
        cp = Foo.__provides__ = self._makeOne(Foo, type(Foo), IBar)
        self.assertTrue(Foo.__provides__ is cp)
        self.assertEqual(list(Foo().__provides__), [IFoo])

    def test___reduce__(self):
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        IBar = InterfaceClass("IBar")
        @implementer(IFoo)
        class Foo(object):
            pass
        cp = Foo.__provides__ = self._makeOne(Foo, type(Foo), IBar)
        self.assertEqual(cp.__reduce__(),
                         (self._getTargetClass(), (Foo, type(Foo), IBar)))

    def test__repr__(self):
        inst = self._makeOne(type(self), type)
        self.assertEqual(
            repr(inst),
            "<zope.interface.declarations.ClassProvides for %r>"  % type(self)
        )

class Test_directlyProvidedBy(unittest.TestCase):

    def _callFUT(self, *args, **kw):
        from zope.interface.declarations import directlyProvidedBy
        return directlyProvidedBy(*args, **kw)

    def test_wo_declarations_in_class_or_instance(self):
        class Foo(object):
            pass
        foo = Foo()
        self.assertEqual(list(self._callFUT(foo)), [])

    def test_w_declarations_in_class_but_not_instance(self):
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        @implementer(IFoo)
        class Foo(object):
            pass
        foo = Foo()
        self.assertEqual(list(self._callFUT(foo)), [])

    def test_w_declarations_in_instance_but_not_class(self):
        from zope.interface.declarations import directlyProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        foo = Foo()
        directlyProvides(foo, IFoo)
        self.assertEqual(list(self._callFUT(foo)), [IFoo])

    def test_w_declarations_in_instance_and_class(self):
        from zope.interface.declarations import directlyProvides
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        IBar = InterfaceClass("IBar")
        @implementer(IFoo)
        class Foo(object):
            pass
        foo = Foo()
        directlyProvides(foo, IBar)
        self.assertEqual(list(self._callFUT(foo)), [IBar])


class Test_classProvides(unittest.TestCase, _Py3ClassAdvice):
    # pylint:disable=exec-used

    def test_called_from_function(self):
        import warnings
        from zope.interface.declarations import classProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        globs = {'classProvides': classProvides, 'IFoo': IFoo}
        locs = {}
        CODE = "\n".join([
            'def foo():',
            '    classProvides(IFoo)'
            ])
        exec(CODE, globs, locs)
        foo = locs['foo']
        with warnings.catch_warnings(record=True) as log:
            warnings.resetwarnings()
            self.assertRaises(TypeError, foo)
            if not PYTHON3:
                self.assertEqual(len(log), 0) # no longer warn

    def test_called_twice_from_class(self):
        import warnings
        from zope.interface.declarations import classProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        IBar = InterfaceClass("IBar")
        globs = {'classProvides': classProvides, 'IFoo': IFoo, 'IBar': IBar}
        locs = {}
        CODE = "\n".join([
            'class Foo(object):',
            '    classProvides(IFoo)',
            '    classProvides(IBar)',
            ])
        with warnings.catch_warnings(record=True) as log:
            warnings.resetwarnings()
            try:
                exec(CODE, globs, locs)
            except TypeError:
                if not PYTHON3:
                    self.assertEqual(len(log), 0) # no longer warn
            else:
                self.fail("Didn't raise TypeError")

    def test_called_once_from_class(self):
        from zope.interface.declarations import classProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        globs = {'classProvides': classProvides, 'IFoo': IFoo}
        locs = {}
        CODE = "\n".join([
            'class Foo(object):',
            '    classProvides(IFoo)',
            ])
        if self._run_generated_code(CODE, globs, locs):
            Foo = locs['Foo']
            spec = Foo.__providedBy__
            self.assertEqual(list(spec), [IFoo])

# Test _classProvides_advice through classProvides, its only caller.


class Test_provider(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.declarations import provider
        return provider

    def _makeOne(self, *args, **kw):
        return self._getTargetClass()(*args, **kw)

    def test_w_class(self):
        from zope.interface.declarations import ClassProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        @self._makeOne(IFoo)
        class Foo(object):
            pass
        self.assertIsInstance(Foo.__provides__, ClassProvides) # pylint:disable=no-member
        self.assertEqual(list(Foo.__provides__), [IFoo]) # pylint:disable=no-member


class Test_moduleProvides(unittest.TestCase):
    # pylint:disable=exec-used

    def test_called_from_function(self):
        from zope.interface.declarations import moduleProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        globs = {'__name__': 'zope.interface.tests.foo',
                 'moduleProvides': moduleProvides, 'IFoo': IFoo}
        locs = {}
        CODE = "\n".join([
            'def foo():',
            '    moduleProvides(IFoo)'
            ])
        exec(CODE, globs, locs)
        foo = locs['foo']
        self.assertRaises(TypeError, foo)

    def test_called_from_class(self):
        from zope.interface.declarations import moduleProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        globs = {'__name__': 'zope.interface.tests.foo',
                 'moduleProvides': moduleProvides, 'IFoo': IFoo}
        locs = {}
        CODE = "\n".join([
            'class Foo(object):',
            '    moduleProvides(IFoo)',
            ])
        with self.assertRaises(TypeError):
            exec(CODE, globs, locs)

    def test_called_once_from_module_scope(self):
        from zope.interface.declarations import moduleProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        globs = {'__name__': 'zope.interface.tests.foo',
                 'moduleProvides': moduleProvides, 'IFoo': IFoo}
        CODE = "\n".join([
            'moduleProvides(IFoo)',
            ])
        exec(CODE, globs)
        spec = globs['__provides__']
        self.assertEqual(list(spec), [IFoo])

    def test_called_twice_from_module_scope(self):
        from zope.interface.declarations import moduleProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        globs = {'__name__': 'zope.interface.tests.foo',
                 'moduleProvides': moduleProvides, 'IFoo': IFoo}

        CODE = "\n".join([
            'moduleProvides(IFoo)',
            'moduleProvides(IFoo)',
            ])
        with self.assertRaises(TypeError):
            exec(CODE, globs)


class Test_getObjectSpecificationFallback(unittest.TestCase):

    def _getFallbackClass(self):
        # pylint:disable=no-name-in-module
        from zope.interface.declarations import getObjectSpecificationFallback
        return getObjectSpecificationFallback

    _getTargetClass = _getFallbackClass

    def _callFUT(self, *args, **kw):
        return self._getTargetClass()(*args, **kw)

    def test_wo_existing_provides_classless(self):
        the_dict = {}
        class Foo(object):
            def __getattribute__(self, name):
                # Emulate object w/o any class
                if name == '__class__':
                    raise AttributeError(name)
                try:
                    return the_dict[name]
                except KeyError:
                    raise AttributeError(name)
            def __setattr__(self, name, value):
                raise NotImplementedError()
        foo = Foo()
        spec = self._callFUT(foo)
        self.assertEqual(list(spec), [])

    def test_existing_provides_is_spec(self):
        from zope.interface.declarations import directlyProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        def foo():
            raise NotImplementedError()
        directlyProvides(foo, IFoo)
        spec = self._callFUT(foo)
        self.assertIs(spec, foo.__provides__) # pylint:disable=no-member

    def test_existing_provides_is_not_spec(self):
        def foo():
            raise NotImplementedError()
        foo.__provides__ = object() # not a valid spec
        spec = self._callFUT(foo)
        self.assertEqual(list(spec), [])

    def test_existing_provides(self):
        from zope.interface.declarations import directlyProvides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        foo = Foo()
        directlyProvides(foo, IFoo)
        spec = self._callFUT(foo)
        self.assertEqual(list(spec), [IFoo])

    def test_wo_provides_on_class_w_implements(self):
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        @implementer(IFoo)
        class Foo(object):
            pass
        foo = Foo()
        spec = self._callFUT(foo)
        self.assertEqual(list(spec), [IFoo])

    def test_wo_provides_on_class_wo_implements(self):
        class Foo(object):
            pass
        foo = Foo()
        spec = self._callFUT(foo)
        self.assertEqual(list(spec), [])

    def test_catches_only_AttributeError_on_provides(self):
        MissingSomeAttrs.test_raises(self, self._callFUT, expected_missing='__provides__')

    def test_catches_only_AttributeError_on_class(self):
        MissingSomeAttrs.test_raises(self, self._callFUT, expected_missing='__class__',
                                     __provides__=None)

    def test_raises_AttributeError_when_provides_fails_type_check_AttributeError(self):
        # isinstance(ob.__provides__, SpecificationBase) is not
        # protected inside any kind of block.

        class Foo(object):
            __provides__ = MissingSomeAttrs(AttributeError)

        # isinstance() ignores AttributeError on __class__
        self._callFUT(Foo())

    def test_raises_AttributeError_when_provides_fails_type_check_RuntimeError(self):
        # isinstance(ob.__provides__, SpecificationBase) is not
        # protected inside any kind of block.
        class Foo(object):
            __provides__ = MissingSomeAttrs(RuntimeError)

        if PYTHON3:
            with self.assertRaises(RuntimeError) as exc:
                self._callFUT(Foo())

            self.assertEqual('__class__', exc.exception.args[0])
        else:
            # Python 2 catches everything.
            self._callFUT(Foo())


class Test_getObjectSpecification(Test_getObjectSpecificationFallback,
                                  OptimizationTestMixin):
    # Repeat tests for C optimizations

    def _getTargetClass(self):
        from zope.interface.declarations import getObjectSpecification
        return getObjectSpecification


class Test_providedByFallback(unittest.TestCase):

    def _getFallbackClass(self):
        # pylint:disable=no-name-in-module
        from zope.interface.declarations import providedByFallback
        return providedByFallback

    _getTargetClass = _getFallbackClass

    def _callFUT(self, *args, **kw):
        return self._getTargetClass()(*args, **kw)

    def test_wo_providedBy_on_class_wo_implements(self):
        class Foo(object):
            pass
        foo = Foo()
        spec = self._callFUT(foo)
        self.assertEqual(list(spec), [])

    def test_w_providedBy_valid_spec(self):
        from zope.interface.declarations import Provides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        foo = Foo()
        foo.__providedBy__ = Provides(Foo, IFoo)
        spec = self._callFUT(foo)
        self.assertEqual(list(spec), [IFoo])

    def test_w_providedBy_invalid_spec(self):
        class Foo(object):
            pass
        foo = Foo()
        foo.__providedBy__ = object()
        spec = self._callFUT(foo)
        self.assertEqual(list(spec), [])

    def test_w_providedBy_invalid_spec_class_w_implements(self):
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        @implementer(IFoo)
        class Foo(object):
            pass
        foo = Foo()
        foo.__providedBy__ = object()
        spec = self._callFUT(foo)
        self.assertEqual(list(spec), [IFoo])

    def test_w_providedBy_invalid_spec_w_provides_no_provides_on_class(self):
        class Foo(object):
            pass
        foo = Foo()
        foo.__providedBy__ = object()
        expected = foo.__provides__ = object()
        spec = self._callFUT(foo)
        self.assertTrue(spec is expected)

    def test_w_providedBy_invalid_spec_w_provides_diff_provides_on_class(self):
        class Foo(object):
            pass
        foo = Foo()
        foo.__providedBy__ = object()
        expected = foo.__provides__ = object()
        Foo.__provides__ = object()
        spec = self._callFUT(foo)
        self.assertTrue(spec is expected)

    def test_w_providedBy_invalid_spec_w_provides_same_provides_on_class(self):
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        @implementer(IFoo)
        class Foo(object):
            pass
        foo = Foo()
        foo.__providedBy__ = object()
        foo.__provides__ = Foo.__provides__ = object()
        spec = self._callFUT(foo)
        self.assertEqual(list(spec), [IFoo])

    def test_super_when_base_implements_interface(self):
        from zope.interface import Interface
        from zope.interface.declarations import implementer

        class IBase(Interface):
            pass

        class IDerived(IBase):
            pass

        @implementer(IBase)
        class Base(object):
            pass

        @implementer(IDerived)
        class Derived(Base):
            pass

        derived = Derived()
        self.assertEqual(list(self._callFUT(derived)), [IDerived, IBase])

        sup = super(Derived, derived)
        fut = self._callFUT(sup)
        self.assertIsNone(fut._dependents)
        self.assertEqual(list(fut), [IBase])

    def test_super_when_base_doesnt_implement_interface(self):
        from zope.interface import Interface
        from zope.interface.declarations import implementer

        class IBase(Interface):
            pass

        class IDerived(IBase):
            pass

        class Base(object):
            pass

        @implementer(IDerived)
        class Derived(Base):
            pass

        derived = Derived()
        self.assertEqual(list(self._callFUT(derived)), [IDerived])

        sup = super(Derived, derived)
        self.assertEqual(list(self._callFUT(sup)), [])

    def test_super_when_base_is_object(self):
        from zope.interface import Interface
        from zope.interface.declarations import implementer

        class IBase(Interface):
            pass

        class IDerived(IBase):
            pass

        @implementer(IDerived)
        class Derived(object):
            pass

        derived = Derived()
        self.assertEqual(list(self._callFUT(derived)), [IDerived])

        sup = super(Derived, derived)
        fut = self._callFUT(sup)
        self.assertIsNone(fut._dependents)
        self.assertEqual(list(fut), [])

    def test_super_when_object_directly_provides(self):
        from zope.interface import Interface
        from zope.interface.declarations import implementer
        from zope.interface.declarations import directlyProvides

        class IBase(Interface):
            pass

        class IDerived(IBase):
            pass

        @implementer(IBase)
        class Base(object):
            pass

        class Derived(Base):
            pass

        derived = Derived()
        self.assertEqual(list(self._callFUT(derived)), [IBase])

        directlyProvides(derived, IDerived)
        self.assertEqual(list(self._callFUT(derived)), [IDerived, IBase])

        sup = super(Derived, derived)
        fut = self._callFUT(sup)
        self.assertIsNone(fut._dependents)
        self.assertEqual(list(fut), [IBase])

    def test_super_multi_level_multi_inheritance(self):
        from zope.interface.declarations import implementer
        from zope.interface import Interface

        class IBase(Interface):
            pass

        class IM1(Interface):
            pass

        class IM2(Interface):
            pass

        class IDerived(IBase):
            pass

        class IUnrelated(Interface):
            pass

        @implementer(IBase)
        class Base(object):
            pass

        @implementer(IM1)
        class M1(Base):
            pass

        @implementer(IM2)
        class M2(Base):
            pass

        @implementer(IDerived, IUnrelated)
        class Derived(M1, M2):
            pass

        d = Derived()
        sd = super(Derived, d)
        sm1 = super(M1, d)
        sm2 = super(M2, d)

        self.assertEqual(list(self._callFUT(d)),
                         [IDerived, IUnrelated, IM1, IBase, IM2])
        self.assertEqual(list(self._callFUT(sd)),
                         [IM1, IBase, IM2])
        self.assertEqual(list(self._callFUT(sm1)),
                         [IM2, IBase])
        self.assertEqual(list(self._callFUT(sm2)),
                         [IBase])

    def test_catches_only_AttributeError_on_providedBy(self):
        MissingSomeAttrs.test_raises(self, self._callFUT,
                                     expected_missing='__providedBy__',
                                     __class__=object)

    def test_catches_only_AttributeError_on_class(self):
        # isinstance() tries to get the __class__, which is non-obvious,
        # so it must be protected too.
        PY3 = str is not bytes
        MissingSomeAttrs.test_raises(self, self._callFUT,
                                     expected_missing='__class__' if PY3 else '__providedBy__')



class Test_providedBy(Test_providedByFallback,
                      OptimizationTestMixin):
    # Repeat tests for C optimizations

    def _getTargetClass(self):
        from zope.interface.declarations import providedBy
        return providedBy


class ObjectSpecificationDescriptorFallbackTests(unittest.TestCase):

    def _getFallbackClass(self):
        # pylint:disable=no-name-in-module
        from zope.interface.declarations \
            import ObjectSpecificationDescriptorFallback
        return ObjectSpecificationDescriptorFallback

    _getTargetClass = _getFallbackClass

    def _makeOne(self, *args, **kw):
        return self._getTargetClass()(*args, **kw)

    def test_accessed_via_class(self):
        from zope.interface.declarations import Provides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        class Foo(object):
            pass
        Foo.__provides__ = Provides(Foo, IFoo)
        Foo.__providedBy__ = self._makeOne()
        self.assertEqual(list(Foo.__providedBy__), [IFoo])

    def test_accessed_via_inst_wo_provides(self):
        from zope.interface.declarations import implementer
        from zope.interface.declarations import Provides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        IBar = InterfaceClass("IBar")
        @implementer(IFoo)
        class Foo(object):
            pass
        Foo.__provides__ = Provides(Foo, IBar)
        Foo.__providedBy__ = self._makeOne()
        foo = Foo()
        self.assertEqual(list(foo.__providedBy__), [IFoo])

    def test_accessed_via_inst_w_provides(self):
        from zope.interface.declarations import directlyProvides
        from zope.interface.declarations import implementer
        from zope.interface.declarations import Provides
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass("IFoo")
        IBar = InterfaceClass("IBar")
        IBaz = InterfaceClass("IBaz")
        @implementer(IFoo)
        class Foo(object):
            pass
        Foo.__provides__ = Provides(Foo, IBar)
        Foo.__providedBy__ = self._makeOne()
        foo = Foo()
        directlyProvides(foo, IBaz)
        self.assertEqual(list(foo.__providedBy__), [IBaz, IFoo])


class ObjectSpecificationDescriptorTests(
        ObjectSpecificationDescriptorFallbackTests,
        OptimizationTestMixin):
    # Repeat tests for C optimizations

    def _getTargetClass(self):
        from zope.interface.declarations import ObjectSpecificationDescriptor
        return ObjectSpecificationDescriptor


# Test _normalizeargs through its callers.


class _Monkey(object):
    # context-manager for replacing module names in the scope of a test.
    def __init__(self, module, **kw):
        self.module = module
        self.to_restore = {key: getattr(module, key) for key in kw}
        for key, value in kw.items():
            setattr(module, key, value)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        for key, value in self.to_restore.items():
            setattr(self.module, key, value)


class _MonkeyDict(object):
    # context-manager for restoring a dict w/in a module in the scope of a test.
    def __init__(self, module, attrname, **kw):
        self.module = module
        self.target = getattr(module, attrname)
        self.to_restore = self.target.copy()
        self.target.clear()
        self.target.update(kw)

    def __enter__(self):
        return self.target

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.target.clear()
        self.target.update(self.to_restore)
