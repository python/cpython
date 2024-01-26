
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
   available on `the PyPy project's home page <https://www.pypy.org/>`_.

Each of these implementations varies in some way from the language as documented
in this manual, or introduces specific information beyond what's covered in the
standard Python documentation.  Please refer to the implementation-specific
documentation to determine what else you need to know about the specific
implementation you're using.


.. _notation:

Notation
========

.. index:: BNF, grammar, syntax, notation

The descriptions of lexical analysis and syntax use a modified
`Backus–Naur form (BNF) <https://en.wikipedia.org/wiki/Backus%E2%80%93Naur_form>`_ grammar
notation.  This uses the following style of definition:

.. productionlist:: notation
   name: `lc_letter` (`lc_letter` | "_")*
   lc_letter: "a"..."z"

The first line says that a ``name`` is an ``lc_letter`` followed by a sequence
of zero or more ``lc_letter``\ s and underscores.  An ``lc_letter`` in turn is
any of the single characters ``'a'`` through ``'z'``.  (This rule is actually
adhered to for the names defined in lexical and grammar rules in this document.)

Each rule begins with a name (which is the name defined by the rule) and
``::=``.  A vertical bar (``|``) is used to separate alternatives; it is the
least binding operator in this notation.  A star (``*``) means zero or more
repetitions of the preceding item; likewise, a plus (``+``) means one or more
repetitions, and a phrase enclosed in square brackets (``[ ]``) means zero or
one occurrences (in other words, the enclosed phrase is optional).  The ``*``
and ``+`` operators bind as tightly as possible; parentheses are used for
grouping.  Literal strings are enclosed in quotes.  White space is only
meaningful to separate tokens. Rules are normally contained on a single line;
rules with many alternatives may be formatted alternatively with each line after
the first beginning with a vertical bar.

.. index:: lexical definitions, ASCII

In lexical definitions (as the example above), two more conventions are used:
Two literal characters separated by three dots mean a choice of any single
character in the given (inclusive) range of ASCII characters.  A phrase between
angular brackets (``<...>``) gives an informal description of the symbol
defined; e.g., this could be used to describe the notion of 'control character'
if needed.

Even though the notation used is almost the same, there is a big difference
between the meaning of lexical and syntactic definitions: a lexical definition
operates on the individual characters of the input source, while a syntax
definition operates on the stream of tokens generated by the lexical analysis.
All uses of BNF in the next chapter ("Lexical Analysis") are lexical
definitions; uses in subsequent chapters are syntactic definitions.

