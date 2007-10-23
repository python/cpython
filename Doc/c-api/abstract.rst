.. highlightlang:: c


.. _abstract:

**********************
Abstract Objects Layer
**********************

The functions in this chapter interact with Python objects regardless of their
type, or with wide classes of object types (e.g. all numerical types, or all
sequence types).  When used on object types for which they do not apply, they
will raise a Python exception.

It is not possible to use these functions on objects that are not properly
initialized, such as a list object that has been created by :cfunc:`PyList_New`,
but whose items have not been set to some non-\ ``NULL`` value yet.


.. _object:

Object Protocol
===============


.. cfunction:: int PyObject_Print(PyObject *o, FILE *fp, int flags)

   Print an object *o*, on file *fp*.  Returns ``-1`` on error.  The flags argument
   is used to enable certain printing options.  The only option currently supported
   is :const:`Py_PRINT_RAW`; if given, the :func:`str` of the object is written
   instead of the :func:`repr`.


.. cfunction:: int PyObject_HasAttrString(PyObject *o, const char *attr_name)

   Returns ``1`` if *o* has the attribute *attr_name*, and ``0`` otherwise.  This
   is equivalent to the Python expression ``hasattr(o, attr_name)``.  This function
   always succeeds.


.. cfunction:: PyObject* PyObject_GetAttrString(PyObject *o, const char *attr_name)

   Retrieve an attribute named *attr_name* from object *o*. Returns the attribute
   value on success, or *NULL* on failure. This is the equivalent of the Python
   expression ``o.attr_name``.


.. cfunction:: int PyObject_HasAttr(PyObject *o, PyObject *attr_name)

   Returns ``1`` if *o* has the attribute *attr_name*, and ``0`` otherwise.  This
   is equivalent to the Python expression ``hasattr(o, attr_name)``.  This function
   always succeeds.


.. cfunction:: PyObject* PyObject_GetAttr(PyObject *o, PyObject *attr_name)

   Retrieve an attribute named *attr_name* from object *o*. Returns the attribute
   value on success, or *NULL* on failure.  This is the equivalent of the Python
   expression ``o.attr_name``.


.. cfunction:: int PyObject_SetAttrString(PyObject *o, const char *attr_name, PyObject *v)

   Set the value of the attribute named *attr_name*, for object *o*, to the value
   *v*. Returns ``-1`` on failure.  This is the equivalent of the Python statement
   ``o.attr_name = v``.


.. cfunction:: int PyObject_SetAttr(PyObject *o, PyObject *attr_name, PyObject *v)

   Set the value of the attribute named *attr_name*, for object *o*, to the value
   *v*. Returns ``-1`` on failure.  This is the equivalent of the Python statement
   ``o.attr_name = v``.


.. cfunction:: int PyObject_DelAttrString(PyObject *o, const char *attr_name)

   Delete attribute named *attr_name*, for object *o*. Returns ``-1`` on failure.
   This is the equivalent of the Python statement: ``del o.attr_name``.


.. cfunction:: int PyObject_DelAttr(PyObject *o, PyObject *attr_name)

   Delete attribute named *attr_name*, for object *o*. Returns ``-1`` on failure.
   This is the equivalent of the Python statement ``del o.attr_name``.


.. cfunction:: PyObject* PyObject_RichCompare(PyObject *o1, PyObject *o2, int opid)

   Compare the values of *o1* and *o2* using the operation specified by *opid*,
   which must be one of :const:`Py_LT`, :const:`Py_LE`, :const:`Py_EQ`,
   :const:`Py_NE`, :const:`Py_GT`, or :const:`Py_GE`, corresponding to ``<``,
   ``<=``, ``==``, ``!=``, ``>``, or ``>=`` respectively. This is the equivalent of
   the Python expression ``o1 op o2``, where ``op`` is the operator corresponding
   to *opid*. Returns the value of the comparison on success, or *NULL* on failure.


.. cfunction:: int PyObject_RichCompareBool(PyObject *o1, PyObject *o2, int opid)

   Compare the values of *o1* and *o2* using the operation specified by *opid*,
   which must be one of :const:`Py_LT`, :const:`Py_LE`, :const:`Py_EQ`,
   :const:`Py_NE`, :const:`Py_GT`, or :const:`Py_GE`, corresponding to ``<``,
   ``<=``, ``==``, ``!=``, ``>``, or ``>=`` respectively. Returns ``-1`` on error,
   ``0`` if the result is false, ``1`` otherwise. This is the equivalent of the
   Python expression ``o1 op o2``, where ``op`` is the operator corresponding to
   *opid*.


