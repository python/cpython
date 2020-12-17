##############################################################################
#
# Copyright (c) 2006 Zope Foundation and Contributors.
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
"""Basic components support
"""
from collections import defaultdict

try:
    from zope.event import notify
except ImportError: # pragma: no cover
    def notify(*arg, **kw): pass

from zope.interface.interfaces import ISpecification
from zope.interface.interfaces import ComponentLookupError
from zope.interface.interfaces import IAdapterRegistration
from zope.interface.interfaces import IComponents
from zope.interface.interfaces import IHandlerRegistration
from zope.interface.interfaces import ISubscriptionAdapterRegistration
from zope.interface.interfaces import IUtilityRegistration
from zope.interface.interfaces import Registered
from zope.interface.interfaces import Unregistered

from zope.interface.interface import Interface
from zope.interface.declarations import implementedBy
from zope.interface.declarations import implementer
from zope.interface.declarations import implementer_only
from zope.interface.declarations import providedBy
from zope.interface.adapter import AdapterRegistry
from zope.interface._compat import CLASS_TYPES
from zope.interface._compat import STRING_TYPES

__all__ = [
    # Components is public API, but
    # the *Registration classes are just implementations
    # of public interfaces.
    'Components',
]

class _UnhashableComponentCounter(object):
    # defaultdict(int)-like object for unhashable components

    def __init__(self, otherdict):
        # [(component, count)]
        self._data = [item for item in otherdict.items()]

    def __getitem__(self, key):
        for component, count in self._data:
            if component == key:
                return count
        return 0

    def __setitem__(self, component, count):
        for i, data in enumerate(self._data):
            if data[0] == component:
                self._data[i] = component, count
                return
        self._data.append((component, count))

    def __delitem__(self, component):
        for i, data in enumerate(self._data):
            if data[0] == component:
                del self._data[i]
                return
        raise KeyError(component) # pragma: no cover

def _defaultdict_int():
    return defaultdict(int)

class _UtilityRegistrations(object):

    def __init__(self, utilities, utility_registrations):
        # {provided -> {component: count}}
        self._cache = defaultdict(_defaultdict_int)
        self._utilities = utilities
        self._utility_registrations = utility_registrations

        self.__populate_cache()

    def __populate_cache(self):
        for ((p, _), data) in iter(self._utility_registrations.items()):
            component = data[0]
            self.__cache_utility(p, component)

    def __cache_utility(self, provided, component):
        try:
            self._cache[provided][component] += 1
        except TypeError:
            # The component is not hashable, and we have a dict. Switch to a strategy
            # that doesn't use hashing.
            prov = self._cache[provided] = _UnhashableComponentCounter(self._cache[provided])
            prov[component] += 1

    def __uncache_utility(self, provided, component):
        provided = self._cache[provided]
        # It seems like this line could raise a TypeError if component isn't
        # hashable and we haven't yet switched to _UnhashableComponentCounter. However,
        # we can't actually get in that situation. In order to get here, we would
        # have had to cache the utility already which would have switched
        # the datastructure if needed.
        count = provided[component]
        count -= 1
        if count == 0:
            del provided[component]
        else:
            provided[component] = count
        return count > 0

    def _is_utility_subscribed(self, provided, component):
        try:
            return self._cache[provided][component] > 0
        except TypeError:
            # Not hashable and we're still using a dict
            return False

    def registerUtility(self, provided, name, component, info, factory):
        subscribed = self._is_utility_subscribed(provided, component)

        self._utility_registrations[(provided, name)] = component, info, factory
        self._utilities.register((), provided, name, component)

        if not subscribed:
            self._utilities.subscribe((), provided, component)

        self.__cache_utility(provided, component)

    def unregisterUtility(self, provided, name, component):
        del self._utility_registrations[(provided, name)]
        self._utilities.unregister((), provided, name)

        subscribed = self.__uncache_utility(provided, component)

        if not subscribed:
            self._utilities.unsubscribe((), provided, component)


