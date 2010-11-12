.. highlightlang:: c

.. _common-structs:

Common Object Structures
========================

There are a large number of structures which are used in the definition of
object types for Python.  This section describes these structures and how they
are used.

All Python objects ultimately share a small number of fields at the beginning
of the object's representation in memory.  These are represented by the
:ctype:`PyObject` and :ctype:`PyVarObject` types, which are defined, in turn,
by the expansions of some macros also used, whether directly or indirectly, in
the definition of all other Python objects.


.. ctype:: PyObject

   All object types are extensions of this type.  This is a type which
   contains the information Python needs to treat a pointer to an object as an
   object.  In a normal "release" build, it contains only the object's
   reference count and a pointer to the corresponding type object.  It
   corresponds to the fields defined by the expansion of the ``PyObject_HEAD``
   macro.


.. ctype:: PyVarObject

   This is an extension of :ctype:`PyObject` that adds the :attr:`ob_size`
   field.  This is only used for objects that have some notion of *length*.
   This type does not often appear in the Python/C API.  It corresponds to the
   fields defined by the expansion of the ``PyObject_VAR_HEAD`` macro.

These macros are used in the definition of :ctype:`PyObject` and
:ctype:`PyVarObject`:


.. cmacro:: PyObject_HEAD

   This is a macro which expands to the declarations of the fields of the
   :ctype:`PyObject` type; it is used when declaring new types which represent
   objects without a varying length.  The specific fields it expands to depend
   on the definition of :cmacro:`Py_TRACE_REFS`.  By default, that macro is
   not defined, and :cmacro:`PyObject_HEAD` expands to::

      Py_ssize_t ob_refcnt;
      PyTypeObject *ob_type;

   When :cmacro:`Py_TRACE_REFS` is defined, it expands to::

      PyObject *_ob_next, *_ob_prev;
      Py_ssize_t ob_refcnt;
      PyTypeObject *ob_type;


.. cmacro:: PyObject_VAR_HEAD

   This is a macro which expands to the declarations of the fields of the
   :ctype:`PyVarObject` type; it is used when declaring new types which
   represent objects with a length that varies from instance to instance.
   This macro always expands to::

      PyObject_HEAD
      Py_ssize_t ob_size;

   Note that :cmacro:`PyObject_HEAD` is part of the expansion, and that its own
   expansion varies depending on the definition of :cmacro:`Py_TRACE_REFS`.


.. cmacro:: PyObject_HEAD_INIT(type)

   This is a macro which expands to initialization values for a new
   :ctype:`PyObject` type.  This macro expands to::

      _PyObject_EXTRA_INIT
      1, type,


.. cmacro:: PyVarObject_HEAD_INIT(type, size)

   This is a macro which expands to initialization values for a new
   :ctype:`PyVarObject` type, including the :attr:`ob_size` field.
   This macro expands to::

      _PyObject_EXTRA_INIT
      1, type, size,


.. ctype:: PyCFunction

   Type of the functions used to implement most Python callables in C.
   Functions of this type take two :ctype:`PyObject\*` parameters and return
   one such value.  If the return value is *NULL*, an exception shall have
   been set.  If not *NULL*, the return value is interpreted as the return
   value of the function as exposed in Python.  The function must return a new
   reference.


.. ctype:: PyMethodDef

   Structure used to describe a method of an extension type.  This structure has
   four fields:

   +------------------+-------------+-------------------------------+
   | Field            | C Type      | Meaning                       |
   +==================+=============+===============================+
   | :attr:`ml_name`  | char \*     | name of the method            |
   +------------------+-------------+-------------------------------+
   | :attr:`ml_meth`  | PyCFunction | pointer to the C              |
   |                  |             | implementation                |
   +------------------+-------------+-------------------------------+
   | :attr:`ml_flags` | int         | flag bits indicating how the  |
   |                  |             | call should be constructed    |
   +------------------+-------------+-------------------------------+
   | :attr:`ml_doc`   | char \*     | points to the contents of the |
   |                  |             | docstring                     |
   +------------------+-------------+-------------------------------+

The :attr:`ml_meth` is a C function pointer.  The functions may be of different
types, but they always return :ctype:`PyObject\*`.  If the function is not of
the :ctype:`PyCFunction`, the compiler will require a cast in the method table.
Even though :ctype:`PyCFunction` defines the first parameter as
:ctype:`PyObject\*`, it is common that the method implementation uses a the
specific C type of the *self* object.

The :attr:`ml_flags` field is a bitfield which can include the following flags.
The individual flags indicate either a calling convention or a binding
convention.  Of the calling convention flags, only :const:`METH_VARARGS` and
:const:`METH_KEYWORDS` can be combined (but note that :const:`METH_KEYWORDS`
alone is equivalent to ``METH_VARARGS | METH_KEYWORDS``). Any of the calling
convention flags can be combined with a binding flag.


.. data:: METH_VARARGS

   This is the typical calling convention, where the methods have the type
   :ctype:`PyCFunction`. The function expects two :ctype:`PyObject\*` values.
   The first one is the *self* object for methods; for module functions, it is
   the module object.  The second parameter (often called *args*) is a tuple
   object representing all arguments.  This parameter is typically processed
   using :cfunc:`PyArg_ParseTuple` or :cfunc:`PyArg_UnpackTuple`.


.. data:: METH_KEYWORDS

   Methods with these flags must be of type :ctype:`PyCFunctionWithKeywords`.
   The function expects three parameters: *self*, *args*, and a dictionary of
   all the keyword arguments.  The flag is typically combined with
   :const:`METH_VARARGS`, and the parameters are typically processed using
   :cfunc:`PyArg_ParseTupleAndKeywords`.