.. cfunction:: int PyObject_Cmp(PyObject *o1, PyObject *o2, int *result)

   .. index:: builtin: cmp

   Compare the values of *o1* and *o2* using a routine provided by *o1*, if one
   exists, otherwise with a routine provided by *o2*.  The result of the comparison
   is returned in *result*.  Returns ``-1`` on failure.  This is the equivalent of
   the Python statement ``result = cmp(o1, o2)``.


.. cfunction:: int PyObject_Compare(PyObject *o1, PyObject *o2)

   .. index:: builtin: cmp

   Compare the values of *o1* and *o2* using a routine provided by *o1*, if one
   exists, otherwise with a routine provided by *o2*.  Returns the result of the
   comparison on success.  On error, the value returned is undefined; use
   :cfunc:`PyErr_Occurred` to detect an error.  This is equivalent to the Python
   expression ``cmp(o1, o2)``.


.. cfunction:: PyObject* PyObject_Repr(PyObject *o)

   .. index:: builtin: repr

   Compute a string representation of object *o*.  Returns the string
   representation on success, *NULL* on failure.  This is the equivalent of the
   Python expression ``repr(o)``.  Called by the :func:`repr` built-in function and
   by reverse quotes.


.. cfunction:: PyObject* PyObject_Str(PyObject *o)

   .. index:: builtin: str

   Compute a string representation of object *o*.  Returns the string
   representation on success, *NULL* on failure.  This is the equivalent of the
   Python expression ``str(o)``.  Called by the :func:`str` built-in function
   and, therefore, by the :func:`print` function.


.. cfunction:: PyObject* PyObject_Unicode(PyObject *o)

   .. index:: builtin: unicode

   Compute a Unicode string representation of object *o*.  Returns the Unicode
   string representation on success, *NULL* on failure. This is the equivalent of
   the Python expression ``unicode(o)``.  Called by the :func:`unicode` built-in
   function.


.. cfunction:: int PyObject_IsInstance(PyObject *inst, PyObject *cls)

   Returns ``1`` if *inst* is an instance of the class *cls* or a subclass of
   *cls*, or ``0`` if not.  On error, returns ``-1`` and sets an exception.  If
   *cls* is a type object rather than a class object, :cfunc:`PyObject_IsInstance`
   returns ``1`` if *inst* is of type *cls*.  If *cls* is a tuple, the check will
   be done against every entry in *cls*. The result will be ``1`` when at least one
   of the checks returns ``1``, otherwise it will be ``0``. If *inst* is not a
   class instance and *cls* is neither a type object, nor a class object, nor a
   tuple, *inst* must have a :attr:`__class__` attribute --- the class relationship
   of the value of that attribute with *cls* will be used to determine the result
   of this function.


Subclass determination is done in a fairly straightforward way, but includes a
wrinkle that implementors of extensions to the class system may want to be aware
of.  If :class:`A` and :class:`B` are class objects, :class:`B` is a subclass of
:class:`A` if it inherits from :class:`A` either directly or indirectly.  If
either is not a class object, a more general mechanism is used to determine the
class relationship of the two objects.  When testing if *B* is a subclass of
*A*, if *A* is *B*, :cfunc:`PyObject_IsSubclass` returns true.  If *A* and *B*
are different objects, *B*'s :attr:`__bases__` attribute is searched in a
depth-first fashion for *A* --- the presence of the :attr:`__bases__` attribute
is considered sufficient for this determination.


.. cfunction:: int PyObject_IsSubclass(PyObject *derived, PyObject *cls)

   Returns ``1`` if the class *derived* is identical to or derived from the class
   *cls*, otherwise returns ``0``.  In case of an error, returns ``-1``. If *cls*
   is a tuple, the check will be done against every entry in *cls*. The result will
   be ``1`` when at least one of the checks returns ``1``, otherwise it will be
   ``0``. If either *derived* or *cls* is not an actual class object (or tuple),
   this function uses the generic algorithm described above.


.. cfunction:: int PyCallable_Check(PyObject *o)

   Determine if the object *o* is callable.  Return ``1`` if the object is callable
   and ``0`` otherwise.  This function always succeeds.


.. cfunction:: PyObject* PyObject_Call(PyObject *callable_object, PyObject *args, PyObject *kw)

   Call a callable Python object *callable_object*, with arguments given by the
   tuple *args*, and named arguments given by the dictionary *kw*. If no named
   arguments are needed, *kw* may be *NULL*. *args* must not be *NULL*, use an
   empty tuple if no arguments are needed. Returns the result of the call on
   success, or *NULL* on failure.  This is the equivalent of the Python expression
   ``callable_object(*args, **kw)``.


