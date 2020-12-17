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
"""Interface object implementation
"""
# pylint:disable=protected-access
import sys
from types import MethodType
from types import FunctionType
import weakref

from zope.interface._compat import _use_c_impl
from zope.interface._compat import PYTHON2 as PY2
from zope.interface.exceptions import Invalid
from zope.interface.ro import ro as calculate_ro
from zope.interface import ro

__all__ = [
    # Most of the public API from this module is directly exported
    # from zope.interface. The only remaining public API intended to
    # be imported from here should be those few things documented as
    # such.
    'InterfaceClass',
    'Specification',
    'adapter_hooks',
]

CO_VARARGS = 4
CO_VARKEYWORDS = 8
# Put in the attrs dict of an interface by ``taggedValue`` and ``invariants``
TAGGED_DATA = '__interface_tagged_values__'
# Put in the attrs dict of an interface by ``interfacemethod``
INTERFACE_METHODS = '__interface_methods__'

_decorator_non_return = object()
_marker = object()



def invariant(call):
    f_locals = sys._getframe(1).f_locals
    tags = f_locals.setdefault(TAGGED_DATA, {})
    invariants = tags.setdefault('invariants', [])
    invariants.append(call)
    return _decorator_non_return


def taggedValue(key, value):
    """Attaches a tagged value to an interface at definition time."""
    f_locals = sys._getframe(1).f_locals
    tagged_values = f_locals.setdefault(TAGGED_DATA, {})
    tagged_values[key] = value
    return _decorator_non_return


class Element(object):
    """
    Default implementation of `zope.interface.interfaces.IElement`.
    """

    # We can't say this yet because we don't have enough
    # infrastructure in place.
    #
    #implements(IElement)

    def __init__(self, __name__, __doc__=''): # pylint:disable=redefined-builtin
        if not __doc__ and __name__.find(' ') >= 0:
            __doc__ = __name__
            __name__ = None

        self.__name__ = __name__
        self.__doc__ = __doc__
        # Tagged values are rare, especially on methods or attributes.
        # Deferring the allocation can save substantial memory.
        self.__tagged_values = None

    def getName(self):
        """ Returns the name of the object. """
        return self.__name__

    def getDoc(self):
        """ Returns the documentation for the object. """
        return self.__doc__

    ###
    # Tagged values.
    #
    # Direct tagged values are set only in this instance. Others
    # may be inherited (for those subclasses that have that concept).
    ###

    def getTaggedValue(self, tag):
        """ Returns the value associated with 'tag'. """
        if not self.__tagged_values:
            raise KeyError(tag)
        return self.__tagged_values[tag]

    def queryTaggedValue(self, tag, default=None):
        """ Returns the value associated with 'tag'. """
        return self.__tagged_values.get(tag, default) if self.__tagged_values else default

    def getTaggedValueTags(self):
        """ Returns a collection of all tags. """
        return self.__tagged_values.keys() if self.__tagged_values else ()

    def setTaggedValue(self, tag, value):
        """ Associates 'value' with 'key'. """
        if self.__tagged_values is None:
            self.__tagged_values = {}
        self.__tagged_values[tag] = value

    queryDirectTaggedValue = queryTaggedValue
    getDirectTaggedValue = getTaggedValue
    getDirectTaggedValueTags = getTaggedValueTags


SpecificationBasePy = object # filled by _use_c_impl.


@_use_c_impl
class SpecificationBase(object):
    # This object is the base of the inheritance hierarchy for ClassProvides:
    #
    # ClassProvides < ClassProvidesBase, Declaration
    # Declaration < Specification < SpecificationBase
    # ClassProvidesBase < SpecificationBase
    #
    # In order to have compatible instance layouts, we need to declare
    # the storage used by Specification and Declaration here (and
    # those classes must have ``__slots__ = ()``); fortunately this is
    # not a waste of space because those are the only two inheritance
    # trees. These all translate into tp_members in C.
    __slots__ = (
        # Things used here.
        '_implied',
        # Things used in Specification.
        '_dependents',
        '_bases',
        '_v_attrs',
        '__iro__',
        '__sro__',
        '__weakref__',
    )

    def providedBy(self, ob):
        """Is the interface implemented by an object
        """
        spec = providedBy(ob)
        return self in spec._implied

    def implementedBy(self, cls):
        """Test whether the specification is implemented by a class or factory.

        Raise TypeError if argument is neither a class nor a callable.
        """
        spec = implementedBy(cls)
        return self in spec._implied

    def isOrExtends(self, interface):
        """Is the interface the same as or extend the given interface
        """
        return interface in self._implied # pylint:disable=no-member

    __call__ = isOrExtends


