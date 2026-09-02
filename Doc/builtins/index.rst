.. _builtins-index:

##############################
  Python built-ins reference
##############################

Python comes with a number of built-in functions and classes.

The built-in classes include data types that would normally be considered part
of the "core" of a language, such as numbers and lists.  For these types, the
Python language core defines the form of literals and places some constraints
on their semantics, but does not fully define the semantics.

The built-ins also include functions and exceptions --- objects that can
be used by all Python code without the need of an :keyword:`import` statement.
Some of these are defined by the core language, but many are not essential for
the core semantics and are only described here.

.. We don't use :numbered: option for the TOC below as it enforces
   numbered sections for the entire builtin docs.  If desired,
   :numbered: can be enabled on a per-page basis.
.. toctree::
   :maxdepth: 2

   stdtypes.rst
   constants.rst
   functions.rst
   exceptions.rst
   threadsafety.rst
   time-complexity.rst
