##############################################################################
#
# Copyright (c) 2001, 2002 Zope Foundation and Contributors.
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
"""Test Interface implementation
"""
# Things we let slide because it's a test
# pylint:disable=protected-access,blacklisted-name,attribute-defined-outside-init
# pylint:disable=too-many-public-methods,too-many-lines,abstract-method
# pylint:disable=redefined-builtin,signature-differs,arguments-differ
# Things you get inheriting from Interface
# pylint:disable=inherit-non-class,no-self-argument,no-method-argument
# Things you get using methods of an Interface 'subclass'
# pylint:disable=no-value-for-parameter
import unittest

from zope.interface._compat import _skip_under_py3k
from zope.interface.tests import MissingSomeAttrs
from zope.interface.tests import OptimizationTestMixin
from zope.interface.tests import CleanUp

_marker = object()


class Test_invariant(unittest.TestCase):

    def test_w_single(self):
        from zope.interface.interface import invariant
        from zope.interface.interface import TAGGED_DATA

        def _check(*args, **kw):
            raise NotImplementedError()

        class Foo(object):
            invariant(_check)

        self.assertEqual(getattr(Foo, TAGGED_DATA, None),
                         {'invariants': [_check]})

    def test_w_multiple(self):
        from zope.interface.interface import invariant
        from zope.interface.interface import TAGGED_DATA

        def _check(*args, **kw):
            raise NotImplementedError()

        def _another_check(*args, **kw):
            raise NotImplementedError()

        class Foo(object):
            invariant(_check)
            invariant(_another_check)

        self.assertEqual(getattr(Foo, TAGGED_DATA, None),
                         {'invariants': [_check, _another_check]})


class Test_taggedValue(unittest.TestCase):

    def test_w_single(self):
        from zope.interface.interface import taggedValue
        from zope.interface.interface import TAGGED_DATA

        class Foo(object):
            taggedValue('bar', ['baz'])

        self.assertEqual(getattr(Foo, TAGGED_DATA, None),
                         {'bar': ['baz']})

    def test_w_multiple(self):
        from zope.interface.interface import taggedValue
        from zope.interface.interface import TAGGED_DATA

        class Foo(object):
            taggedValue('bar', ['baz'])
            taggedValue('qux', 'spam')

        self.assertEqual(getattr(Foo, TAGGED_DATA, None),
                         {'bar': ['baz'], 'qux': 'spam'})

    def test_w_multiple_overwriting(self):
        from zope.interface.interface import taggedValue
        from zope.interface.interface import TAGGED_DATA

        class Foo(object):
            taggedValue('bar', ['baz'])
            taggedValue('qux', 'spam')
            taggedValue('bar', 'frob')

        self.assertEqual(getattr(Foo, TAGGED_DATA, None),
                         {'bar': 'frob', 'qux': 'spam'})


class ElementTests(unittest.TestCase):

    DEFAULT_NAME = 'AnElement'

    def _getTargetClass(self):
        from zope.interface.interface import Element
        return Element

    def _makeOne(self, name=None):
        if name is None:
            name = self.DEFAULT_NAME
        return self._getTargetClass()(name)

    def test_ctor_defaults(self):
        element = self._makeOne()
        self.assertEqual(element.__name__, self.DEFAULT_NAME)
        self.assertEqual(element.getName(), self.DEFAULT_NAME)
        self.assertEqual(element.__doc__, '')
        self.assertEqual(element.getDoc(), '')
        self.assertEqual(list(element.getTaggedValueTags()), [])

    def test_ctor_no_doc_space_in_name(self):
        element = self._makeOne('An Element')
        self.assertEqual(element.__name__, None)
        self.assertEqual(element.__doc__, 'An Element')

    def test_getTaggedValue_miss(self):
        element = self._makeOne()
        self.assertRaises(KeyError, element.getTaggedValue, 'nonesuch')

    def test_getDirectTaggedValueTags(self):
        element = self._makeOne()
        self.assertEqual([], list(element.getDirectTaggedValueTags()))

        element.setTaggedValue('foo', 'bar')
        self.assertEqual(['foo'], list(element.getDirectTaggedValueTags()))

    def test_queryTaggedValue_miss(self):
        element = self._makeOne()
        self.assertEqual(element.queryTaggedValue('nonesuch'), None)

    def test_queryTaggedValue_miss_w_default(self):
        element = self._makeOne()
        self.assertEqual(element.queryTaggedValue('nonesuch', 'bar'), 'bar')

    def test_getDirectTaggedValue_miss(self):
        element = self._makeOne()
        self.assertRaises(KeyError, element.getDirectTaggedValue, 'nonesuch')

    def test_queryDirectTaggedValue_miss(self):
        element = self._makeOne()
        self.assertEqual(element.queryDirectTaggedValue('nonesuch'), None)

    def test_queryDirectTaggedValue_miss_w_default(self):
        element = self._makeOne()
        self.assertEqual(element.queryDirectTaggedValue('nonesuch', 'bar'), 'bar')

    def test_setTaggedValue(self):
        element = self._makeOne()
        element.setTaggedValue('foo', 'bar')
        self.assertEqual(list(element.getTaggedValueTags()), ['foo'])
        self.assertEqual(element.getTaggedValue('foo'), 'bar')
        self.assertEqual(element.queryTaggedValue('foo'), 'bar')

    def test_verifies(self):
        from zope.interface.interfaces import IElement
        from zope.interface.verify import verifyObject

        element = self._makeOne()
        verifyObject(IElement, element)


class GenericSpecificationBaseTests(unittest.TestCase):
    # Tests that work with both implementations
    def _getFallbackClass(self):
        from zope.interface.interface import SpecificationBasePy # pylint:disable=no-name-in-module
        return SpecificationBasePy

    _getTargetClass = _getFallbackClass

    def _makeOne(self):
        return self._getTargetClass()()

    def test_providedBy_miss(self):
        from zope.interface import interface
        from zope.interface.declarations import _empty
        sb = self._makeOne()
        def _providedBy(obj):
            return _empty
        with _Monkey(interface, providedBy=_providedBy):
            self.assertFalse(sb.providedBy(object()))

    def test_implementedBy_miss(self):
        from zope.interface import interface
        from zope.interface.declarations import _empty
        sb = self._makeOne()
        def _implementedBy(obj):
            return _empty
        with _Monkey(interface, implementedBy=_implementedBy):
            self.assertFalse(sb.implementedBy(object()))


class SpecificationBaseTests(GenericSpecificationBaseTests,
                             OptimizationTestMixin):
    # Tests that use the C implementation

    def _getTargetClass(self):
        from zope.interface.interface import SpecificationBase
        return SpecificationBase

class SpecificationBasePyTests(GenericSpecificationBaseTests):
    # Tests that only work with the Python implementation

    def test___call___miss(self):
        sb = self._makeOne()
        sb._implied = {}  # not defined by SpecificationBasePy
        self.assertFalse(sb.isOrExtends(object()))

    def test___call___hit(self):
        sb = self._makeOne()
        testing = object()
        sb._implied = {testing: {}}  # not defined by SpecificationBasePy
        self.assertTrue(sb(testing))

    def test_isOrExtends_miss(self):
        sb = self._makeOne()
        sb._implied = {}  # not defined by SpecificationBasePy
        self.assertFalse(sb.isOrExtends(object()))

    def test_isOrExtends_hit(self):
        sb = self._makeOne()
        testing = object()
        sb._implied = {testing: {}}  # not defined by SpecificationBasePy
        self.assertTrue(sb(testing))

    def test_implementedBy_hit(self):
        from zope.interface import interface
        sb = self._makeOne()
        class _Decl(object):
            _implied = {sb: {},}
        def _implementedBy(obj):
            return _Decl()
        with _Monkey(interface, implementedBy=_implementedBy):
            self.assertTrue(sb.implementedBy(object()))

    def test_providedBy_hit(self):
        from zope.interface import interface
        sb = self._makeOne()
        class _Decl(object):
            _implied = {sb: {},}
        def _providedBy(obj):
            return _Decl()
        with _Monkey(interface, providedBy=_providedBy):
            self.assertTrue(sb.providedBy(object()))


class NameAndModuleComparisonTestsMixin(CleanUp):

    def _makeOneToCompare(self):
        return self._makeOne('a', 'b')

    def __check_NotImplemented_comparison(self, name):
        # Without the correct attributes of __name__ and __module__,
        # comparison switches to the reverse direction.

        import operator
        ib = self._makeOneToCompare()
        op = getattr(operator, name)
        meth = getattr(ib, '__%s__' % name)

        # If either the __name__ or __module__ attribute
        # is missing from the other object, then we return
        # NotImplemented.
        class RaisesErrorOnMissing(object):
            Exc = AttributeError
            def __getattribute__(self, name):
                try:
                    return object.__getattribute__(self, name)
                except AttributeError:
                    exc = RaisesErrorOnMissing.Exc
                    raise exc(name)

        class RaisesErrorOnModule(RaisesErrorOnMissing):
            def __init__(self):
                self.__name__ = 'foo'
            @property
            def __module__(self):
                raise AttributeError

        class RaisesErrorOnName(RaisesErrorOnMissing):
            def __init__(self):
                self.__module__ = 'foo'

        self.assertEqual(RaisesErrorOnModule().__name__, 'foo')
        self.assertEqual(RaisesErrorOnName().__module__, 'foo')
        with self.assertRaises(AttributeError):
            getattr(RaisesErrorOnModule(), '__module__')
        with self.assertRaises(AttributeError):
            getattr(RaisesErrorOnName(), '__name__')

        for cls in RaisesErrorOnModule, RaisesErrorOnName:
            self.assertIs(meth(cls()), NotImplemented)

        # If the other object has a comparison function, returning
        # NotImplemented means Python calls it.

        class AllowsAnyComparison(RaisesErrorOnMissing):
            def __eq__(self, other):
                return True
            __lt__ = __eq__
            __le__ = __eq__
            __gt__ = __eq__
            __ge__ = __eq__
            __ne__ = __eq__

        self.assertTrue(op(ib, AllowsAnyComparison()))
        self.assertIs(meth(AllowsAnyComparison()), NotImplemented)

        # If it doesn't have the comparison, Python raises a TypeError.
        class AllowsNoComparison(object):
            __eq__ = None
            __lt__ = __eq__
            __le__ = __eq__
            __gt__ = __eq__
            __ge__ = __eq__
            __ne__ = __eq__

        self.assertIs(meth(AllowsNoComparison()), NotImplemented)
        with self.assertRaises(TypeError):
            op(ib, AllowsNoComparison())

        # Errors besides AttributeError are passed
        class MyException(Exception):
            pass

        RaisesErrorOnMissing.Exc = MyException

        with self.assertRaises(MyException):
            getattr(RaisesErrorOnModule(), '__module__')
        with self.assertRaises(MyException):
            getattr(RaisesErrorOnName(), '__name__')

        for cls in RaisesErrorOnModule, RaisesErrorOnName:
            with self.assertRaises(MyException):
                op(ib, cls())
            with self.assertRaises(MyException):
                meth(cls())

    def test__lt__NotImplemented(self):
        self.__check_NotImplemented_comparison('lt')

    def test__le__NotImplemented(self):
        self.__check_NotImplemented_comparison('le')

    def test__gt__NotImplemented(self):
        self.__check_NotImplemented_comparison('gt')

    def test__ge__NotImplemented(self):
        self.__check_NotImplemented_comparison('ge')


