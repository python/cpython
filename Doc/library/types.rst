:mod:`types` --- Dynamic type creation and names for built-in types
===================================================================

.. module:: types
   :synopsis: Names for built-in types.

**Source code:** :source:`Lib/types.py`

--------------

This module defines utility function to assist in dynamic creation of
new types.

It also defines names for some object types that are used by the standard
Python interpreter, but not exposed as builtins like :class:`int` or
:class:`str` are.


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

.. seealso::

   :ref:`metaclasses`
      Full details of the class creation process supported by these functions

   :pep:`3115` - Metaclasses in Python 3000
      Introduced the ``__prepare__`` namespace hook


Standard Interpreter Types
--------------------------

This module provides names for many of the types that are required to
implement a Python interpreter. It deliberately avoids including some of
the types that arise only incidentally during processing such as the
``listiterator`` type.

Typical use is of these names is for :func:`isinstance` or
:func:`issubclass` checks.

Standard names are defined for the following types:

.. data:: FunctionType
          LambdaType

   The type of user-defined functions and functions created by
   :keyword:`lambda`  expressions.


.. data:: GeneratorType

   The type of :term:`generator`-iterator objects, produced by calling a
   generator function.


.. data:: CodeType

   .. index:: builtin: compile

   The type for code objects such as returned by :func:`compile`.


.. data:: MethodType

   The type of methods of user-defined class instances.


.. data:: BuiltinFunctionType
          BuiltinMethodType

   The type of built-in functions like :func:`len` or :func:`sys.exit`, and
   methods of built-in classes.  (Here, the term "built-in" means "written in
   C".)


.. data:: ModuleType

   The type of modules.


.. data:: TracebackType

   The type of traceback objects such as found in ``sys.exc_info()[2]``.


.. data:: FrameType

   The type of frame objects such as found in ``tb.tb_frame`` if ``tb`` is a
   traceback object.


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


.. class:: SimpleNamespace

   A simple :class:`object` subclass that provides attribute access to its
   namespace, as well as a meaningful repr.

   Unlike :class:`object`, with ``SimpleNamespace`` you can add and remove
   attributes.  If a ``SimpleNamespace`` object is initialized with keyword
   arguments, those are directly added to the underlying namespace.

   The type is roughly equivalent to the following code::

       class SimpleNamespace:
           def __init__(self, **kwargs):
               self.__dict__.update(kwargs)
           def __repr__(self):
               keys = sorted(self.__dict__)
               items = ("{}={!r}".format(k, self.__dict__[k]) for k in keys)
               return "{}({})".format(type(self).__name__, ", ".join(items))

   ``SimpleNamespace`` may be useful as a replacement for ``class NS: pass``.
   However, for a structured record type use :func:`~collections.namedtuple`
   instead.

   .. versionadded:: 3.3
