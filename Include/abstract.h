#ifndef Py_ABSTRACTOBJECT_H
#define Py_ABSTRACTOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/* Abstract Object Interface (many thanks to Jim Fulton) */

/*
   PROPOSAL: A Generic Python Object Interface for Python C Modules

Problem

  Python modules written in C that must access Python objects must do
  so through routines whose interfaces are described by a set of
  include files.  Unfortunately, these routines vary according to the
  object accessed.  To use these routines, the C programmer must check
  the type of the object being used and must call a routine based on
  the object type.  For example, to access an element of a sequence,
  the programmer must determine whether the sequence is a list or a
  tuple:

    if(is_tupleobject(o))
      e=gettupleitem(o,i)
    else if(is_listitem(o))
      e=getlistitem(o,i)

  If the programmer wants to get an item from another type of object
  that provides sequence behavior, there is no clear way to do it
  correctly.  

  The persistent programmer may peruse object.h and find that the
  _typeobject structure provides a means of invoking up to (currently
  about) 41 special operators.  So, for example, a routine can get an
  item from any object that provides sequence behavior. However, to
  use this mechanism, the programmer must make their code dependent on
  the current Python implementation.

  Also, certain semantics, especially memory management semantics, may
  differ by the type of object being used.  Unfortunately, these
  semantics are not clearly described in the current include files.
  An abstract interface providing more consistent semantics is needed. 

Proposal

  I propose the creation of a standard interface (with an associated
  library of routines and/or macros) for generically obtaining the
  services of Python objects.  This proposal can be viewed as one
  components of a Python C interface consisting of several components.

  From the viewpoint of of C access to Python services, we have (as
  suggested by Guido in off-line discussions):

  - "Very high level layer": two or three functions that let you exec or
    eval arbitrary Python code given as a string in a module whose name is
    given, passing C values in and getting C values out using
    mkvalue/getargs style format strings.  This does not require the user
    to declare any variables of type "PyObject *".  This should be enough
    to write a simple application that gets Python code from the user,
    execs it, and returns the output or errors.  (Error handling must also
    be part of this API.)

  - "Abstract objects layer": which is the subject of this proposal.
    It has many functions operating on objects, and lest you do many
    things from C that you can also write in Python, without going
    through the Python parser.

  - "Concrete objects layer": This is the public type-dependent
    interface provided by the standard built-in types, such as floats,
    strings, and lists.  This interface exists and is currently
    documented by the collection of include files provides with the
    Python distributions.

  From the point of view of Python accessing services provided by C
  modules: 

  - "Python module interface": this interface consist of the basic
    routines used to define modules and their members.  Most of the
    current extensions-writing guide deals with this interface.

  - "Built-in object interface": this is the interface that a new
    built-in type must provide and the mechanisms and rules that a
    developer of a new built-in type must use and follow.

  This proposal is a "first-cut" that is intended to spur
  discussion. See especially the lists of notes.

  The Python C object interface will provide four protocols: object,
  numeric, sequence, and mapping.  Each protocol consists of a
  collection of related operations.  If an operation that is not
  provided by a particular type is invoked, then a standard exception,
  NotImplementedError is raised with a operation name as an argument.
  In addition, for convenience this interface defines a set of
  constructors for building objects of built-in types.  This is needed
  so new objects can be returned from C functions that otherwise treat
  objects generically.

Memory Management

  For all of the functions described in this proposal, if a function
  retains a reference to a Python object passed as an argument, then the
  function will increase the reference count of the object.  It is
  unnecessary for the caller to increase the reference count of an
  argument in anticipation of the object's retention.

  All Python objects returned from functions should be treated as new
  objects.  Functions that return objects assume that the caller will
  retain a reference and the reference count of the object has already
  been incremented to account for this fact.  A caller that does not
  retain a reference to an object that is returned from a function
  must decrement the reference count of the object (using
  DECREF(object)) to prevent memory leaks.

  Note that the behavior mentioned here is different from the current
  behavior for some objects (e.g. lists and tuples) when certain
  type-specific routines are called directly (e.g. setlistitem).  The
  proposed abstraction layer will provide a consistent memory
  management interface, correcting for inconsistent behavior for some
  built-in types.

Protocols

xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx*/

