:mod:`types` --- Dynamic type creation and names for built-in types
===================================================================

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
   in ``lambda ns: ns``.

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
   an ``__mro_entries__`` method is replaced with an unpacked result of
   calling this method.  If a *bases* item is an instance of :class:`type`,
   or it doesn't have an ``__mro_entries__`` method, then it is included in
   the return tuple unchanged.

   .. versionadded:: 3.7

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

.. data:: FunctionType
          LambdaType

   The type of user-defined functions and functions created by
   :keyword:`lambda`  expressions.


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

   .. index:: builtin: compile

   The type for code objects such as returned by :func:`compile`.

   .. audit-event:: code.__new__ code,filename,name,argcount,posonlyargcount,kwonlyargcount,nlocals,stacksize,flags CodeType

   Note that the audited arguments may not match the names or positions
   required by the initializer.

   .. method:: CodeType.replace(**kwargs)

     Return a copy of the code object with new values for the specified fields.

     .. versionadded:: 3.8

.. data:: CellType

   The type for cell objects: such objects are used as containers for
   a function's free variables.

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


.. data:: MethodDescriptorType

   The type of methods of some built-in data types such as :meth:`str.join`.

   .. versionadded:: 3.7


.. data:: ClassMethodDescriptorType

   The type of *unbound* class methods of some built-in data types such as
   ``dict.__dict__['fromkeys']``.

   .. versionadded:: 3.7


.. class:: ModuleType(name, doc=None)

   The type of :term:`modules <module>`. Constructor takes the name of the
   module to be created and optionally its :term:`docstring`.

   .. note::
      Use :func:`importlib.util.module_from_spec` to create a new module if you
      wish to set the various import-controlled attributes.

   .. attribute:: __doc__

      The :term:`docstring` of the module. Defaults to ``None``.

   .. attribute:: __loader__

      The :term:`loader` which loaded the module. Defaults to ``None``.

      .. versionchanged:: 3.4
         Defaults to ``None``. Previously the attribute was optional.

   .. attribute:: __name__

      The name of the module.

   .. attribute:: __package__

      Which :term:`package` a module belongs to. If the module is top-level
      (i.e. not a part of any specific package) then the attribute should be set
      to ``''``, else it should be set to the name of the package (which can be
      :attr:`__name__` if the module is a package itself). Defaults to ``None``.

      .. versionchanged:: 3.4
         Defaults to ``None``. Previously the attribute was optional.


.. class:: TracebackType(tb_next, tb_frame, tb_lasti, tb_lineno)

   The type of traceback objects such as found in ``sys.exc_info()[2]``.

   See :ref:`the language reference <traceback-objects>` for details of the
   available attributes and operations, and guidance on creating tracebacks
   dynamically.


.. data:: FrameType

   The type of frame objects such as found in ``tb.tb_frame`` if ``tb`` is a
   traceback object.

   See :ref:`the language reference <frame-objects>` for details of the
   available attributes and operations.


.. data:: GetSetDescriptorType

   The type of objects defined in extension modules with ``PyGetSetDef``, such
   as ``FrameType.f_locals`` or ``array.array.typecode``.  This type is used as
   descriptor for object attributes; it has the same purpose as the
   :class:`property` type, but for classes defined in extension modules.


.. data:: MemberDescriptorType

   The type of objects defined in extension modules with ``PyMemberDef``, such
   as ``datetime.timedelta.days``.  This type is used as descriptor for simple C
   data members which use standard conversion functions; it has the same purpose
   as the :class:`property` type, but for classes defined in extension modules.

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


Additional Utility Classes and Functions
----------------------------------------

.. class:: SimpleNamespace

   A simple :class:`object` subclass that provides attribute access to its
   namespace, as well as a meaningful repr.

   Unlike :class:`object`, with ``SimpleNamespace`` you can add and remove
   attributes.  If a ``SimpleNamespace`` object is initialized with keyword
   arguments, those are directly added to the underlying namespace.

   The type is roughly equivalent to the following code::

       class SimpleNamespace:
           def __init__(self, /, **kwargs):
               self.__dict__.update(kwargs)

           def __repr__(self):
               keys = sorted(self.__dict__)
               items = ("{}={!r}".format(k, self.__dict__[k]) for k in keys)
               return "{}({})".format(type(self).__name__, ", ".join(items))

           def __eq__(self, other):
               return self.__dict__ == other.__dict__

   ``SimpleNamespace`` may be useful as a replacement for ``class NS: pass``.
   However, for a structured record type use :func:`~collections.namedtuple`
   instead.

   .. versionadded:: 3.3


.. function:: DynamicClassAttribute(fget=None, fset=None, fdel=None, doc=None)

   Route attribute access on a class to __getattr__.

   This is a descriptor, used to define attributes that act differently when
   accessed through an instance and through a class.  Instance access remains
   normal, but access to an attribute through a class will be routed to the
   class's __getattr__ method; this is done by raising AttributeError.

   This allows one to have properties active on an instance, and have virtual
   attributes on the class with the same name (see Enum for an example).

   .. versionadded:: 3.4


Coroutine Utility Functions
---------------------------

.. function:: coroutine(gen_func)

   This function transforms a :term:`generator` function into a
   :term:`coroutine function` which returns a generator-based coroutine.
   The generator-based coroutine is still a :term:`generator iterator`,
   but is also considered to be a :term:`coroutine` object and is
   :term:`awaitable`.  However, it may not necessarily implement
   the :meth:`__await__` method.

   If *gen_func* is a generator function, it will be modified in-place.

   If *gen_func* is not a generator function, it will be wrapped. If it
   returns an instance of :class:`collections.abc.Generator`, the instance
   will be wrapped in an *awaitable* proxy object.  All other types
   of objects will be returned as is.

   .. versionadded:: 3.5
