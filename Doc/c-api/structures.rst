.. highlight:: c

.. _common-structs:

Common Object Structures
========================

There are a large number of structures which are used in the definition of
object types for Python.  This section describes these structures and how they
are used.


Base object types and macros
----------------------------

All Python objects ultimately share a small number of fields at the beginning
of the object's representation in memory.  These are represented by the
:c:type:`PyObject` and :c:type:`PyVarObject` types, which are defined, in turn,
by the expansions of some macros also used, whether directly or indirectly, in
the definition of all other Python objects.  Additional macros can be found
under :ref:`reference counting <countingrefs>`.


.. c:type:: PyObject

   All object types are extensions of this type.  This is a type which
   contains the information Python needs to treat a pointer to an object as an
   object.  In a normal "release" build, it contains only the object's
   reference count and a pointer to the corresponding type object.
   Nothing is actually declared to be a :c:type:`PyObject`, but every pointer
   to a Python object can be cast to a :c:expr:`PyObject*`.  Access to the
   members must be done by using the macros :c:macro:`Py_REFCNT` and
   :c:macro:`Py_TYPE`.


.. c:type:: PyVarObject

   This is an extension of :c:type:`PyObject` that adds the :c:member:`~PyVarObject.ob_size`
   field.  This is only used for objects that have some notion of *length*.
   This type does not often appear in the Python/C API.
   Access to the members must be done by using the macros
   :c:macro:`Py_REFCNT`, :c:macro:`Py_TYPE`, and :c:macro:`Py_SIZE`.


.. c:macro:: PyObject_HEAD

   This is a macro used when declaring new types which represent objects
   without a varying length.  The PyObject_HEAD macro expands to::

      PyObject ob_base;

   See documentation of :c:type:`PyObject` above.


.. c:macro:: PyObject_VAR_HEAD

   This is a macro used when declaring new types which represent objects
   with a length that varies from instance to instance.
   The PyObject_VAR_HEAD macro expands to::

      PyVarObject ob_base;

   See documentation of :c:type:`PyVarObject` above.


.. c:var:: PyTypeObject PyBaseObject_Type

   The base class of all other objects, the same as :class:`object` in Python.


.. c:function:: int Py_Is(PyObject *x, PyObject *y)

   Test if the *x* object is the *y* object, the same as ``x is y`` in Python.

   .. versionadded:: 3.10


.. c:function:: int Py_IsNone(PyObject *x)

   Test if an object is the ``None`` singleton,
   the same as ``x is None`` in Python.

   .. versionadded:: 3.10


.. c:function:: int Py_IsTrue(PyObject *x)

   Test if an object is the ``True`` singleton,
   the same as ``x is True`` in Python.

   .. versionadded:: 3.10


.. c:function:: int Py_IsFalse(PyObject *x)

   Test if an object is the ``False`` singleton,
   the same as ``x is False`` in Python.

   .. versionadded:: 3.10


.. c:function:: PyTypeObject* Py_TYPE(PyObject *o)

   Get the type of the Python object *o*.

   Return a :term:`borrowed reference`.

   Use the :c:func:`Py_SET_TYPE` function to set an object type.

   .. versionchanged:: 3.11
      :c:func:`Py_TYPE()` is changed to an inline static function.
      The parameter type is no longer :c:expr:`const PyObject*`.


.. c:function:: int Py_IS_TYPE(PyObject *o, PyTypeObject *type)

   Return non-zero if the object *o* type is *type*. Return zero otherwise.
   Equivalent to: ``Py_TYPE(o) == type``.

   .. versionadded:: 3.9


.. c:function:: void Py_SET_TYPE(PyObject *o, PyTypeObject *type)

   Set the object *o* type to *type*.

   .. versionadded:: 3.9


.. c:function:: Py_ssize_t Py_SIZE(PyVarObject *o)

   Get the size of the Python object *o*.

   Use the :c:func:`Py_SET_SIZE` function to set an object size.

   .. versionchanged:: 3.11
      :c:func:`Py_SIZE()` is changed to an inline static function.
      The parameter type is no longer :c:expr:`const PyVarObject*`.


.. c:function:: void Py_SET_SIZE(PyVarObject *o, Py_ssize_t size)

   Set the object *o* size to *size*.

   .. versionadded:: 3.9


.. c:macro:: PyObject_HEAD_INIT(type)

   This is a macro which expands to initialization values for a new
   :c:type:`PyObject` type.  This macro expands to::

      _PyObject_EXTRA_INIT
      1, type,


