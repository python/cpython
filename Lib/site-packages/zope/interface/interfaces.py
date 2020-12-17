##############################################################################
#
# Copyright (c) 2002 Zope Foundation and Contributors.
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
"""Interface Package Interfaces
"""
__docformat__ = 'restructuredtext'

from zope.interface.interface import Attribute
from zope.interface.interface import Interface
from zope.interface.declarations import implementer

__all__ = [
    'IAdapterRegistration',
    'IAdapterRegistry',
    'IAttribute',
    'IComponentLookup',
    'IComponentRegistry',
    'IComponents',
    'IDeclaration',
    'IElement',
    'IHandlerRegistration',
    'IInterface',
    'IInterfaceDeclaration',
    'IMethod',
    'IObjectEvent',
    'IRegistered',
    'IRegistration',
    'IRegistrationEvent',
    'ISpecification',
    'ISubscriptionAdapterRegistration',
    'IUnregistered',
    'IUtilityRegistration',
]

# pylint:disable=inherit-non-class,no-method-argument,no-self-argument
# pylint:disable=unexpected-special-method-signature
# pylint:disable=too-many-lines

class IElement(Interface):
    """
    Objects that have basic documentation and tagged values.

    Known derivatives include :class:`IAttribute` and its derivative
    :class:`IMethod`; these have no notion of inheritance.
    :class:`IInterface` is also a derivative, and it does have a
    notion of inheritance, expressed through its ``__bases__`` and
    ordered in its ``__iro__`` (both defined by
    :class:`ISpecification`).
    """

    # pylint:disable=arguments-differ

    # Note that defining __doc__ as an Attribute hides the docstring
    # from introspection. When changing it, also change it in the Sphinx
    # ReST files.

    __name__ = Attribute('__name__', 'The object name')
    __doc__ = Attribute('__doc__', 'The object doc string')

    ###
    # Tagged values.
    #
    # Direct values are established in this instance. Others may be
    # inherited. Although ``IElement`` itself doesn't have a notion of
    # inheritance, ``IInterface`` *does*. It might have been better to
    # make ``IInterface`` define new methods
    # ``getIndirectTaggedValue``, etc, to include inheritance instead
    # of overriding ``getTaggedValue`` to do that, but that ship has sailed.
    # So to keep things nice and symmetric, we define the ``Direct`` methods here.
    ###

    def getTaggedValue(tag):
        """Returns the value associated with *tag*.

        Raise a `KeyError` if the tag isn't set.

        If the object has a notion of inheritance, this searches
        through the inheritance hierarchy and returns the nearest result.
        If there is no such notion, this looks only at this object.

        .. versionchanged:: 4.7.0
           This method should respect inheritance if present.
        """

    def queryTaggedValue(tag, default=None):
        """
        As for `getTaggedValue`, but instead of raising a `KeyError`, returns *default*.


        .. versionchanged:: 4.7.0
           This method should respect inheritance if present.
        """

    def getTaggedValueTags():
        """
        Returns a collection of all tags in no particular order.

        If the object has a notion of inheritance, this
        includes all the inherited tagged values. If there is
        no such notion, this looks only at this object.

        .. versionchanged:: 4.7.0
           This method should respect inheritance if present.
        """

    def setTaggedValue(tag, value):
        """
        Associates *value* with *key* directly in this object.
        """

    def getDirectTaggedValue(tag):
        """
        As for `getTaggedValue`, but never includes inheritance.

        .. versionadded:: 5.0.0
        """

    def queryDirectTaggedValue(tag, default=None):
        """
        As for `queryTaggedValue`, but never includes inheritance.

        .. versionadded:: 5.0.0
        """

    def getDirectTaggedValueTags():
        """
        As for `getTaggedValueTags`, but includes only tags directly
        set on this object.

        .. versionadded:: 5.0.0
        """


class IAttribute(IElement):
    """Attribute descriptors"""

    interface = Attribute('interface',
                          'Stores the interface instance in which the '
                          'attribute is located.')


class IMethod(IAttribute):
    """Method attributes"""

    def getSignatureInfo():
        """Returns the signature information.

        This method returns a dictionary with the following string keys:

        - positional
            A sequence of the names of positional arguments.
        - required
            A sequence of the names of required arguments.
        - optional
            A dictionary mapping argument names to their default values.
        - varargs
            The name of the varargs argument (or None).
        - kwargs
            The name of the kwargs argument (or None).
        """

    def getSignatureString():
        """Return a signature string suitable for inclusion in documentation.

        This method returns the function signature string. For example, if you
        have ``def func(a, b, c=1, d='f')``, then the signature string is ``"(a, b,
        c=1, d='f')"``.
        """

