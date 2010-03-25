.. highlightlang:: c


.. _concrete:

**********************
Concrete Objects Layer
**********************

The functions in this chapter are specific to certain Python object types.
Passing them an object of the wrong type is not a good idea; if you receive an
object from a Python program and you are not sure that it has the right type,
you must perform a type check first; for example, to check that an object is a
dictionary, use :cfunc:`PyDict_Check`.  The chapter is structured like the
"family tree" of Python object types.

.. warning::

   While the functions described in this chapter carefully check the type of the
   objects which are passed in, many of them do not check for *NULL* being passed
   instead of a valid object.  Allowing *NULL* to be passed in can cause memory
   access violations and immediate termination of the interpreter.


.. _fundamental:

Fundamental Objects
===================

This section describes Python type objects and the singleton object ``None``.

.. toctree::

   type.rst
   none.rst


.. _numericobjects:

Numeric Objects
===============

.. index:: object: numeric

.. toctree::

   int.rst
   bool.rst
   long.rst
   float.rst
   complex.rst


.. _sequenceobjects:

Sequence Objects
================

.. index:: object: sequence

Generic operations on sequence objects were discussed in the previous chapter;
this section deals with the specific kinds of sequence objects that are
intrinsic to the Python language.

.. toctree::

   bytearray.rst
   string.rst
   unicode.rst
   buffer.rst
   tuple.rst
   list.rst


.. _mapobjects:

Mapping Objects
===============

.. index:: object: mapping

.. toctree::

   dict.rst


.. _otherobjects:

Other Objects
=============

.. toctree::

   class.rst
   function.rst
   method.rst
   file.rst
   module.rst
   iterator.rst
   descriptor.rst
   slice.rst
   weakref.rst
   capsule.rst
   cobject.rst
   cell.rst
   gen.rst
   datetime.rst
   set.rst
   code.rst
