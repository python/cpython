.. highlightlang:: c


.. _newtypes:

*****************************
Object Implementation Support
*****************************

This chapter describes the functions, types, and macros used when defining new
object types.


.. _allocating-objects:

Allocating Objects on the Heap
==============================


.. cfunction:: PyObject* _PyObject_New(PyTypeObject *type)


.. cfunction:: PyVarObject* _PyObject_NewVar(PyTypeObject *type, Py_ssize_t size)


.. cfunction:: void _PyObject_Del(PyObject *op)


.. cfunction:: PyObject* PyObject_Init(PyObject *op, PyTypeObject *type)

   Initialize a newly-allocated object *op* with its type and initial reference.
   Returns the initialized object.  If *type* indicates that the object
   participates in the cyclic garbage detector, it is added to the detector's set
   of observed objects. Other fields of the object are not affected.


.. cfunction:: PyVarObject* PyObject_InitVar(PyVarObject *op, PyTypeObject *type, Py_ssize_t size)

   This does everything :cfunc:`PyObject_Init` does, and also initializes the
   length information for a variable-size object.


.. cfunction:: TYPE* PyObject_New(TYPE, PyTypeObject *type)

   Allocate a new Python object using the C structure type *TYPE* and the Python
   type object *type*.  Fields not defined by the Python object header are not
   initialized; the object's reference count will be one.  The size of the memory
   allocation is determined from the :attr:`tp_basicsize` field of the type object.


.. cfunction:: TYPE* PyObject_NewVar(TYPE, PyTypeObject *type, Py_ssize_t size)

   Allocate a new Python object using the C structure type *TYPE* and the Python
   type object *type*.  Fields not defined by the Python object header are not
   initialized.  The allocated memory allows for the *TYPE* structure plus *size*
   fields of the size given by the :attr:`tp_itemsize` field of *type*.  This is
   useful for implementing objects like tuples, which are able to determine their
   size at construction time.  Embedding the array of fields into the same
   allocation decreases the number of allocations, improving the memory management
   efficiency.


.. cfunction:: void PyObject_Del(PyObject *op)

   Releases memory allocated to an object using :cfunc:`PyObject_New` or
   :cfunc:`PyObject_NewVar`.  This is normally called from the :attr:`tp_dealloc`
   handler specified in the object's type.  The fields of the object should not be
   accessed after this call as the memory is no longer a valid Python object.


.. cfunction:: PyObject* Py_InitModule(char *name, PyMethodDef *methods)

   Create a new module object based on a name and table of functions, returning the
   new module object.

   .. versionchanged:: 2.3
      Older versions of Python did not support *NULL* as the value for the *methods*
      argument.


.. cfunction:: PyObject* Py_InitModule3(char *name, PyMethodDef *methods, char *doc)

   Create a new module object based on a name and table of functions, returning the
   new module object.  If *doc* is non-*NULL*, it will be used to define the
   docstring for the module.

   .. versionchanged:: 2.3
      Older versions of Python did not support *NULL* as the value for the *methods*
      argument.


.. cfunction:: PyObject* Py_InitModule4(char *name, PyMethodDef *methods, char *doc, PyObject *self, int apiver)

   Create a new module object based on a name and table of functions, returning the
   new module object.  If *doc* is non-*NULL*, it will be used to define the
   docstring for the module.  If *self* is non-*NULL*, it will passed to the
   functions of the module as their (otherwise *NULL*) first parameter.  (This was
   added as an experimental feature, and there are no known uses in the current
   version of Python.)  For *apiver*, the only value which should be passed is
   defined by the constant :const:`PYTHON_API_VERSION`.

   .. note::

      Most uses of this function should probably be using the :cfunc:`Py_InitModule3`
      instead; only use this if you are sure you need it.

   .. versionchanged:: 2.3
      Older versions of Python did not support *NULL* as the value for the *methods*
      argument.


.. cvar:: PyObject _Py_NoneStruct

   Object which is visible in Python as ``None``.  This should only be accessed
   using the ``Py_None`` macro, which evaluates to a pointer to this object.


.. _common-structs:

Common Object Structures
========================

There are a large number of structures which are used in the definition of
object types for Python.  This section describes these structures and how they
are used.

All Python objects ultimately share a small number of fields at the beginning of
the object's representation in memory.  These are represented by the
:ctype:`PyObject` and :ctype:`PyVarObject` types, which are defined, in turn, by
the expansions of some macros also used, whether directly or indirectly, in the
definition of all other Python objects.


.. ctype:: PyObject

   All object types are extensions of this type.  This is a type which contains the
   information Python needs to treat a pointer to an object as an object.  In a
   normal "release" build, it contains only the objects reference count and a
   pointer to the corresponding type object.  It corresponds to the fields defined
   by the expansion of the ``PyObject_HEAD`` macro.


.. ctype:: PyVarObject

   This is an extension of :ctype:`PyObject` that adds the :attr:`ob_size` field.
   This is only used for objects that have some notion of *length*.  This type does
   not often appear in the Python/C API.  It corresponds to the fields defined by
   the expansion of the ``PyObject_VAR_HEAD`` macro.

These macros are used in the definition of :ctype:`PyObject` and
:ctype:`PyVarObject`:


.. cmacro:: PyObject_HEAD

   This is a macro which expands to the declarations of the fields of the
   :ctype:`PyObject` type; it is used when declaring new types which represent
   objects without a varying length.  The specific fields it expands to depend on
   the definition of :cmacro:`Py_TRACE_REFS`.  By default, that macro is not
   defined, and :cmacro:`PyObject_HEAD` expands to::

      Py_ssize_t ob_refcnt;
      PyTypeObject *ob_type;

   When :cmacro:`Py_TRACE_REFS` is defined, it expands to::

      PyObject *_ob_next, *_ob_prev;
      Py_ssize_t ob_refcnt;
      PyTypeObject *ob_type;


.. cmacro:: PyObject_VAR_HEAD

   This is a macro which expands to the declarations of the fields of the
   :ctype:`PyVarObject` type; it is used when declaring new types which represent
   objects with a length that varies from instance to instance.  This macro always
   expands to::

      PyObject_HEAD
      Py_ssize_t ob_size;

   Note that :cmacro:`PyObject_HEAD` is part of the expansion, and that its own
   expansion varies depending on the definition of :cmacro:`Py_TRACE_REFS`.

PyObject_HEAD_INIT


.. ctype:: PyCFunction

   Type of the functions used to implement most Python callables in C. Functions of
   this type take two :ctype:`PyObject\*` parameters and return one such value.  If
   the return value is *NULL*, an exception shall have been set.  If not *NULL*,
   the return value is interpreted as the return value of the function as exposed
   in Python.  The function must return a new reference.


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
   :ctype:`PyCFunction`. The function expects two :ctype:`PyObject\*` values.  The
   first one is the *self* object for methods; for module functions, it has the
   value given to :cfunc:`Py_InitModule4` (or *NULL* if :cfunc:`Py_InitModule` was
   used).  The second parameter (often called *args*) is a tuple object
   representing all arguments. This parameter is typically processed using
   :cfunc:`PyArg_ParseTuple` or :cfunc:`PyArg_UnpackTuple`.


.. data:: METH_KEYWORDS

   Methods with these flags must be of type :ctype:`PyCFunctionWithKeywords`.  The
   function expects three parameters: *self*, *args*, and a dictionary of all the
   keyword arguments.  The flag is typically combined with :const:`METH_VARARGS`,
   and the parameters are typically processed using
   :cfunc:`PyArg_ParseTupleAndKeywords`.


.. data:: METH_NOARGS

   Methods without parameters don't need to check whether arguments are given if
   they are listed with the :const:`METH_NOARGS` flag.  They need to be of type
   :ctype:`PyCFunction`.  When used with object methods, the first parameter is
   typically named ``self`` and will hold a reference to the object instance.  In
   all cases the second parameter will be *NULL*.


.. data:: METH_O

   Methods with a single object argument can be listed with the :const:`METH_O`
   flag, instead of invoking :cfunc:`PyArg_ParseTuple` with a ``"O"`` argument.
   They have the type :ctype:`PyCFunction`, with the *self* parameter, and a
   :ctype:`PyObject\*` parameter representing the single argument.


.. data:: METH_OLDARGS

   This calling convention is deprecated.  The method must be of type
   :ctype:`PyCFunction`.  The second argument is *NULL* if no arguments are given,
   a single object if exactly one argument is given, and a tuple of objects if more
   than one argument is given.  There is no way for a function using this
   convention to distinguish between a call with multiple arguments and a call with
   a tuple as the only argument.