class ISpecification(Interface):
    """Object Behavioral specifications"""
    # pylint:disable=arguments-differ
    def providedBy(object): # pylint:disable=redefined-builtin
        """Test whether the interface is implemented by the object

        Return true of the object asserts that it implements the
        interface, including asserting that it implements an extended
        interface.
        """

    def implementedBy(class_):
        """Test whether the interface is implemented by instances of the class

        Return true of the class asserts that its instances implement the
        interface, including asserting that they implement an extended
        interface.
        """

    def isOrExtends(other):
        """Test whether the specification is or extends another
        """

    def extends(other, strict=True):
        """Test whether a specification extends another

        The specification extends other if it has other as a base
        interface or if one of it's bases extends other.

        If strict is false, then the specification extends itself.
        """

    def weakref(callback=None):
        """Return a weakref to the specification

        This method is, regrettably, needed to allow weakrefs to be
        computed to security-proxied specifications.  While the
        zope.interface package does not require zope.security or
        zope.proxy, it has to be able to coexist with it.

        """

    __bases__ = Attribute("""Base specifications

    A tuple of specifications from which this specification is
    directly derived.

    """)

    __sro__ = Attribute("""Specification-resolution order

    A tuple of the specification and all of it's ancestor
    specifications from most specific to least specific. The specification
    itself is the first element.

    (This is similar to the method-resolution order for new-style classes.)
    """)

    __iro__ = Attribute("""Interface-resolution order

    A tuple of the specification's ancestor interfaces from
    most specific to least specific.  The specification itself is
    included if it is an interface.

    (This is similar to the method-resolution order for new-style classes.)
    """)

    def get(name, default=None):
        """Look up the description for a name

        If the named attribute is not defined, the default is
        returned.
        """


class IInterface(ISpecification, IElement):
    """Interface objects

    Interface objects describe the behavior of an object by containing
    useful information about the object.  This information includes:

    - Prose documentation about the object.  In Python terms, this
      is called the "doc string" of the interface.  In this element,
      you describe how the object works in prose language and any
      other useful information about the object.

    - Descriptions of attributes.  Attribute descriptions include
      the name of the attribute and prose documentation describing
      the attributes usage.

    - Descriptions of methods.  Method descriptions can include:

        - Prose "doc string" documentation about the method and its
          usage.

        - A description of the methods arguments; how many arguments
          are expected, optional arguments and their default values,
          the position or arguments in the signature, whether the
          method accepts arbitrary arguments and whether the method
          accepts arbitrary keyword arguments.

    - Optional tagged data.  Interface objects (and their attributes and
      methods) can have optional, application specific tagged data
      associated with them.  Examples uses for this are examples,
      security assertions, pre/post conditions, and other possible
      information you may want to associate with an Interface or its
      attributes.

    Not all of this information is mandatory.  For example, you may
    only want the methods of your interface to have prose
    documentation and not describe the arguments of the method in
    exact detail.  Interface objects are flexible and let you give or
    take any of these components.

    Interfaces are created with the Python class statement using
    either `zope.interface.Interface` or another interface, as in::

      from zope.interface import Interface

      class IMyInterface(Interface):
        '''Interface documentation'''

        def meth(arg1, arg2):
            '''Documentation for meth'''

        # Note that there is no self argument

     class IMySubInterface(IMyInterface):
        '''Interface documentation'''

        def meth2():
            '''Documentation for meth2'''

    You use interfaces in two ways:

    - You assert that your object implement the interfaces.

      There are several ways that you can declare that an object
      provides an interface:

      1. Call `zope.interface.implementer` on your class definition.

      2. Call `zope.interface.directlyProvides` on your object.

      3. Call `zope.interface.classImplements` to declare that instances
         of a class implement an interface.

         For example::

           from zope.interface import classImplements

           classImplements(some_class, some_interface)

         This approach is useful when it is not an option to modify
         the class source.  Note that this doesn't affect what the
         class itself implements, but only what its instances
         implement.

    - You query interface meta-data. See the IInterface methods and
      attributes for details.

    """
    # pylint:disable=arguments-differ
    def names(all=False): # pylint:disable=redefined-builtin
        """Get the interface attribute names

        Return a collection of the names of the attributes, including
        methods, included in the interface definition.

        Normally, only directly defined attributes are included. If
        a true positional or keyword argument is given, then
        attributes defined by base classes will be included.
        """

    def namesAndDescriptions(all=False): # pylint:disable=redefined-builtin
        """Get the interface attribute names and descriptions

        Return a collection of the names and descriptions of the
        attributes, including methods, as name-value pairs, included
        in the interface definition.

        Normally, only directly defined attributes are included. If
        a true positional or keyword argument is given, then
        attributes defined by base classes will be included.
        """

    def __getitem__(name):
        """Get the description for a name

        If the named attribute is not defined, a `KeyError` is raised.
        """

    def direct(name):
        """Get the description for the name if it was defined by the interface

        If the interface doesn't define the name, returns None.
        """

    def validateInvariants(obj, errors=None):
        """Validate invariants

        Validate object to defined invariants.  If errors is None,
        raises first Invalid error; if errors is a list, appends all errors
        to list, then raises Invalid with the errors as the first element
        of the "args" tuple."""

    def __contains__(name):
        """Test whether the name is defined by the interface"""

    def __iter__():
        """Return an iterator over the names defined by the interface

        The names iterated include all of the names defined by the
        interface directly and indirectly by base interfaces.
        """

    __module__ = Attribute("""The name of the module defining the interface""")