/*  Object Protocol: */

     /* Implemented elsewhere:

     int PyObject_Print(PyObject *o, FILE *fp, int flags);

         Print an object, o, on file, fp.  Returns -1 on
	 error.  The flags argument is used to enable certain printing
	 options. The only option currently supported is Py_Print_RAW. 

         (What should be said about Py_Print_RAW?)	

       */

     /* Implemented elsewhere:

     int PyObject_HasAttrString(PyObject *o, char *attr_name);

         Returns 1 if o has the attribute attr_name, and 0 otherwise.
	 This is equivalent to the Python expression:
	 hasattr(o,attr_name). 

	 This function always succeeds.

       */

     /* Implemented elsewhere:

     PyObject* PyObject_GetAttrString(PyObject *o, char *attr_name);

	 Retrieve an attributed named attr_name form object o.
	 Returns the attribute value on success, or NULL on failure.
	 This is the equivalent of the Python expression: o.attr_name.

       */

     /* Implemented elsewhere:

     int PyObject_HasAttr(PyObject *o, PyObject *attr_name);

         Returns 1 if o has the attribute attr_name, and 0 otherwise.
	 This is equivalent to the Python expression:
	 hasattr(o,attr_name). 

	 This function always succeeds.

       */

     /* Implemented elsewhere:

     PyObject* PyObject_GetAttr(PyObject *o, PyObject *attr_name);

	 Retrieve an attributed named attr_name form object o.
	 Returns the attribute value on success, or NULL on failure.
	 This is the equivalent of the Python expression: o.attr_name.

       */


     /* Implemented elsewhere:

     int PyObject_SetAttrString(PyObject *o, char *attr_name, PyObject *v);

	 Set the value of the attribute named attr_name, for object o,
	 to the value, v. Returns -1 on failure.  This is
	 the equivalent of the Python statement: o.attr_name=v.

       */

     /* Implemented elsewhere:

     int PyObject_SetAttr(PyObject *o, PyObject *attr_name, PyObject *v);

	 Set the value of the attribute named attr_name, for object o,
	 to the value, v. Returns -1 on failure.  This is
	 the equivalent of the Python statement: o.attr_name=v.

       */

     /* implemented as a macro:

     int PyObject_DelAttrString(PyObject *o, char *attr_name);

	 Delete attribute named attr_name, for object o. Returns
	 -1 on failure.  This is the equivalent of the Python
	 statement: del o.attr_name.

       */
#define  PyObject_DelAttrString(O,A) PyObject_SetAttrString((O),(A),NULL)

     /* implemented as a macro:

     int PyObject_DelAttr(PyObject *o, PyObject *attr_name);

	 Delete attribute named attr_name, for object o. Returns -1
	 on failure.  This is the equivalent of the Python
	 statement: del o.attr_name.

       */
