:mod:`pyclbr` --- Python class browser support
==============================================

.. module:: pyclbr
   :synopsis: Supports information extraction for a Python class browser.

.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

**Source code:** :source:`Lib/pyclbr.py`

--------------

The :mod:`pyclbr` module provides limited information about the
functions, classes, and methods defined in a Python-coded module.  The
information is sufficient to implement a module browser.  The
information is extracted from the Python source code rather than by
importing the module, so this module is safe to use with untrusted code.
This restriction makes it impossible to use this module with modules not
implemented in Python, including all standard and optional extension
modules.


.. function:: readmodule(module, path=None)

   Return a dictionary mapping module-level class names to class
   descriptors.  If possible, descriptors for imported base classes are
   included.  Parameter *module* is a string with the name of the module
   to read; it may be the name of a module within a package.  If given,
   *path* is a sequence of directory paths prepended to ``sys.path``,
   which is used to locate the module source code.


.. function:: readmodule_ex(module, path=None)

   Return a dictionary-based tree containing a function or class
   descriptors for each function and class defined in the module with a
   ``def`` or ``class`` statement.  The returned dictionary maps
   module-level function and class names to their descriptors.  Nested
   objects are entered into the children dictionary of their parent.  As
   with readmodule, *module* names the module to be read and *path* is
   prepended to sys.path.  If the module being read is a package, the
   returned dictionary has a key ``'__path__'`` whose value is a list
   containing the package search path.

.. versionadded:: 3.7
   Descriptors for nested definitions.  They are accessed through the
   new children attribute.  Each has a new parent attribute.

The descriptors returned by these functions are instances of
Function and Class classes.  Users are not expected to create instances
of these classes.


.. _pyclbr-function-objects:

Function Objects
----------------
Class :class:`Function` instances describe functions defined by def
statements.  They have the following attributes:


.. attribute:: Function.file

   Name of the file in which the function is defined.


.. attribute:: Function.module

   The name of the module defining the function described.


.. attribute:: Function.name

   The name of the function.


.. attribute:: Function.lineno

   The line number in the file where the definition starts.


.. attribute:: Function.parent

   For top-level functions, None.  For nested functions, the parent.

   .. versionadded:: 3.7


.. attribute:: Function.children

   A dictionary mapping names to descriptors for nested functions and
   classes.

   .. versionadded:: 3.7


.. _pyclbr-class-objects:

Class Objects
-------------
Class :class:`Class` instances describe classes defined by class
statements.  They have the same attributes as Functions and two more.


.. attribute:: Class.file

   Name of the file in which the class is defined.


.. attribute:: Class.module

   The name of the module defining the class described.


.. attribute:: Class.name

   The name of the class.


.. attribute:: Class.lineno

   The line number in the file where the definition starts.


.. attribute:: Class.parent

   For top-level classes, None.  For nested classes, the parent.

   .. versionadded:: 3.7


.. attribute:: Class.children

   A dictionary mapping names to descriptors for nested functions and
   classes.

   .. versionadded:: 3.7


.. attribute:: Class.super

   A list of :class:`Class` objects which describe the immediate base
   classes of the class being described.  Classes which are named as
   superclasses but which are not discoverable by :func:`readmodule_ex`
   are listed as a string with the class name instead of as
   :class:`Class` objects.


.. attribute:: Class.methods

   A dictionary mapping method names to line numbers.  This can be
   derived from the newer children dictionary, but remains for
   back-compatibility.