@implementer(IComponents)
class Components(object):

    _v_utility_registrations_cache = None

    def __init__(self, name='', bases=()):
        # __init__ is used for test cleanup as well as initialization.
        # XXX add a separate API for test cleanup.
        assert isinstance(name, STRING_TYPES)
        self.__name__ = name
        self._init_registries()
        self._init_registrations()
        self.__bases__ = tuple(bases)
        self._v_utility_registrations_cache = None

    def __repr__(self):
        return "<%s %s>" % (self.__class__.__name__, self.__name__)

    def __reduce__(self):
        # Mimic what a persistent.Persistent object does and elide
        # _v_ attributes so that they don't get saved in ZODB.
        # This allows us to store things that cannot be pickled in such
        # attributes.
        reduction = super(Components, self).__reduce__()
        # (callable, args, state, listiter, dictiter)
        # We assume the state is always a dict; the last three items
        # are technically optional and can be missing or None.
        filtered_state = {k: v for k, v in reduction[2].items()
                          if not k.startswith('_v_')}
        reduction = list(reduction)
        reduction[2] = filtered_state
        return tuple(reduction)

    def _init_registries(self):
        # Subclasses have never been required to call this method
        # if they override it, merely to fill in these two attributes.
        self.adapters = AdapterRegistry()
        self.utilities = AdapterRegistry()

    def _init_registrations(self):
        self._utility_registrations = {}
        self._adapter_registrations = {}
        self._subscription_registrations = []
        self._handler_registrations = []

    @property
    def _utility_registrations_cache(self):
        # We use a _v_ attribute internally so that data aren't saved in ZODB,
        # because this object cannot be pickled.
        cache = self._v_utility_registrations_cache
        if (cache is None
            or cache._utilities is not self.utilities
            or cache._utility_registrations is not self._utility_registrations):
            cache = self._v_utility_registrations_cache = _UtilityRegistrations(
                self.utilities,
                self._utility_registrations)
        return cache

    def _getBases(self):
        # Subclasses might override
        return self.__dict__.get('__bases__', ())

    def _setBases(self, bases):
        # Subclasses might override
        self.adapters.__bases__ = tuple([
            base.adapters for base in bases])
        self.utilities.__bases__ = tuple([
            base.utilities for base in bases])
        self.__dict__['__bases__'] = tuple(bases)

    __bases__ = property(
        lambda self: self._getBases(),
        lambda self, bases: self._setBases(bases),
        )

    def registerUtility(self, component=None, provided=None, name=u'',
                        info=u'', event=True, factory=None):
        if factory:
            if component:
                raise TypeError("Can't specify factory and component.")
            component = factory()

        if provided is None:
            provided = _getUtilityProvided(component)

        if name == u'':
            name = _getName(component)

        reg = self._utility_registrations.get((provided, name))
        if reg is not None:
            if reg[:2] == (component, info):
                # already registered
                return
            self.unregisterUtility(reg[0], provided, name)

        self._utility_registrations_cache.registerUtility(
            provided, name, component, info, factory)

        if event:
            notify(Registered(
                UtilityRegistration(self, provided, name, component, info,
                                    factory)
                ))

    def unregisterUtility(self, component=None, provided=None, name=u'',
                          factory=None):
        if factory:
            if component:
                raise TypeError("Can't specify factory and component.")
            component = factory()

        if provided is None:
            if component is None:
                raise TypeError("Must specify one of component, factory and "
                                "provided")
            provided = _getUtilityProvided(component)

        old = self._utility_registrations.get((provided, name))
        if (old is None) or ((component is not None) and
                             (component != old[0])):
            return False

        if component is None:
            component = old[0]

        # Note that component is now the old thing registered
        self._utility_registrations_cache.unregisterUtility(
            provided, name, component)

        notify(Unregistered(
            UtilityRegistration(self, provided, name, component, *old[1:])
            ))

        return True

    def registeredUtilities(self):
        for ((provided, name), data
             ) in iter(self._utility_registrations.items()):
            yield UtilityRegistration(self, provided, name, *data)

    def queryUtility(self, provided, name=u'', default=None):
        return self.utilities.lookup((), provided, name, default)

    def getUtility(self, provided, name=u''):
        utility = self.utilities.lookup((), provided, name)
        if utility is None:
            raise ComponentLookupError(provided, name)
        return utility

    def getUtilitiesFor(self, interface):
        for name, utility in self.utilities.lookupAll((), interface):
            yield name, utility

    def getAllUtilitiesRegisteredFor(self, interface):
        return self.utilities.subscriptions((), interface)

    def registerAdapter(self, factory, required=None, provided=None,
                        name=u'', info=u'', event=True):
        if provided is None:
            provided = _getAdapterProvided(factory)
        required = _getAdapterRequired(factory, required)
        if name == u'':
            name = _getName(factory)
        self._adapter_registrations[(required, provided, name)
                                    ] = factory, info
        self.adapters.register(required, provided, name, factory)

        if event:
            notify(Registered(
                AdapterRegistration(self, required, provided, name,
                                    factory, info)
                ))


    def unregisterAdapter(self, factory=None,
                          required=None, provided=None, name=u'',
                          ):
        if provided is None:
            if factory is None:
                raise TypeError("Must specify one of factory and provided")
            provided = _getAdapterProvided(factory)

        if (required is None) and (factory is None):
            raise TypeError("Must specify one of factory and required")

        required = _getAdapterRequired(factory, required)
        old = self._adapter_registrations.get((required, provided, name))
        if (old is None) or ((factory is not None) and
                             (factory != old[0])):
            return False

        del self._adapter_registrations[(required, provided, name)]
        self.adapters.unregister(required, provided, name)

        notify(Unregistered(
            AdapterRegistration(self, required, provided, name,
                                *old)
            ))

        return True

    def registeredAdapters(self):
        for ((required, provided, name), (component, info)
             ) in iter(self._adapter_registrations.items()):
            yield AdapterRegistration(self, required, provided, name,
                                      component, info)

    def queryAdapter(self, object, interface, name=u'', default=None):
        return self.adapters.queryAdapter(object, interface, name, default)

    def getAdapter(self, object, interface, name=u''):
        adapter = self.adapters.queryAdapter(object, interface, name)
        if adapter is None:
            raise ComponentLookupError(object, interface, name)
        return adapter

    def queryMultiAdapter(self, objects, interface, name=u'',
                          default=None):
        return self.adapters.queryMultiAdapter(
            objects, interface, name, default)

    def getMultiAdapter(self, objects, interface, name=u''):
        adapter = self.adapters.queryMultiAdapter(objects, interface, name)
        if adapter is None:
            raise ComponentLookupError(objects, interface, name)
        return adapter

    def getAdapters(self, objects, provided):
        for name, factory in self.adapters.lookupAll(
            list(map(providedBy, objects)),
            provided):
            adapter = factory(*objects)
            if adapter is not None:
                yield name, adapter

    def registerSubscriptionAdapter(self,
                                    factory, required=None, provided=None,
                                    name=u'', info=u'',
                                    event=True):
        if name:
            raise TypeError("Named subscribers are not yet supported")
        if provided is None:
            provided = _getAdapterProvided(factory)
        required = _getAdapterRequired(factory, required)
        self._subscription_registrations.append(
            (required, provided, name, factory, info)
            )
        self.adapters.subscribe(required, provided, factory)

        if event:
            notify(Registered(
                SubscriptionRegistration(self, required, provided, name,
                                         factory, info)
                ))

    def registeredSubscriptionAdapters(self):
        for data in self._subscription_registrations:
            yield SubscriptionRegistration(self, *data)

    def unregisterSubscriptionAdapter(self, factory=None,
                          required=None, provided=None, name=u'',
                          ):
        if name:
            raise TypeError("Named subscribers are not yet supported")
        if provided is None:
            if factory is None:
                raise TypeError("Must specify one of factory and provided")
            provided = _getAdapterProvided(factory)

        if (required is None) and (factory is None):
            raise TypeError("Must specify one of factory and required")

        required = _getAdapterRequired(factory, required)

        if factory is None:
            new = [(r, p, n, f, i)
                   for (r, p, n, f, i)
                   in self._subscription_registrations
                   if not (r == required and p == provided)
                   ]
        else:
            new = [(r, p, n, f, i)
                   for (r, p, n, f, i)
                   in self._subscription_registrations
                   if not (r == required and p == provided and f == factory)
                   ]

        if len(new) == len(self._subscription_registrations):
            return False


        self._subscription_registrations[:] = new
        self.adapters.unsubscribe(required, provided, factory)

        notify(Unregistered(
            SubscriptionRegistration(self, required, provided, name,
                                     factory, '')
            ))

        return True

    def subscribers(self, objects, provided):
        return self.adapters.subscribers(objects, provided)

    def registerHandler(self,
                        factory, required=None,
                        name=u'', info=u'',
                        event=True):
        if name:
            raise TypeError("Named handlers are not yet supported")
        required = _getAdapterRequired(factory, required)
        self._handler_registrations.append(
            (required, name, factory, info)
            )
        self.adapters.subscribe(required, None, factory)

        if event:
            notify(Registered(
                HandlerRegistration(self, required, name, factory, info)
                ))

    def registeredHandlers(self):
        for data in self._handler_registrations:
            yield HandlerRegistration(self, *data)

    def unregisterHandler(self, factory=None, required=None, name=u''):
        if name:
            raise TypeError("Named subscribers are not yet supported")

        if (required is None) and (factory is None):
            raise TypeError("Must specify one of factory and required")

        required = _getAdapterRequired(factory, required)

        if factory is None:
            new = [(r, n, f, i)
                   for (r, n, f, i)
                   in self._handler_registrations
                   if r != required
                   ]
        else:
            new = [(r, n, f, i)
                   for (r, n, f, i)
                   in self._handler_registrations
                   if not (r == required and f == factory)
                   ]

        if len(new) == len(self._handler_registrations):
            return False

        self._handler_registrations[:] = new
        self.adapters.unsubscribe(required, None, factory)

        notify(Unregistered(
            HandlerRegistration(self, required, name, factory, '')
            ))

        return True

    def handle(self, *objects):
        self.adapters.subscribers(objects, None)