#define  PyObject_DelAttr(O,A) PyObject_SetAttr((O),(A),NULL)

     DL_IMPORT(int) PyObject_Cmp(PyObject *o1, PyObject *o2, int *result);

       /*
	 Compare the values of o1 and o2 using a routine provided by
	 o1, if one exists, otherwise with a routine provided by o2.
	 The result of the comparison is returned in result.  Returns
	 -1 on failure.  This is the equivalent of the Python
	 statement: result=cmp(o1,o2).

       */

     /* Implemented elsewhere:

     int PyObject_Compare(PyObject *o1, PyObject *o2);

	 Compare the values of o1 and o2 using a routine provided by
	 o1, if one exists, otherwise with a routine provided by o2.
	 Returns the result of the comparison on success.  On error,
	 the value returned is undefined. This is equivalent to the
	 Python expression: cmp(o1,o2).

       */

     /* Implemented elsewhere:

     PyObject *PyObject_Repr(PyObject *o);

	 Compute the string representation of object, o.  Returns the
	 string representation on success, NULL on failure.  This is
	 the equivalent of the Python expression: repr(o).

	 Called by the repr() built-in function and by reverse quotes.

       */

     /* Implemented elsewhere:

     PyObject *PyObject_Str(PyObject *o);

	 Compute the string representation of object, o.  Returns the
	 string representation on success, NULL on failure.  This is
	 the equivalent of the Python expression: str(o).)

	 Called by the str() built-in function and by the print
	 statement.

       */

     DL_IMPORT(int) PyCallable_Check(PyObject *o);

       /*
	 Determine if the object, o, is callable.  Return 1 if the
	 object is callable and 0 otherwise.

	 This function always succeeds.

       */


     
     DL_IMPORT(PyObject *) PyObject_CallObject(PyObject *callable_object,
                                               PyObject *args);

       /*

	 Call a callable Python object, callable_object, with
	 arguments given by the tuple, args.  If no arguments are
	 needed, then args may be NULL.  Returns the result of the
	 call on success, or NULL on failure.  This is the equivalent
	 of the Python expression: apply(o,args).

       */

     DL_IMPORT(PyObject *) PyObject_CallFunction(PyObject *callable_object,
                                                 char *format, ...);

       /*
	 Call a callable Python object, callable_object, with a
	 variable number of C arguments. The C arguments are described
	 using a mkvalue-style format string. The format may be NULL,
	 indicating that no arguments are provided.  Returns the
	 result of the call on success, or NULL on failure.  This is
	 the equivalent of the Python expression: apply(o,args).

       */


     DL_IMPORT(PyObject *) PyObject_CallMethod(PyObject *o, char *m,
                                               char *format, ...);

       /*
	 Call the method named m of object o with a variable number of
	 C arguments.  The C arguments are described by a mkvalue
	 format string.  The format may be NULL, indicating that no
	 arguments are provided. Returns the result of the call on
	 success, or NULL on failure.  This is the equivalent of the
	 Python expression: o.method(args).

         Note that Special method names, such as "__add__",
	 "__getitem__", and so on are not supported. The specific
	 abstract-object routines for these must be used.

       */


     /* Implemented elsewhere:

     long PyObject_Hash(PyObject *o);

         Compute and return the hash, hash_value, of an object, o.  On
	 failure, return -1.  This is the equivalent of the Python
	 expression: hash(o).

       */


     /* Implemented elsewhere:

     int PyObject_IsTrue(PyObject *o);

	 Returns 1 if the object, o, is considered to be true, and
	 0 otherwise. This is equivalent to the Python expression:
	 not not o

	 This function always succeeds.
	 
       */

     /* Implemented elsewhere:

     int PyObject_Not(PyObject *o);

	 Returns 0 if the object, o, is considered to be true, and
	 1 otherwise. This is equivalent to the Python expression:
	 not o

	 This function always succeeds.
	 
       */

     DL_IMPORT(PyObject *) PyObject_Type(PyObject *o);

       /*
	 On success, returns a type object corresponding to the object
	 type of object o. On failure, returns NULL.  This is
	 equivalent to the Python expression: type(o).
       */

     DL_IMPORT(int) PyObject_Size(PyObject *o);

       /*
         Return the size of object o.  If the object, o, provides
	 both sequence and mapping protocols, the sequence size is
	 returned. On error, -1 is returned.  This is the equivalent
	 to the Python expression: len(o).

       */

       /* For DLL compatibility */
#undef PyObject_Length
     DL_IMPORT(int) PyObject_Length(PyObject *o);
#define PyObject_Length PyObject_Size


     DL_IMPORT(PyObject *) PyObject_GetItem(PyObject *o, PyObject *key);

       /*
	 Return element of o corresponding to the object, key, or NULL
	 on failure. This is the equivalent of the Python expression:
	 o[key].

       */

     DL_IMPORT(int) PyObject_SetItem(PyObject *o, PyObject *key, PyObject *v);

       /*
	 Map the object, key, to the value, v.  Returns
	 -1 on failure.  This is the equivalent of the Python
	 statement: o[key]=v.
       */

     DL_IMPORT(int) PyObject_DelItem(PyObject *o, PyObject *key);

       /*
	 Delete the mapping for key from *o.  Returns -1 on failure.
	 This is the equivalent of the Python statement: del o[key].
       */

     DL_IMPORT(int) PyObject_AsCharBuffer(PyObject *obj,
					  const char **buffer,
					  int *buffer_len);

       /* 
	  Takes an arbitrary object which must support the (character,
	  single segment) buffer interface and returns a pointer to a
	  read-only memory location useable as character based input
	  for subsequent processing.

	  0 is returned on success.  buffer and buffer_len are only
	  set in case no error occurrs. Otherwise, -1 is returned and
	  an exception set.

       */

     DL_IMPORT(int) PyObject_AsReadBuffer(PyObject *obj,
					  const void **buffer,
					  int *buffer_len);

       /* 
	  Same as PyObject_AsCharBuffer() except that this API expects
	  (readable, single segment) buffer interface and returns a
	  pointer to a read-only memory location which can contain
	  arbitrary data.

	  0 is returned on success.  buffer and buffer_len are only
	  set in case no error occurrs.  Otherwise, -1 is returned and
	  an exception set.

       */

     DL_IMPORT(int) PyObject_AsWriteBuffer(PyObject *obj,
					   void **buffer,
					   int *buffer_len);

       /* 
	  Takes an arbitrary object which must support the (writeable,
	  single segment) buffer interface and returns a pointer to a
	  writeable memory location in buffer of size buffer_len.

	  0 is returned on success.  buffer and buffer_len are only
	  set in case no error occurrs. Otherwise, -1 is returned and
	  an exception set.

       */

