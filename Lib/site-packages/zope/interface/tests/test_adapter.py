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
"""Adapter registry tests
"""
import unittest

from zope.interface.tests import OptimizationTestMixin

# pylint:disable=inherit-non-class,protected-access,too-many-lines
# pylint:disable=attribute-defined-outside-init,blacklisted-name

def _makeInterfaces():
    from zope.interface import Interface

    class IB0(Interface):
        pass
    class IB1(IB0):
        pass
    class IB2(IB0):
        pass
    class IB3(IB2, IB1):
        pass
    class IB4(IB1, IB2):
        pass

    class IF0(Interface):
        pass
    class IF1(IF0):
        pass

    class IR0(Interface):
        pass
    class IR1(IR0):
        pass

    return IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1


class BaseAdapterRegistryTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.adapter import BaseAdapterRegistry
        class _CUT(BaseAdapterRegistry):
            class LookupClass(object):
                _changed = _extendors = ()
                def __init__(self, reg):
                    pass
                def changed(self, orig):
                    self._changed += (orig,)
                def add_extendor(self, provided):
                    self._extendors += (provided,)
                def remove_extendor(self, provided):
                    self._extendors = tuple([x for x in self._extendors
                                             if x != provided])
        for name in BaseAdapterRegistry._delegated:
            setattr(_CUT.LookupClass, name, object())
        return _CUT

    def _makeOne(self):
        return self._getTargetClass()()

    def test_lookup_delegation(self):
        CUT = self._getTargetClass()
        registry = CUT()
        for name in CUT._delegated:
            self.assertTrue(
                getattr(registry, name) is getattr(registry._v_lookup, name))

    def test__generation_on_first_creation(self):
        registry = self._makeOne()
        # Bumped to 1 in BaseAdapterRegistry.__init__
        self.assertEqual(registry._generation, 1)

    def test__generation_after_calling_changed(self):
        registry = self._makeOne()
        orig = object()
        registry.changed(orig)
        # Bumped to 1 in BaseAdapterRegistry.__init__
        self.assertEqual(registry._generation, 2)
        self.assertEqual(registry._v_lookup._changed, (registry, orig,))

    def test__generation_after_changing___bases__(self):
        class _Base(object):
            pass
        registry = self._makeOne()
        registry.__bases__ = (_Base,)
        self.assertEqual(registry._generation, 2)

    def test_register(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        registry.register([IB0], IR0, '', 'A1')
        self.assertEqual(registry.registered([IB0], IR0, ''), 'A1')
        self.assertEqual(len(registry._adapters), 2) #order 0 and order 1
        self.assertEqual(registry._generation, 2)

    def test_register_with_invalid_name(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        with self.assertRaises(ValueError):
            registry.register([IB0], IR0, object(), 'A1')

    def test_register_with_value_None_unregisters(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        registry.register([None], IR0, '', 'A1')
        registry.register([None], IR0, '', None)
        self.assertEqual(len(registry._adapters), 0)

    def test_register_with_same_value(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        _value = object()
        registry.register([None], IR0, '', _value)
        _before = registry._generation
        registry.register([None], IR0, '', _value)
        self.assertEqual(registry._generation, _before) # skipped changed()

    def test_registered_empty(self):
        registry = self._makeOne()
        self.assertEqual(registry.registered([None], None, ''), None)

    def test_registered_non_empty_miss(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        registry.register([IB1], None, '', 'A1')
        self.assertEqual(registry.registered([IB2], None, ''), None)

    def test_registered_non_empty_hit(self):
        registry = self._makeOne()
        registry.register([None], None, '', 'A1')
        self.assertEqual(registry.registered([None], None, ''), 'A1')

    def test_unregister_empty(self):
        registry = self._makeOne()
        registry.unregister([None], None, '') #doesn't raise
        self.assertEqual(registry.registered([None], None, ''), None)

    def test_unregister_non_empty_miss_on_required(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        registry.register([IB1], None, '', 'A1')
        registry.unregister([IB2], None, '') #doesn't raise
        self.assertEqual(registry.registered([IB1], None, ''), 'A1')

    def test_unregister_non_empty_miss_on_name(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        registry.register([IB1], None, '', 'A1')
        registry.unregister([IB1], None, 'nonesuch') #doesn't raise
        self.assertEqual(registry.registered([IB1], None, ''), 'A1')

    def test_unregister_with_value_not_None_miss(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        orig = object()
        nomatch = object()
        registry.register([IB1], None, '', orig)
        registry.unregister([IB1], None, '', nomatch) #doesn't raise
        self.assertIs(registry.registered([IB1], None, ''), orig)

    def test_unregister_hit_clears_empty_subcomponents(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        one = object()
        another = object()
        registry.register([IB1, IB2], None, '', one)
        registry.register([IB1, IB3], None, '', another)
        self.assertIn(IB2, registry._adapters[2][IB1])
        self.assertIn(IB3, registry._adapters[2][IB1])
        registry.unregister([IB1, IB3], None, '', another)
        self.assertIn(IB2, registry._adapters[2][IB1])
        self.assertNotIn(IB3, registry._adapters[2][IB1])

    def test_unsubscribe_empty(self):
        registry = self._makeOne()
        registry.unsubscribe([None], None, '') #doesn't raise
        self.assertEqual(registry.registered([None], None, ''), None)

    def test_unsubscribe_hit(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        orig = object()
        registry.subscribe([IB1], None, orig)
        registry.unsubscribe([IB1], None, orig) #doesn't raise
        self.assertEqual(len(registry._subscribers), 0)

    def test_unsubscribe_after_multiple(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        first = object()
        second = object()
        third = object()
        fourth = object()
        registry.subscribe([IB1], None, first)
        registry.subscribe([IB1], None, second)
        registry.subscribe([IB1], IR0, third)
        registry.subscribe([IB1], IR0, fourth)
        registry.unsubscribe([IB1], IR0, fourth)
        registry.unsubscribe([IB1], IR0, third)
        registry.unsubscribe([IB1], None, second)
        registry.unsubscribe([IB1], None, first)
        self.assertEqual(len(registry._subscribers), 0)

    def test_unsubscribe_w_None_after_multiple(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        first = object()
        second = object()

        registry.subscribe([IB1], None, first)
        registry.subscribe([IB1], None, second)
        registry.unsubscribe([IB1], None)
        self.assertEqual(len(registry._subscribers), 0)

    def test_unsubscribe_non_empty_miss_on_required(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        registry.subscribe([IB1], None, 'A1')
        self.assertEqual(len(registry._subscribers), 2)
        registry.unsubscribe([IB2], None, '') #doesn't raise
        self.assertEqual(len(registry._subscribers), 2)

    def test_unsubscribe_non_empty_miss_on_value(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        registry.subscribe([IB1], None, 'A1')
        self.assertEqual(len(registry._subscribers), 2)
        registry.unsubscribe([IB1], None, 'A2') #doesn't raise
        self.assertEqual(len(registry._subscribers), 2)

    def test_unsubscribe_with_value_not_None_miss(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        orig = object()
        nomatch = object()
        registry.subscribe([IB1], None, orig)
        registry.unsubscribe([IB1], None, nomatch) #doesn't raise
        self.assertEqual(len(registry._subscribers), 2)

    def _instance_method_notify_target(self):
        self.fail("Example method, not intended to be called.")

    def test_unsubscribe_instance_method(self):
        IB0, IB1, IB2, IB3, IB4, IF0, IF1, IR0, IR1 = _makeInterfaces() # pylint:disable=unused-variable
        registry = self._makeOne()
        self.assertEqual(len(registry._subscribers), 0)
        registry.subscribe([IB1], None, self._instance_method_notify_target)
        registry.unsubscribe([IB1], None, self._instance_method_notify_target)
        self.assertEqual(len(registry._subscribers), 0)


class LookupBaseFallbackTests(unittest.TestCase):

    def _getFallbackClass(self):
        from zope.interface.adapter import LookupBaseFallback # pylint:disable=no-name-in-module
        return LookupBaseFallback

    _getTargetClass = _getFallbackClass

    def _makeOne(self, uc_lookup=None, uc_lookupAll=None,
                 uc_subscriptions=None):
        # pylint:disable=function-redefined
        if uc_lookup is None:
            def uc_lookup(self, required, provided, name):
                pass
        if uc_lookupAll is None:
            def uc_lookupAll(self, required, provided):
                raise NotImplementedError()
        if uc_subscriptions is None:
            def uc_subscriptions(self, required, provided):
                raise NotImplementedError()
        class Derived(self._getTargetClass()):
            _uncached_lookup = uc_lookup
            _uncached_lookupAll = uc_lookupAll
            _uncached_subscriptions = uc_subscriptions
        return Derived()

    def test_lookup_w_invalid_name(self):
        def _lookup(self, required, provided, name):
            self.fail("This should never be called")
        lb = self._makeOne(uc_lookup=_lookup)
        with self.assertRaises(ValueError):
            lb.lookup(('A',), 'B', object())

    def test_lookup_miss_no_default(self):
        _called_with = []
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))

        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup(('A',), 'B', 'C')
        self.assertIsNone(found)
        self.assertEqual(_called_with, [(('A',), 'B', 'C')])

    def test_lookup_miss_w_default(self):
        _called_with = []
        _default = object()
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))

        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup(('A',), 'B', 'C', _default)
        self.assertIs(found, _default)
        self.assertEqual(_called_with, [(('A',), 'B', 'C')])

    def test_lookup_not_cached(self):
        _called_with = []
        a, b, c = object(), object(), object()
        _results = [a, b, c]
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))
            return _results.pop(0)
        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup(('A',), 'B', 'C')
        self.assertIs(found, a)
        self.assertEqual(_called_with, [(('A',), 'B', 'C')])
        self.assertEqual(_results, [b, c])

    def test_lookup_cached(self):
        _called_with = []
        a, b, c = object(), object(), object()
        _results = [a, b, c]
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))
            return _results.pop(0)
        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup(('A',), 'B', 'C')
        found = lb.lookup(('A',), 'B', 'C')
        self.assertIs(found, a)
        self.assertEqual(_called_with, [(('A',), 'B', 'C')])
        self.assertEqual(_results, [b, c])

    def test_lookup_not_cached_multi_required(self):
        _called_with = []
        a, b, c = object(), object(), object()
        _results = [a, b, c]
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))
            return _results.pop(0)
        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup(('A', 'D'), 'B', 'C')
        self.assertIs(found, a)
        self.assertEqual(_called_with, [(('A', 'D'), 'B', 'C')])
        self.assertEqual(_results, [b, c])

    def test_lookup_cached_multi_required(self):
        _called_with = []
        a, b, c = object(), object(), object()
        _results = [a, b, c]
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))
            return _results.pop(0)
        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup(('A', 'D'), 'B', 'C')
        found = lb.lookup(('A', 'D'), 'B', 'C')
        self.assertIs(found, a)
        self.assertEqual(_called_with, [(('A', 'D'), 'B', 'C')])
        self.assertEqual(_results, [b, c])

    def test_lookup_not_cached_after_changed(self):
        _called_with = []
        a, b, c = object(), object(), object()
        _results = [a, b, c]
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))
            return _results.pop(0)
        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup(('A',), 'B', 'C')
        lb.changed(lb)
        found = lb.lookup(('A',), 'B', 'C')
        self.assertIs(found, b)
        self.assertEqual(_called_with,
                         [(('A',), 'B', 'C'), (('A',), 'B', 'C')])
        self.assertEqual(_results, [c])

    def test_lookup1_w_invalid_name(self):
        def _lookup(self, required, provided, name):
            self.fail("This should never be called")

        lb = self._makeOne(uc_lookup=_lookup)
        with self.assertRaises(ValueError):
            lb.lookup1('A', 'B', object())

    def test_lookup1_miss_no_default(self):
        _called_with = []
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))

        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup1('A', 'B', 'C')
        self.assertIsNone(found)
        self.assertEqual(_called_with, [(('A',), 'B', 'C')])

    def test_lookup1_miss_w_default(self):
        _called_with = []
        _default = object()
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))

        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup1('A', 'B', 'C', _default)
        self.assertIs(found, _default)
        self.assertEqual(_called_with, [(('A',), 'B', 'C')])

    def test_lookup1_miss_w_default_negative_cache(self):
        _called_with = []
        _default = object()
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))

        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup1('A', 'B', 'C', _default)
        self.assertIs(found, _default)
        found = lb.lookup1('A', 'B', 'C', _default)
        self.assertIs(found, _default)
        self.assertEqual(_called_with, [(('A',), 'B', 'C')])

    def test_lookup1_not_cached(self):
        _called_with = []
        a, b, c = object(), object(), object()
        _results = [a, b, c]
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))
            return _results.pop(0)
        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup1('A', 'B', 'C')
        self.assertIs(found, a)
        self.assertEqual(_called_with, [(('A',), 'B', 'C')])
        self.assertEqual(_results, [b, c])

    def test_lookup1_cached(self):
        _called_with = []
        a, b, c = object(), object(), object()
        _results = [a, b, c]
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))
            return _results.pop(0)
        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup1('A', 'B', 'C')
        found = lb.lookup1('A', 'B', 'C')
        self.assertIs(found, a)
        self.assertEqual(_called_with, [(('A',), 'B', 'C')])
        self.assertEqual(_results, [b, c])

    def test_lookup1_not_cached_after_changed(self):
        _called_with = []
        a, b, c = object(), object(), object()
        _results = [a, b, c]
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))
            return _results.pop(0)
        lb = self._makeOne(uc_lookup=_lookup)
        found = lb.lookup1('A', 'B', 'C')
        lb.changed(lb)
        found = lb.lookup1('A', 'B', 'C')
        self.assertIs(found, b)
        self.assertEqual(_called_with,
                         [(('A',), 'B', 'C'), (('A',), 'B', 'C')])
        self.assertEqual(_results, [c])

    def test_adapter_hook_w_invalid_name(self):
        req, prv = object(), object()
        lb = self._makeOne()
        with self.assertRaises(ValueError):
            lb.adapter_hook(prv, req, object())

    def test_adapter_hook_miss_no_default(self):
        req, prv = object(), object()
        lb = self._makeOne()
        found = lb.adapter_hook(prv, req, '')
        self.assertIsNone(found)

    def test_adapter_hook_miss_w_default(self):
        req, prv, _default = object(), object(), object()
        lb = self._makeOne()
        found = lb.adapter_hook(prv, req, '', _default)
        self.assertIs(found, _default)

    def test_adapter_hook_hit_factory_returns_None(self):
        _f_called_with = []
        def _factory(context):
            _f_called_with.append(context)

        def _lookup(self, required, provided, name):
            return _factory
        req, prv, _default = object(), object(), object()
        lb = self._makeOne(uc_lookup=_lookup)
        adapted = lb.adapter_hook(prv, req, 'C', _default)
        self.assertIs(adapted, _default)
        self.assertEqual(_f_called_with, [req])

    def test_adapter_hook_hit_factory_returns_adapter(self):
        _f_called_with = []
        _adapter = object()
        def _factory(context):
            _f_called_with.append(context)
            return _adapter
        def _lookup(self, required, provided, name):
            return _factory
        req, prv, _default = object(), object(), object()
        lb = self._makeOne(uc_lookup=_lookup)
        adapted = lb.adapter_hook(prv, req, 'C', _default)
        self.assertIs(adapted, _adapter)
        self.assertEqual(_f_called_with, [req])

    def test_adapter_hook_super_unwraps(self):
        _f_called_with = []
        def _factory(context):
            _f_called_with.append(context)
            return context
        def _lookup(self, required, provided, name=''):
            return _factory
        required = super(LookupBaseFallbackTests, self)
        provided = object()
        lb = self._makeOne(uc_lookup=_lookup)
        adapted = lb.adapter_hook(provided, required)
        self.assertIs(adapted, self)
        self.assertEqual(_f_called_with, [self])

    def test_queryAdapter(self):
        _f_called_with = []
        _adapter = object()
        def _factory(context):
            _f_called_with.append(context)
            return _adapter
        def _lookup(self, required, provided, name):
            return _factory
        req, prv, _default = object(), object(), object()
        lb = self._makeOne(uc_lookup=_lookup)
        adapted = lb.queryAdapter(req, prv, 'C', _default)
        self.assertIs(adapted, _adapter)
        self.assertEqual(_f_called_with, [req])

    def test_lookupAll_uncached(self):
        _called_with = []
        _results = [object(), object(), object()]
        def _lookupAll(self, required, provided):
            _called_with.append((required, provided))
            return tuple(_results)
        lb = self._makeOne(uc_lookupAll=_lookupAll)
        found = lb.lookupAll('A', 'B')
        self.assertEqual(found, tuple(_results))
        self.assertEqual(_called_with, [(('A',), 'B')])

    def test_lookupAll_cached(self):
        _called_with = []
        _results = [object(), object(), object()]
        def _lookupAll(self, required, provided):
            _called_with.append((required, provided))
            return tuple(_results)
        lb = self._makeOne(uc_lookupAll=_lookupAll)
        found = lb.lookupAll('A', 'B')
        found = lb.lookupAll('A', 'B')
        self.assertEqual(found, tuple(_results))
        self.assertEqual(_called_with, [(('A',), 'B')])

    def test_subscriptions_uncached(self):
        _called_with = []
        _results = [object(), object(), object()]
        def _subscriptions(self, required, provided):
            _called_with.append((required, provided))
            return tuple(_results)
        lb = self._makeOne(uc_subscriptions=_subscriptions)
        found = lb.subscriptions('A', 'B')
        self.assertEqual(found, tuple(_results))
        self.assertEqual(_called_with, [(('A',), 'B')])

    def test_subscriptions_cached(self):
        _called_with = []
        _results = [object(), object(), object()]
        def _subscriptions(self, required, provided):
            _called_with.append((required, provided))
            return tuple(_results)
        lb = self._makeOne(uc_subscriptions=_subscriptions)
        found = lb.subscriptions('A', 'B')
        found = lb.subscriptions('A', 'B')
        self.assertEqual(found, tuple(_results))
        self.assertEqual(_called_with, [(('A',), 'B')])