.. cfunction:: PyObject* PyObject_CallObject(PyObject *callable_object, PyObject *args)

   Call a callable Python object *callable_object*, with arguments given by the
   tuple *args*.  If no arguments are needed, then *args* may be *NULL*.  Returns
   the result of the call on success, or *NULL* on failure.  This is the equivalent
   of the Python expression ``callable_object(*args)``.


.. cfunction:: PyObject* PyObject_CallFunction(PyObject *callable, char *format, ...)

   Call a callable Python object *callable*, with a variable number of C arguments.
   The C arguments are described using a :cfunc:`Py_BuildValue` style format
   string.  The format may be *NULL*, indicating that no arguments are provided.
   Returns the result of the call on success, or *NULL* on failure.  This is the
   equivalent of the Python expression ``callable(*args)``. Note that if you only
   pass :ctype:`PyObject \*` args, :cfunc:`PyObject_CallFunctionObjArgs` is a
   faster alternative.


.. cfunction:: PyObject* PyObject_CallMethod(PyObject *o, char *method, char *format, ...)

   Call the method named *method* of object *o* with a variable number of C
   arguments.  The C arguments are described by a :cfunc:`Py_BuildValue` format
   string that should  produce a tuple.  The format may be *NULL*, indicating that
   no arguments are provided. Returns the result of the call on success, or *NULL*
   on failure.  This is the equivalent of the Python expression ``o.method(args)``.
   Note that if you only pass :ctype:`PyObject \*` args,
   :cfunc:`PyObject_CallMethodObjArgs` is a faster alternative.


.. cfunction:: PyObject* PyObject_CallFunctionObjArgs(PyObject *callable, ..., NULL)

   Call a callable Python object *callable*, with a variable number of
   :ctype:`PyObject\*` arguments.  The arguments are provided as a variable number
   of parameters followed by *NULL*. Returns the result of the call on success, or
   *NULL* on failure.


.. cfunction:: PyObject* PyObject_CallMethodObjArgs(PyObject *o, PyObject *name, ..., NULL)

   Calls a method of the object *o*, where the name of the method is given as a
   Python string object in *name*.  It is called with a variable number of
   :ctype:`PyObject\*` arguments.  The arguments are provided as a variable number
   of parameters followed by *NULL*. Returns the result of the call on success, or
   *NULL* on failure.


.. cfunction:: long PyObject_Hash(PyObject *o)

   .. index:: builtin: hash

   Compute and return the hash value of an object *o*.  On failure, return ``-1``.
   This is the equivalent of the Python expression ``hash(o)``.


.. cfunction:: int PyObject_IsTrue(PyObject *o)

   Returns ``1`` if the object *o* is considered to be true, and ``0`` otherwise.
   This is equivalent to the Python expression ``not not o``.  On failure, return
   ``-1``.


.. cfunction:: int PyObject_Not(PyObject *o)

   Returns ``0`` if the object *o* is considered to be true, and ``1`` otherwise.
   This is equivalent to the Python expression ``not o``.  On failure, return
   ``-1``.


.. cfunction:: PyObject* PyObject_Type(PyObject *o)

   .. index:: builtin: type

   When *o* is non-*NULL*, returns a type object corresponding to the object type
   of object *o*. On failure, raises :exc:`SystemError` and returns *NULL*.  This
   is equivalent to the Python expression ``type(o)``. This function increments the
   reference count of the return value. There's really no reason to use this
   function instead of the common expression ``o->ob_type``, which returns a
   pointer of type :ctype:`PyTypeObject\*`, except when the incremented reference
   count is needed.


.. cfunction:: int PyObject_TypeCheck(PyObject *o, PyTypeObject *type)

   Return true if the object *o* is of type *type* or a subtype of *type*.  Both
   parameters must be non-*NULL*.


.. cfunction:: Py_ssize_t PyObject_Length(PyObject *o)
               Py_ssize_t PyObject_Size(PyObject *o)

   .. index:: builtin: len

   Return the length of object *o*.  If the object *o* provides either the sequence
   and mapping protocols, the sequence length is returned.  On error, ``-1`` is
   returned.  This is the equivalent to the Python expression ``len(o)``.


.. cfunction:: PyObject* PyObject_GetItem(PyObject *o, PyObject *key)

   Return element of *o* corresponding to the object *key* or *NULL* on failure.
   This is the equivalent of the Python expression ``o[key]``.


.. cfunction:: int PyObject_SetItem(PyObject *o, PyObject *key, PyObject *v)

   Map the object *key* to the value *v*.  Returns ``-1`` on failure.  This is the
   equivalent of the Python statement ``o[key] = v``.


.. cfunction:: int PyObject_DelItem(PyObject *o, PyObject *key)

   Delete the mapping for *key* from *o*.  Returns ``-1`` on failure. This is the
   equivalent of the Python statement ``del o[key]``.


