##############################################################################
# Copyright (c) 2020 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
##############################################################################

import itertools
from types import FunctionType

from zope.interface import classImplements
from zope.interface import Interface
from zope.interface.interface import fromFunction
from zope.interface.interface import InterfaceClass
from zope.interface.interface import _decorator_non_return

__all__ = [
    # Nothing public here.
]


# pylint:disable=inherit-non-class,
# pylint:disable=no-self-argument,no-method-argument
# pylint:disable=unexpected-special-method-signature

class optional(object):
    # Apply this decorator to a method definition to make it
    # optional (remove it from the list of required names), overriding
    # the definition inherited from the ABC.
    def __init__(self, method):
        self.__doc__ = method.__doc__


class ABCInterfaceClass(InterfaceClass):
    """
    An interface that is automatically derived from a
    :class:`abc.ABCMeta` type.

    Internal use only.

    The body of the interface definition *must* define
    a property ``abc`` that is the ABC to base the interface on.

    If ``abc`` is *not* in the interface definition, a regular
    interface will be defined instead (but ``extra_classes`` is still
    respected).

    Use the ``@optional`` decorator on method definitions if
    the ABC defines methods that are not actually required in all cases
    because the Python language has multiple ways to implement a protocol.
    For example, the ``iter()`` protocol can be implemented with
    ``__iter__`` or the pair ``__len__`` and ``__getitem__``.

    When created, any existing classes that are registered to conform
    to the ABC are declared to implement this interface. This is *not*
    automatically updated as the ABC registry changes. If the body of the
    interface definition defines ``extra_classes``, it should be a
    tuple giving additional classes to declare implement the interface.

    Note that this is not fully symmetric. For example, it is usually
    the case that a subclass relationship carries the interface
    declarations over::

        >>> from zope.interface import Interface
        >>> class I1(Interface):
        ...     pass
        ...
        >>> from zope.interface import implementer
        >>> @implementer(I1)
        ... class Root(object):
        ...     pass
        ...
        >>> class Child(Root):
        ...     pass
        ...
        >>> child = Child()
        >>> isinstance(child, Root)
        True
        >>> from zope.interface import providedBy
        >>> list(providedBy(child))
        [<InterfaceClass __main__.I1>]

    However, that's not the case with ABCs and ABC interfaces. Just
    because ``isinstance(A(), AnABC)`` and ``isinstance(B(), AnABC)``
    are both true, that doesn't mean there's any class hierarchy
    relationship between ``A`` and ``B``, or between either of them
    and ``AnABC``. Thus, if ``AnABC`` implemented ``IAnABC``, it would
    not follow that either ``A`` or ``B`` implements ``IAnABC`` (nor
    their instances provide it)::

        >>> class SizedClass(object):
        ...     def __len__(self): return 1
        ...
        >>> from collections.abc import Sized
        >>> isinstance(SizedClass(), Sized)
        True
        >>> from zope.interface import classImplements
        >>> classImplements(Sized, I1)
        None
        >>> list(providedBy(SizedClass()))
        []

    Thus, to avoid conflicting assumptions, ABCs should not be
    declared to implement their parallel ABC interface. Only concrete
    classes specifically registered with the ABC should be declared to
    do so.

    .. versionadded:: 5.0.0
    """

    # If we could figure out invalidation, and used some special
    # Specification/Declaration instances, and override the method ``providedBy`` here,
    # perhaps we could more closely integrate with ABC virtual inheritance?

    def __init__(self, name, bases, attrs):
        # go ahead and give us a name to ease debugging.
        self.__name__ = name
        extra_classes = attrs.pop('extra_classes', ())
        ignored_classes = attrs.pop('ignored_classes', ())

        if 'abc' not in attrs:
            # Something like ``IList(ISequence)``: We're extending
            # abc interfaces but not an ABC interface ourself.
            InterfaceClass.__init__(self, name, bases, attrs)
            ABCInterfaceClass.__register_classes(self, extra_classes, ignored_classes)
            self.__class__ = InterfaceClass
            return

        based_on = attrs.pop('abc')
        self.__abc = based_on
        self.__extra_classes = tuple(extra_classes)
        self.__ignored_classes = tuple(ignored_classes)

        assert name[1:] == based_on.__name__, (name, based_on)
        methods = {
            # Passing the name is important in case of aliases,
            # e.g., ``__ror__ = __or__``.
            k: self.__method_from_function(v, k)
            for k, v in vars(based_on).items()
            if isinstance(v, FunctionType) and not self.__is_private_name(k)
            and not self.__is_reverse_protocol_name(k)
        }

        methods['__doc__'] = self.__create_class_doc(attrs)
        # Anything specified in the body takes precedence.
        methods.update(attrs)
        InterfaceClass.__init__(self, name, bases, methods)
        self.__register_classes()

    @staticmethod
    def __optional_methods_to_docs(attrs):
        optionals = {k: v for k, v in attrs.items() if isinstance(v, optional)}
        for k in optionals:
            attrs[k] = _decorator_non_return

        if not optionals:
            return ''

        docs = "\n\nThe following methods are optional:\n - " + "\n-".join(
            "%s\n%s" % (k, v.__doc__) for k, v in optionals.items()
        )
        return docs

    def __create_class_doc(self, attrs):
        based_on = self.__abc
        def ref(c):
            mod = c.__module__
            name = c.__name__
            if mod == str.__module__:
                return "`%s`" % name
            if mod == '_io':
                mod = 'io'
            return "`%s.%s`" % (mod, name)
        implementations_doc = "\n - ".join(
            ref(c)
            for c in sorted(self.getRegisteredConformers(), key=ref)
        )
        if implementations_doc:
            implementations_doc = "\n\nKnown implementations are:\n\n - " + implementations_doc

        based_on_doc = (based_on.__doc__ or '')
        based_on_doc = based_on_doc.splitlines()
        based_on_doc = based_on_doc[0] if based_on_doc else ''

        doc = """Interface for the ABC `%s.%s`.\n\n%s%s%s""" % (
            based_on.__module__, based_on.__name__,
            attrs.get('__doc__', based_on_doc),
            self.__optional_methods_to_docs(attrs),
            implementations_doc
        )
        return doc


    @staticmethod
    def __is_private_name(name):
        if name.startswith('__') and name.endswith('__'):
            return False
        return name.startswith('_')

    @staticmethod
    def __is_reverse_protocol_name(name):
        # The reverse names, like __rand__,
        # aren't really part of the protocol. The interpreter has
        # very complex behaviour around invoking those. PyPy
        # doesn't always even expose them as attributes.
        return name.startswith('__r') and name.endswith('__')

    def __method_from_function(self, function, name):
        method = fromFunction(function, self, name=name)
        # Eliminate the leading *self*, which is implied in
        # an interface, but explicit in an ABC.
        method.positional = method.positional[1:]
        return method

    def __register_classes(self, conformers=None, ignored_classes=None):
        # Make the concrete classes already present in our ABC's registry
        # declare that they implement this interface.
        conformers = conformers if conformers is not None else self.getRegisteredConformers()
        ignored = ignored_classes if ignored_classes is not None else self.__ignored_classes
        for cls in conformers:
            if cls in ignored:
                continue
            classImplements(cls, self)

    def getABC(self):
        """
        Return the ABC this interface represents.
        """
        return self.__abc

    def getRegisteredConformers(self):
        """
        Return an iterable of the classes that are known to conform to
        the ABC this interface parallels.
        """
        based_on = self.__abc

        # The registry only contains things that aren't already
        # known to be subclasses of the ABC. But the ABC is in charge
        # of checking that, so its quite possible that registrations
        # are in fact ignored, winding up just in the _abc_cache.
        try:
            registered = list(based_on._abc_registry) + list(based_on._abc_cache)
        except AttributeError:
            # Rewritten in C in CPython 3.7.
            # These expose the underlying weakref.
            from abc import _get_dump
            data = _get_dump(based_on)
            registry = data[0]
            cache = data[1]
            registered = [x() for x in itertools.chain(registry, cache)]
            registered = [x for x in registered if x is not None]

        return set(itertools.chain(registered, self.__extra_classes))


def _create_ABCInterface():
    # It's a two-step process to create the root ABCInterface, because
    # without specifying a corresponding ABC, using the normal constructor
    # gets us a plain InterfaceClass object, and there is no ABC to associate with the
    # root.
    abc_name_bases_attrs = ('ABCInterface', (Interface,), {})
    instance = ABCInterfaceClass.__new__(ABCInterfaceClass, *abc_name_bases_attrs)
    InterfaceClass.__init__(instance, *abc_name_bases_attrs)
    return instance

ABCInterface = _create_ABCInterface()
