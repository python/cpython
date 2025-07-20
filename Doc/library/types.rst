:mod:`!types` --- Dynamic type creation and names for built-in types
====================================================================

.. module:: types
   :synopsis: Names for built-in types.

**Source code:** :source:`Lib/types.py`

--------------

This module defines utility functions to assist in dynamic creation of
new types.

It also defines names for some object types that are used by the standard
Python interpreter, but not exposed as builtins like :class:`int` or
:class:`str` are.

Finally, it provides some additional type-related utility classes and functions
that are not fundamental enough to be builtins.


Dynamic Type Creation
---------------------

.. function:: new_class(name, bases=(), kwds=None, exec_body=None)

   Creates a class object dynamically using the appropriate metaclass.

   The first three arguments are the components that make up a class
   definition header: the class name, the base classes (in order), the
   keyword arguments (such as ``metaclass``).

   The *exec_body* argument is a callback that is used to populate the
   freshly created class namespace. It should accept the class namespace
   as its sole argument and update the namespace directly with the class
   contents. If no callback is provided, it has the same effect as passing
   in ``lambda ns: None``.

   .. versionadded:: 3.3

.. function:: prepare_class(name, bases=(), kwds=None)

   Calculates the appropriate metaclass and creates the class namespace.

   The arguments are the components that make up a class definition header:
   the class name, the base classes (in order) and the keyword arguments
   (such as ``metaclass``).

   The return value is a 3-tuple: ``metaclass, namespace, kwds``

   *metaclass* is the appropriate metaclass, *namespace* is the
   prepared class namespace and *kwds* is an updated copy of the passed
   in *kwds* argument with any ``'metaclass'`` entry removed. If no *kwds*
   argument is passed in, this will be an empty dict.

   .. versionadded:: 3.3

   .. versionchanged:: 3.6

      The default value for the ``namespace`` element of the returned
      tuple has changed.  Now an insertion-order-preserving mapping is
      used when the metaclass does not have a ``__prepare__`` method.

.. seealso::

   :ref:`metaclasses`
      Full details of the class creation process supported by these functions

   :pep:`3115` - Metaclasses in Python 3000
      Introduced the ``__prepare__`` namespace hook

.. function:: resolve_bases(bases)

   Resolve MRO entries dynamically as specified by :pep:`560`.

   This function looks for items in *bases* that are not instances of
   :class:`type`, and returns a tuple where each such object that has
   an :meth:`~object.__mro_entries__` method is replaced with an unpacked result of
   calling this method.  If a *bases* item is an instance of :class:`type`,
   or it doesn't have an :meth:`!__mro_entries__` method, then it is included in
   the return tuple unchanged.

   .. versionadded:: 3.7

.. function:: get_original_bases(cls, /)

    Return the tuple of objects originally given as the bases of *cls* before
    the :meth:`~object.__mro_entries__` method has been called on any bases
    (following the mechanisms laid out in :pep:`560`). This is useful for
    introspecting :ref:`Generics <user-defined-generics>`.

    For classes that have an ``__orig_bases__`` attribute, this
    function returns the value of ``cls.__orig_bases__``.
    For classes without the ``__orig_bases__`` attribute,
    :attr:`cls.__bases__ <type.__bases__>` is returned.

    Examples::

        from typing import TypeVar, Generic, NamedTuple, TypedDict

        T = TypeVar("T")
        class Foo(Generic[T]): ...
        class Bar(Foo[int], float): ...
        class Baz(list[str]): ...
        Eggs = NamedTuple("Eggs", [("a", int), ("b", str)])
        Spam = TypedDict("Spam", {"a": int, "b": str})

        assert Bar.__bases__ == (Foo, float)
        assert get_original_bases(Bar) == (Foo[int], float)

        assert Baz.__bases__ == (list,)
        assert get_original_bases(Baz) == (list[str],)

        assert Eggs.__bases__ == (tuple,)
        assert get_original_bases(Eggs) == (NamedTuple,)

        assert Spam.__bases__ == (dict,)
        assert get_original_bases(Spam) == (TypedDict,)

        assert int.__bases__ == (object,)
        assert get_original_bases(int) == (object,)

    .. versionadded:: 3.12

.. seealso::

   :pep:`560` - Core support for typing module and generic types


Standard Interpreter Types
--------------------------

This module provides names for many of the types that are required to
implement a Python interpreter. It deliberately avoids including some of
the types that arise only incidentally during processing such as the
``listiterator`` type.

Typical use of these names is for :func:`isinstance` or
:func:`issubclass` checks.


If you instantiate any of these types, note that signatures may vary between Python versions.

Standard names are defined for the following types:

.. data:: NoneType

   The type of :data:`None`.

   .. versionadded:: 3.10


.. data:: FunctionType
          LambdaType

   The type of user-defined functions and functions created by
   :keyword:`lambda`  expressions.

   .. audit-event:: function.__new__ code types.FunctionType

   The audit event only occurs for direct instantiation of function objects,
   and is not raised for normal compilation.


.. data:: GeneratorType

   The type of :term:`generator`-iterator objects, created by
   generator functions.


