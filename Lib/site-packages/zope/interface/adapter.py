##############################################################################
#
# Copyright (c) 2004 Zope Foundation and Contributors.
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
"""Adapter management
"""
import weakref

from zope.interface import implementer
from zope.interface import providedBy
from zope.interface import Interface
from zope.interface import ro
from zope.interface.interfaces import IAdapterRegistry

from zope.interface._compat import _normalize_name
from zope.interface._compat import STRING_TYPES
from zope.interface._compat import _use_c_impl

__all__ = [
    'AdapterRegistry',
    'VerifyingAdapterRegistry',
]

# ``tuple`` and ``list`` cooperate so that ``tuple([some list])``
# directly allocates and iterates at the C level without using a
# Python iterator. That's not the case for
# ``tuple(generator_expression)`` or ``tuple(map(func, it))``.
##
# 3.8
# ``tuple([t for t in range(10)])``      -> 610ns
# ``tuple(t for t in range(10))``        -> 696ns
# ``tuple(map(lambda t: t, range(10)))`` -> 881ns
##
# 2.7
# ``tuple([t fon t in range(10)])``      -> 625ns
# ``tuple(t for t in range(10))``        -> 665ns
# ``tuple(map(lambda t: t, range(10)))`` -> 958ns
#
# All three have substantial variance.