class InterfaceBaseTestsMixin(NameAndModuleComparisonTestsMixin):
    # Tests for both C and Python implementation

    def _getTargetClass(self):
        raise NotImplementedError

    def _getFallbackClass(self):
        # pylint:disable=no-name-in-module
        from zope.interface.interface import InterfaceBasePy
        return InterfaceBasePy

    def _makeOne(self, object_should_provide=False, name=None, module=None):
        class IB(self._getTargetClass()):
            def _call_conform(self, conform):
                return conform(self)
            def providedBy(self, obj):
                return object_should_provide
        return IB(name, module)

    def test___call___w___conform___returning_value(self):
        ib = self._makeOne(False)
        conformed = object()
        class _Adapted(object):
            def __conform__(self, iface):
                return conformed
        self.assertIs(ib(_Adapted()), conformed)

    def test___call___wo___conform___ob_no_provides_w_alternate(self):
        ib = self._makeOne(False)
        __traceback_info__ = ib, self._getTargetClass()
        adapted = object()
        alternate = object()
        self.assertIs(ib(adapted, alternate), alternate)

    def test___call___w___conform___ob_no_provides_wo_alternate(self):
        ib = self._makeOne(False)
        with self.assertRaises(TypeError) as exc:
            ib(object())

        self.assertIn('Could not adapt', str(exc.exception))

    def test___call___w_no_conform_catches_only_AttributeError(self):
        MissingSomeAttrs.test_raises(self, self._makeOne(), expected_missing='__conform__')


class InterfaceBaseTests(InterfaceBaseTestsMixin,
                         OptimizationTestMixin,
                         unittest.TestCase):
    # Tests that work with the C implementation
    def _getTargetClass(self):
        from zope.interface.interface import InterfaceBase
        return InterfaceBase


class InterfaceBasePyTests(InterfaceBaseTestsMixin, unittest.TestCase):
    # Tests that only work with the Python implementation

    _getTargetClass = InterfaceBaseTestsMixin._getFallbackClass

    def test___call___w___conform___miss_ob_provides(self):
        ib = self._makeOne(True)
        class _Adapted(object):
            def __conform__(self, iface):
                return None
        adapted = _Adapted()
        self.assertIs(ib(adapted), adapted)

    def test___adapt___ob_provides(self):
        ib = self._makeOne(True)
        adapted = object()
        self.assertIs(ib.__adapt__(adapted), adapted)

    def test___adapt___ob_no_provides_uses_hooks(self):
        from zope.interface import interface
        ib = self._makeOne(False)
        adapted = object()
        _missed = []
        def _hook_miss(iface, obj):
            _missed.append((iface, obj))
        def _hook_hit(iface, obj):
            return obj
        with _Monkey(interface, adapter_hooks=[_hook_miss, _hook_hit]):
            self.assertIs(ib.__adapt__(adapted), adapted)
            self.assertEqual(_missed, [(ib, adapted)])

class SpecificationTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.interface import Specification
        return Specification

    def _makeOne(self, bases=_marker):
        if bases is _marker:
            return self._getTargetClass()()
        return self._getTargetClass()(bases)

    def test_ctor(self):
        from zope.interface.interface import Interface
        spec = self._makeOne()
        self.assertEqual(spec.__bases__, ())
        self.assertEqual(len(spec._implied), 2)
        self.assertTrue(spec in spec._implied)
        self.assertTrue(Interface in spec._implied)
        self.assertEqual(len(spec.dependents), 0)

    def test_subscribe_first_time(self):
        spec = self._makeOne()
        dep = DummyDependent()
        spec.subscribe(dep)
        self.assertEqual(len(spec.dependents), 1)
        self.assertEqual(spec.dependents[dep], 1)

    def test_subscribe_again(self):
        spec = self._makeOne()
        dep = DummyDependent()
        spec.subscribe(dep)
        spec.subscribe(dep)
        self.assertEqual(spec.dependents[dep], 2)

    def test_unsubscribe_miss(self):
        spec = self._makeOne()
        dep = DummyDependent()
        self.assertRaises(KeyError, spec.unsubscribe, dep)

    def test_unsubscribe(self):
        spec = self._makeOne()
        dep = DummyDependent()
        spec.subscribe(dep)
        spec.subscribe(dep)
        spec.unsubscribe(dep)
        self.assertEqual(spec.dependents[dep], 1)
        spec.unsubscribe(dep)
        self.assertFalse(dep in spec.dependents)

    def test___setBases_subscribes_bases_and_notifies_dependents(self):
        from zope.interface.interface import Interface
        spec = self._makeOne()
        dep = DummyDependent()
        spec.subscribe(dep)
        class I(Interface):
            pass
        class J(Interface):
            pass
        spec.__bases__ = (I,)
        self.assertEqual(dep._changed, [spec])
        self.assertEqual(I.dependents[spec], 1)
        spec.__bases__ = (J,)
        self.assertEqual(I.dependents.get(spec), None)
        self.assertEqual(J.dependents[spec], 1)

    def test_changed_clears_volatiles_and_implied(self):
        from zope.interface.interface import Interface
        class I(Interface):
            pass
        spec = self._makeOne()
        spec._v_attrs = 'Foo'
        spec._implied[I] = ()
        spec.changed(spec)
        self.assertIsNone(spec._v_attrs)
        self.assertFalse(I in spec._implied)

    def test_interfaces_skips_already_seen(self):
        from zope.interface.interface import Interface
        class IFoo(Interface):
            pass
        spec = self._makeOne([IFoo, IFoo])
        self.assertEqual(list(spec.interfaces()), [IFoo])

    def test_extends_strict_wo_self(self):
        from zope.interface.interface import Interface
        class IFoo(Interface):
            pass
        spec = self._makeOne(IFoo)
        self.assertFalse(spec.extends(IFoo, strict=True))

    def test_extends_strict_w_self(self):
        spec = self._makeOne()
        self.assertFalse(spec.extends(spec, strict=True))

    def test_extends_non_strict_w_self(self):
        spec = self._makeOne()
        self.assertTrue(spec.extends(spec, strict=False))

    def test_get_hit_w__v_attrs(self):
        spec = self._makeOne()
        foo = object()
        spec._v_attrs = {'foo': foo}
        self.assertTrue(spec.get('foo') is foo)

    def test_get_hit_from_base_wo__v_attrs(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import Interface
        class IFoo(Interface):
            foo = Attribute('foo')
        class IBar(Interface):
            bar = Attribute('bar')
        spec = self._makeOne([IFoo, IBar])
        self.assertTrue(spec.get('foo') is IFoo.get('foo'))
        self.assertTrue(spec.get('bar') is IBar.get('bar'))

    def test_multiple_inheritance_no_interfaces(self):
        # If we extend an object that implements interfaces,
        # plus ane that doesn't, we do not interject `Interface`
        # early in the resolution order. It stays at the end,
        # like it should.
        # See https://github.com/zopefoundation/zope.interface/issues/8
        from zope.interface.interface import Interface
        from zope.interface.declarations import implementer
        from zope.interface.declarations import implementedBy

        class IDefaultViewName(Interface):
            pass

        class Context(object):
            pass

        class RDBModel(Context):
            pass

        class IOther(Interface):
            pass

        @implementer(IOther)
        class OtherBase(object):
            pass

        class Model(OtherBase, Context):
            pass

        self.assertEqual(
            implementedBy(Model).__sro__,
            (
                implementedBy(Model),
                implementedBy(OtherBase),
                IOther,
                implementedBy(Context),
                implementedBy(object),
                Interface, # This used to be wrong, it used to be 2 too high.
            )
        )


class InterfaceClassTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.interface import InterfaceClass
        return InterfaceClass

    def _makeOne(self, name='ITest', bases=(), attrs=None, __doc__=None,
                 __module__=None):
        return self._getTargetClass()(name, bases, attrs, __doc__, __module__)

    def test_ctor_defaults(self):
        klass = self._getTargetClass()
        inst = klass('ITesting')
        self.assertEqual(inst.__name__, 'ITesting')
        self.assertEqual(inst.__doc__, '')
        self.assertEqual(inst.__bases__, ())
        self.assertEqual(inst.getBases(), ())

    def test_ctor_bad_bases(self):
        klass = self._getTargetClass()
        self.assertRaises(TypeError, klass, 'ITesting', (object(),))

    def test_ctor_w_attrs_attrib_methods(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        ATTRS = {'foo': Attribute('Foo', ''),
                 'bar': fromFunction(_bar),
                }
        klass = self._getTargetClass()
        inst = klass('ITesting', attrs=ATTRS)
        self.assertEqual(inst.__name__, 'ITesting')
        self.assertEqual(inst.__doc__, '')
        self.assertEqual(inst.__bases__, ())
        self.assertEqual(inst.names(), ATTRS.keys())

    def test_ctor_attrs_w___locals__(self):
        ATTRS = {'__locals__': {}}
        klass = self._getTargetClass()
        inst = klass('ITesting', attrs=ATTRS)
        self.assertEqual(inst.__name__, 'ITesting')
        self.assertEqual(inst.__doc__, '')
        self.assertEqual(inst.__bases__, ())
        self.assertEqual(list(inst.names()), [])

    def test_ctor_attrs_w___annotations__(self):
        ATTRS = {'__annotations__': {}}
        klass = self._getTargetClass()
        inst = klass('ITesting', attrs=ATTRS)
        self.assertEqual(inst.__name__, 'ITesting')
        self.assertEqual(inst.__doc__, '')
        self.assertEqual(inst.__bases__, ())
        self.assertEqual(list(inst.names()), [])

    def test_ctor_attrs_w__decorator_non_return(self):
        from zope.interface.interface import _decorator_non_return
        ATTRS = {'dropme': _decorator_non_return}
        klass = self._getTargetClass()
        inst = klass('ITesting', attrs=ATTRS)
        self.assertEqual(inst.__name__, 'ITesting')
        self.assertEqual(inst.__doc__, '')
        self.assertEqual(inst.__bases__, ())
        self.assertEqual(list(inst.names()), [])

    def test_ctor_attrs_w_invalid_attr_type(self):
        from zope.interface.exceptions import InvalidInterface
        ATTRS = {'invalid': object()}
        klass = self._getTargetClass()
        self.assertRaises(InvalidInterface, klass, 'ITesting', attrs=ATTRS)

    def test_ctor_w_explicit___doc__(self):
        ATTRS = {'__doc__': 'ATTR'}
        klass = self._getTargetClass()
        inst = klass('ITesting', attrs=ATTRS, __doc__='EXPLICIT')
        self.assertEqual(inst.__doc__, 'EXPLICIT')

    def test_interfaces(self):
        iface = self._makeOne()
        self.assertEqual(list(iface.interfaces()), [iface])

    def test_getBases(self):
        iface = self._makeOne()
        sub = self._makeOne('ISub', bases=(iface,))
        self.assertEqual(sub.getBases(), (iface,))

    def test_isEqualOrExtendedBy_identity(self):
        iface = self._makeOne()
        self.assertTrue(iface.isEqualOrExtendedBy(iface))

    def test_isEqualOrExtendedBy_subiface(self):
        iface = self._makeOne()
        sub = self._makeOne('ISub', bases=(iface,))
        self.assertTrue(iface.isEqualOrExtendedBy(sub))
        self.assertFalse(sub.isEqualOrExtendedBy(iface))

    def test_isEqualOrExtendedBy_unrelated(self):
        one = self._makeOne('One')
        another = self._makeOne('Another')
        self.assertFalse(one.isEqualOrExtendedBy(another))
        self.assertFalse(another.isEqualOrExtendedBy(one))

    def test_names_w_all_False_ignores_bases(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        BASE_ATTRS = {'foo': Attribute('Foo', ''),
                      'bar': fromFunction(_bar),
                     }
        DERIVED_ATTRS = {'baz': Attribute('Baz', ''),
                        }
        base = self._makeOne('IBase', attrs=BASE_ATTRS)
        derived = self._makeOne('IDerived', bases=(base,), attrs=DERIVED_ATTRS)
        self.assertEqual(sorted(derived.names(all=False)), ['baz'])

    def test_names_w_all_True_no_bases(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        ATTRS = {'foo': Attribute('Foo', ''),
                 'bar': fromFunction(_bar),
                }
        one = self._makeOne(attrs=ATTRS)
        self.assertEqual(sorted(one.names(all=True)), ['bar', 'foo'])

    def test_names_w_all_True_w_bases_simple(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        BASE_ATTRS = {'foo': Attribute('Foo', ''),
                      'bar': fromFunction(_bar),
                     }
        DERIVED_ATTRS = {'baz': Attribute('Baz', ''),
                        }
        base = self._makeOne('IBase', attrs=BASE_ATTRS)
        derived = self._makeOne('IDerived', bases=(base,), attrs=DERIVED_ATTRS)
        self.assertEqual(sorted(derived.names(all=True)), ['bar', 'baz', 'foo'])

    def test_names_w_all_True_bases_w_same_names(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        def _foo():
            """DOCSTRING"""
        BASE_ATTRS = {'foo': Attribute('Foo', ''),
                      'bar': fromFunction(_bar),
                     }
        DERIVED_ATTRS = {'foo': fromFunction(_foo),
                         'baz': Attribute('Baz', ''),
                        }
        base = self._makeOne('IBase', attrs=BASE_ATTRS)
        derived = self._makeOne('IDerived', bases=(base,), attrs=DERIVED_ATTRS)
        self.assertEqual(sorted(derived.names(all=True)), ['bar', 'baz', 'foo'])

    def test___iter__(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        def _foo():
            """DOCSTRING"""
        BASE_ATTRS = {'foo': Attribute('Foo', ''),
                      'bar': fromFunction(_bar),
                     }
        DERIVED_ATTRS = {'foo': fromFunction(_foo),
                         'baz': Attribute('Baz', ''),
                        }
        base = self._makeOne('IBase', attrs=BASE_ATTRS)
        derived = self._makeOne('IDerived', bases=(base,), attrs=DERIVED_ATTRS)
        self.assertEqual(sorted(derived), ['bar', 'baz', 'foo'])

    def test_namesAndDescriptions_w_all_False_ignores_bases(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        BASE_ATTRS = {'foo': Attribute('Foo', ''),
                      'bar': fromFunction(_bar),
                     }
        DERIVED_ATTRS = {'baz': Attribute('Baz', ''),
                        }
        base = self._makeOne('IBase', attrs=BASE_ATTRS)
        derived = self._makeOne('IDerived', bases=(base,), attrs=DERIVED_ATTRS)
        self.assertEqual(sorted(derived.namesAndDescriptions(all=False)),
                         [('baz', DERIVED_ATTRS['baz']),
                         ])

    def test_namesAndDescriptions_w_all_True_no_bases(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        ATTRS = {'foo': Attribute('Foo', ''),
                 'bar': fromFunction(_bar),
                }
        one = self._makeOne(attrs=ATTRS)
        self.assertEqual(sorted(one.namesAndDescriptions(all=False)),
                         [('bar', ATTRS['bar']),
                          ('foo', ATTRS['foo']),
                         ])

    def test_namesAndDescriptions_w_all_True_simple(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        BASE_ATTRS = {'foo': Attribute('Foo', ''),
                      'bar': fromFunction(_bar),
                     }
        DERIVED_ATTRS = {'baz': Attribute('Baz', ''),
                        }
        base = self._makeOne('IBase', attrs=BASE_ATTRS)
        derived = self._makeOne('IDerived', bases=(base,), attrs=DERIVED_ATTRS)
        self.assertEqual(sorted(derived.namesAndDescriptions(all=True)),
                         [('bar', BASE_ATTRS['bar']),
                          ('baz', DERIVED_ATTRS['baz']),
                          ('foo', BASE_ATTRS['foo']),
                         ])

    def test_namesAndDescriptions_w_all_True_bases_w_same_names(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        def _foo():
            """DOCSTRING"""
        BASE_ATTRS = {'foo': Attribute('Foo', ''),
                      'bar': fromFunction(_bar),
                     }
        DERIVED_ATTRS = {'foo': fromFunction(_foo),
                         'baz': Attribute('Baz', ''),
                        }
        base = self._makeOne('IBase', attrs=BASE_ATTRS)
        derived = self._makeOne('IDerived', bases=(base,), attrs=DERIVED_ATTRS)
        self.assertEqual(sorted(derived.namesAndDescriptions(all=True)),
                         [('bar', BASE_ATTRS['bar']),
                          ('baz', DERIVED_ATTRS['baz']),
                          ('foo', DERIVED_ATTRS['foo']),
                         ])

    def test_getDescriptionFor_miss(self):
        one = self._makeOne()
        self.assertRaises(KeyError, one.getDescriptionFor, 'nonesuch')

    def test_getDescriptionFor_hit(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        ATTRS = {'foo': Attribute('Foo', ''),
                 'bar': fromFunction(_bar),
                }
        one = self._makeOne(attrs=ATTRS)
        self.assertEqual(one.getDescriptionFor('foo'), ATTRS['foo'])
        self.assertEqual(one.getDescriptionFor('bar'), ATTRS['bar'])

    def test___getitem___miss(self):
        one = self._makeOne()
        def _test():
            return one['nonesuch']
        self.assertRaises(KeyError, _test)

    def test___getitem___hit(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        ATTRS = {'foo': Attribute('Foo', ''),
                 'bar': fromFunction(_bar),
                }
        one = self._makeOne(attrs=ATTRS)
        self.assertEqual(one['foo'], ATTRS['foo'])
        self.assertEqual(one['bar'], ATTRS['bar'])

    def test___contains___miss(self):
        one = self._makeOne()
        self.assertFalse('nonesuch' in one)

    def test___contains___hit(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        ATTRS = {'foo': Attribute('Foo', ''),
                 'bar': fromFunction(_bar),
                }
        one = self._makeOne(attrs=ATTRS)
        self.assertTrue('foo' in one)
        self.assertTrue('bar' in one)

    def test_direct_miss(self):
        one = self._makeOne()
        self.assertEqual(one.direct('nonesuch'), None)

    def test_direct_hit_local_miss_bases(self):
        from zope.interface.interface import Attribute
        from zope.interface.interface import fromFunction
        def _bar():
            """DOCSTRING"""
        def _foo():
            """DOCSTRING"""
        BASE_ATTRS = {'foo': Attribute('Foo', ''),
                      'bar': fromFunction(_bar),
                     }
        DERIVED_ATTRS = {'foo': fromFunction(_foo),
                         'baz': Attribute('Baz', ''),
                        }
        base = self._makeOne('IBase', attrs=BASE_ATTRS)
        derived = self._makeOne('IDerived', bases=(base,), attrs=DERIVED_ATTRS)
        self.assertEqual(derived.direct('foo'), DERIVED_ATTRS['foo'])
        self.assertEqual(derived.direct('baz'), DERIVED_ATTRS['baz'])
        self.assertEqual(derived.direct('bar'), None)

    def test_queryDescriptionFor_miss(self):
        iface = self._makeOne()
        self.assertEqual(iface.queryDescriptionFor('nonesuch'), None)

    def test_queryDescriptionFor_hit(self):
        from zope.interface import Attribute
        ATTRS = {'attr': Attribute('Title', 'Description')}
        iface = self._makeOne(attrs=ATTRS)
        self.assertEqual(iface.queryDescriptionFor('attr'), ATTRS['attr'])

    def test_validateInvariants_pass(self):
        _called_with = []
        def _passable(*args, **kw):
            _called_with.append((args, kw))
            return True
        iface = self._makeOne()
        obj = object()
        iface.setTaggedValue('invariants', [_passable])
        self.assertEqual(iface.validateInvariants(obj), None)
        self.assertEqual(_called_with, [((obj,), {})])

    def test_validateInvariants_fail_wo_errors_passed(self):
        from zope.interface.exceptions import Invalid
        _passable_called_with = []
        def _passable(*args, **kw):
            _passable_called_with.append((args, kw))
            return True
        _fail_called_with = []
        def _fail(*args, **kw):
            _fail_called_with.append((args, kw))
            raise Invalid
        iface = self._makeOne()
        obj = object()
        iface.setTaggedValue('invariants', [_passable, _fail])
        self.assertRaises(Invalid, iface.validateInvariants, obj)
        self.assertEqual(_passable_called_with, [((obj,), {})])
        self.assertEqual(_fail_called_with, [((obj,), {})])

    def test_validateInvariants_fail_w_errors_passed(self):
        from zope.interface.exceptions import Invalid
        _errors = []
        _fail_called_with = []
        def _fail(*args, **kw):
            _fail_called_with.append((args, kw))
            raise Invalid
        iface = self._makeOne()
        obj = object()
        iface.setTaggedValue('invariants', [_fail])
        self.assertRaises(Invalid, iface.validateInvariants, obj, _errors)
        self.assertEqual(_fail_called_with, [((obj,), {})])
        self.assertEqual(len(_errors), 1)
        self.assertTrue(isinstance(_errors[0], Invalid))

    def test_validateInvariants_fail_in_base_wo_errors_passed(self):
        from zope.interface.exceptions import Invalid
        _passable_called_with = []
        def _passable(*args, **kw):
            _passable_called_with.append((args, kw))
            return True
        _fail_called_with = []
        def _fail(*args, **kw):
            _fail_called_with.append((args, kw))
            raise Invalid
        base = self._makeOne('IBase')
        derived = self._makeOne('IDerived', (base,))
        obj = object()
        base.setTaggedValue('invariants', [_fail])
        derived.setTaggedValue('invariants', [_passable])
        self.assertRaises(Invalid, derived.validateInvariants, obj)
        self.assertEqual(_passable_called_with, [((obj,), {})])
        self.assertEqual(_fail_called_with, [((obj,), {})])

    def test_validateInvariants_fail_in_base_w_errors_passed(self):
        from zope.interface.exceptions import Invalid
        _errors = []
        _passable_called_with = []
        def _passable(*args, **kw):
            _passable_called_with.append((args, kw))
            return True
        _fail_called_with = []
        def _fail(*args, **kw):
            _fail_called_with.append((args, kw))
            raise Invalid
        base = self._makeOne('IBase')
        derived = self._makeOne('IDerived', (base,))
        obj = object()
        base.setTaggedValue('invariants', [_fail])
        derived.setTaggedValue('invariants', [_passable])
        self.assertRaises(Invalid, derived.validateInvariants, obj, _errors)
        self.assertEqual(_passable_called_with, [((obj,), {})])
        self.assertEqual(_fail_called_with, [((obj,), {})])
        self.assertEqual(len(_errors), 1)
        self.assertTrue(isinstance(_errors[0], Invalid))

    def test_validateInvariants_inherited_not_called_multiple_times(self):
        _passable_called_with = []

        def _passable(*args, **kw):
            _passable_called_with.append((args, kw))
            return True

        obj = object()
        base = self._makeOne('IBase')
        base.setTaggedValue('invariants', [_passable])
        derived = self._makeOne('IDerived', (base,))
        derived.validateInvariants(obj)
        self.assertEqual(1, len(_passable_called_with))

    def test___reduce__(self):
        iface = self._makeOne('PickleMe')
        self.assertEqual(iface.__reduce__(), 'PickleMe')

    def test___hash___normal(self):
        iface = self._makeOne('HashMe')
        self.assertEqual(hash(iface),
                         hash((('HashMe',
                                'zope.interface.tests.test_interface'))))

    def test___hash___missing_required_attrs(self):
        class Derived(self._getTargetClass()):
            def __init__(self): # pylint:disable=super-init-not-called
                pass # Don't call base class.
        derived = Derived()
        with self.assertRaises(AttributeError):
            hash(derived)

    def test_comparison_with_None(self):
        # pylint:disable=singleton-comparison,misplaced-comparison-constant
        iface = self._makeOne()
        self.assertTrue(iface < None)
        self.assertTrue(iface <= None)
        self.assertFalse(iface == None)
        self.assertTrue(iface != None)
        self.assertFalse(iface >= None)
        self.assertFalse(iface > None)

        self.assertFalse(None < iface)
        self.assertFalse(None <= iface)
        self.assertFalse(None == iface)
        self.assertTrue(None != iface)
        self.assertTrue(None >= iface)
        self.assertTrue(None > iface)

    def test_comparison_with_same_instance(self):
        # pylint:disable=comparison-with-itself
        iface = self._makeOne()

        self.assertFalse(iface < iface)
        self.assertTrue(iface <= iface)
        self.assertTrue(iface == iface)
        self.assertFalse(iface != iface)
        self.assertTrue(iface >= iface)
        self.assertFalse(iface > iface)

    def test_comparison_with_same_named_instance_in_other_module(self):

        one = self._makeOne('IName', __module__='zope.interface.tests.one')
        other = self._makeOne('IName', __module__='zope.interface.tests.other')

        self.assertTrue(one < other)
        self.assertFalse(other < one)
        self.assertTrue(one <= other)
        self.assertFalse(other <= one)
        self.assertFalse(one == other)
        self.assertFalse(other == one)
        self.assertTrue(one != other)
        self.assertTrue(other != one)
        self.assertFalse(one >= other)
        self.assertTrue(other >= one)
        self.assertFalse(one > other)
        self.assertTrue(other > one)

    def test_assignment_to__class__(self):
        # https://github.com/zopefoundation/zope.interface/issues/6
        class MyException(Exception):
            pass

        class MyInterfaceClass(self._getTargetClass()):
            def __call__(self, target):
                raise MyException(target)

        IFoo = self._makeOne('IName')
        self.assertIsInstance(IFoo, self._getTargetClass())
        self.assertIs(type(IFoo), self._getTargetClass())

        with self.assertRaises(TypeError):
            IFoo(1)

        IFoo.__class__ = MyInterfaceClass
        self.assertIsInstance(IFoo, MyInterfaceClass)
        self.assertIs(type(IFoo), MyInterfaceClass)

        with self.assertRaises(MyException):
            IFoo(1)

    def test_assignment_to__class__2(self):
        # https://github.com/zopefoundation/zope.interface/issues/6
        # This is essentially a transcription of the
        # test presented in the bug report.
        from zope.interface import Interface
        class MyInterfaceClass(self._getTargetClass()):
            def __call__(self, *args):
                return args

        IFoo = MyInterfaceClass('IFoo', (Interface,))
        self.assertEqual(IFoo(1), (1,))

        class IBar(IFoo):
            pass

        self.assertEqual(IBar(1), (1,))

        class ISpam(Interface):
            pass

        with self.assertRaises(TypeError):
            ISpam()

        ISpam.__class__ = MyInterfaceClass
        self.assertEqual(ISpam(1), (1,))

    def test__module__is_readonly(self):
        inst = self._makeOne()
        with self.assertRaises((AttributeError, TypeError)):
            # CPython 2.7 raises TypeError. Everything else
            # raises AttributeError.
            inst.__module__ = 'different.module'


class InterfaceTests(unittest.TestCase):

    def test_attributes_link_to_interface(self):
        from zope.interface import Interface
        from zope.interface import Attribute

        class I1(Interface):
            attr = Attribute("My attr")

        self.assertTrue(I1['attr'].interface is I1)

    def test_methods_link_to_interface(self):
        from zope.interface import Interface

        class I1(Interface):

            def method(foo, bar, bingo):
                "A method"

        self.assertTrue(I1['method'].interface is I1)

    def test_classImplements_simple(self):
        from zope.interface import Interface
        from zope.interface import implementedBy
        from zope.interface import providedBy

        class ICurrent(Interface):
            def method1(a, b):
                pass
            def method2(a, b):
                pass

        class IOther(Interface):
            pass

        class Current(object):
            __implemented__ = ICurrent
            def method1(self, a, b):
                raise NotImplementedError()
            def method2(self, a, b):
                raise NotImplementedError()

        current = Current()

        self.assertTrue(ICurrent.implementedBy(Current))
        self.assertFalse(IOther.implementedBy(Current))
        self.assertEqual(ICurrent, ICurrent)
        self.assertTrue(ICurrent in implementedBy(Current))
        self.assertFalse(IOther in implementedBy(Current))
        self.assertTrue(ICurrent in providedBy(current))
        self.assertFalse(IOther in providedBy(current))

    def test_classImplements_base_not_derived(self):
        from zope.interface import Interface
        from zope.interface import implementedBy
        from zope.interface import providedBy
        class IBase(Interface):
            def method():
                pass
        class IDerived(IBase):
            pass
        class Current():
            __implemented__ = IBase
            def method(self):
                raise NotImplementedError()
        current = Current()

        self.assertTrue(IBase.implementedBy(Current))
        self.assertFalse(IDerived.implementedBy(Current))
        self.assertTrue(IBase in implementedBy(Current))
        self.assertFalse(IDerived in implementedBy(Current))
        self.assertTrue(IBase in providedBy(current))
        self.assertFalse(IDerived in providedBy(current))

    def test_classImplements_base_and_derived(self):
        from zope.interface import Interface
        from zope.interface import implementedBy
        from zope.interface import providedBy

        class IBase(Interface):
            def method():
                pass

        class IDerived(IBase):
            pass

        class Current(object):
            __implemented__ = IDerived
            def method(self):
                raise NotImplementedError()

        current = Current()

        self.assertTrue(IBase.implementedBy(Current))
        self.assertTrue(IDerived.implementedBy(Current))
        self.assertFalse(IBase in implementedBy(Current))
        self.assertTrue(IBase in implementedBy(Current).flattened())
        self.assertTrue(IDerived in implementedBy(Current))
        self.assertFalse(IBase in providedBy(current))
        self.assertTrue(IBase in providedBy(current).flattened())
        self.assertTrue(IDerived in providedBy(current))

    def test_classImplements_multiple(self):
        from zope.interface import Interface
        from zope.interface import implementedBy
        from zope.interface import providedBy

        class ILeft(Interface):
            def method():
                pass

        class IRight(ILeft):
            pass

        class Left(object):
            __implemented__ = ILeft

            def method(self):
                raise NotImplementedError()

        class Right(object):
            __implemented__ = IRight

        class Ambi(Left, Right):
            pass

        ambi = Ambi()

        self.assertTrue(ILeft.implementedBy(Ambi))
        self.assertTrue(IRight.implementedBy(Ambi))
        self.assertTrue(ILeft in implementedBy(Ambi))
        self.assertTrue(IRight in implementedBy(Ambi))
        self.assertTrue(ILeft in providedBy(ambi))
        self.assertTrue(IRight in providedBy(ambi))

    def test_classImplements_multiple_w_explict_implements(self):
        from zope.interface import Interface
        from zope.interface import implementedBy
        from zope.interface import providedBy

        class ILeft(Interface):

            def method():
                pass

        class IRight(ILeft):
            pass

        class IOther(Interface):
            pass

        class Left():
            __implemented__ = ILeft

            def method(self):
                raise NotImplementedError()

        class Right(object):
            __implemented__ = IRight

        class Other(object):
            __implemented__ = IOther

        class Mixed(Left, Right):
            __implemented__ = Left.__implemented__, Other.__implemented__

        mixed = Mixed()

        self.assertTrue(ILeft.implementedBy(Mixed))
        self.assertFalse(IRight.implementedBy(Mixed))
        self.assertTrue(IOther.implementedBy(Mixed))
        self.assertTrue(ILeft in implementedBy(Mixed))
        self.assertFalse(IRight in implementedBy(Mixed))
        self.assertTrue(IOther in implementedBy(Mixed))
        self.assertTrue(ILeft in providedBy(mixed))
        self.assertFalse(IRight in providedBy(mixed))
        self.assertTrue(IOther in providedBy(mixed))

    def testInterfaceExtendsInterface(self):
        from zope.interface import Interface

        new = Interface.__class__
        FunInterface = new('FunInterface')
        BarInterface = new('BarInterface', (FunInterface,))
        BobInterface = new('BobInterface')
        BazInterface = new('BazInterface', (BobInterface, BarInterface,))

        self.assertTrue(BazInterface.extends(BobInterface))
        self.assertTrue(BazInterface.extends(BarInterface))
        self.assertTrue(BazInterface.extends(FunInterface))
        self.assertFalse(BobInterface.extends(FunInterface))
        self.assertFalse(BobInterface.extends(BarInterface))
        self.assertTrue(BarInterface.extends(FunInterface))
        self.assertFalse(BarInterface.extends(BazInterface))

    def test_verifyClass(self):
        from zope.interface import Attribute
        from zope.interface import Interface
        from zope.interface.verify import verifyClass


        class ICheckMe(Interface):
            attr = Attribute(u'My attr')

            def method():
                "A method"

        class CheckMe(object):
            __implemented__ = ICheckMe
            attr = 'value'

            def method(self):
                raise NotImplementedError()

        self.assertTrue(verifyClass(ICheckMe, CheckMe))

    def test_verifyObject(self):
        from zope.interface import Attribute
        from zope.interface import Interface
        from zope.interface.verify import verifyObject


        class ICheckMe(Interface):
            attr = Attribute(u'My attr')

            def method():
                "A method"

        class CheckMe(object):
            __implemented__ = ICheckMe
            attr = 'value'

            def method(self):
                raise NotImplementedError()

        check_me = CheckMe()

        self.assertTrue(verifyObject(ICheckMe, check_me))

    def test_interface_object_provides_Interface(self):
        from zope.interface import Interface

        class AnInterface(Interface):
            pass

        self.assertTrue(Interface.providedBy(AnInterface))

    def test_names_simple(self):
        from zope.interface import Attribute
        from zope.interface import Interface


        class ISimple(Interface):
            attr = Attribute(u'My attr')

            def method():
                pass

        self.assertEqual(sorted(ISimple.names()), ['attr', 'method'])

    def test_names_derived(self):
        from zope.interface import Attribute
        from zope.interface import Interface


        class IBase(Interface):
            attr = Attribute(u'My attr')

            def method():
                pass

        class IDerived(IBase):
            attr2 = Attribute(u'My attr2')

            def method():
                pass

            def method2():
                pass

        self.assertEqual(sorted(IDerived.names()),
                         ['attr2', 'method', 'method2'])
        self.assertEqual(sorted(IDerived.names(all=True)),
                         ['attr', 'attr2', 'method', 'method2'])

    def test_namesAndDescriptions_simple(self):
        from zope.interface import Attribute
        from zope.interface.interface import Method
        from zope.interface import Interface


        class ISimple(Interface):
            attr = Attribute(u'My attr')

            def method():
                "My method"

        name_values = sorted(ISimple.namesAndDescriptions())

        self.assertEqual(len(name_values), 2)
        self.assertEqual(name_values[0][0], 'attr')
        self.assertTrue(isinstance(name_values[0][1], Attribute))
        self.assertEqual(name_values[0][1].__name__, 'attr')
        self.assertEqual(name_values[0][1].__doc__, 'My attr')
        self.assertEqual(name_values[1][0], 'method')
        self.assertTrue(isinstance(name_values[1][1], Method))
        self.assertEqual(name_values[1][1].__name__, 'method')
        self.assertEqual(name_values[1][1].__doc__, 'My method')

    def test_namesAndDescriptions_derived(self):
        from zope.interface import Attribute
        from zope.interface import Interface
        from zope.interface.interface import Method


        class IBase(Interface):
            attr = Attribute(u'My attr')

            def method():
                "My method"

        class IDerived(IBase):
            attr2 = Attribute(u'My attr2')

            def method():
                "My method, overridden"

            def method2():
                "My method2"

        name_values = sorted(IDerived.namesAndDescriptions())

        self.assertEqual(len(name_values), 3)
        self.assertEqual(name_values[0][0], 'attr2')
        self.assertTrue(isinstance(name_values[0][1], Attribute))
        self.assertEqual(name_values[0][1].__name__, 'attr2')
        self.assertEqual(name_values[0][1].__doc__, 'My attr2')
        self.assertEqual(name_values[1][0], 'method')
        self.assertTrue(isinstance(name_values[1][1], Method))
        self.assertEqual(name_values[1][1].__name__, 'method')
        self.assertEqual(name_values[1][1].__doc__, 'My method, overridden')
        self.assertEqual(name_values[2][0], 'method2')
        self.assertTrue(isinstance(name_values[2][1], Method))
        self.assertEqual(name_values[2][1].__name__, 'method2')
        self.assertEqual(name_values[2][1].__doc__, 'My method2')

        name_values = sorted(IDerived.namesAndDescriptions(all=True))

        self.assertEqual(len(name_values), 4)
        self.assertEqual(name_values[0][0], 'attr')
        self.assertTrue(isinstance(name_values[0][1], Attribute))
        self.assertEqual(name_values[0][1].__name__, 'attr')
        self.assertEqual(name_values[0][1].__doc__, 'My attr')
        self.assertEqual(name_values[1][0], 'attr2')
        self.assertTrue(isinstance(name_values[1][1], Attribute))
        self.assertEqual(name_values[1][1].__name__, 'attr2')
        self.assertEqual(name_values[1][1].__doc__, 'My attr2')
        self.assertEqual(name_values[2][0], 'method')
        self.assertTrue(isinstance(name_values[2][1], Method))
        self.assertEqual(name_values[2][1].__name__, 'method')
        self.assertEqual(name_values[2][1].__doc__, 'My method, overridden')
        self.assertEqual(name_values[3][0], 'method2')
        self.assertTrue(isinstance(name_values[3][1], Method))
        self.assertEqual(name_values[3][1].__name__, 'method2')
        self.assertEqual(name_values[3][1].__doc__, 'My method2')

    def test_getDescriptionFor_nonesuch_no_default(self):
        from zope.interface import Interface

        class IEmpty(Interface):
            pass

        self.assertRaises(KeyError, IEmpty.getDescriptionFor, 'nonesuch')

    def test_getDescriptionFor_simple(self):
        from zope.interface import Attribute
        from zope.interface.interface import Method
        from zope.interface import Interface


        class ISimple(Interface):
            attr = Attribute(u'My attr')

            def method():
                "My method"

        a_desc = ISimple.getDescriptionFor('attr')
        self.assertTrue(isinstance(a_desc, Attribute))
        self.assertEqual(a_desc.__name__, 'attr')
        self.assertEqual(a_desc.__doc__, 'My attr')

        m_desc = ISimple.getDescriptionFor('method')
        self.assertTrue(isinstance(m_desc, Method))
        self.assertEqual(m_desc.__name__, 'method')
        self.assertEqual(m_desc.__doc__, 'My method')

    def test_getDescriptionFor_derived(self):
        from zope.interface import Attribute
        from zope.interface.interface import Method
        from zope.interface import Interface


        class IBase(Interface):
            attr = Attribute(u'My attr')

            def method():
                "My method"

        class IDerived(IBase):
            attr2 = Attribute(u'My attr2')

            def method():
                "My method, overridden"

            def method2():
                "My method2"

        a_desc = IDerived.getDescriptionFor('attr')
        self.assertTrue(isinstance(a_desc, Attribute))
        self.assertEqual(a_desc.__name__, 'attr')
        self.assertEqual(a_desc.__doc__, 'My attr')

        m_desc = IDerived.getDescriptionFor('method')
        self.assertTrue(isinstance(m_desc, Method))
        self.assertEqual(m_desc.__name__, 'method')
        self.assertEqual(m_desc.__doc__, 'My method, overridden')

        a2_desc = IDerived.getDescriptionFor('attr2')
        self.assertTrue(isinstance(a2_desc, Attribute))
        self.assertEqual(a2_desc.__name__, 'attr2')
        self.assertEqual(a2_desc.__doc__, 'My attr2')

        m2_desc = IDerived.getDescriptionFor('method2')
        self.assertTrue(isinstance(m2_desc, Method))
        self.assertEqual(m2_desc.__name__, 'method2')
        self.assertEqual(m2_desc.__doc__, 'My method2')

    def test___getitem__nonesuch(self):
        from zope.interface import Interface

        class IEmpty(Interface):
            pass

        self.assertRaises(KeyError, IEmpty.__getitem__, 'nonesuch')

    def test___getitem__simple(self):
        from zope.interface import Attribute
        from zope.interface.interface import Method
        from zope.interface import Interface


        class ISimple(Interface):
            attr = Attribute(u'My attr')

            def method():
                "My method"

        a_desc = ISimple['attr']
        self.assertTrue(isinstance(a_desc, Attribute))
        self.assertEqual(a_desc.__name__, 'attr')
        self.assertEqual(a_desc.__doc__, 'My attr')

        m_desc = ISimple['method']
        self.assertTrue(isinstance(m_desc, Method))
        self.assertEqual(m_desc.__name__, 'method')
        self.assertEqual(m_desc.__doc__, 'My method')

    def test___getitem___derived(self):
        from zope.interface import Attribute
        from zope.interface.interface import Method
        from zope.interface import Interface


        class IBase(Interface):
            attr = Attribute(u'My attr')

            def method():
                "My method"

        class IDerived(IBase):
            attr2 = Attribute(u'My attr2')

            def method():
                "My method, overridden"

            def method2():
                "My method2"

        a_desc = IDerived['attr']
        self.assertTrue(isinstance(a_desc, Attribute))
        self.assertEqual(a_desc.__name__, 'attr')
        self.assertEqual(a_desc.__doc__, 'My attr')

        m_desc = IDerived['method']
        self.assertTrue(isinstance(m_desc, Method))
        self.assertEqual(m_desc.__name__, 'method')
        self.assertEqual(m_desc.__doc__, 'My method, overridden')

        a2_desc = IDerived['attr2']
        self.assertTrue(isinstance(a2_desc, Attribute))
        self.assertEqual(a2_desc.__name__, 'attr2')
        self.assertEqual(a2_desc.__doc__, 'My attr2')

        m2_desc = IDerived['method2']
        self.assertTrue(isinstance(m2_desc, Method))
        self.assertEqual(m2_desc.__name__, 'method2')
        self.assertEqual(m2_desc.__doc__, 'My method2')

    def test___contains__nonesuch(self):
        from zope.interface import Interface

        class IEmpty(Interface):
            pass

        self.assertFalse('nonesuch' in IEmpty)

    def test___contains__simple(self):
        from zope.interface import Attribute
        from zope.interface import Interface


        class ISimple(Interface):
            attr = Attribute(u'My attr')

            def method():
                "My method"

        self.assertTrue('attr' in ISimple)
        self.assertTrue('method' in ISimple)

    def test___contains__derived(self):
        from zope.interface import Attribute
        from zope.interface import Interface


        class IBase(Interface):
            attr = Attribute(u'My attr')

            def method():
                "My method"

        class IDerived(IBase):
            attr2 = Attribute(u'My attr2')

            def method():
                "My method, overridden"

            def method2():
                "My method2"

        self.assertTrue('attr' in IDerived)
        self.assertTrue('method' in IDerived)
        self.assertTrue('attr2' in IDerived)
        self.assertTrue('method2' in IDerived)

    def test___iter__empty(self):
        from zope.interface import Interface

        class IEmpty(Interface):
            pass

        self.assertEqual(list(IEmpty), [])

    def test___iter__simple(self):
        from zope.interface import Attribute
        from zope.interface import Interface


        class ISimple(Interface):
            attr = Attribute(u'My attr')

            def method():
                "My method"

        self.assertEqual(sorted(list(ISimple)), ['attr', 'method'])

    def test___iter__derived(self):
        from zope.interface import Attribute
        from zope.interface import Interface


        class IBase(Interface):
            attr = Attribute(u'My attr')

            def method():
                "My method"

        class IDerived(IBase):
            attr2 = Attribute(u'My attr2')

            def method():
                "My method, overridden"

            def method2():
                "My method2"

        self.assertEqual(sorted(list(IDerived)),
                         ['attr', 'attr2', 'method', 'method2'])

    def test_function_attributes_become_tagged_values(self):
        from zope.interface import Interface

        class ITagMe(Interface):
            def method():
                pass
            method.optional = 1

        method = ITagMe['method']
        self.assertEqual(method.getTaggedValue('optional'), 1)

    def test___doc___non_element(self):
        from zope.interface import Interface

        class IHaveADocString(Interface):
            "xxx"

        self.assertEqual(IHaveADocString.__doc__, "xxx")
        self.assertEqual(list(IHaveADocString), [])

    def test___doc___as_element(self):
        from zope.interface import Attribute
        from zope.interface import Interface

        class IHaveADocString(Interface):
            "xxx"
            __doc__ = Attribute('the doc')

        self.assertEqual(IHaveADocString.__doc__, "")
        self.assertEqual(list(IHaveADocString), ['__doc__'])

    def _errorsEqual(self, has_invariant, error_len, error_msgs, iface):
        from zope.interface.exceptions import Invalid
        self.assertRaises(Invalid, iface.validateInvariants, has_invariant)
        e = []
        try:
            iface.validateInvariants(has_invariant, e)
            self.fail("validateInvariants should always raise")
        except Invalid as error:
            self.assertEqual(error.args[0], e)

        self.assertEqual(len(e), error_len)
        msgs = [error.args[0] for error in e]
        msgs.sort()
        for msg in msgs:
            self.assertEqual(msg, error_msgs.pop(0))

    def test_invariant_simple(self):
        from zope.interface import Attribute
        from zope.interface import Interface
        from zope.interface import directlyProvides
        from zope.interface import invariant

        class IInvariant(Interface):
            foo = Attribute('foo')
            bar = Attribute('bar; must eval to Boolean True if foo does')
            invariant(_ifFooThenBar)

        class HasInvariant(object):
            pass

        # set up
        has_invariant = HasInvariant()
        directlyProvides(has_invariant, IInvariant)

        # the tests
        self.assertEqual(IInvariant.getTaggedValue('invariants'),
                         [_ifFooThenBar])
        self.assertEqual(IInvariant.validateInvariants(has_invariant), None)
        has_invariant.bar = 27
        self.assertEqual(IInvariant.validateInvariants(has_invariant), None)
        has_invariant.foo = 42
        self.assertEqual(IInvariant.validateInvariants(has_invariant), None)
        del has_invariant.bar
        self._errorsEqual(has_invariant, 1, ['If Foo, then Bar!'],
                          IInvariant)

    def test_invariant_nested(self):
        from zope.interface import Attribute
        from zope.interface import Interface
        from zope.interface import directlyProvides
        from zope.interface import invariant

        class IInvariant(Interface):
            foo = Attribute('foo')
            bar = Attribute('bar; must eval to Boolean True if foo does')
            invariant(_ifFooThenBar)

        class ISubInvariant(IInvariant):
            invariant(_barGreaterThanFoo)

        class HasInvariant(object):
            pass

        # nested interfaces with invariants:
        self.assertEqual(ISubInvariant.getTaggedValue('invariants'),
                         [_barGreaterThanFoo])
        has_invariant = HasInvariant()
        directlyProvides(has_invariant, ISubInvariant)
        has_invariant.foo = 42
        # even though the interface has changed, we should still only have one
        # error.
        self._errorsEqual(has_invariant, 1, ['If Foo, then Bar!'],
                          ISubInvariant)
        # however, if we set foo to 0 (Boolean False) and bar to a negative
        # number then we'll get the new error
        has_invariant.foo = 2
        has_invariant.bar = 1
        self._errorsEqual(has_invariant, 1,
                          ['Please, Boo MUST be greater than Foo!'],
                          ISubInvariant)
        # and if we set foo to a positive number and boo to 0, we'll
        # get both errors!
        has_invariant.foo = 1
        has_invariant.bar = 0
        self._errorsEqual(has_invariant, 2,
                          ['If Foo, then Bar!',
                           'Please, Boo MUST be greater than Foo!'],
                          ISubInvariant)
        # for a happy ending, we'll make the invariants happy
        has_invariant.foo = 1
        has_invariant.bar = 2
        self.assertEqual(IInvariant.validateInvariants(has_invariant), None)

    def test_invariant_mutandis(self):
        from zope.interface import Attribute
        from zope.interface import Interface
        from zope.interface import directlyProvides
        from zope.interface import invariant

        class IInvariant(Interface):
            foo = Attribute('foo')
            bar = Attribute('bar; must eval to Boolean True if foo does')
            invariant(_ifFooThenBar)

        class HasInvariant(object):
            pass

        # now we'll do two invariants on the same interface,
        # just to make sure that a small
        # multi-invariant interface is at least minimally tested.
        has_invariant = HasInvariant()
        directlyProvides(has_invariant, IInvariant)
        has_invariant.foo = 42

        # if you really need to mutate, then this would be the way to do it.
        # Probably a bad idea, though. :-)
        old_invariants = IInvariant.getTaggedValue('invariants')
        invariants = old_invariants[:]
        invariants.append(_barGreaterThanFoo)
        IInvariant.setTaggedValue('invariants', invariants)

        # even though the interface has changed, we should still only have one
        # error.
        self._errorsEqual(has_invariant, 1, ['If Foo, then Bar!'],
                          IInvariant)
        # however, if we set foo to 0 (Boolean False) and bar to a negative
        # number then we'll get the new error
        has_invariant.foo = 2
        has_invariant.bar = 1
        self._errorsEqual(has_invariant, 1,
                          ['Please, Boo MUST be greater than Foo!'], IInvariant)
        # and if we set foo to a positive number and boo to 0, we'll
        # get both errors!
        has_invariant.foo = 1
        has_invariant.bar = 0
        self._errorsEqual(has_invariant, 2,
                          ['If Foo, then Bar!',
                           'Please, Boo MUST be greater than Foo!'],
                          IInvariant)
        # for another happy ending, we'll make the invariants happy again
        has_invariant.foo = 1
        has_invariant.bar = 2
        self.assertEqual(IInvariant.validateInvariants(has_invariant), None)
        # clean up
        IInvariant.setTaggedValue('invariants', old_invariants)

    def test___doc___element(self):
        from zope.interface import Interface
        from zope.interface import Attribute
        class IDocstring(Interface):
            "xxx"

        self.assertEqual(IDocstring.__doc__, "xxx")
        self.assertEqual(list(IDocstring), [])

        class IDocstringAndAttribute(Interface):
            "xxx"

            __doc__ = Attribute('the doc')

        self.assertEqual(IDocstringAndAttribute.__doc__, "")
        self.assertEqual(list(IDocstringAndAttribute), ['__doc__'])

    @_skip_under_py3k
    def testIssue228(self):
        # Test for http://collector.zope.org/Zope3-dev/228
        # Old style classes don't have a '__class__' attribute
        # No old style classes in Python 3, so the test becomes moot.
        from zope.interface import Interface

        class I(Interface):
            "xxx"

        class OldStyle:
            __providedBy__ = None

        self.assertRaises(AttributeError, I.providedBy, OldStyle)

    def test_invariant_as_decorator(self):
        from zope.interface import Interface
        from zope.interface import Attribute
        from zope.interface import implementer
        from zope.interface import invariant
        from zope.interface.exceptions import Invalid

        class IRange(Interface):
            min = Attribute("Lower bound")
            max = Attribute("Upper bound")

            @invariant
            def range_invariant(ob):
                if ob.max < ob.min:
                    raise Invalid('max < min')

        @implementer(IRange)
        class Range(object):

            def __init__(self, min, max):
                self.min, self.max = min, max

        IRange.validateInvariants(Range(1, 2))
        IRange.validateInvariants(Range(1, 1))
        try:
            IRange.validateInvariants(Range(2, 1))
        except Invalid as e:
            self.assertEqual(str(e), 'max < min')

    def test_taggedValue(self):
        from zope.interface import Attribute
        from zope.interface import Interface
        from zope.interface import taggedValue

        class ITagged(Interface):
            foo = Attribute('foo')
            bar = Attribute('bar; must eval to Boolean True if foo does')
            taggedValue('qux', 'Spam')

        class IDerived(ITagged):
            taggedValue('qux', 'Spam Spam')
            taggedValue('foo', 'bar')

        class IDerived2(IDerived):
            pass

        self.assertEqual(ITagged.getTaggedValue('qux'), 'Spam')
        self.assertRaises(KeyError, ITagged.getTaggedValue, 'foo')
        self.assertEqual(list(ITagged.getTaggedValueTags()), ['qux'])

        self.assertEqual(IDerived2.getTaggedValue('qux'), 'Spam Spam')
        self.assertEqual(IDerived2.getTaggedValue('foo'), 'bar')
        self.assertEqual(set(IDerived2.getTaggedValueTags()), set(['qux', 'foo']))

    def _make_taggedValue_tree(self, base):
        from zope.interface import taggedValue
        from zope.interface import Attribute
        O = base
        class F(O):
            taggedValue('tag', 'F')
            tag = Attribute('F')
        class E(O):
            taggedValue('tag', 'E')
            tag = Attribute('E')
        class D(O):
            taggedValue('tag', 'D')
            tag = Attribute('D')
        class C(D, F):
            taggedValue('tag', 'C')
            tag = Attribute('C')
        class B(D, E):
            pass
        class A(B, C):
            pass

        return A

    def test_getTaggedValue_follows__iro__(self):
        # And not just looks at __bases__.
        # https://github.com/zopefoundation/zope.interface/issues/190
        from zope.interface import Interface

        # First, confirm that looking at a true class
        # hierarchy follows the __mro__.
        class_A = self._make_taggedValue_tree(object)
        self.assertEqual(class_A.tag.__name__, 'C')

        # Now check that Interface does, both for attributes...
        iface_A = self._make_taggedValue_tree(Interface)
        self.assertEqual(iface_A['tag'].__name__, 'C')
        # ... and for tagged values.
        self.assertEqual(iface_A.getTaggedValue('tag'), 'C')
        self.assertEqual(iface_A.queryTaggedValue('tag'), 'C')
        # Of course setting something lower overrides it.
        assert iface_A.__bases__[0].__name__ == 'B'
        iface_A.__bases__[0].setTaggedValue('tag', 'B')
        self.assertEqual(iface_A.getTaggedValue('tag'), 'B')

    def test_getDirectTaggedValue_ignores__iro__(self):
        # https://github.com/zopefoundation/zope.interface/issues/190
        from zope.interface import Interface

        A = self._make_taggedValue_tree(Interface)
        self.assertIsNone(A.queryDirectTaggedValue('tag'))
        self.assertEqual([], list(A.getDirectTaggedValueTags()))

        with self.assertRaises(KeyError):
            A.getDirectTaggedValue('tag')

        A.setTaggedValue('tag', 'A')
        self.assertEqual(A.queryDirectTaggedValue('tag'), 'A')
        self.assertEqual(A.getDirectTaggedValue('tag'), 'A')
        self.assertEqual(['tag'], list(A.getDirectTaggedValueTags()))

        assert A.__bases__[1].__name__ == 'C'
        C = A.__bases__[1]
        self.assertEqual(C.queryDirectTaggedValue('tag'), 'C')
        self.assertEqual(C.getDirectTaggedValue('tag'), 'C')
        self.assertEqual(['tag'], list(C.getDirectTaggedValueTags()))

    def test_description_cache_management(self):
        # See https://bugs.launchpad.net/zope.interface/+bug/185974
        # There was a bug where the cache used by Specification.get() was not
        # cleared when the bases were changed.
        from zope.interface import Interface
        from zope.interface import Attribute

        class I1(Interface):
            a = Attribute('a')

        class I2(I1):
            pass

        class I3(I2):
            pass

        self.assertTrue(I3.get('a') is I1.get('a'))

        I2.__bases__ = (Interface,)
        self.assertTrue(I3.get('a') is None)

    def test___call___defers_to___conform___(self):
        from zope.interface import Interface
        from zope.interface import implementer

        class I(Interface):
            pass

        @implementer(I)
        class C(object):
            def __conform__(self, proto):
                return 0

        self.assertEqual(I(C()), 0)

    def test___call___object_implements(self):
        from zope.interface import Interface
        from zope.interface import implementer

        class I(Interface):
            pass

        @implementer(I)
        class C(object):
            pass

        c = C()
        self.assertTrue(I(c) is c)

    def test___call___miss_wo_alternate(self):
        from zope.interface import Interface

        class I(Interface):
            pass

        class C(object):
            pass

        c = C()
        self.assertRaises(TypeError, I, c)

    def test___call___miss_w_alternate(self):
        from zope.interface import Interface

        class I(Interface):
            pass

        class C(object):
            pass

        c = C()
        self.assertTrue(I(c, self) is self)

    def test___call___w_adapter_hook(self):
        from zope.interface import Interface
        from zope.interface.interface import adapter_hooks

        def _miss(iface, obj):
            pass

        def _hit(iface, obj):
            return self

        class I(Interface):
            pass

        class C(object):
            pass

        c = C()

        old_adapter_hooks = adapter_hooks[:]
        adapter_hooks[:] = [_miss, _hit]
        try:
            self.assertTrue(I(c) is self)
        finally:
            adapter_hooks[:] = old_adapter_hooks

    def test___call___w_overridden_adapt(self):
        from zope.interface import Interface
        from zope.interface import interfacemethod
        from zope.interface import implementer

        class I(Interface):

            @interfacemethod
            def __adapt__(self, obj):
                return 42

        @implementer(I)
        class O(object):
            pass

        self.assertEqual(42, I(object()))
        # __adapt__ can ignore the fact that the object provides
        # the interface if it chooses.
        self.assertEqual(42, I(O()))

    def test___call___w_overridden_adapt_and_conform(self):
        # Conform is first, taking precedence over __adapt__,
        # *if* it returns non-None
        from zope.interface import Interface
        from zope.interface import interfacemethod
        from zope.interface import implementer

        class IAdapt(Interface):
            @interfacemethod
            def __adapt__(self, obj):
                return 42

        class ISimple(Interface):
            """Nothing special."""

        @implementer(IAdapt)
        class Conform24(object):
            def __conform__(self, iface):
                return 24

        @implementer(IAdapt)
        class ConformNone(object):
            def __conform__(self, iface):
                return None

        self.assertEqual(42, IAdapt(object()))

        self.assertEqual(24, ISimple(Conform24()))
        self.assertEqual(24, IAdapt(Conform24()))

        with self.assertRaises(TypeError):
            ISimple(ConformNone())

        self.assertEqual(42, IAdapt(ConformNone()))


    def test___call___w_overridden_adapt_call_super(self):
        import sys
        from zope.interface import Interface
        from zope.interface import interfacemethod
        from zope.interface import implementer

        class I(Interface):

            @interfacemethod
            def __adapt__(self, obj):
                if not self.providedBy(obj):
                    return 42
                if sys.version_info[:2] > (3, 5):
                    # Python 3.5 raises 'RuntimeError: super() __class__ is not a type'
                    return super().__adapt__(obj)

                return super(type(I), self).__adapt__(obj)

        @implementer(I)
        class O(object):
            pass

        self.assertEqual(42, I(object()))
        o = O()
        self.assertIs(o, I(o))

    def test___adapt___as_method_and_implementation(self):
        from zope.interface import Interface
        from zope.interface import interfacemethod

        class I(Interface):
            @interfacemethod
            def __adapt__(self, obj):
                return 42

            def __adapt__(to_adapt):
                "This is a protocol"

        self.assertEqual(42, I(object()))
        self.assertEqual(I['__adapt__'].getSignatureString(), '(to_adapt)')

    def test___adapt__inheritance_and_type(self):
        from zope.interface import Interface
        from zope.interface import interfacemethod

        class IRoot(Interface):
            """Root"""

        class IWithAdapt(IRoot):
            @interfacemethod
            def __adapt__(self, obj):
                return 42

        class IOther(IRoot):
            """Second branch"""

        class IUnrelated(Interface):
            """Unrelated"""

        class IDerivedAdapt(IUnrelated, IWithAdapt, IOther):
            """Inherits an adapt"""
            # Order of "inheritance" matters here.

        class IDerived2Adapt(IDerivedAdapt):
            """Overrides an inherited custom adapt."""
            @interfacemethod
            def __adapt__(self, obj):
                return 24

        self.assertEqual(42, IDerivedAdapt(object()))
        for iface in IRoot, IWithAdapt, IOther, IUnrelated, IDerivedAdapt:
            self.assertEqual(__name__, iface.__module__)

        for iface in IRoot, IOther, IUnrelated:
            self.assertEqual(type(IRoot), type(Interface))

        # But things that implemented __adapt__ got a new type
        self.assertNotEqual(type(Interface), type(IWithAdapt))
        self.assertEqual(type(IWithAdapt), type(IDerivedAdapt))
        self.assertIsInstance(IWithAdapt, type(Interface))

        self.assertEqual(24, IDerived2Adapt(object()))
        self.assertNotEqual(type(IDerived2Adapt), type(IDerivedAdapt))
        self.assertIsInstance(IDerived2Adapt, type(IDerivedAdapt))

    def test_interfacemethod_is_general(self):
        from zope.interface import Interface
        from zope.interface import interfacemethod

        class I(Interface):

            @interfacemethod
            def __call__(self, obj):
                """Replace an existing method"""
                return 42

            @interfacemethod
            def this_is_new(self):
                return 42

        self.assertEqual(I(self), 42)
        self.assertEqual(I.this_is_new(), 42)


class AttributeTests(ElementTests):

    DEFAULT_NAME = 'TestAttribute'

    def _getTargetClass(self):
        from zope.interface.interface import Attribute
        return Attribute

    def test__repr__w_interface(self):
        method = self._makeOne()
        method.interface = type(self)
        r = repr(method)
        self.assertTrue(r.startswith('<zope.interface.interface.Attribute object at'), r)
        self.assertTrue(r.endswith(' ' + __name__ + '.AttributeTests.TestAttribute>'), r)

    def test__repr__wo_interface(self):
        method = self._makeOne()
        r = repr(method)
        self.assertTrue(r.startswith('<zope.interface.interface.Attribute object at'), r)
        self.assertTrue(r.endswith(' TestAttribute>'), r)

    def test__str__w_interface(self):
        method = self._makeOne()
        method.interface = type(self)
        r = str(method)
        self.assertEqual(r, __name__ + '.AttributeTests.TestAttribute')

    def test__str__wo_interface(self):
        method = self._makeOne()
        r = str(method)
        self.assertEqual(r, 'TestAttribute')


class MethodTests(AttributeTests):

    DEFAULT_NAME = 'TestMethod'

    def _getTargetClass(self):
        from zope.interface.interface import Method
        return Method

    def test_optional_as_property(self):
        method = self._makeOne()
        self.assertEqual(method.optional, {})
        method.optional = {'foo': 'bar'}
        self.assertEqual(method.optional, {'foo': 'bar'})
        del method.optional
        self.assertEqual(method.optional, {})

    def test___call___raises_BrokenImplementation(self):
        from zope.interface.exceptions import BrokenImplementation
        method = self._makeOne()
        try:
            method()
        except BrokenImplementation as e:
            self.assertEqual(e.interface, None)
            self.assertEqual(e.name, self.DEFAULT_NAME)
        else:
            self.fail('__call__ should raise BrokenImplementation')

    def test_getSignatureInfo_bare(self):
        method = self._makeOne()
        info = method.getSignatureInfo()
        self.assertEqual(list(info['positional']), [])
        self.assertEqual(list(info['required']), [])
        self.assertEqual(info['optional'], {})
        self.assertEqual(info['varargs'], None)
        self.assertEqual(info['kwargs'], None)

    def test_getSignatureString_bare(self):
        method = self._makeOne()
        self.assertEqual(method.getSignatureString(), '()')

    def test_getSignatureString_w_only_required(self):
        method = self._makeOne()
        method.positional = method.required = ['foo']
        self.assertEqual(method.getSignatureString(), '(foo)')

    def test_getSignatureString_w_optional(self):
        method = self._makeOne()
        method.positional = method.required = ['foo']
        method.optional = {'foo': 'bar'}
        self.assertEqual(method.getSignatureString(), "(foo='bar')")

    def test_getSignatureString_w_varargs(self):
        method = self._makeOne()
        method.varargs = 'args'
        self.assertEqual(method.getSignatureString(), "(*args)")

    def test_getSignatureString_w_kwargs(self):
        method = self._makeOne()
        method.kwargs = 'kw'
        self.assertEqual(method.getSignatureString(), "(**kw)")

    def test__repr__w_interface(self):
        method = self._makeOne()
        method.kwargs = 'kw'
        method.interface = type(self)
        r = repr(method)
        self.assertTrue(r.startswith('<zope.interface.interface.Method object at'), r)
        self.assertTrue(r.endswith(' ' + __name__ + '.MethodTests.TestMethod(**kw)>'), r)

    def test__repr__wo_interface(self):
        method = self._makeOne()
        method.kwargs = 'kw'
        r = repr(method)
        self.assertTrue(r.startswith('<zope.interface.interface.Method object at'), r)
        self.assertTrue(r.endswith(' TestMethod(**kw)>'), r)

    def test__str__w_interface(self):
        method = self._makeOne()
        method.kwargs = 'kw'
        method.interface = type(self)
        r = str(method)
        self.assertEqual(r, __name__ + '.MethodTests.TestMethod(**kw)')

    def test__str__wo_interface(self):
        method = self._makeOne()
        method.kwargs = 'kw'
        r = str(method)
        self.assertEqual(r, 'TestMethod(**kw)')


class Test_fromFunction(unittest.TestCase):

    def _callFUT(self, *args, **kw):
        from zope.interface.interface import fromFunction
        return fromFunction(*args, **kw)

    def test_bare(self):
        def _func():
            "DOCSTRING"
        method = self._callFUT(_func)
        self.assertEqual(method.getName(), '_func')
        self.assertEqual(method.getDoc(), 'DOCSTRING')
        self.assertEqual(method.interface, None)
        self.assertEqual(list(method.getTaggedValueTags()), [])
        info = method.getSignatureInfo()
        self.assertEqual(list(info['positional']), [])
        self.assertEqual(list(info['required']), [])
        self.assertEqual(info['optional'], {})
        self.assertEqual(info['varargs'], None)
        self.assertEqual(info['kwargs'], None)

    def test_w_interface(self):
        from zope.interface.interface import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        def _func():
            "DOCSTRING"
        method = self._callFUT(_func, interface=IFoo)
        self.assertEqual(method.interface, IFoo)

    def test_w_name(self):
        def _func():
            "DOCSTRING"
        method = self._callFUT(_func, name='anotherName')
        self.assertEqual(method.getName(), 'anotherName')

    def test_w_only_required(self):
        def _func(foo):
            "DOCSTRING"
        method = self._callFUT(_func)
        info = method.getSignatureInfo()
        self.assertEqual(list(info['positional']), ['foo'])
        self.assertEqual(list(info['required']), ['foo'])
        self.assertEqual(info['optional'], {})
        self.assertEqual(info['varargs'], None)
        self.assertEqual(info['kwargs'], None)

    def test_w_optional(self):
        def _func(foo='bar'):
            "DOCSTRING"
        method = self._callFUT(_func)
        info = method.getSignatureInfo()
        self.assertEqual(list(info['positional']), ['foo'])
        self.assertEqual(list(info['required']), [])
        self.assertEqual(info['optional'], {'foo': 'bar'})
        self.assertEqual(info['varargs'], None)
        self.assertEqual(info['kwargs'], None)

    def test_w_optional_self(self):
        # This is a weird case, trying to cover the following code in
        # FUT::
        #
        # nr = na-len(defaults)
        # if nr < 0:
        #     defaults=defaults[-nr:]
        #     nr = 0
        def _func(self='bar'):
            "DOCSTRING"
        method = self._callFUT(_func, imlevel=1)
        info = method.getSignatureInfo()
        self.assertEqual(list(info['positional']), [])
        self.assertEqual(list(info['required']), [])
        self.assertEqual(info['optional'], {})
        self.assertEqual(info['varargs'], None)
        self.assertEqual(info['kwargs'], None)

    def test_w_varargs(self):
        def _func(*args):
            "DOCSTRING"
        method = self._callFUT(_func)
        info = method.getSignatureInfo()
        self.assertEqual(list(info['positional']), [])
        self.assertEqual(list(info['required']), [])
        self.assertEqual(info['optional'], {})
        self.assertEqual(info['varargs'], 'args')
        self.assertEqual(info['kwargs'], None)

    def test_w_kwargs(self):
        def _func(**kw):
            "DOCSTRING"
        method = self._callFUT(_func)
        info = method.getSignatureInfo()
        self.assertEqual(list(info['positional']), [])
        self.assertEqual(list(info['required']), [])
        self.assertEqual(info['optional'], {})
        self.assertEqual(info['varargs'], None)
        self.assertEqual(info['kwargs'], 'kw')

    def test_full_spectrum(self):
        def _func(foo, bar='baz', *args, **kw): # pylint:disable=keyword-arg-before-vararg
            "DOCSTRING"
        method = self._callFUT(_func)
        info = method.getSignatureInfo()
        self.assertEqual(list(info['positional']), ['foo', 'bar'])
        self.assertEqual(list(info['required']), ['foo'])
        self.assertEqual(info['optional'], {'bar': 'baz'})
        self.assertEqual(info['varargs'], 'args')
        self.assertEqual(info['kwargs'], 'kw')


class Test_fromMethod(unittest.TestCase):

    def _callFUT(self, *args, **kw):
        from zope.interface.interface import fromMethod
        return fromMethod(*args, **kw)

    def test_no_args(self):
        class Foo(object):
            def bar(self):
                "DOCSTRING"
        method = self._callFUT(Foo.bar)
        self.assertEqual(method.getName(), 'bar')
        self.assertEqual(method.getDoc(), 'DOCSTRING')
        self.assertEqual(method.interface, None)
        self.assertEqual(list(method.getTaggedValueTags()), [])
        info = method.getSignatureInfo()
        self.assertEqual(list(info['positional']), [])
        self.assertEqual(list(info['required']), [])
        self.assertEqual(info['optional'], {})
        self.assertEqual(info['varargs'], None)
        self.assertEqual(info['kwargs'], None)

    def test_full_spectrum(self):
        class Foo(object):
            def bar(self, foo, bar='baz', *args, **kw): # pylint:disable=keyword-arg-before-vararg
                "DOCSTRING"
        method = self._callFUT(Foo.bar)
        info = method.getSignatureInfo()
        self.assertEqual(list(info['positional']), ['foo', 'bar'])
        self.assertEqual(list(info['required']), ['foo'])
        self.assertEqual(info['optional'], {'bar': 'baz'})
        self.assertEqual(info['varargs'], 'args')
        self.assertEqual(info['kwargs'], 'kw')

    def test_w_non_method(self):
        def foo():
            "DOCSTRING"
        method = self._callFUT(foo)
        self.assertEqual(method.getName(), 'foo')
        self.assertEqual(method.getDoc(), 'DOCSTRING')
        self.assertEqual(method.interface, None)
        self.assertEqual(list(method.getTaggedValueTags()), [])
        info = method.getSignatureInfo()
        self.assertEqual(list(info['positional']), [])
        self.assertEqual(list(info['required']), [])
        self.assertEqual(info['optional'], {})
        self.assertEqual(info['varargs'], None)
        self.assertEqual(info['kwargs'], None)

class DummyDependent(object):

    def __init__(self):
        self._changed = []

    def changed(self, originally_changed):
        self._changed.append(originally_changed)


def _barGreaterThanFoo(obj):
    from zope.interface.exceptions import Invalid
    foo = getattr(obj, 'foo', None)
    bar = getattr(obj, 'bar', None)
    if foo is not None and isinstance(foo, type(bar)):
        # type checking should be handled elsewhere (like, say,
        # schema); these invariants should be intra-interface
        # constraints.  This is a hacky way to do it, maybe, but you
        # get the idea
        if not bar > foo:
            raise Invalid('Please, Boo MUST be greater than Foo!')

def _ifFooThenBar(obj):
    from zope.interface.exceptions import Invalid
    if getattr(obj, 'foo', None) and not getattr(obj, 'bar', None):
        raise Invalid('If Foo, then Bar!')


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
