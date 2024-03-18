:mod:`token` --- Constants used with Python parse trees
=======================================================

.. module:: token
   :synopsis: Constants representing terminal nodes of the parse tree.

.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

**Source code:** :source:`Lib/token.py`

--------------

This module provides constants which represent the numeric values of leaf nodes
of the parse tree (terminal tokens).  Refer to the file :file:`Grammar/Tokens`
in the Python distribution for the definitions of the names in the context of
the language grammar.  The specific numeric values which the names map to may
change between Python versions.

The module also provides a mapping from numeric codes to names and some
functions.  The functions mirror definitions in the Python C header files.


.. data:: tok_name

   Dictionary mapping the numeric values of the constants defined in this module
   back to name strings, allowing more human-readable representation of parse trees
   to be generated.


.. function:: ISTERMINAL(x)

   Return ``True`` for terminal token values.


.. function:: ISNONTERMINAL(x)

   Return ``True`` for non-terminal token values.


.. function:: ISEOF(x)

   Return ``True`` if *x* is the marker indicating the end of input.


The token constants are:

.. include:: token-list.inc

The following token type values aren't used by the C tokenizer but are needed for
the :mod:`tokenize` module.

.. data:: COMMENT
   :noindex:

   Token value used to indicate a comment.


.. data:: NL
   :noindex:

   Token value used to indicate a non-terminating newline.  The
   :data:`NEWLINE` token indicates the end of a logical line of Python code;
   ``NL`` tokens are generated when a logical line of code is continued over
   multiple physical lines.


.. data:: ENCODING

   Token value that indicates the encoding used to decode the source bytes
   into text. The first token returned by :func:`tokenize.tokenize` will
   always be an ``ENCODING`` token.


.. data:: TYPE_COMMENT
   :noindex:

   Token value indicating that a type comment was recognized.  Such
   tokens are only produced when :func:`ast.parse()` is invoked with
   ``type_comments=True``.


.. versionchanged:: 3.5
   Added :data:`!AWAIT` and :data:`!ASYNC` tokens.

.. versionchanged:: 3.7
   Added :data:`COMMENT`, :data:`NL` and :data:`ENCODING` tokens.

.. versionchanged:: 3.7
   Removed :data:`!AWAIT` and :data:`!ASYNC` tokens. "async" and "await" are
   now tokenized as :data:`NAME` tokens.

.. versionchanged:: 3.8
   Added :data:`TYPE_COMMENT`, :data:`TYPE_IGNORE`, :data:`COLONEQUAL`.
   Added :data:`!AWAIT` and :data:`!ASYNC` tokens back (they're needed
   to support parsing older Python versions for :func:`ast.parse` with
   ``feature_version`` set to 6 or lower).

.. versionchanged:: 3.13
   Removed :data:`!AWAIT` and :data:`!ASYNC` tokens again.

