
:mod:`dl` --- Call C functions in shared objects
================================================

.. module:: dl
   :platform: Unix
   :synopsis: Call C functions in shared objects.
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


.. % ?????????? Anyone????????????

The :mod:`dl` module defines an interface to the :cfunc:`dlopen` function, which
is the most common interface on Unix platforms for handling dynamically linked
libraries. It allows the program to call arbitrary functions in such a library.

.. warning::

   The :mod:`dl` module bypasses the Python type system and  error handling. If
   used incorrectly it may cause segmentation faults, crashes or other incorrect
   behaviour.

.. note::

   This module will not work unless ``sizeof(int) == sizeof(long) == sizeof(char
   *)`` If this is not the case, :exc:`SystemError` will be raised on import.

The :mod:`dl` module defines the following function:


.. function:: open(name[, mode=RTLD_LAZY])

   Open a shared object file, and return a handle. Mode signifies late binding
   (:const:`RTLD_LAZY`) or immediate binding (:const:`RTLD_NOW`). Default is
   :const:`RTLD_LAZY`. Note that some systems do not support :const:`RTLD_NOW`.

   Return value is a :class:`dlobject`.

The :mod:`dl` module defines the following constants:


.. data:: RTLD_LAZY

   Useful as an argument to :func:`open`.


.. data:: RTLD_NOW

   Useful as an argument to :func:`open`.  Note that on systems which do not
   support immediate binding, this constant will not appear in the module. For
   maximum portability, use :func:`hasattr` to determine if the system supports
   immediate binding.

The :mod:`dl` module defines the following exception:


.. exception:: error

   Exception raised when an error has occurred inside the dynamic loading and
   linking routines.

Example::

   >>> import dl, time
   >>> a=dl.open('/lib/libc.so.6')
   >>> a.call('time'), time.time()
   (929723914, 929723914.498)

This example was tried on a Debian GNU/Linux system, and is a good example of
the fact that using this module is usually a bad alternative.


.. _dl-objects:

Dl Objects
----------

Dl objects, as returned by :func:`open` above, have the following methods:


.. method:: dl.close()

   Free all resources, except the memory.


.. method:: dl.sym(name)

   Return the pointer for the function named *name*, as a number, if it exists in
   the referenced shared object, otherwise ``None``. This is useful in code like::

      >>> if a.sym('time'): 
      ...     a.call('time')
      ... else: 
      ...     time.time()

   (Note that this function will return a non-zero number, as zero is the *NULL*
   pointer)


.. method:: dl.call(name[, arg1[, arg2...]])

   Call the function named *name* in the referenced shared object. The arguments
   must be either Python integers, which will be  passed as is, Python strings, to
   which a pointer will be passed,  or ``None``, which will be passed as *NULL*.
   Note that  strings should only be passed to functions as :ctype:`const char\*`,
   as Python will not like its string mutated.

   There must be at most 10 arguments, and arguments not given will be treated as
   ``None``. The function's return value must be a C :ctype:`long`, which is a
   Python integer.

