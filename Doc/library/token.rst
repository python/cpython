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

Note that a token's value may depend on tokenizer options. For example, a
``"+"`` token may be reported as either :data:`PLUS` or :data:`OP`, or
a ``"match"`` token may be either :data:`NAME` or :data:`SOFT_KEYWORD`.


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

.. data:: NAME

   Token value that indicates an :ref:`identifier <identifiers>`.
   Note that keywords are also initially tokenized an ``NAME`` tokens.

.. data:: NUMBER

   Token value that indicates a :ref:`numeric literal <numbers>`

.. data:: STRING

   Token value that indicates a :ref:`string or byte literal <strings>`,
   excluding :ref:`formatted string literals <f-strings>`.
   The token string is not interpreted:
   it includes the surrounding quotation marks and the prefix (if given);
   backslashes are included literally, without processing escape sequences.

.. data:: OP

   A generic token value that indicates an
   :ref:`operator <operators>` or :ref:`delimiter <delimiters>`.

   .. impl-detail::

      This value is only reported by the :mod:`tokenize` module.
      Internally, the tokenizer uses
      :ref:`exact token types <token_operators_delimiters>` instead.

.. data:: COMMENT

   Token value used to indicate a comment.
   The parser ignores :data:`!COMMENT` tokens.

.. data:: NEWLINE

   Token value that indicates the end of a :ref:`logical line <logical-lines>`.

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

.. data:: FSTRING_START

   Token value used to indicate the beginning of an
   :ref:`f-string literal <f-strings>`.

   .. impl-detail::

      The token string includes the prefix and the opening quote(s), but none
      of the contents of the literal.

.. data:: FSTRING_MIDDLE

   Token value used for literal text inside an :ref:`f-string literal <f-strings>`,
   including format specifications.

   .. impl-detail::

      Replacement fields (that is, the non-literal parts of f-strings) use
      the same tokens as other expressions, and are delimited by
      :data:`LBRACE`, :data:`RBRACE`, :data:`EXCLAMATION` and :data:`COLON`
      tokens.

.. data:: FSTRING_END

   Token value used to indicate the end of a :ref:`f-string <f-strings>`.

   .. impl-detail::

      The token string contains the closing quote(s).

.. data:: TSTRING_START

   Token value used to indicate the beginning of a template string literal.

   .. impl-detail::

      The token string includes the prefix and the opening quote(s), but none
      of the contents of the literal.

   .. versionadded:: 3.14

.. data:: TSTRING_MIDDLE

   Token value used for literal text inside a template string literal
   including format specifications.

   .. impl-detail::

      Replacement fields (that is, the non-literal parts of t-strings) use
      the same tokens as other expressions, and are delimited by
      :data:`LBRACE`, :data:`RBRACE`, :data:`EXCLAMATION` and :data:`COLON`
      tokens.

   .. versionadded:: 3.14

.. data:: TSTRING_END

   Token value used to indicate the end of a template string literal.

   .. impl-detail::

      The token string contains the closing quote(s).

   .. versionadded:: 3.14

.. data:: ENDMARKER

   Token value that indicates the end of input.
   Used in :ref:`top-level grammar rules <top-level>`.

.. data:: ENCODING

   Token value that indicates the encoding used to decode the source bytes
   into text. The first token returned by :func:`tokenize.tokenize` will
   always be an ``ENCODING`` token.

   .. impl-detail::

      This token type isn't used by the C tokenizer but is needed for
      the :mod:`tokenize` module.


The following token types are not produced by the :mod:`tokenize` module,
and are defined for special uses in the tokenizer or parser:

.. data:: TYPE_IGNORE

   Token value indicating that a ``type: ignore`` comment was recognized.
   Such tokens are produced instead of regular :data:`COMMENT` tokens only
   with the :data:`~ast.PyCF_TYPE_COMMENTS` flag.

.. data:: TYPE_COMMENT

   Token value indicating that a type comment was recognized.
   Such tokens are produced instead of regular :data:`COMMENT` tokens only
   with the :data:`~ast.PyCF_TYPE_COMMENTS` flag.

.. data:: SOFT_KEYWORD

   Token value indicating a :ref:`soft keyword <soft-keywords>`.

   The tokenizer never produces this value.
   To check for a soft keyword, pass a :data:`NAME` token's string to
   :func:`keyword.issoftkeyword`.

.. data:: ERRORTOKEN

   Token value used to indicate wrong input.

   The :mod:`tokenize` module generally indicates errors by
   raising exceptions instead of emitting this token.
   It can also emit tokens such as :data:`OP` or :data:`NAME` with strings that
   are later rejected by the parser.


.. _token_operators_delimiters:

The remaining tokens represent specific :ref:`operators <operators>` and
:ref:`delimiters <delimiters>`.
(The :mod:`tokenize` module reports these as :data:`OP`; see ``exact_type``
in the :mod:`tokenize` documentation for details.)

.. include:: token-list.inc


The following non-token constants are provided:

.. data:: N_TOKENS

   The number of token types defined in this module.

.. NT_OFFSET is deliberately undocumented; if you need it you should be
   reading the source

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

.. versionchanged:: 3.12
   Added :data:`EXCLAMATION`.

.. versionchanged:: 3.13
   Removed :data:`!AWAIT` and :data:`!ASYNC` tokens again.

