:mod:`types` --- Names for built-in types
=========================================

.. module:: types
   :synopsis: Names for built-in types.

**Source code:** :source:`Lib/types.py`

--------------

This module defines names for some object types that are used by the standard
Python interpreter, but not for the types defined by various extension modules.
Also, it does not include some of the types that arise during processing such as
the ``listiterator`` type. It is safe to use ``from types import *`` --- the
module does not export any names besides the ones listed here. New names
exported by future versions of this module will all end in ``Type``.

Typical use is for functions that do different things depending on their
argument types, like the following::

   from types import *
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

   .. index:: builtin: type

   The type of type objects (such as returned by :func:`type`); alias of the
   built-in :class:`type`.


.. data:: BooleanType

   The type of the :class:`bool` values ``True`` and ``False``; alias of the
   built-in :class:`bool`.

   .. versionadded:: 2.3


.. data:: IntType

   The type of integers (e.g. ``1``); alias of the built-in :class:`int`.


.. data:: LongType

   The type of long integers (e.g. ``1L``); alias of the built-in :class:`long`.


.. data:: FloatType

   The type of floating point numbers (e.g. ``1.0``); alias of the built-in
   :class:`float`.


.. data:: ComplexType

   The type of complex numbers (e.g. ``1.0j``).  This is not defined if Python was
   built without complex number support.


.. data:: StringType

   The type of character strings (e.g. ``'Spam'``); alias of the built-in
   :class:`str`.


.. data:: UnicodeType

   The type of Unicode character strings (e.g. ``u'Spam'``).  This is not defined
   if Python was built without Unicode support.  It's an alias of the built-in
   :class:`unicode`.


.. data:: TupleType

   The type of tuples (e.g. ``(1, 2, 3, 'Spam')``); alias of the built-in
   :class:`tuple`.


.. data:: ListType

   The type of lists (e.g. ``[0, 1, 2, 3]``); alias of the built-in
   :class:`list`.


.. data:: DictType

   The type of dictionaries (e.g. ``{'Bacon': 1, 'Ham': 0}``); alias of the
   built-in :class:`dict`.


.. data:: DictionaryType

   An alternate name for ``DictType``.


.. data:: FunctionType
          LambdaType

   The type of user-defined functions and functions created by :keyword:`lambda`
   expressions.


.. data:: GeneratorType

   The type of :term:`generator`-iterator objects, produced by calling a
   generator function.

   .. versionadded:: 2.2


.. data:: CodeType

   .. index:: builtin: compile

   The type for code objects such as returned by :func:`compile`.


.. data:: ClassType

   The type of user-defined old-style classes.


.. data:: InstanceType

   The type of instances of user-defined old-style classes.


.. data:: MethodType

   The type of methods of user-defined class instances.


.. data:: UnboundMethodType

   An alternate name for ``MethodType``.


.. data:: BuiltinFunctionType
          BuiltinMethodType

   The type of built-in functions like :func:`len` or :func:`sys.exit`, and
   methods of built-in classes.  (Here, the term "built-in" means "written in
   C".)


.. data:: ModuleType

   The type of modules.


.. data:: FileType

   The type of open file objects such as ``sys.stdout``; alias of the built-in
   :class:`file`.


.. data:: XRangeType

   .. index:: builtin: xrange

   The type of range objects returned by :func:`xrange`; alias of the built-in
   :class:`xrange`.


.. data:: SliceType

   .. index:: builtin: slice

   The type of objects returned by :func:`slice`; alias of the built-in
   :class:`slice`.


.. data:: EllipsisType

   The type of ``Ellipsis``.


.. data:: TracebackType

   The type of traceback objects such as found in ``sys.exc_traceback``.


.. data:: FrameType

   The type of frame objects such as found in ``tb.tb_frame`` if ``tb`` is a
   traceback object.


.. data:: BufferType

   .. index:: builtin: buffer

   The type of buffer objects created by the :func:`buffer` function.


.. data:: DictProxyType

   The type of dict proxies, such as ``TypeType.__dict__``.


.. data:: NotImplementedType

   The type of ``NotImplemented``


.. data:: GetSetDescriptorType

   The type of objects defined in extension modules with ``PyGetSetDef``, such
   as ``FrameType.f_locals`` or ``array.array.typecode``.  This type is used as
   descriptor for object attributes; it has the same purpose as the
   :class:`property` type, but for classes defined in extension modules.

   .. versionadded:: 2.5


.. data:: MemberDescriptorType

   The type of objects defined in extension modules with ``PyMemberDef``, such
   as ``datetime.timedelta.days``.  This type is used as descriptor for simple C
   data members which use standard conversion functions; it has the same purpose
   as the :class:`property` type, but for classes defined in extension modules.

   .. impl-detail::

      In other implementations of Python, this type may be identical to
      ``GetSetDescriptorType``.

   .. versionadded:: 2.5


.. data:: StringTypes

   A sequence containing ``StringType`` and ``UnicodeType`` used to facilitate
   easier checking for any string object.  Using this is more portable than using a
   sequence of the two string types constructed elsewhere since it only contains
   ``UnicodeType`` if it has been built in the running version of Python.  For
   example: ``isinstance(s, types.StringTypes)``.

   .. versionadded:: 2.2