.. c:macro:: PyVarObject_HEAD_INIT(type, size)

   This is a macro which expands to initialization values for a new
   :c:type:`PyVarObject` type, including the :c:member:`~PyVarObject.ob_size` field.
   This macro expands to::

      _PyObject_EXTRA_INIT
      1, type, size,


Implementing functions and methods
----------------------------------

.. c:type:: PyCFunction

   Type of the functions used to implement most Python callables in C.
   Functions of this type take two :c:expr:`PyObject*` parameters and return
   one such value.  If the return value is ``NULL``, an exception shall have
   been set.  If not ``NULL``, the return value is interpreted as the return
   value of the function as exposed in Python.  The function must return a new
   reference.

   The function signature is::

      PyObject *PyCFunction(PyObject *self,
                            PyObject *args);

.. c:type:: PyCFunctionWithKeywords

   Type of the functions used to implement Python callables in C
   with signature :ref:`METH_VARARGS | METH_KEYWORDS <METH_VARARGS-METH_KEYWORDS>`.
   The function signature is::

      PyObject *PyCFunctionWithKeywords(PyObject *self,
                                        PyObject *args,
                                        PyObject *kwargs);


.. c:type:: PyCFunctionFast

   Type of the functions used to implement Python callables in C
   with signature :c:macro:`METH_FASTCALL`.
   The function signature is::

      PyObject *PyCFunctionFast(PyObject *self,
                                PyObject *const *args,
                                Py_ssize_t nargs);

.. c:type:: PyCFunctionFastWithKeywords

   Type of the functions used to implement Python callables in C
   with signature :ref:`METH_FASTCALL | METH_KEYWORDS <METH_FASTCALL-METH_KEYWORDS>`.
   The function signature is::

      PyObject *PyCFunctionFastWithKeywords(PyObject *self,
                                            PyObject *const *args,
                                            Py_ssize_t nargs,
                                            PyObject *kwnames);

.. c:type:: PyCMethod

   Type of the functions used to implement Python callables in C
   with signature :ref:`METH_METHOD | METH_FASTCALL | METH_KEYWORDS <METH_METHOD-METH_FASTCALL-METH_KEYWORDS>`.
   The function signature is::

      PyObject *PyCMethod(PyObject *self,
                          PyTypeObject *defining_class,
                          PyObject *const *args,
                          Py_ssize_t nargs,
                          PyObject *kwnames)

   .. versionadded:: 3.9


.. c:type:: PyMethodDef

   Structure used to describe a method of an extension type.  This structure has
   four fields:

   .. c:member:: const char *ml_name

      Name of the method.

   .. c:member:: PyCFunction ml_meth

      Pointer to the C implementation.

   .. c:member:: int ml_flags

      Flags bits indicating how the call should be constructed.

   .. c:member:: const char *ml_doc

      Points to the contents of the docstring.

The :c:member:`~PyMethodDef.ml_meth` is a C function pointer.
The functions may be of different
types, but they always return :c:expr:`PyObject*`.  If the function is not of
the :c:type:`PyCFunction`, the compiler will require a cast in the method table.
Even though :c:type:`PyCFunction` defines the first parameter as
:c:expr:`PyObject*`, it is common that the method implementation uses the
specific C type of the *self* object.

The :c:member:`~PyMethodDef.ml_flags` field is a bitfield which can include
the following flags.
The individual flags indicate either a calling convention or a binding
convention.

There are these calling conventions:

.. c:macro:: METH_VARARGS

   This is the typical calling convention, where the methods have the type
   :c:type:`PyCFunction`. The function expects two :c:expr:`PyObject*` values.
   The first one is the *self* object for methods; for module functions, it is
   the module object.  The second parameter (often called *args*) is a tuple
   object representing all arguments. This parameter is typically processed
   using :c:func:`PyArg_ParseTuple` or :c:func:`PyArg_UnpackTuple`.


.. c:macro:: METH_KEYWORDS

   Can only be used in certain combinations with other flags:
   :ref:`METH_VARARGS | METH_KEYWORDS <METH_VARARGS-METH_KEYWORDS>`,
   :ref:`METH_FASTCALL | METH_KEYWORDS <METH_FASTCALL-METH_KEYWORDS>` and
   :ref:`METH_METHOD | METH_FASTCALL | METH_KEYWORDS <METH_METHOD-METH_FASTCALL-METH_KEYWORDS>`.