These two constants are not used to indicate the calling convention but the
binding when use with methods of classes.  These may not be used for functions
defined for modules.  At most one of these flags may be set for any given
method.


.. data:: METH_CLASS

   .. index:: builtin: classmethod

   The method will be passed the type object as the first parameter rather than an
   instance of the type.  This is used to create *class methods*, similar to what
   is created when using the :func:`classmethod` built-in function.

   .. versionadded:: 2.3


.. data:: METH_STATIC

   .. index:: builtin: staticmethod

   The method will be passed *NULL* as the first parameter rather than an instance
   of the type.  This is used to create *static methods*, similar to what is
   created when using the :func:`staticmethod` built-in function.

   .. versionadded:: 2.3

One other constant controls whether a method is loaded in place of another
definition with the same method name.


.. data:: METH_COEXIST

   The method will be loaded in place of existing definitions.  Without
   *METH_COEXIST*, the default is to skip repeated definitions.  Since slot
   wrappers are loaded before the method table, the existence of a *sq_contains*
   slot, for example, would generate a wrapped method named :meth:`__contains__`
   and preclude the loading of a corresponding PyCFunction with the same name.
   With the flag defined, the PyCFunction will be loaded in place of the wrapper
   object and will co-exist with the slot.  This is helpful because calls to
   PyCFunctions are optimized more than wrapper object calls.

   .. versionadded:: 2.4


.. cfunction:: PyObject* Py_FindMethod(PyMethodDef table[], PyObject *ob, char *name)

   Return a bound method object for an extension type implemented in C.  This can
   be useful in the implementation of a :attr:`tp_getattro` or :attr:`tp_getattr`
   handler that does not use the :cfunc:`PyObject_GenericGetAttr` function.


.. _type-structs:

Type Objects
============

Perhaps one of the most important structures of the Python object system is the
structure that defines a new type: the :ctype:`PyTypeObject` structure.  Type
objects can be handled using any of the :cfunc:`PyObject_\*` or
:cfunc:`PyType_\*` functions, but do not offer much that's interesting to most
Python applications. These objects are fundamental to how objects behave, so
they are very important to the interpreter itself and to any extension module
that implements new types.

Type objects are fairly large compared to most of the standard types. The reason
for the size is that each type object stores a large number of values, mostly C
function pointers, each of which implements a small part of the type's
functionality.  The fields of the type object are examined in detail in this
section.  The fields will be described in the order in which they occur in the
structure.

Typedefs: unaryfunc, binaryfunc, ternaryfunc, inquiry, coercion, intargfunc,
intintargfunc, intobjargproc, intintobjargproc, objobjargproc, destructor,
freefunc, printfunc, getattrfunc, getattrofunc, setattrfunc, setattrofunc,
cmpfunc, reprfunc, hashfunc

The structure definition for :ctype:`PyTypeObject` can be found in
:file:`Include/object.h`.  For convenience of reference, this repeats the
definition found there:

.. literalinclude:: ../includes/typestruct.h


The type object structure extends the :ctype:`PyVarObject` structure. The
:attr:`ob_size` field is used for dynamic types (created by  :func:`type_new`,
usually called from a class statement). Note that :cdata:`PyType_Type` (the
metatype) initializes :attr:`tp_itemsize`, which means that its instances (i.e.
type objects) *must* have the :attr:`ob_size` field.


.. cmember:: PyObject* PyObject._ob_next
             PyObject* PyObject._ob_prev

   These fields are only present when the macro ``Py_TRACE_REFS`` is defined.
   Their initialization to *NULL* is taken care of by the ``PyObject_HEAD_INIT``
   macro.  For statically allocated objects, these fields always remain *NULL*.
   For dynamically allocated objects, these two fields are used to link the object
   into a doubly-linked list of *all* live objects on the heap.  This could be used
   for various debugging purposes; currently the only use is to print the objects
   that are still alive at the end of a run when the environment variable
   :envvar:`PYTHONDUMPREFS` is set.

   These fields are not inherited by subtypes.


.. cmember:: Py_ssize_t PyObject.ob_refcnt

   This is the type object's reference count, initialized to ``1`` by the
   ``PyObject_HEAD_INIT`` macro.  Note that for statically allocated type objects,
   the type's instances (objects whose :attr:`ob_type` points back to the type) do
   *not* count as references.  But for dynamically allocated type objects, the
   instances *do* count as references.

   This field is not inherited by subtypes.


.. cmember:: PyTypeObject* PyObject.ob_type

   This is the type's type, in other words its metatype.  It is initialized by the
   argument to the ``PyObject_HEAD_INIT`` macro, and its value should normally be
   ``&PyType_Type``.  However, for dynamically loadable extension modules that must
   be usable on Windows (at least), the compiler complains that this is not a valid
   initializer.  Therefore, the convention is to pass *NULL* to the
   ``PyObject_HEAD_INIT`` macro and to initialize this field explicitly at the
   start of the module's initialization function, before doing anything else.  This
   is typically done like this::

      Foo_Type.ob_type = &PyType_Type;

   This should be done before any instances of the type are created.
   :cfunc:`PyType_Ready` checks if :attr:`ob_type` is *NULL*, and if so,
   initializes it: in Python 2.2, it is set to ``&PyType_Type``; in Python 2.2.1
   and later it is initialized to the :attr:`ob_type` field of the base class.
   :cfunc:`PyType_Ready` will not change this field if it is non-zero.

   In Python 2.2, this field is not inherited by subtypes.  In 2.2.1, and in 2.3
   and beyond, it is inherited by subtypes.


.. cmember:: Py_ssize_t PyVarObject.ob_size

   For statically allocated type objects, this should be initialized to zero.  For
   dynamically allocated type objects, this field has a special internal meaning.

   This field is not inherited by subtypes.


.. cmember:: char* PyTypeObject.tp_name

   Pointer to a NUL-terminated string containing the name of the type. For types
   that are accessible as module globals, the string should be the full module
   name, followed by a dot, followed by the type name; for built-in types, it
   should be just the type name.  If the module is a submodule of a package, the
   full package name is part of the full module name.  For example, a type named
   :class:`T` defined in module :mod:`M` in subpackage :mod:`Q` in package :mod:`P`
   should have the :attr:`tp_name` initializer ``"P.Q.M.T"``.

   For dynamically allocated type objects, this should just be the type name, and
   the module name explicitly stored in the type dict as the value for key
   ``'__module__'``.

   For statically allocated type objects, the tp_name field should contain a dot.
   Everything before the last dot is made accessible as the :attr:`__module__`
   attribute, and everything after the last dot is made accessible as the
   :attr:`__name__` attribute.

   If no dot is present, the entire :attr:`tp_name` field is made accessible as the
   :attr:`__name__` attribute, and the :attr:`__module__` attribute is undefined
   (unless explicitly set in the dictionary, as explained above).  This means your
   type will be impossible to pickle.

   This field is not inherited by subtypes.


.. cmember:: Py_ssize_t PyTypeObject.tp_basicsize
             Py_ssize_t PyTypeObject.tp_itemsize

   These fields allow calculating the size in bytes of instances of the type.

   There are two kinds of types: types with fixed-length instances have a zero
   :attr:`tp_itemsize` field, types with variable-length instances have a non-zero
   :attr:`tp_itemsize` field.  For a type with fixed-length instances, all
   instances have the same size, given in :attr:`tp_basicsize`.

   For a type with variable-length instances, the instances must have an
   :attr:`ob_size` field, and the instance size is :attr:`tp_basicsize` plus N
   times :attr:`tp_itemsize`, where N is the "length" of the object.  The value of
   N is typically stored in the instance's :attr:`ob_size` field.  There are
   exceptions:  for example, long ints use a negative :attr:`ob_size` to indicate a
   negative number, and N is ``abs(ob_size)`` there.  Also, the presence of an
   :attr:`ob_size` field in the instance layout doesn't mean that the instance
   structure is variable-length (for example, the structure for the list type has
   fixed-length instances, yet those instances have a meaningful :attr:`ob_size`
   field).

   The basic size includes the fields in the instance declared by the macro
   :cmacro:`PyObject_HEAD` or :cmacro:`PyObject_VAR_HEAD` (whichever is used to
   declare the instance struct) and this in turn includes the :attr:`_ob_prev` and
   :attr:`_ob_next` fields if they are present.  This means that the only correct
   way to get an initializer for the :attr:`tp_basicsize` is to use the
   :keyword:`sizeof` operator on the struct used to declare the instance layout.
   The basic size does not include the GC header size (this is new in Python 2.2;
   in 2.1 and 2.0, the GC header size was included in :attr:`tp_basicsize`).

   These fields are inherited separately by subtypes.  If the base type has a
   non-zero :attr:`tp_itemsize`, it is generally not safe to set
   :attr:`tp_itemsize` to a different non-zero value in a subtype (though this
   depends on the implementation of the base type).

   A note about alignment: if the variable items require a particular alignment,
   this should be taken care of by the value of :attr:`tp_basicsize`.  Example:
   suppose a type implements an array of ``double``. :attr:`tp_itemsize` is
   ``sizeof(double)``. It is the programmer's responsibility that
   :attr:`tp_basicsize` is a multiple of ``sizeof(double)`` (assuming this is the
   alignment requirement for ``double``).


