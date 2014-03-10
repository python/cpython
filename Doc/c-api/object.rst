.. highlightlang:: c

.. _object:

Object Protocol
===============


.. c:var:: PyObject* Py_NotImplemented

   The ``NotImplemented`` singleton, used to signal that an operation is
   not implemented for the given type combination.


.. c:macro:: Py_RETURN_NOTIMPLEMENTED

   Properly handle returning :c:data:`Py_NotImplemented` from within a C
   function (that is, increment the reference count of NotImplemented and
   return it).


.. c:function:: int PyObject_Print(PyObject *o, FILE *fp, int flags)

   Print an object *o*, on file *fp*.  Returns ``-1`` on error.  The flags argument
   is used to enable certain printing options.  The only option currently supported
   is :const:`Py_PRINT_RAW`; if given, the :func:`str` of the object is written
   instead of the :func:`repr`.


.. c:function:: int PyObject_HasAttr(PyObject *o, PyObject *attr_name)

   Returns ``1`` if *o* has the attribute *attr_name*, and ``0`` otherwise.  This
   is equivalent to the Python expression ``hasattr(o, attr_name)``.  This function
   always succeeds.


.. c:function:: int PyObject_HasAttrString(PyObject *o, const char *attr_name)

   Returns ``1`` if *o* has the attribute *attr_name*, and ``0`` otherwise.  This
   is equivalent to the Python expression ``hasattr(o, attr_name)``.  This function
   always succeeds.


.. c:function:: PyObject* PyObject_GetAttr(PyObject *o, PyObject *attr_name)

   Retrieve an attribute named *attr_name* from object *o*. Returns the attribute
   value on success, or *NULL* on failure.  This is the equivalent of the Python
   expression ``o.attr_name``.


.. c:function:: PyObject* PyObject_GetAttrString(PyObject *o, const char *attr_name)

   Retrieve an attribute named *attr_name* from object *o*. Returns the attribute
   value on success, or *NULL* on failure. This is the equivalent of the Python
   expression ``o.attr_name``.


.. c:function:: PyObject* PyObject_GenericGetAttr(PyObject *o, PyObject *name)

   Generic attribute getter function that is meant to be put into a type
   object's ``tp_getattro`` slot.  It looks for a descriptor in the dictionary
   of classes in the object's MRO as well as an attribute in the object's
   :attr:`~object.__dict__` (if present).  As outlined in :ref:`descriptors`,
   data descriptors take preference over instance attributes, while non-data
   descriptors don't.  Otherwise, an :exc:`AttributeError` is raised.


.. c:function:: int PyObject_SetAttr(PyObject *o, PyObject *attr_name, PyObject *v)

   Set the value of the attribute named *attr_name*, for object *o*, to the value
   *v*. Returns ``-1`` on failure.  This is the equivalent of the Python statement
   ``o.attr_name = v``.


.. c:function:: int PyObject_SetAttrString(PyObject *o, const char *attr_name, PyObject *v)

   Set the value of the attribute named *attr_name*, for object *o*, to the value
   *v*. Returns ``-1`` on failure.  This is the equivalent of the Python statement
   ``o.attr_name = v``.


.. c:function:: int PyObject_GenericSetAttr(PyObject *o, PyObject *name, PyObject *value)

   Generic attribute setter function that is meant to be put into a type
   object's ``tp_setattro`` slot.  It looks for a data descriptor in the
   dictionary of classes in the object's MRO, and if found it takes preference
   over setting the attribute in the instance dictionary. Otherwise, the
   attribute is set in the object's :attr:`~object.__dict__` (if present).
   Otherwise, an :exc:`AttributeError` is raised and ``-1`` is returned.


.. c:function:: int PyObject_DelAttr(PyObject *o, PyObject *attr_name)

   Delete attribute named *attr_name*, for object *o*. Returns ``-1`` on failure.
   This is the equivalent of the Python statement ``del o.attr_name``.


.. c:function:: int PyObject_DelAttrString(PyObject *o, const char *attr_name)

   Delete attribute named *attr_name*, for object *o*. Returns ``-1`` on failure.
   This is the equivalent of the Python statement ``del o.attr_name``.


.. c:function:: PyObject* PyType_GenericGetDict(PyObject *o, void *context)

   A generic implementation for the getter of a ``__dict__`` descriptor. It
   creates the dictionary if necessary.

   .. versionadded:: 3.3