class BaseAdapterRegistry(object):

    # List of methods copied from lookup sub-objects:
    _delegated = ('lookup', 'queryMultiAdapter', 'lookup1', 'queryAdapter',
                  'adapter_hook', 'lookupAll', 'names',
                  'subscriptions', 'subscribers')

    # All registries maintain a generation that can be used by verifying
    # registries
    _generation = 0

    def __init__(self, bases=()):

        # The comments here could be improved. Possibly this bit needs
        # explaining in a separate document, as the comments here can
        # be quite confusing. /regebro

        # {order -> {required -> {provided -> {name -> value}}}}
        # Here "order" is actually an index in a list, "required" and
        # "provided" are interfaces, and "required" is really a nested
        # key.  So, for example:
        # for order == 0 (that is, self._adapters[0]), we have:
        #   {provided -> {name -> value}}
        # but for order == 2 (that is, self._adapters[2]), we have:
        #   {r1 -> {r2 -> {provided -> {name -> value}}}}
        #
        self._adapters = []

        # {order -> {required -> {provided -> {name -> [value]}}}}
        # where the remarks about adapters above apply
        self._subscribers = []

        # Set, with a reference count, keeping track of the interfaces
        # for which we have provided components:
        self._provided = {}

        # Create ``_v_lookup`` object to perform lookup.  We make this a
        # separate object to to make it easier to implement just the
        # lookup functionality in C.  This object keeps track of cache
        # invalidation data in two kinds of registries.

        #   Invalidating registries have caches that are invalidated
        #     when they or their base registies change.  An invalidating
        #     registry can only have invalidating registries as bases.
        #     See LookupBaseFallback below for the pertinent logic.

        #   Verifying registies can't rely on getting invalidation messages,
        #     so have to check the generations of base registries to determine
        #     if their cache data are current.  See VerifyingBasePy below
        #     for the pertinent object.
        self._createLookup()

        # Setting the bases causes the registries described above
        # to be initialized (self._setBases -> self.changed ->
        # self._v_lookup.changed).

        self.__bases__ = bases

    def _setBases(self, bases):
        self.__dict__['__bases__'] = bases
        self.ro = ro.ro(self)
        self.changed(self)

    __bases__ = property(lambda self: self.__dict__['__bases__'],
                         lambda self, bases: self._setBases(bases),
                         )

    def _createLookup(self):
        self._v_lookup = self.LookupClass(self)
        for name in self._delegated:
            self.__dict__[name] = getattr(self._v_lookup, name)

    def changed(self, originally_changed):
        self._generation += 1
        self._v_lookup.changed(originally_changed)

    def register(self, required, provided, name, value):
        if not isinstance(name, STRING_TYPES):
            raise ValueError('name is not a string')
        if value is None:
            self.unregister(required, provided, name, value)
            return

        required = tuple([_convert_None_to_Interface(r) for r in required])
        name = _normalize_name(name)
        order = len(required)
        byorder = self._adapters
        while len(byorder) <= order:
            byorder.append({})
        components = byorder[order]
        key = required + (provided,)

        for k in key:
            d = components.get(k)
            if d is None:
                d = {}
                components[k] = d
            components = d

        if components.get(name) is value:
            return

        components[name] = value

        n = self._provided.get(provided, 0) + 1
        self._provided[provided] = n
        if n == 1:
            self._v_lookup.add_extendor(provided)

        self.changed(self)

    def registered(self, required, provided, name=u''):
        required = tuple([_convert_None_to_Interface(r) for r in required])
        name = _normalize_name(name)
        order = len(required)
        byorder = self._adapters
        if len(byorder) <= order:
            return None

        components = byorder[order]
        key = required + (provided,)

        for k in key:
            d = components.get(k)
            if d is None:
                return None
            components = d

        return components.get(name)

    def unregister(self, required, provided, name, value=None):
        required = tuple([_convert_None_to_Interface(r) for r in required])
        order = len(required)
        byorder = self._adapters
        if order >= len(byorder):
            return False
        components = byorder[order]
        key = required + (provided,)

        # Keep track of how we got to `components`:
        lookups = []
        for k in key:
            d = components.get(k)
            if d is None:
                return
            lookups.append((components, k))
            components = d

        old = components.get(name)
        if old is None:
            return
        if (value is not None) and (old is not value):
            return

        del components[name]
        if not components:
            # Clean out empty containers, since we don't want our keys
            # to reference global objects (interfaces) unnecessarily.
            # This is often a problem when an interface is slated for
            # removal; a hold-over entry in the registry can make it
            # difficult to remove such interfaces.
            for comp, k in reversed(lookups):
                d = comp[k]
                if d:
                    break
                else:
                    del comp[k]
            while byorder and not byorder[-1]:
                del byorder[-1]
        n = self._provided[provided] - 1
        if n == 0:
            del self._provided[provided]
            self._v_lookup.remove_extendor(provided)
        else:
            self._provided[provided] = n

        self.changed(self)

    def subscribe(self, required, provided, value):
        required = tuple([_convert_None_to_Interface(r) for r in required])
        name = u''
        order = len(required)
        byorder = self._subscribers
        while len(byorder) <= order:
            byorder.append({})
        components = byorder[order]
        key = required + (provided,)

        for k in key:
            d = components.get(k)
            if d is None:
                d = {}
                components[k] = d
            components = d

        components[name] = components.get(name, ()) + (value, )

        if provided is not None:
            n = self._provided.get(provided, 0) + 1
            self._provided[provided] = n
            if n == 1:
                self._v_lookup.add_extendor(provided)

        self.changed(self)

    def unsubscribe(self, required, provided, value=None):
        required = tuple([_convert_None_to_Interface(r) for r in required])
        order = len(required)
        byorder = self._subscribers
        if order >= len(byorder):
            return
        components = byorder[order]
        key = required + (provided,)

        # Keep track of how we got to `components`:
        lookups = []
        for k in key:
            d = components.get(k)
            if d is None:
                return
            lookups.append((components, k))
            components = d

        old = components.get(u'')
        if not old:
            # this is belt-and-suspenders against the failure of cleanup below
            return  # pragma: no cover

        if value is None:
            new = ()
        else:
            new = tuple([v for v in old if v != value])

        if new == old:
            return

        if new:
            components[u''] = new
        else:
            # Instead of setting components[u''] = new, we clean out
            # empty containers, since we don't want our keys to
            # reference global objects (interfaces) unnecessarily.  This
            # is often a problem when an interface is slated for
            # removal; a hold-over entry in the registry can make it
            # difficult to remove such interfaces.
            del components[u'']
            for comp, k in reversed(lookups):
                d = comp[k]
                if d:
                    break
                else:
                    del comp[k]
            while byorder and not byorder[-1]:
                del byorder[-1]

        if provided is not None:
            n = self._provided[provided] + len(new) - len(old)
            if n == 0:
                del self._provided[provided]
                self._v_lookup.remove_extendor(provided)

        self.changed(self)

    # XXX hack to fake out twisted's use of a private api.  We need to get them
    # to use the new registed method.
    def get(self, _): # pragma: no cover
        class XXXTwistedFakeOut:
            selfImplied = {}
        return XXXTwistedFakeOut