.. cfunction:: int PyObject_AsFileDescriptor(PyObject *o)

   Derives a file-descriptor from a Python object.


.. cfunction:: PyObject* PyObject_Dir(PyObject *o)

   This is equivalent to the Python expression ``dir(o)``, returning a (possibly
   empty) list of strings appropriate for the object argument, or *NULL* if there
   was an error.  If the argument is *NULL*, this is like the Python ``dir()``,
   returning the names of the current locals; in this case, if no execution frame
   is active then *NULL* is returned but :cfunc:`PyErr_Occurred` will return false.


.. cfunction:: PyObject* PyObject_GetIter(PyObject *o)

   This is equivalent to the Python expression ``iter(o)``. It returns a new
   iterator for the object argument, or the object  itself if the object is already
   an iterator.  Raises :exc:`TypeError` and returns *NULL* if the object cannot be
   iterated.


.. _number:

Number Protocol
===============


.. cfunction:: int PyNumber_Check(PyObject *o)

   Returns ``1`` if the object *o* provides numeric protocols, and false otherwise.
   This function always succeeds.


.. cfunction:: PyObject* PyNumber_Add(PyObject *o1, PyObject *o2)

   Returns the result of adding *o1* and *o2*, or *NULL* on failure.  This is the
   equivalent of the Python expression ``o1 + o2``.


.. cfunction:: PyObject* PyNumber_Subtract(PyObject *o1, PyObject *o2)

   Returns the result of subtracting *o2* from *o1*, or *NULL* on failure.  This is
   the equivalent of the Python expression ``o1 - o2``.


.. cfunction:: PyObject* PyNumber_Multiply(PyObject *o1, PyObject *o2)

   Returns the result of multiplying *o1* and *o2*, or *NULL* on failure.  This is
   the equivalent of the Python expression ``o1 * o2``.


.. cfunction:: PyObject* PyNumber_Divide(PyObject *o1, PyObject *o2)

   Returns the result of dividing *o1* by *o2*, or *NULL* on failure.  This is the
   equivalent of the Python expression ``o1 / o2``.


.. cfunction:: PyObject* PyNumber_FloorDivide(PyObject *o1, PyObject *o2)

   Return the floor of *o1* divided by *o2*, or *NULL* on failure.  This is
   equivalent to the "classic" division of integers.


.. cfunction:: PyObject* PyNumber_TrueDivide(PyObject *o1, PyObject *o2)

   Return a reasonable approximation for the mathematical value of *o1* divided by
   *o2*, or *NULL* on failure.  The return value is "approximate" because binary
   floating point numbers are approximate; it is not possible to represent all real
   numbers in base two.  This function can return a floating point value when
   passed two integers.


.. cfunction:: PyObject* PyNumber_Remainder(PyObject *o1, PyObject *o2)

   Returns the remainder of dividing *o1* by *o2*, or *NULL* on failure.  This is
   the equivalent of the Python expression ``o1 % o2``.


.. cfunction:: PyObject* PyNumber_Divmod(PyObject *o1, PyObject *o2)

   .. index:: builtin: divmod

   See the built-in function :func:`divmod`. Returns *NULL* on failure.  This is
   the equivalent of the Python expression ``divmod(o1, o2)``.


.. cfunction:: PyObject* PyNumber_Power(PyObject *o1, PyObject *o2, PyObject *o3)

   .. index:: builtin: pow

   See the built-in function :func:`pow`. Returns *NULL* on failure.  This is the
   equivalent of the Python expression ``pow(o1, o2, o3)``, where *o3* is optional.
   If *o3* is to be ignored, pass :cdata:`Py_None` in its place (passing *NULL* for
   *o3* would cause an illegal memory access).


.. cfunction:: PyObject* PyNumber_Negative(PyObject *o)

   Returns the negation of *o* on success, or *NULL* on failure. This is the
   equivalent of the Python expression ``-o``.


.. cfunction:: PyObject* PyNumber_Positive(PyObject *o)

   Returns *o* on success, or *NULL* on failure.  This is the equivalent of the
   Python expression ``+o``.


.. cfunction:: PyObject* PyNumber_Absolute(PyObject *o)

   .. index:: builtin: abs

   Returns the absolute value of *o*, or *NULL* on failure.  This is the equivalent
   of the Python expression ``abs(o)``.


.. cfunction:: PyObject* PyNumber_Invert(PyObject *o)

   Returns the bitwise negation of *o* on success, or *NULL* on failure.  This is
   the equivalent of the Python expression ``~o``.