class IDeclaration(ISpecification):
    """Interface declaration

    Declarations are used to express the interfaces implemented by
    classes or provided by objects.
    """

    def __contains__(interface):
        """Test whether an interface is in the specification

        Return true if the given interface is one of the interfaces in
        the specification and false otherwise.
        """

    def __iter__():
        """Return an iterator for the interfaces in the specification
        """

    def flattened():
        """Return an iterator of all included and extended interfaces

        An iterator is returned for all interfaces either included in
        or extended by interfaces included in the specifications
        without duplicates. The interfaces are in "interface
        resolution order". The interface resolution order is such that
        base interfaces are listed after interfaces that extend them
        and, otherwise, interfaces are included in the order that they
        were defined in the specification.
        """

    def __sub__(interfaces):
        """Create an interface specification with some interfaces excluded

        The argument can be an interface or an interface
        specifications.  The interface or interfaces given in a
        specification are subtracted from the interface specification.

        Removing an interface that is not in the specification does
        not raise an error. Doing so has no effect.

        Removing an interface also removes sub-interfaces of the interface.

        """

    def __add__(interfaces):
        """Create an interface specification with some interfaces added

        The argument can be an interface or an interface
        specifications.  The interface or interfaces given in a
        specification are added to the interface specification.

        Adding an interface that is already in the specification does
        not raise an error. Doing so has no effect.
        """

    def __nonzero__():
        """Return a true value of the interface specification is non-empty
        """