.. data:: METH_NOARGS

   Methods without parameters don't need to check whether arguments are given if
   they are listed with the :const:`METH_NOARGS` flag.  They need to be of type
   :ctype:`PyCFunction`.  The first parameter is typically named ``self`` and
   will hold a reference to the module or object instance.  In all cases the
   second parameter will be *NULL*.


.. data:: METH_O

   Methods with a single object argument can be listed with the :const:`METH_O`
   flag, instead of invoking :cfunc:`PyArg_ParseTuple` with a ``"O"`` argument.
   They have the type :ctype:`PyCFunction`, with the *self* parameter, and a
   :ctype:`PyObject\*` parameter representing the single argument.


.. data:: METH_OLDARGS

   This calling convention is deprecated.  The method must be of type
   :ctype:`PyCFunction`.  The second argument is *NULL* if no arguments are
   given, a single object if exactly one argument is given, and a tuple of
   objects if more than one argument is given.  There is no way for a function
   using this convention to distinguish between a call with multiple arguments
   and a call with a tuple as the only argument.

These two constants are not used to indicate the calling convention but the
binding when use with methods of classes.  These may not be used for functions
defined for modules.  At most one of these flags may be set for any given
method.


.. data:: METH_CLASS

   .. index:: builtin: classmethod

   The method will be passed the type object as the first parameter rather
   than an instance of the type.  This is used to create *class methods*,
   similar to what is created when using the :func:`classmethod` built-in
   function.

   .. versionadded:: 2.3


.. data:: METH_STATIC

   .. index:: builtin: staticmethod

   The method will be passed *NULL* as the first parameter rather than an
   instance of the type.  This is used to create *static methods*, similar to
   what is created when using the :func:`staticmethod` built-in function.

   .. versionadded:: 2.3

One other constant controls whether a method is loaded in place of another
definition with the same method name.


.. data:: METH_COEXIST

   The method will be loaded in place of existing definitions.  Without
   *METH_COEXIST*, the default is to skip repeated definitions.  Since slot
   wrappers are loaded before the method table, the existence of a
   *sq_contains* slot, for example, would generate a wrapped method named
   :meth:`__contains__` and preclude the loading of a corresponding
   PyCFunction with the same name.  With the flag defined, the PyCFunction
   will be loaded in place of the wrapper object and will co-exist with the
   slot.  This is helpful because calls to PyCFunctions are optimized more
   than wrapper object calls.

   .. versionadded:: 2.4


.. ctype:: PyMemberDef

   Structure which describes an attribute of a type which corresponds to a C
   struct member.  Its fields are:

   +------------------+-------------+-------------------------------+
   | Field            | C Type      | Meaning                       |
   +==================+=============+===============================+
   | :attr:`name`     | char \*     | name of the member            |
   +------------------+-------------+-------------------------------+
   | :attr:`type`     | int         | the type of the member in the |
   |                  |             | C struct                      |
   +------------------+-------------+-------------------------------+
   | :attr:`offset`   | Py_ssize_t  | the offset in bytes that the  |
   |                  |             | member is located on the      |
   |                  |             | type's object struct          |
   +------------------+-------------+-------------------------------+
   | :attr:`flags`    | int         | flag bits indicating if the   |
   |                  |             | field should be read-only or  |
   |                  |             | writable                      |
   +------------------+-------------+-------------------------------+
   | :attr:`doc`      | char \*     | points to the contents of the |
   |                  |             | docstring                     |
   +------------------+-------------+-------------------------------+

   :attr:`type` can be one of many ``T_`` macros corresponding to various C
   types.  When the member is accessed in Python, it will be converted to the
   equivalent Python type.

   =============== ==================
   Macro name      C type
   =============== ==================
   T_SHORT         short
   T_INT           int
   T_LONG          long
   T_FLOAT         float
   T_DOUBLE        double
   T_STRING        char \*
   T_OBJECT        PyObject \*
   T_OBJECT_EX     PyObject \*
   T_CHAR          char
   T_BYTE          char
   T_UBYTE         unsigned char
   T_UINT          unsigned int
   T_USHORT        unsigned short
   T_ULONG         unsigned long
   T_BOOL          char
   T_LONGLONG      long long
   T_ULONGLONG     unsigned long long
   T_PYSSIZET      Py_ssize_t
   =============== ==================

   :cmacro:`T_OBJECT` and :cmacro:`T_OBJECT_EX` differ in that
   :cmacro:`T_OBJECT` returns ``None`` if the member is *NULL* and
   :cmacro:`T_OBJECT_EX` raises an :exc:`AttributeError`.  Try to use
   :cmacro:`T_OBJECT_EX` over :cmacro:`T_OBJECT` because :cmacro:`T_OBJECT_EX`
   handles use of the :keyword:`del` statement on that attribute more correctly
   than :cmacro:`T_OBJECT`.

   :attr:`flags` can be 0 for write and read access or :cmacro:`READONLY` for
   read-only access.  Using :cmacro:`T_STRING` for :attr:`type` implies
   :cmacro:`READONLY`.  Only :cmacro:`T_OBJECT` and :cmacro:`T_OBJECT_EX`
   members can be deleted.  (They are set to *NULL*).


.. cfunction:: PyObject* Py_FindMethod(PyMethodDef table[], PyObject *ob, char *name)

   Return a bound method object for an extension type implemented in C.  This
   can be useful in the implementation of a :attr:`tp_getattro` or
   :attr:`tp_getattr` handler that does not use the
   :cfunc:`PyObject_GenericGetAttr` function.