.. cfunction:: PyObject* PyNumber_Lshift(PyObject *o1, PyObject *o2)

   Returns the result of left shifting *o1* by *o2* on success, or *NULL* on
   failure.  This is the equivalent of the Python expression ``o1 << o2``.


.. cfunction:: PyObject* PyNumber_Rshift(PyObject *o1, PyObject *o2)

   Returns the result of right shifting *o1* by *o2* on success, or *NULL* on
   failure.  This is the equivalent of the Python expression ``o1 >> o2``.


.. cfunction:: PyObject* PyNumber_And(PyObject *o1, PyObject *o2)

   Returns the "bitwise and" of *o1* and *o2* on success and *NULL* on failure.
   This is the equivalent of the Python expression ``o1 & o2``.


.. cfunction:: PyObject* PyNumber_Xor(PyObject *o1, PyObject *o2)

   Returns the "bitwise exclusive or" of *o1* by *o2* on success, or *NULL* on
   failure.  This is the equivalent of the Python expression ``o1 ^ o2``.


.. cfunction:: PyObject* PyNumber_Or(PyObject *o1, PyObject *o2)

   Returns the "bitwise or" of *o1* and *o2* on success, or *NULL* on failure.
   This is the equivalent of the Python expression ``o1 | o2``.


.. cfunction:: PyObject* PyNumber_InPlaceAdd(PyObject *o1, PyObject *o2)

   Returns the result of adding *o1* and *o2*, or *NULL* on failure.  The operation
   is done *in-place* when *o1* supports it.  This is the equivalent of the Python
   statement ``o1 += o2``.


.. cfunction:: PyObject* PyNumber_InPlaceSubtract(PyObject *o1, PyObject *o2)

   Returns the result of subtracting *o2* from *o1*, or *NULL* on failure.  The
   operation is done *in-place* when *o1* supports it.  This is the equivalent of
   the Python statement ``o1 -= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceMultiply(PyObject *o1, PyObject *o2)

   Returns the result of multiplying *o1* and *o2*, or *NULL* on failure.  The
   operation is done *in-place* when *o1* supports it.  This is the equivalent of
   the Python statement ``o1 *= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceDivide(PyObject *o1, PyObject *o2)

   Returns the result of dividing *o1* by *o2*, or *NULL* on failure.  The
   operation is done *in-place* when *o1* supports it. This is the equivalent of
   the Python statement ``o1 /= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceFloorDivide(PyObject *o1, PyObject *o2)

   Returns the mathematical floor of dividing *o1* by *o2*, or *NULL* on failure.
   The operation is done *in-place* when *o1* supports it.  This is the equivalent
   of the Python statement ``o1 //= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceTrueDivide(PyObject *o1, PyObject *o2)

   Return a reasonable approximation for the mathematical value of *o1* divided by
   *o2*, or *NULL* on failure.  The return value is "approximate" because binary
   floating point numbers are approximate; it is not possible to represent all real
   numbers in base two.  This function can return a floating point value when
   passed two integers.  The operation is done *in-place* when *o1* supports it.


.. cfunction:: PyObject* PyNumber_InPlaceRemainder(PyObject *o1, PyObject *o2)

   Returns the remainder of dividing *o1* by *o2*, or *NULL* on failure.  The
   operation is done *in-place* when *o1* supports it.  This is the equivalent of
   the Python statement ``o1 %= o2``.


.. cfunction:: PyObject* PyNumber_InPlacePower(PyObject *o1, PyObject *o2, PyObject *o3)

   .. index:: builtin: pow

   See the built-in function :func:`pow`. Returns *NULL* on failure.  The operation
   is done *in-place* when *o1* supports it.  This is the equivalent of the Python
   statement ``o1 **= o2`` when o3 is :cdata:`Py_None`, or an in-place variant of
   ``pow(o1, o2, o3)`` otherwise. If *o3* is to be ignored, pass :cdata:`Py_None`
   in its place (passing *NULL* for *o3* would cause an illegal memory access).


.. cfunction:: PyObject* PyNumber_InPlaceLshift(PyObject *o1, PyObject *o2)

   Returns the result of left shifting *o1* by *o2* on success, or *NULL* on
   failure.  The operation is done *in-place* when *o1* supports it.  This is the
   equivalent of the Python statement ``o1 <<= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceRshift(PyObject *o1, PyObject *o2)

   Returns the result of right shifting *o1* by *o2* on success, or *NULL* on
   failure.  The operation is done *in-place* when *o1* supports it.  This is the
   equivalent of the Python statement ``o1 >>= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceAnd(PyObject *o1, PyObject *o2)

   Returns the "bitwise and" of *o1* and *o2* on success and *NULL* on failure. The
   operation is done *in-place* when *o1* supports it.  This is the equivalent of
   the Python statement ``o1 &= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceXor(PyObject *o1, PyObject *o2)

   Returns the "bitwise exclusive or" of *o1* by *o2* on success, or *NULL* on
   failure.  The operation is done *in-place* when *o1* supports it.  This is the
   equivalent of the Python statement ``o1 ^= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceOr(PyObject *o1, PyObject *o2)

   Returns the "bitwise or" of *o1* and *o2* on success, or *NULL* on failure.  The
   operation is done *in-place* when *o1* supports it.  This is the equivalent of
   the Python statement ``o1 |= o2``.


