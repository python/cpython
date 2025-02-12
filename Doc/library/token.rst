:mod:`!token` --- Constants used with Python parse trees
========================================================

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

.. data:: ENDMARKER

   Token value that indicates the end of input.
   Used in :ref:`top-level grammar rules <top-level>`.

.. data:: NAME

   Token value that indicates an :ref:`identifier <identifiers>`.
   Note that keywords are also identifiers.

.. data:: NUMBER

   Token value that indicates a :ref:`numeric literal <numbers>`

.. data:: STRING

   Token value that indicates a :ref:`string or byte literal <strings>`.
   The token string is not interpreted: it includes the prefix (if any)
   and the quote characters; escape sequences are included with their
   initial backslash.

.. data:: NEWLINE

   Token value that indicates the end of a :ref:`logical line <logical-lines>`
   of Python code.

.. data:: NL

   Token value used to indicate a non-terminating newline.
   :data:`!NL` tokens are generated when a logical line of code is continued
   over multiple physical lines. The parser ignores :data:`!NL` tokens.

.. data:: INDENT

   Token value used at the beginning of a :ref:`logical line <logical-lines>`
   to indicate the start of an :ref:`indented block <indentation>`.

.. data:: DEDENT

   Token value used at the beginning of a :ref:`logical line <logical-lines>`
   to indicate the end of an :ref:`indented block <indentation>`.

.. data:: OP

   A generic token value returned by the :mod:`tokenize` module for
   :ref:`operator <operators>` and :ref:`delimiter <delimiters>`.
   See the :mod:`tokenize` module documentation for details.

.. data:: TYPE_IGNORE

   Token value indicating that a ``type: ignore`` comment was recognized.
   Such tokens are only produced when :func:`ast.parse` is invoked with
   ``type_comments=True``.

.. data:: TYPE_COMMENT

   Token value indicating that a type comment was recognized.  Such
   tokens are only produced when :func:`ast.parse` is invoked with
   ``type_comments=True``.

.. data:: SOFT_KEYWORD

.. data:: FSTRING_START

   .. impl-detail::

      Token value used to indicate the beginning of a
      :ref:`f-string <f-strings>`.
      The token string includes the prefix and the opening quote, but none
      of the contents of the literal.

.. data:: FSTRING_MIDDLE

   .. impl-detail::

      Token value used for literal text inside an :ref:`f-string <f-strings>`,
      including format specifications.

      Replacement fields (that is, the non-literal parts of f-strings) use
      the same tokens as other expressions, and are delimited by :data:`LBRACE`
      and :data:`RBRACE` tokens.

.. data:: FSTRING_END

   .. impl-detail::

      Token value used to indicate the end of a :ref:`f-string <f-strings>`.
      The token string contains the closing quote.

.. data:: COMMENT

   Token value used to indicate a comment.
   The parser ignores :data:`!COMMENT` tokens.

.. data:: ERRORTOKEN

   Token value used to indicate wrong input.

   .. impl-detail::

      The :mod:`tokenize` module generally indicates errors by
      raising exceptions instead of emitting this token.

.. data:: ENCODING

   Token value that indicates the encoding used to decode the source bytes
   into text. The first token returned by :func:`tokenize.tokenize` will
   always be an ``ENCODING`` token.

   .. impl-detail::

      This token type isn't used by the C tokenizer but is needed for
      the :mod:`tokenize` module.

The remaining tokens represent literal text; most are :ref:`operators`
and :ref:`delimiters`:

.. include:: token-list.inc

.. data:: N_TOKENS

.. data:: NT_OFFSET

.. data:: EXACT_TOKEN_TYPES

   A dictionary mapping the string representation of a token to its numeric code.

   .. versionadded:: 3.8


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

