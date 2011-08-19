:mod:`pyclbr` --- Python class browser support
==============================================

.. module:: pyclbr
   :synopsis: Supports information extraction for a Python class browser.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

**Source code:** :source:`Lib/pyclbr.py`

--------------

The :mod:`pyclbr` module can be used to determine some limited information
about the classes, methods and top-level functions defined in a module.  The
information provided is sufficient to implement a traditional three-pane
class browser.  The information is extracted from the source code rather
than by importing the module, so this module is safe to use with untrusted
code.  This restriction makes it impossible to use this module with modules
not implemented in Python, including all standard and optional extension
modules.


.. function:: readmodule(module[, path=None])

   Read a module and return a dictionary mapping class names to class
   descriptor objects.  The parameter *module* should be the name of a
   module as a string; it may be the name of a module within a package.  The
   *path* parameter should be a sequence, and is used to augment the value
   of ``sys.path``, which is used to locate module source code.


.. function:: readmodule_ex(module[, path=None])

   Like :func:`readmodule`, but the returned dictionary, in addition to
   mapping class names to class descriptor objects, also maps top-level
   function names to function descriptor objects.  Moreover, if the module
   being read is a package, the key ``'__path__'`` in the returned
   dictionary has as its value a list which contains the package search
   path.


.. _pyclbr-class-objects:

Class Objects
-------------

The :class:`Class` objects used as values in the dictionary returned by
:func:`readmodule` and :func:`readmodule_ex` provide the following data
attributes:


.. attribute:: Class.module

   The name of the module defining the class described by the class descriptor.


.. attribute:: Class.name

   The name of the class.


.. attribute:: Class.super

   A list of :class:`Class` objects which describe the immediate base
   classes of the class being described.  Classes which are named as
   superclasses but which are not discoverable by :func:`readmodule` are
   listed as a string with the class name instead of as :class:`Class`
   objects.


.. attribute:: Class.methods

   A dictionary mapping method names to line numbers.


.. attribute:: Class.file

   Name of the file containing the ``class`` statement defining the class.


.. attribute:: Class.lineno

   The line number of the ``class`` statement within the file named by
   :attr:`~Class.file`.


.. _pyclbr-function-objects:

Function Objects
----------------

The :class:`Function` objects used as values in the dictionary returned by
:func:`readmodule_ex` provide the following attributes:


.. attribute:: Function.module

   The name of the module defining the function described by the function
   descriptor.


.. attribute:: Function.name

   The name of the function.


.. attribute:: Function.file

   Name of the file containing the ``def`` statement defining the function.


.. attribute:: Function.lineno

   The line number of the ``def`` statement within the file named by
   :attr:`~Function.file`.

