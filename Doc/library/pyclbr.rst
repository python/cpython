
:mod:`pyclbr` --- Python class browser support
==============================================

.. module:: pyclbr
   :synopsis: Supports information extraction for a Python class browser.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


The :mod:`pyclbr` can be used to determine some limited information about the
classes, methods and top-level functions defined in a module.  The information
provided is sufficient to implement a traditional three-pane class browser.  The
information is extracted from the source code rather than by importing the
module, so this module is safe to use with untrusted source code.  This
restriction makes it impossible to use this module with modules not implemented
in Python, including many standard and optional extension modules.


.. function:: readmodule(module[, path])

   Read a module and return a dictionary mapping class names to class descriptor
   objects.  The parameter *module* should be the name of a module as a string; it
   may be the name of a module within a package.  The *path* parameter should be a
   sequence, and is used to augment the value of ``sys.path``, which is used to
   locate module source code.

   .. % The 'inpackage' parameter appears to be for internal use only....


.. function:: readmodule_ex(module[, path])

   Like :func:`readmodule`, but the returned dictionary, in addition to mapping
   class names to class descriptor objects, also maps top-level function names to
   function descriptor objects.  Moreover, if the module being read is a package,
   the key ``'__path__'`` in the returned dictionary has as its value a list which
   contains the package search path.

   .. % The 'inpackage' parameter appears to be for internal use only....


.. _pyclbr-class-objects:

Class Descriptor Objects
------------------------

The class descriptor objects used as values in the dictionary returned by
:func:`readmodule` and :func:`readmodule_ex` provide the following data members:


.. attribute:: class_descriptor.module

   The name of the module defining the class described by the class descriptor.


.. attribute:: class_descriptor.name

   The name of the class.


.. attribute:: class_descriptor.super

   A list of class descriptors which describe the immediate base classes of the
   class being described.  Classes which are named as superclasses but which are
   not discoverable by :func:`readmodule` are listed as a string with the class
   name instead of class descriptors.


.. attribute:: class_descriptor.methods

   A dictionary mapping method names to line numbers.


.. attribute:: class_descriptor.file

   Name of the file containing the ``class`` statement defining the class.


.. attribute:: class_descriptor.lineno

   The line number of the ``class`` statement within the file named by
   :attr:`file`.


.. _pyclbr-function-objects:

Function Descriptor Objects
---------------------------

The function descriptor objects used as values in the dictionary returned by
:func:`readmodule_ex` provide the following data members:


.. attribute:: function_descriptor.module

   The name of the module defining the function described by the function
   descriptor.


.. attribute:: function_descriptor.name

   The name of the function.


.. attribute:: function_descriptor.file

   Name of the file containing the ``def`` statement defining the function.


.. attribute:: function_descriptor.lineno

   The line number of the ``def`` statement within the file named by :attr:`file`.