def _getName(component):
    try:
        return component.__component_name__
    except AttributeError:
        return u''

def _getUtilityProvided(component):
    provided = list(providedBy(component))
    if len(provided) == 1:
        return provided[0]
    raise TypeError(
        "The utility doesn't provide a single interface "
        "and no provided interface was specified.")

def _getAdapterProvided(factory):
    provided = list(implementedBy(factory))
    if len(provided) == 1:
        return provided[0]
    raise TypeError(
        "The adapter factory doesn't implement a single interface "
        "and no provided interface was specified.")

def _getAdapterRequired(factory, required):
    if required is None:
        try:
            required = factory.__component_adapts__
        except AttributeError:
            raise TypeError(
                "The adapter factory doesn't have a __component_adapts__ "
                "attribute and no required specifications were specified"
                )
    elif ISpecification.providedBy(required):
        raise TypeError("the required argument should be a list of "
                        "interfaces, not a single interface")

    result = []
    for r in required:
        if r is None:
            r = Interface
        elif not ISpecification.providedBy(r):
            if isinstance(r, CLASS_TYPES):
                r = implementedBy(r)
            else:
                raise TypeError("Required specification must be a "
                                "specification or class, not %r" % type(r)
                                )
        result.append(r)
    return tuple(result)


@implementer(IUtilityRegistration)
class UtilityRegistration(object):

    def __init__(self, registry, provided, name, component, doc, factory=None):
        (self.registry, self.provided, self.name, self.component, self.info,
         self.factory
         ) = registry, provided, name, component, doc, factory

    def __repr__(self):
        return '%s(%r, %s, %r, %s, %r, %r)' % (
                self.__class__.__name__,
                self.registry,
                getattr(self.provided, '__name__', None), self.name,
                getattr(self.component, '__name__', repr(self.component)),
                self.factory, self.info,
                )

    def __hash__(self):
        return id(self)

    def __eq__(self, other):
        return repr(self) == repr(other)

    def __ne__(self, other):
        return repr(self) != repr(other)

    def __lt__(self, other):
        return repr(self) < repr(other)

    def __le__(self, other):
        return repr(self) <= repr(other)

    def __gt__(self, other):
        return repr(self) > repr(other)

    def __ge__(self, other):
        return repr(self) >= repr(other)

