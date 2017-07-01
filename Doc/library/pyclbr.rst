:mod:`pyclbr` --- Python class browser support
==============================================

.. module:: pyclbr
   :synopsis: Supports information extraction for a Python class browser.

.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

**Source code:** :source:`Lib/pyclbr.py`

--------------

The :mod:`pyclbr` module can be used to determine some limited information
about the classes, methods, and functions defined in a module.  The
information provided is sufficient to implement a module browser.  The
information is extracted from the source code rather than by importing the
module, so this module is safe to use with untrusted code.  This restriction
makes it impossible to use this module with modules not implemented in Python,
including all standard and optional extension modules.


.. function:: readmodule(module, path=None)

   Read a module and return a dictionary mapping class names to class
   descriptor objects.  The parameter *module* should be the name of a
   module as a string; it may be the name of a module within a package.  The
   *path* parameter should be a sequence, and is used to augment the value
   of ``sys.path``, which is used to locate module source code.


.. function:: readmodule_ex(module, path=None)

   Like :func:`readmodule`, but the returned dictionary, in addition to
   mapping class names to class descriptor objects, also maps function
   names to function descriptor objects.  Moreover, if the module
   being read is a package, the key ``'__path__'`` in the returned
   dictionary has as its value a list which contains the package search
   path.


.. _pyclbr-object-objects:

Object Objects
--------------
The class :class:`Object` is the base class for the classes
:class:`Class` and :class:`Function`. It provides the following
data members:


.. attribute:: Object.module

   The name of the module defining the object described.


.. attribute:: Object.name

   The name of the object.


.. attribute:: Object.file

   Name of the file in which the object was defined.


.. attribute:: Object.lineno

   The line number in the file named by :attr:`~Object.file` where
   the definition of the object started.


.. attribute:: Object.parent

   The parent of this object, if any.

   .. versionadded:: 3.7


.. attribute:: Object.children

   A dictionary mapping object names to the objects that are defined inside the
   namespace created by the current object.

   .. versionadded:: 3.7


.. versionchanged:: 3.7
   :class:`Object` was added as a base class for :class:`Class` and
   :class:`Function` and, except as otherwise noted, the attributes
   were previously common to those two classes.



.. _pyclbr-class-objects:

Class Objects
-------------

:class:`Class` is a subclass of :class:`Object` whose objects are used as values
in the dictionary returned by :func:`readmodule` and :func:`readmodule_ex`.
In addition to the attributes from :class:`Object`, :class:`Class` objects
also provide the following attributes:


.. attribute:: Class.super

   A list of :class:`Class` objects which describe the immediate base
   classes of the class being described.  Classes which are named as
   superclasses but which are not discoverable by :func:`readmodule` are
   listed as a string with the class name instead of as :class:`Class`
   objects.


.. attribute:: Class.methods

   A dictionary mapping method names to line numbers.


.. versionchanged:: 3.7
   :class:`Class` became a subclass of :class:`Object`.


.. _pyclbr-function-objects:

Function Objects
----------------

:class:`Function` is a subclass of :class:`Object` whose objects are used as
values in the dictionary returned by :func:`readmodule_ex`.  The only instance
attributes are those from :class:`Object`.

.. versionchanged:: 3.7
   :class:`Function` became a subclass of :class:`Object`.