class IInterfaceDeclaration(Interface):
    """
    Declare and check the interfaces of objects.

    The functions defined in this interface are used to declare the
    interfaces that objects provide and to query the interfaces that
    have been declared.

    Interfaces can be declared for objects in two ways:

        - Interfaces are declared for instances of the object's class

        - Interfaces are declared for the object directly.

    The interfaces declared for an object are, therefore, the union of
    interfaces declared for the object directly and the interfaces
    declared for instances of the object's class.

    Note that we say that a class implements the interfaces provided
    by it's instances. An instance can also provide interfaces
    directly. The interfaces provided by an object are the union of
    the interfaces provided directly and the interfaces implemented by
    the class.

    This interface is implemented by :mod:`zope.interface`.
    """
    # pylint:disable=arguments-differ
    ###
    # Defining interfaces
    ###

    Interface = Attribute("The base class used to create new interfaces")

    def taggedValue(key, value):
        """
        Attach a tagged value to an interface while defining the interface.

        This is a way of executing :meth:`IElement.setTaggedValue` from
        the definition of the interface. For example::

             class IFoo(Interface):
                 taggedValue('key', 'value')

        .. seealso:: `zope.interface.taggedValue`
        """

    def invariant(checker_function):
        """
        Attach an invariant checker function to an interface while defining it.

        Invariants can later be validated against particular implementations by
        calling :meth:`IInterface.validateInvariants`.

        For example::

             def check_range(ob):
                 if ob.max < ob.min:
                     raise ValueError("max value is less than min value")

             class IRange(Interface):
                 min = Attribute("The min value")
                 max = Attribute("The max value")

                 invariant(check_range)

        .. seealso:: `zope.interface.invariant`
        """

    def interfacemethod(method):
        """
        A decorator that transforms a method specification into an
        implementation method.

        This is used to override methods of ``Interface`` or provide new methods.
        Definitions using this decorator will not appear in :meth:`IInterface.names()`.
        It is possible to have an implementation method and a method specification
        of the same name.

        For example::

             class IRange(Interface):
                 @interfacemethod
                 def __adapt__(self, obj):
                     if isinstance(obj, range):
                         # Return the builtin ``range`` as-is
                         return obj
                     return super(type(IRange), self).__adapt__(obj)

        You can use ``super`` to call the parent class functionality. Note that
        the zero-argument version (``super().__adapt__``) works on Python 3.6 and above, but
        prior to that the two-argument version must be used, and the class must be explicitly
        passed as the first argument.

        .. versionadded:: 5.1.0
        .. seealso:: `zope.interface.interfacemethod`
        """

    ###
    # Querying interfaces
    ###

    def providedBy(ob):
        """
        Return the interfaces provided by an object.

        This is the union of the interfaces directly provided by an
        object and interfaces implemented by it's class.

        The value returned is an `IDeclaration`.

        .. seealso:: `zope.interface.providedBy`
        """

    def implementedBy(class_):
        """
        Return the interfaces implemented for a class's instances.

        The value returned is an `IDeclaration`.

        .. seealso:: `zope.interface.implementedBy`
        """

    ###
    # Declaring interfaces
    ###

    def classImplements(class_, *interfaces):
        """
        Declare additional interfaces implemented for instances of a class.

        The arguments after the class are one or more interfaces or
        interface specifications (`IDeclaration` objects).

        The interfaces given (including the interfaces in the
        specifications) are added to any interfaces previously
        declared.

        Consider the following example::

          class C(A, B):
             ...

          classImplements(C, I1, I2)


        Instances of ``C`` provide ``I1``, ``I2``, and whatever interfaces
        instances of ``A`` and ``B`` provide. This is equivalent to::

            @implementer(I1, I2)
            class C(A, B):
                pass

        .. seealso:: `zope.interface.classImplements`
        .. seealso:: `zope.interface.implementer`
        """

    def classImplementsFirst(cls, interface):
        """
        See :func:`zope.interface.classImplementsFirst`.
        """

    def implementer(*interfaces):
        """
        Create a decorator for declaring interfaces implemented by a
        factory.

        A callable is returned that makes an implements declaration on
        objects passed to it.

        .. seealso:: :meth:`classImplements`
        """

    def classImplementsOnly(class_, *interfaces):
        """
        Declare the only interfaces implemented by instances of a class.

        The arguments after the class are one or more interfaces or
        interface specifications (`IDeclaration` objects).

        The interfaces given (including the interfaces in the
        specifications) replace any previous declarations.

        Consider the following example::

          class C(A, B):
             ...

          classImplements(C, IA, IB. IC)
          classImplementsOnly(C. I1, I2)

        Instances of ``C`` provide only ``I1``, ``I2``, and regardless of
        whatever interfaces instances of ``A`` and ``B`` implement.

        .. seealso:: `zope.interface.classImplementsOnly`
        """

    def implementer_only(*interfaces):
        """
        Create a decorator for declaring the only interfaces implemented.

        A callable is returned that makes an implements declaration on
        objects passed to it.

        .. seealso:: `zope.interface.implementer_only`
        """

    def directlyProvidedBy(object): # pylint:disable=redefined-builtin
        """
        Return the interfaces directly provided by the given object.

        The value returned is an `IDeclaration`.

        .. seealso:: `zope.interface.directlyProvidedBy`
        """

    def directlyProvides(object, *interfaces): # pylint:disable=redefined-builtin
        """
        Declare interfaces declared directly for an object.

        The arguments after the object are one or more interfaces or
        interface specifications (`IDeclaration` objects).

        .. caution::
           The interfaces given (including the interfaces in the
           specifications) *replace* interfaces previously
           declared for the object. See :meth:`alsoProvides` to add
           additional interfaces.

        Consider the following example::

          class C(A, B):
             ...

          ob = C()
          directlyProvides(ob, I1, I2)

        The object, ``ob`` provides ``I1``, ``I2``, and whatever interfaces
        instances have been declared for instances of ``C``.

        To remove directly provided interfaces, use `directlyProvidedBy` and
        subtract the unwanted interfaces. For example::

          directlyProvides(ob, directlyProvidedBy(ob)-I2)

        removes I2 from the interfaces directly provided by
        ``ob``. The object, ``ob`` no longer directly provides ``I2``,
        although it might still provide ``I2`` if it's class
        implements ``I2``.

        To add directly provided interfaces, use `directlyProvidedBy` and
        include additional interfaces.  For example::

          directlyProvides(ob, directlyProvidedBy(ob), I2)

        adds I2 to the interfaces directly provided by ob.

        .. seealso:: `zope.interface.directlyProvides`
        """

    def alsoProvides(object, *interfaces): # pylint:disable=redefined-builtin
        """
        Declare additional interfaces directly for an object.

        For example::

          alsoProvides(ob, I1)

        is equivalent to::

          directlyProvides(ob, directlyProvidedBy(ob), I1)

        .. seealso:: `zope.interface.alsoProvides`
        """

    def noLongerProvides(object, interface): # pylint:disable=redefined-builtin
        """
        Remove an interface from the list of an object's directly provided
        interfaces.

        For example::

          noLongerProvides(ob, I1)

        is equivalent to::

          directlyProvides(ob, directlyProvidedBy(ob) - I1)

        with the exception that if ``I1`` is an interface that is
        provided by ``ob`` through the class's implementation,
        `ValueError` is raised.

        .. seealso:: `zope.interface.noLongerProvides`
        """

    def implements(*interfaces):
        """
        Declare interfaces implemented by instances of a class.

        .. deprecated:: 5.0
           This only works for Python 2. The `implementer` decorator
           is preferred for all versions.

        This function is called in a class definition (Python 2.x only).

        The arguments are one or more interfaces or interface
        specifications (`IDeclaration` objects).

        The interfaces given (including the interfaces in the
        specifications) are added to any interfaces previously
        declared.

        Previous declarations include declarations for base classes
        unless implementsOnly was used.

        This function is provided for convenience. It provides a more
        convenient way to call `classImplements`. For example::

          implements(I1)

        is equivalent to calling::

          classImplements(C, I1)

        after the class has been created.

        Consider the following example (Python 2.x only)::

          class C(A, B):
            implements(I1, I2)


        Instances of ``C`` implement ``I1``, ``I2``, and whatever interfaces
        instances of ``A`` and ``B`` implement.
        """

    def implementsOnly(*interfaces):
        """
        Declare the only interfaces implemented by instances of a class.

        .. deprecated:: 5.0
           This only works for Python 2. The `implementer_only` decorator
           is preferred for all versions.

        This function is called in a class definition (Python 2.x only).

        The arguments are one or more interfaces or interface
        specifications (`IDeclaration` objects).

        Previous declarations including declarations for base classes
        are overridden.

        This function is provided for convenience. It provides a more
        convenient way to call `classImplementsOnly`. For example::

          implementsOnly(I1)

        is equivalent to calling::

          classImplementsOnly(I1)

        after the class has been created.

        Consider the following example (Python 2.x only)::

          class C(A, B):
            implementsOnly(I1, I2)


        Instances of ``C`` implement ``I1``, ``I2``, regardless of what
        instances of ``A`` and ``B`` implement.
        """

    def classProvides(*interfaces):
        """
        Declare interfaces provided directly by a class.

        .. deprecated:: 5.0
           This only works for Python 2. The `provider` decorator
           is preferred for all versions.

        This function is called in a class definition.

        The arguments are one or more interfaces or interface
        specifications (`IDeclaration` objects).

        The given interfaces (including the interfaces in the
        specifications) are used to create the class's direct-object
        interface specification.  An error will be raised if the module
        class has an direct interface specification.  In other words, it is
        an error to call this function more than once in a class
        definition.

        Note that the given interfaces have nothing to do with the
        interfaces implemented by instances of the class.

        This function is provided for convenience. It provides a more
        convenient way to call `directlyProvides` for a class. For example::

          classProvides(I1)

        is equivalent to calling::

          directlyProvides(theclass, I1)

        after the class has been created.
        """

    def provider(*interfaces):
        """
        A class decorator version of `classProvides`.

        .. seealso:: `zope.interface.provider`
        """

    def moduleProvides(*interfaces):
        """
        Declare interfaces provided by a module.

        This function is used in a module definition.

        The arguments are one or more interfaces or interface
        specifications (`IDeclaration` objects).

        The given interfaces (including the interfaces in the
        specifications) are used to create the module's direct-object
        interface specification.  An error will be raised if the module
        already has an interface specification.  In other words, it is
        an error to call this function more than once in a module
        definition.

        This function is provided for convenience. It provides a more
        convenient way to call `directlyProvides` for a module. For example::

          moduleImplements(I1)

        is equivalent to::

          directlyProvides(sys.modules[__name__], I1)

        .. seealso:: `zope.interface.moduleProvides`
        """

    def Declaration(*interfaces):
        """
        Create an interface specification.

        The arguments are one or more interfaces or interface
        specifications (`IDeclaration` objects).

        A new interface specification (`IDeclaration`) with the given
        interfaces is returned.

        .. seealso:: `zope.interface.Declaration`
        """