.. cmember:: destructor PyTypeObject.tp_dealloc

   A pointer to the instance destructor function.  This function must be defined
   unless the type guarantees that its instances will never be deallocated (as is
   the case for the singletons ``None`` and ``Ellipsis``).

   The destructor function is called by the :cfunc:`Py_DECREF` and
   :cfunc:`Py_XDECREF` macros when the new reference count is zero.  At this point,
   the instance is still in existence, but there are no references to it.  The
   destructor function should free all references which the instance owns, free all
   memory buffers owned by the instance (using the freeing function corresponding
   to the allocation function used to allocate the buffer), and finally (as its
   last action) call the type's :attr:`tp_free` function.  If the type is not
   subtypable (doesn't have the :const:`Py_TPFLAGS_BASETYPE` flag bit set), it is
   permissible to call the object deallocator directly instead of via
   :attr:`tp_free`.  The object deallocator should be the one used to allocate the
   instance; this is normally :cfunc:`PyObject_Del` if the instance was allocated
   using :cfunc:`PyObject_New` or :cfunc:`PyObject_VarNew`, or
   :cfunc:`PyObject_GC_Del` if the instance was allocated using
   :cfunc:`PyObject_GC_New` or :cfunc:`PyObject_GC_VarNew`.

   This field is inherited by subtypes.


.. cmember:: printfunc PyTypeObject.tp_print

   An optional pointer to the instance print function.

   The print function is only called when the instance is printed to a *real* file;
   when it is printed to a pseudo-file (like a :class:`StringIO` instance), the
   instance's :attr:`tp_repr` or :attr:`tp_str` function is called to convert it to
   a string.  These are also called when the type's :attr:`tp_print` field is
   *NULL*.  A type should never implement :attr:`tp_print` in a way that produces
   different output than :attr:`tp_repr` or :attr:`tp_str` would.

   The print function is called with the same signature as :cfunc:`PyObject_Print`:
   ``int tp_print(PyObject *self, FILE *file, int flags)``.  The *self* argument is
   the instance to be printed.  The *file* argument is the stdio file to which it
   is to be printed.  The *flags* argument is composed of flag bits. The only flag
   bit currently defined is :const:`Py_PRINT_RAW`. When the :const:`Py_PRINT_RAW`
   flag bit is set, the instance should be printed the same way as :attr:`tp_str`
   would format it; when the :const:`Py_PRINT_RAW` flag bit is clear, the instance
   should be printed the same was as :attr:`tp_repr` would format it. It should
   return ``-1`` and set an exception condition when an error occurred during the
   comparison.

   It is possible that the :attr:`tp_print` field will be deprecated. In any case,
   it is recommended not to define :attr:`tp_print`, but instead to rely on
   :attr:`tp_repr` and :attr:`tp_str` for printing.

   This field is inherited by subtypes.


.. cmember:: getattrfunc PyTypeObject.tp_getattr

   An optional pointer to the get-attribute-string function.

   This field is deprecated.  When it is defined, it should point to a function
   that acts the same as the :attr:`tp_getattro` function, but taking a C string
   instead of a Python string object to give the attribute name.  The signature is
   the same as for :cfunc:`PyObject_GetAttrString`.

   This field is inherited by subtypes together with :attr:`tp_getattro`: a subtype
   inherits both :attr:`tp_getattr` and :attr:`tp_getattro` from its base type when
   the subtype's :attr:`tp_getattr` and :attr:`tp_getattro` are both *NULL*.


.. cmember:: setattrfunc PyTypeObject.tp_setattr

   An optional pointer to the set-attribute-string function.

   This field is deprecated.  When it is defined, it should point to a function
   that acts the same as the :attr:`tp_setattro` function, but taking a C string
   instead of a Python string object to give the attribute name.  The signature is
   the same as for :cfunc:`PyObject_SetAttrString`.

   This field is inherited by subtypes together with :attr:`tp_setattro`: a subtype
   inherits both :attr:`tp_setattr` and :attr:`tp_setattro` from its base type when
   the subtype's :attr:`tp_setattr` and :attr:`tp_setattro` are both *NULL*.


.. cmember:: cmpfunc PyTypeObject.tp_compare

   An optional pointer to the three-way comparison function.

   The signature is the same as for :cfunc:`PyObject_Compare`. The function should
   return ``1`` if *self* greater than *other*, ``0`` if *self* is equal to
   *other*, and ``-1`` if *self* less than *other*.  It should return ``-1`` and
   set an exception condition when an error occurred during the comparison.

   This field is inherited by subtypes together with :attr:`tp_richcompare` and
   :attr:`tp_hash`: a subtypes inherits all three of :attr:`tp_compare`,
   :attr:`tp_richcompare`, and :attr:`tp_hash` when the subtype's
   :attr:`tp_compare`, :attr:`tp_richcompare`, and :attr:`tp_hash` are all *NULL*.


.. cmember:: reprfunc PyTypeObject.tp_repr

   .. index:: builtin: repr

   An optional pointer to a function that implements the built-in function
   :func:`repr`.

   The signature is the same as for :cfunc:`PyObject_Repr`; it must return a string
   or a Unicode object.  Ideally, this function should return a string that, when
   passed to :func:`eval`, given a suitable environment, returns an object with the
   same value.  If this is not feasible, it should return a string starting with
   ``'<'`` and ending with ``'>'`` from which both the type and the value of the
   object can be deduced.

   When this field is not set, a string of the form ``<%s object at %p>`` is
   returned, where ``%s`` is replaced by the type name, and ``%p`` by the object's
   memory address.

   This field is inherited by subtypes.

.. cmember:: PyNumberMethods *tp_as_number;

   XXX

.. cmember:: PySequenceMethods *tp_as_sequence;

   XXX

.. cmember:: PyMappingMethods *tp_as_mapping;

   XXX


.. cmember:: hashfunc PyTypeObject.tp_hash

   .. index:: builtin: hash

   An optional pointer to a function that implements the built-in function
   :func:`hash`.

   The signature is the same as for :cfunc:`PyObject_Hash`; it must return a C
   long.  The value ``-1`` should not be returned as a normal return value; when an
   error occurs during the computation of the hash value, the function should set
   an exception and return ``-1``.

   When this field is not set, two possibilities exist: if the :attr:`tp_compare`
   and :attr:`tp_richcompare` fields are both *NULL*, a default hash value based on
   the object's address is returned; otherwise, a :exc:`TypeError` is raised.

   This field is inherited by subtypes together with :attr:`tp_richcompare` and
   :attr:`tp_compare`: a subtypes inherits all three of :attr:`tp_compare`,
   :attr:`tp_richcompare`, and :attr:`tp_hash`, when the subtype's
   :attr:`tp_compare`, :attr:`tp_richcompare` and :attr:`tp_hash` are all *NULL*.


.. cmember:: ternaryfunc PyTypeObject.tp_call

   An optional pointer to a function that implements calling the object.  This
   should be *NULL* if the object is not callable.  The signature is the same as
   for :cfunc:`PyObject_Call`.

   This field is inherited by subtypes.


.. cmember:: reprfunc PyTypeObject.tp_str

   An optional pointer to a function that implements the built-in operation
   :func:`str`.  (Note that :class:`str` is a type now, and :func:`str` calls the
   constructor for that type.  This constructor calls :cfunc:`PyObject_Str` to do
   the actual work, and :cfunc:`PyObject_Str` will call this handler.)

   The signature is the same as for :cfunc:`PyObject_Str`; it must return a string
   or a Unicode object.  This function should return a "friendly" string
   representation of the object, as this is the representation that will be used by
   the print statement.

   When this field is not set, :cfunc:`PyObject_Repr` is called to return a string
   representation.

   This field is inherited by subtypes.


.. cmember:: getattrofunc PyTypeObject.tp_getattro

   An optional pointer to the get-attribute function.

   The signature is the same as for :cfunc:`PyObject_GetAttr`.  It is usually
   convenient to set this field to :cfunc:`PyObject_GenericGetAttr`, which
   implements the normal way of looking for object attributes.

   This field is inherited by subtypes together with :attr:`tp_getattr`: a subtype
   inherits both :attr:`tp_getattr` and :attr:`tp_getattro` from its base type when
   the subtype's :attr:`tp_getattr` and :attr:`tp_getattro` are both *NULL*.


.. cmember:: setattrofunc PyTypeObject.tp_setattro

   An optional pointer to the set-attribute function.

   The signature is the same as for :cfunc:`PyObject_SetAttr`.  It is usually
   convenient to set this field to :cfunc:`PyObject_GenericSetAttr`, which
   implements the normal way of setting object attributes.

   This field is inherited by subtypes together with :attr:`tp_setattr`: a subtype
   inherits both :attr:`tp_setattr` and :attr:`tp_setattro` from its base type when
   the subtype's :attr:`tp_setattr` and :attr:`tp_setattro` are both *NULL*.


.. cmember:: PyBufferProcs* PyTypeObject.tp_as_buffer

   Pointer to an additional structure that contains fields relevant only to objects
   which implement the buffer interface.  These fields are documented in
   :ref:`buffer-structs`.

   The :attr:`tp_as_buffer` field is not inherited, but the contained fields are
   inherited individually.


.. cmember:: long PyTypeObject.tp_flags

   This field is a bit mask of various flags.  Some flags indicate variant
   semantics for certain situations; others are used to indicate that certain
   fields in the type object (or in the extension structures referenced via
   :attr:`tp_as_number`, :attr:`tp_as_sequence`, :attr:`tp_as_mapping`, and
   :attr:`tp_as_buffer`) that were historically not always present are valid; if
   such a flag bit is clear, the type fields it guards must not be accessed and
   must be considered to have a zero or *NULL* value instead.

   Inheritance of this field is complicated.  Most flag bits are inherited
   individually, i.e. if the base type has a flag bit set, the subtype inherits
   this flag bit.  The flag bits that pertain to extension structures are strictly
   inherited if the extension structure is inherited, i.e. the base type's value of
   the flag bit is copied into the subtype together with a pointer to the extension
   structure.  The :const:`Py_TPFLAGS_HAVE_GC` flag bit is inherited together with
   the :attr:`tp_traverse` and :attr:`tp_clear` fields, i.e. if the
   :const:`Py_TPFLAGS_HAVE_GC` flag bit is clear in the subtype and the
   :attr:`tp_traverse` and :attr:`tp_clear` fields in the subtype exist (as
   indicated by the :const:`Py_TPFLAGS_HAVE_RICHCOMPARE` flag bit) and have *NULL*
   values.

   The following bit masks are currently defined; these can be or-ed together using
   the ``|`` operator to form the value of the :attr:`tp_flags` field.  The macro
   :cfunc:`PyType_HasFeature` takes a type and a flags value, *tp* and *f*, and
   checks whether ``tp->tp_flags & f`` is non-zero.


   .. data:: Py_TPFLAGS_HAVE_GETCHARBUFFER

      If this bit is set, the :ctype:`PyBufferProcs` struct referenced by
      :attr:`tp_as_buffer` has the :attr:`bf_getcharbuffer` field.


   .. data:: Py_TPFLAGS_HAVE_SEQUENCE_IN

      If this bit is set, the :ctype:`PySequenceMethods` struct referenced by
      :attr:`tp_as_sequence` has the :attr:`sq_contains` field.


   .. data:: Py_TPFLAGS_GC

      This bit is obsolete.  The bit it used to name is no longer in use.  The symbol
      is now defined as zero.


   .. data:: Py_TPFLAGS_HAVE_INPLACEOPS

      If this bit is set, the :ctype:`PySequenceMethods` struct referenced by
      :attr:`tp_as_sequence` and the :ctype:`PyNumberMethods` structure referenced by
      :attr:`tp_as_number` contain the fields for in-place operators. In particular,
      this means that the :ctype:`PyNumberMethods` structure has the fields
      :attr:`nb_inplace_add`, :attr:`nb_inplace_subtract`,
      :attr:`nb_inplace_multiply`, :attr:`nb_inplace_divide`,
      :attr:`nb_inplace_remainder`, :attr:`nb_inplace_power`,
      :attr:`nb_inplace_lshift`, :attr:`nb_inplace_rshift`, :attr:`nb_inplace_and`,
      :attr:`nb_inplace_xor`, and :attr:`nb_inplace_or`; and the
      :ctype:`PySequenceMethods` struct has the fields :attr:`sq_inplace_concat` and
      :attr:`sq_inplace_repeat`.


   .. data:: Py_TPFLAGS_CHECKTYPES

      If this bit is set, the binary and ternary operations in the
      :ctype:`PyNumberMethods` structure referenced by :attr:`tp_as_number` accept
      arguments of arbitrary object types, and do their own type conversions if
      needed.  If this bit is clear, those operations require that all arguments have
      the current type as their type, and the caller is supposed to perform a coercion
      operation first.  This applies to :attr:`nb_add`, :attr:`nb_subtract`,
      :attr:`nb_multiply`, :attr:`nb_divide`, :attr:`nb_remainder`, :attr:`nb_divmod`,
      :attr:`nb_power`, :attr:`nb_lshift`, :attr:`nb_rshift`, :attr:`nb_and`,
      :attr:`nb_xor`, and :attr:`nb_or`.


   .. data:: Py_TPFLAGS_HAVE_RICHCOMPARE

      If this bit is set, the type object has the :attr:`tp_richcompare` field, as
      well as the :attr:`tp_traverse` and the :attr:`tp_clear` fields.


   .. data:: Py_TPFLAGS_HAVE_WEAKREFS

      If this bit is set, the :attr:`tp_weaklistoffset` field is defined.  Instances
      of a type are weakly referenceable if the type's :attr:`tp_weaklistoffset` field
      has a value greater than zero.


   .. data:: Py_TPFLAGS_HAVE_ITER

      If this bit is set, the type object has the :attr:`tp_iter` and
      :attr:`tp_iternext` fields.


   .. data:: Py_TPFLAGS_HAVE_CLASS

      If this bit is set, the type object has several new fields defined starting in
      Python 2.2: :attr:`tp_methods`, :attr:`tp_members`, :attr:`tp_getset`,
      :attr:`tp_base`, :attr:`tp_dict`, :attr:`tp_descr_get`, :attr:`tp_descr_set`,
      :attr:`tp_dictoffset`, :attr:`tp_init`, :attr:`tp_alloc`, :attr:`tp_new`,
      :attr:`tp_free`, :attr:`tp_is_gc`, :attr:`tp_bases`, :attr:`tp_mro`,
      :attr:`tp_cache`, :attr:`tp_subclasses`, and :attr:`tp_weaklist`.


   .. data:: Py_TPFLAGS_HEAPTYPE

      This bit is set when the type object itself is allocated on the heap.  In this
      case, the :attr:`ob_type` field of its instances is considered a reference to
      the type, and the type object is INCREF'ed when a new instance is created, and
      DECREF'ed when an instance is destroyed (this does not apply to instances of
      subtypes; only the type referenced by the instance's ob_type gets INCREF'ed or
      DECREF'ed).


   .. data:: Py_TPFLAGS_BASETYPE

      This bit is set when the type can be used as the base type of another type.  If
      this bit is clear, the type cannot be subtyped (similar to a "final" class in
      Java).


   .. data:: Py_TPFLAGS_READY

      This bit is set when the type object has been fully initialized by
      :cfunc:`PyType_Ready`.


   .. data:: Py_TPFLAGS_READYING

      This bit is set while :cfunc:`PyType_Ready` is in the process of initializing
      the type object.


   .. data:: Py_TPFLAGS_HAVE_GC

      This bit is set when the object supports garbage collection.  If this bit
      is set, instances must be created using :cfunc:`PyObject_GC_New` and
      destroyed using :cfunc:`PyObject_GC_Del`.  More information in section
      :ref:`supporting-cycle-detection`.  This bit also implies that the
      GC-related fields :attr:`tp_traverse` and :attr:`tp_clear` are present in
      the type object; but those fields also exist when
      :const:`Py_TPFLAGS_HAVE_GC` is clear but
      :const:`Py_TPFLAGS_HAVE_RICHCOMPARE` is set.


   .. data:: Py_TPFLAGS_DEFAULT

      This is a bitmask of all the bits that pertain to the existence of certain
      fields in the type object and its extension structures. Currently, it includes
      the following bits: :const:`Py_TPFLAGS_HAVE_GETCHARBUFFER`,
      :const:`Py_TPFLAGS_HAVE_SEQUENCE_IN`, :const:`Py_TPFLAGS_HAVE_INPLACEOPS`,
      :const:`Py_TPFLAGS_HAVE_RICHCOMPARE`, :const:`Py_TPFLAGS_HAVE_WEAKREFS`,
      :const:`Py_TPFLAGS_HAVE_ITER`, and :const:`Py_TPFLAGS_HAVE_CLASS`.


.. cmember:: char* PyTypeObject.tp_doc

   An optional pointer to a NUL-terminated C string giving the docstring for this
   type object.  This is exposed as the :attr:`__doc__` attribute on the type and
   instances of the type.

   This field is *not* inherited by subtypes.

The following three fields only exist if the
:const:`Py_TPFLAGS_HAVE_RICHCOMPARE` flag bit is set.


.. cmember:: traverseproc PyTypeObject.tp_traverse

   An optional pointer to a traversal function for the garbage collector.  This is
   only used if the :const:`Py_TPFLAGS_HAVE_GC` flag bit is set.  More information
   about Python's garbage collection scheme can be found in section
   :ref:`supporting-cycle-detection`.

   The :attr:`tp_traverse` pointer is used by the garbage collector to detect
   reference cycles. A typical implementation of a :attr:`tp_traverse` function
   simply calls :cfunc:`Py_VISIT` on each of the instance's members that are Python
   objects.  For exampe, this is function :cfunc:`local_traverse` from the
   :mod:`thread` extension module::

      static int
      local_traverse(localobject *self, visitproc visit, void *arg)
      {
          Py_VISIT(self->args);
          Py_VISIT(self->kw);
          Py_VISIT(self->dict);
          return 0;
      }

   Note that :cfunc:`Py_VISIT` is called only on those members that can participate
   in reference cycles.  Although there is also a ``self->key`` member, it can only
   be *NULL* or a Python string and therefore cannot be part of a reference cycle.

   On the other hand, even if you know a member can never be part of a cycle, as a
   debugging aid you may want to visit it anyway just so the :mod:`gc` module's
   :func:`get_referents` function will include it.

   Note that :cfunc:`Py_VISIT` requires the *visit* and *arg* parameters to
   :cfunc:`local_traverse` to have these specific names; don't name them just
   anything.

   This field is inherited by subtypes together with :attr:`tp_clear` and the
   :const:`Py_TPFLAGS_HAVE_GC` flag bit: the flag bit, :attr:`tp_traverse`, and
   :attr:`tp_clear` are all inherited from the base type if they are all zero in
   the subtype *and* the subtype has the :const:`Py_TPFLAGS_HAVE_RICHCOMPARE` flag
   bit set.


.. cmember:: inquiry PyTypeObject.tp_clear

   An optional pointer to a clear function for the garbage collector. This is only
   used if the :const:`Py_TPFLAGS_HAVE_GC` flag bit is set.

   The :attr:`tp_clear` member function is used to break reference cycles in cyclic
   garbage detected by the garbage collector.  Taken together, all :attr:`tp_clear`
   functions in the system must combine to break all reference cycles.  This is
   subtle, and if in any doubt supply a :attr:`tp_clear` function.  For example,
   the tuple type does not implement a :attr:`tp_clear` function, because it's
   possible to prove that no reference cycle can be composed entirely of tuples.
   Therefore the :attr:`tp_clear` functions of other types must be sufficient to
   break any cycle containing a tuple.  This isn't immediately obvious, and there's
   rarely a good reason to avoid implementing :attr:`tp_clear`.

   Implementations of :attr:`tp_clear` should drop the instance's references to
   those of its members that may be Python objects, and set its pointers to those
   members to *NULL*, as in the following example::

      static int
      local_clear(localobject *self)
      {
          Py_CLEAR(self->key);
          Py_CLEAR(self->args);
          Py_CLEAR(self->kw);
          Py_CLEAR(self->dict);
          return 0;
      }

   The :cfunc:`Py_CLEAR` macro should be used, because clearing references is
   delicate:  the reference to the contained object must not be decremented until
   after the pointer to the contained object is set to *NULL*.  This is because
   decrementing the reference count may cause the contained object to become trash,
   triggering a chain of reclamation activity that may include invoking arbitrary
   Python code (due to finalizers, or weakref callbacks, associated with the
   contained object). If it's possible for such code to reference *self* again,
   it's important that the pointer to the contained object be *NULL* at that time,
   so that *self* knows the contained object can no longer be used.  The
   :cfunc:`Py_CLEAR` macro performs the operations in a safe order.

   Because the goal of :attr:`tp_clear` functions is to break reference cycles,
   it's not necessary to clear contained objects like Python strings or Python
   integers, which can't participate in reference cycles. On the other hand, it may
   be convenient to clear all contained Python objects, and write the type's
   :attr:`tp_dealloc` function to invoke :attr:`tp_clear`.

   More information about Python's garbage collection scheme can be found in
   section :ref:`supporting-cycle-detection`.

   This field is inherited by subtypes together with :attr:`tp_traverse` and the
   :const:`Py_TPFLAGS_HAVE_GC` flag bit: the flag bit, :attr:`tp_traverse`, and
   :attr:`tp_clear` are all inherited from the base type if they are all zero in
   the subtype *and* the subtype has the :const:`Py_TPFLAGS_HAVE_RICHCOMPARE` flag
   bit set.


.. cmember:: richcmpfunc PyTypeObject.tp_richcompare

   An optional pointer to the rich comparison function.

   The signature is the same as for :cfunc:`PyObject_RichCompare`. The function
   should return the result of the comparison (usually ``Py_True`` or
   ``Py_False``).  If the comparison is undefined, it must return
   ``Py_NotImplemented``, if another error occurred it must return ``NULL`` and set
   an exception condition.

   This field is inherited by subtypes together with :attr:`tp_compare` and
   :attr:`tp_hash`: a subtype inherits all three of :attr:`tp_compare`,
   :attr:`tp_richcompare`, and :attr:`tp_hash`, when the subtype's
   :attr:`tp_compare`, :attr:`tp_richcompare`, and :attr:`tp_hash` are all *NULL*.

   The following constants are defined to be used as the third argument for
   :attr:`tp_richcompare` and for :cfunc:`PyObject_RichCompare`:

   +----------------+------------+
   | Constant       | Comparison |
   +================+============+
   | :const:`Py_LT` | ``<``      |
   +----------------+------------+
   | :const:`Py_LE` | ``<=``     |
   +----------------+------------+
   | :const:`Py_EQ` | ``==``     |
   +----------------+------------+
   | :const:`Py_NE` | ``!=``     |
   +----------------+------------+
   | :const:`Py_GT` | ``>``      |
   +----------------+------------+
   | :const:`Py_GE` | ``>=``     |
   +----------------+------------+

The next field only exists if the :const:`Py_TPFLAGS_HAVE_WEAKREFS` flag bit is
set.


.. cmember:: long PyTypeObject.tp_weaklistoffset

   If the instances of this type are weakly referenceable, this field is greater
   than zero and contains the offset in the instance structure of the weak
   reference list head (ignoring the GC header, if present); this offset is used by
   :cfunc:`PyObject_ClearWeakRefs` and the :cfunc:`PyWeakref_\*` functions.  The
   instance structure needs to include a field of type :ctype:`PyObject\*` which is
   initialized to *NULL*.

   Do not confuse this field with :attr:`tp_weaklist`; that is the list head for
   weak references to the type object itself.

   This field is inherited by subtypes, but see the rules listed below. A subtype
   may override this offset; this means that the subtype uses a different weak
   reference list head than the base type.  Since the list head is always found via
   :attr:`tp_weaklistoffset`, this should not be a problem.

   When a type defined by a class statement has no :attr:`__slots__` declaration,
   and none of its base types are weakly referenceable, the type is made weakly
   referenceable by adding a weak reference list head slot to the instance layout
   and setting the :attr:`tp_weaklistoffset` of that slot's offset.

   When a type's :attr:`__slots__` declaration contains a slot named
   :attr:`__weakref__`, that slot becomes the weak reference list head for
   instances of the type, and the slot's offset is stored in the type's
   :attr:`tp_weaklistoffset`.

   When a type's :attr:`__slots__` declaration does not contain a slot named
   :attr:`__weakref__`, the type inherits its :attr:`tp_weaklistoffset` from its
   base type.

The next two fields only exist if the :const:`Py_TPFLAGS_HAVE_CLASS` flag bit is
set.


.. cmember:: getiterfunc PyTypeObject.tp_iter

   An optional pointer to a function that returns an iterator for the object.  Its
   presence normally signals that the instances of this type are iterable (although
   sequences may be iterable without this function, and classic instances always
   have this function, even if they don't define an :meth:`__iter__` method).

   This function has the same signature as :cfunc:`PyObject_GetIter`.

   This field is inherited by subtypes.


.. cmember:: iternextfunc PyTypeObject.tp_iternext

   An optional pointer to a function that returns the next item in an iterator, or
   raises :exc:`StopIteration` when the iterator is exhausted.  Its presence
   normally signals that the instances of this type are iterators (although classic
   instances always have this function, even if they don't define a :meth:`next`
   method).

   Iterator types should also define the :attr:`tp_iter` function, and that
   function should return the iterator instance itself (not a new iterator
   instance).

   This function has the same signature as :cfunc:`PyIter_Next`.

   This field is inherited by subtypes.

The next fields, up to and including :attr:`tp_weaklist`, only exist if the
:const:`Py_TPFLAGS_HAVE_CLASS` flag bit is set.


.. cmember:: struct PyMethodDef* PyTypeObject.tp_methods

   An optional pointer to a static *NULL*-terminated array of :ctype:`PyMethodDef`
   structures, declaring regular methods of this type.

   For each entry in the array, an entry is added to the type's dictionary (see
   :attr:`tp_dict` below) containing a method descriptor.

   This field is not inherited by subtypes (methods are inherited through a
   different mechanism).


.. cmember:: struct PyMemberDef* PyTypeObject.tp_members

   An optional pointer to a static *NULL*-terminated array of :ctype:`PyMemberDef`
   structures, declaring regular data members (fields or slots) of instances of
   this type.

   For each entry in the array, an entry is added to the type's dictionary (see
   :attr:`tp_dict` below) containing a member descriptor.

   This field is not inherited by subtypes (members are inherited through a
   different mechanism).


