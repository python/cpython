.. _builtin-index:

##################################
  Built-in Functions and Classes
##################################

Python comes with a number of built-in functions and classes.

The built-in classes include data types that would normally be considered part of the "core" of a
language, such as numbers and lists.  For these types, the Python language core
defines the form of literals and places some constraints on their semantics, but
does not fully define the semantics.

The built-ins also includes functions and exceptions --- objects that can
be used by all Python code without the need of an :keyword:`import` statement.
Some of these are defined by the core language, but many are not essential for
the core semantics and are only described here.

.. We don't use :numbered: option for the TOC below as it enforces
   numbered sections for the entire stdlib docs.  If desired,
   :numbered: can be enabled on a per-module basis.
.. toctree::
   :maxdepth: 2

   functions.rst
   constants.rst
   stdtypes.rst
   exceptions.rst
   threadsafety.rst
   time-complexity.rst