class NameAndModuleComparisonMixin(object):
    # Internal use. Implement the basic sorting operators (but not (in)equality
    # or hashing). Subclasses must provide ``__name__`` and ``__module__``
    # attributes. Subclasses will be mutually comparable; but because equality
    # and hashing semantics are missing from this class, take care in how
    # you define those two attributes: If you stick with the default equality
    # and hashing (identity based) you should make sure that all possible ``__name__``
    # and ``__module__`` pairs are unique ACROSS ALL SUBCLASSES. (Actually, pretty
    # much the same thing goes if you define equality and hashing to be based on
    # those two attributes: they must still be consistent ACROSS ALL SUBCLASSES.)

    # pylint:disable=assigning-non-slot
    __slots__ = ()

    def _compare(self, other):
        """
        Compare *self* to *other* based on ``__name__`` and ``__module__``.

        Return 0 if they are equal, return 1 if *self* is
        greater than *other*, and return -1 if *self* is less than
        *other*.

        If *other* does not have ``__name__`` or ``__module__``, then
        return ``NotImplemented``.

        .. caution::
           This allows comparison to things well outside the type hierarchy,
           perhaps not symmetrically.

           For example, ``class Foo(object)`` and ``class Foo(Interface)``
           in the same file would compare equal, depending on the order of
           operands. Writing code like this by hand would be unusual, but it could
           happen with dynamic creation of types and interfaces.

        None is treated as a pseudo interface that implies the loosest
        contact possible, no contract. For that reason, all interfaces
        sort before None.
        """
        if other is self:
            return 0

        if other is None:
            return -1

        n1 = (self.__name__, self.__module__)
        try:
            n2 = (other.__name__, other.__module__)
        except AttributeError:
            return NotImplemented

        # This spelling works under Python3, which doesn't have cmp().
        return (n1 > n2) - (n1 < n2)

    def __lt__(self, other):
        c = self._compare(other)
        if c is NotImplemented:
            return c
        return c < 0

    def __le__(self, other):
        c = self._compare(other)
        if c is NotImplemented:
            return c
        return c <= 0

    def __gt__(self, other):
        c = self._compare(other)
        if c is NotImplemented:
            return c
        return c > 0

    def __ge__(self, other):
        c = self._compare(other)
        if c is NotImplemented:
            return c
        return c >= 0


@_use_c_impl
class InterfaceBase(NameAndModuleComparisonMixin, SpecificationBasePy):
    """Base class that wants to be replaced with a C base :)
    """

    __slots__ = (
        '__name__',
        '__ibmodule__',
        '_v_cached_hash',
    )

    def __init__(self, name=None, module=None):
        self.__name__ = name
        self.__ibmodule__ = module

    def _call_conform(self, conform):
        raise NotImplementedError

    @property
    def __module_property__(self):
        # This is for _InterfaceMetaClass
        return self.__ibmodule__

    def __call__(self, obj, alternate=_marker):
        """Adapt an object to the interface
        """
        try:
            conform = obj.__conform__
        except AttributeError:
            conform = None

        if conform is not None:
            adapter = self._call_conform(conform)
            if adapter is not None:
                return adapter

        adapter = self.__adapt__(obj)

        if adapter is not None:
            return adapter
        if alternate is not _marker:
            return alternate
        raise TypeError("Could not adapt", obj, self)

    def __adapt__(self, obj):
        """Adapt an object to the receiver
        """
        if self.providedBy(obj):
            return obj

        for hook in adapter_hooks:
            adapter = hook(self, obj)
            if adapter is not None:
                return adapter

        return None

    def __hash__(self):
        # pylint:disable=assigning-non-slot,attribute-defined-outside-init
        try:
            return self._v_cached_hash
        except AttributeError:
            self._v_cached_hash = hash((self.__name__, self.__module__))
        return self._v_cached_hash

    def __eq__(self, other):
        c = self._compare(other)
        if c is NotImplemented:
            return c
        return c == 0

    def __ne__(self, other):
        if other is self:
            return False

        c = self._compare(other)
        if c is NotImplemented:
            return c
        return c != 0