.. cmember:: struct PyGetSetDef* PyTypeObject.tp_getset

   An optional pointer to a static *NULL*-terminated array of :ctype:`PyGetSetDef`
   structures, declaring computed attributes of instances of this type.

   For each entry in the array, an entry is added to the type's dictionary (see
   :attr:`tp_dict` below) containing a getset descriptor.

   This field is not inherited by subtypes (computed attributes are inherited
   through a different mechanism).

   Docs for PyGetSetDef (XXX belong elsewhere)::

      typedef PyObject *(*getter)(PyObject *, void *);
      typedef int (*setter)(PyObject *, PyObject *, void *);

      typedef struct PyGetSetDef {
          char *name;    /* attribute name */
          getter get;    /* C function to get the attribute */
          setter set;    /* C function to set the attribute */
          char *doc;     /* optional doc string */
          void *closure; /* optional additional data for getter and setter */
      } PyGetSetDef;


.. cmember:: PyTypeObject* PyTypeObject.tp_base

   An optional pointer to a base type from which type properties are inherited.  At
   this level, only single inheritance is supported; multiple inheritance require
   dynamically creating a type object by calling the metatype.

   This field is not inherited by subtypes (obviously), but it defaults to
   ``&PyBaseObject_Type`` (which to Python programmers is known as the type
   :class:`object`).