.. _METH_VARARGS-METH_KEYWORDS:

:c:expr:`METH_VARARGS | METH_KEYWORDS`
   Methods with these flags must be of type :c:type:`PyCFunctionWithKeywords`.
   The function expects three parameters: *self*, *args*, *kwargs* where
   *kwargs* is a dictionary of all the keyword arguments or possibly ``NULL``
   if there are no keyword arguments.  The parameters are typically processed
   using :c:func:`PyArg_ParseTupleAndKeywords`.


.. c:macro:: METH_FASTCALL

   Fast calling convention supporting only positional arguments.
   The methods have the type :c:type:`PyCFunctionFast`.
   The first parameter is *self*, the second parameter is a C array
   of :c:expr:`PyObject*` values indicating the arguments and the third
   parameter is the number of arguments (the length of the array).

   .. versionadded:: 3.7

   .. versionchanged:: 3.10

      ``METH_FASTCALL`` is now part of the :ref:`stable ABI <stable-abi>`.


.. _METH_FASTCALL-METH_KEYWORDS:

:c:expr:`METH_FASTCALL | METH_KEYWORDS`
   Extension of :c:macro:`METH_FASTCALL` supporting also keyword arguments,
   with methods of type :c:type:`PyCFunctionFastWithKeywords`.
   Keyword arguments are passed the same way as in the
   :ref:`vectorcall protocol <vectorcall>`:
   there is an additional fourth :c:expr:`PyObject*` parameter
   which is a tuple representing the names of the keyword arguments
   (which are guaranteed to be strings)
   or possibly ``NULL`` if there are no keywords.  The values of the keyword
   arguments are stored in the *args* array, after the positional arguments.

   .. versionadded:: 3.7


.. c:macro:: METH_METHOD

   Can only be used in the combination with other flags:
   :ref:`METH_METHOD | METH_FASTCALL | METH_KEYWORDS <METH_METHOD-METH_FASTCALL-METH_KEYWORDS>`.


.. _METH_METHOD-METH_FASTCALL-METH_KEYWORDS:

:c:expr:`METH_METHOD | METH_FASTCALL | METH_KEYWORDS`
   Extension of :ref:`METH_FASTCALL | METH_KEYWORDS <METH_FASTCALL-METH_KEYWORDS>`
   supporting the *defining class*, that is,
   the class that contains the method in question.
   The defining class might be a superclass of ``Py_TYPE(self)``.

   The method needs to be of type :c:type:`PyCMethod`, the same as for
   ``METH_FASTCALL | METH_KEYWORDS`` with ``defining_class`` argument added after
   ``self``.

   .. versionadded:: 3.9


.. c:macro:: METH_NOARGS

   Methods without parameters don't need to check whether arguments are given if
   they are listed with the :c:macro:`METH_NOARGS` flag.  They need to be of type
   :c:type:`PyCFunction`.  The first parameter is typically named *self* and will
   hold a reference to the module or object instance.  In all cases the second
   parameter will be ``NULL``.

   The function must have 2 parameters. Since the second parameter is unused,
   :c:macro:`Py_UNUSED` can be used to prevent a compiler warning.


.. c:macro:: METH_O

   Methods with a single object argument can be listed with the :c:macro:`METH_O`
   flag, instead of invoking :c:func:`PyArg_ParseTuple` with a ``"O"`` argument.
   They have the type :c:type:`PyCFunction`, with the *self* parameter, and a
   :c:expr:`PyObject*` parameter representing the single argument.


These two constants are not used to indicate the calling convention but the
binding when use with methods of classes.  These may not be used for functions
defined for modules.  At most one of these flags may be set for any given
method.


.. c:macro:: METH_CLASS

   .. index:: pair: built-in function; classmethod

   The method will be passed the type object as the first parameter rather
   than an instance of the type.  This is used to create *class methods*,
   similar to what is created when using the :func:`classmethod` built-in
   function.


.. c:macro:: METH_STATIC

   .. index:: pair: built-in function; staticmethod

   The method will be passed ``NULL`` as the first parameter rather than an
   instance of the type.  This is used to create *static methods*, similar to
   what is created when using the :func:`staticmethod` built-in function.

One other constant controls whether a method is loaded in place of another
definition with the same method name.