adapter_hooks = _use_c_impl([], 'adapter_hooks')


class Specification(SpecificationBase):
    """Specifications

    An interface specification is used to track interface declarations
    and component registrations.

    This class is a base class for both interfaces themselves and for
    interface specifications (declarations).

    Specifications are mutable.  If you reassign their bases, their
    relations with other specifications are adjusted accordingly.
    """
    __slots__ = ()

    # The root of all Specifications. This will be assigned `Interface`,
    # once it is defined.
    _ROOT = None

    # Copy some base class methods for speed
    isOrExtends = SpecificationBase.isOrExtends
    providedBy = SpecificationBase.providedBy

    def __init__(self, bases=()):
        # There are many leaf interfaces with no dependents,
        # and a few with very many. It's a heavily left-skewed
        # distribution. In a survey of Plone and Zope related packages
        # that loaded 2245 InterfaceClass objects and 2235 ClassProvides
        # instances, there were a total of 7000 Specification objects created.
        # 4700 had 0 dependents, 1400 had 1, 382 had 2 and so on. Only one
        # for <type> had 1664. So there's savings to be had deferring
        # the creation of dependents.
        self._dependents = None # type: weakref.WeakKeyDictionary
        self._bases = ()
        self._implied = {}
        self._v_attrs = None
        self.__iro__ = ()
        self.__sro__ = ()

        self.__bases__ = tuple(bases)

    @property
    def dependents(self):
        if self._dependents is None:
            self._dependents = weakref.WeakKeyDictionary()
        return self._dependents

    def subscribe(self, dependent):
        self._dependents[dependent] = self.dependents.get(dependent, 0) + 1

    def unsubscribe(self, dependent):
        try:
            n = self._dependents[dependent]
        except TypeError:
            raise KeyError(dependent)
        n -= 1
        if not n:
            del self.dependents[dependent]
        else:
            assert n > 0
            self.dependents[dependent] = n

    def __setBases(self, bases):
        # Remove ourselves as a dependent of our old bases
        for b in self.__bases__:
            b.unsubscribe(self)

        # Register ourselves as a dependent of our new bases
        self._bases = bases
        for b in bases:
            b.subscribe(self)

        self.changed(self)

    __bases__ = property(
        lambda self: self._bases,
        __setBases,
        )

    def _calculate_sro(self):
        """
        Calculate and return the resolution order for this object, using its ``__bases__``.

        Ensures that ``Interface`` is always the last (lowest priority) element.
        """
        # We'd like to make Interface the lowest priority as a
        # property of the resolution order algorithm. That almost
        # works out naturally, but it fails when class inheritance has
        # some bases that DO implement an interface, and some that DO
        # NOT. In such a mixed scenario, you wind up with a set of
        # bases to consider that look like this: [[..., Interface],
        # [..., object], ...]. Depending on the order if inheritance,
        # Interface can wind up before or after object, and that can
        # happen at any point in the tree, meaning Interface can wind
        # up somewhere in the middle of the order. Since Interface is
        # treated as something that everything winds up implementing
        # anyway (a catch-all for things like adapters), having it high up
        # the order is bad. It's also bad to have it at the end, just before
        # some concrete class: concrete classes should be HIGHER priority than
        # interfaces (because there's only one class, but many implementations).
        #
        # One technically nice way to fix this would be to have
        # ``implementedBy(object).__bases__ = (Interface,)``
        #
        # But: (1) That fails for old-style classes and (2) that causes
        # everything to appear to *explicitly* implement Interface, when up
        # to this point it's been an implicit virtual sort of relationship.
        #
        # So we force the issue by mutating the resolution order.

        # Note that we let C3 use pre-computed __sro__ for our bases.
        # This requires that by the time this method is invoked, our bases
        # have settled their SROs. Thus, ``changed()`` must first
        # update itself before telling its descendents of changes.
        sro = calculate_ro(self, base_mros={
            b: b.__sro__
            for b in self.__bases__
        })
        root = self._ROOT
        if root is not None and sro and sro[-1] is not root:
            # In one dataset of 1823 Interface objects, 1117 ClassProvides objects,
            # sro[-1] was root 4496 times, and only not root 118 times. So it's
            # probably worth checking.

            # Once we don't have to deal with old-style classes,
            # we can add a check and only do this if base_count > 1,
            # if we tweak the bootstrapping for ``<implementedBy object>``
            sro = [
                x
                for x in sro
                if x is not root
            ]
            sro.append(root)

        return sro

    def changed(self, originally_changed):
        """
        We, or something we depend on, have changed.

        By the time this is called, the things we depend on,
        such as our bases, should themselves be stable.
        """
        self._v_attrs = None

        implied = self._implied
        implied.clear()

        ancestors = self._calculate_sro()
        self.__sro__ = tuple(ancestors)
        self.__iro__ = tuple([ancestor for ancestor in ancestors
                              if isinstance(ancestor, InterfaceClass)
                              ])

        for ancestor in ancestors:
            # We directly imply our ancestors:
            implied[ancestor] = ()

        # Now, advise our dependents of change
        # (being careful not to create the WeakKeyDictionary if not needed):
        for dependent in tuple(self._dependents.keys() if self._dependents else ()):
            dependent.changed(originally_changed)

        # Just in case something called get() at some point
        # during that process and we have a cycle of some sort
        # make sure we didn't cache incomplete results.
        self._v_attrs = None

    def interfaces(self):
        """Return an iterator for the interfaces in the specification.
        """
        seen = {}
        for base in self.__bases__:
            for interface in base.interfaces():
                if interface not in seen:
                    seen[interface] = 1
                    yield interface

    def extends(self, interface, strict=True):
        """Does the specification extend the given interface?

        Test whether an interface in the specification extends the
        given interface
        """
        return ((interface in self._implied)
                and
                ((not strict) or (self != interface))
                )

    def weakref(self, callback=None):
        return weakref.ref(self, callback)

    def get(self, name, default=None):
        """Query for an attribute description
        """
        attrs = self._v_attrs
        if attrs is None:
            attrs = self._v_attrs = {}
        attr = attrs.get(name)
        if attr is None:
            for iface in self.__iro__:
                attr = iface.direct(name)
                if attr is not None:
                    attrs[name] = attr
                    break

        return default if attr is None else attr