_not_in_mapping = object()

@_use_c_impl
class LookupBase(object):

    def __init__(self):
        self._cache = {}
        self._mcache = {}
        self._scache = {}

    def changed(self, ignored=None):
        self._cache.clear()
        self._mcache.clear()
        self._scache.clear()

    def _getcache(self, provided, name):
        cache = self._cache.get(provided)
        if cache is None:
            cache = {}
            self._cache[provided] = cache
        if name:
            c = cache.get(name)
            if c is None:
                c = {}
                cache[name] = c
            cache = c
        return cache

    def lookup(self, required, provided, name=u'', default=None):
        if not isinstance(name, STRING_TYPES):
            raise ValueError('name is not a string')
        cache = self._getcache(provided, name)
        required = tuple(required)
        if len(required) == 1:
            result = cache.get(required[0], _not_in_mapping)
        else:
            result = cache.get(tuple(required), _not_in_mapping)

        if result is _not_in_mapping:
            result = self._uncached_lookup(required, provided, name)
            if len(required) == 1:
                cache[required[0]] = result
            else:
                cache[tuple(required)] = result

        if result is None:
            return default

        return result

    def lookup1(self, required, provided, name=u'', default=None):
        if not isinstance(name, STRING_TYPES):
            raise ValueError('name is not a string')
        cache = self._getcache(provided, name)
        result = cache.get(required, _not_in_mapping)
        if result is _not_in_mapping:
            return self.lookup((required, ), provided, name, default)

        if result is None:
            return default

        return result

    def queryAdapter(self, object, provided, name=u'', default=None):
        return self.adapter_hook(provided, object, name, default)

    def adapter_hook(self, provided, object, name=u'', default=None):
        if not isinstance(name, STRING_TYPES):
            raise ValueError('name is not a string')
        required = providedBy(object)
        cache = self._getcache(provided, name)
        factory = cache.get(required, _not_in_mapping)
        if factory is _not_in_mapping:
            factory = self.lookup((required, ), provided, name)

        if factory is not None:
            if isinstance(object, super):
                object = object.__self__
            result = factory(object)
            if result is not None:
                return result

        return default

    def lookupAll(self, required, provided):
        cache = self._mcache.get(provided)
        if cache is None:
            cache = {}
            self._mcache[provided] = cache

        required = tuple(required)
        result = cache.get(required, _not_in_mapping)
        if result is _not_in_mapping:
            result = self._uncached_lookupAll(required, provided)
            cache[required] = result

        return result


    def subscriptions(self, required, provided):
        cache = self._scache.get(provided)
        if cache is None:
            cache = {}
            self._scache[provided] = cache

        required = tuple(required)
        result = cache.get(required, _not_in_mapping)
        if result is _not_in_mapping:
            result = self._uncached_subscriptions(required, provided)
            cache[required] = result

        return result