.. cmember:: PyObject* PyTypeObject.tp_dict

   The type's dictionary is stored here by :cfunc:`PyType_Ready`.

   This field should normally be initialized to *NULL* before PyType_Ready is
   called; it may also be initialized to a dictionary containing initial attributes
   for the type.  Once :cfunc:`PyType_Ready` has initialized the type, extra
   attributes for the type may be added to this dictionary only if they don't
   correspond to overloaded operations (like :meth:`__add__`).

   This field is not inherited by subtypes (though the attributes defined in here
   are inherited through a different mechanism).


.. cmember:: descrgetfunc PyTypeObject.tp_descr_get

   An optional pointer to a "descriptor get" function.

   The function signature is ::

      PyObject * tp_descr_get(PyObject *self, PyObject *obj, PyObject *type);

   XXX blah, blah.

   This field is inherited by subtypes.


.. cmember:: descrsetfunc PyTypeObject.tp_descr_set

   An optional pointer to a "descriptor set" function.

   The function signature is ::

      int tp_descr_set(PyObject *self, PyObject *obj, PyObject *value);

   This field is inherited by subtypes.

   XXX blah, blah.


.. cmember:: long PyTypeObject.tp_dictoffset

   If the instances of this type have a dictionary containing instance variables,
   this field is non-zero and contains the offset in the instances of the type of
   the instance variable dictionary; this offset is used by
   :cfunc:`PyObject_GenericGetAttr`.

   Do not confuse this field with :attr:`tp_dict`; that is the dictionary for
   attributes of the type object itself.

   If the value of this field is greater than zero, it specifies the offset from
   the start of the instance structure.  If the value is less than zero, it
   specifies the offset from the *end* of the instance structure.  A negative
   offset is more expensive to use, and should only be used when the instance
   structure contains a variable-length part.  This is used for example to add an
   instance variable dictionary to subtypes of :class:`str` or :class:`tuple`. Note
   that the :attr:`tp_basicsize` field should account for the dictionary added to
   the end in that case, even though the dictionary is not included in the basic
   object layout.  On a system with a pointer size of 4 bytes,
   :attr:`tp_dictoffset` should be set to ``-4`` to indicate that the dictionary is
   at the very end of the structure.

   The real dictionary offset in an instance can be computed from a negative
   :attr:`tp_dictoffset` as follows::

      dictoffset = tp_basicsize + abs(ob_size)*tp_itemsize + tp_dictoffset
      if dictoffset is not aligned on sizeof(void*):
          round up to sizeof(void*)

   where :attr:`tp_basicsize`, :attr:`tp_itemsize` and :attr:`tp_dictoffset` are
   taken from the type object, and :attr:`ob_size` is taken from the instance.  The
   absolute value is taken because long ints use the sign of :attr:`ob_size` to
   store the sign of the number.  (There's never a need to do this calculation
   yourself; it is done for you by :cfunc:`_PyObject_GetDictPtr`.)

   This field is inherited by subtypes, but see the rules listed below. A subtype
   may override this offset; this means that the subtype instances store the
   dictionary at a difference offset than the base type.  Since the dictionary is
   always found via :attr:`tp_dictoffset`, this should not be a problem.

   When a type defined by a class statement has no :attr:`__slots__` declaration,
   and none of its base types has an instance variable dictionary, a dictionary
   slot is added to the instance layout and the :attr:`tp_dictoffset` is set to
   that slot's offset.

   When a type defined by a class statement has a :attr:`__slots__` declaration,
   the type inherits its :attr:`tp_dictoffset` from its base type.

   (Adding a slot named :attr:`__dict__` to the :attr:`__slots__` declaration does
   not have the expected effect, it just causes confusion.  Maybe this should be
   added as a feature just like :attr:`__weakref__` though.)