.. c:macro:: METH_COEXIST

   The method will be loaded in place of existing definitions.  Without
   *METH_COEXIST*, the default is to skip repeated definitions.  Since slot
   wrappers are loaded before the method table, the existence of a
   *sq_contains* slot, for example, would generate a wrapped method named
   :meth:`~object.__contains__` and preclude the loading of a corresponding
   PyCFunction with the same name.  With the flag defined, the PyCFunction
   will be loaded in place of the wrapper object and will co-exist with the
   slot.  This is helpful because calls to PyCFunctions are optimized more
   than wrapper object calls.

.. c:function:: PyObject * PyCMethod_New(PyMethodDef *ml, PyObject *self, PyObject *module, PyTypeObject *cls)

   Turn *ml* into a Python :term:`callable` object.
   The caller must ensure that *ml* outlives the :term:`callable`.
   Typically, *ml* is defined as a static variable.

   The *self* parameter will be passed as the *self* argument
   to the C function in ``ml->ml_meth`` when invoked.
   *self* can be ``NULL``.

   The :term:`callable` object's ``__module__`` attribute
   can be set from the given *module* argument.
   *module* should be a Python string,
   which will be used as name of the module the function is defined in.
   If unavailable, it can be set to :const:`None` or ``NULL``.

   .. seealso:: :attr:`function.__module__`

   The *cls* parameter will be passed as the *defining_class*
   argument to the C function.
   Must be set if :c:macro:`METH_METHOD` is set on ``ml->ml_flags``.

   .. versionadded:: 3.9


.. c:function:: PyObject * PyCFunction_NewEx(PyMethodDef *ml, PyObject *self, PyObject *module)

   Equivalent to ``PyCMethod_New(ml, self, module, NULL)``.


.. c:function:: PyObject * PyCFunction_New(PyMethodDef *ml, PyObject *self)

   Equivalent to ``PyCMethod_New(ml, self, NULL, NULL)``.


Accessing attributes of extension types
---------------------------------------