class _InterfaceMetaClass(type):
    # Handling ``__module__`` on ``InterfaceClass`` is tricky. We need
    # to be able to read it on a type and get the expected string. We
    # also need to be able to set it on an instance and get the value
    # we set. So far so good. But what gets tricky is that we'd like
    # to store the value in the C structure (``InterfaceBase.__ibmodule__``) for
    # direct access during equality, sorting, and hashing. "No
    # problem, you think, I'll just use a property" (well, the C
    # equivalents, ``PyMemberDef`` or ``PyGetSetDef``).
    #
    # Except there is a problem. When a subclass is created, the
    # metaclass (``type``) always automatically puts the expected
    # string in the class's dictionary under ``__module__``, thus
    # overriding the property inherited from the superclass. Writing
    # ``Subclass.__module__`` still works, but
    # ``Subclass().__module__`` fails.
    #
    # There are multiple ways to work around this:
    #
    # (1) Define ``InterfaceBase.__getattribute__`` to watch for
    # ``__module__`` and return the C storage.
    #
    # This works, but slows down *all* attribute access (except,
    # ironically, to ``__module__``) by about 25% (40ns becomes 50ns)
    # (when implemented in C). Since that includes methods like
    # ``providedBy``, that's probably not acceptable.
    #
    # All the other methods involve modifying subclasses. This can be
    # done either on the fly in some cases, as instances are
    # constructed, or by using a metaclass. These next few can be done on the fly.
    #
    # (2) Make ``__module__`` a descriptor in each subclass dictionary.
    # It can't be a straight up ``@property`` descriptor, though, because accessing
    # it on the class returns a ``property`` object, not the desired string.
    #
    # (3) Implement a data descriptor (``__get__`` and ``__set__``)
    # that is both a subclass of string, and also does the redirect of
    # ``__module__`` to ``__ibmodule__`` and does the correct thing
    # with the ``instance`` argument to ``__get__`` is None (returns
    # the class's value.) (Why must it be a subclass of string? Because
    # when it' s in the class's dict, it's defined on an *instance* of the
    # metaclass; descriptors in an instance's dict aren't honored --- their
    # ``__get__`` is never invoked --- so it must also *be* the value we want
    # returned.)
    #
    # This works, preserves the ability to read and write
    # ``__module__``, and eliminates any penalty accessing other
    # attributes. But it slows down accessing ``__module__`` of
    # instances by 200% (40ns to 124ns), requires editing class dicts on the fly
    # (in InterfaceClass.__init__), thus slightly slowing down all interface creation,
    # and is ugly.
    #
    # (4) As in the last step, but make it a non-data descriptor (no ``__set__``).
    #
    # If you then *also* store a copy of ``__ibmodule__`` in
    # ``__module__`` in the instance's dict, reading works for both
    # class and instance and is full speed for instances. But the cost
    # is storage space, and you can't write to it anymore, not without
    # things getting out of sync.
    #
    # (Actually, ``__module__`` was never meant to be writable. Doing
    # so would break BTrees and normal dictionaries, as well as the
    # repr, maybe more.)
    #
    # That leaves us with a metaclass. (Recall that a class is an
    # instance of its metaclass, so properties/descriptors defined in
    # the metaclass are used when accessing attributes on the
    # instance/class. We'll use that to define ``__module__``.) Here
    # we can have our cake and eat it too: no extra storage, and
    # C-speed access to the underlying storage. The only substantial
    # cost is that metaclasses tend to make people's heads hurt. (But
    # still less than the descriptor-is-string, hopefully.)

    __slots__ = ()

    def __new__(cls, name, bases, attrs):
        # Figure out what module defined the interface.
        # This is copied from ``InterfaceClass.__init__``;
        # reviewers aren't sure how AttributeError or KeyError
        # could be raised.
        __module__ = sys._getframe(1).f_globals['__name__']
        # Get the C optimized __module__ accessor and give it
        # to the new class.
        moduledescr = InterfaceBase.__dict__['__module__']
        if isinstance(moduledescr, str):
            # We're working with the Python implementation,
            # not the C version
            moduledescr = InterfaceBase.__dict__['__module_property__']
        attrs['__module__'] = moduledescr
        kind = type.__new__(cls, name, bases, attrs)
        kind.__module = __module__
        return kind

    @property
    def __module__(cls):
        return cls.__module

    def __repr__(cls):
        return "<class '%s.%s'>" % (
            cls.__module,
            cls.__name__,
        )