@implementer(IAdapterRegistration)
class AdapterRegistration(object):

    def __init__(self, registry, required, provided, name, component, doc):
        (self.registry, self.required, self.provided, self.name,
         self.factory, self.info
         ) = registry, required, provided, name, component, doc

    def __repr__(self):
        return '%s(%r, %s, %s, %r, %s, %r)' % (
            self.__class__.__name__,
            self.registry,
            '[' + ", ".join([r.__name__ for r in self.required]) + ']',
            getattr(self.provided, '__name__', None), self.name,
            getattr(self.factory, '__name__', repr(self.factory)), self.info,
            )

    def __hash__(self):
        return id(self)

    def __eq__(self, other):
        return repr(self) == repr(other)

    def __ne__(self, other):
        return repr(self) != repr(other)

    def __lt__(self, other):
        return repr(self) < repr(other)

    def __le__(self, other):
        return repr(self) <= repr(other)

    def __gt__(self, other):
        return repr(self) > repr(other)

    def __ge__(self, other):
        return repr(self) >= repr(other)

@implementer_only(ISubscriptionAdapterRegistration)
class SubscriptionRegistration(AdapterRegistration):
    pass


@implementer_only(IHandlerRegistration)
class HandlerRegistration(AdapterRegistration):

    def __init__(self, registry, required, name, handler, doc):
        (self.registry, self.required, self.name, self.handler, self.info
         ) = registry, required, name, handler, doc

    @property
    def factory(self):
        return self.handler

    provided = None

    def __repr__(self):
        return '%s(%r, %s, %r, %s, %r)' % (
            self.__class__.__name__,
            self.registry,
            '[' + ", ".join([r.__name__ for r in self.required]) + ']',
            self.name,
            getattr(self.factory, '__name__', repr(self.factory)), self.info,
            )