class LookupBaseTests(LookupBaseFallbackTests,
                      OptimizationTestMixin):

    def _getTargetClass(self):
        from zope.interface.adapter import LookupBase
        return LookupBase


class VerifyingBaseFallbackTests(unittest.TestCase):

    def _getFallbackClass(self):
        from zope.interface.adapter import VerifyingBaseFallback # pylint:disable=no-name-in-module
        return VerifyingBaseFallback

    _getTargetClass = _getFallbackClass

    def _makeOne(self, registry, uc_lookup=None, uc_lookupAll=None,
                 uc_subscriptions=None):
        # pylint:disable=function-redefined
        if uc_lookup is None:
            def uc_lookup(self, required, provided, name):
                raise NotImplementedError()
        if uc_lookupAll is None:
            def uc_lookupAll(self, required, provided):
                raise NotImplementedError()
        if uc_subscriptions is None:
            def uc_subscriptions(self, required, provided):
                raise NotImplementedError()
        class Derived(self._getTargetClass()):
            _uncached_lookup = uc_lookup
            _uncached_lookupAll = uc_lookupAll
            _uncached_subscriptions = uc_subscriptions
            def __init__(self, registry):
                super(Derived, self).__init__()
                self._registry = registry
        derived = Derived(registry)
        derived.changed(derived) # init. '_verify_ro' / '_verify_generations'
        return derived

    def _makeRegistry(self, depth):
        class WithGeneration(object):
            _generation = 1
        class Registry:
            def __init__(self, depth):
                self.ro = [WithGeneration() for i in range(depth)]
        return Registry(depth)

    def test_lookup(self):
        _called_with = []
        a, b, c = object(), object(), object()
        _results = [a, b, c]
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))
            return _results.pop(0)
        reg = self._makeRegistry(3)
        lb = self._makeOne(reg, uc_lookup=_lookup)
        found = lb.lookup(('A',), 'B', 'C')
        found = lb.lookup(('A',), 'B', 'C')
        self.assertIs(found, a)
        self.assertEqual(_called_with, [(('A',), 'B', 'C')])
        self.assertEqual(_results, [b, c])
        reg.ro[1]._generation += 1
        found = lb.lookup(('A',), 'B', 'C')
        self.assertIs(found, b)
        self.assertEqual(_called_with,
                         [(('A',), 'B', 'C'), (('A',), 'B', 'C')])
        self.assertEqual(_results, [c])

    def test_lookup1(self):
        _called_with = []
        a, b, c = object(), object(), object()
        _results = [a, b, c]
        def _lookup(self, required, provided, name):
            _called_with.append((required, provided, name))
            return _results.pop(0)
        reg = self._makeRegistry(3)
        lb = self._makeOne(reg, uc_lookup=_lookup)
        found = lb.lookup1('A', 'B', 'C')
        found = lb.lookup1('A', 'B', 'C')
        self.assertIs(found, a)
        self.assertEqual(_called_with, [(('A',), 'B', 'C')])
        self.assertEqual(_results, [b, c])
        reg.ro[1]._generation += 1
        found = lb.lookup1('A', 'B', 'C')
        self.assertIs(found, b)
        self.assertEqual(_called_with,
                         [(('A',), 'B', 'C'), (('A',), 'B', 'C')])
        self.assertEqual(_results, [c])

    def test_adapter_hook(self):
        a, b, _c = [object(), object(), object()]
        def _factory1(context):
            return a
        def _factory2(context):
            return b
        def _factory3(context):
            self.fail("This should never be called")
        _factories = [_factory1, _factory2, _factory3]
        def _lookup(self, required, provided, name):
            return _factories.pop(0)
        req, prv, _default = object(), object(), object()
        reg = self._makeRegistry(3)
        lb = self._makeOne(reg, uc_lookup=_lookup)
        adapted = lb.adapter_hook(prv, req, 'C', _default)
        self.assertIs(adapted, a)
        adapted = lb.adapter_hook(prv, req, 'C', _default)
        self.assertIs(adapted, a)
        reg.ro[1]._generation += 1
        adapted = lb.adapter_hook(prv, req, 'C', _default)
        self.assertIs(adapted, b)

    def test_queryAdapter(self):
        a, b, _c = [object(), object(), object()]
        def _factory1(context):
            return a
        def _factory2(context):
            return b
        def _factory3(context):
            self.fail("This should never be called")
        _factories = [_factory1, _factory2, _factory3]
        def _lookup(self, required, provided, name):
            return _factories.pop(0)
        req, prv, _default = object(), object(), object()
        reg = self._makeRegistry(3)
        lb = self._makeOne(reg, uc_lookup=_lookup)
        adapted = lb.queryAdapter(req, prv, 'C', _default)
        self.assertIs(adapted, a)
        adapted = lb.queryAdapter(req, prv, 'C', _default)
        self.assertIs(adapted, a)
        reg.ro[1]._generation += 1
        adapted = lb.adapter_hook(prv, req, 'C', _default)
        self.assertIs(adapted, b)

    def test_lookupAll(self):
        _results_1 = [object(), object(), object()]
        _results_2 = [object(), object(), object()]
        _results = [_results_1, _results_2]
        def _lookupAll(self, required, provided):
            return tuple(_results.pop(0))
        reg = self._makeRegistry(3)
        lb = self._makeOne(reg, uc_lookupAll=_lookupAll)
        found = lb.lookupAll('A', 'B')
        self.assertEqual(found, tuple(_results_1))
        found = lb.lookupAll('A', 'B')
        self.assertEqual(found, tuple(_results_1))
        reg.ro[1]._generation += 1
        found = lb.lookupAll('A', 'B')
        self.assertEqual(found, tuple(_results_2))

    def test_subscriptions(self):
        _results_1 = [object(), object(), object()]
        _results_2 = [object(), object(), object()]
        _results = [_results_1, _results_2]
        def _subscriptions(self, required, provided):
            return tuple(_results.pop(0))
        reg = self._makeRegistry(3)
        lb = self._makeOne(reg, uc_subscriptions=_subscriptions)
        found = lb.subscriptions('A', 'B')
        self.assertEqual(found, tuple(_results_1))
        found = lb.subscriptions('A', 'B')
        self.assertEqual(found, tuple(_results_1))
        reg.ro[1]._generation += 1
        found = lb.subscriptions('A', 'B')
        self.assertEqual(found, tuple(_results_2))


