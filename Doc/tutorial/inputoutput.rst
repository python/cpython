.. _tut-io:

****************
Input and Output
****************

There are several ways to present the output of a program; data can be printed
in a human-readable form, or written to a file for future use. This chapter will
discuss some of the possibilities.


.. _tut-formatting:

Fancier Output Formatting
=========================

So far we've encountered two ways of writing values: *expression statements* and
the :keyword:`print` statement.  (A third way is using the :meth:`write` method
of file objects; the standard output file can be referenced as ``sys.stdout``.
See the Library Reference for more information on this.)

.. index:: module: string

Often you'll want more control over the formatting of your output than simply
printing space-separated values.  There are two ways to format your output; the
first way is to do all the string handling yourself; using string slicing and
concatenation operations you can create any layout you can imagine.  The
standard module :mod:`string` contains some useful operations for padding
strings to a given column width; these will be discussed shortly.  The second
way is to use the :meth:`str.format` method.

One question remains, of course: how do you convert values to strings? Luckily,
Python has ways to convert any value to a string: pass it to the :func:`repr`
or :func:`str` functions.

The :func:`str` function is meant to return representations of values which are
fairly human-readable, while :func:`repr` is meant to generate representations
which can be read by the interpreter (or will force a :exc:`SyntaxError` if
there is not equivalent syntax).  For objects which don't have a particular
representation for human consumption, :func:`str` will return the same value as
:func:`repr`.  Many values, such as numbers or structures like lists and
dictionaries, have the same representation using either function.  Strings and
floating point numbers, in particular, have two distinct representations.

Some examples::

   >>> s = 'Hello, world.'
   >>> str(s)
   'Hello, world.'
   >>> repr(s)
   "'Hello, world.'"
   >>> str(0.1)
   '0.1'
   >>> repr(0.1)
   '0.10000000000000001'
   >>> x = 10 * 3.25
   >>> y = 200 * 200
   >>> s = 'The value of x is ' + repr(x) + ', and y is ' + repr(y) + '...'
   >>> print s
   The value of x is 32.5, and y is 40000...
   >>> # The repr() of a string adds string quotes and backslashes:
   ... hello = 'hello, world\n'
   >>> hellos = repr(hello)
   >>> print hellos
   'hello, world\n'
   >>> # The argument to repr() may be any Python object:
   ... repr((x, y, ('spam', 'eggs')))
   "(32.5, 40000, ('spam', 'eggs'))"

Here are two ways to write a table of squares and cubes::

   >>> for x in range(1, 11):
   ...     print repr(x).rjust(2), repr(x*x).rjust(3),
   ...     # Note trailing comma on previous line
   ...     print repr(x*x*x).rjust(4)
   ...
    1   1    1
    2   4    8
    3   9   27
    4  16   64
    5  25  125
    6  36  216
    7  49  343
    8  64  512
    9  81  729
   10 100 1000

   >>> for x in range(1,11):
   ...     print '{0:2d} {1:3d} {2:4d}'.format(x, x*x, x*x*x)
   ...
    1   1    1
    2   4    8
    3   9   27
    4  16   64
    5  25  125
    6  36  216
    7  49  343
    8  64  512
    9  81  729
   10 100 1000

(Note that in the first example, one space between each column was added by the
way :keyword:`print` works: it always adds spaces between its arguments.)

This example demonstrates the :meth:`rjust` method of string objects, which
right-justifies a string in a field of a given width by padding it with spaces
on the left.  There are similar methods :meth:`ljust` and :meth:`center`.  These
methods do not write anything, they just return a new string.  If the input
string is too long, they don't truncate it, but return it unchanged; this will
mess up your column lay-out but that's usually better than the alternative,
which would be lying about a value.  (If you really want truncation you can
always add a slice operation, as in ``x.ljust(n)[:n]``.)

There is another method, :meth:`zfill`, which pads a numeric string on the left
with zeros.  It understands about plus and minus signs::

   >>> '12'.zfill(5)
   '00012'
   >>> '-3.14'.zfill(7)
   '-003.14'
   >>> '3.14159265359'.zfill(5)
   '3.14159265359'

Basic usage of the :meth:`str.format` method looks like this::

   >>> print 'We are the {0} who say "{1}!"'.format('knights', 'Ni')
   We are the knights who say "Ni!"

The brackets and characters within them (called format fields) are replaced with
the objects passed into the format method.  The number in the brackets refers to
the position of the object passed into the format method. ::

   >>> print '{0} and {1}'.format('spam', 'eggs')
   spam and eggs
   >>> print '{1} and {0}'.format('spam', 'eggs')
   eggs and spam

If keyword arguments are used in the format method, their values are referred to
by using the name of the argument. ::

   >>> print 'This {food} is {adjective}.'.format(
   ...       food='spam', adjective='absolutely horrible')
   This spam is absolutely horrible.