.. data:: CoroutineType

   The type of :term:`coroutine` objects, created by
   :keyword:`async def` functions.

   .. versionadded:: 3.5


.. data:: AsyncGeneratorType

   The type of :term:`asynchronous generator`-iterator objects, created by
   asynchronous generator functions.

   .. versionadded:: 3.6


.. class:: CodeType(**kwargs)

   .. index:: pair: built-in function; compile

   The type of :ref:`code objects <code-objects>` such as returned by :func:`compile`.

   .. audit-event:: code.__new__ code,filename,name,argcount,posonlyargcount,kwonlyargcount,nlocals,stacksize,flags types.CodeType

   Note that the audited arguments may not match the names or positions
   required by the initializer.  The audit event only occurs for direct
   instantiation of code objects, and is not raised for normal compilation.

.. data:: CellType

   The type for cell objects: such objects are used as containers for
   a function's :term:`closure variables <closure variable>`.

   .. versionadded:: 3.8


.. data:: MethodType

   The type of methods of user-defined class instances.


.. data:: BuiltinFunctionType
          BuiltinMethodType

   The type of built-in functions like :func:`len` or :func:`sys.exit`, and
   methods of built-in classes.  (Here, the term "built-in" means "written in
   C".)


.. data:: WrapperDescriptorType

   The type of methods of some built-in data types and base classes such as
   :meth:`object.__init__` or :meth:`object.__lt__`.

   .. versionadded:: 3.7


.. data:: MethodWrapperType

   The type of *bound* methods of some built-in data types and base classes.
   For example it is the type of :code:`object().__str__`.

   .. versionadded:: 3.7


.. data:: NotImplementedType

   The type of :data:`NotImplemented`.

   .. versionadded:: 3.10


.. data:: MethodDescriptorType

   The type of methods of some built-in data types such as :meth:`str.join`.

   .. versionadded:: 3.7


.. data:: ClassMethodDescriptorType

   The type of *unbound* class methods of some built-in data types such as
   ``dict.__dict__['fromkeys']``.

   .. versionadded:: 3.7


.. class:: ModuleType(name, doc=None)

   The type of :term:`modules <module>`. The constructor takes the name of the
   module to be created and optionally its :term:`docstring`.

   .. seealso::

      :ref:`Documentation on module objects <module-objects>`
         Provides details on the special attributes that can be found on
         instances of :class:`!ModuleType`.

      :func:`importlib.util.module_from_spec`
         Modules created using the :class:`!ModuleType` constructor are
         created with many of their special attributes unset or set to default
         values. :func:`!module_from_spec` provides a more robust way of
         creating :class:`!ModuleType` instances which ensures the various
         attributes are set appropriately.

.. data:: EllipsisType

   The type of :data:`Ellipsis`.

   .. versionadded:: 3.10

.. class:: GenericAlias(t_origin, t_args)

   The type of :ref:`parameterized generics <types-genericalias>` such as
   ``list[int]``.

   ``t_origin`` should be a non-parameterized generic class, such as ``list``,
   ``tuple`` or ``dict``.  ``t_args`` should be a :class:`tuple` (possibly of
   length 1) of types which parameterize ``t_origin``::

      >>> from types import GenericAlias

      >>> list[int] == GenericAlias(list, (int,))
      True
      >>> dict[str, int] == GenericAlias(dict, (str, int))
      True

   .. versionadded:: 3.9

   .. versionchanged:: 3.9.2
      This type can now be subclassed.

   .. seealso::

      :ref:`Generic Alias Types<types-genericalias>`
         In-depth documentation on instances of :class:`!types.GenericAlias`

      :pep:`585` - Type Hinting Generics In Standard Collections
         Introducing the :class:`!types.GenericAlias` class

.. class:: UnionType

   The type of :ref:`union type expressions<types-union>`.

   .. versionadded:: 3.10

   .. versionchanged:: 3.14

      This is now an alias for :class:`typing.Union`.

.. class:: TracebackType(tb_next, tb_frame, tb_lasti, tb_lineno)

   The type of traceback objects such as found in ``sys.exception().__traceback__``.

   See :ref:`the language reference <traceback-objects>` for details of the
   available attributes and operations, and guidance on creating tracebacks
   dynamically.


.. data:: FrameType

   The type of :ref:`frame objects <frame-objects>` such as found in
   :attr:`tb.tb_frame <traceback.tb_frame>` if ``tb`` is a traceback object.


.. data:: GetSetDescriptorType

   The type of objects defined in extension modules with ``PyGetSetDef``, such
   as :attr:`FrameType.f_locals <frame.f_locals>` or ``array.array.typecode``.
   This type is used as
   descriptor for object attributes; it has the same purpose as the
   :class:`property` type, but for classes defined in extension modules.


.. data:: MemberDescriptorType

   The type of objects defined in extension modules with ``PyMemberDef``, such
   as ``datetime.timedelta.days``.  This type is used as descriptor for simple C
   data members which use standard conversion functions; it has the same purpose
   as the :class:`property` type, but for classes defined in extension modules.

   In addition, when a class is defined with a :attr:`~object.__slots__` attribute, then for
   each slot, an instance of :class:`!MemberDescriptorType` will be added as an attribute
   on the class. This allows the slot to appear in the class's :attr:`~type.__dict__`.

   .. impl-detail::

      In other implementations of Python, this type may be identical to
      ``GetSetDescriptorType``.

.. class:: MappingProxyType(mapping)

   Read-only proxy of a mapping. It provides a dynamic view on the mapping's
   entries, which means that when the mapping changes, the view reflects these
   changes.

   .. versionadded:: 3.3

   .. versionchanged:: 3.9

      Updated to support the new union (``|``) operator from :pep:`584`, which
      simply delegates to the underlying mapping.

   .. describe:: key in proxy

      Return ``True`` if the underlying mapping has a key *key*, else
      ``False``.

   .. describe:: proxy[key]

      Return the item of the underlying mapping with key *key*.  Raises a
      :exc:`KeyError` if *key* is not in the underlying mapping.

   .. describe:: iter(proxy)

      Return an iterator over the keys of the underlying mapping.  This is a
      shortcut for ``iter(proxy.keys())``.

   .. describe:: len(proxy)

      Return the number of items in the underlying mapping.

   .. method:: copy()

      Return a shallow copy of the underlying mapping.

   .. method:: get(key[, default])

      Return the value for *key* if *key* is in the underlying mapping, else
      *default*.  If *default* is not given, it defaults to ``None``, so that
      this method never raises a :exc:`KeyError`.

   .. method:: items()

      Return a new view of the underlying mapping's items (``(key, value)``
      pairs).

   .. method:: keys()

      Return a new view of the underlying mapping's keys.

   .. method:: values()

      Return a new view of the underlying mapping's values.

   .. describe:: reversed(proxy)

      Return a reverse iterator over the keys of the underlying mapping.

      .. versionadded:: 3.9

   .. describe:: hash(proxy)

      Return a hash of the underlying mapping.

      .. versionadded:: 3.12