@_use_c_impl
class VerifyingBase(LookupBaseFallback):
    # Mixin for lookups against registries which "chain" upwards, and
    # whose lookups invalidate their own caches whenever a parent registry
    # bumps its own '_generation' counter.  E.g., used by
    # zope.component.persistentregistry

    def changed(self, originally_changed):
        LookupBaseFallback.changed(self, originally_changed)
        self._verify_ro = self._registry.ro[1:]
        self._verify_generations = [r._generation for r in self._verify_ro]

    def _verify(self):
        if ([r._generation for r in self._verify_ro]
            != self._verify_generations):
            self.changed(None)

    def _getcache(self, provided, name):
        self._verify()
        return LookupBaseFallback._getcache(self, provided, name)

    def lookupAll(self, required, provided):
        self._verify()
        return LookupBaseFallback.lookupAll(self, required, provided)

    def subscriptions(self, required, provided):
        self._verify()
        return LookupBaseFallback.subscriptions(self, required, provided)


class AdapterLookupBase(object):

    def __init__(self, registry):
        self._registry = registry
        self._required = {}
        self.init_extendors()
        super(AdapterLookupBase, self).__init__()

    def changed(self, ignored=None):
        super(AdapterLookupBase, self).changed(None)
        for r in self._required.keys():
            r = r()
            if r is not None:
                r.unsubscribe(self)
        self._required.clear()


    # Extendors
    # ---------

    # When given an target interface for an adapter lookup, we need to consider
    # adapters for interfaces that extend the target interface.  This is
    # what the extendors dictionary is about.  It tells us all of the
    # interfaces that extend an interface for which there are adapters
    # registered.

    # We could separate this by order and name, thus reducing the
    # number of provided interfaces to search at run time.  The tradeoff,
    # however, is that we have to store more information.  For example,
    # if the same interface is provided for multiple names and if the
    # interface extends many interfaces, we'll have to keep track of
    # a fair bit of information for each name.  It's better to
    # be space efficient here and be time efficient in the cache
    # implementation.

    # TODO: add invalidation when a provided interface changes, in case
    # the interface's __iro__ has changed.  This is unlikely enough that
    # we'll take our chances for now.

    def init_extendors(self):
        self._extendors = {}
        for p in self._registry._provided:
            self.add_extendor(p)

    def add_extendor(self, provided):
        _extendors = self._extendors
        for i in provided.__iro__:
            extendors = _extendors.get(i, ())
            _extendors[i] = (
                [e for e in extendors if provided.isOrExtends(e)]
                +
                [provided]
                +
                [e for e in extendors if not provided.isOrExtends(e)]
                )

    def remove_extendor(self, provided):
        _extendors = self._extendors
        for i in provided.__iro__:
            _extendors[i] = [e for e in _extendors.get(i, ())
                             if e != provided]


    def _subscribe(self, *required):
        _refs = self._required
        for r in required:
            ref = r.weakref()
            if ref not in _refs:
                r.subscribe(self)
                _refs[ref] = 1

    def _uncached_lookup(self, required, provided, name=u''):
        required = tuple(required)
        result = None
        order = len(required)
        for registry in self._registry.ro:
            byorder = registry._adapters
            if order >= len(byorder):
                continue

            extendors = registry._v_lookup._extendors.get(provided)
            if not extendors:
                continue

            components = byorder[order]
            result = _lookup(components, required, extendors, name, 0,
                             order)
            if result is not None:
                break

        self._subscribe(*required)

        return result

    def queryMultiAdapter(self, objects, provided, name=u'', default=None):
        factory = self.lookup([providedBy(o) for o in objects], provided, name)
        if factory is None:
            return default

        result = factory(*[o.__self__ if isinstance(o, super) else o for o in objects])
        if result is None:
            return default

        return result

    def _uncached_lookupAll(self, required, provided):
        required = tuple(required)
        order = len(required)
        result = {}
        for registry in reversed(self._registry.ro):
            byorder = registry._adapters
            if order >= len(byorder):
                continue
            extendors = registry._v_lookup._extendors.get(provided)
            if not extendors:
                continue
            components = byorder[order]
            _lookupAll(components, required, extendors, result, 0, order)

        self._subscribe(*required)

        return tuple(result.items())

    def names(self, required, provided):
        return [c[0] for c in self.lookupAll(required, provided)]

    def _uncached_subscriptions(self, required, provided):
        required = tuple(required)
        order = len(required)
        result = []
        for registry in reversed(self._registry.ro):
            byorder = registry._subscribers
            if order >= len(byorder):
                continue

            if provided is None:
                extendors = (provided, )
            else:
                extendors = registry._v_lookup._extendors.get(provided)
                if extendors is None:
                    continue

            _subscriptions(byorder[order], required, extendors, u'',
                           result, 0, order)

        self._subscribe(*required)

        return result

    def subscribers(self, objects, provided):
        subscriptions = self.subscriptions([providedBy(o) for o in objects], provided)
        if provided is None:
            result = ()
            for subscription in subscriptions:
                subscription(*objects)
        else:
            result = []
            for subscription in subscriptions:
                subscriber = subscription(*objects)
                if subscriber is not None:
                    result.append(subscriber)
        return result