.. cmember:: initproc PyTypeObject.tp_init

   An optional pointer to an instance initialization function.

   This function corresponds to the :meth:`__init__` method of classes.  Like
   :meth:`__init__`, it is possible to create an instance without calling
   :meth:`__init__`, and it is possible to reinitialize an instance by calling its
   :meth:`__init__` method again.

   The function signature is ::

      int tp_init(PyObject *self, PyObject *args, PyObject *kwds)

   The self argument is the instance to be initialized; the *args* and *kwds*
   arguments represent positional and keyword arguments of the call to
   :meth:`__init__`.

   The :attr:`tp_init` function, if not *NULL*, is called when an instance is
   created normally by calling its type, after the type's :attr:`tp_new` function
   has returned an instance of the type.  If the :attr:`tp_new` function returns an
   instance of some other type that is not a subtype of the original type, no
   :attr:`tp_init` function is called; if :attr:`tp_new` returns an instance of a
   subtype of the original type, the subtype's :attr:`tp_init` is called.  (VERSION
   NOTE: described here is what is implemented in Python 2.2.1 and later.  In
   Python 2.2, the :attr:`tp_init` of the type of the object returned by
   :attr:`tp_new` was always called, if not *NULL*.)

   This field is inherited by subtypes.


.. cmember:: allocfunc PyTypeObject.tp_alloc

   An optional pointer to an instance allocation function.

   The function signature is ::

      PyObject *tp_alloc(PyTypeObject *self, Py_ssize_t nitems)

   The purpose of this function is to separate memory allocation from memory
   initialization.  It should return a pointer to a block of memory of adequate
   length for the instance, suitably aligned, and initialized to zeros, but with
   :attr:`ob_refcnt` set to ``1`` and :attr:`ob_type` set to the type argument.  If
   the type's :attr:`tp_itemsize` is non-zero, the object's :attr:`ob_size` field
   should be initialized to *nitems* and the length of the allocated memory block
   should be ``tp_basicsize + nitems*tp_itemsize``, rounded up to a multiple of
   ``sizeof(void*)``; otherwise, *nitems* is not used and the length of the block
   should be :attr:`tp_basicsize`.

   Do not use this function to do any other instance initialization, not even to
   allocate additional memory; that should be done by :attr:`tp_new`.

   This field is inherited by static subtypes, but not by dynamic subtypes
   (subtypes created by a class statement); in the latter, this field is always set
   to :cfunc:`PyType_GenericAlloc`, to force a standard heap allocation strategy.
   That is also the recommended value for statically defined types.


