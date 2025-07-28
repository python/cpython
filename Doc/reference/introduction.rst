
.. _introduction:

************
Introduction
************

This reference manual describes the Python programming language. It is not
intended as a tutorial.

While I am trying to be as precise as possible, I chose to use English rather
than formal specifications for everything except syntax and lexical analysis.
This should make the document more understandable to the average reader, but
will leave room for ambiguities. Consequently, if you were coming from Mars and
tried to re-implement Python from this document alone, you might have to guess
things and in fact you would probably end up implementing quite a different
language. On the other hand, if you are using Python and wonder what the precise
rules about a particular area of the language are, you should definitely be able
to find them here. If you would like to see a more formal definition of the
language, maybe you could volunteer your time --- or invent a cloning machine
:-).

It is dangerous to add too many implementation details to a language reference
document --- the implementation may change, and other implementations of the
same language may work differently.  On the other hand, CPython is the one
Python implementation in widespread use (although alternate implementations
continue to gain support), and its particular quirks are sometimes worth being
mentioned, especially where the implementation imposes additional limitations.
Therefore, you'll find short "implementation notes" sprinkled throughout the
text.

Every Python implementation comes with a number of built-in and standard
modules.  These are documented in :ref:`library-index`.  A few built-in modules
are mentioned when they interact in a significant way with the language
definition.


.. _implementations:

Alternate Implementations
=========================

Though there is one Python implementation which is by far the most popular,
there are some alternate implementations which are of particular interest to
different audiences.

Known implementations include:

CPython
   This is the original and most-maintained implementation of Python, written in C.
   New language features generally appear here first.

Jython
   Python implemented in Java.  This implementation can be used as a scripting
   language for Java applications, or can be used to create applications using the
   Java class libraries.  It is also often used to create tests for Java libraries.
   More information can be found at `the Jython website <https://www.jython.org/>`_.

Python for .NET
   This implementation actually uses the CPython implementation, but is a managed
   .NET application and makes .NET libraries available.  It was created by Brian
   Lloyd.  For more information, see the `Python for .NET home page
   <https://pythonnet.github.io/>`_.

IronPython
   An alternate Python for .NET.  Unlike Python.NET, this is a complete Python
   implementation that generates IL, and compiles Python code directly to .NET
   assemblies.  It was created by Jim Hugunin, the original creator of Jython.  For
   more information, see `the IronPython website <https://ironpython.net/>`_.

PyPy
   An implementation of Python written completely in Python. It supports several
   advanced features not found in other implementations like stackless support
   and a Just in Time compiler. One of the goals of the project is to encourage
   experimentation with the language itself by making it easier to modify the
   interpreter (since it is written in Python).  Additional information is
   available on `the PyPy project's home page <https://pypy.org/>`_.

Each of these implementations varies in some way from the language as documented
in this manual, or introduces specific information beyond what's covered in the
standard Python documentation.  Please refer to the implementation-specific
documentation to determine what else you need to know about the specific
implementation you're using.


.. _notation:

Notation
========

.. index:: BNF, grammar, syntax, notation

The descriptions of lexical analysis and syntax use a grammar notation that
is a mixture of
`EBNF <https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form>`_
and `PEG <https://en.wikipedia.org/wiki/Parsing_expression_grammar>`_.
For example:

.. grammar-snippet::
   :group: notation

   name:   `letter` (`letter` | `digit` | "_")*
   letter: "a"..."z" | "A"..."Z"
   digit:  "0"..."9"

In this example, the first line says that a ``name`` is a ``letter`` followed
by a sequence of zero or more ``letter``\ s, ``digit``\ s, and underscores.
A ``letter`` in turn is any of the single characters ``'a'`` through
``'z'`` and ``A`` through ``Z``; a ``digit`` is a single character from ``0``
to ``9``.