_InterfaceClassBase = _InterfaceMetaClass(
    'InterfaceClass',
    # From least specific to most specific.
    (InterfaceBase, Specification, Element),
    {'__slots__': ()}
)


def interfacemethod(func):
    """
    Convert a method specification to an actual method of the interface.

    This is a decorator that functions like `staticmethod` et al.

    The primary use of this decorator is to allow interface definitions to
    define the ``__adapt__`` method, but other interface methods can be
    overridden this way too.

    .. seealso:: `zope.interface.interfaces.IInterfaceDeclaration.interfacemethod`
    """
    f_locals = sys._getframe(1).f_locals
    methods = f_locals.setdefault(INTERFACE_METHODS, {})
    methods[func.__name__] = func
    return _decorator_non_return


class InterfaceClass(_InterfaceClassBase):
    """
    Prototype (scarecrow) Interfaces Implementation.

    Note that it is not possible to change the ``__name__`` or ``__module__``
    after an instance of this object has been constructed.
    """

    # We can't say this yet because we don't have enough
    # infrastructure in place.
    #
    #implements(IInterface)

    def __new__(cls, name=None, bases=(), attrs=None, __doc__=None, # pylint:disable=redefined-builtin
                __module__=None):
        assert isinstance(bases, tuple)
        attrs = attrs or {}
        needs_custom_class = attrs.pop(INTERFACE_METHODS, None)
        if needs_custom_class:
            needs_custom_class.update(
                {'__classcell__': attrs.pop('__classcell__')}
                if '__classcell__' in attrs
                else {}
            )
            if '__adapt__' in needs_custom_class:
                # We need to tell the C code to call this.
                needs_custom_class['_CALL_CUSTOM_ADAPT'] = 1

            if issubclass(cls, _InterfaceClassWithCustomMethods):
                cls_bases = (cls,)
            elif cls is InterfaceClass:
                cls_bases = (_InterfaceClassWithCustomMethods,)
            else:
                cls_bases = (cls, _InterfaceClassWithCustomMethods)

            cls = type(cls)( # pylint:disable=self-cls-assignment
                name + "<WithCustomMethods>",
                cls_bases,
                needs_custom_class
            )
        elif PY2 and bases and len(bases) > 1:
            bases_with_custom_methods = tuple(
                type(b)
                for b in bases
                if issubclass(type(b), _InterfaceClassWithCustomMethods)
            )

            # If we have a subclass of InterfaceClass in *bases*,
            # Python 3 is smart enough to pass that as *cls*, but Python
            # 2 just passes whatever the first base in *bases* is. This means that if
            # we have multiple inheritance, and one of our bases has already defined
            # a custom method like ``__adapt__``, we do the right thing automatically
            # and extend it on Python 3, but not necessarily on Python 2. To fix this, we need
            # to run the MRO algorithm and get the most derived base manually.
            # Note that this only works for consistent resolution orders
            if bases_with_custom_methods:
                cls = type( # pylint:disable=self-cls-assignment
                    name + "<WithCustomMethods>",
                    bases_with_custom_methods,
                    {}
                ).__mro__[1] # Not the class we created, the most derived.

        return _InterfaceClassBase.__new__(cls)

    def __init__(self, name, bases=(), attrs=None, __doc__=None,  # pylint:disable=redefined-builtin
                 __module__=None):
        # We don't call our metaclass parent directly
        # pylint:disable=non-parent-init-called
        # pylint:disable=super-init-not-called
        if not all(isinstance(base, InterfaceClass) for base in bases):
            raise TypeError('Expected base interfaces')

        if attrs is None:
            attrs = {}

        if __module__ is None:
            __module__ = attrs.get('__module__')
            if isinstance(__module__, str):
                del attrs['__module__']
            else:
                try:
                    # Figure out what module defined the interface.
                    # This is how cPython figures out the module of
                    # a class, but of course it does it in C. :-/
                    __module__ = sys._getframe(1).f_globals['__name__']
                except (AttributeError, KeyError): # pragma: no cover
                    pass

        InterfaceBase.__init__(self, name, __module__)
        # These asserts assisted debugging the metaclass
        # assert '__module__' not in self.__dict__
        # assert self.__ibmodule__ is self.__module__ is __module__

        d = attrs.get('__doc__')
        if d is not None:
            if not isinstance(d, Attribute):
                if __doc__ is None:
                    __doc__ = d
                del attrs['__doc__']

        if __doc__ is None:
            __doc__ = ''

        Element.__init__(self, name, __doc__)

        tagged_data = attrs.pop(TAGGED_DATA, None)
        if tagged_data is not None:
            for key, val in tagged_data.items():
                self.setTaggedValue(key, val)

        Specification.__init__(self, bases)
        self.__attrs = self.__compute_attrs(attrs)

        self.__identifier__ = "%s.%s" % (__module__, name)

    def __compute_attrs(self, attrs):
        # Make sure that all recorded attributes (and methods) are of type
        # `Attribute` and `Method`
        def update_value(aname, aval):
            if isinstance(aval, Attribute):
                aval.interface = self
                if not aval.__name__:
                    aval.__name__ = aname
            elif isinstance(aval, FunctionType):
                aval = fromFunction(aval, self, name=aname)
            else:
                raise InvalidInterface("Concrete attribute, " + aname)
            return aval

        return {
            aname: update_value(aname, aval)
            for aname, aval in attrs.items()
            if aname not in (
                # __locals__: Python 3 sometimes adds this.
                '__locals__',
                # __qualname__: PEP 3155 (Python 3.3+)
                '__qualname__',
                # __annotations__: PEP 3107 (Python 3.0+)
                '__annotations__',
            )
            and aval is not _decorator_non_return
        }

    def interfaces(self):
        """Return an iterator for the interfaces in the specification.
        """
        yield self

    def getBases(self):
        return self.__bases__

    def isEqualOrExtendedBy(self, other):
        """Same interface or extends?"""
        return self == other or other.extends(self)

    def names(self, all=False): # pylint:disable=redefined-builtin
        """Return the attribute names defined by the interface."""
        if not all:
            return self.__attrs.keys()

        r = self.__attrs.copy()

        for base in self.__bases__:
            r.update(dict.fromkeys(base.names(all)))

        return r.keys()

    def __iter__(self):
        return iter(self.names(all=True))

    def namesAndDescriptions(self, all=False): # pylint:disable=redefined-builtin
        """Return attribute names and descriptions defined by interface."""
        if not all:
            return self.__attrs.items()

        r = {}
        for base in self.__bases__[::-1]:
            r.update(dict(base.namesAndDescriptions(all)))

        r.update(self.__attrs)

        return r.items()

    def getDescriptionFor(self, name):
        """Return the attribute description for the given name."""
        r = self.get(name)
        if r is not None:
            return r

        raise KeyError(name)

    __getitem__ = getDescriptionFor

    def __contains__(self, name):
        return self.get(name) is not None

    def direct(self, name):
        return self.__attrs.get(name)

    def queryDescriptionFor(self, name, default=None):
        return self.get(name, default)

    def validateInvariants(self, obj, errors=None):
        """validate object to defined invariants."""

        for iface in self.__iro__:
            for invariant in iface.queryDirectTaggedValue('invariants', ()):
                try:
                    invariant(obj)
                except Invalid as error:
                     if errors is not None:
                         errors.append(error)
                     else:
                         raise

        if errors:
            raise Invalid(errors)

    def queryTaggedValue(self, tag, default=None):
        """
        Queries for the value associated with *tag*, returning it from the nearest
        interface in the ``__iro__``.

        If not found, returns *default*.
        """
        for iface in self.__iro__:
            value = iface.queryDirectTaggedValue(tag, _marker)
            if value is not _marker:
                return value
        return default

    def getTaggedValue(self, tag):
        """ Returns the value associated with 'tag'. """
        value = self.queryTaggedValue(tag, default=_marker)
        if value is _marker:
            raise KeyError(tag)
        return value

    def getTaggedValueTags(self):
        """ Returns a list of all tags. """
        keys = set()
        for base in self.__iro__:
            keys.update(base.getDirectTaggedValueTags())
        return keys

    def __repr__(self):  # pragma: no cover
        try:
            return self._v_repr
        except AttributeError:
            name = self.__name__
            m = self.__ibmodule__
            if m:
                name = '%s.%s' % (m, name)
            r = "<%s %s>" % (self.__class__.__name__, name)
            self._v_repr = r # pylint:disable=attribute-defined-outside-init
            return r

    def _call_conform(self, conform):
        try:
            return conform(self)
        except TypeError: # pragma: no cover
            # We got a TypeError. It might be an error raised by
            # the __conform__ implementation, or *we* may have
            # made the TypeError by calling an unbound method
            # (object is a class).  In the later case, we behave
            # as though there is no __conform__ method. We can
            # detect this case by checking whether there is more
            # than one traceback object in the traceback chain:
            if sys.exc_info()[2].tb_next is not None:
                # There is more than one entry in the chain, so
                # reraise the error:
                raise
            # This clever trick is from Phillip Eby

        return None # pragma: no cover

    def __reduce__(self):
        return self.__name__