.. c:function:: int PyType_GenericSetDict(PyObject *o, void *context)

   A generic implementation for the setter of a ``__dict__`` descriptor. This
   implementation does not allow the dictionary to be deleted.

   .. versionadded:: 3.3


.. c:function:: PyObject* PyObject_RichCompare(PyObject *o1, PyObject *o2, int opid)

   Compare the values of *o1* and *o2* using the operation specified by *opid*,
   which must be one of :const:`Py_LT`, :const:`Py_LE`, :const:`Py_EQ`,
   :const:`Py_NE`, :const:`Py_GT`, or :const:`Py_GE`, corresponding to ``<``,
   ``<=``, ``==``, ``!=``, ``>``, or ``>=`` respectively. This is the equivalent of
   the Python expression ``o1 op o2``, where ``op`` is the operator corresponding
   to *opid*. Returns the value of the comparison on success, or *NULL* on failure.


.. c:function:: int PyObject_RichCompareBool(PyObject *o1, PyObject *o2, int opid)

   Compare the values of *o1* and *o2* using the operation specified by *opid*,
   which must be one of :const:`Py_LT`, :const:`Py_LE`, :const:`Py_EQ`,
   :const:`Py_NE`, :const:`Py_GT`, or :const:`Py_GE`, corresponding to ``<``,
   ``<=``, ``==``, ``!=``, ``>``, or ``>=`` respectively. Returns ``-1`` on error,
   ``0`` if the result is false, ``1`` otherwise. This is the equivalent of the
   Python expression ``o1 op o2``, where ``op`` is the operator corresponding to
   *opid*.

.. note::
   If *o1* and *o2* are the same object, :c:func:`PyObject_RichCompareBool`
   will always return ``1`` for :const:`Py_EQ` and ``0`` for :const:`Py_NE`.

.. c:function:: PyObject* PyObject_Repr(PyObject *o)

   .. index:: builtin: repr

   Compute a string representation of object *o*.  Returns the string
   representation on success, *NULL* on failure.  This is the equivalent of the
   Python expression ``repr(o)``.  Called by the :func:`repr` built-in function.

   .. versionchanged:: 3.4
      This function now includes a debug assertion to help ensure that it
      does not silently discard an active exception.

.. c:function:: PyObject* PyObject_ASCII(PyObject *o)

   .. index:: builtin: ascii

   As :c:func:`PyObject_Repr`, compute a string representation of object *o*, but
   escape the non-ASCII characters in the string returned by
   :c:func:`PyObject_Repr` with ``\x``, ``\u`` or ``\U`` escapes.  This generates
   a string similar to that returned by :c:func:`PyObject_Repr` in Python 2.
   Called by the :func:`ascii` built-in function.

   .. index:: string; PyObject_Str (C function)


.. c:function:: PyObject* PyObject_Str(PyObject *o)

   Compute a string representation of object *o*.  Returns the string
   representation on success, *NULL* on failure.  This is the equivalent of the
   Python expression ``str(o)``.  Called by the :func:`str` built-in function
   and, therefore, by the :func:`print` function.

   .. versionchanged:: 3.4
      This function now includes a debug assertion to help ensure that it
      does not silently discard an active exception.

.. c:function:: PyObject* PyObject_Bytes(PyObject *o)

   .. index:: builtin: bytes

   Compute a bytes representation of object *o*.  *NULL* is returned on
   failure and a bytes object on success.  This is equivalent to the Python
   expression ``bytes(o)``, when *o* is not an integer.  Unlike ``bytes(o)``,
   a TypeError is raised when *o* is an integer instead of a zero-initialized
   bytes object.

.. c:function:: int PyObject_IsInstance(PyObject *inst, PyObject *cls)

   Returns ``1`` if *inst* is an instance of the class *cls* or a subclass of
   *cls*, or ``0`` if not.  On error, returns ``-1`` and sets an exception.  If
   *cls* is a type object rather than a class object, :c:func:`PyObject_IsInstance`
   returns ``1`` if *inst* is of type *cls*.  If *cls* is a tuple, the check will
   be done against every entry in *cls*. The result will be ``1`` when at least one
   of the checks returns ``1``, otherwise it will be ``0``. If *inst* is not a
   class instance and *cls* is neither a type object, nor a class object, nor a
   tuple, *inst* must have a :attr:`~instance.__class__` attribute --- the
   class relationship of the value of that attribute with *cls* will be used
   to determine the result of this function.


