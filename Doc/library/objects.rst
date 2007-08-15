
.. _builtin:

****************
Built-in Objects
****************

.. index::
   pair: built-in; types
   pair: built-in; exceptions
   pair: built-in; functions
   pair: built-in; constants
   single: symbol table

Names for built-in exceptions and functions and a number of constants are found
in a separate  symbol table.  This table is searched last when the interpreter
looks up the meaning of a name, so local and global user-defined names can
override built-in names.  Built-in types are described together here for easy
reference. [#]_

The tables in this chapter document the priorities of operators by listing them
in order of ascending priority (within a table) and grouping operators that have
the same priority in the same box. Binary operators of the same priority group
from left to right. (Unary operators group from right to left, but there you
have no real choice.)  See :ref:`operator-summary` for the complete picture on
operator priorities.

.. rubric:: Footnotes

.. [#] Most descriptions sorely lack explanations of the exceptions that may be raised
   --- this will be fixed in a future version of this manual.