Interface = InterfaceClass("Interface", __module__='zope.interface')
# Interface is the only member of its own SRO.
Interface._calculate_sro = lambda: (Interface,)
Interface.changed(Interface)
assert Interface.__sro__ == (Interface,)
Specification._ROOT = Interface
ro._ROOT = Interface

class _InterfaceClassWithCustomMethods(InterfaceClass):
    """
    Marker class for interfaces with custom methods that override InterfaceClass methods.
    """


class Attribute(Element):
    """Attribute descriptions
    """

    # We can't say this yet because we don't have enough
    # infrastructure in place.
    #
    # implements(IAttribute)

    interface = None

    def _get_str_info(self):
        """Return extra data to put at the end of __str__."""
        return ""

    def __str__(self):
        of = ''
        if self.interface is not None:
            of = self.interface.__module__ + '.' + self.interface.__name__ + '.'
        # self.__name__ may be None during construction (e.g., debugging)
        return of + (self.__name__ or '<unknown>') + self._get_str_info()

    def __repr__(self):
        return "<%s.%s object at 0x%x %s>" % (
            type(self).__module__,
            type(self).__name__,
            id(self),
            self
        )


class Method(Attribute):
    """Method interfaces

    The idea here is that you have objects that describe methods.
    This provides an opportunity for rich meta-data.
    """

    # We can't say this yet because we don't have enough
    # infrastructure in place.
    #
    # implements(IMethod)

    positional = required = ()
    _optional = varargs = kwargs = None
    def _get_optional(self):
        if self._optional is None:
            return {}
        return self._optional
    def _set_optional(self, opt):
        self._optional = opt
    def _del_optional(self):
        self._optional = None
    optional = property(_get_optional, _set_optional, _del_optional)

    def __call__(self, *args, **kw):
        raise BrokenImplementation(self.interface, self.__name__)

    def getSignatureInfo(self):
        return {'positional': self.positional,
                'required': self.required,
                'optional': self.optional,
                'varargs': self.varargs,
                'kwargs': self.kwargs,
                }

    def getSignatureString(self):
        sig = []
        for v in self.positional:
            sig.append(v)
            if v in self.optional.keys():
                sig[-1] += "=" + repr(self.optional[v])
        if self.varargs:
            sig.append("*" + self.varargs)
        if self.kwargs:
            sig.append("**" + self.kwargs)

        return "(%s)" % ", ".join(sig)

    _get_str_info = getSignatureString