Subclass determination is done in a fairly straightforward way, but includes a
wrinkle that implementors of extensions to the class system may want to be aware
of.  If :class:`A` and :class:`B` are class objects, :class:`B` is a subclass of
:class:`A` if it inherits from :class:`A` either directly or indirectly.  If
either is not a class object, a more general mechanism is used to determine the
class relationship of the two objects.  When testing if *B* is a subclass of
*A*, if *A* is *B*, :c:func:`PyObject_IsSubclass` returns true.  If *A* and *B*
are different objects, *B*'s :attr:`~class.__bases__` attribute is searched in
a depth-first fashion for *A* --- the presence of the :attr:`~class.__bases__`
attribute is considered sufficient for this determination.


.. c:function:: int PyObject_IsSubclass(PyObject *derived, PyObject *cls)

   Returns ``1`` if the class *derived* is identical to or derived from the class
   *cls*, otherwise returns ``0``.  In case of an error, returns ``-1``. If *cls*
   is a tuple, the check will be done against every entry in *cls*. The result will
   be ``1`` when at least one of the checks returns ``1``, otherwise it will be
   ``0``. If either *derived* or *cls* is not an actual class object (or tuple),
   this function uses the generic algorithm described above.


.. c:function:: int PyCallable_Check(PyObject *o)

   Determine if the object *o* is callable.  Return ``1`` if the object is callable
   and ``0`` otherwise.  This function always succeeds.


.. c:function:: PyObject* PyObject_Call(PyObject *callable_object, PyObject *args, PyObject *kw)

   Call a callable Python object *callable_object*, with arguments given by the
   tuple *args*, and named arguments given by the dictionary *kw*. If no named
   arguments are needed, *kw* may be *NULL*. *args* must not be *NULL*, use an
   empty tuple if no arguments are needed. Returns the result of the call on
   success, or *NULL* on failure.  This is the equivalent of the Python expression
   ``callable_object(*args, **kw)``.


.. c:function:: PyObject* PyObject_CallObject(PyObject *callable_object, PyObject *args)

   Call a callable Python object *callable_object*, with arguments given by the
   tuple *args*.  If no arguments are needed, then *args* may be *NULL*.  Returns
   the result of the call on success, or *NULL* on failure.  This is the equivalent
   of the Python expression ``callable_object(*args)``.


.. c:function:: PyObject* PyObject_CallFunction(PyObject *callable, const char *format, ...)

   Call a callable Python object *callable*, with a variable number of C arguments.
   The C arguments are described using a :c:func:`Py_BuildValue` style format
   string.  The format may be *NULL*, indicating that no arguments are provided.
   Returns the result of the call on success, or *NULL* on failure.  This is the
   equivalent of the Python expression ``callable(*args)``. Note that if you only
   pass :c:type:`PyObject \*` args, :c:func:`PyObject_CallFunctionObjArgs` is a
   faster alternative.

   .. versionchanged:: 3.4
      The type of *format* was changed from ``char *``.


.. c:function:: PyObject* PyObject_CallMethod(PyObject *o, const char *method, const char *format, ...)

   Call the method named *method* of object *o* with a variable number of C
   arguments.  The C arguments are described by a :c:func:`Py_BuildValue` format
   string that should  produce a tuple.  The format may be *NULL*, indicating that
   no arguments are provided. Returns the result of the call on success, or *NULL*
   on failure.  This is the equivalent of the Python expression ``o.method(args)``.
   Note that if you only pass :c:type:`PyObject \*` args,
   :c:func:`PyObject_CallMethodObjArgs` is a faster alternative.

   .. versionchanged:: 3.4
      The types of *method* and *format* were changed from ``char *``.


.. c:function:: PyObject* PyObject_CallFunctionObjArgs(PyObject *callable, ..., NULL)

   Call a callable Python object *callable*, with a variable number of
   :c:type:`PyObject\*` arguments.  The arguments are provided as a variable number
   of parameters followed by *NULL*. Returns the result of the call on success, or
   *NULL* on failure.


.. c:function:: PyObject* PyObject_CallMethodObjArgs(PyObject *o, PyObject *name, ..., NULL)

   Calls a method of the object *o*, where the name of the method is given as a
   Python string object in *name*.  It is called with a variable number of
   :c:type:`PyObject\*` arguments.  The arguments are provided as a variable number
   of parameters followed by *NULL*. Returns the result of the call on success, or
   *NULL* on failure.