class IAdapterRegistry(Interface):
    """Provide an interface-based registry for adapters

    This registry registers objects that are in some sense "from" a
    sequence of specification to an interface and a name.

    No specific semantics are assumed for the registered objects,
    however, the most common application will be to register factories
    that adapt objects providing required specifications to a provided
    interface.
    """

    def register(required, provided, name, value):
        """Register a value

        A value is registered for a *sequence* of required specifications, a
        provided interface, and a name, which must be text.
        """

    def registered(required, provided, name=u''):
        """Return the component registered for the given interfaces and name

        name must be text.

        Unlike the lookup method, this methods won't retrieve
        components registered for more specific required interfaces or
        less specific provided interfaces.

        If no component was registered exactly for the given
        interfaces and name, then None is returned.

        """

    def lookup(required, provided, name='', default=None):
        """Lookup a value

        A value is looked up based on a *sequence* of required
        specifications, a provided interface, and a name, which must be
        text.
        """

    def queryMultiAdapter(objects, provided, name=u'', default=None):
        """Adapt a sequence of objects to a named, provided, interface
        """

    def lookup1(required, provided, name=u'', default=None):
        """Lookup a value using a single required interface

        A value is looked up based on a single required
        specifications, a provided interface, and a name, which must be
        text.
        """

    def queryAdapter(object, provided, name=u'', default=None): # pylint:disable=redefined-builtin
        """Adapt an object using a registered adapter factory.
        """

    def adapter_hook(provided, object, name=u'', default=None): # pylint:disable=redefined-builtin
        """Adapt an object using a registered adapter factory.

        name must be text.
        """

    def lookupAll(required, provided):
        """Find all adapters from the required to the provided interfaces

        An iterable object is returned that provides name-value two-tuples.
        """

    def names(required, provided): # pylint:disable=arguments-differ
        """Return the names for which there are registered objects
        """

    def subscribe(required, provided, subscriber): # pylint:disable=arguments-differ
        """Register a subscriber

        A subscriber is registered for a *sequence* of required
        specifications, a provided interface, and a name.

        Multiple subscribers may be registered for the same (or
        equivalent) interfaces.

        .. versionchanged:: 5.1.1
           Correct the method signature to remove the ``name`` parameter.
           Subscribers have no names.
        """

    def subscriptions(required, provided):
        """Get a sequence of subscribers

        Subscribers for a **sequence** of *required* interfaces, and a *provided*
        interface are returned.

        .. versionchanged:: 5.1.1
           Correct the method signature to remove the ``name`` parameter.
           Subscribers have no names.
        """

    def subscribers(objects, provided):
        """Get a sequence of subscription adapters

        .. versionchanged:: 5.1.1
           Correct the method signature to remove the ``name`` parameter.
           Subscribers have no names.
        """