.. cfunction:: PyObject* PyNumber_Int(PyObject *o)

   .. index:: builtin: int

   Returns the *o* converted to an integer object on success, or *NULL* on failure.
   If the argument is outside the integer range a long object will be returned
   instead. This is the equivalent of the Python expression ``int(o)``.


.. cfunction:: PyObject* PyNumber_Long(PyObject *o)

   .. index:: builtin: long

   Returns the *o* converted to a long integer object on success, or *NULL* on
   failure.  This is the equivalent of the Python expression ``long(o)``.


.. cfunction:: PyObject* PyNumber_Float(PyObject *o)

   .. index:: builtin: float

   Returns the *o* converted to a float object on success, or *NULL* on failure.
   This is the equivalent of the Python expression ``float(o)``.


.. cfunction:: PyObject* PyNumber_Index(PyObject *o)

   Returns the *o* converted to a Python int or long on success or *NULL* with a
   TypeError exception raised on failure.


.. cfunction:: Py_ssize_t PyNumber_AsSsize_t(PyObject *o, PyObject *exc)

   Returns *o* converted to a Py_ssize_t value if *o* can be interpreted as an
   integer. If *o* can be converted to a Python int or long but the attempt to
   convert to a Py_ssize_t value would raise an :exc:`OverflowError`, then the
   *exc* argument is the type of exception that will be raised (usually
   :exc:`IndexError` or :exc:`OverflowError`).  If *exc* is *NULL*, then the
   exception is cleared and the value is clipped to *PY_SSIZE_T_MIN* for a negative
   integer or *PY_SSIZE_T_MAX* for a positive integer.


.. cfunction:: int PyIndex_Check(PyObject *o)

   Returns True if *o* is an index integer (has the nb_index slot of  the
   tp_as_number structure filled in).


.. _sequence:

Sequence Protocol
=================


.. cfunction:: int PySequence_Check(PyObject *o)

   Return ``1`` if the object provides sequence protocol, and ``0`` otherwise.
   This function always succeeds.


.. cfunction:: Py_ssize_t PySequence_Size(PyObject *o)

   .. index:: builtin: len

   Returns the number of objects in sequence *o* on success, and ``-1`` on failure.
   For objects that do not provide sequence protocol, this is equivalent to the
   Python expression ``len(o)``.


.. cfunction:: Py_ssize_t PySequence_Length(PyObject *o)

   Alternate name for :cfunc:`PySequence_Size`.


.. cfunction:: PyObject* PySequence_Concat(PyObject *o1, PyObject *o2)

   Return the concatenation of *o1* and *o2* on success, and *NULL* on failure.
   This is the equivalent of the Python expression ``o1 + o2``.


.. cfunction:: PyObject* PySequence_Repeat(PyObject *o, Py_ssize_t count)

   Return the result of repeating sequence object *o* *count* times, or *NULL* on
   failure.  This is the equivalent of the Python expression ``o * count``.


.. cfunction:: PyObject* PySequence_InPlaceConcat(PyObject *o1, PyObject *o2)

   Return the concatenation of *o1* and *o2* on success, and *NULL* on failure.
   The operation is done *in-place* when *o1* supports it.  This is the equivalent
   of the Python expression ``o1 += o2``.


.. cfunction:: PyObject* PySequence_InPlaceRepeat(PyObject *o, Py_ssize_t count)

   Return the result of repeating sequence object *o* *count* times, or *NULL* on
   failure.  The operation is done *in-place* when *o* supports it.  This is the
   equivalent of the Python expression ``o *= count``.


.. cfunction:: PyObject* PySequence_GetItem(PyObject *o, Py_ssize_t i)

   Return the *i*th element of *o*, or *NULL* on failure. This is the equivalent of
   the Python expression ``o[i]``.


.. cfunction:: PyObject* PySequence_GetSlice(PyObject *o, Py_ssize_t i1, Py_ssize_t i2)

   Return the slice of sequence object *o* between *i1* and *i2*, or *NULL* on
   failure. This is the equivalent of the Python expression ``o[i1:i2]``.