.. c:type:: PyMemberDef

   Structure which describes an attribute of a type which corresponds to a C
   struct member.
   When defining a class, put a NULL-terminated array of these
   structures in the :c:member:`~PyTypeObject.tp_members` slot.

   Its fields are, in order:

   .. c:member:: const char* name

         Name of the member.
         A NULL value marks the end of a ``PyMemberDef[]`` array.

         The string should be static, no copy is made of it.

   .. c:member:: int type

      The type of the member in the C struct.
      See :ref:`PyMemberDef-types` for the possible values.

   .. c:member:: Py_ssize_t offset

      The offset in bytes that the member is located on the type’s object struct.

   .. c:member:: int flags

      Zero or more of the :ref:`PyMemberDef-flags`, combined using bitwise OR.

   .. c:member:: const char* doc

      The docstring, or NULL.
      The string should be static, no copy is made of it.
      Typically, it is defined using :c:macro:`PyDoc_STR`.

   By default (when :c:member:`~PyMemberDef.flags` is ``0``), members allow
   both read and write access.
   Use the :c:macro:`Py_READONLY` flag for read-only access.
   Certain types, like :c:macro:`Py_T_STRING`, imply :c:macro:`Py_READONLY`.
   Only :c:macro:`Py_T_OBJECT_EX` (and legacy :c:macro:`T_OBJECT`) members can
   be deleted.

   .. _pymemberdef-offsets:

   For heap-allocated types (created using :c:func:`PyType_FromSpec` or similar),
   ``PyMemberDef`` may contain a definition for the special member
   ``"__vectorcalloffset__"``, corresponding to
   :c:member:`~PyTypeObject.tp_vectorcall_offset` in type objects.
   These must be defined with ``Py_T_PYSSIZET`` and ``Py_READONLY``, for example::

      static PyMemberDef spam_type_members[] = {
          {"__vectorcalloffset__", Py_T_PYSSIZET,
           offsetof(Spam_object, vectorcall), Py_READONLY},
          {NULL}  /* Sentinel */
      };

   (You may need to ``#include <stddef.h>`` for :c:func:`!offsetof`.)

   The legacy offsets :c:member:`~PyTypeObject.tp_dictoffset` and
   :c:member:`~PyTypeObject.tp_weaklistoffset` can be defined similarly using
   ``"__dictoffset__"`` and ``"__weaklistoffset__"`` members, but extensions
   are strongly encouraged to use :c:macro:`Py_TPFLAGS_MANAGED_DICT` and
   :c:macro:`Py_TPFLAGS_MANAGED_WEAKREF` instead.

   .. versionchanged:: 3.12

      ``PyMemberDef`` is always available.
      Previously, it required including ``"structmember.h"``.

.. c:function:: PyObject* PyMember_GetOne(const char *obj_addr, struct PyMemberDef *m)

   Get an attribute belonging to the object at address *obj_addr*.  The
   attribute is described by ``PyMemberDef`` *m*.  Returns ``NULL``
   on error.

   .. versionchanged:: 3.12

      ``PyMember_GetOne`` is always available.
      Previously, it required including ``"structmember.h"``.

.. c:function:: int PyMember_SetOne(char *obj_addr, struct PyMemberDef *m, PyObject *o)

   Set an attribute belonging to the object at address *obj_addr* to object *o*.
   The attribute to set is described by ``PyMemberDef`` *m*.  Returns ``0``
   if successful and a negative value on failure.

   .. versionchanged:: 3.12

      ``PyMember_SetOne`` is always available.
      Previously, it required including ``"structmember.h"``.

.. _PyMemberDef-flags:

Member flags
^^^^^^^^^^^^

The following flags can be used with :c:member:`PyMemberDef.flags`:

.. c:macro:: Py_READONLY

   Not writable.

.. c:macro:: Py_AUDIT_READ

   Emit an ``object.__getattr__`` :ref:`audit event <audit-events>`
   before reading.

.. c:macro:: Py_RELATIVE_OFFSET

   Indicates that the :c:member:`~PyMemberDef.offset` of this ``PyMemberDef``
   entry indicates an offset from the subclass-specific data, rather than
   from ``PyObject``.

   Can only be used as part of :c:member:`Py_tp_members <PyTypeObject.tp_members>`
   :c:type:`slot <PyType_Slot>` when creating a class using negative
   :c:member:`~PyType_Spec.basicsize`.
   It is mandatory in that case.

   This flag is only used in :c:type:`PyType_Slot`.
   When setting :c:member:`~PyTypeObject.tp_members` during
   class creation, Python clears it and sets
   :c:member:`PyMemberDef.offset` to the offset from the ``PyObject`` struct.

.. index::
   single: READ_RESTRICTED (C macro)
   single: WRITE_RESTRICTED (C macro)
   single: RESTRICTED (C macro)

.. versionchanged:: 3.10

   The :c:macro:`!RESTRICTED`, :c:macro:`!READ_RESTRICTED` and
   :c:macro:`!WRITE_RESTRICTED` macros available with
   ``#include "structmember.h"`` are deprecated.
   :c:macro:`!READ_RESTRICTED` and :c:macro:`!RESTRICTED` are equivalent to
   :c:macro:`Py_AUDIT_READ`; :c:macro:`!WRITE_RESTRICTED` does nothing.

.. index::
   single: READONLY (C macro)

.. versionchanged:: 3.12

   The :c:macro:`!READONLY` macro was renamed to :c:macro:`Py_READONLY`.
   The :c:macro:`!PY_AUDIT_READ` macro was renamed with the ``Py_`` prefix.
   The new names are now always available.
   Previously, these required ``#include "structmember.h"``.
   The header is still available and it provides the old names.

.. _PyMemberDef-types:

Member types
^^^^^^^^^^^^

:c:member:`PyMemberDef.type` can be one of the following macros corresponding
to various C types.
When the member is accessed in Python, it will be converted to the
equivalent Python type.
When it is set from Python, it will be converted back to the C type.
If that is not possible, an exception such as :exc:`TypeError` or
:exc:`ValueError` is raised.

Unless marked (D), attributes defined this way cannot be deleted
using e.g. :keyword:`del` or :py:func:`delattr`.

================================ ============================= ======================
Macro name                       C type                        Python type
================================ ============================= ======================
.. c:macro:: Py_T_BYTE           :c:expr:`char`                :py:class:`int`
.. c:macro:: Py_T_SHORT          :c:expr:`short`               :py:class:`int`
.. c:macro:: Py_T_INT            :c:expr:`int`                 :py:class:`int`
.. c:macro:: Py_T_LONG           :c:expr:`long`                :py:class:`int`
.. c:macro:: Py_T_LONGLONG       :c:expr:`long long`           :py:class:`int`
.. c:macro:: Py_T_UBYTE          :c:expr:`unsigned char`       :py:class:`int`
.. c:macro:: Py_T_UINT           :c:expr:`unsigned int`        :py:class:`int`
.. c:macro:: Py_T_USHORT         :c:expr:`unsigned short`      :py:class:`int`
.. c:macro:: Py_T_ULONG          :c:expr:`unsigned long`       :py:class:`int`
.. c:macro:: Py_T_ULONGLONG      :c:expr:`unsigned long long`  :py:class:`int`
.. c:macro:: Py_T_PYSSIZET       :c:expr:`Py_ssize_t`          :py:class:`int`
.. c:macro:: Py_T_FLOAT          :c:expr:`float`               :py:class:`float`
.. c:macro:: Py_T_DOUBLE         :c:expr:`double`              :py:class:`float`
.. c:macro:: Py_T_BOOL           :c:expr:`char`                :py:class:`bool`
                                 (written as 0 or 1)
.. c:macro:: Py_T_STRING         :c:expr:`const char *` (*)    :py:class:`str` (RO)
.. c:macro:: Py_T_STRING_INPLACE :c:expr:`const char[]` (*)    :py:class:`str` (RO)
.. c:macro:: Py_T_CHAR           :c:expr:`char` (0-127)        :py:class:`str` (**)
.. c:macro:: Py_T_OBJECT_EX      :c:expr:`PyObject *`          :py:class:`object` (D)
================================ ============================= ======================

   (*): Zero-terminated, UTF8-encoded C string.
   With :c:macro:`!Py_T_STRING` the C representation is a pointer;
   with :c:macro:`!Py_T_STRING_INPLACE` the string is stored directly
   in the structure.

   (**): String of length 1. Only ASCII is accepted.

   (RO): Implies :c:macro:`Py_READONLY`.

   (D): Can be deleted, in which case the pointer is set to ``NULL``.
   Reading a ``NULL`` pointer raises :py:exc:`AttributeError`.

.. index::
   single: T_BYTE (C macro)
   single: T_SHORT (C macro)
   single: T_INT (C macro)
   single: T_LONG (C macro)
   single: T_LONGLONG (C macro)
   single: T_UBYTE (C macro)
   single: T_USHORT (C macro)
   single: T_UINT (C macro)
   single: T_ULONG (C macro)
   single: T_ULONGULONG (C macro)
   single: T_PYSSIZET (C macro)
   single: T_FLOAT (C macro)
   single: T_DOUBLE (C macro)
   single: T_BOOL (C macro)
   single: T_CHAR (C macro)
   single: T_STRING (C macro)
   single: T_STRING_INPLACE (C macro)
   single: T_OBJECT_EX (C macro)
   single: structmember.h

.. versionadded:: 3.12

   In previous versions, the macros were only available with
   ``#include "structmember.h"`` and were named without the ``Py_`` prefix
   (e.g. as ``T_INT``).
   The header is still available and contains the old names, along with
   the following deprecated types:

   .. c:macro:: T_OBJECT

      Like ``Py_T_OBJECT_EX``, but ``NULL`` is converted to ``None``.
      This results in surprising behavior in Python: deleting the attribute
      effectively sets it to ``None``.

   .. c:macro:: T_NONE

      Always ``None``. Must be used with :c:macro:`Py_READONLY`.

Defining Getters and Setters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. c:type:: PyGetSetDef

   Structure to define property-like access for a type. See also description of
   the :c:member:`PyTypeObject.tp_getset` slot.

   .. c:member:: const char* name

      attribute name

   .. c:member:: getter get

      C function to get the attribute.

   .. c:member:: setter set

      Optional C function to set or delete the attribute.
      If ``NULL``, the attribute is read-only.

   .. c:member:: const char* doc

      optional docstring

   .. c:member:: void* closure

      Optional user data pointer, providing additional data for getter and setter.

.. c:type:: PyObject *(*getter)(PyObject *, void *)

   The ``get`` function takes one :c:expr:`PyObject*` parameter (the
   instance) and a user data pointer (the associated ``closure``):

   It should return a new reference on success or ``NULL`` with a set exception
   on failure.

.. c:type:: int (*setter)(PyObject *, PyObject *, void *)

   ``set`` functions take two :c:expr:`PyObject*` parameters (the instance and
   the value to be set) and a user data pointer (the associated ``closure``):

   In case the attribute should be deleted the second parameter is ``NULL``.
   Should return ``0`` on success or ``-1`` with a set exception on failure.