# begin formerly in zope.component

class ComponentLookupError(LookupError):
    """A component could not be found."""

class Invalid(Exception):
    """A component doesn't satisfy a promise."""

class IObjectEvent(Interface):
    """An event related to an object.

    The object that generated this event is not necessarily the object
    refered to by location.
    """

    object = Attribute("The subject of the event.")


@implementer(IObjectEvent)
class ObjectEvent(object):

    def __init__(self, object): # pylint:disable=redefined-builtin
        self.object = object


class IComponentLookup(Interface):
    """Component Manager for a Site

    This object manages the components registered at a particular site. The
    definition of a site is intentionally vague.
    """

    adapters = Attribute(
        "Adapter Registry to manage all registered adapters.")

    utilities = Attribute(
        "Adapter Registry to manage all registered utilities.")

    def queryAdapter(object, interface, name=u'', default=None): # pylint:disable=redefined-builtin
        """Look for a named adapter to an interface for an object

        If a matching adapter cannot be found, returns the default.
        """

    def getAdapter(object, interface, name=u''): # pylint:disable=redefined-builtin
        """Look for a named adapter to an interface for an object

        If a matching adapter cannot be found, a `ComponentLookupError`
        is raised.
        """

    def queryMultiAdapter(objects, interface, name=u'', default=None):
        """Look for a multi-adapter to an interface for multiple objects

        If a matching adapter cannot be found, returns the default.
        """

    def getMultiAdapter(objects, interface, name=u''):
        """Look for a multi-adapter to an interface for multiple objects

        If a matching adapter cannot be found, a `ComponentLookupError`
        is raised.
        """

    def getAdapters(objects, provided):
        """Look for all matching adapters to a provided interface for objects

        Return an iterable of name-adapter pairs for adapters that
        provide the given interface.
        """

    def subscribers(objects, provided):
        """Get subscribers

        Subscribers are returned that provide the provided interface
        and that depend on and are comuted from the sequence of
        required objects.
        """

    def handle(*objects):
        """Call handlers for the given objects

        Handlers registered for the given objects are called.
        """

    def queryUtility(interface, name='', default=None):
        """Look up a utility that provides an interface.

        If one is not found, returns default.
        """

    def getUtilitiesFor(interface):
        """Look up the registered utilities that provide an interface.

        Returns an iterable of name-utility pairs.
        """

    def getAllUtilitiesRegisteredFor(interface):
        """Return all registered utilities for an interface

        This includes overridden utilities.

        An iterable of utility instances is returned.  No names are
        returned.
        """

class IRegistration(Interface):
    """A registration-information object
    """

    registry = Attribute("The registry having the registration")

    name = Attribute("The registration name")

    info = Attribute("""Information about the registration

    This is information deemed useful to people browsing the
    configuration of a system. It could, for example, include
    commentary or information about the source of the configuration.
    """)

class IUtilityRegistration(IRegistration):
    """Information about the registration of a utility
    """

    factory = Attribute("The factory used to create the utility. Optional.")
    component = Attribute("The object registered")
    provided = Attribute("The interface provided by the component")

class _IBaseAdapterRegistration(IRegistration):
    """Information about the registration of an adapter
    """

    factory = Attribute("The factory used to create adapters")

    required = Attribute("""The adapted interfaces

    This is a sequence of interfaces adapters by the registered
    factory.  The factory will be caled with a sequence of objects, as
    positional arguments, that provide these interfaces.
    """)

    provided = Attribute("""The interface provided by the adapters.

    This interface is implemented by the factory
    """)