def fromFunction(func, interface=None, imlevel=0, name=None):
    name = name or func.__name__
    method = Method(name, func.__doc__)
    defaults = getattr(func, '__defaults__', None) or ()
    code = func.__code__
    # Number of positional arguments
    na = code.co_argcount - imlevel
    names = code.co_varnames[imlevel:]
    opt = {}
    # Number of required arguments
    defaults_count = len(defaults)
    if not defaults_count:
        # PyPy3 uses ``__defaults_count__`` for builtin methods
        # like ``dict.pop``. Surprisingly, these don't have recorded
        # ``__defaults__``
        defaults_count = getattr(func, '__defaults_count__', 0)

    nr = na - defaults_count
    if nr < 0:
        defaults = defaults[-nr:]
        nr = 0

    # Determine the optional arguments.
    opt.update(dict(zip(names[nr:], defaults)))

    method.positional = names[:na]
    method.required = names[:nr]
    method.optional = opt

    argno = na

    # Determine the function's variable argument's name (i.e. *args)
    if code.co_flags & CO_VARARGS:
        method.varargs = names[argno]
        argno = argno + 1
    else:
        method.varargs = None

    # Determine the function's keyword argument's name (i.e. **kw)
    if code.co_flags & CO_VARKEYWORDS:
        method.kwargs = names[argno]
    else:
        method.kwargs = None

    method.interface = interface

    for key, value in func.__dict__.items():
        method.setTaggedValue(key, value)

    return method


def fromMethod(meth, interface=None, name=None):
    if isinstance(meth, MethodType):
        func = meth.__func__
    else:
        func = meth
    return fromFunction(func, interface, imlevel=1, name=name)


# Now we can create the interesting interfaces and wire them up:
def _wire():
    from zope.interface.declarations import classImplements
    # From lest specific to most specific.
    from zope.interface.interfaces import IElement
    classImplements(Element, IElement)

    from zope.interface.interfaces import IAttribute
    classImplements(Attribute, IAttribute)

    from zope.interface.interfaces import IMethod
    classImplements(Method, IMethod)

    from zope.interface.interfaces import ISpecification
    classImplements(Specification, ISpecification)

    from zope.interface.interfaces import IInterface
    classImplements(InterfaceClass, IInterface)


# We import this here to deal with module dependencies.
# pylint:disable=wrong-import-position
from zope.interface.declarations import implementedBy
from zope.interface.declarations import providedBy
from zope.interface.exceptions import InvalidInterface
from zope.interface.exceptions import BrokenImplementation

# This ensures that ``Interface`` winds up in the flattened()
# list of the immutable declaration. It correctly overrides changed()
# as a no-op, so we bypass that.
from zope.interface.declarations import _empty
Specification.changed(_empty, _empty)