class AdapterLookup(AdapterLookupBase, LookupBase):
    pass

@implementer(IAdapterRegistry)
class AdapterRegistry(BaseAdapterRegistry):

    LookupClass = AdapterLookup

    def __init__(self, bases=()):
        # AdapterRegisties are invalidating registries, so
        # we need to keep track of our invalidating subregistries.
        self._v_subregistries = weakref.WeakKeyDictionary()

        super(AdapterRegistry, self).__init__(bases)

    def _addSubregistry(self, r):
        self._v_subregistries[r] = 1

    def _removeSubregistry(self, r):
        if r in self._v_subregistries:
            del self._v_subregistries[r]

    def _setBases(self, bases):
        old = self.__dict__.get('__bases__', ())
        for r in old:
            if r not in bases:
                r._removeSubregistry(self)
        for r in bases:
            if r not in old:
                r._addSubregistry(self)

        super(AdapterRegistry, self)._setBases(bases)

    def changed(self, originally_changed):
        super(AdapterRegistry, self).changed(originally_changed)

        for sub in self._v_subregistries.keys():
            sub.changed(originally_changed)


class VerifyingAdapterLookup(AdapterLookupBase, VerifyingBase):
    pass

@implementer(IAdapterRegistry)
class VerifyingAdapterRegistry(BaseAdapterRegistry):

    LookupClass = VerifyingAdapterLookup

def _convert_None_to_Interface(x):
    if x is None:
        return Interface
    else:
        return x

def _lookup(components, specs, provided, name, i, l):
    # this function is called very often.
    # The components.get in loops is executed 100 of 1000s times.
    # by loading get into a local variable the bytecode
    # "LOAD_FAST 0 (components)" in the loop can be eliminated.
    components_get = components.get
    if i < l:
        for spec in specs[i].__sro__:
            comps = components_get(spec)
            if comps:
                r = _lookup(comps, specs, provided, name, i+1, l)
                if r is not None:
                    return r
    else:
        for iface in provided:
            comps = components_get(iface)
            if comps:
                r = comps.get(name)
                if r is not None:
                    return r

    return None

def _lookupAll(components, specs, provided, result, i, l):
    components_get = components.get  # see _lookup above
    if i < l:
        for spec in reversed(specs[i].__sro__):
            comps = components_get(spec)
            if comps:
                _lookupAll(comps, specs, provided, result, i+1, l)
    else:
        for iface in reversed(provided):
            comps = components_get(iface)
            if comps:
                result.update(comps)

def _subscriptions(components, specs, provided, name, result, i, l):
    components_get = components.get  # see _lookup above
    if i < l:
        for spec in reversed(specs[i].__sro__):
            comps = components_get(spec)
            if comps:
                _subscriptions(comps, specs, provided, name, result, i+1, l)
    else:
        for iface in reversed(provided):
            comps = components_get(iface)
            if comps:
                comps = comps.get(name)
                if comps:
                    result.extend(comps)