class VerifyingBaseTests(VerifyingBaseFallbackTests,
                         OptimizationTestMixin):

    def _getTargetClass(self):
        from zope.interface.adapter import VerifyingBase
        return VerifyingBase


class AdapterLookupBaseTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.adapter import AdapterLookupBase
        return AdapterLookupBase

    def _makeOne(self, registry):
        return self._getTargetClass()(registry)

    def _makeSubregistry(self, *provided):
        class Subregistry:
            def __init__(self):
                self._adapters = []
                self._subscribers = []
        return Subregistry()

    def _makeRegistry(self, *provided):
        class Registry:
            def __init__(self, provided):
                self._provided = provided
                self.ro = []
        return Registry(provided)

    def test_ctor_empty_registry(self):
        registry = self._makeRegistry()
        alb = self._makeOne(registry)
        self.assertEqual(alb._extendors, {})

    def test_ctor_w_registry_provided(self):
        from zope.interface import Interface
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry(IFoo, IBar)
        alb = self._makeOne(registry)
        self.assertEqual(sorted(alb._extendors.keys()),
                         sorted([IBar, IFoo, Interface]))
        self.assertEqual(alb._extendors[IFoo], [IFoo, IBar])
        self.assertEqual(alb._extendors[IBar], [IBar])
        self.assertEqual(sorted(alb._extendors[Interface]),
                         sorted([IFoo, IBar]))

    def test_changed_empty_required(self):
        # ALB.changed expects to call a mixed in changed.
        class Mixin(object):
            def changed(self, *other):
                pass
        class Derived(self._getTargetClass(), Mixin):
            pass
        registry = self._makeRegistry()
        alb = Derived(registry)
        alb.changed(alb)

    def test_changed_w_required(self):
        # ALB.changed expects to call a mixed in changed.
        class Mixin(object):
            def changed(self, *other):
                pass
        class Derived(self._getTargetClass(), Mixin):
            pass
        class FauxWeakref(object):
            _unsub = None
            def __init__(self, here):
                self._here = here
            def __call__(self):
                return self if self._here else None
            def unsubscribe(self, target):
                self._unsub = target
        gone = FauxWeakref(False)
        here = FauxWeakref(True)
        registry = self._makeRegistry()
        alb = Derived(registry)
        alb._required[gone] = 1
        alb._required[here] = 1
        alb.changed(alb)
        self.assertEqual(len(alb._required), 0)
        self.assertEqual(gone._unsub, None)
        self.assertEqual(here._unsub, alb)

    def test_init_extendors_after_registry_update(self):
        from zope.interface import Interface
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry()
        alb = self._makeOne(registry)
        registry._provided = [IFoo, IBar]
        alb.init_extendors()
        self.assertEqual(sorted(alb._extendors.keys()),
                         sorted([IBar, IFoo, Interface]))
        self.assertEqual(alb._extendors[IFoo], [IFoo, IBar])
        self.assertEqual(alb._extendors[IBar], [IBar])
        self.assertEqual(sorted(alb._extendors[Interface]),
                         sorted([IFoo, IBar]))

    def test_add_extendor(self):
        from zope.interface import Interface
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry()
        alb = self._makeOne(registry)
        alb.add_extendor(IFoo)
        alb.add_extendor(IBar)
        self.assertEqual(sorted(alb._extendors.keys()),
                         sorted([IBar, IFoo, Interface]))
        self.assertEqual(alb._extendors[IFoo], [IFoo, IBar])
        self.assertEqual(alb._extendors[IBar], [IBar])
        self.assertEqual(sorted(alb._extendors[Interface]),
                         sorted([IFoo, IBar]))

    def test_remove_extendor(self):
        from zope.interface import Interface
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry(IFoo, IBar)
        alb = self._makeOne(registry)
        alb.remove_extendor(IFoo)
        self.assertEqual(sorted(alb._extendors.keys()),
                         sorted([IFoo, IBar, Interface]))
        self.assertEqual(alb._extendors[IFoo], [IBar])
        self.assertEqual(alb._extendors[IBar], [IBar])
        self.assertEqual(sorted(alb._extendors[Interface]),
                         sorted([IBar]))

    # test '_subscribe' via its callers, '_uncached_lookup', etc.

    def test__uncached_lookup_empty_ro(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry()
        alb = self._makeOne(registry)
        result = alb._uncached_lookup((IFoo,), IBar)
        self.assertEqual(result, None)
        self.assertEqual(len(alb._required), 1)
        self.assertIn(IFoo.weakref(), alb._required)

    def test__uncached_lookup_order_miss(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        result = alb._uncached_lookup((IFoo,), IBar)
        self.assertEqual(result, None)

    def test__uncached_lookup_extendors_miss(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry()
        subr = self._makeSubregistry()
        subr._adapters = [{}, {}] #utilities, single adapters
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_lookup((IFoo,), IBar)
        self.assertEqual(result, None)

    def test__uncached_lookup_components_miss_wrong_iface(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        IQux = InterfaceClass('IQux')
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        irrelevant = object()
        subr._adapters = [ #utilities, single adapters
            {},
            {IFoo: {IQux: {'': irrelevant},
                   }},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_lookup((IFoo,), IBar)
        self.assertEqual(result, None)

    def test__uncached_lookup_components_miss_wrong_name(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()

        wrongname = object()
        subr._adapters = [ #utilities, single adapters
            {},
            {IFoo: {IBar: {'wrongname': wrongname},
                   }},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_lookup((IFoo,), IBar)
        self.assertEqual(result, None)

    def test__uncached_lookup_simple_hit(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        _expected = object()
        subr._adapters = [ #utilities, single adapters
            {},
            {IFoo: {IBar: {'': _expected}}},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_lookup((IFoo,), IBar)
        self.assertIs(result, _expected)

    def test__uncached_lookup_repeated_hit(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        _expected = object()
        subr._adapters = [ #utilities, single adapters
            {},
            {IFoo: {IBar: {'': _expected}}},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_lookup((IFoo,), IBar)
        result2 = alb._uncached_lookup((IFoo,), IBar)
        self.assertIs(result, _expected)
        self.assertIs(result2, _expected)

    def test_queryMultiAdaptor_lookup_miss(self):
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        @implementer(IFoo)
        class Foo(object):
            pass
        foo = Foo()
        registry = self._makeRegistry()
        subr = self._makeSubregistry()
        subr._adapters = [ #utilities, single adapters
            {},
            {},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        alb.lookup = alb._uncached_lookup # provided by derived
        subr._v_lookup = alb
        _default = object()
        result = alb.queryMultiAdapter((foo,), IBar, default=_default)
        self.assertIs(result, _default)

    def test_queryMultiAdapter_errors_on_attribute_access(self):
        # Any error on attribute access previously lead to using the _empty singleton as "requires"
        # argument (See https://github.com/zopefoundation/zope.interface/issues/162)
        # but after https://github.com/zopefoundation/zope.interface/issues/200
        # they get propagated.
        from zope.interface.interface import InterfaceClass
        from zope.interface.tests import MissingSomeAttrs

        IFoo = InterfaceClass('IFoo')
        registry = self._makeRegistry()
        alb = self._makeOne(registry)
        alb.lookup = alb._uncached_lookup

        def test(ob):
            return alb.queryMultiAdapter(
                (ob,),
                IFoo,
            )

        PY3 = str is not bytes
        MissingSomeAttrs.test_raises(self, test,
                                     expected_missing='__class__' if PY3 else '__providedBy__')

    def test_queryMultiAdaptor_factory_miss(self):
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        @implementer(IFoo)
        class Foo(object):
            pass
        foo = Foo()
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        _expected = object()
        _called_with = []
        def _factory(context):
            _called_with.append(context)

        subr._adapters = [ #utilities, single adapters
            {},
            {IFoo: {IBar: {'': _factory}}},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        alb.lookup = alb._uncached_lookup # provided by derived
        subr._v_lookup = alb
        _default = object()
        result = alb.queryMultiAdapter((foo,), IBar, default=_default)
        self.assertIs(result, _default)
        self.assertEqual(_called_with, [foo])

    def test_queryMultiAdaptor_factory_hit(self):
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        @implementer(IFoo)
        class Foo(object):
            pass
        foo = Foo()
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        _expected = object()
        _called_with = []
        def _factory(context):
            _called_with.append(context)
            return _expected
        subr._adapters = [ #utilities, single adapters
            {},
            {IFoo: {IBar: {'': _factory}}},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        alb.lookup = alb._uncached_lookup # provided by derived
        subr._v_lookup = alb
        _default = object()
        result = alb.queryMultiAdapter((foo,), IBar, default=_default)
        self.assertIs(result, _expected)
        self.assertEqual(_called_with, [foo])

    def test_queryMultiAdapter_super_unwraps(self):
        alb = self._makeOne(self._makeRegistry())
        def lookup(*args):
            return factory
        def factory(*args):
            return args
        alb.lookup = lookup

        objects = [
            super(AdapterLookupBaseTests, self),
            42,
            "abc",
            super(AdapterLookupBaseTests, self),
        ]

        result = alb.queryMultiAdapter(objects, None)
        self.assertEqual(result, (
            self,
            42,
            "abc",
            self,
        ))

    def test__uncached_lookupAll_empty_ro(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry()
        alb = self._makeOne(registry)
        result = alb._uncached_lookupAll((IFoo,), IBar)
        self.assertEqual(result, ())
        self.assertEqual(len(alb._required), 1)
        self.assertIn(IFoo.weakref(), alb._required)

    def test__uncached_lookupAll_order_miss(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_lookupAll((IFoo,), IBar)
        self.assertEqual(result, ())

    def test__uncached_lookupAll_extendors_miss(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry()
        subr = self._makeSubregistry()
        subr._adapters = [{}, {}] #utilities, single adapters
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_lookupAll((IFoo,), IBar)
        self.assertEqual(result, ())

    def test__uncached_lookupAll_components_miss(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        IQux = InterfaceClass('IQux')
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        irrelevant = object()
        subr._adapters = [ #utilities, single adapters
            {},
            {IFoo: {IQux: {'': irrelevant}}},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_lookupAll((IFoo,), IBar)
        self.assertEqual(result, ())

    def test__uncached_lookupAll_simple_hit(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        _expected = object()
        _named = object()
        subr._adapters = [ #utilities, single adapters
            {},
            {IFoo: {IBar: {'': _expected, 'named': _named}}},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_lookupAll((IFoo,), IBar)
        self.assertEqual(sorted(result), [('', _expected), ('named', _named)])

    def test_names(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        _expected = object()
        _named = object()
        subr._adapters = [ #utilities, single adapters
            {},
            {IFoo: {IBar: {'': _expected, 'named': _named}}},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        alb.lookupAll = alb._uncached_lookupAll
        subr._v_lookup = alb
        result = alb.names((IFoo,), IBar)
        self.assertEqual(sorted(result), ['', 'named'])

    def test__uncached_subscriptions_empty_ro(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry()
        alb = self._makeOne(registry)
        result = alb._uncached_subscriptions((IFoo,), IBar)
        self.assertEqual(result, [])
        self.assertEqual(len(alb._required), 1)
        self.assertIn(IFoo.weakref(), alb._required)

    def test__uncached_subscriptions_order_miss(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_subscriptions((IFoo,), IBar)
        self.assertEqual(result, [])

    def test__uncached_subscriptions_extendors_miss(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry()
        subr = self._makeSubregistry()
        subr._subscribers = [{}, {}] #utilities, single adapters
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_subscriptions((IFoo,), IBar)
        self.assertEqual(result, [])

    def test__uncached_subscriptions_components_miss_wrong_iface(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        IQux = InterfaceClass('IQux')
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        irrelevant = object()
        subr._subscribers = [ #utilities, single adapters
            {},
            {IFoo: {IQux: {'': irrelevant}}},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_subscriptions((IFoo,), IBar)
        self.assertEqual(result, [])

    def test__uncached_subscriptions_components_miss_wrong_name(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        wrongname = object()
        subr._subscribers = [ #utilities, single adapters
            {},
            {IFoo: {IBar: {'wrongname': wrongname}}},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_subscriptions((IFoo,), IBar)
        self.assertEqual(result, [])

    def test__uncached_subscriptions_simple_hit(self):
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        class Foo(object):
            def __lt__(self, other):
                return True
        _exp1, _exp2 = Foo(), Foo()
        subr._subscribers = [ #utilities, single adapters
            {},
            {IFoo: {IBar: {'': (_exp1, _exp2)}}},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        subr._v_lookup = alb
        result = alb._uncached_subscriptions((IFoo,), IBar)
        self.assertEqual(sorted(result), sorted([_exp1, _exp2]))

    def test_subscribers_wo_provided(self):
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        @implementer(IFoo)
        class Foo(object):
            pass
        foo = Foo()
        registry = self._makeRegistry(IFoo, IBar)
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        _called = {}
        def _factory1(context):
            _called.setdefault('_factory1', []).append(context)
        def _factory2(context):
            _called.setdefault('_factory2', []).append(context)
        subr._subscribers = [ #utilities, single adapters
            {},
            {IFoo: {None: {'': (_factory1, _factory2)}}},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        alb.subscriptions = alb._uncached_subscriptions
        subr._v_lookup = alb
        result = alb.subscribers((foo,), None)
        self.assertEqual(result, ())
        self.assertEqual(_called, {'_factory1': [foo], '_factory2': [foo]})

    def test_subscribers_w_provided(self):
        from zope.interface.declarations import implementer
        from zope.interface.interface import InterfaceClass
        IFoo = InterfaceClass('IFoo')
        IBar = InterfaceClass('IBar', (IFoo,))
        @implementer(IFoo)
        class Foo(object):
            pass
        foo = Foo()
        registry = self._makeRegistry(IFoo, IBar)
        registry = self._makeRegistry(IFoo, IBar)
        subr = self._makeSubregistry()
        _called = {}
        _exp1, _exp2 = object(), object()
        def _factory1(context):
            _called.setdefault('_factory1', []).append(context)
            return _exp1
        def _factory2(context):
            _called.setdefault('_factory2', []).append(context)
            return _exp2
        def _side_effect_only(context):
            _called.setdefault('_side_effect_only', []).append(context)

        subr._subscribers = [ #utilities, single adapters
            {},
            {IFoo: {IBar: {'': (_factory1, _factory2, _side_effect_only)}}},
        ]
        registry.ro.append(subr)
        alb = self._makeOne(registry)
        alb.subscriptions = alb._uncached_subscriptions
        subr._v_lookup = alb
        result = alb.subscribers((foo,), IBar)
        self.assertEqual(result, [_exp1, _exp2])
        self.assertEqual(_called,
                         {'_factory1': [foo],
                          '_factory2': [foo],
                          '_side_effect_only': [foo],
                         })


class VerifyingAdapterRegistryTests(unittest.TestCase):
    # This is also the base for AdapterRegistryTests. That makes the
    # inheritance seems backwards, but even though they implement the
    # same interfaces, VAR and AR each only extend BAR; and neither
    # one will pass the test cases for BAR (it uses a special
    # LookupClass just for the tests).

    def _getTargetClass(self):
        from zope.interface.adapter import VerifyingAdapterRegistry
        return VerifyingAdapterRegistry

    def _makeOne(self, *args, **kw):
        return self._getTargetClass()(*args, **kw)

    def test_verify_object_provides_IAdapterRegistry(self):
        from zope.interface.verify import verifyObject
        from zope.interface.interfaces import IAdapterRegistry
        registry = self._makeOne()
        verifyObject(IAdapterRegistry, registry)


class AdapterRegistryTests(VerifyingAdapterRegistryTests):

    def _getTargetClass(self):
        from zope.interface.adapter import AdapterRegistry
        return AdapterRegistry

    def test_ctor_no_bases(self):
        ar = self._makeOne()
        self.assertEqual(len(ar._v_subregistries), 0)

    def test_ctor_w_bases(self):
        base = self._makeOne()
        sub = self._makeOne([base])
        self.assertEqual(len(sub._v_subregistries), 0)
        self.assertEqual(len(base._v_subregistries), 1)
        self.assertIn(sub, base._v_subregistries)

    # test _addSubregistry / _removeSubregistry via only caller, _setBases

    def test__setBases_removing_existing_subregistry(self):
        before = self._makeOne()
        after = self._makeOne()
        sub = self._makeOne([before])
        sub.__bases__ = [after]
        self.assertEqual(len(before._v_subregistries), 0)
        self.assertEqual(len(after._v_subregistries), 1)
        self.assertIn(sub, after._v_subregistries)

    def test__setBases_wo_stray_entry(self):
        before = self._makeOne()
        stray = self._makeOne()
        after = self._makeOne()
        sub = self._makeOne([before])
        sub.__dict__['__bases__'].append(stray)
        sub.__bases__ = [after]
        self.assertEqual(len(before._v_subregistries), 0)
        self.assertEqual(len(after._v_subregistries), 1)
        self.assertIn(sub, after._v_subregistries)

    def test__setBases_w_existing_entry_continuing(self):
        before = self._makeOne()
        after = self._makeOne()
        sub = self._makeOne([before])
        sub.__bases__ = [before, after]
        self.assertEqual(len(before._v_subregistries), 1)
        self.assertEqual(len(after._v_subregistries), 1)
        self.assertIn(sub, before._v_subregistries)
        self.assertIn(sub, after._v_subregistries)

    def test_changed_w_subregistries(self):
        base = self._makeOne()
        class Derived(object):
            _changed = None
            def changed(self, originally_changed):
                self._changed = originally_changed
        derived1, derived2 = Derived(), Derived()
        base._addSubregistry(derived1)
        base._addSubregistry(derived2)
        orig = object()
        base.changed(orig)
        self.assertIs(derived1._changed, orig)
        self.assertIs(derived2._changed, orig)


class Test_utils(unittest.TestCase):

    def test__convert_None_to_Interface_w_None(self):
        from zope.interface.adapter import _convert_None_to_Interface
        from zope.interface.interface import Interface
        self.assertIs(_convert_None_to_Interface(None), Interface)

    def test__convert_None_to_Interface_w_other(self):
        from zope.interface.adapter import _convert_None_to_Interface
        other = object()
        self.assertIs(_convert_None_to_Interface(other), other)

    def test__normalize_name_str(self):
        from zope.interface.adapter import _normalize_name
        STR = b'str'
        UNICODE = u'str'
        norm = _normalize_name(STR)
        self.assertEqual(norm, UNICODE)
        self.assertIsInstance(norm, type(UNICODE))

    def test__normalize_name_unicode(self):
        from zope.interface.adapter import _normalize_name

        USTR = u'ustr'
        self.assertEqual(_normalize_name(USTR), USTR)

    def test__normalize_name_other(self):
        from zope.interface.adapter import _normalize_name
        for other in 1, 1.0, (), [], {}, object():
            self.assertRaises(TypeError, _normalize_name, other)

    # _lookup, _lookupAll, and _subscriptions tested via their callers
    # (AdapterLookupBase.{lookup,lookupAll,subscriptions}).