.. cfunction:: int PySequence_SetItem(PyObject *o, Py_ssize_t i, PyObject *v)

   Assign object *v* to the *i*th element of *o*.  Returns ``-1`` on failure.  This
   is the equivalent of the Python statement ``o[i] = v``.  This function *does
   not* steal a reference to *v*.


.. cfunction:: int PySequence_DelItem(PyObject *o, Py_ssize_t i)

   Delete the *i*th element of object *o*.  Returns ``-1`` on failure.  This is the
   equivalent of the Python statement ``del o[i]``.


.. cfunction:: int PySequence_SetSlice(PyObject *o, Py_ssize_t i1, Py_ssize_t i2, PyObject *v)

   Assign the sequence object *v* to the slice in sequence object *o* from *i1* to
   *i2*.  This is the equivalent of the Python statement ``o[i1:i2] = v``.


.. cfunction:: int PySequence_DelSlice(PyObject *o, Py_ssize_t i1, Py_ssize_t i2)

   Delete the slice in sequence object *o* from *i1* to *i2*.  Returns ``-1`` on
   failure.  This is the equivalent of the Python statement ``del o[i1:i2]``.


.. cfunction:: Py_ssize_t PySequence_Count(PyObject *o, PyObject *value)

   Return the number of occurrences of *value* in *o*, that is, return the number
   of keys for which ``o[key] == value``.  On failure, return ``-1``.  This is
   equivalent to the Python expression ``o.count(value)``.


.. cfunction:: int PySequence_Contains(PyObject *o, PyObject *value)

   Determine if *o* contains *value*.  If an item in *o* is equal to *value*,
   return ``1``, otherwise return ``0``. On error, return ``-1``.  This is
   equivalent to the Python expression ``value in o``.


.. cfunction:: Py_ssize_t PySequence_Index(PyObject *o, PyObject *value)

   Return the first index *i* for which ``o[i] == value``.  On error, return
   ``-1``.    This is equivalent to the Python expression ``o.index(value)``.


.. cfunction:: PyObject* PySequence_List(PyObject *o)

   Return a list object with the same contents as the arbitrary sequence *o*.  The
   returned list is guaranteed to be new.


.. cfunction:: PyObject* PySequence_Tuple(PyObject *o)

   .. index:: builtin: tuple

   Return a tuple object with the same contents as the arbitrary sequence *o* or
   *NULL* on failure.  If *o* is a tuple, a new reference will be returned,
   otherwise a tuple will be constructed with the appropriate contents.  This is
   equivalent to the Python expression ``tuple(o)``.


.. cfunction:: PyObject* PySequence_Fast(PyObject *o, const char *m)

   Returns the sequence *o* as a tuple, unless it is already a tuple or list, in
   which case *o* is returned.  Use :cfunc:`PySequence_Fast_GET_ITEM` to access the
   members of the result.  Returns *NULL* on failure.  If the object is not a
   sequence, raises :exc:`TypeError` with *m* as the message text.


.. cfunction:: PyObject* PySequence_Fast_GET_ITEM(PyObject *o, Py_ssize_t i)

   Return the *i*th element of *o*, assuming that *o* was returned by
   :cfunc:`PySequence_Fast`, *o* is not *NULL*, and that *i* is within bounds.


.. cfunction:: PyObject** PySequence_Fast_ITEMS(PyObject *o)

   Return the underlying array of PyObject pointers.  Assumes that *o* was returned
   by :cfunc:`PySequence_Fast` and *o* is not *NULL*.


.. cfunction:: PyObject* PySequence_ITEM(PyObject *o, Py_ssize_t i)

   Return the *i*th element of *o* or *NULL* on failure. Macro form of
   :cfunc:`PySequence_GetItem` but without checking that
   :cfunc:`PySequence_Check(o)` is true and without adjustment for negative
   indices.


.. cfunction:: Py_ssize_t PySequence_Fast_GET_SIZE(PyObject *o)

   Returns the length of *o*, assuming that *o* was returned by
   :cfunc:`PySequence_Fast` and that *o* is not *NULL*.  The size can also be
   gotten by calling :cfunc:`PySequence_Size` on *o*, but
   :cfunc:`PySequence_Fast_GET_SIZE` is faster because it can assume *o* is a list
   or tuple.


.. _mapping:

Mapping Protocol
================


.. cfunction:: int PyMapping_Check(PyObject *o)

   Return ``1`` if the object provides mapping protocol, and ``0`` otherwise.  This
   function always succeeds.


.. cfunction:: Py_ssize_t PyMapping_Length(PyObject *o)

   .. index:: builtin: len

   Returns the number of keys in object *o* on success, and ``-1`` on failure.  For
   objects that do not provide mapping protocol, this is equivalent to the Python
   expression ``len(o)``.