.. cmember:: newfunc PyTypeObject.tp_new

   An optional pointer to an instance creation function.

   If this function is *NULL* for a particular type, that type cannot be called to
   create new instances; presumably there is some other way to create instances,
   like a factory function.

   The function signature is ::

      PyObject *tp_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)

   The subtype argument is the type of the object being created; the *args* and
   *kwds* arguments represent positional and keyword arguments of the call to the
   type.  Note that subtype doesn't have to equal the type whose :attr:`tp_new`
   function is called; it may be a subtype of that type (but not an unrelated
   type).

   The :attr:`tp_new` function should call ``subtype->tp_alloc(subtype, nitems)``
   to allocate space for the object, and then do only as much further
   initialization as is absolutely necessary.  Initialization that can safely be
   ignored or repeated should be placed in the :attr:`tp_init` handler.  A good
   rule of thumb is that for immutable types, all initialization should take place
   in :attr:`tp_new`, while for mutable types, most initialization should be
   deferred to :attr:`tp_init`.

   This field is inherited by subtypes, except it is not inherited by static types
   whose :attr:`tp_base` is *NULL* or ``&PyBaseObject_Type``.  The latter exception
   is a precaution so that old extension types don't become callable simply by
   being linked with Python 2.2.


.. cmember:: destructor PyTypeObject.tp_free

   An optional pointer to an instance deallocation function.

   The signature of this function has changed slightly: in Python 2.2 and 2.2.1,
   its signature is :ctype:`destructor`::

      void tp_free(PyObject *)

   In Python 2.3 and beyond, its signature is :ctype:`freefunc`::

      void tp_free(void *)

   The only initializer that is compatible with both versions is ``_PyObject_Del``,
   whose definition has suitably adapted in Python 2.3.

   This field is inherited by static subtypes, but not by dynamic subtypes
   (subtypes created by a class statement); in the latter, this field is set to a
   deallocator suitable to match :cfunc:`PyType_GenericAlloc` and the value of the
   :const:`Py_TPFLAGS_HAVE_GC` flag bit.


.. cmember:: inquiry PyTypeObject.tp_is_gc

   An optional pointer to a function called by the garbage collector.

   The garbage collector needs to know whether a particular object is collectible
   or not.  Normally, it is sufficient to look at the object's type's
   :attr:`tp_flags` field, and check the :const:`Py_TPFLAGS_HAVE_GC` flag bit.  But
   some types have a mixture of statically and dynamically allocated instances, and
   the statically allocated instances are not collectible.  Such types should
   define this function; it should return ``1`` for a collectible instance, and
   ``0`` for a non-collectible instance. The signature is ::

      int tp_is_gc(PyObject *self)

   (The only example of this are types themselves.  The metatype,
   :cdata:`PyType_Type`, defines this function to distinguish between statically
   and dynamically allocated types.)

   This field is inherited by subtypes.  (VERSION NOTE: in Python 2.2, it was not
   inherited.  It is inherited in 2.2.1 and later versions.)


.. cmember:: PyObject* PyTypeObject.tp_bases

   Tuple of base types.

   This is set for types created by a class statement.  It should be *NULL* for
   statically defined types.

   This field is not inherited.


.. cmember:: PyObject* PyTypeObject.tp_mro

   Tuple containing the expanded set of base types, starting with the type itself
   and ending with :class:`object`, in Method Resolution Order.

   This field is not inherited; it is calculated fresh by :cfunc:`PyType_Ready`.


.. cmember:: PyObject* PyTypeObject.tp_cache

   Unused.  Not inherited.  Internal use only.


.. cmember:: PyObject* PyTypeObject.tp_subclasses

   List of weak references to subclasses.  Not inherited.  Internal use only.


.. cmember:: PyObject* PyTypeObject.tp_weaklist

   Weak reference list head, for weak references to this type object.  Not
   inherited.  Internal use only.

The remaining fields are only defined if the feature test macro
:const:`COUNT_ALLOCS` is defined, and are for internal use only. They are
documented here for completeness.  None of these fields are inherited by
subtypes.


.. cmember:: Py_ssize_t PyTypeObject.tp_allocs

   Number of allocations.


.. cmember:: Py_ssize_t PyTypeObject.tp_frees

   Number of frees.


.. cmember:: Py_ssize_t PyTypeObject.tp_maxalloc

   Maximum simultaneously allocated objects.


.. cmember:: PyTypeObject* PyTypeObject.tp_next

   Pointer to the next type object with a non-zero :attr:`tp_allocs` field.

Also, note that, in a garbage collected Python, tp_dealloc may be called from
any Python thread, not just the thread which created the object (if the object
becomes part of a refcount cycle, that cycle might be collected by a garbage
collection on any thread).  This is not a problem for Python API calls, since
the thread on which tp_dealloc is called will own the Global Interpreter Lock
(GIL). However, if the object being destroyed in turn destroys objects from some
other C or C++ library, care should be taken to ensure that destroying those
objects on the thread which called tp_dealloc will not violate any assumptions
of the library.


.. _mapping-structs:

Mapping Object Structures
=========================


.. ctype:: PyMappingMethods

   Structure used to hold pointers to the functions used to implement the mapping
   protocol for an extension type.


.. _number-structs:

Number Object Structures
========================


.. ctype:: PyNumberMethods

   Structure used to hold pointers to the functions an extension type uses to
   implement the number protocol.


.. _sequence-structs:

Sequence Object Structures
==========================


.. ctype:: PySequenceMethods

   Structure used to hold pointers to the functions which an object uses to
   implement the sequence protocol.


.. _buffer-structs:

Buffer Object Structures
========================

.. sectionauthor:: Greg J. Stein <greg@lyra.org>


The buffer interface exports a model where an object can expose its internal
data as a set of chunks of data, where each chunk is specified as a
pointer/length pair.  These chunks are called :dfn:`segments` and are presumed
to be non-contiguous in memory.

If an object does not export the buffer interface, then its :attr:`tp_as_buffer`
member in the :ctype:`PyTypeObject` structure should be *NULL*.  Otherwise, the
:attr:`tp_as_buffer` will point to a :ctype:`PyBufferProcs` structure.

.. note::

   It is very important that your :ctype:`PyTypeObject` structure uses
   :const:`Py_TPFLAGS_DEFAULT` for the value of the :attr:`tp_flags` member rather
   than ``0``.  This tells the Python runtime that your :ctype:`PyBufferProcs`
   structure contains the :attr:`bf_getcharbuffer` slot. Older versions of Python
   did not have this member, so a new Python interpreter using an old extension
   needs to be able to test for its presence before using it.


.. ctype:: PyBufferProcs

   Structure used to hold the function pointers which define an implementation of
   the buffer protocol.

   The first slot is :attr:`bf_getreadbuffer`, of type :ctype:`getreadbufferproc`.
   If this slot is *NULL*, then the object does not support reading from the
   internal data.  This is non-sensical, so implementors should fill this in, but
   callers should test that the slot contains a non-*NULL* value.

   The next slot is :attr:`bf_getwritebuffer` having type
   :ctype:`getwritebufferproc`.  This slot may be *NULL* if the object does not
   allow writing into its returned buffers.

   The third slot is :attr:`bf_getsegcount`, with type :ctype:`getsegcountproc`.
   This slot must not be *NULL* and is used to inform the caller how many segments
   the object contains.  Simple objects such as :ctype:`PyString_Type` and
   :ctype:`PyBuffer_Type` objects contain a single segment.

   .. index:: single: PyType_HasFeature()

   The last slot is :attr:`bf_getcharbuffer`, of type :ctype:`getcharbufferproc`.
   This slot will only be present if the :const:`Py_TPFLAGS_HAVE_GETCHARBUFFER`
   flag is present in the :attr:`tp_flags` field of the object's
   :ctype:`PyTypeObject`. Before using this slot, the caller should test whether it
   is present by using the :cfunc:`PyType_HasFeature` function.  If the flag is
   present, :attr:`bf_getcharbuffer` may be *NULL*, indicating that the object's
   contents cannot be used as *8-bit characters*. The slot function may also raise
   an error if the object's contents cannot be interpreted as 8-bit characters.
   For example, if the object is an array which is configured to hold floating
   point values, an exception may be raised if a caller attempts to use
   :attr:`bf_getcharbuffer` to fetch a sequence of 8-bit characters. This notion of
   exporting the internal buffers as "text" is used to distinguish between objects
   that are binary in nature, and those which have character-based content.

   .. note::

      The current policy seems to state that these characters may be multi-byte
      characters. This implies that a buffer size of *N* does not mean there are *N*
      characters present.