Positional and keyword arguments can be arbitrarily combined::

   >>> print 'The story of {0}, {1}, and {other}.'.format('Bill', 'Manfred',
   ...                                                    other='Georg')
   The story of Bill, Manfred, and Georg.

An optional ``':'`` and format specifier can follow the field name. This also
greater control over how the value is formatted.  The following example
truncates the Pi to three places after the decimal.

   >>> import math
   >>> print 'The value of PI is approximately {0:.3f}.'.format(math.pi)
   The value of PI is approximately 3.142.

Passing an integer after the ``':'`` will cause that field to be a minimum
number of characters wide.  This is useful for making tables pretty.::

   >>> table = {'Sjoerd': 4127, 'Jack': 4098, 'Dcab': 7678}
   >>> for name, phone in table.items():
   ...     print '{0:10} ==> {1:10d}'.format(name, phone)
   ...
   Jack       ==>       4098
   Dcab       ==>       7678
   Sjoerd     ==>       4127

If you have a really long format string that you don't want to split up, it
would be nice if you could reference the variables to be formatted by name
instead of by position.  This can be done by simply passing the dict and using
square brackets ``'[]'`` to access the keys ::

   >>> table = {'Sjoerd': 4127, 'Jack': 4098, 'Dcab': 8637678}
   >>> print ('Jack: {0[Jack]:d}; Sjoerd: {0[Sjoerd]:d}; '
   ...        'Dcab: {0[Dcab]:d}'.format(table))
   Jack: 4098; Sjoerd: 4127; Dcab: 8637678

This could also be done by passing the table as keyword arguments with the '**'
notation.::

   >>> table = {'Sjoerd': 4127, 'Jack': 4098, 'Dcab': 8637678}
   >>> print 'Jack: {Jack:d}; Sjoerd: {Sjoerd:d}; Dcab: {Dcab:d}'.format(**table)
   Jack: 4098; Sjoerd: 4127; Dcab: 8637678

This is particularly useful in combination with the new built-in :func:`vars`
function, which returns a dictionary containing all local variables.

For a complete overview of string formatting with :meth:`str.format`, see
:ref:`formatstrings`.


Old string formatting
---------------------

The ``%`` operator can also be used for string formatting. It interprets the
left argument much like a :cfunc:`sprintf`\ -style format string to be applied
to the right argument, and returns the string resulting from this formatting
operation. For example::

   >>> import math
   >>> print 'The value of PI is approximately %5.3f.' % math.pi
   The value of PI is approximately 3.142.

Since :meth:`str.format` is quite new, a lot of Python code still uses the ``%``
operator. However, because this old style of formatting will eventually removed
from the language :meth:`str.format` should generally be used.

More information can be found in the :ref:`string-formatting` section.


.. _tut-files:

Reading and Writing Files
=========================

.. index::
   builtin: open
   object: file

:func:`open` returns a file object, and is most commonly used with two
arguments: ``open(filename, mode)``.

::

   >>> f = open('/tmp/workfile', 'w')
   >>> print f
   <open file '/tmp/workfile', mode 'w' at 80a0960>

The first argument is a string containing the filename.  The second argument is
another string containing a few characters describing the way in which the file
will be used.  *mode* can be ``'r'`` when the file will only be read, ``'w'``
for only writing (an existing file with the same name will be erased), and
``'a'`` opens the file for appending; any data written to the file is
automatically added to the end.  ``'r+'`` opens the file for both reading and
writing. The *mode* argument is optional; ``'r'`` will be assumed if it's
omitted.

On Windows, ``'b'`` appended to the mode opens the file in binary mode, so there
are also modes like ``'rb'``, ``'wb'``, and ``'r+b'``.  Windows makes a
distinction between text and binary files; the end-of-line characters in text
files are automatically altered slightly when data is read or written.  This
behind-the-scenes modification to file data is fine for ASCII text files, but
it'll corrupt binary data like that in :file:`JPEG` or :file:`EXE` files.  Be
very careful to use binary mode when reading and writing such files.  On Unix,
it doesn't hurt to append a ``'b'`` to the mode, so you can use it
platform-independently for all binary files.


.. _tut-filemethods:

Methods of File Objects
-----------------------

The rest of the examples in this section will assume that a file object called
``f`` has already been created.

To read a file's contents, call ``f.read(size)``, which reads some quantity of
data and returns it as a string.  *size* is an optional numeric argument.  When
*size* is omitted or negative, the entire contents of the file will be read and
returned; it's your problem if the file is twice as large as your machine's
memory. Otherwise, at most *size* bytes are read and returned.  If the end of
the file has been reached, ``f.read()`` will return an empty string (``""``).
::

   >>> f.read()
   'This is the entire file.\n'
   >>> f.read()
   ''