.. cfunction:: int PyMapping_DelItemString(PyObject *o, char *key)

   Remove the mapping for object *key* from the object *o*. Return ``-1`` on
   failure.  This is equivalent to the Python statement ``del o[key]``.


.. cfunction:: int PyMapping_DelItem(PyObject *o, PyObject *key)

   Remove the mapping for object *key* from the object *o*. Return ``-1`` on
   failure.  This is equivalent to the Python statement ``del o[key]``.


.. cfunction:: int PyMapping_HasKeyString(PyObject *o, char *key)

   On success, return ``1`` if the mapping object has the key *key* and ``0``
   otherwise.  This is equivalent to the Python expression ``key in o``.
   This function always succeeds.


.. cfunction:: int PyMapping_HasKey(PyObject *o, PyObject *key)

   Return ``1`` if the mapping object has the key *key* and ``0`` otherwise.  This
   is equivalent to the Python expression ``key in o``.  This function always
   succeeds.


.. cfunction:: PyObject* PyMapping_Keys(PyObject *o)

   On success, return a list of the keys in object *o*.  On failure, return *NULL*.
   This is equivalent to the Python expression ``o.keys()``.


.. cfunction:: PyObject* PyMapping_Values(PyObject *o)

   On success, return a list of the values in object *o*.  On failure, return
   *NULL*. This is equivalent to the Python expression ``o.values()``.


.. cfunction:: PyObject* PyMapping_Items(PyObject *o)

   On success, return a list of the items in object *o*, where each item is a tuple
   containing a key-value pair.  On failure, return *NULL*. This is equivalent to
   the Python expression ``o.items()``.


.. cfunction:: PyObject* PyMapping_GetItemString(PyObject *o, char *key)

   Return element of *o* corresponding to the object *key* or *NULL* on failure.
   This is the equivalent of the Python expression ``o[key]``.


.. cfunction:: int PyMapping_SetItemString(PyObject *o, char *key, PyObject *v)

   Map the object *key* to the value *v* in object *o*. Returns ``-1`` on failure.
   This is the equivalent of the Python statement ``o[key] = v``.


.. _iterator:

Iterator Protocol
=================

There are only a couple of functions specifically for working with iterators.

.. cfunction:: int PyIter_Check(PyObject *o)

   Return true if the object *o* supports the iterator protocol.


.. cfunction:: PyObject* PyIter_Next(PyObject *o)

   Return the next value from the iteration *o*.  If the object is an iterator,
   this retrieves the next value from the iteration, and returns *NULL* with no
   exception set if there are no remaining items.  If the object is not an
   iterator, :exc:`TypeError` is raised, or if there is an error in retrieving the
   item, returns *NULL* and passes along the exception.

To write a loop which iterates over an iterator, the C code should look
something like this::

   PyObject *iterator = PyObject_GetIter(obj);
   PyObject *item;

   if (iterator == NULL) {
       /* propagate error */
   }

   while (item = PyIter_Next(iterator)) {
       /* do something with item */
       ...
       /* release reference when done */
       Py_DECREF(item);
   }

   Py_DECREF(iterator);

   if (PyErr_Occurred()) {
       /* propagate error */
   }
   else {
       /* continue doing useful work */
   }


.. _abstract-buffer:

Buffer Protocol
===============


.. cfunction:: int PyObject_AsCharBuffer(PyObject *obj, const char **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a read-only memory location useable as character- based
   input.  The *obj* argument must support the single-segment character buffer
   interface.  On success, returns ``0``, sets *buffer* to the memory location and
   *buffer_len* to the buffer length.  Returns ``-1`` and sets a :exc:`TypeError`
   on error.


.. cfunction:: int PyObject_AsReadBuffer(PyObject *obj, const void **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a read-only memory location containing arbitrary data.  The
   *obj* argument must support the single-segment readable buffer interface.  On
   success, returns ``0``, sets *buffer* to the memory location and *buffer_len* to
   the buffer length.  Returns ``-1`` and sets a :exc:`TypeError` on error.


.. cfunction:: int PyObject_CheckReadBuffer(PyObject *o)

   Returns ``1`` if *o* supports the single-segment readable buffer interface.
   Otherwise returns ``0``.


.. cfunction:: int PyObject_AsWriteBuffer(PyObject *obj, void **buffer, Py_ssize_t *buffer_len)

   Returns a pointer to a writable memory location.  The *obj* argument must
   support the single-segment, character buffer interface.  On success, returns
   ``0``, sets *buffer* to the memory location and *buffer_len* to the buffer
   length.  Returns ``-1`` and sets a :exc:`TypeError` on error.

