
:mod:`token` --- Constants used with Python parse trees
=======================================================

.. module:: token
   :synopsis: Constants representing terminal nodes of the parse tree.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


This module provides constants which represent the numeric values of leaf nodes
of the parse tree (terminal tokens).  Refer to the file :file:`Grammar/Grammar`
in the Python distribution for the definitions of the names in the context of
the language grammar.  The specific numeric values which the names map to may
change between Python versions.

This module also provides one data object and some functions.  The functions
mirror definitions in the Python C header files.


.. data:: tok_name

   Dictionary mapping the numeric values of the constants defined in this module
   back to name strings, allowing more human-readable representation of parse trees
   to be generated.


.. function:: ISTERMINAL(x)

   Return true for terminal token values.


.. function:: ISNONTERMINAL(x)

   Return true for non-terminal token values.


.. function:: ISEOF(x)

   Return true if *x* is the marker indicating the end of input.


.. seealso::

   Module :mod:`parser`
      The second example for the :mod:`parser` module shows how to use the
      :mod:`symbol` module.

