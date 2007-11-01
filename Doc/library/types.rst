
:mod:`types` --- Names for built-in types
=========================================

.. module:: types
   :synopsis: Names for built-in types.


This module defines names for some object types that are used by the standard
Python interpreter, but not for the types defined by various extension modules.
Also, it does not include some of the types that arise during processing such as
the ``listiterator`` type. New names exported by future versions of this module
will all end in ``Type``.

Typical use is for functions that do different things depending on their
argument types, like the following::

   from types import IntType
   def delete(mylist, item):
       if type(item) is IntType:
          del mylist[item]
       else:
          mylist.remove(item)

Starting in Python 2.2, built-in factory functions such as :func:`int` and
:func:`str` are also names for the corresponding types.  This is now the
preferred way to access the type instead of using the :mod:`types` module.
Accordingly, the example above should be written as follows::

   def delete(mylist, item):
       if isinstance(item, int):
          del mylist[item]
       else:
          mylist.remove(item)

The module defines the following names:


.. data:: NoneType

   The type of ``None``.


.. data:: TypeType
          ClassType

   .. index:: builtin: type

   The type of type objects (such as returned by :func:`type`) and user-defined
   classes without metaclass; alias of the built-in :class:`type`.


.. data:: ObjectType

   Alias of the built-in :func:`object`.


.. data:: BooleanType

   The type of the :class:`bool` values ``True`` and ``False``; alias of the
   built-in :class:`bool`.


.. data:: IntType
          LongType

   The type of integers (e.g. ``1``); alias of the built-in :class:`int`.


.. data:: FloatType

   The type of floating point numbers (e.g. ``1.0``); alias of the built-in
   :class:`float`.


.. data:: ComplexType

   The type of complex numbers (e.g. ``1.0j``); alias of the built-in
   :class:`complex`.  This is not defined if Python was built without complex
   number support.


.. data:: StringType

   The type of character strings (e.g. ``'Spam'``); alias of the built-in
   :class:`str`.


.. data:: TupleType

   The type of tuples (e.g. ``(1, 2, 3, 'Spam')``); alias of the built-in
   :class:`tuple`.


.. data:: ListType

   The type of lists (e.g. ``[0, 1, 2, 3]``); alias of the built-in
   :class:`list`.


.. data:: DictType
          DictionaryType

   The type of dictionaries (e.g. ``{'Bacon': 1, 'Ham': 0}``); alias of the
   built-in :class:`dict`.


.. data:: FunctionType
          LambdaType

   The type of user-defined functions and lambdas.


.. data:: GeneratorType

   The type of :term:`generator`-iterator objects, produced by calling a
   generator function.


.. data:: CodeType

   .. index:: builtin: compile

   The type for code objects such as returned by :func:`compile`.


.. data:: MethodType
          UnboundMethdType

   The type of methods of user-defined class instances.


.. data:: BuiltinFunctionType
          BuiltinMethodType

   The type of built-in functions like :func:`len` or :func:`sys.exit`.


.. data:: ModuleType

   The type of modules.


.. data:: SliceType

   .. index:: builtin: slice

   The type of objects returned by :func:`slice`; alias of the built-in
   :class:`slice`.


.. data:: EllipsisType

   The type of ``Ellipsis``.


.. data:: TracebackType

   The type of traceback objects such as found in ``sys.exc_info()[2]``.


.. data:: FrameType

   The type of frame objects such as found in ``tb.tb_frame`` if ``tb`` is a
   traceback object.


.. XXX!
.. data:: BufferType

   .. index:: builtin: buffer

   The type of buffer objects created by the :func:`buffer` function.


.. data:: DictProxyType

   The type of dict proxies, such as ``TypeType.__dict__``.


.. data:: NotImplementedType

   The type of ``NotImplemented``


.. data:: GetSetDescriptorType

   The type of objects defined in extension modules with ``PyGetSetDef``, such as
   ``FrameType.f_locals`` or ``array.array.typecode``.  This constant is not
   defined in implementations of Python that do not have such extension types, so
   for portable code use ``hasattr(types, 'GetSetDescriptorType')``.


.. data:: MemberDescriptorType

   The type of objects defined in extension modules with ``PyMemberDef``, such as
   ``datetime.timedelta.days``.  This constant is not defined in implementations of
   Python that do not have such extension types, so for portable code use
   ``hasattr(types, 'MemberDescriptorType')``.
