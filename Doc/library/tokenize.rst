:mod:`!tokenize` --- Tokenizer for Python source
================================================

.. module:: tokenize
   :synopsis: Lexical scanner for Python source code.

**Source code:** :source:`Lib/tokenize.py`

--------------

The :mod:`!tokenize` module provides a lexical scanner for Python source code,
implemented in Python.  The scanner in this module returns comments as tokens
as well, making it useful for implementing "pretty-printers", including
colorizers for on-screen displays.

To simplify token stream handling, all :ref:`operator <operators>` and
:ref:`delimiter <delimiters>` tokens and :data:`Ellipsis` are returned using
the generic :data:`~token.OP` token type.
The :attr:`TokenInfo.exact_type` property gives the exact token type.


.. warning::

   Note that the functions in this module are only designed to parse
   syntactically valid Python code (code that does not raise when parsed
   using :func:`ast.parse`).  The behavior of the functions in this module is
   **undefined** when providing invalid Python code and it can change at any
   point.

Tokenizing input
----------------

The primary entry point is a :term:`generator`:

.. function:: tokenize(readline)

   The :func:`.tokenize` generator requires one argument, *readline*, which
   must be a callable object which provides the same interface as the
   :meth:`io.IOBase.readline` method of file objects.  Each call to the
   function should return one line of input as bytes.

   The generator produces :class:`TokenInfo` objects.

   .. versionchanged:: 3.1
      Changed to yield :class:`TokenInfo` objects instead of plain tuples.

   :func:`.tokenize` determines the source encoding of the file by looking for a
   UTF-8 BOM or encoding cookie, according to :pep:`263`.

.. function:: generate_tokens(readline)

   Tokenize a source reading unicode strings instead of bytes.

   Like :func:`.tokenize`, the *readline* argument is a callable returning
   a single line of input. However, :func:`generate_tokens` expects *readline*
   to return a str object rather than bytes.

   The result is an iterator yielding :class:`TokenInfo` objects, exactly like
   :func:`.tokenize`. It does not yield an :data:`~token.ENCODING` token.

.. class:: TokenInfo(type, string, start, end, line)

   A :term:`named tuple` describing a token. It has five fields and an
   additional :attr:`exact_type` property.

   .. versionadded:: 3.1

   .. attribute:: type

      An integer identifying the token type, as defined by the constants in
      :mod:`token`, such as :data:`~token.NAME` and :data:`~token.STRING`.
      Operators and delimiters have type :data:`~token.OP`; use
      :attr:`exact_type` for their specific token type.

   .. attribute:: string

      The token text as a :class:`str`.
      For :data:`~token.STRING` tokens, this includes the quotes, any prefix,
      and escape sequences, without evaluating them.

      For an :data:`~token.ENCODING` token, this is the encoding name.
      Tokens inserted without corresponding source text can have an empty
      string. In :data:`~token.FSTRING_MIDDLE` and
      :data:`~token.TSTRING_MIDDLE` tokens, doubled braces are replaced by
      single braces.

   .. attribute:: start

      A 2-tuple ``(srow, scol)`` of ints specifying the inclusive start
      position of the token in the source. Rows are numbered from 1 and
      columns from 0. Columns count Unicode code points; a tab counts as
      one column, without expanding it to a tab stop.

      For an :data:`~token.ENCODING` token, this is ``(0, 0)``.

   .. attribute:: end

      A 2-tuple ``(erow, ecol)`` of ints specifying the exclusive end
      position of the token. Rows are numbered from 1 and columns from 0.
      For a token spanning multiple lines, *erow* is the final row and
      *ecol* is the column on that row.

      For example, the name ``spam`` at the beginning of the first line
      has :attr:`start` equal to ``(1, 0)`` and :attr:`end` equal to ``(1, 4)``.

      For an :data:`~token.ENCODING` token, this is ``(0, 0)``.
      A :data:`~token.NEWLINE` or :data:`~token.NL` token inserted at the end
      of input without a final newline can have an end column one beyond
      the source text.

   .. attribute:: line

      A :class:`str` containing the full source line on which
      the token was found, including surrounding text and the line terminator
      if present. For a token spanning multiple lines, this can contain
      several source lines. It is empty for :data:`~token.ENCODING` and
      :data:`~token.ENDMARKER` tokens.

   .. attribute:: exact_type

      An integer identifying the specific operator type for
      :data:`~token.OP` tokens, as defined by the constants in :mod:`token`.
      For all other token types, this equals :attr:`type`.
      For example, a ``+`` token has :attr:`type` equal to :data:`~token.OP`
      and :attr:`exact_type` equal to :data:`~token.PLUS`.

      .. versionadded:: 3.3


