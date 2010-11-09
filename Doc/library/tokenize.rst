:mod:`tokenize` --- Tokenizer for Python source
===============================================

.. module:: tokenize
   :synopsis: Lexical scanner for Python source code.
.. moduleauthor:: Ka Ping Yee
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


The :mod:`tokenize` module provides a lexical scanner for Python source code,
implemented in Python.  The scanner in this module returns comments as tokens
as well, making it useful for implementing "pretty-printers," including
colorizers for on-screen displays.

The primary entry point is a :term:`generator`:

.. function:: tokenize(readline)

   The :func:`tokenize` generator requires one argument, *readline*, which
   must be a callable object which provides the same interface as the
   :meth:`io.IOBase.readline` method of file objects.  Each call to the
   function should return one line of input as bytes.

   The generator produces 5-tuples with these members: the token type; the
   token string; a 2-tuple ``(srow, scol)`` of ints specifying the row and
   column where the token begins in the source; a 2-tuple ``(erow, ecol)`` of
   ints specifying the row and column where the token ends in the source; and
   the line on which the token was found. The line passed (the last tuple item)
   is the *logical* line; continuation lines are included.  The 5 tuple is
   returned as a :term:`named tuple` with the field names:
   ``type string start end line``.

   .. versionchanged:: 3.1
      Added support for named tuples.

   :func:`tokenize` determines the source encoding of the file by looking for a
   UTF-8 BOM or encoding cookie, according to :pep:`263`.


All constants from the :mod:`token` module are also exported from
:mod:`tokenize`, as are three additional token type values:

.. data:: COMMENT

   Token value used to indicate a comment.


.. data:: NL

   Token value used to indicate a non-terminating newline.  The NEWLINE token
   indicates the end of a logical line of Python code; NL tokens are generated
   when a logical line of code is continued over multiple physical lines.


.. data:: ENCODING

    Token value that indicates the encoding used to decode the source bytes
    into text. The first token returned by :func:`tokenize` will always be an
    ENCODING token.


Another function is provided to reverse the tokenization process. This is
useful for creating tools that tokenize a script, modify the token stream, and
write back the modified script.


.. function:: untokenize(iterable)

    Converts tokens back into Python source code.  The *iterable* must return
    sequences with at least two elements, the token type and the token string.
    Any additional sequence elements are ignored.

    The reconstructed script is returned as a single string.  The result is
    guaranteed to tokenize back to match the input so that the conversion is
    lossless and round-trips are assured.  The guarantee applies only to the
    token type and token string as the spacing between tokens (column
    positions) may change.

    It returns bytes, encoded using the ENCODING token, which is the first
    token sequence output by :func:`tokenize`.


:func:`tokenize` needs to detect the encoding of source files it tokenizes. The
function it uses to do this is available:

.. function:: detect_encoding(readline)

    The :func:`detect_encoding` function is used to detect the encoding that
    should be used to decode a Python source file. It requires one argument,
    readline, in the same way as the :func:`tokenize` generator.

    It will call readline a maximum of twice, and return the encoding used
    (as a string) and a list of any lines (not decoded from bytes) it has read
    in.

    It detects the encoding from the presence of a UTF-8 BOM or an encoding
    cookie as specified in :pep:`263`. If both a BOM and a cookie are present,
    but disagree, a SyntaxError will be raised. Note that if the BOM is found,
    ``'utf-8-sig'`` will be returned as an encoding.

    If no encoding is specified, then the default of ``'utf-8'`` will be
    returned.

    Use :func:`open` to open Python source files: it uses
    :func:`detect_encoding` to detect the file encoding.


.. function:: open(filename)

   Open a file in read only mode using the encoding detected by
   :func:`detect_encoding`.

   .. versionadded:: 3.2


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

        >>> exec(s) #doctest: +ELLIPSIS
        -3.21716034272e-0...7

        Output from calculations with Decimal should be identical across all
        platforms.

        >>> exec(decistmt(s))
        -3.217160342717258261933904529E-7
        """
        result = []
        g = tokenize(BytesIO(s.encode('utf-8')).readline) # tokenize the string
        for toknum, tokval, _, _, _  in g:
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

