.. _tut-informal:

**********************************
An Informal Introduction to Python
**********************************

In the following examples, input and output are distinguished by the presence or
absence of prompts (``>>>`` and ``...``): to repeat the example, you must type
everything after the prompt, when the prompt appears; lines that do not begin
with a prompt are output from the interpreter. Note that a secondary prompt on a
line by itself in an example means you must type a blank line; this is used to
end a multi-line command.

Many of the examples in this manual, even those entered at the interactive
prompt, include comments.  Comments in Python start with the hash character,
``#``, and extend to the end of the physical line.  A comment may appear at
the start of a line or following whitespace or code, but not within a string
literal.  A hash character within a string literal is just a hash character.

Some examples::

   # this is the first comment
   SPAM = 1                 # and this is the second comment
                            # ... and now a third!
   STRING = "# This is not a comment."


.. _tut-calculator:

Using Python as a Calculator
============================

Let's try some simple Python commands.  Start the interpreter and wait for the
primary prompt, ``>>>``.  (It shouldn't take long.)


.. _tut-numbers:

Numbers
-------

The interpreter acts as a simple calculator: you can type an expression at it
and it will write the value.  Expression syntax is straightforward: the
operators ``+``, ``-``, ``*`` and ``/`` work just like in most other languages
(for example, Pascal or C); parentheses can be used for grouping.  For example::

   >>> 2+2
   4
   >>> # This is a comment
   ... 2+2
   4
   >>> 2+2  # and a comment on the same line as code
   4
   >>> (50-5*6)/4
   5
   >>> # Integer division returns the floor:
   ... 7/3
   2
   >>> 7/-3
   -3

The equal sign (``'='``) is used to assign a value to a variable. Afterwards, no
result is displayed before the next interactive prompt::

   >>> width = 20
   >>> height = 5*9
   >>> width * height
   900

A value can be assigned to several variables simultaneously::

   >>> x = y = z = 0  # Zero x, y and z
   >>> x
   0
   >>> y
   0
   >>> z
   0

There is full support for floating point; operators with mixed type operands
convert the integer operand to floating point::

   >>> 3 * 3.75 / 1.5
   7.5
   >>> 7.0 / 2
   3.5

Complex numbers are also supported; imaginary numbers are written with a suffix
of ``j`` or ``J``.  Complex numbers with a nonzero real component are written as
``(real+imagj)``, or can be created with the ``complex(real, imag)`` function.
::

   >>> 1j * 1J
   (-1+0j)
   >>> 1j * complex(0,1)
   (-1+0j)
   >>> 3+1j*3
   (3+3j)
   >>> (3+1j)*3
   (9+3j)
   >>> (1+2j)/(1+1j)
   (1.5+0.5j)

Complex numbers are always represented as two floating point numbers, the real
and imaginary part.  To extract these parts from a complex number *z*, use
``z.real`` and ``z.imag``.   ::

   >>> a=1.5+0.5j
   >>> a.real
   1.5
   >>> a.imag
   0.5

The conversion functions to floating point and integer (:func:`float`,
:func:`int` and :func:`long`) don't work for complex numbers --- there is no one
correct way to convert a complex number to a real number.  Use ``abs(z)`` to get
its magnitude (as a float) or ``z.real`` to get its real part. ::

   >>> a=3.0+4.0j
   >>> float(a)
   Traceback (most recent call last):
     File "<stdin>", line 1, in ?
   TypeError: can't convert complex to float; use abs(z)
   >>> a.real
   3.0
   >>> a.imag
   4.0
   >>> abs(a)  # sqrt(a.real**2 + a.imag**2)
   5.0
   >>>

In interactive mode, the last printed expression is assigned to the variable
``_``.  This means that when you are using Python as a desk calculator, it is
somewhat easier to continue calculations, for example::

   >>> tax = 12.5 / 100
   >>> price = 100.50
   >>> price * tax
   12.5625
   >>> price + _
   113.0625
   >>> round(_, 2)
   113.06
   >>>

This variable should be treated as read-only by the user.  Don't explicitly
assign a value to it --- you would create an independent local variable with the
same name masking the built-in variable with its magic behavior.


.. _tut-strings:

Strings
-------

Besides numbers, Python can also manipulate strings, which can be expressed in
several ways.  They can be enclosed in single quotes or double quotes::

   >>> 'spam eggs'
   'spam eggs'
   >>> 'doesn\'t'
   "doesn't"
   >>> "doesn't"
   "doesn't"
   >>> '"Yes," he said.'
   '"Yes," he said.'
   >>> "\"Yes,\" he said."
   '"Yes," he said.'
   >>> '"Isn\'t," she said.'
   '"Isn\'t," she said.'

String literals can span multiple lines in several ways.  Continuation lines can
be used, with a backslash as the last character on the line indicating that the
next line is a logical continuation of the line::

   hello = "This is a rather long string containing\n\
   several lines of text just as you would do in C.\n\
       Note that whitespace at the beginning of the line is\
    significant."

   print hello

Note that newlines still need to be embedded in the string using ``\n``; the
newline following the trailing backslash is discarded.  This example would print
the following::

   This is a rather long string containing
   several lines of text just as you would do in C.
       Note that whitespace at the beginning of the line is significant.

If we make the string literal a "raw" string, however, the ``\n`` sequences are
not converted to newlines, but the backslash at the end of the line, and the
newline character in the source, are both included in the string as data.  Thus,
the example::

   hello = r"This is a rather long string containing\n\
   several lines of text much as you would do in C."

   print hello

would print::

   This is a rather long string containing\n\
   several lines of text much as you would do in C.

Or, strings can be surrounded in a pair of matching triple-quotes: ``"""`` or
``'''``.  End of lines do not need to be escaped when using triple-quotes, but
they will be included in the string. ::

   print """
   Usage: thingy [OPTIONS] 
        -h                        Display this usage message
        -H hostname               Hostname to connect to
   """

produces the following output::

   Usage: thingy [OPTIONS] 
        -h                        Display this usage message
        -H hostname               Hostname to connect to

The interpreter prints the result of string operations in the same way as they
are typed for input: inside quotes, and with quotes and other funny characters
escaped by backslashes, to show the precise value.  The string is enclosed in
double quotes if the string contains a single quote and no double quotes, else
it's enclosed in single quotes.  (The :keyword:`print` statement, described
later, can be used to write strings without quotes or escapes.)

Strings can be concatenated (glued together) with the ``+`` operator, and
repeated with ``*``::

   >>> word = 'Help' + 'A'
   >>> word
   'HelpA'
   >>> '<' + word*5 + '>'
   '<HelpAHelpAHelpAHelpAHelpA>'

Two string literals next to each other are automatically concatenated; the first
line above could also have been written ``word = 'Help' 'A'``; this only works
with two literals, not with arbitrary string expressions::

   >>> 'str' 'ing'                   #  <-  This is ok
   'string'
   >>> 'str'.strip() + 'ing'   #  <-  This is ok
   'string'
   >>> 'str'.strip() 'ing'     #  <-  This is invalid
     File "<stdin>", line 1, in ?
       'str'.strip() 'ing'
                         ^
   SyntaxError: invalid syntax

Strings can be subscripted (indexed); like in C, the first character of a string
has subscript (index) 0.  There is no separate character type; a character is
simply a string of size one.  Like in Icon, substrings can be specified with the
*slice notation*: two indices separated by a colon. ::

   >>> word[4]
   'A'
   >>> word[0:2]
   'He'
   >>> word[2:4]
   'lp'

Slice indices have useful defaults; an omitted first index defaults to zero, an
omitted second index defaults to the size of the string being sliced. ::

   >>> word[:2]    # The first two characters
   'He'
   >>> word[2:]    # Everything except the first two characters
   'lpA'

Unlike a C string, Python strings cannot be changed.  Assigning to an  indexed
position in the string results in an error::

   >>> word[0] = 'x'
   Traceback (most recent call last):
     File "<stdin>", line 1, in ?
   TypeError: object doesn't support item assignment
   >>> word[:1] = 'Splat'
   Traceback (most recent call last):
     File "<stdin>", line 1, in ?
   TypeError: object doesn't support slice assignment

However, creating a new string with the combined content is easy and efficient::

   >>> 'x' + word[1:]
   'xelpA'
   >>> 'Splat' + word[4]
   'SplatA'

Here's a useful invariant of slice operations: ``s[:i] + s[i:]`` equals ``s``.
::

   >>> word[:2] + word[2:]
   'HelpA'
   >>> word[:3] + word[3:]
   'HelpA'

Degenerate slice indices are handled gracefully: an index that is too large is
replaced by the string size, an upper bound smaller than the lower bound returns
an empty string. ::

   >>> word[1:100]
   'elpA'
   >>> word[10:]
   ''
   >>> word[2:1]
   ''

Indices may be negative numbers, to start counting from the right. For example::

   >>> word[-1]     # The last character
   'A'
   >>> word[-2]     # The last-but-one character
   'p'
   >>> word[-2:]    # The last two characters
   'pA'
   >>> word[:-2]    # Everything except the last two characters
   'Hel'

But note that -0 is really the same as 0, so it does not count from the right!
::

   >>> word[-0]     # (since -0 equals 0)
   'H'

Out-of-range negative slice indices are truncated, but don't try this for
single-element (non-slice) indices::

   >>> word[-100:]
   'HelpA'
   >>> word[-10]    # error
   Traceback (most recent call last):
     File "<stdin>", line 1, in ?
   IndexError: string index out of range

One way to remember how slices work is to think of the indices as pointing
*between* characters, with the left edge of the first character numbered 0.
Then the right edge of the last character of a string of *n* characters has
index *n*, for example::

    +---+---+---+---+---+ 
    | H | e | l | p | A |
    +---+---+---+---+---+ 
    0   1   2   3   4   5 
   -5  -4  -3  -2  -1

The first row of numbers gives the position of the indices 0...5 in the string;
the second row gives the corresponding negative indices. The slice from *i* to
*j* consists of all characters between the edges labeled *i* and *j*,
respectively.

For non-negative indices, the length of a slice is the difference of the
indices, if both are within bounds.  For example, the length of ``word[1:3]`` is
2.

The built-in function :func:`len` returns the length of a string::

   >>> s = 'supercalifragilisticexpialidocious'
   >>> len(s)
   34


.. seealso::

   :ref:`typesseq`
      Strings, and the Unicode strings described in the next section, are
      examples of *sequence types*, and support the common operations supported
      by such types.

   :ref:`string-methods`
      Both strings and Unicode strings support a large number of methods for
      basic transformations and searching.

   :ref:`string-formatting`
      The formatting operations invoked when strings and Unicode strings are the
      left operand of the ``%`` operator are described in more detail here.


.. _tut-unicodestrings:

Unicode Strings
---------------

.. sectionauthor:: Marc-Andre Lemburg <mal@lemburg.com>


Starting with Python 2.0 a new data type for storing text data is available to
the programmer: the Unicode object. It can be used to store and manipulate
Unicode data (see http://www.unicode.org/) and integrates well with the existing
string objects, providing auto-conversions where necessary.

Unicode has the advantage of providing one ordinal for every character in every
script used in modern and ancient texts. Previously, there were only 256
possible ordinals for script characters. Texts were typically bound to a code
page which mapped the ordinals to script characters. This lead to very much
confusion especially with respect to internationalization (usually written as
``i18n`` --- ``'i'`` + 18 characters + ``'n'``) of software.  Unicode solves
these problems by defining one code page for all scripts.

Creating Unicode strings in Python is just as simple as creating normal
strings::

   >>> u'Hello World !'
   u'Hello World !'

The small ``'u'`` in front of the quote indicates that a Unicode string is
supposed to be created. If you want to include special characters in the string,
you can do so by using the Python *Unicode-Escape* encoding. The following
example shows how::

   >>> u'Hello\u0020World !'
   u'Hello World !'

The escape sequence ``\u0020`` indicates to insert the Unicode character with
the ordinal value 0x0020 (the space character) at the given position.

Other characters are interpreted by using their respective ordinal values
directly as Unicode ordinals.  If you have literal strings in the standard
Latin-1 encoding that is used in many Western countries, you will find it
convenient that the lower 256 characters of Unicode are the same as the 256
characters of Latin-1.

For experts, there is also a raw mode just like the one for normal strings. You
have to prefix the opening quote with 'ur' to have Python use the
*Raw-Unicode-Escape* encoding. It will only apply the above ``\uXXXX``
conversion if there is an uneven number of backslashes in front of the small
'u'. ::

   >>> ur'Hello\u0020World !'
   u'Hello World !'
   >>> ur'Hello\\u0020World !'
   u'Hello\\\\u0020World !'

The raw mode is most useful when you have to enter lots of backslashes, as can
be necessary in regular expressions.

Apart from these standard encodings, Python provides a whole set of other ways
of creating Unicode strings on the basis of a known encoding.

.. index:: builtin: unicode

The built-in function :func:`unicode` provides access to all registered Unicode
codecs (COders and DECoders). Some of the more well known encodings which these
codecs can convert are *Latin-1*, *ASCII*, *UTF-8*, and *UTF-16*. The latter two
are variable-length encodings that store each Unicode character in one or more
bytes. The default encoding is normally set to ASCII, which passes through
characters in the range 0 to 127 and rejects any other characters with an error.
When a Unicode string is printed, written to a file, or converted with
:func:`str`, conversion takes place using this default encoding. ::

   >>> u"abc"
   u'abc'
   >>> str(u"abc")
   'abc'
   >>> u"äöü"
   u'\xe4\xf6\xfc'
   >>> str(u"äöü")
   Traceback (most recent call last):
     File "<stdin>", line 1, in ?
   UnicodeEncodeError: 'ascii' codec can't encode characters in position 0-2: ordinal not in range(128)

To convert a Unicode string into an 8-bit string using a specific encoding,
Unicode objects provide an :func:`encode` method that takes one argument, the
name of the encoding.  Lowercase names for encodings are preferred. ::

   >>> u"äöü".encode('utf-8')
   '\xc3\xa4\xc3\xb6\xc3\xbc'

If you have data in a specific encoding and want to produce a corresponding
Unicode string from it, you can use the :func:`unicode` function with the
encoding name as the second argument. ::

   >>> unicode('\xc3\xa4\xc3\xb6\xc3\xbc', 'utf-8')
   u'\xe4\xf6\xfc'


.. _tut-lists:

Lists
-----

Python knows a number of *compound* data types, used to group together other
values.  The most versatile is the *list*, which can be written as a list of
comma-separated values (items) between square brackets.  List items need not all
have the same type. ::

   >>> a = ['spam', 'eggs', 100, 1234]
   >>> a
   ['spam', 'eggs', 100, 1234]

Like string indices, list indices start at 0, and lists can be sliced,
concatenated and so on::

   >>> a[0]
   'spam'
   >>> a[3]
   1234
   >>> a[-2]
   100
   >>> a[1:-1]
   ['eggs', 100]
   >>> a[:2] + ['bacon', 2*2]
   ['spam', 'eggs', 'bacon', 4]
   >>> 3*a[:3] + ['Boo!']
   ['spam', 'eggs', 100, 'spam', 'eggs', 100, 'spam', 'eggs', 100, 'Boo!']

Unlike strings, which are *immutable*, it is possible to change individual
elements of a list::

   >>> a
   ['spam', 'eggs', 100, 1234]
   >>> a[2] = a[2] + 23
   >>> a
   ['spam', 'eggs', 123, 1234]

Assignment to slices is also possible, and this can even change the size of the
list or clear it entirely::

   >>> # Replace some items:
   ... a[0:2] = [1, 12]
   >>> a
   [1, 12, 123, 1234]
   >>> # Remove some:
   ... a[0:2] = []
   >>> a
   [123, 1234]
   >>> # Insert some:
   ... a[1:1] = ['bletch', 'xyzzy']
   >>> a
   [123, 'bletch', 'xyzzy', 1234]
   >>> # Insert (a copy of) itself at the beginning
   >>> a[:0] = a
   >>> a
   [123, 'bletch', 'xyzzy', 1234, 123, 'bletch', 'xyzzy', 1234]
   >>> # Clear the list: replace all items with an empty list
   >>> a[:] = []
   >>> a
   []

The built-in function :func:`len` also applies to lists::

   >>> a = ['a', 'b', 'c', 'd']
   >>> len(a)
   4

It is possible to nest lists (create lists containing other lists), for
example::

   >>> q = [2, 3]
   >>> p = [1, q, 4]
   >>> len(p)
   3
   >>> p[1]
   [2, 3]
   >>> p[1][0]
   2
   >>> p[1].append('xtra')     # See section 5.1
   >>> p
   [1, [2, 3, 'xtra'], 4]
   >>> q
   [2, 3, 'xtra']

Note that in the last example, ``p[1]`` and ``q`` really refer to the same
object!  We'll come back to *object semantics* later.


.. _tut-firststeps:

First Steps Towards Programming
===============================

Of course, we can use Python for more complicated tasks than adding two and two
together.  For instance, we can write an initial sub-sequence of the *Fibonacci*
series as follows::

   >>> # Fibonacci series:
   ... # the sum of two elements defines the next
   ... a, b = 0, 1
   >>> while b < 10:
   ...       print b
   ...       a, b = b, a+b
   ... 
   1
   1
   2
   3
   5
   8

This example introduces several new features.

* The first line contains a *multiple assignment*: the variables ``a`` and ``b``
  simultaneously get the new values 0 and 1.  On the last line this is used again,
  demonstrating that the expressions on the right-hand side are all evaluated
  first before any of the assignments take place.  The right-hand side expressions
  are evaluated  from the left to the right.

* The :keyword:`while` loop executes as long as the condition (here: ``b < 10``)
  remains true.  In Python, like in C, any non-zero integer value is true; zero is
  false.  The condition may also be a string or list value, in fact any sequence;
  anything with a non-zero length is true, empty sequences are false.  The test
  used in the example is a simple comparison.  The standard comparison operators
  are written the same as in C: ``<`` (less than), ``>`` (greater than), ``==``
  (equal to), ``<=`` (less than or equal to), ``>=`` (greater than or equal to)
  and ``!=`` (not equal to).

* The *body* of the loop is *indented*: indentation is Python's way of grouping
  statements.  Python does not (yet!) provide an intelligent input line editing
  facility, so you have to type a tab or space(s) for each indented line.  In
  practice you will prepare more complicated input for Python with a text editor;
  most text editors have an auto-indent facility.  When a compound statement is
  entered interactively, it must be followed by a blank line to indicate
  completion (since the parser cannot guess when you have typed the last line).
  Note that each line within a basic block must be indented by the same amount.

* The :keyword:`print` statement writes the value of the expression(s) it is
  given.  It differs from just writing the expression you want to write (as we did
  earlier in the calculator examples) in the way it handles multiple expressions
  and strings.  Strings are printed without quotes, and a space is inserted
  between items, so you can format things nicely, like this::

     >>> i = 256*256
     >>> print 'The value of i is', i
     The value of i is 65536

  A trailing comma avoids the newline after the output::

     >>> a, b = 0, 1
     >>> while b < 1000:
     ...     print b,
     ...     a, b = b, a+b
     ... 
     1 1 2 3 5 8 13 21 34 55 89 144 233 377 610 987

  Note that the interpreter inserts a newline before it prints the next prompt if
  the last line was not completed.
