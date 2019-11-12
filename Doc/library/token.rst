:mod:`token` --- Constants used with Python parse trees
=======================================================

.. module:: token
   :synopsis: Constants representing terminal nodes of the parse tree.

.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>

**Source code:** :source:`Lib/token.py`

--------------

This module provides constants which represent the numeric values of leaf nodes
of the parse tree (terminal tokens).  Refer to the file :file:`Grammar/Grammar`
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

.. data:: ENDMARKER
          NAME
          NUMBER
          STRING
          NEWLINE
          INDENT
          DEDENT
          LPAR
          RPAR
          LSQB
          RSQB
          COLON
          COMMA
          SEMI
          PLUS
          MINUS
          STAR
          SLASH
          VBAR
          AMPER
          LESS
          GREATER
          EQUAL
          DOT
          PERCENT
          LBRACE
          RBRACE
          EQEQUAL
          NOTEQUAL
          LESSEQUAL
          GREATEREQUAL
          TILDE
          CIRCUMFLEX
          LEFTSHIFT
          RIGHTSHIFT
          DOUBLESTAR
          PLUSEQUAL
          MINEQUAL
          STAREQUAL
          SLASHEQUAL
          PERCENTEQUAL
          AMPEREQUAL
          VBAREQUAL
          CIRCUMFLEXEQUAL
          LEFTSHIFTEQUAL
          RIGHTSHIFTEQUAL
          DOUBLESTAREQUAL
          DOUBLESLASH
          DOUBLESLASHEQUAL
          AT
          ATEQUAL
          RARROW
          ELLIPSIS
          OP
          ERRORTOKEN
          N_TOKENS
          NT_OFFSET


The following token type values aren't used by the C tokenizer but are needed for
the :mod:`tokenize` module.

.. data:: COMMENT

   Token value used to indicate a comment.


.. data:: NL

   Token value used to indicate a non-terminating newline.  The
   :data:`NEWLINE` token indicates the end of a logical line of Python code;
   ``NL`` tokens are generated when a logical line of code is continued over
   multiple physical lines.


.. data:: ENCODING

   Token value that indicates the encoding used to decode the source bytes
   into text. The first token returned by :func:`tokenize.tokenize` will
   always be an ``ENCODING`` token.


.. versionchanged:: 3.5
   Added :data:`AWAIT` and :data:`ASYNC` tokens.

.. versionchanged:: 3.7
   Added :data:`COMMENT`, :data:`NL` and :data:`ENCODING` tokens.

.. versionchanged:: 3.7
   Removed :data:`AWAIT` and :data:`ASYNC` tokens. "async" and "await" are
   now tokenized as :data:`NAME` tokens.