Each rule begins with a name (which identifies the rule that's being defined)
followed by a colon, ``:``.
The definition to the right of the colon uses the following syntax elements:

* ``name``: A name refers to another rule.
  Where possible, it is a link to the rule's definition.

  * ``TOKEN``: An uppercase name refers to a :term:`token`.
    For the purposes of grammar definitions, tokens are the same as rules.

* ``"text"``, ``'text'``: Text in single or double quotes must match literally
  (without the quotes). The type of quote is chosen according to the meaning
  of ``text``:

  * ``'if'``: A name in single quotes denotes a :ref:`keyword <keywords>`.
  * ``"case"``: A name in double quotes denotes a
    :ref:`soft-keyword <soft-keywords>`.
  * ``'@'``: A non-letter symbol in single quotes denotes an
    :py:data:`~token.OP` token, that is, a :ref:`delimiter <delimiters>` or
    :ref:`operator <operators>`.

* ``e1 e2``: Items separated only by whitespace denote a sequence.
  Here, ``e1`` must be followed by ``e2``.
* ``e1 | e2``: A vertical bar is used to separate alternatives.
  It denotes PEG's "ordered choice": if ``e1`` matches, ``e2`` is
  not considered.
  In traditional PEG grammars, this is written as a slash, ``/``, rather than
  a vertical bar.
  See :pep:`617` for more background and details.
* ``e*``: A star means zero or more repetitions of the preceding item.
* ``e+``: Likewise, a plus means one or more repetitions.
* ``[e]``: A phrase enclosed in square brackets means zero or
  one occurrences. In other words, the enclosed phrase is optional.
* ``e?``: A question mark has exactly the same meaning as square brackets:
  the preceding item is optional.
* ``(e)``: Parentheses are used for grouping.

The following notation is only used in
:ref:`lexical definitions <notation-lexical-vs-syntactic>`.

* ``"a"..."z"``: Two literal characters separated by three dots mean a choice
  of any single character in the given (inclusive) range of ASCII characters.
* ``<...>``: A phrase between angular brackets gives an informal description
  of the matched symbol (for example, ``<any ASCII character except "\">``),
  or an abbreviation that is defined in nearby text (for example, ``<Lu>``).

.. _lexical-lookaheads:

Some definitions also use *lookaheads*, which indicate that an element
must (or must not) match at a given position, but without consuming any input:

* ``&e``: a positive lookahead (that is, ``e`` is required to match)
* ``!e``: a negative lookahead (that is, ``e`` is required *not* to match)

The unary operators (``*``, ``+``, ``?``) bind as tightly as possible;
the vertical bar (``|``) binds most loosely.

White space is only meaningful to separate tokens.

Rules are normally contained on a single line, but rules that are too long
may be wrapped:

.. grammar-snippet::
   :group: notation

   literal: stringliteral | bytesliteral
            | integer | floatnumber | imagnumber

Alternatively, rules may be formatted with the first line ending at the colon,
and each alternative beginning with a vertical bar on a new line.
For example:


.. grammar-snippet::
   :group: notation-alt

   literal:
      | stringliteral
      | bytesliteral
      | integer
      | floatnumber
      | imagnumber

This does *not* mean that there is an empty first alternative.

.. index:: lexical definitions

.. _notation-lexical-vs-syntactic:

Lexical and Syntactic definitions
---------------------------------

There is some difference between *lexical* and *syntactic* analysis:
the :term:`lexical analyzer` operates on the individual characters of the
input source, while the *parser* (syntactic analyzer) operates on the stream
of :term:`tokens <token>` generated by the lexical analysis.
However, in some cases the exact boundary between the two phases is a
CPython implementation detail.

The practical difference between the two is that in *lexical* definitions,
all whitespace is significant.
The lexical analyzer :ref:`discards <whitespace>` all whitespace that is not
converted to tokens like :data:`token.INDENT` or :data:`~token.NEWLINE`.
*Syntactic* definitions then use these tokens, rather than source characters.

This documentation uses the same BNF grammar for both styles of definitions.
All uses of BNF in the next chapter (:ref:`lexical`) are lexical definitions;
uses in subsequent chapters are syntactic definitions.