.. c:function:: Py_hash_t PyObject_Hash(PyObject *o)

   .. index:: builtin: hash

   Compute and return the hash value of an object *o*.  On failure, return ``-1``.
   This is the equivalent of the Python expression ``hash(o)``.

   .. versionchanged:: 3.2
      The return type is now Py_hash_t.  This is a signed integer the same size
      as Py_ssize_t.


.. c:function:: Py_hash_t PyObject_HashNotImplemented(PyObject *o)

   Set a :exc:`TypeError` indicating that ``type(o)`` is not hashable and return ``-1``.
   This function receives special treatment when stored in a ``tp_hash`` slot,
   allowing a type to explicitly indicate to the interpreter that it is not
   hashable.


.. c:function:: int PyObject_IsTrue(PyObject *o)

   Returns ``1`` if the object *o* is considered to be true, and ``0`` otherwise.
   This is equivalent to the Python expression ``not not o``.  On failure, return
   ``-1``.


.. c:function:: int PyObject_Not(PyObject *o)

   Returns ``0`` if the object *o* is considered to be true, and ``1`` otherwise.
   This is equivalent to the Python expression ``not o``.  On failure, return
   ``-1``.


.. c:function:: PyObject* PyObject_Type(PyObject *o)

   .. index:: builtin: type

   When *o* is non-*NULL*, returns a type object corresponding to the object type
   of object *o*. On failure, raises :exc:`SystemError` and returns *NULL*.  This
   is equivalent to the Python expression ``type(o)``. This function increments the
   reference count of the return value. There's really no reason to use this
   function instead of the common expression ``o->ob_type``, which returns a
   pointer of type :c:type:`PyTypeObject\*`, except when the incremented reference
   count is needed.


.. c:function:: int PyObject_TypeCheck(PyObject *o, PyTypeObject *type)

   Return true if the object *o* is of type *type* or a subtype of *type*.  Both
   parameters must be non-*NULL*.


.. c:function:: Py_ssize_t PyObject_Length(PyObject *o)
               Py_ssize_t PyObject_Size(PyObject *o)

   .. index:: builtin: len

   Return the length of object *o*.  If the object *o* provides either the sequence
   and mapping protocols, the sequence length is returned.  On error, ``-1`` is
   returned.  This is the equivalent to the Python expression ``len(o)``.


.. c:function:: Py_ssize_t PyObject_LengthHint(PyObject *o, Py_ssize_t default)

   Return an estimated length for the object *o*. First try to return its
   actual length, then an estimate using :meth:`~object.__length_hint__`, and
   finally return the default value. On error return ``-1``. This is the
   equivalent to the Python expression ``operator.length_hint(o, default)``.

   .. versionadded:: 3.4

.. c:function:: PyObject* PyObject_GetItem(PyObject *o, PyObject *key)

   Return element of *o* corresponding to the object *key* or *NULL* on failure.
   This is the equivalent of the Python expression ``o[key]``.


.. c:function:: int PyObject_SetItem(PyObject *o, PyObject *key, PyObject *v)

   Map the object *key* to the value *v*.  Returns ``-1`` on failure.  This is the
   equivalent of the Python statement ``o[key] = v``.


.. c:function:: int PyObject_DelItem(PyObject *o, PyObject *key)

   Delete the mapping for *key* from *o*.  Returns ``-1`` on failure. This is the
   equivalent of the Python statement ``del o[key]``.


.. c:function:: PyObject* PyObject_Dir(PyObject *o)

   This is equivalent to the Python expression ``dir(o)``, returning a (possibly
   empty) list of strings appropriate for the object argument, or *NULL* if there
   was an error.  If the argument is *NULL*, this is like the Python ``dir()``,
   returning the names of the current locals; in this case, if no execution frame
   is active then *NULL* is returned but :c:func:`PyErr_Occurred` will return false.


.. c:function:: PyObject* PyObject_GetIter(PyObject *o)

   This is equivalent to the Python expression ``iter(o)``. It returns a new
   iterator for the object argument, or the object  itself if the object is already
   an iterator.  Raises :exc:`TypeError` and returns *NULL* if the object cannot be
   iterated.
