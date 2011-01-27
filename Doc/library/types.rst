:mod:`types` --- Names for built-in types
=========================================

.. module:: types
   :synopsis: Names for built-in types.

**Source code:** :source:`Lib/types.py`

--------------

This module defines names for some object types that are used by the standard
Python interpreter, but not exposed as builtins like :class:`int` or
:class:`str` are.  Also, it does not include some of the types that arise
transparently during processing such as the ``listiterator`` type.

Typical use is for :func:`isinstance` or :func:`issubclass` checks.

The module defines the following names:

.. data:: FunctionType
          LambdaType

   The type of user-defined functions and functions created by :keyword:`lambda`
   expressions.


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