.. class:: CapsuleType

   The type of :ref:`capsule objects <capsules>`.

   .. versionadded:: 3.13


Additional Utility Classes and Functions
----------------------------------------

.. class:: SimpleNamespace

   A simple :class:`object` subclass that provides attribute access to its
   namespace, as well as a meaningful repr.

   Unlike :class:`object`, with :class:`!SimpleNamespace` you can add and remove
   attributes.

   :py:class:`SimpleNamespace` objects may be initialized
   in the same way as :class:`dict`: either with keyword arguments,
   with a single positional argument, or with both.
   When initialized with keyword arguments,
   those are directly added to the underlying namespace.
   Alternatively, when initialized with a positional argument,
   the underlying namespace will be updated with key-value pairs
   from that argument (either a mapping object or
   an :term:`iterable` object producing key-value pairs).
   All such keys must be strings.

   The type is roughly equivalent to the following code::

       class SimpleNamespace:
           def __init__(self, mapping_or_iterable=(), /, **kwargs):
               self.__dict__.update(mapping_or_iterable)
               self.__dict__.update(kwargs)

           def __repr__(self):
               items = (f"{k}={v!r}" for k, v in self.__dict__.items())
               return "{}({})".format(type(self).__name__, ", ".join(items))

           def __eq__(self, other):
               if isinstance(self, SimpleNamespace) and isinstance(other, SimpleNamespace):
                  return self.__dict__ == other.__dict__
               return NotImplemented

   ``SimpleNamespace`` may be useful as a replacement for ``class NS: pass``.
   However, for a structured record type use :func:`~collections.namedtuple`
   instead.

   :class:`!SimpleNamespace` objects are supported by :func:`copy.replace`.

   .. versionadded:: 3.3

   .. versionchanged:: 3.9
      Attribute order in the repr changed from alphabetical to insertion (like
      ``dict``).

   .. versionchanged:: 3.13
      Added support for an optional positional argument.

.. function:: DynamicClassAttribute(fget=None, fset=None, fdel=None, doc=None)

   Route attribute access on a class to __getattr__.

   This is a descriptor, used to define attributes that act differently when
   accessed through an instance and through a class.  Instance access remains
   normal, but access to an attribute through a class will be routed to the
   class's __getattr__ method; this is done by raising AttributeError.

   This allows one to have properties active on an instance, and have virtual
   attributes on the class with the same name (see :class:`enum.Enum` for an example).

   .. versionadded:: 3.4


Coroutine Utility Functions
---------------------------

.. function:: coroutine(gen_func)

   This function transforms a :term:`generator` function into a
   :term:`coroutine function` which returns a generator-based coroutine.
   The generator-based coroutine is still a :term:`generator iterator`,
   but is also considered to be a :term:`coroutine` object and is
   :term:`awaitable`.  However, it may not necessarily implement
   the :meth:`~object.__await__` method.

   If *gen_func* is a generator function, it will be modified in-place.

   If *gen_func* is not a generator function, it will be wrapped. If it
   returns an instance of :class:`collections.abc.Generator`, the instance
   will be wrapped in an *awaitable* proxy object.  All other types
   of objects will be returned as is.

   .. versionadded:: 3.5