class IAdapterRegistration(_IBaseAdapterRegistration):
    """Information about the registration of an adapter
    """

class ISubscriptionAdapterRegistration(_IBaseAdapterRegistration):
    """Information about the registration of a subscription adapter
    """

class IHandlerRegistration(IRegistration):

    handler = Attribute("An object called used to handle an event")

    required = Attribute("""The handled interfaces

    This is a sequence of interfaces handled by the registered
    handler.  The handler will be caled with a sequence of objects, as
    positional arguments, that provide these interfaces.
    """)

class IRegistrationEvent(IObjectEvent):
    """An event that involves a registration"""


@implementer(IRegistrationEvent)
class RegistrationEvent(ObjectEvent):
    """There has been a change in a registration
    """
    def __repr__(self):
        return "%s event:\n%r" % (self.__class__.__name__, self.object)

class IRegistered(IRegistrationEvent):
    """A component or factory was registered
    """

@implementer(IRegistered)
class Registered(RegistrationEvent):
    pass

class IUnregistered(IRegistrationEvent):
    """A component or factory was unregistered
    """

@implementer(IUnregistered)
class Unregistered(RegistrationEvent):
    """A component or factory was unregistered
    """


class IComponentRegistry(Interface):
    """Register components
    """

    def registerUtility(component=None, provided=None, name=u'',
                        info=u'', factory=None):
        """Register a utility

        :param factory:
           Factory for the component to be registered.

        :param component:
           The registered component

        :param provided:
           This is the interface provided by the utility.  If the
           component provides a single interface, then this
           argument is optional and the component-implemented
           interface will be used.

        :param name:
           The utility name.

        :param info:
           An object that can be converted to a string to provide
           information about the registration.

        Only one of *component* and *factory* can be used.

        A `IRegistered` event is generated with an `IUtilityRegistration`.
        """

    def unregisterUtility(component=None, provided=None, name=u'',
                          factory=None):
        """Unregister a utility

        :returns:
            A boolean is returned indicating whether the registry was
            changed.  If the given *component* is None and there is no
            component registered, or if the given *component* is not
            None and is not registered, then the function returns
            False, otherwise it returns True.

        :param factory:
           Factory for the component to be unregistered.

        :param component:
           The registered component The given component can be
           None, in which case any component registered to provide
           the given provided interface with the given name is
           unregistered.

        :param provided:
           This is the interface provided by the utility.  If the
           component is not None and provides a single interface,
           then this argument is optional and the
           component-implemented interface will be used.

        :param name:
           The utility name.

        Only one of *component* and *factory* can be used.
        An `IUnregistered` event is generated with an `IUtilityRegistration`.
        """

    def registeredUtilities():
        """Return an iterable of `IUtilityRegistration` instances.

        These registrations describe the current utility registrations
        in the object.
        """

    def registerAdapter(factory, required=None, provided=None, name=u'',
                        info=u''):
        """Register an adapter factory

        :param factory:
            The object used to compute the adapter

        :param required:
            This is a sequence of specifications for objects to be
            adapted.  If omitted, then the value of the factory's
            ``__component_adapts__`` attribute will be used.  The
            ``__component_adapts__`` attribute is
            normally set in class definitions using
            the `.adapter`
            decorator.  If the factory doesn't have a
            ``__component_adapts__`` adapts attribute, then this
            argument is required.

        :param provided:
            This is the interface provided by the adapter and
            implemented by the factory.  If the factory
            implements a single interface, then this argument is
            optional and the factory-implemented interface will be
            used.

        :param name:
            The adapter name.

        :param info:
           An object that can be converted to a string to provide
           information about the registration.

        A `IRegistered` event is generated with an `IAdapterRegistration`.
        """

    def unregisterAdapter(factory=None, required=None,
                          provided=None, name=u''):
        """Unregister an adapter factory

        :returns:
            A boolean is returned indicating whether the registry was
            changed.  If the given component is None and there is no
            component registered, or if the given component is not
            None and is not registered, then the function returns
            False, otherwise it returns True.

        :param factory:
            This is the object used to compute the adapter. The
            factory can be None, in which case any factory
            registered to implement the given provided interface
            for the given required specifications with the given
            name is unregistered.

        :param required:
            This is a sequence of specifications for objects to be
            adapted.  If the factory is not None and the required
            arguments is omitted, then the value of the factory's
            __component_adapts__ attribute will be used.  The
            __component_adapts__ attribute attribute is normally
            set in class definitions using adapts function, or for
            callables using the adapter decorator.  If the factory
            is None or doesn't have a __component_adapts__ adapts
            attribute, then this argument is required.

        :param provided:
            This is the interface provided by the adapter and
            implemented by the factory.  If the factory is not
            None and implements a single interface, then this
            argument is optional and the factory-implemented
            interface will be used.

        :param name:
            The adapter name.

        An `IUnregistered` event is generated with an `IAdapterRegistration`.
        """

    def registeredAdapters():
        """Return an iterable of `IAdapterRegistration` instances.

        These registrations describe the current adapter registrations
        in the object.
        """

    def registerSubscriptionAdapter(factory, required=None, provides=None,
                                    name=u'', info=''):
        """Register a subscriber factory

        :param factory:
            The object used to compute the adapter

        :param required:
            This is a sequence of specifications for objects to be
            adapted.  If omitted, then the value of the factory's
            ``__component_adapts__`` attribute will be used.  The
            ``__component_adapts__`` attribute is
            normally set using the adapter
            decorator.  If the factory doesn't have a
            ``__component_adapts__`` adapts attribute, then this
            argument is required.

        :param provided:
            This is the interface provided by the adapter and
            implemented by the factory.  If the factory implements
            a single interface, then this argument is optional and
            the factory-implemented interface will be used.

        :param name:
            The adapter name.

            Currently, only the empty string is accepted.  Other
            strings will be accepted in the future when support for
            named subscribers is added.

        :param info:
           An object that can be converted to a string to provide
           information about the registration.

        A `IRegistered` event is generated with an
        `ISubscriptionAdapterRegistration`.
        """

    def unregisterSubscriptionAdapter(factory=None, required=None,
                                      provides=None, name=u''):
        """Unregister a subscriber factory.

        :returns:
            A boolean is returned indicating whether the registry was
            changed.  If the given component is None and there is no
            component registered, or if the given component is not
            None and is not registered, then the function returns
            False, otherwise it returns True.

        :param factory:
            This is the object used to compute the adapter. The
            factory can be None, in which case any factories
            registered to implement the given provided interface
            for the given required specifications with the given
            name are unregistered.

        :param required:
            This is a sequence of specifications for objects to be
            adapted.  If omitted, then the value of the factory's
            ``__component_adapts__`` attribute will be used.  The
            ``__component_adapts__`` attribute is
            normally set using the adapter
            decorator.  If the factory doesn't have a
            ``__component_adapts__`` adapts attribute, then this
            argument is required.

        :param provided:
            This is the interface provided by the adapter and
            implemented by the factory.  If the factory is not
            None implements a single interface, then this argument
            is optional and the factory-implemented interface will
            be used.

        :param name:
            The adapter name.

            Currently, only the empty string is accepted.  Other
            strings will be accepted in the future when support for
            named subscribers is added.

        An `IUnregistered` event is generated with an
        `ISubscriptionAdapterRegistration`.
        """

    def registeredSubscriptionAdapters():
        """Return an iterable of `ISubscriptionAdapterRegistration` instances.

        These registrations describe the current subscription adapter
        registrations in the object.
        """

    def registerHandler(handler, required=None, name=u'', info=''):
        """Register a handler.

        A handler is a subscriber that doesn't compute an adapter
        but performs some function when called.

        :param handler:
            The object used to handle some event represented by
            the objects passed to it.

        :param required:
            This is a sequence of specifications for objects to be
            adapted.  If omitted, then the value of the factory's
            ``__component_adapts__`` attribute will be used.  The
            ``__component_adapts__`` attribute is
            normally set using the adapter
            decorator.  If the factory doesn't have a
            ``__component_adapts__`` adapts attribute, then this
            argument is required.

        :param name:
            The handler name.

            Currently, only the empty string is accepted.  Other
            strings will be accepted in the future when support for
            named handlers is added.

        :param info:
           An object that can be converted to a string to provide
           information about the registration.


        A `IRegistered` event is generated with an `IHandlerRegistration`.
        """

    def unregisterHandler(handler=None, required=None, name=u''):
        """Unregister a handler.

        A handler is a subscriber that doesn't compute an adapter
        but performs some function when called.

        :returns: A boolean is returned indicating whether the registry was
            changed.

        :param handler:
            This is the object used to handle some event
            represented by the objects passed to it. The handler
            can be None, in which case any handlers registered for
            the given required specifications with the given are
            unregistered.

        :param required:
            This is a sequence of specifications for objects to be
            adapted.  If omitted, then the value of the factory's
            ``__component_adapts__`` attribute will be used.  The
            ``__component_adapts__`` attribute is
            normally set using the adapter
            decorator.  If the factory doesn't have a
            ``__component_adapts__`` adapts attribute, then this
            argument is required.

        :param name:
            The handler name.

            Currently, only the empty string is accepted.  Other
            strings will be accepted in the future when support for
            named handlers is added.

        An `IUnregistered` event is generated with an `IHandlerRegistration`.
        """

    def registeredHandlers():
        """Return an iterable of `IHandlerRegistration` instances.

        These registrations describe the current handler registrations
        in the object.
        """


class IComponents(IComponentLookup, IComponentRegistry):
    """Component registration and access
    """


# end formerly in zope.component