/*  Number Protocol:*/

     DL_IMPORT(int) PyNumber_Check(PyObject *o);

       /*
         Returns 1 if the object, o, provides numeric protocols, and
	 false otherwise. 

	 This function always succeeds.

       */

     DL_IMPORT(PyObject *) PyNumber_Add(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of adding o1 and o2, or null on failure.
	 This is the equivalent of the Python expression: o1+o2.


       */

     DL_IMPORT(PyObject *) PyNumber_Subtract(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of subtracting o2 from o1, or null on
	 failure.  This is the equivalent of the Python expression:
	 o1-o2.

       */

     DL_IMPORT(PyObject *) PyNumber_Multiply(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of multiplying o1 and o2, or null on
	 failure.  This is the equivalent of the Python expression:
	 o1*o2.


       */

     DL_IMPORT(PyObject *) PyNumber_Divide(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of dividing o1 by o2, or null on failure.
	 This is the equivalent of the Python expression: o1/o2.


       */

     DL_IMPORT(PyObject *) PyNumber_Remainder(PyObject *o1, PyObject *o2);

       /*
	 Returns the remainder of dividing o1 by o2, or null on
	 failure.  This is the equivalent of the Python expression:
	 o1%o2.


       */

     DL_IMPORT(PyObject *) PyNumber_Divmod(PyObject *o1, PyObject *o2);

       /*
	 See the built-in function divmod.  Returns NULL on failure.
	 This is the equivalent of the Python expression:
	 divmod(o1,o2).


       */

     DL_IMPORT(PyObject *) PyNumber_Power(PyObject *o1, PyObject *o2,
                                          PyObject *o3);

       /*
	 See the built-in function pow.  Returns NULL on failure.
	 This is the equivalent of the Python expression:
	 pow(o1,o2,o3), where o3 is optional.

       */

     DL_IMPORT(PyObject *) PyNumber_Negative(PyObject *o);

       /*
	 Returns the negation of o on success, or null on failure.
	 This is the equivalent of the Python expression: -o.

       */

     DL_IMPORT(PyObject *) PyNumber_Positive(PyObject *o);

       /*
         Returns the (what?) of o on success, or NULL on failure.
	 This is the equivalent of the Python expression: +o.

       */

     DL_IMPORT(PyObject *) PyNumber_Absolute(PyObject *o);

       /*
	 Returns the absolute value of o, or null on failure.  This is
	 the equivalent of the Python expression: abs(o).

       */

     DL_IMPORT(PyObject *) PyNumber_Invert(PyObject *o);

       /*
	 Returns the bitwise negation of o on success, or NULL on
	 failure.  This is the equivalent of the Python expression:
	 ~o.


       */

     DL_IMPORT(PyObject *) PyNumber_Lshift(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of left shifting o1 by o2 on success, or
	 NULL on failure.  This is the equivalent of the Python
	 expression: o1 << o2.


       */

     DL_IMPORT(PyObject *) PyNumber_Rshift(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of right shifting o1 by o2 on success, or
	 NULL on failure.  This is the equivalent of the Python
	 expression: o1 >> o2.

       */

     DL_IMPORT(PyObject *) PyNumber_And(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of bitwise and of o1 and o2 on success, or
	 NULL on failure. This is the equivalent of the Python
	 expression: o1&o2.


       */

     DL_IMPORT(PyObject *) PyNumber_Xor(PyObject *o1, PyObject *o2);

       /*
	 Returns the bitwise exclusive or of o1 by o2 on success, or
	 NULL on failure.  This is the equivalent of the Python
	 expression: o1^o2.


       */

     DL_IMPORT(PyObject *) PyNumber_Or(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of bitwise or or o1 and o2 on success, or
	 NULL on failure.  This is the equivalent of the Python
	 expression: o1|o2.

       */

     /* Implemented elsewhere:

     int PyNumber_Coerce(PyObject **p1, PyObject **p2);

	 This function takes the addresses of two variables of type
	 PyObject*.

	 If the objects pointed to by *p1 and *p2 have the same type,
	 increment their reference count and return 0 (success).
	 If the objects can be converted to a common numeric type,
	 replace *p1 and *p2 by their converted value (with 'new'
	 reference counts), and return 0.
	 If no conversion is possible, or if some other error occurs,
	 return -1 (failure) and don't increment the reference counts.
	 The call PyNumber_Coerce(&o1, &o2) is equivalent to the Python
	 statement o1, o2 = coerce(o1, o2).

       */

     DL_IMPORT(PyObject *) PyNumber_Int(PyObject *o);

       /*
	 Returns the o converted to an integer object on success, or
	 NULL on failure.  This is the equivalent of the Python
	 expression: int(o).

       */

     DL_IMPORT(PyObject *) PyNumber_Long(PyObject *o);

       /*
	 Returns the o converted to a long integer object on success,
	 or NULL on failure.  This is the equivalent of the Python
	 expression: long(o).

       */

     DL_IMPORT(PyObject *) PyNumber_Float(PyObject *o);

       /*
	 Returns the o converted to a float object on success, or NULL
	 on failure.  This is the equivalent of the Python expression:
	 float(o).
       */
	 
/*  In-place variants of (some of) the above number protocol functions */

     DL_IMPORT(PyObject *) PyNumber_InPlaceAdd(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of adding o2 to o1, possibly in-place, or null
	 on failure.  This is the equivalent of the Python expression:
	 o1 += o2.

       */

     DL_IMPORT(PyObject *) PyNumber_InPlaceSubtract(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of subtracting o2 from o1, possibly in-place or
	 null on failure.  This is the equivalent of the Python expression:
	 o1 -= o2.

       */

     DL_IMPORT(PyObject *) PyNumber_InPlaceMultiply(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of multiplying o1 by o2, possibly in-place, or
	 null on failure.  This is the equivalent of the Python expression:
	 o1 *= o2.

       */

     DL_IMPORT(PyObject *) PyNumber_InPlaceDivide(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of dividing o1 by o2, possibly in-place, or null
	 on failure.  This is the equivalent of the Python expression:
	 o1 /= o2.

       */

     DL_IMPORT(PyObject *) PyNumber_InPlaceRemainder(PyObject *o1, PyObject *o2);

       /*
	 Returns the remainder of dividing o1 by o2, possibly in-place, or
	 null on failure.  This is the equivalent of the Python expression:
	 o1 %= o2.

       */

     DL_IMPORT(PyObject *) PyNumber_InPlacePower(PyObject *o1, PyObject *o2,
     						 PyObject *o3);

       /*
	 Returns the result of raising o1 to the power of o2, possibly
	 in-place, or null on failure.  This is the equivalent of the Python
	 expression: o1 **= o2, or pow(o1, o2, o3) if o3 is present.

       */

     DL_IMPORT(PyObject *) PyNumber_InPlaceLshift(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of left shifting o1 by o2, possibly in-place, or
	 null on failure.  This is the equivalent of the Python expression:
	 o1 <<= o2.

       */

     DL_IMPORT(PyObject *) PyNumber_InPlaceRshift(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of right shifting o1 by o2, possibly in-place or
	 null on failure.  This is the equivalent of the Python expression:
	 o1 >>= o2.

       */

     DL_IMPORT(PyObject *) PyNumber_InPlaceAnd(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of bitwise and of o1 and o2, possibly in-place,
	 or null on failure. This is the equivalent of the Python
	 expression: o1 &= o2.

       */

     DL_IMPORT(PyObject *) PyNumber_InPlaceXor(PyObject *o1, PyObject *o2);

       /*
	 Returns the bitwise exclusive or of o1 by o2, possibly in-place, or
	 null on failure.  This is the equivalent of the Python expression:
	 o1 ^= o2.

       */

     DL_IMPORT(PyObject *) PyNumber_InPlaceOr(PyObject *o1, PyObject *o2);

       /*
	 Returns the result of bitwise or or o1 and o2, possibly in-place,
	 or null on failure.  This is the equivalent of the Python
	 expression: o1 |= o2.

       */


/*  Sequence protocol:*/

     DL_IMPORT(int) PySequence_Check(PyObject *o);

       /*
         Return 1 if the object provides sequence protocol, and zero
	 otherwise.  

	 This function always succeeds.

       */

     DL_IMPORT(int) PySequence_Size(PyObject *o);

       /*
         Return the size of sequence object o, or -1 on failure.

       */

       /* For DLL compatibility */
#undef PySequence_Length
     DL_IMPORT(int) PySequence_Length(PyObject *o);
#define PySequence_Length PySequence_Size


     DL_IMPORT(PyObject *) PySequence_Concat(PyObject *o1, PyObject *o2);

       /*
	 Return the concatenation of o1 and o2 on success, and NULL on
	 failure.   This is the equivalent of the Python
	 expression: o1+o2.

       */

     DL_IMPORT(PyObject *) PySequence_Repeat(PyObject *o, int count);

       /*
	 Return the result of repeating sequence object o count times,
	 or NULL on failure.  This is the equivalent of the Python
	 expression: o1*count.

       */

     DL_IMPORT(PyObject *) PySequence_GetItem(PyObject *o, int i);

       /*
	 Return the ith element of o, or NULL on failure. This is the
	 equivalent of the Python expression: o[i].
       */

     DL_IMPORT(PyObject *) PySequence_GetSlice(PyObject *o, int i1, int i2);

       /*
	 Return the slice of sequence object o between i1 and i2, or
	 NULL on failure. This is the equivalent of the Python
	 expression: o[i1:i2].

       */

     DL_IMPORT(int) PySequence_SetItem(PyObject *o, int i, PyObject *v);

       /*
	 Assign object v to the ith element of o.  Returns
	 -1 on failure.  This is the equivalent of the Python
	 statement: o[i]=v.

       */

     DL_IMPORT(int) PySequence_DelItem(PyObject *o, int i);

       /*
	 Delete the ith element of object v.  Returns
	 -1 on failure.  This is the equivalent of the Python
	 statement: del o[i].
       */

     DL_IMPORT(int) PySequence_SetSlice(PyObject *o, int i1, int i2,
                                        PyObject *v);

       /*
         Assign the sequence object, v, to the slice in sequence
	 object, o, from i1 to i2.  Returns -1 on failure. This is the
	 equivalent of the Python statement: o[i1:i2]=v.
       */

     DL_IMPORT(int) PySequence_DelSlice(PyObject *o, int i1, int i2);

       /*
	 Delete the slice in sequence object, o, from i1 to i2.
	 Returns -1 on failure. This is the equivalent of the Python
	 statement: del o[i1:i2].
       */

     DL_IMPORT(PyObject *) PySequence_Tuple(PyObject *o);

       /*
	 Returns the sequence, o, as a tuple on success, and NULL on failure.
	 This is equivalent to the Python expression: tuple(o)
       */


     DL_IMPORT(PyObject *) PySequence_List(PyObject *o);

       /*
	 Returns the sequence, o, as a list on success, and NULL on failure.
	 This is equivalent to the Python expression: list(o)
       */

     DL_IMPORT(PyObject *) PySequence_Fast(PyObject *o, const char* m);

       /*
         Returns the sequence, o, as a tuple, unless it's already a
         tuple or list.  Use PySequence_Fast_GET_ITEM to access the
         members of this list.

         Returns NULL on failure.  If the object is not a sequence,
         raises a TypeError exception with m as the message text.
       */

#define PySequence_Fast_GET_ITEM(o, i)\
     (PyList_Check(o) ? PyList_GET_ITEM(o, i) : PyTuple_GET_ITEM(o, i))

       /*
	 Return the ith element of o, assuming that o was returned by
         PySequence_Fast, and that i is within bounds.
       */

     DL_IMPORT(int) PySequence_Count(PyObject *o, PyObject *value);

       /*
         Return the number of occurrences on value on o, that is,
	 return the number of keys for which o[key]==value.  On
	 failure, return -1.  This is equivalent to the Python
	 expression: o.count(value).
       */

     DL_IMPORT(int) PySequence_Contains(PyObject *o, PyObject *value);

/* For DLL-level backwards compatibility */
#undef PySequence_In
     DL_IMPORT(int) PySequence_In(PyObject *o, PyObject *value);

/* For source-level backwards compatibility */
#define PySequence_In PySequence_Contains

       /*
	 Determine if o contains value.  If an item in o is equal to
	 X, return 1, otherwise return 0.  On error, return -1.  This
	 is equivalent to the Python expression: value in o.
       */

     DL_IMPORT(int) PySequence_Index(PyObject *o, PyObject *value);

       /*
	 Return the first index for which o[i]=value.  On error,
	 return -1.    This is equivalent to the Python
	 expression: o.index(value).
       */

/* In-place versions of some of the above Sequence functions. */

     DL_IMPORT(PyObject *) PySequence_InPlaceConcat(PyObject *o1, PyObject *o2);

       /*
	 Append o2 to o1, in-place when possible. Return the resulting
	 object, which could be o1, or NULL on failure.  This is the
	 equivalent of the Python expression: o1 += o2.

       */

     DL_IMPORT(PyObject *) PySequence_InPlaceRepeat(PyObject *o, int count);

       /*
	 Repeat o1 by count, in-place when possible. Return the resulting
	 object, which could be o1, or NULL on failure.  This is the
	 equivalent of the Python expression: o1 *= count.

       */

/*  Mapping protocol:*/

     DL_IMPORT(int) PyMapping_Check(PyObject *o);

       /*
         Return 1 if the object provides mapping protocol, and zero
	 otherwise.  

	 This function always succeeds.
       */

     DL_IMPORT(int) PyMapping_Size(PyObject *o);

       /*
         Returns the number of keys in object o on success, and -1 on
	 failure.  For objects that do not provide sequence protocol,
	 this is equivalent to the Python expression: len(o).
       */

       /* For DLL compatibility */
#undef PyMapping_Length
     DL_IMPORT(int) PyMapping_Length(PyObject *o);
#define PyMapping_Length PyMapping_Size


     /* implemented as a macro:

     int PyMapping_DelItemString(PyObject *o, char *key);

	 Remove the mapping for object, key, from the object *o.
	 Returns -1 on failure.  This is equivalent to
	 the Python statement: del o[key].
       */
#define PyMapping_DelItemString(O,K) PyDict_DelItemString((O),(K))

     /* implemented as a macro:

     int PyMapping_DelItem(PyObject *o, PyObject *key);

	 Remove the mapping for object, key, from the object *o.
	 Returns -1 on failure.  This is equivalent to
	 the Python statement: del o[key].
       */
#define PyMapping_DelItem(O,K) PyDict_DelItem((O),(K))

     DL_IMPORT(int) PyMapping_HasKeyString(PyObject *o, char *key);

       /*
	 On success, return 1 if the mapping object has the key, key,
	 and 0 otherwise.  This is equivalent to the Python expression:
	 o.has_key(key). 

	 This function always succeeds.
       */

     DL_IMPORT(int) PyMapping_HasKey(PyObject *o, PyObject *key);

       /*
	 Return 1 if the mapping object has the key, key,
	 and 0 otherwise.  This is equivalent to the Python expression:
	 o.has_key(key). 

	 This function always succeeds.

       */

     /* Implemented as macro:

     PyObject *PyMapping_Keys(PyObject *o);

         On success, return a list of the keys in object o.  On
	 failure, return NULL. This is equivalent to the Python
	 expression: o.keys().
       */
#define PyMapping_Keys(O) PyObject_CallMethod(O,"keys",NULL)

     /* Implemented as macro:

     PyObject *PyMapping_Values(PyObject *o);

         On success, return a list of the values in object o.  On
	 failure, return NULL. This is equivalent to the Python
	 expression: o.values().
       */
#define PyMapping_Values(O) PyObject_CallMethod(O,"values",NULL)

     /* Implemented as macro:

     PyObject *PyMapping_Items(PyObject *o);

         On success, return a list of the items in object o, where
	 each item is a tuple containing a key-value pair.  On
	 failure, return NULL. This is equivalent to the Python
	 expression: o.items().

       */
#define PyMapping_Items(O) PyObject_CallMethod(O,"items",NULL)

     DL_IMPORT(PyObject *) PyMapping_GetItemString(PyObject *o, char *key);

       /*
	 Return element of o corresponding to the object, key, or NULL
	 on failure. This is the equivalent of the Python expression:
	 o[key].
       */

     DL_IMPORT(int) PyMapping_SetItemString(PyObject *o, char *key,
                                            PyObject *value);

       /*
	 Map the object, key, to the value, v.  Returns 
	 -1 on failure.  This is the equivalent of the Python
	 statement: o[key]=v.
      */


#ifdef __cplusplus
}
#endif
#endif /* Py_ABSTRACTOBJECT_H */