``f.readline()`` reads a single line from the file; a newline character (``\n``)
is left at the end of the string, and is only omitted on the last line of the
file if the file doesn't end in a newline.  This makes the return value
unambiguous; if ``f.readline()`` returns an empty string, the end of the file
has been reached, while a blank line is represented by ``'\n'``, a string
containing only a single newline.   ::

   >>> f.readline()
   'This is the first line of the file.\n'
   >>> f.readline()
   'Second line of the file\n'
   >>> f.readline()
   ''

``f.readlines()`` returns a list containing all the lines of data in the file.
If given an optional parameter *sizehint*, it reads that many bytes from the
file and enough more to complete a line, and returns the lines from that.  This
is often used to allow efficient reading of a large file by lines, but without
having to load the entire file in memory.  Only complete lines will be returned.
::

   >>> f.readlines()
   ['This is the first line of the file.\n', 'Second line of the file\n']

An alternative approach to reading lines is to loop over the file object. This is
memory efficient, fast, and leads to simpler code::

   >>> for line in f:
           print line,

   This is the first line of the file.
   Second line of the file

The alternative approach is simpler but does not provide as fine-grained
control.  Since the two approaches manage line buffering differently, they
should not be mixed.

``f.write(string)`` writes the contents of *string* to the file, returning
``None``.   ::

   >>> f.write('This is a test\n')

To write something other than a string, it needs to be converted to a string
first::

   >>> value = ('the answer', 42)
   >>> s = str(value)
   >>> f.write(s)

``f.tell()`` returns an integer giving the file object's current position in the
file, measured in bytes from the beginning of the file.  To change the file
object's position, use ``f.seek(offset, from_what)``.  The position is computed
from adding *offset* to a reference point; the reference point is selected by
the *from_what* argument.  A *from_what* value of 0 measures from the beginning
of the file, 1 uses the current file position, and 2 uses the end of the file as
the reference point.  *from_what* can be omitted and defaults to 0, using the
beginning of the file as the reference point. ::

   >>> f = open('/tmp/workfile', 'r+')
   >>> f.write('0123456789abcdef')
   >>> f.seek(5)     # Go to the 6th byte in the file
   >>> f.read(1)
   '5'
   >>> f.seek(-3, 2) # Go to the 3rd byte before the end
   >>> f.read(1)
   'd'

When you're done with a file, call ``f.close()`` to close it and free up any
system resources taken up by the open file.  After calling ``f.close()``,
attempts to use the file object will automatically fail. ::

   >>> f.close()
   >>> f.read()
   Traceback (most recent call last):
     File "<stdin>", line 1, in ?
   ValueError: I/O operation on closed file

It is good practice to use the :keyword:`with` keyword when dealing with file
objects.  This has the advantage that the file is properly closed after its
suite finishes, even if an exception is raised on the way.  It is also much
shorter than writing equivalent :keyword:`try`\ -\ :keyword:`finally` blocks::

    >>> with open('/tmp/workfile', 'r') as f:
    ...     read_data = f.read()
    >>> f.closed
    True

File objects have some additional methods, such as :meth:`isatty` and
:meth:`truncate` which are less frequently used; consult the Library Reference
for a complete guide to file objects.


.. _tut-pickle:

The :mod:`pickle` Module
------------------------

.. index:: module: pickle

Strings can easily be written to and read from a file. Numbers take a bit more
effort, since the :meth:`read` method only returns strings, which will have to
be passed to a function like :func:`int`, which takes a string like ``'123'``
and returns its numeric value 123.  However, when you want to save more complex
data types like lists, dictionaries, or class instances, things get a lot more
complicated.

Rather than have users be constantly writing and debugging code to save
complicated data types, Python provides a standard module called :mod:`pickle`.
This is an amazing module that can take almost any Python object (even some
forms of Python code!), and convert it to a string representation; this process
is called :dfn:`pickling`.  Reconstructing the object from the string
representation is called :dfn:`unpickling`.  Between pickling and unpickling,
the string representing the object may have been stored in a file or data, or
sent over a network connection to some distant machine.

If you have an object ``x``, and a file object ``f`` that's been opened for
writing, the simplest way to pickle the object takes only one line of code::

   pickle.dump(x, f)

To unpickle the object again, if ``f`` is a file object which has been opened
for reading::

   x = pickle.load(f)

(There are other variants of this, used when pickling many objects or when you
don't want to write the pickled data to a file; consult the complete
documentation for :mod:`pickle` in the Python Library Reference.)

:mod:`pickle` is the standard way to make Python objects which can be stored and
reused by other programs or by a future invocation of the same program; the
technical term for this is a :dfn:`persistent` object.  Because :mod:`pickle` is
so widely used, many authors who write Python extensions take care to ensure
that new data types such as matrices can be properly pickled and unpickled.


