:mod:`new` --- Creation of runtime internal objects
===================================================

.. module:: new
   :synopsis: Interface to the creation of runtime implementation objects.
   :deprecated:

.. deprecated:: 2.6
   The :mod:`new` module has been removed in Python 3.0.  Use the :mod:`types`
   module's classes instead.

.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


The :mod:`new` module allows an interface to the interpreter object creation
functions. This is for use primarily in marshal-type functions, when a new
object needs to be created "magically" and not by using the regular creation
functions. This module provides a low-level interface to the interpreter, so
care must be exercised when using this module. It is possible to supply
non-sensical arguments which crash the interpreter when the object is used.

The :mod:`new` module defines the following functions:


.. function:: instance(class[, dict])

   This function creates an instance of *class* with dictionary *dict* without
   calling the :meth:`__init__` constructor.  If *dict* is omitted or ``None``, a
   new, empty dictionary is created for the new instance.  Note that there are no
   guarantees that the object will be in a consistent state.


.. function:: instancemethod(function, instance, class)

   This function will return a method object, bound to *instance*, or unbound if
   *instance* is ``None``.  *function* must be callable.


.. function:: function(code, globals[, name[, argdefs[, closure]]])

   Returns a (Python) function with the given code and globals. If *name* is given,
   it must be a string or ``None``.  If it is a string, the function will have the
   given name, otherwise the function name will be taken from ``code.co_name``.  If
   *argdefs* is given, it must be a tuple and will be used to determine the default
   values of parameters.  If *closure* is given, it must be ``None`` or a tuple of
   cell objects containing objects to bind to the names in ``code.co_freevars``.


.. function:: code(argcount, nlocals, stacksize, flags, codestring, constants, names, varnames, filename, name, firstlineno, lnotab)

   This function is an interface to the :c:func:`PyCode_New` C function.

   .. XXX This is still undocumented!


.. function:: module(name[, doc])

   This function returns a new module object with name *name*. *name* must be a
   string. The optional *doc* argument can have any type.


.. function:: classobj(name, baseclasses, dict)

   This function returns a new class object, with name *name*, derived from
   *baseclasses* (which should be a tuple of classes) and with namespace *dict*.

