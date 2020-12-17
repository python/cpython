##############################################################################
#
# Copyright (c) 2001, 2002, 2009 Zope Foundation and Contributors.
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
"""Component Registry Tests"""
# pylint:disable=protected-access
import unittest

from zope.interface import Interface
from zope.interface.adapter import VerifyingAdapterRegistry

from zope.interface.registry import Components

class ComponentsTests(unittest.TestCase):

    def _getTargetClass(self):
        return Components

    def _makeOne(self, name='test', *args, **kw):
        return self._getTargetClass()(name, *args, **kw)

    def _wrapEvents(self):
        from zope.interface import registry
        _events = []
        def _notify(*args, **kw):
            _events.append((args, kw))
        _monkey = _Monkey(registry, notify=_notify)
        return _monkey, _events

    def test_ctor_no_bases(self):
        from zope.interface.adapter import AdapterRegistry
        comp = self._makeOne('testing')
        self.assertEqual(comp.__name__, 'testing')
        self.assertEqual(comp.__bases__, ())
        self.assertTrue(isinstance(comp.adapters, AdapterRegistry))
        self.assertTrue(isinstance(comp.utilities, AdapterRegistry))
        self.assertEqual(comp.adapters.__bases__, ())
        self.assertEqual(comp.utilities.__bases__, ())
        self.assertEqual(comp._utility_registrations, {})
        self.assertEqual(comp._adapter_registrations, {})
        self.assertEqual(comp._subscription_registrations, [])
        self.assertEqual(comp._handler_registrations, [])

    def test_ctor_w_base(self):
        base = self._makeOne('base')
        comp = self._makeOne('testing', (base,))
        self.assertEqual(comp.__name__, 'testing')
        self.assertEqual(comp.__bases__, (base,))
        self.assertEqual(comp.adapters.__bases__, (base.adapters,))
        self.assertEqual(comp.utilities.__bases__, (base.utilities,))

    def test___repr__(self):
        comp = self._makeOne('testing')
        self.assertEqual(repr(comp), '<Components testing>')

    # test _init_registries / _init_registrations via only caller, __init__.

    def test_assign_to___bases__(self):
        base1 = self._makeOne('base1')
        base2 = self._makeOne('base2')
        comp = self._makeOne()
        comp.__bases__ = (base1, base2)
        self.assertEqual(comp.__bases__, (base1, base2))
        self.assertEqual(comp.adapters.__bases__,
                         (base1.adapters, base2.adapters))
        self.assertEqual(comp.utilities.__bases__,
                         (base1.utilities, base2.utilities))

    def test_registerUtility_with_component_name(self):
        from zope.interface.declarations import named, InterfaceClass


        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')

        @named(u'foo')
        class Foo(object):
            pass
        foo = Foo()
        _info = u'info'

        comp = self._makeOne()
        comp.registerUtility(foo, ifoo, info=_info)
        self.assertEqual(
            comp._utility_registrations[ifoo, u'foo'],
            (foo, _info, None))

    def test_registerUtility_both_factory_and_component(self):
        def _factory():
            raise NotImplementedError()
        _to_reg = object()
        comp = self._makeOne()
        self.assertRaises(TypeError, comp.registerUtility,
                          component=_to_reg, factory=_factory)

    def test_registerUtility_w_component(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Registered
        from zope.interface.registry import UtilityRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name = u'name'
        _to_reg = object()
        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerUtility(_to_reg, ifoo, _name, _info)
        self.assertTrue(comp.utilities._adapters[0][ifoo][_name] is _to_reg)
        self.assertEqual(comp._utility_registrations[ifoo, _name],
                         (_to_reg, _info, None))
        self.assertEqual(comp.utilities._subscribers[0][ifoo][''], (_to_reg,))
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, UtilityRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.component is _to_reg)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is None)

    def test_registerUtility_w_factory(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Registered
        from zope.interface.registry import UtilityRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name = u'name'
        _to_reg = object()
        def _factory():
            return _to_reg
        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerUtility(None, ifoo, _name, _info, factory=_factory)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, UtilityRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.component is _to_reg)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is _factory)

    def test_registerUtility_no_provided_available(self):
        class Foo(object):
            pass

        _info = u'info'
        _name = u'name'
        _to_reg = Foo()
        comp = self._makeOne()
        self.assertRaises(TypeError,
                          comp.registerUtility, _to_reg, None, _name, _info)

    def test_registerUtility_wo_provided(self):
        from zope.interface.declarations import directlyProvides
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Registered
        from zope.interface.registry import UtilityRegistration

        class IFoo(InterfaceClass):
            pass
        class Foo(object):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name = u'name'
        _to_reg = Foo()
        directlyProvides(_to_reg, ifoo)
        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerUtility(_to_reg, None, _name, _info)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, UtilityRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.component is _to_reg)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is None)

    def test_registerUtility_duplicates_existing_reg(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name = u'name'
        _to_reg = object()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, _name, _info)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerUtility(_to_reg, ifoo, _name, _info)
        self.assertEqual(len(_events), 0)

    def test_registerUtility_w_different_info(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info1 = u'info1'
        _info2 = u'info2'
        _name = u'name'
        _to_reg = object()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, _name, _info1)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerUtility(_to_reg, ifoo, _name, _info2)
        self.assertEqual(len(_events), 2)  # unreg, reg
        self.assertEqual(comp._utility_registrations[(ifoo, _name)],
                         (_to_reg, _info2, None))  # replaced
        self.assertEqual(comp.utilities._subscribers[0][ifoo][u''],
                         (_to_reg,))

    def test_registerUtility_w_different_names_same_component(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name1 = u'name1'
        _name2 = u'name2'
        _other_reg = object()
        _to_reg = object()
        comp = self._makeOne()
        comp.registerUtility(_other_reg, ifoo, _name1, _info)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerUtility(_to_reg, ifoo, _name2, _info)
        self.assertEqual(len(_events), 1)  # reg
        self.assertEqual(comp._utility_registrations[(ifoo, _name1)],
                         (_other_reg, _info, None))
        self.assertEqual(comp._utility_registrations[(ifoo, _name2)],
                         (_to_reg, _info, None))
        self.assertEqual(comp.utilities._subscribers[0][ifoo][u''],
                         (_other_reg, _to_reg,))

    def test_registerUtility_replaces_existing_reg(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.interfaces import Registered
        from zope.interface.registry import UtilityRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name = u'name'
        _before, _after = object(), object()
        comp = self._makeOne()
        comp.registerUtility(_before, ifoo, _name, _info)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerUtility(_after, ifoo, _name, _info)
        self.assertEqual(len(_events), 2)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, UtilityRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.component is _before)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is None)
        args, kw = _events[1]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, UtilityRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.component is _after)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is None)

    def test_registerUtility_w_existing_subscr(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name1 = u'name1'
        _name2 = u'name2'
        _to_reg = object()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, _name1, _info)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerUtility(_to_reg, ifoo, _name2, _info)
        self.assertEqual(comp.utilities._subscribers[0][ifoo][''], (_to_reg,))

    def test_registerUtility_wo_event(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name = u'name'
        _to_reg = object()
        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerUtility(_to_reg, ifoo, _name, _info, False)
        self.assertEqual(len(_events), 0)

    def test_registerUtility_changes_object_identity_after(self):
        # If a subclass changes the identity of the _utility_registrations,
        # the cache is updated and the right thing still happens.
        class CompThatChangesAfter1Reg(self._getTargetClass()):
            reg_count = 0
            def registerUtility(self, *args):
                self.reg_count += 1
                super(CompThatChangesAfter1Reg, self).registerUtility(*args)
                if self.reg_count == 1:
                    self._utility_registrations = dict(self._utility_registrations)

        comp = CompThatChangesAfter1Reg()
        comp.registerUtility(object(), Interface)

        self.assertEqual(len(list(comp.registeredUtilities())), 1)

        class IFoo(Interface):
            pass

        comp.registerUtility(object(), IFoo)
        self.assertEqual(len(list(comp.registeredUtilities())), 2)

    def test_registerUtility_changes_object_identity_before(self):
        # If a subclass changes the identity of the _utility_registrations,
        # the cache is updated and the right thing still happens.
        class CompThatChangesAfter2Reg(self._getTargetClass()):
            reg_count = 0
            def registerUtility(self, *args):
                self.reg_count += 1
                if self.reg_count == 2:
                    self._utility_registrations = dict(self._utility_registrations)

                super(CompThatChangesAfter2Reg, self).registerUtility(*args)

        comp = CompThatChangesAfter2Reg()
        comp.registerUtility(object(), Interface)

        self.assertEqual(len(list(comp.registeredUtilities())), 1)

        class IFoo(Interface):
            pass

        comp.registerUtility(object(), IFoo)
        self.assertEqual(len(list(comp.registeredUtilities())), 2)


        class IBar(Interface):
            pass

        comp.registerUtility(object(), IBar)
        self.assertEqual(len(list(comp.registeredUtilities())), 3)


    def test_unregisterUtility_neither_factory_nor_component_nor_provided(self):
        comp = self._makeOne()
        self.assertRaises(TypeError, comp.unregisterUtility,
                          component=None, provided=None, factory=None)

    def test_unregisterUtility_both_factory_and_component(self):
        def _factory():
            raise NotImplementedError()
        _to_reg = object()
        comp = self._makeOne()
        self.assertRaises(TypeError, comp.unregisterUtility,
                          component=_to_reg, factory=_factory)

    def test_unregisterUtility_w_component_miss(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _name = u'name'
        _to_reg = object()
        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterUtility(_to_reg, ifoo, _name)
        self.assertFalse(unreg)
        self.assertFalse(_events)

    def test_unregisterUtility_w_component(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import UtilityRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _name = u'name'
        _to_reg = object()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, _name)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterUtility(_to_reg, ifoo, _name)
        self.assertTrue(unreg)
        self.assertFalse(comp.utilities._adapters) # all erased
        self.assertFalse((ifoo, _name) in comp._utility_registrations)
        self.assertFalse(comp.utilities._subscribers)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, UtilityRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.component is _to_reg)
        self.assertTrue(event.object.factory is None)

    def test_unregisterUtility_w_factory(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import UtilityRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name = u'name'
        _to_reg = object()
        def _factory():
            return _to_reg
        comp = self._makeOne()
        comp.registerUtility(None, ifoo, _name, _info, factory=_factory)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterUtility(None, ifoo, _name, factory=_factory)
        self.assertTrue(unreg)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, UtilityRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.component is _to_reg)
        self.assertTrue(event.object.factory is _factory)

    def test_unregisterUtility_wo_explicit_provided(self):
        from zope.interface.declarations import directlyProvides
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import UtilityRegistration

        class IFoo(InterfaceClass):
            pass
        class Foo(object):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name = u'name'
        _to_reg = Foo()
        directlyProvides(_to_reg, ifoo)
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, _name, _info)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterUtility(_to_reg, None, _name)
        self.assertTrue(unreg)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, UtilityRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.component is _to_reg)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is None)

    def test_unregisterUtility_wo_component_or_factory(self):
        from zope.interface.declarations import directlyProvides
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import UtilityRegistration

        class IFoo(InterfaceClass):
            pass
        class Foo(object):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name = u'name'
        _to_reg = Foo()
        directlyProvides(_to_reg, ifoo)
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, _name, _info)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            # Just pass the interface / name
            unreg = comp.unregisterUtility(provided=ifoo, name=_name)
        self.assertTrue(unreg)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, UtilityRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.component is _to_reg)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is None)

    def test_unregisterUtility_w_existing_subscr(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name1 = u'name1'
        _name2 = u'name2'
        _to_reg = object()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, _name1, _info)
        comp.registerUtility(_to_reg, ifoo, _name2, _info)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.unregisterUtility(_to_reg, ifoo, _name2)
        self.assertEqual(comp.utilities._subscribers[0][ifoo][''], (_to_reg,))

    def test_unregisterUtility_w_existing_subscr_non_hashable(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name1 = u'name1'
        _name2 = u'name2'
        _to_reg = dict()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, _name1, _info)
        comp.registerUtility(_to_reg, ifoo, _name2, _info)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.unregisterUtility(_to_reg, ifoo, _name2)
        self.assertEqual(comp.utilities._subscribers[0][ifoo][''], (_to_reg,))

    def test_unregisterUtility_w_existing_subscr_non_hashable_fresh_cache(self):
        # We correctly populate the cache of registrations if it has gone away
        # (for example, the Components was unpickled)
        from zope.interface.declarations import InterfaceClass
        from zope.interface.registry import _UtilityRegistrations

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name1 = u'name1'
        _name2 = u'name2'
        _to_reg = dict()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, _name1, _info)
        comp.registerUtility(_to_reg, ifoo, _name2, _info)

        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.unregisterUtility(_to_reg, ifoo, _name2)
        self.assertEqual(comp.utilities._subscribers[0][ifoo][''], (_to_reg,))

    def test_unregisterUtility_w_existing_subscr_non_hashable_reinitted(self):
        # We correctly populate the cache of registrations if the base objects change
        # out from under us
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name1 = u'name1'
        _name2 = u'name2'
        _to_reg = dict()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, _name1, _info)
        comp.registerUtility(_to_reg, ifoo, _name2, _info)

        # zope.component.testing does this
        comp.__init__('base')

        comp.registerUtility(_to_reg, ifoo, _name2, _info)

        _monkey, _events = self._wrapEvents()
        with _monkey:
            # Nothing to do, but we don't break either
            comp.unregisterUtility(_to_reg, ifoo, _name2)
        self.assertEqual(0, len(comp.utilities._subscribers))

    def test_unregisterUtility_w_existing_subscr_other_component(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name1 = u'name1'
        _name2 = u'name2'
        _other_reg = object()
        _to_reg = object()
        comp = self._makeOne()
        comp.registerUtility(_other_reg, ifoo, _name1, _info)
        comp.registerUtility(_to_reg, ifoo, _name2, _info)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.unregisterUtility(_to_reg, ifoo, _name2)
        self.assertEqual(comp.utilities._subscribers[0][ifoo][''],
                         (_other_reg,))

    def test_unregisterUtility_w_existing_subscr_other_component_mixed_hash(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name1 = u'name1'
        _name2 = u'name2'
        # First register something hashable
        _other_reg = object()
        # Then it transfers to something unhashable
        _to_reg = dict()
        comp = self._makeOne()
        comp.registerUtility(_other_reg, ifoo, _name1, _info)
        comp.registerUtility(_to_reg, ifoo, _name2, _info)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.unregisterUtility(_to_reg, ifoo, _name2)
        self.assertEqual(comp.utilities._subscribers[0][ifoo][''],
                         (_other_reg,))

    def test_registeredUtilities_empty(self):
        comp = self._makeOne()
        self.assertEqual(list(comp.registeredUtilities()), [])

    def test_registeredUtilities_notempty(self):
        from zope.interface.declarations import InterfaceClass

        from zope.interface.registry import UtilityRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name1 = u'name1'
        _name2 = u'name2'
        _to_reg = object()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, _name1, _info)
        comp.registerUtility(_to_reg, ifoo, _name2, _info)
        reg = sorted(comp.registeredUtilities(), key=lambda r: r.name)
        self.assertEqual(len(reg), 2)
        self.assertTrue(isinstance(reg[0], UtilityRegistration))
        self.assertTrue(reg[0].registry is comp)
        self.assertTrue(reg[0].provided is ifoo)
        self.assertTrue(reg[0].name is _name1)
        self.assertTrue(reg[0].component is _to_reg)
        self.assertTrue(reg[0].info is _info)
        self.assertTrue(reg[0].factory is None)
        self.assertTrue(isinstance(reg[1], UtilityRegistration))
        self.assertTrue(reg[1].registry is comp)
        self.assertTrue(reg[1].provided is ifoo)
        self.assertTrue(reg[1].name is _name2)
        self.assertTrue(reg[1].component is _to_reg)
        self.assertTrue(reg[1].info is _info)
        self.assertTrue(reg[1].factory is None)

    def test_queryUtility_miss_no_default(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        self.assertTrue(comp.queryUtility(ifoo) is None)

    def test_queryUtility_miss_w_default(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        _default = object()
        self.assertTrue(comp.queryUtility(ifoo, default=_default) is _default)

    def test_queryUtility_hit(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _to_reg = object()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo)
        self.assertTrue(comp.queryUtility(ifoo) is _to_reg)

    def test_getUtility_miss(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import ComponentLookupError
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        self.assertRaises(ComponentLookupError, comp.getUtility, ifoo)

    def test_getUtility_hit(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _to_reg = object()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo)
        self.assertTrue(comp.getUtility(ifoo) is _to_reg)

    def test_getUtilitiesFor_miss(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        self.assertEqual(list(comp.getUtilitiesFor(ifoo)), [])

    def test_getUtilitiesFor_hit(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _name1 = u'name1'
        _name2 = u'name2'
        _to_reg = object()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, name=_name1)
        comp.registerUtility(_to_reg, ifoo, name=_name2)
        self.assertEqual(sorted(comp.getUtilitiesFor(ifoo)),
                         [(_name1, _to_reg), (_name2, _to_reg)])

    def test_getAllUtilitiesRegisteredFor_miss(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        self.assertEqual(list(comp.getAllUtilitiesRegisteredFor(ifoo)), [])

    def test_getAllUtilitiesRegisteredFor_hit(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _name1 = u'name1'
        _name2 = u'name2'
        _to_reg = object()
        comp = self._makeOne()
        comp.registerUtility(_to_reg, ifoo, name=_name1)
        comp.registerUtility(_to_reg, ifoo, name=_name2)
        self.assertEqual(list(comp.getAllUtilitiesRegisteredFor(ifoo)),
                         [_to_reg])

    def test_registerAdapter_with_component_name(self):
        from zope.interface.declarations import named, InterfaceClass


        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')

        @named(u'foo')
        class Foo(object):
            pass
        _info = u'info'

        comp = self._makeOne()
        comp.registerAdapter(Foo, (ibar,), ifoo, info=_info)

        self.assertEqual(
            comp._adapter_registrations[(ibar,), ifoo, u'foo'],
            (Foo, _info))

    def test_registerAdapter_w_explicit_provided_and_required(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Registered
        from zope.interface.registry import AdapterRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        _info = u'info'
        _name = u'name'

        def _factory(context):
            raise NotImplementedError()
        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerAdapter(_factory, (ibar,), ifoo, _name, _info)
        self.assertTrue(comp.adapters._adapters[1][ibar][ifoo][_name]
                        is _factory)
        self.assertEqual(comp._adapter_registrations[(ibar,), ifoo, _name],
                         (_factory, _info))
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, AdapterRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is _factory)

    def test_registerAdapter_no_provided_available(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass

        ibar = IFoo('IBar')
        _info = u'info'
        _name = u'name'

        class _Factory(object):
            pass

        comp = self._makeOne()
        self.assertRaises(TypeError, comp.registerAdapter, _Factory, (ibar,),
                          name=_name, info=_info)

    def test_registerAdapter_wo_explicit_provided(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        from zope.interface.interfaces import Registered
        from zope.interface.registry import AdapterRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        _info = u'info'
        _name = u'name'
        _to_reg = object()

        @implementer(ifoo)
        class _Factory(object):
            pass

        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerAdapter(_Factory, (ibar,), name=_name, info=_info)
        self.assertTrue(comp.adapters._adapters[1][ibar][ifoo][_name]
                        is _Factory)
        self.assertEqual(comp._adapter_registrations[(ibar,), ifoo, _name],
                         (_Factory, _info))
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, AdapterRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is _Factory)

    def test_registerAdapter_no_required_available(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')

        _info = u'info'
        _name = u'name'
        class _Factory(object):
           pass

        comp = self._makeOne()
        self.assertRaises(TypeError, comp.registerAdapter, _Factory,
                          provided=ifoo, name=_name, info=_info)

    def test_registerAdapter_w_invalid_required(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        _info = u'info'
        _name = u'name'
        class _Factory(object):
            pass
        comp = self._makeOne()
        self.assertRaises(TypeError, comp.registerAdapter, _Factory,
                          ibar, provided=ifoo, name=_name, info=_info)

    def test_registerAdapter_w_required_containing_None(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interface import Interface
        from zope.interface.interfaces import Registered
        from zope.interface.registry import AdapterRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _name = u'name'
        class _Factory(object):
            pass
        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerAdapter(_Factory, [None], provided=ifoo,
                                 name=_name, info=_info)
        self.assertTrue(comp.adapters._adapters[1][Interface][ifoo][_name]
                        is _Factory)
        self.assertEqual(comp._adapter_registrations[(Interface,), ifoo, _name],
                         (_Factory, _info))
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, AdapterRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (Interface,))
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is _Factory)

    def test_registerAdapter_w_required_containing_class(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        from zope.interface.declarations import implementedBy
        from zope.interface.interfaces import Registered
        from zope.interface.registry import AdapterRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        _info = u'info'
        _name = u'name'
        class _Factory(object):
            pass

        @implementer(ibar)
        class _Context(object):
            pass
        _ctx_impl = implementedBy(_Context)
        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerAdapter(_Factory, [_Context], provided=ifoo,
                                 name=_name, info=_info)
        self.assertTrue(comp.adapters._adapters[1][_ctx_impl][ifoo][_name]
                        is _Factory)
        self.assertEqual(comp._adapter_registrations[(_ctx_impl,), ifoo, _name],
                         (_Factory, _info))
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, AdapterRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (_ctx_impl,))
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is _Factory)

    def test_registerAdapter_w_required_containing_junk(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')

        _info = u'info'
        _name = u'name'
        class _Factory(object):
            pass
        comp = self._makeOne()
        self.assertRaises(TypeError, comp.registerAdapter, _Factory, [object()],
                          provided=ifoo, name=_name, info=_info)

    def test_registerAdapter_wo_explicit_required(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Registered
        from zope.interface.registry import AdapterRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        _info = u'info'
        _name = u'name'
        class _Factory(object):
            __component_adapts__ = (ibar,)

        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerAdapter(_Factory, provided=ifoo, name=_name,
                                 info=_info)
        self.assertTrue(comp.adapters._adapters[1][ibar][ifoo][_name]
                        is _Factory)
        self.assertEqual(comp._adapter_registrations[(ibar,), ifoo, _name],
                         (_Factory, _info))
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, AdapterRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertTrue(event.object.name is _name)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is _Factory)

    def test_registerAdapter_wo_event(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        _info = u'info'
        _name = u'name'

        def _factory(context):
            raise NotImplementedError()
        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerAdapter(_factory, (ibar,), ifoo, _name, _info,
                                 event=False)
        self.assertEqual(len(_events), 0)

    def test_unregisterAdapter_neither_factory_nor_provided(self):
        comp = self._makeOne()
        self.assertRaises(TypeError, comp.unregisterAdapter,
                          factory=None, provided=None)

    def test_unregisterAdapter_neither_factory_nor_required(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        self.assertRaises(TypeError, comp.unregisterAdapter,
                          factory=None, provided=ifoo, required=None)

    def test_unregisterAdapter_miss(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        class _Factory(object):
            pass

        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterAdapter(_Factory, (ibar,), ifoo)
        self.assertFalse(unreg)

    def test_unregisterAdapter_hit_w_explicit_provided_and_required(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import AdapterRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        class _Factory(object):
            pass

        comp = self._makeOne()
        comp.registerAdapter(_Factory, (ibar,), ifoo)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterAdapter(_Factory, (ibar,), ifoo)
        self.assertTrue(unreg)
        self.assertFalse(comp.adapters._adapters)
        self.assertFalse(comp._adapter_registrations)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, AdapterRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertEqual(event.object.name, '')
        self.assertEqual(event.object.info, '')
        self.assertTrue(event.object.factory is _Factory)

    def test_unregisterAdapter_wo_explicit_provided(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import AdapterRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        @implementer(ifoo)
        class _Factory(object):
            pass

        comp = self._makeOne()
        comp.registerAdapter(_Factory, (ibar,), ifoo)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterAdapter(_Factory, (ibar,))
        self.assertTrue(unreg)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, AdapterRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertEqual(event.object.name, '')
        self.assertEqual(event.object.info, '')
        self.assertTrue(event.object.factory is _Factory)

    def test_unregisterAdapter_wo_explicit_required(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import AdapterRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        class _Factory(object):
            __component_adapts__ = (ibar,)

        comp = self._makeOne()
        comp.registerAdapter(_Factory, (ibar,), ifoo)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterAdapter(_Factory, provided=ifoo)
        self.assertTrue(unreg)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, AdapterRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertEqual(event.object.name, '')
        self.assertEqual(event.object.info, '')
        self.assertTrue(event.object.factory is _Factory)

    def test_registeredAdapters_empty(self):
        comp = self._makeOne()
        self.assertEqual(list(comp.registeredAdapters()), [])

    def test_registeredAdapters_notempty(self):
        from zope.interface.declarations import InterfaceClass

        from zope.interface.registry import AdapterRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IFoo')
        _info = u'info'
        _name1 = u'name1'
        _name2 = u'name2'
        class _Factory(object):
            pass

        comp = self._makeOne()
        comp.registerAdapter(_Factory, (ibar,), ifoo, _name1, _info)
        comp.registerAdapter(_Factory, (ibar,), ifoo, _name2, _info)
        reg = sorted(comp.registeredAdapters(), key=lambda r: r.name)
        self.assertEqual(len(reg), 2)
        self.assertTrue(isinstance(reg[0], AdapterRegistration))
        self.assertTrue(reg[0].registry is comp)
        self.assertTrue(reg[0].provided is ifoo)
        self.assertEqual(reg[0].required, (ibar,))
        self.assertTrue(reg[0].name is _name1)
        self.assertTrue(reg[0].info is _info)
        self.assertTrue(reg[0].factory is _Factory)
        self.assertTrue(isinstance(reg[1], AdapterRegistration))
        self.assertTrue(reg[1].registry is comp)
        self.assertTrue(reg[1].provided is ifoo)
        self.assertEqual(reg[1].required, (ibar,))
        self.assertTrue(reg[1].name is _name2)
        self.assertTrue(reg[1].info is _info)
        self.assertTrue(reg[1].factory is _Factory)

    def test_queryAdapter_miss_no_default(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        _context = object()
        self.assertTrue(comp.queryAdapter(_context, ifoo) is None)

    def test_queryAdapter_miss_w_default(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        _context = object()
        _default = object()
        self.assertTrue(
            comp.queryAdapter(_context, ifoo, default=_default) is _default)

    def test_queryAdapter_hit(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        class _Factory(object):
            def __init__(self, context):
                self.context = context
        @implementer(ibar)
        class _Context(object):
            pass
        _context = _Context()
        comp = self._makeOne()
        comp.registerAdapter(_Factory, (ibar,), ifoo)
        adapter = comp.queryAdapter(_context, ifoo)
        self.assertTrue(isinstance(adapter, _Factory))
        self.assertTrue(adapter.context is _context)

    def test_getAdapter_miss(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        from zope.interface.interfaces import ComponentLookupError
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        @implementer(ibar)
        class _Context(object):
            pass
        _context = _Context()
        comp = self._makeOne()
        self.assertRaises(ComponentLookupError,
                          comp.getAdapter, _context, ifoo)

    def test_getAdapter_hit(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        class _Factory(object):
            def __init__(self, context):
                self.context = context
        @implementer(ibar)
        class _Context(object):
            pass
        _context = _Context()
        comp = self._makeOne()
        comp.registerAdapter(_Factory, (ibar,), ifoo)
        adapter = comp.getAdapter(_context, ifoo)
        self.assertIsInstance(adapter, _Factory)
        self.assertIs(adapter.context, _context)

    def test_getAdapter_hit_super(self):
        from zope.interface import Interface
        from zope.interface.declarations import implementer

        class IBase(Interface):
            pass

        class IDerived(IBase):
            pass

        class IFoo(Interface):
            pass

        @implementer(IBase)
        class Base(object):
            pass

        @implementer(IDerived)
        class Derived(Base):
            pass

        class AdapterBase(object):
            def __init__(self, context):
                self.context = context

        class AdapterDerived(object):
            def __init__(self, context):
                self.context = context

        comp = self._makeOne()
        comp.registerAdapter(AdapterDerived, (IDerived,), IFoo)
        comp.registerAdapter(AdapterBase, (IBase,), IFoo)
        self._should_not_change(comp)

        derived = Derived()
        adapter = comp.getAdapter(derived, IFoo)
        self.assertIsInstance(adapter, AdapterDerived)
        self.assertIs(adapter.context, derived)

        supe = super(Derived, derived)
        adapter = comp.getAdapter(supe, IFoo)
        self.assertIsInstance(adapter, AdapterBase)
        self.assertIs(adapter.context, derived)

    def test_getAdapter_hit_super_when_parent_implements_interface_diamond(self):
        from zope.interface import Interface
        from zope.interface.declarations import implementer

        class IBase(Interface):
            pass

        class IDerived(IBase):
            pass

        class IFoo(Interface):
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

        class AdapterBase(object):
            def __init__(self, context):
                self.context = context

        class AdapterDerived(object):
            def __init__(self, context):
                self.context = context

        comp = self._makeOne()
        comp.registerAdapter(AdapterDerived, (IDerived,), IFoo)
        comp.registerAdapter(AdapterBase, (IBase,), IFoo)
        self._should_not_change(comp)

        derived = Derived()
        adapter = comp.getAdapter(derived, IFoo)
        self.assertIsInstance(adapter, AdapterDerived)
        self.assertIs(adapter.context, derived)

        supe = super(Derived, derived)
        adapter = comp.getAdapter(supe, IFoo)
        self.assertIsInstance(adapter, AdapterBase)
        self.assertIs(adapter.context, derived)

    def test_queryMultiAdapter_miss(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        ibaz = IFoo('IBaz')
        @implementer(ibar)
        class _Context1(object):
            pass
        @implementer(ibaz)
        class _Context2(object):
            pass
        _context1 = _Context1()
        _context2 = _Context2()
        comp = self._makeOne()
        self.assertEqual(comp.queryMultiAdapter((_context1, _context2), ifoo),
                         None)

    def test_queryMultiAdapter_miss_w_default(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        ibaz = IFoo('IBaz')
        @implementer(ibar)
        class _Context1(object):
            pass
        @implementer(ibaz)
        class _Context2(object):
            pass
        _context1 = _Context1()
        _context2 = _Context2()
        _default = object()
        comp = self._makeOne()
        self.assertTrue(
            comp.queryMultiAdapter((_context1, _context2), ifoo,
                                   default=_default) is _default)

    def test_queryMultiAdapter_hit(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        ibaz = IFoo('IBaz')
        @implementer(ibar)
        class _Context1(object):
            pass
        @implementer(ibaz)
        class _Context2(object):
            pass
        _context1 = _Context1()
        _context2 = _Context2()
        class _Factory(object):
            def __init__(self, context1, context2):
                self.context = context1, context2
        comp = self._makeOne()
        comp.registerAdapter(_Factory, (ibar, ibaz), ifoo)
        adapter = comp.queryMultiAdapter((_context1, _context2), ifoo)
        self.assertTrue(isinstance(adapter, _Factory))
        self.assertEqual(adapter.context, (_context1, _context2))

    def test_getMultiAdapter_miss(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        from zope.interface.interfaces import ComponentLookupError
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        ibaz = IFoo('IBaz')
        @implementer(ibar)
        class _Context1(object):
            pass
        @implementer(ibaz)
        class _Context2(object):
            pass
        _context1 = _Context1()
        _context2 = _Context2()
        comp = self._makeOne()
        self.assertRaises(ComponentLookupError,
                          comp.getMultiAdapter, (_context1, _context2), ifoo)

    def test_getMultiAdapter_hit(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        ibaz = IFoo('IBaz')
        @implementer(ibar)
        class _Context1(object):
            pass
        @implementer(ibaz)
        class _Context2(object):
            pass
        _context1 = _Context1()
        _context2 = _Context2()
        class _Factory(object):
            def __init__(self, context1, context2):
                self.context = context1, context2
        comp = self._makeOne()
        comp.registerAdapter(_Factory, (ibar, ibaz), ifoo)
        adapter = comp.getMultiAdapter((_context1, _context2), ifoo)
        self.assertTrue(isinstance(adapter, _Factory))
        self.assertEqual(adapter.context, (_context1, _context2))

    def _should_not_change(self, comp):
        # Be sure that none of the underlying structures
        # get told that they have changed during this process
        # because that invalidates caches.
        def no_changes(*args):
            self.fail("Nothing should get changed")
        comp.changed = no_changes
        comp.adapters.changed = no_changes
        comp.adapters._v_lookup.changed = no_changes

    def test_getMultiAdapter_hit_super(self):
        from zope.interface import Interface
        from zope.interface.declarations import implementer

        class IBase(Interface):
            pass

        class IDerived(IBase):
            pass

        class IFoo(Interface):
            pass

        @implementer(IBase)
        class Base(object):
            pass

        @implementer(IDerived)
        class Derived(Base):
            pass

        class AdapterBase(object):
            def __init__(self, context1, context2):
                self.context1 = context1
                self.context2 = context2

        class AdapterDerived(AdapterBase):
            pass

        comp = self._makeOne()
        comp.registerAdapter(AdapterDerived, (IDerived, IDerived), IFoo)
        comp.registerAdapter(AdapterBase, (IBase, IDerived), IFoo)
        self._should_not_change(comp)

        derived = Derived()
        adapter = comp.getMultiAdapter((derived, derived), IFoo)
        self.assertIsInstance(adapter, AdapterDerived)
        self.assertIs(adapter.context1, derived)
        self.assertIs(adapter.context2, derived)

        supe = super(Derived, derived)
        adapter = comp.getMultiAdapter((supe, derived), IFoo)
        self.assertIsInstance(adapter, AdapterBase)
        self.assertNotIsInstance(adapter, AdapterDerived)
        self.assertIs(adapter.context1, derived)
        self.assertIs(adapter.context2, derived)

    def test_getAdapters_empty(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        ibaz = IFoo('IBaz')
        @implementer(ibar)
        class _Context1(object):
            pass
        @implementer(ibaz)
        class _Context2(object):
            pass
        _context1 = _Context1()
        _context2 = _Context2()
        comp = self._makeOne()
        self.assertEqual(
            list(comp.getAdapters((_context1, _context2), ifoo)), [])

    def test_getAdapters_factory_returns_None(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        ibaz = IFoo('IBaz')
        @implementer(ibar)
        class _Context1(object):
            pass
        @implementer(ibaz)
        class _Context2(object):
            pass
        _context1 = _Context1()
        _context2 = _Context2()
        comp = self._makeOne()
        _called_with = []
        def _side_effect_only(context1, context2):
            _called_with.append((context1, context2))
            return None
        comp.registerAdapter(_side_effect_only, (ibar, ibaz), ifoo)
        self.assertEqual(
            list(comp.getAdapters((_context1, _context2), ifoo)), [])
        self.assertEqual(_called_with, [(_context1, _context2)])

    def test_getAdapters_non_empty(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        ibaz = IFoo('IBaz')
        @implementer(ibar)
        class _Context1(object):
            pass
        @implementer(ibaz)
        class _Context2(object):
            pass
        _context1 = _Context1()
        _context2 = _Context2()
        class _Factory1(object):
            def __init__(self, context1, context2):
                self.context = context1, context2
        class _Factory2(object):
            def __init__(self, context1, context2):
                self.context = context1, context2
        _name1 = u'name1'
        _name2 = u'name2'
        comp = self._makeOne()
        comp.registerAdapter(_Factory1, (ibar, ibaz), ifoo, name=_name1)
        comp.registerAdapter(_Factory2, (ibar, ibaz), ifoo, name=_name2)
        found = sorted(comp.getAdapters((_context1, _context2), ifoo))
        self.assertEqual(len(found), 2)
        self.assertEqual(found[0][0], _name1)
        self.assertTrue(isinstance(found[0][1], _Factory1))
        self.assertEqual(found[1][0], _name2)
        self.assertTrue(isinstance(found[1][1], _Factory2))

    def test_registerSubscriptionAdapter_w_nonblank_name(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        _name = u'name'
        _info = u'info'
        def _factory(context):
            raise NotImplementedError()

        comp = self._makeOne()
        self.assertRaises(TypeError, comp.registerSubscriptionAdapter,
                          _factory, (ibar,), ifoo, _name, _info)

    def test_registerSubscriptionAdapter_w_explicit_provided_and_required(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Registered
        from zope.interface.registry import SubscriptionRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        _blank = u''
        _info = u'info'
        def _factory(context):
            raise NotImplementedError()
        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerSubscriptionAdapter(_factory, (ibar,), ifoo,
                                             info=_info)
        reg = comp.adapters._subscribers[1][ibar][ifoo][_blank]
        self.assertEqual(len(reg), 1)
        self.assertTrue(reg[0] is _factory)
        self.assertEqual(comp._subscription_registrations,
                         [((ibar,), ifoo, _blank, _factory, _info)])
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, SubscriptionRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertEqual(event.object.name, _blank)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is _factory)

    def test_registerSubscriptionAdapter_wo_explicit_provided(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        from zope.interface.interfaces import Registered
        from zope.interface.registry import SubscriptionRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        _info = u'info'
        _blank = u''

        @implementer(ifoo)
        class _Factory(object):
            pass

        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerSubscriptionAdapter(_Factory, (ibar,), info=_info)
        reg = comp.adapters._subscribers[1][ibar][ifoo][_blank]
        self.assertEqual(len(reg), 1)
        self.assertTrue(reg[0] is _Factory)
        self.assertEqual(comp._subscription_registrations,
                         [((ibar,), ifoo, _blank, _Factory, _info)])
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, SubscriptionRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertEqual(event.object.name, _blank)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is _Factory)

    def test_registerSubscriptionAdapter_wo_explicit_required(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Registered
        from zope.interface.registry import SubscriptionRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        _info = u'info'
        _blank = u''
        class _Factory(object):
            __component_adapts__ = (ibar,)

        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerSubscriptionAdapter(
                    _Factory, provided=ifoo, info=_info)
        reg = comp.adapters._subscribers[1][ibar][ifoo][_blank]
        self.assertEqual(len(reg), 1)
        self.assertTrue(reg[0] is _Factory)
        self.assertEqual(comp._subscription_registrations,
                         [((ibar,), ifoo, _blank, _Factory, _info)])
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, SubscriptionRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertEqual(event.object.name, _blank)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is _Factory)

    def test_registerSubscriptionAdapter_wo_event(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        _blank = u''
        _info = u'info'

        def _factory(context):
            raise NotImplementedError()

        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerSubscriptionAdapter(_factory, (ibar,), ifoo,
                                             info=_info, event=False)
        self.assertEqual(len(_events), 0)

    def test_registeredSubscriptionAdapters_empty(self):
        comp = self._makeOne()
        self.assertEqual(list(comp.registeredSubscriptionAdapters()), [])

    def test_registeredSubscriptionAdapters_notempty(self):
        from zope.interface.declarations import InterfaceClass

        from zope.interface.registry import SubscriptionRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IFoo')
        _info = u'info'
        _blank = u''
        class _Factory(object):
            pass

        comp = self._makeOne()
        comp.registerSubscriptionAdapter(_Factory, (ibar,), ifoo, info=_info)
        comp.registerSubscriptionAdapter(_Factory, (ibar,), ifoo, info=_info)
        reg = list(comp.registeredSubscriptionAdapters())
        self.assertEqual(len(reg), 2)
        self.assertTrue(isinstance(reg[0], SubscriptionRegistration))
        self.assertTrue(reg[0].registry is comp)
        self.assertTrue(reg[0].provided is ifoo)
        self.assertEqual(reg[0].required, (ibar,))
        self.assertEqual(reg[0].name, _blank)
        self.assertTrue(reg[0].info is _info)
        self.assertTrue(reg[0].factory is _Factory)
        self.assertTrue(isinstance(reg[1], SubscriptionRegistration))
        self.assertTrue(reg[1].registry is comp)
        self.assertTrue(reg[1].provided is ifoo)
        self.assertEqual(reg[1].required, (ibar,))
        self.assertEqual(reg[1].name, _blank)
        self.assertTrue(reg[1].info is _info)
        self.assertTrue(reg[1].factory is _Factory)

    def test_unregisterSubscriptionAdapter_w_nonblank_name(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        _nonblank = u'nonblank'
        comp = self._makeOne()
        self.assertRaises(TypeError, comp.unregisterSubscriptionAdapter,
                          required=ifoo, provided=ibar, name=_nonblank)

    def test_unregisterSubscriptionAdapter_neither_factory_nor_provided(self):
        comp = self._makeOne()
        self.assertRaises(TypeError, comp.unregisterSubscriptionAdapter,
                          factory=None, provided=None)

    def test_unregisterSubscriptionAdapter_neither_factory_nor_required(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        self.assertRaises(TypeError, comp.unregisterSubscriptionAdapter,
                          factory=None, provided=ifoo, required=None)

    def test_unregisterSubscriptionAdapter_miss(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        class _Factory(object):
            pass

        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterSubscriptionAdapter(_Factory, (ibar,), ifoo)
        self.assertFalse(unreg)
        self.assertFalse(_events)

    def test_unregisterSubscriptionAdapter_hit_wo_factory(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import SubscriptionRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        class _Factory(object):
            pass

        comp = self._makeOne()
        comp.registerSubscriptionAdapter(_Factory, (ibar,), ifoo)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterSubscriptionAdapter(None, (ibar,), ifoo)
        self.assertTrue(unreg)
        self.assertFalse(comp.adapters._subscribers)
        self.assertFalse(comp._subscription_registrations)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, SubscriptionRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertEqual(event.object.name, '')
        self.assertEqual(event.object.info, '')
        self.assertTrue(event.object.factory is None)

    def test_unregisterSubscriptionAdapter_hit_w_factory(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import SubscriptionRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        class _Factory(object):
            pass

        comp = self._makeOne()
        comp.registerSubscriptionAdapter(_Factory, (ibar,), ifoo)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterSubscriptionAdapter(_Factory, (ibar,), ifoo)
        self.assertTrue(unreg)
        self.assertFalse(comp.adapters._subscribers)
        self.assertFalse(comp._subscription_registrations)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, SubscriptionRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertEqual(event.object.name, '')
        self.assertEqual(event.object.info, '')
        self.assertTrue(event.object.factory is _Factory)

    def test_unregisterSubscriptionAdapter_wo_explicit_provided(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import SubscriptionRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        @implementer(ifoo)
        class _Factory(object):
            pass

        comp = self._makeOne()
        comp.registerSubscriptionAdapter(_Factory, (ibar,), ifoo)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterSubscriptionAdapter(_Factory, (ibar,))
        self.assertTrue(unreg)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, SubscriptionRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertEqual(event.object.name, '')
        self.assertEqual(event.object.info, '')
        self.assertTrue(event.object.factory is _Factory)

    def test_unregisterSubscriptionAdapter_wo_explicit_required(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import SubscriptionRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        class _Factory(object):
            __component_adapts__ = (ibar,)

        comp = self._makeOne()
        comp.registerSubscriptionAdapter(_Factory, (ibar,), ifoo)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterSubscriptionAdapter(_Factory, provided=ifoo)
        self.assertTrue(unreg)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, SubscriptionRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertTrue(event.object.provided is ifoo)
        self.assertEqual(event.object.required, (ibar,))
        self.assertEqual(event.object.name, '')
        self.assertEqual(event.object.info, '')
        self.assertTrue(event.object.factory is _Factory)

    def test_subscribers_empty(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        comp = self._makeOne()
        @implementer(ibar)
        class Bar(object):
            pass
        bar = Bar()
        self.assertEqual(list(comp.subscribers((bar,), ifoo)), [])

    def test_subscribers_non_empty(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        class _Factory(object):
            __component_adapts__ = (ibar,)
            def __init__(self, context):
                self._context = context
        class _Derived(_Factory):
            pass
        comp = self._makeOne()
        comp.registerSubscriptionAdapter(_Factory, (ibar,), ifoo)
        comp.registerSubscriptionAdapter(_Derived, (ibar,), ifoo)
        @implementer(ibar)
        class Bar(object):
            pass
        bar = Bar()
        subscribers = comp.subscribers((bar,), ifoo)
        def _klassname(x):
            return x.__class__.__name__
        subscribers = sorted(subscribers, key=_klassname)
        self.assertEqual(len(subscribers), 2)
        self.assertTrue(isinstance(subscribers[0], _Derived))
        self.assertTrue(isinstance(subscribers[1], _Factory))

    def test_registerHandler_w_nonblank_name(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _nonblank = u'nonblank'
        comp = self._makeOne()
        def _factory(context):
            raise NotImplementedError()

        self.assertRaises(TypeError, comp.registerHandler, _factory,
                          required=ifoo, name=_nonblank)

    def test_registerHandler_w_explicit_required(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Registered
        from zope.interface.registry import HandlerRegistration

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _blank = u''
        _info = u'info'
        def _factory(context):
            raise NotImplementedError()

        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerHandler(_factory, (ifoo,), info=_info)
        reg = comp.adapters._subscribers[1][ifoo][None][_blank]
        self.assertEqual(len(reg), 1)
        self.assertTrue(reg[0] is _factory)
        self.assertEqual(comp._handler_registrations,
                         [((ifoo,), _blank, _factory, _info)])
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Registered))
        self.assertTrue(isinstance(event.object, HandlerRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertEqual(event.object.required, (ifoo,))
        self.assertEqual(event.object.name, _blank)
        self.assertTrue(event.object.info is _info)
        self.assertTrue(event.object.factory is _factory)

    def test_registerHandler_wo_explicit_required_no_event(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _info = u'info'
        _blank = u''
        class _Factory(object):
            __component_adapts__ = (ifoo,)
            pass

        comp = self._makeOne()
        _monkey, _events = self._wrapEvents()
        with _monkey:
            comp.registerHandler(_Factory, info=_info, event=False)
        reg = comp.adapters._subscribers[1][ifoo][None][_blank]
        self.assertEqual(len(reg), 1)
        self.assertTrue(reg[0] is _Factory)
        self.assertEqual(comp._handler_registrations,
                         [((ifoo,), _blank, _Factory, _info)])
        self.assertEqual(len(_events), 0)

    def test_registeredHandlers_empty(self):
        comp = self._makeOne()
        self.assertFalse(list(comp.registeredHandlers()))

    def test_registeredHandlers_non_empty(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.registry import HandlerRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        def _factory1(context):
            raise NotImplementedError()
        def _factory2(context):
            raise NotImplementedError()
        comp = self._makeOne()
        comp.registerHandler(_factory1, (ifoo,))
        comp.registerHandler(_factory2, (ifoo,))
        def _factory_name(x):
            return x.factory.__code__.co_name
        subscribers = sorted(comp.registeredHandlers(), key=_factory_name)
        self.assertEqual(len(subscribers), 2)
        self.assertTrue(isinstance(subscribers[0], HandlerRegistration))
        self.assertEqual(subscribers[0].required, (ifoo,))
        self.assertEqual(subscribers[0].name, '')
        self.assertEqual(subscribers[0].factory, _factory1)
        self.assertEqual(subscribers[0].info, '')
        self.assertTrue(isinstance(subscribers[1], HandlerRegistration))
        self.assertEqual(subscribers[1].required, (ifoo,))
        self.assertEqual(subscribers[1].name, '')
        self.assertEqual(subscribers[1].factory, _factory2)
        self.assertEqual(subscribers[1].info, '')

    def test_unregisterHandler_w_nonblank_name(self):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _nonblank = u'nonblank'
        comp = self._makeOne()
        self.assertRaises(TypeError, comp.unregisterHandler,
                          required=(ifoo,), name=_nonblank)

    def test_unregisterHandler_neither_factory_nor_required(self):
        comp = self._makeOne()
        self.assertRaises(TypeError, comp.unregisterHandler)

    def test_unregisterHandler_miss(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        unreg = comp.unregisterHandler(required=(ifoo,))
        self.assertFalse(unreg)

    def test_unregisterHandler_hit_w_factory_and_explicit_provided(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import HandlerRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        def _factory(context):
            raise NotImplementedError()
        comp = self._makeOne()
        comp.registerHandler(_factory, (ifoo,))
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterHandler(_factory, (ifoo,))
        self.assertTrue(unreg)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, HandlerRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertEqual(event.object.required, (ifoo,))
        self.assertEqual(event.object.name, '')
        self.assertTrue(event.object.factory is _factory)

    def test_unregisterHandler_hit_w_only_explicit_provided(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import HandlerRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        def _factory(context):
            raise NotImplementedError()
        comp = self._makeOne()
        comp.registerHandler(_factory, (ifoo,))
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterHandler(required=(ifoo,))
        self.assertTrue(unreg)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, HandlerRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertEqual(event.object.required, (ifoo,))
        self.assertEqual(event.object.name, '')
        self.assertTrue(event.object.factory is None)

    def test_unregisterHandler_wo_explicit_required(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.interfaces import Unregistered
        from zope.interface.registry import HandlerRegistration
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        class _Factory(object):
            __component_adapts__ = (ifoo,)

        comp = self._makeOne()
        comp.registerHandler(_Factory)
        _monkey, _events = self._wrapEvents()
        with _monkey:
            unreg = comp.unregisterHandler(_Factory)
        self.assertTrue(unreg)
        self.assertEqual(len(_events), 1)
        args, kw = _events[0]
        event, = args
        self.assertEqual(kw, {})
        self.assertTrue(isinstance(event, Unregistered))
        self.assertTrue(isinstance(event.object, HandlerRegistration))
        self.assertTrue(event.object.registry is comp)
        self.assertEqual(event.object.required, (ifoo,))
        self.assertEqual(event.object.name, '')
        self.assertEqual(event.object.info, '')
        self.assertTrue(event.object.factory is _Factory)

    def test_handle_empty(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        comp = self._makeOne()
        @implementer(ifoo)
        class Bar(object):
            pass
        bar = Bar()
        comp.handle((bar,)) # doesn't raise

    def test_handle_non_empty(self):
        from zope.interface.declarations import InterfaceClass
        from zope.interface.declarations import implementer
        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        _called_1 = []
        def _factory_1(context):
                _called_1.append(context)
        _called_2 = []
        def _factory_2(context):
                _called_2.append(context)
        comp = self._makeOne()
        comp.registerHandler(_factory_1, (ifoo,))
        comp.registerHandler(_factory_2, (ifoo,))
        @implementer(ifoo)
        class Bar(object):
            pass
        bar = Bar()
        comp.handle(bar)
        self.assertEqual(_called_1, [bar])
        self.assertEqual(_called_2, [bar])


class UnhashableComponentsTests(ComponentsTests):

    def _getTargetClass(self):
        # Mimic what pyramid does to create an unhashable
        # registry
        class Components(super(UnhashableComponentsTests, self)._getTargetClass(), dict):
            pass
        return Components

# Test _getUtilityProvided, _getAdapterProvided, _getAdapterRequired via their
# callers (Component.registerUtility, Component.registerAdapter).


class UtilityRegistrationTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.registry import UtilityRegistration
        return UtilityRegistration

    def _makeOne(self, component=None, factory=None):
        from zope.interface.declarations import InterfaceClass

        class InterfaceClassSubclass(InterfaceClass):
            pass

        ifoo = InterfaceClassSubclass('IFoo')
        class _Registry(object):
            def __repr__(self):
                return '_REGISTRY'
        registry = _Registry()
        name = u'name'
        doc = 'DOCSTRING'
        klass = self._getTargetClass()
        return (klass(registry, ifoo, name, component, doc, factory),
                registry,
                name,
               )

    def test_class_conforms_to_IUtilityRegistration(self):
        from zope.interface.verify import verifyClass
        from zope.interface.interfaces import IUtilityRegistration
        verifyClass(IUtilityRegistration, self._getTargetClass())

    def test_instance_conforms_to_IUtilityRegistration(self):
        from zope.interface.verify import verifyObject
        from zope.interface.interfaces import IUtilityRegistration
        ur, _, _ =  self._makeOne()
        verifyObject(IUtilityRegistration, ur)

    def test___repr__(self):
        class _Component(object):
            __name__ = 'TEST'
        _component = _Component()
        ur, _registry, _name = self._makeOne(_component)
        self.assertEqual(repr(ur),
            "UtilityRegistration(_REGISTRY, IFoo, %r, TEST, None, 'DOCSTRING')"
                            % (_name))

    def test___repr___provided_wo_name(self):
        class _Component(object):
            def __repr__(self):
                return 'TEST'
        _component = _Component()
        ur, _registry, _name = self._makeOne(_component)
        ur.provided = object()
        self.assertEqual(repr(ur),
            "UtilityRegistration(_REGISTRY, None, %r, TEST, None, 'DOCSTRING')"
                            % (_name))

    def test___repr___component_wo_name(self):
        class _Component(object):
            def __repr__(self):
                return 'TEST'
        _component = _Component()
        ur, _registry, _name = self._makeOne(_component)
        ur.provided = object()
        self.assertEqual(repr(ur),
            "UtilityRegistration(_REGISTRY, None, %r, TEST, None, 'DOCSTRING')"
                            % (_name))

    def test___hash__(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        self.assertEqual(ur.__hash__(), id(ur))

    def test___eq___identity(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        self.assertTrue(ur == ur)

    def test___eq___hit(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        ur2, _, _ = self._makeOne(_component)
        self.assertTrue(ur == ur2)

    def test___eq___miss(self):
        _component = object()
        _component2 = object()
        ur, _registry, _name = self._makeOne(_component)
        ur2, _, _ = self._makeOne(_component2)
        self.assertFalse(ur == ur2)

    def test___ne___identity(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        self.assertFalse(ur != ur)

    def test___ne___hit(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        ur2, _, _ = self._makeOne(_component)
        self.assertFalse(ur != ur2)

    def test___ne___miss(self):
        _component = object()
        _component2 = object()
        ur, _registry, _name = self._makeOne(_component)
        ur2, _, _ = self._makeOne(_component2)
        self.assertTrue(ur != ur2)

    def test___lt___identity(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        self.assertFalse(ur < ur)

    def test___lt___hit(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        ur2, _, _ = self._makeOne(_component)
        self.assertFalse(ur < ur2)

    def test___lt___miss(self):
        _component = object()
        _component2 = object()
        ur, _registry, _name = self._makeOne(_component)
        ur2, _, _ = self._makeOne(_component2)
        ur2.name = _name + '2'
        self.assertTrue(ur < ur2)

    def test___le___identity(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        self.assertTrue(ur <= ur)

    def test___le___hit(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        ur2, _, _ = self._makeOne(_component)
        self.assertTrue(ur <= ur2)

    def test___le___miss(self):
        _component = object()
        _component2 = object()
        ur, _registry, _name = self._makeOne(_component)
        ur2, _, _ = self._makeOne(_component2)
        ur2.name = _name + '2'
        self.assertTrue(ur <= ur2)

    def test___gt___identity(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        self.assertFalse(ur > ur)

    def test___gt___hit(self):
        _component = object()
        _component2 = object()
        ur, _registry, _name = self._makeOne(_component)
        ur2, _, _ = self._makeOne(_component2)
        ur2.name = _name + '2'
        self.assertTrue(ur2 > ur)

    def test___gt___miss(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        ur2, _, _ = self._makeOne(_component)
        self.assertFalse(ur2 > ur)

    def test___ge___identity(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        self.assertTrue(ur >= ur)

    def test___ge___miss(self):
        _component = object()
        _component2 = object()
        ur, _registry, _name = self._makeOne(_component)
        ur2, _, _ = self._makeOne(_component2)
        ur2.name = _name + '2'
        self.assertFalse(ur >= ur2)

    def test___ge___hit(self):
        _component = object()
        ur, _registry, _name = self._makeOne(_component)
        ur2, _, _ = self._makeOne(_component)
        ur2.name = _name + '2'
        self.assertTrue(ur2 >= ur)


class AdapterRegistrationTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.registry import AdapterRegistration
        return AdapterRegistration

    def _makeOne(self, component=None):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        class _Registry(object):
            def __repr__(self):
                return '_REGISTRY'
        registry = _Registry()
        name = u'name'
        doc = 'DOCSTRING'
        klass = self._getTargetClass()
        return (klass(registry, (ibar,), ifoo, name, component, doc),
                registry,
                name,
               )

    def test_class_conforms_to_IAdapterRegistration(self):
        from zope.interface.verify import verifyClass
        from zope.interface.interfaces import IAdapterRegistration
        verifyClass(IAdapterRegistration, self._getTargetClass())

    def test_instance_conforms_to_IAdapterRegistration(self):
        from zope.interface.verify import verifyObject
        from zope.interface.interfaces import IAdapterRegistration
        ar, _, _ =  self._makeOne()
        verifyObject(IAdapterRegistration, ar)

    def test___repr__(self):
        class _Component(object):
            __name__ = 'TEST'
        _component = _Component()
        ar, _registry, _name = self._makeOne(_component)
        self.assertEqual(repr(ar),
            ("AdapterRegistration(_REGISTRY, [IBar], IFoo, %r, TEST, "
           + "'DOCSTRING')") % (_name))

    def test___repr___provided_wo_name(self):
        class _Component(object):
            def __repr__(self):
                return 'TEST'
        _component = _Component()
        ar, _registry, _name = self._makeOne(_component)
        ar.provided = object()
        self.assertEqual(repr(ar),
            ("AdapterRegistration(_REGISTRY, [IBar], None, %r, TEST, "
           + "'DOCSTRING')") % (_name))

    def test___repr___component_wo_name(self):
        class _Component(object):
            def __repr__(self):
                return 'TEST'
        _component = _Component()
        ar, _registry, _name = self._makeOne(_component)
        ar.provided = object()
        self.assertEqual(repr(ar),
            ("AdapterRegistration(_REGISTRY, [IBar], None, %r, TEST, "
           + "'DOCSTRING')") % (_name))

    def test___hash__(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        self.assertEqual(ar.__hash__(), id(ar))

    def test___eq___identity(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        self.assertTrue(ar == ar)

    def test___eq___hit(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component)
        self.assertTrue(ar == ar2)

    def test___eq___miss(self):
        _component = object()
        _component2 = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component2)
        self.assertFalse(ar == ar2)

    def test___ne___identity(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        self.assertFalse(ar != ar)

    def test___ne___miss(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component)
        self.assertFalse(ar != ar2)

    def test___ne___hit_component(self):
        _component = object()
        _component2 = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component2)
        self.assertTrue(ar != ar2)

    def test___ne___hit_provided(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ibaz = IFoo('IBaz')
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component)
        ar2.provided = ibaz
        self.assertTrue(ar != ar2)

    def test___ne___hit_required(self):
        from zope.interface.declarations import InterfaceClass
        class IFoo(InterfaceClass):
            pass
        ibaz = IFoo('IBaz')
        _component = object()
        _component2 = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component2)
        ar2.required = (ibaz,)
        self.assertTrue(ar != ar2)

    def test___lt___identity(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        self.assertFalse(ar < ar)

    def test___lt___hit(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component)
        self.assertFalse(ar < ar2)

    def test___lt___miss(self):
        _component = object()
        _component2 = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component2)
        ar2.name = _name + '2'
        self.assertTrue(ar < ar2)

    def test___le___identity(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        self.assertTrue(ar <= ar)

    def test___le___hit(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component)
        self.assertTrue(ar <= ar2)

    def test___le___miss(self):
        _component = object()
        _component2 = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component2)
        ar2.name = _name + '2'
        self.assertTrue(ar <= ar2)

    def test___gt___identity(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        self.assertFalse(ar > ar)

    def test___gt___hit(self):
        _component = object()
        _component2 = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component2)
        ar2.name = _name + '2'
        self.assertTrue(ar2 > ar)

    def test___gt___miss(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component)
        self.assertFalse(ar2 > ar)

    def test___ge___identity(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        self.assertTrue(ar >= ar)

    def test___ge___miss(self):
        _component = object()
        _component2 = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component2)
        ar2.name = _name + '2'
        self.assertFalse(ar >= ar2)

    def test___ge___hit(self):
        _component = object()
        ar, _registry, _name = self._makeOne(_component)
        ar2, _, _ = self._makeOne(_component)
        ar2.name = _name + '2'
        self.assertTrue(ar2 >= ar)


class SubscriptionRegistrationTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.registry import SubscriptionRegistration
        return SubscriptionRegistration

    def _makeOne(self, component=None):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        ibar = IFoo('IBar')
        class _Registry(object):
            def __repr__(self): # pragma: no cover
                return '_REGISTRY'
        registry = _Registry()
        name = u'name'
        doc = 'DOCSTRING'
        klass = self._getTargetClass()
        return (klass(registry, (ibar,), ifoo, name, component, doc),
                registry,
                name,
               )

    def test_class_conforms_to_ISubscriptionAdapterRegistration(self):
        from zope.interface.verify import verifyClass
        from zope.interface.interfaces import ISubscriptionAdapterRegistration
        verifyClass(ISubscriptionAdapterRegistration, self._getTargetClass())

    def test_instance_conforms_to_ISubscriptionAdapterRegistration(self):
        from zope.interface.verify import verifyObject
        from zope.interface.interfaces import ISubscriptionAdapterRegistration
        sar, _, _ =  self._makeOne()
        verifyObject(ISubscriptionAdapterRegistration, sar)


class HandlerRegistrationTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.registry import HandlerRegistration
        return HandlerRegistration

    def _makeOne(self, component=None):
        from zope.interface.declarations import InterfaceClass

        class IFoo(InterfaceClass):
            pass
        ifoo = IFoo('IFoo')
        class _Registry(object):
            def __repr__(self):
                return '_REGISTRY'
        registry = _Registry()
        name = u'name'
        doc = 'DOCSTRING'
        klass = self._getTargetClass()
        return (klass(registry, (ifoo,), name, component, doc),
                registry,
                name,
               )

    def test_class_conforms_to_IHandlerRegistration(self):
        from zope.interface.verify import verifyClass
        from zope.interface.interfaces import IHandlerRegistration
        verifyClass(IHandlerRegistration, self._getTargetClass())

    def test_instance_conforms_to_IHandlerRegistration(self):
        from zope.interface.verify import verifyObject
        from zope.interface.interfaces import IHandlerRegistration
        hr, _, _ =  self._makeOne()
        verifyObject(IHandlerRegistration, hr)

    def test_properties(self):
        def _factory(context):
            raise NotImplementedError()
        hr, _, _ =  self._makeOne(_factory)
        self.assertTrue(hr.handler is _factory)
        self.assertTrue(hr.factory is hr.handler)
        self.assertTrue(hr.provided is None)

    def test___repr___factory_w_name(self):
        class _Factory(object):
            __name__ = 'TEST'
        hr, _registry, _name =  self._makeOne(_Factory())
        self.assertEqual(repr(hr),
            ("HandlerRegistration(_REGISTRY, [IFoo], %r, TEST, "
           + "'DOCSTRING')") % (_name))

    def test___repr___factory_wo_name(self):
        class _Factory(object):
            def __repr__(self):
                return 'TEST'
        hr, _registry, _name =  self._makeOne(_Factory())
        self.assertEqual(repr(hr),
            ("HandlerRegistration(_REGISTRY, [IFoo], %r, TEST, "
           + "'DOCSTRING')") % (_name))

class PersistentAdapterRegistry(VerifyingAdapterRegistry):

    def __getstate__(self):
        state = self.__dict__.copy()
        for k in list(state):
            if k in self._delegated or k.startswith('_v'):
                state.pop(k)
        state.pop('ro', None)
        return state

    def __setstate__(self, state):
        bases = state.pop('__bases__', ())
        self.__dict__.update(state)
        self._createLookup()
        self.__bases__ = bases
        self._v_lookup.changed(self)

class PersistentComponents(Components):
    # Mimic zope.component.persistentregistry.PersistentComponents:
    # we should be picklalable, but not persistent.Persistent ourself.

    def _init_registries(self):
        self.adapters = PersistentAdapterRegistry()
        self.utilities = PersistentAdapterRegistry()

class PersistentDictComponents(PersistentComponents, dict):
    # Like Pyramid's Registry, we subclass Components and dict
    pass


class PersistentComponentsDict(dict, PersistentComponents):
    # Like the above, but inheritance is flipped
    def __init__(self, name):
        dict.__init__(self)
        PersistentComponents.__init__(self, name)

class TestPersistentComponents(unittest.TestCase):

    def _makeOne(self):
        return PersistentComponents('test')

    def _check_equality_after_pickle(self, made):
        pass

    def test_pickles_empty(self):
        import pickle
        comp = self._makeOne()
        pickle.dumps(comp)
        comp2 = pickle.loads(pickle.dumps(comp))

        self.assertEqual(comp2.__name__, 'test')

    def test_pickles_with_utility_registration(self):
        import pickle
        comp = self._makeOne()
        utility = object()
        comp.registerUtility(
            utility,
            Interface)

        self.assertIs(utility,
                      comp.getUtility(Interface))

        comp2 = pickle.loads(pickle.dumps(comp))
        self.assertEqual(comp2.__name__, 'test')

        # The utility is still registered
        self.assertIsNotNone(comp2.getUtility(Interface))

        # We can register another one
        comp2.registerUtility(
            utility,
            Interface)
        self.assertIs(utility,
                      comp2.getUtility(Interface))

        self._check_equality_after_pickle(comp2)


class TestPersistentDictComponents(TestPersistentComponents):

    def _getTargetClass(self):
        return PersistentDictComponents

    def _makeOne(self):
        comp = self._getTargetClass()(name='test')
        comp['key'] = 42
        return comp

    def _check_equality_after_pickle(self, made):
        self.assertIn('key', made)
        self.assertEqual(made['key'], 42)

class TestPersistentComponentsDict(TestPersistentDictComponents):

    def _getTargetClass(self):
        return PersistentComponentsDict

class _Monkey(object):
    # context-manager for replacing module names in the scope of a test.
    def __init__(self, module, **kw):
        self.module = module
        self.to_restore = dict([(key, getattr(module, key)) for key in kw])
        for key, value in kw.items():
            setattr(module, key, value)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        for key, value in self.to_restore.items():
            setattr(self.module, key, value)