All constants from the :mod:`token` module are also exported from
:mod:`!tokenize`.

Another function is provided to reverse the tokenization process. This is
useful for creating tools that tokenize a script, modify the token stream, and
write back the modified script.


.. function:: untokenize(iterable)

    Converts tokens back into Python source code.  The *iterable* must return
    sequences with at least two elements, the token type and the token string.
    Any additional sequence elements are ignored.

    The result is guaranteed to tokenize back to match the input so that the
    conversion is lossless and round-trips are assured.  The guarantee applies
    only to the token type and token string as the spacing between tokens
    (column positions) may change.

    It returns bytes, encoded using the :data:`~token.ENCODING` token, which
    is the first token sequence output by :func:`.tokenize`. If there is no
    encoding token in the input, it returns a str instead.


:func:`.tokenize` needs to detect the encoding of source files it tokenizes. The
function it uses to do this is available:

.. function:: detect_encoding(readline)

    The :func:`detect_encoding` function is used to detect the encoding that
    should be used to decode a Python source file. It requires one argument,
    readline, in the same way as the :func:`.tokenize` generator.

    It will call readline a maximum of twice, and return the encoding used
    (as a string) and a list of any lines (not decoded from bytes) it has read
    in.

    It detects the encoding from the presence of a UTF-8 BOM or an encoding
    cookie as specified in :pep:`263`. If both a BOM and a cookie are present,
    but disagree, a :exc:`SyntaxError` will be raised. Note that if the BOM is found,
    ``'utf-8-sig'`` will be returned as an encoding.

    If no encoding is specified, then the default of ``'utf-8'`` will be
    returned.

    Use :func:`.open` to open Python source files: it uses
    :func:`detect_encoding` to detect the file encoding.


.. function:: open(filename)

   Open a file in read only mode using the encoding detected by
   :func:`detect_encoding`.

   .. versionadded:: 3.2