.. data:: Py_TPFLAGS_HAVE_GETCHARBUFFER

   Flag bit set in the type structure to indicate that the :attr:`bf_getcharbuffer`
   slot is known.  This being set does not indicate that the object supports the
   buffer interface or that the :attr:`bf_getcharbuffer` slot is non-*NULL*.


.. ctype:: Py_ssize_t (*readbufferproc) (PyObject *self, Py_ssize_t segment, void **ptrptr)

   Return a pointer to a readable segment of the buffer in ``*ptrptr``.  This
   function is allowed to raise an exception, in which case it must return ``-1``.
   The *segment* which is specified must be zero or positive, and strictly less
   than the number of segments returned by the :attr:`bf_getsegcount` slot
   function.  On success, it returns the length of the segment, and sets
   ``*ptrptr`` to a pointer to that memory.


.. ctype:: Py_ssize_t (*writebufferproc) (PyObject *self, Py_ssize_t segment, void **ptrptr)

   Return a pointer to a writable memory buffer in ``*ptrptr``, and the length of
   that segment as the function return value.  The memory buffer must correspond to
   buffer segment *segment*.  Must return ``-1`` and set an exception on error.
   :exc:`TypeError` should be raised if the object only supports read-only buffers,
   and :exc:`SystemError` should be raised when *segment* specifies a segment that
   doesn't exist.

   .. % Why doesn't it raise ValueError for this one?
   .. % GJS: because you shouldn't be calling it with an invalid
   .. % segment. That indicates a blatant programming error in the C
   .. % code.


.. ctype:: Py_ssize_t (*segcountproc) (PyObject *self, Py_ssize_t *lenp)

   Return the number of memory segments which comprise the buffer.  If *lenp* is
   not *NULL*, the implementation must report the sum of the sizes (in bytes) of
   all segments in ``*lenp``. The function cannot fail.


.. ctype:: Py_ssize_t (*charbufferproc) (PyObject *self, Py_ssize_t segment, const char **ptrptr)

   Return the size of the segment *segment* that *ptrptr*  is set to.  ``*ptrptr``
   is set to the memory buffer. Returns ``-1`` on error.


.. _supporting-iteration:

Supporting the Iterator Protocol
================================


.. _supporting-cycle-detection:

Supporting Cyclic Garbage Collection
====================================

Python's support for detecting and collecting garbage which involves circular
references requires support from object types which are "containers" for other
objects which may also be containers.  Types which do not store references to
other objects, or which only store references to atomic types (such as numbers
or strings), do not need to provide any explicit support for garbage collection.

.. An example showing the use of these interfaces can be found in "Supporting the
.. Cycle Collector (XXX not found: ../ext/example-cycle-support.html)".

To create a container type, the :attr:`tp_flags` field of the type object must
include the :const:`Py_TPFLAGS_HAVE_GC` and provide an implementation of the
:attr:`tp_traverse` handler.  If instances of the type are mutable, a
:attr:`tp_clear` implementation must also be provided.


.. data:: Py_TPFLAGS_HAVE_GC

   Objects with a type with this flag set must conform with the rules documented
   here.  For convenience these objects will be referred to as container objects.

Constructors for container types must conform to two rules:

#. The memory for the object must be allocated using :cfunc:`PyObject_GC_New` or
   :cfunc:`PyObject_GC_VarNew`.

#. Once all the fields which may contain references to other containers are
   initialized, it must call :cfunc:`PyObject_GC_Track`.


.. cfunction:: TYPE* PyObject_GC_New(TYPE, PyTypeObject *type)

   Analogous to :cfunc:`PyObject_New` but for container objects with the
   :const:`Py_TPFLAGS_HAVE_GC` flag set.


.. cfunction:: TYPE* PyObject_GC_NewVar(TYPE, PyTypeObject *type, Py_ssize_t size)

   Analogous to :cfunc:`PyObject_NewVar` but for container objects with the
   :const:`Py_TPFLAGS_HAVE_GC` flag set.


.. cfunction:: PyVarObject * PyObject_GC_Resize(PyVarObject *op, Py_ssize_t)

   Resize an object allocated by :cfunc:`PyObject_NewVar`.  Returns the resized
   object or *NULL* on failure.


.. cfunction:: void PyObject_GC_Track(PyObject *op)

   Adds the object *op* to the set of container objects tracked by the collector.
   The collector can run at unexpected times so objects must be valid while being
   tracked.  This should be called once all the fields followed by the
   :attr:`tp_traverse` handler become valid, usually near the end of the
   constructor.


.. cfunction:: void _PyObject_GC_TRACK(PyObject *op)

   A macro version of :cfunc:`PyObject_GC_Track`.  It should not be used for
   extension modules.

Similarly, the deallocator for the object must conform to a similar pair of
rules:

#. Before fields which refer to other containers are invalidated,
   :cfunc:`PyObject_GC_UnTrack` must be called.

#. The object's memory must be deallocated using :cfunc:`PyObject_GC_Del`.


.. cfunction:: void PyObject_GC_Del(void *op)

   Releases memory allocated to an object using :cfunc:`PyObject_GC_New` or
   :cfunc:`PyObject_GC_NewVar`.


.. cfunction:: void PyObject_GC_UnTrack(void *op)

   Remove the object *op* from the set of container objects tracked by the
   collector.  Note that :cfunc:`PyObject_GC_Track` can be called again on this
   object to add it back to the set of tracked objects.  The deallocator
   (:attr:`tp_dealloc` handler) should call this for the object before any of the
   fields used by the :attr:`tp_traverse` handler become invalid.


.. cfunction:: void _PyObject_GC_UNTRACK(PyObject *op)

   A macro version of :cfunc:`PyObject_GC_UnTrack`.  It should not be used for
   extension modules.

The :attr:`tp_traverse` handler accepts a function parameter of this type:


.. ctype:: int (*visitproc)(PyObject *object, void *arg)

   Type of the visitor function passed to the :attr:`tp_traverse` handler.  The
   function should be called with an object to traverse as *object* and the third
   parameter to the :attr:`tp_traverse` handler as *arg*.  The Python core uses
   several visitor functions to implement cyclic garbage detection; it's not
   expected that users will need to write their own visitor functions.

The :attr:`tp_traverse` handler must have the following type:


.. ctype:: int (*traverseproc)(PyObject *self, visitproc visit, void *arg)

   Traversal function for a container object.  Implementations must call the
   *visit* function for each object directly contained by *self*, with the
   parameters to *visit* being the contained object and the *arg* value passed to
   the handler.  The *visit* function must not be called with a *NULL* object
   argument.  If *visit* returns a non-zero value that value should be returned
   immediately.

To simplify writing :attr:`tp_traverse` handlers, a :cfunc:`Py_VISIT` macro is
provided.  In order to use this macro, the :attr:`tp_traverse` implementation
must name its arguments exactly *visit* and *arg*:


.. cfunction:: void Py_VISIT(PyObject *o)

   Call the *visit* callback, with arguments *o* and *arg*. If *visit* returns a
   non-zero value, then return it.  Using this macro, :attr:`tp_traverse` handlers
   look like::

      static int
      my_traverse(Noddy *self, visitproc visit, void *arg)
      {
          Py_VISIT(self->foo);
          Py_VISIT(self->bar);
          return 0;
      }

   .. versionadded:: 2.4

The :attr:`tp_clear` handler must be of the :ctype:`inquiry` type, or *NULL* if
the object is immutable.


.. ctype:: int (*inquiry)(PyObject *self)

   Drop references that may have created reference cycles.  Immutable objects do
   not have to define this method since they can never directly create reference
   cycles.  Note that the object must still be valid after calling this method
   (don't just call :cfunc:`Py_DECREF` on a reference).  The collector will call
   this method if it detects that this object is involved in a reference cycle.