.. exception:: TokenError

   Raised when either a docstring or expression that may be split over several
   lines is not completed anywhere in the file, for example::

      """Beginning of
      docstring

   or::

      [1,
       2,
       3

.. _tokenize-cli:

Command-line usage
------------------

.. versionadded:: 3.3

The :mod:`!tokenize` module can be executed as a script from the command line.
It is as simple as:

.. code-block:: sh

   python -m tokenize [-e] [filename.py]

The following options are accepted:

.. program:: tokenize

.. option:: -h, --help

   show this help message and exit

.. option:: -e, --exact

   display token names using the exact type

If :file:`filename.py` is specified its contents are tokenized to stdout.
Otherwise, tokenization is performed on stdin.

.. versionadded:: 3.15
   Output is in color by default and can be
   :ref:`controlled using environment variables <using-on-controlling-color>`.

Examples
--------

Example of a script rewriter that transforms float literals into Decimal
objects::

    from tokenize import tokenize, untokenize, NUMBER, STRING, NAME, OP
    from io import BytesIO

    def decistmt(s):
        """Substitute Decimals for floats in a string of statements.

        >>> from decimal import Decimal
        >>> s = 'print(+21.3e-5*-.1234/81.7)'
        >>> decistmt(s)
        "print (+Decimal ('21.3e-5')*-Decimal ('.1234')/Decimal ('81.7'))"

        The format of the exponent is inherited from the platform C library.
        Known cases are "e-007" (Windows) and "e-07" (not Windows).  Since
        we're only showing 12 digits, and the 13th isn't close to 5, the
        rest of the output should be platform-independent.

        >>> exec(s)  #doctest: +ELLIPSIS
        -3.21716034272e-0...7

        Output from calculations with Decimal should be identical across all
        platforms.

        >>> exec(decistmt(s))
        -3.217160342717258261933904529E-7
        """
        result = []
        g = tokenize(BytesIO(s.encode('utf-8')).readline)  # tokenize the string
        for toknum, tokval, _, _, _ in g:
            if toknum == NUMBER and '.' in tokval:  # replace NUMBER tokens
                result.extend([
                    (NAME, 'Decimal'),
                    (OP, '('),
                    (STRING, repr(tokval)),
                    (OP, ')')
                ])
            else:
                result.append((toknum, tokval))
        return untokenize(result).decode('utf-8')

Example of tokenizing from the command line.  The script::

    def say_hello():
        print("Hello, World!")

    say_hello()

will be tokenized to the following output where the first column is the range
of the line/column coordinates where the token is found, the second column is
the name of the token, and the final column is the value of the token (if any):

.. code-block:: shell-session

    $ python -m tokenize hello.py
    0,0-0,0:            ENCODING       'utf-8'
    1,0-1,3:            NAME           'def'
    1,4-1,13:           NAME           'say_hello'
    1,13-1,14:          OP             '('
    1,14-1,15:          OP             ')'
    1,15-1,16:          OP             ':'
    1,16-1,17:          NEWLINE        '\n'
    2,0-2,4:            INDENT         '    '
    2,4-2,9:            NAME           'print'
    2,9-2,10:           OP             '('
    2,10-2,25:          STRING         '"Hello, World!"'
    2,25-2,26:          OP             ')'
    2,26-2,27:          NEWLINE        '\n'
    3,0-3,1:            NL             '\n'
    4,0-4,0:            DEDENT         ''
    4,0-4,9:            NAME           'say_hello'
    4,9-4,10:           OP             '('
    4,10-4,11:          OP             ')'
    4,11-4,12:          NEWLINE        '\n'
    5,0-5,0:            ENDMARKER      ''

The exact token type names can be displayed using the :option:`-e` option:

.. code-block:: shell-session

    $ python -m tokenize -e hello.py
    0,0-0,0:            ENCODING       'utf-8'
    1,0-1,3:            NAME           'def'
    1,4-1,13:           NAME           'say_hello'
    1,13-1,14:          LPAR           '('
    1,14-1,15:          RPAR           ')'
    1,15-1,16:          COLON          ':'
    1,16-1,17:          NEWLINE        '\n'
    2,0-2,4:            INDENT         '    '
    2,4-2,9:            NAME           'print'
    2,9-2,10:           LPAR           '('
    2,10-2,25:          STRING         '"Hello, World!"'
    2,25-2,26:          RPAR           ')'
    2,26-2,27:          NEWLINE        '\n'
    3,0-3,1:            NL             '\n'
    4,0-4,0:            DEDENT         ''
    4,0-4,9:            NAME           'say_hello'
    4,9-4,10:           LPAR           '('
    4,10-4,11:          RPAR           ')'
    4,11-4,12:          NEWLINE        '\n'
    5,0-5,0:            ENDMARKER      ''

Example of tokenizing a file programmatically, reading unicode
strings instead of bytes with :func:`generate_tokens`::

    import tokenize

    with tokenize.open('hello.py') as f:
        tokens = tokenize.generate_tokens(f.readline)
        for token in tokens:
            print(token)

Or reading bytes directly with :func:`.tokenize`::

    import tokenize

    with open('hello.py', 'rb') as f:
        tokens = tokenize.tokenize(f.readline)
        for token in tokens:
            print(token)
