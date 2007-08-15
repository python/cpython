
.. _built-in-funcs:

Built-in Functions
==================

The Python interpreter has a number of functions built into it that are always
available.  They are listed here in alphabetical order.


.. function:: __import__(name[, globals[, locals[, fromlist[, level]]]])

   .. index::
      statement: import
      module: ihooks
      module: rexec
      module: imp

   .. note::

      This is an advanced function that is not needed in everyday Python
      programming.

   The function is invoked by the :keyword:`import` statement.  It mainly exists
   so that you can replace it with another function that has a compatible
   interface, in order to change the semantics of the :keyword:`import` statement.
   For examples of why and how you would do this, see the standard library modules
   :mod:`ihooks` and :mod:`rexec`.  See also the built-in module :mod:`imp`, which
   defines some useful operations out of which you can build your own
   :func:`__import__` function.

   For example, the statement ``import spam`` results in the following call:
   ``__import__('spam',`` ``globals(),`` ``locals(), [], -1)``; the statement
   ``from spam.ham import eggs`` results in ``__import__('spam.ham', globals(),
   locals(), ['eggs'], -1)``.  Note that even though ``locals()`` and ``['eggs']``
   are passed in as arguments, the :func:`__import__` function does not set the
   local variable named ``eggs``; this is done by subsequent code that is generated
   for the import statement.  (In fact, the standard implementation does not use
   its *locals* argument at all, and uses its *globals* only to determine the
   package context of the :keyword:`import` statement.)

   When the *name* variable is of the form ``package.module``, normally, the
   top-level package (the name up till the first dot) is returned, *not* the
   module named by *name*.  However, when a non-empty *fromlist* argument is
   given, the module named by *name* is returned.  This is done for
   compatibility with the bytecode generated for the different kinds of import
   statement; when using ``import spam.ham.eggs``, the top-level package
   :mod:`spam` must be placed in the importing namespace, but when using ``from
   spam.ham import eggs``, the ``spam.ham`` subpackage must be used to find the
   ``eggs`` variable.  As a workaround for this behavior, use :func:`getattr` to
   extract the desired components.  For example, you could define the following
   helper::

      def my_import(name):
          mod = __import__(name)
          components = name.split('.')
          for comp in components[1:]:
              mod = getattr(mod, comp)
          return mod

   *level* specifies whether to use absolute or relative imports. The default is
   ``-1`` which indicates both absolute and relative imports will be attempted.
   ``0`` means only perform absolute imports. Positive values for *level* indicate
   the number of parent directories to search relative to the directory of the
   module calling :func:`__import__`.

   .. versionchanged:: 2.5
      The level parameter was added.

   .. versionchanged:: 2.5
      Keyword support for parameters was added.


.. function:: abs(x)

   Return the absolute value of a number.  The argument may be a plain or long
   integer or a floating point number.  If the argument is a complex number, its
   magnitude is returned.


.. function:: all(iterable)

   Return True if all elements of the *iterable* are true. Equivalent to::

      def all(iterable):
          for element in iterable:
              if not element:
                  return False
          return True

   .. versionadded:: 2.5


.. function:: any(iterable)

   Return True if any element of the *iterable* is true. Equivalent to::

      def any(iterable):
          for element in iterable:
              if element:
                  return True
          return False

   .. versionadded:: 2.5


.. function:: basestring()

   This abstract type is the superclass for :class:`str`.  It
   cannot be called or instantiated, but it can be used to test whether an object
   is an instance of :class:`str` (or a user-defined type inherited from
   :class:`basestring`).

   .. versionadded:: 2.3


.. function:: bin(x)

   Convert an integer number to a binary string. The result is a valid Python
   expression.  If *x* is not a Python :class:`int` object, it has to define an
   :meth:`__index__` method that returns an integer.

   .. versionadded:: 3.0


.. function:: bool([x])

   Convert a value to a Boolean, using the standard truth testing procedure.  If
   *x* is false or omitted, this returns :const:`False`; otherwise it returns
   :const:`True`. :class:`bool` is also a class, which is a subclass of
   :class:`int`. Class :class:`bool` cannot be subclassed further.  Its only
   instances are :const:`False` and :const:`True`.

   .. index:: pair: Boolean; type

   .. versionadded:: 2.2.1

   .. versionchanged:: 2.3
      If no argument is given, this function returns :const:`False`.


.. function:: chr(i)

   Return the string of one character whose Unicode codepoint is the integer *i*.  For
   example, ``chr(97)`` returns the string ``'a'``. This is the inverse of
   :func:`ord`.  The valid range for the argument depends how Python was
   configured -- it may be either UCS2 [0..0xFFFF] or UCS4 [0..0x10FFFF].
   :exc:`ValueError` will be raised if *i* is outside that range.


.. function:: classmethod(function)

   Return a class method for *function*.

   A class method receives the class as implicit first argument, just like an
   instance method receives the instance. To declare a class method, use this
   idiom::

      class C:
          @classmethod
          def f(cls, arg1, arg2, ...): ...

   The ``@classmethod`` form is a function decorator -- see the description of
   function definitions in :ref:`function` for details.

   It can be called either on the class (such as ``C.f()``) or on an instance (such
   as ``C().f()``).  The instance is ignored except for its class. If a class
   method is called for a derived class, the derived class object is passed as the
   implied first argument.

   Class methods are different than C++ or Java static methods. If you want those,
   see :func:`staticmethod` in this section.

   For more information on class methods, consult the documentation on the standard
   type hierarchy in :ref:`types`.

   .. versionadded:: 2.2

   .. versionchanged:: 2.4
      Function decorator syntax added.


.. function:: cmp(x, y)

   Compare the two objects *x* and *y* and return an integer according to the
   outcome.  The return value is negative if ``x < y``, zero if ``x == y`` and
   strictly positive if ``x > y``.


.. function:: compile(source, filename, mode[, flags[, dont_inherit]])

   Compile the *source* into a code object.  Code objects can be executed by a call
   to :func:`exec` or evaluated by a call to :func:`eval`.  The *filename* argument
   should give the file from which the code was read; pass some recognizable value
   if it wasn't read from a file (``'<string>'`` is commonly used). The *mode*
   argument specifies what kind of code must be compiled; it can be ``'exec'`` if
   *source* consists of a sequence of statements, ``'eval'`` if it consists of a
   single expression, or ``'single'`` if it consists of a single interactive
   statement (in the latter case, expression statements that evaluate to something
   else than ``None`` will be printed).

   When compiling multi-line statements, two caveats apply: line endings must be
   represented by a single newline character (``'\n'``), and the input must be
   terminated by at least one newline character.  If line endings are represented
   by ``'\r\n'``, use the string :meth:`replace` method to change them into
   ``'\n'``.

   The optional arguments *flags* and *dont_inherit* (which are new in Python 2.2)
   control which future statements (see :pep:`236`) affect the compilation of
   *source*.  If neither is present (or both are zero) the code is compiled with
   those future statements that are in effect in the code that is calling compile.
   If the *flags* argument is given and *dont_inherit* is not (or is zero) then the
   future statements specified by the *flags* argument are used in addition to
   those that would be used anyway. If *dont_inherit* is a non-zero integer then
   the *flags* argument is it -- the future statements in effect around the call to
   compile are ignored.

   Future statements are specified by bits which can be bitwise or-ed together to
   specify multiple statements.  The bitfield required to specify a given feature
   can be found as the :attr:`compiler_flag` attribute on the :class:`_Feature`
   instance in the :mod:`__future__` module.


.. function:: complex([real[, imag]])

   Create a complex number with the value *real* + *imag*\*j or convert a string or
   number to a complex number.  If the first parameter is a string, it will be
   interpreted as a complex number and the function must be called without a second
   parameter.  The second parameter can never be a string. Each argument may be any
   numeric type (including complex). If *imag* is omitted, it defaults to zero and
   the function serves as a numeric conversion function like :func:`int`,
   :func:`long` and :func:`float`.  If both arguments are omitted, returns ``0j``.

   The complex type is described in :ref:`typesnumeric`.


.. function:: delattr(object, name)

   This is a relative of :func:`setattr`.  The arguments are an object and a
   string.  The string must be the name of one of the object's attributes.  The
   function deletes the named attribute, provided the object allows it.  For
   example, ``delattr(x, 'foobar')`` is equivalent to ``del x.foobar``.


.. function:: dict([arg])
   :noindex:

   Create a new data dictionary, optionally with items taken from *arg*.
   The dictionary type is described in :ref:`typesmapping`.

   For other containers see the built in :class:`list`, :class:`set`, and
   :class:`tuple` classes, and the :mod:`collections` module.


.. function:: dir([object])

   Without arguments, return the list of names in the current local scope.  With an
   argument, attempt to return a list of valid attributes for that object.

   If the object has a method named :meth:`__dir__`, this method will be called and
   must return the list of attributes. This allows objects that implement a custom
   :func:`__getattr__` or :func:`__getattribute__` function to customize the way
   :func:`dir` reports their attributes.

   If the object does not provide :meth:`__dir__`, the function tries its best to
   gather information from the object's :attr:`__dict__` attribute, if defined, and
   from its type object.  The resulting list is not necessarily complete, and may
   be inaccurate when the object has a custom :func:`__getattr__`.

   The default :func:`dir` mechanism behaves differently with different types of
   objects, as it attempts to produce the most relevant, rather than complete,
   information:

   * If the object is a module object, the list contains the names of the module's
     attributes.

   * If the object is a type or class object, the list contains the names of its
     attributes, and recursively of the attributes of its bases.

   * Otherwise, the list contains the object's attributes' names, the names of its
     class's attributes, and recursively of the attributes of its class's base
     classes.

   The resulting list is sorted alphabetically.  For example::

      >>> import struct
      >>> dir()
      ['__builtins__', '__doc__', '__name__', 'struct']
      >>> dir(struct)
      ['__doc__', '__name__', 'calcsize', 'error', 'pack', 'unpack']
      >>> class Foo(object):
      ...     def __dir__(self):
      ...         return ["kan", "ga", "roo"]
      ...
      >>> f = Foo()
      >>> dir(f)
      ['ga', 'kan', 'roo']

   .. note::

      Because :func:`dir` is supplied primarily as a convenience for use at an
      interactive prompt, it tries to supply an interesting set of names more than it
      tries to supply a rigorously or consistently defined set of names, and its
      detailed behavior may change across releases.


.. function:: divmod(a, b)

   Take two (non complex) numbers as arguments and return a pair of numbers
   consisting of their quotient and remainder when using long division.  With mixed
   operand types, the rules for binary arithmetic operators apply.  For plain and
   long integers, the result is the same as ``(a // b, a % b)``. For floating point
   numbers the result is ``(q, a % b)``, where *q* is usually ``math.floor(a / b)``
   but may be 1 less than that.  In any case ``q * b + a % b`` is very close to
   *a*, if ``a % b`` is non-zero it has the same sign as *b*, and ``0 <= abs(a % b)
   < abs(b)``.

   .. versionchanged:: 2.3
      Using :func:`divmod` with complex numbers is deprecated.


.. function:: enumerate(iterable)

   Return an enumerate object. *iterable* must be a sequence, an iterator, or some
   other object which supports iteration.  The :meth:`__next__` method of the
   iterator returned by :func:`enumerate` returns a tuple containing a count (from
   zero) and the corresponding value obtained from iterating over *iterable*.
   :func:`enumerate` is useful for obtaining an indexed series: ``(0, seq[0])``,
   ``(1, seq[1])``, ``(2, seq[2])``, .... For example::

      >>> for i, season in enumerate(['Spring', 'Summer', 'Fall', 'Winter')]:
      >>>     print i, season
      0 Spring
      1 Summer
      2 Fall
      3 Winter

   .. versionadded:: 2.3


.. function:: eval(expression[, globals[, locals]])

   The arguments are a string and optional globals and locals.  If provided,
   *globals* must be a dictionary.  If provided, *locals* can be any mapping
   object.

   .. versionchanged:: 2.4
      formerly *locals* was required to be a dictionary.

   The *expression* argument is parsed and evaluated as a Python expression
   (technically speaking, a condition list) using the *globals* and *locals*
   dictionaries as global and local name space.  If the *globals* dictionary is
   present and lacks '__builtins__', the current globals are copied into *globals*
   before *expression* is parsed.  This means that *expression* normally has full
   access to the standard :mod:`__builtin__` module and restricted environments are
   propagated.  If the *locals* dictionary is omitted it defaults to the *globals*
   dictionary.  If both dictionaries are omitted, the expression is executed in the
   environment where :keyword:`eval` is called.  The return value is the result of
   the evaluated expression. Syntax errors are reported as exceptions.  Example::

      >>> x = 1
      >>> print eval('x+1')
      2

   This function can also be used to execute arbitrary code objects (such as those
   created by :func:`compile`).  In this case pass a code object instead of a
   string.  The code object must have been compiled passing ``'eval'`` as the
   *kind* argument.

   Hints: dynamic execution of statements is supported by the :func:`exec`
   function.  The :func:`globals` and :func:`locals` functions
   returns the current global and local dictionary, respectively, which may be
   useful to pass around for use by :func:`eval` or :func:`exec`.


.. function:: exec(object[, globals[, locals]])

   This function supports dynamic execution of Python code. *object* must be either
   a string, an open file object, or a code object.  If it is a string, the string
   is parsed as a suite of Python statements which is then executed (unless a
   syntax error occurs).  If it is an open file, the file is parsed until EOF and
   executed.  If it is a code object, it is simply executed.  In all cases, the
   code that's executed is expected to be valid as file input (see the section
   "File input" in the Reference Manual). Be aware that the :keyword:`return` and
   :keyword:`yield` statements may not be used outside of function definitions even
   within the context of code passed to the :func:`exec` function. The return value
   is ``None``.

   In all cases, if the optional parts are omitted, the code is executed in the
   current scope.  If only *globals* is provided, it must be a dictionary, which
   will be used for both the global and the local variables.  If *globals* and
   *locals* are given, they are used for the global and local variables,
   respectively.  If provided, *locals* can be any mapping object.

   If the *globals* dictionary does not contain a value for the key
   ``__builtins__``, a reference to the dictionary of the built-in module
   :mod:`__builtin__` is inserted under that key.  That way you can control what
   builtins are available to the executed code by inserting your own
   ``__builtins__`` dictionary into *globals* before passing it to :func:`exec`.

   .. note::

      The built-in functions :func:`globals` and :func:`locals` return the current
      global and local dictionary, respectively, which may be useful to pass around
      for use as the second and third argument to :func:`exec`.

   .. warning::

      The default *locals* act as described for function :func:`locals` below:
      modifications to the default *locals* dictionary should not be attempted.  Pass
      an explicit *locals* dictionary if you need to see effects of the code on
      *locals* after function :func:`execfile` returns.  :func:`exec` cannot be
      used reliably to modify a function's locals.


.. function:: filter(function, iterable)

   Construct a list from those elements of *iterable* for which *function* returns
   true.  *iterable* may be either a sequence, a container which supports
   iteration, or an iterator,  If *iterable* is a string or a tuple, the result
   also has that type; otherwise it is always a list.  If *function* is ``None``,
   the identity function is assumed, that is, all elements of *iterable* that are
   false are removed.

   Note that ``filter(function, iterable)`` is equivalent to ``[item for item in
   iterable if function(item)]`` if function is not ``None`` and ``[item for item
   in iterable if item]`` if function is ``None``.


.. function:: float([x])

   Convert a string or a number to floating point.  If the argument is a string, it
   must contain a possibly signed decimal or floating point number, possibly
   embedded in whitespace. Otherwise, the argument may be a plain or long integer
   or a floating point number, and a floating point number with the same value
   (within Python's floating point precision) is returned.  If no argument is
   given, returns ``0.0``.

   .. note::

      .. index::
         single: NaN
         single: Infinity

      When passing in a string, values for NaN and Infinity may be returned, depending
      on the underlying C library.  The specific set of strings accepted which cause
      these values to be returned depends entirely on the C library and is known to
      vary.

   The float type is described in :ref:`typesnumeric`.

.. function:: frozenset([iterable])
   :noindex:

   Return a frozenset object, optionally with elements taken from *iterable*.
   The frozenset type is described in :ref:`types-set`.

   For other containers see the built in :class:`dict`, :class:`list`, and
   :class:`tuple` classes, and the :mod:`collections` module.

   .. versionadded:: 2.4


.. function:: getattr(object, name[, default])

   Return the value of the named attributed of *object*.  *name* must be a string.
   If the string is the name of one of the object's attributes, the result is the
   value of that attribute.  For example, ``getattr(x, 'foobar')`` is equivalent to
   ``x.foobar``.  If the named attribute does not exist, *default* is returned if
   provided, otherwise :exc:`AttributeError` is raised.


.. function:: globals()

   Return a dictionary representing the current global symbol table. This is always
   the dictionary of the current module (inside a function or method, this is the
   module where it is defined, not the module from which it is called).


.. function:: hasattr(object, name)

   The arguments are an object and a string.  The result is ``True`` if the string
   is the name of one of the object's attributes, ``False`` if not. (This is
   implemented by calling ``getattr(object, name)`` and seeing whether it raises an
   exception or not.)


.. function:: hash(object)

   Return the hash value of the object (if it has one).  Hash values are integers.
   They are used to quickly compare dictionary keys during a dictionary lookup.
   Numeric values that compare equal have the same hash value (even if they are of
   different types, as is the case for 1 and 1.0).


.. function:: help([object])

   Invoke the built-in help system.  (This function is intended for interactive
   use.)  If no argument is given, the interactive help system starts on the
   interpreter console.  If the argument is a string, then the string is looked up
   as the name of a module, function, class, method, keyword, or documentation
   topic, and a help page is printed on the console.  If the argument is any other
   kind of object, a help page on the object is generated.

   .. versionadded:: 2.2


.. function:: hex(x)

   Convert an integer number to a hexadecimal string. The result is a valid Python
   expression.  If *x* is not a Python :class:`int` object, it has to define an
   :meth:`__index__` method that returns an integer.

   .. versionchanged:: 2.4
      Formerly only returned an unsigned literal.


.. function:: id(object)

   Return the "identity" of an object.  This is an integer (or long integer) which
   is guaranteed to be unique and constant for this object during its lifetime.
   Two objects with non-overlapping lifetimes may have the same :func:`id` value.
   (Implementation note: this is the address of the object.)


.. function:: int([x[, radix]])

   Convert a string or number to an integer.  If the argument is a string, it
   must contain a possibly signed number of arbitrary size,
   possibly embedded in whitespace.  The *radix* parameter gives the base for the
   conversion and may be any integer in the range [2, 36], or zero.  If *radix* is
   zero, the interpretation is the same as for integer literals.  If *radix* is
   specified and *x* is not a string, :exc:`TypeError` is raised.  Otherwise, the
   argument may be another integer, a floating point number or any other object
   that has an :meth:`__int__` method.  Conversion
   of floating point numbers to integers truncates (towards zero).  If no
   arguments are given, returns ``0``.

   The integer type is described in :ref:`typesnumeric`.


.. function:: isinstance(object, classinfo)

   Return true if the *object* argument is an instance of the *classinfo* argument,
   or of a (direct or indirect) subclass thereof.  Also return true if *classinfo*
   is a type object (new-style class) and *object* is an object of that type or of
   a (direct or indirect) subclass thereof.  If *object* is not a class instance or
   an object of the given type, the function always returns false.  If *classinfo*
   is neither a class object nor a type object, it may be a tuple of class or type
   objects, or may recursively contain other such tuples (other sequence types are
   not accepted).  If *classinfo* is not a class, type, or tuple of classes, types,
   and such tuples, a :exc:`TypeError` exception is raised.

   .. versionchanged:: 2.2
      Support for a tuple of type information was added.


.. function:: issubclass(class, classinfo)

   Return true if *class* is a subclass (direct or indirect) of *classinfo*.  A
   class is considered a subclass of itself. *classinfo* may be a tuple of class
   objects, in which case every entry in *classinfo* will be checked. In any other
   case, a :exc:`TypeError` exception is raised.

   .. versionchanged:: 2.3
      Support for a tuple of type information was added.


.. function:: iter(o[, sentinel])

   Return an iterator object.  The first argument is interpreted very differently
   depending on the presence of the second argument. Without a second argument, *o*
   must be a collection object which supports the iteration protocol (the
   :meth:`__iter__` method), or it must support the sequence protocol (the
   :meth:`__getitem__` method with integer arguments starting at ``0``).  If it
   does not support either of those protocols, :exc:`TypeError` is raised. If the
   second argument, *sentinel*, is given, then *o* must be a callable object.  The
   iterator created in this case will call *o* with no arguments for each call to
   its :meth:`__next__` method; if the value returned is equal to *sentinel*,
   :exc:`StopIteration` will be raised, otherwise the value will be returned.

   .. versionadded:: 2.2


.. function:: len(s)

   Return the length (the number of items) of an object.  The argument may be a
   sequence (string, tuple or list) or a mapping (dictionary).


.. function:: list([iterable])

   Return a list whose items are the same and in the same order as *iterable*'s
   items.  *iterable* may be either a sequence, a container that supports
   iteration, or an iterator object.  If *iterable* is already a list, a copy is
   made and returned, similar to ``iterable[:]``.  For instance, ``list('abc')``
   returns ``['a', 'b', 'c']`` and ``list( (1, 2, 3) )`` returns ``[1, 2, 3]``.  If
   no argument is given, returns a new empty list, ``[]``.

   :class:`list` is a mutable sequence type, as documented in
   :ref:`typesseq`. For other containers see the built in :class:`dict`,
   :class:`set`, and :class:`tuple` classes, and the :mod:`collections` module.


.. function:: locals()

   Update and return a dictionary representing the current local symbol table.

   .. warning::

      The contents of this dictionary should not be modified; changes may not affect
      the values of local variables used by the interpreter.

   Free variables are returned by *locals* when it is called in a function block.
   Modifications of free variables may not affect the values used by the
   interpreter.  Free variables are not returned in class blocks.


.. function:: map(function, iterable, ...)

   Apply *function* to every item of *iterable* and return a list of the results.
   If additional *iterable* arguments are passed, *function* must take that many
   arguments and is applied to the items from all iterables in parallel.  If one
   iterable is shorter than another it is assumed to be extended with ``None``
   items.  If *function* is ``None``, the identity function is assumed; if there
   are multiple arguments, :func:`map` returns a list consisting of tuples
   containing the corresponding items from all iterables (a kind of transpose
   operation).  The *iterable* arguments may be a sequence  or any iterable object;
   the result is always a list.


.. function:: max(iterable[, args...][key])

   With a single argument *iterable*, return the largest item of a non-empty
   iterable (such as a string, tuple or list).  With more than one argument, return
   the largest of the arguments.

   The optional *key* argument specifies a one-argument ordering function like that
   used for :meth:`list.sort`.  The *key* argument, if supplied, must be in keyword
   form (for example, ``max(a,b,c,key=func)``).

   .. versionchanged:: 2.5
      Added support for the optional *key* argument.


.. function:: min(iterable[, args...][key])

   With a single argument *iterable*, return the smallest item of a non-empty
   iterable (such as a string, tuple or list).  With more than one argument, return
   the smallest of the arguments.

   The optional *key* argument specifies a one-argument ordering function like that
   used for :meth:`list.sort`.  The *key* argument, if supplied, must be in keyword
   form (for example, ``min(a,b,c,key=func)``).

   .. versionchanged:: 2.5
      Added support for the optional *key* argument.


.. function:: next(iterator[, default])

   Retrieve the next item from the *iterable* by calling its :meth:`__next__`
   method.  If *default* is given, it is returned if the iterator is exhausted,
   otherwise :exc:`StopIteration` is raised.


.. function:: object()

   Return a new featureless object.  :class:`object` is a base for all new style
   classes.  It has the methods that are common to all instances of new style
   classes.

   .. versionadded:: 2.2

   .. versionchanged:: 2.3
      This function does not accept any arguments. Formerly, it accepted arguments but
      ignored them.


.. function:: oct(x)

   Convert an integer number to an octal string.  The result is a valid Python
   expression.  If *x* is not a Python :class:`int` object, it has to define an
   :meth:`__index__` method that returns an integer.

   .. versionchanged:: 2.4
      Formerly only returned an unsigned literal.


.. function:: open(filename[, mode[, bufsize]])

   Open a file, returning an object of the :class:`file` type described in
   section :ref:`bltin-file-objects`.  If the file cannot be opened,
   :exc:`IOError` is raised.  When opening a file, it's preferable to use
   :func:`open` instead of invoking the :class:`file` constructor directly.

   The first two arguments are the same as for ``stdio``'s :cfunc:`fopen`:
   *filename* is the file name to be opened, and *mode* is a string indicating how
   the file is to be opened.

   The most commonly-used values of *mode* are ``'r'`` for reading, ``'w'`` for
   writing (truncating the file if it already exists), and ``'a'`` for appending
   (which on *some* Unix systems means that *all* writes append to the end of the
   file regardless of the current seek position).  If *mode* is omitted, it
   defaults to ``'r'``.  When opening a binary file, you should append ``'b'`` to
   the *mode* value to open the file in binary mode, which will improve
   portability.  (Appending ``'b'`` is useful even on systems that don't treat
   binary and text files differently, where it serves as documentation.)  See below
   for more possible values of *mode*.

   .. index::
      single: line-buffered I/O
      single: unbuffered I/O
      single: buffer size, I/O
      single: I/O control; buffering

   The optional *bufsize* argument specifies the file's desired buffer size: 0
   means unbuffered, 1 means line buffered, any other positive value means use a
   buffer of (approximately) that size.  A negative *bufsize* means to use the
   system default, which is usually line buffered for tty devices and fully
   buffered for other files.  If omitted, the system default is used. [#]_

   Modes ``'r+'``, ``'w+'`` and ``'a+'`` open the file for updating (note that
   ``'w+'`` truncates the file).  Append ``'b'`` to the mode to open the file in
   binary mode, on systems that differentiate between binary and text files; on
   systems that don't have this distinction, adding the ``'b'`` has no effect.

   In addition to the standard :cfunc:`fopen` values *mode* may be ``'U'`` or
   ``'rU'``.  Python is usually built with universal newline support; supplying
   ``'U'`` opens the file as a text file, but lines may be terminated by any of the
   following: the Unix end-of-line convention ``'\n'``,  the Macintosh convention
   ``'\r'``, or the Windows convention ``'\r\n'``. All of these external
   representations are seen as ``'\n'`` by the Python program. If Python is built
   without universal newline support a *mode* with ``'U'`` is the same as normal
   text mode.  Note that file objects so opened also have an attribute called
   :attr:`newlines` which has a value of ``None`` (if no newlines have yet been
   seen), ``'\n'``, ``'\r'``, ``'\r\n'``, or a tuple containing all the newline
   types seen.

   Python enforces that the mode, after stripping ``'U'``, begins with ``'r'``,
   ``'w'`` or ``'a'``.

   See also the :mod:`fileinput` module.

   .. versionchanged:: 2.5
      Restriction on first letter of mode string introduced.


.. function:: ord(c)

   Given a string of length one, return an integer representing the Unicode code
   point of the character when the argument is a unicode object, or the value of
   the byte when the argument is an 8-bit string. For example, ``ord('a')`` returns
   the integer ``97``, ``ord(u'\u2020')`` returns ``8224``.  This is the inverse of
   :func:`chr` for 8-bit strings and of :func:`unichr` for unicode objects.  If a
   unicode argument is given and Python was built with UCS2 Unicode, then the
   character's code point must be in the range [0..65535] inclusive; otherwise the
   string length is two, and a :exc:`TypeError` will be raised.


.. function:: pow(x, y[, z])

   Return *x* to the power *y*; if *z* is present, return *x* to the power *y*,
   modulo *z* (computed more efficiently than ``pow(x, y) % z``). The two-argument
   form ``pow(x, y)`` is equivalent to using the power operator: ``x**y``.

   The arguments must have numeric types.  With mixed operand types, the coercion
   rules for binary arithmetic operators apply.  For int and long int operands, the
   result has the same type as the operands (after coercion) unless the second
   argument is negative; in that case, all arguments are converted to float and a
   float result is delivered.  For example, ``10**2`` returns ``100``, but
   ``10**-2`` returns ``0.01``.  (This last feature was added in Python 2.2.  In
   Python 2.1 and before, if both arguments were of integer types and the second
   argument was negative, an exception was raised.) If the second argument is
   negative, the third argument must be omitted. If *z* is present, *x* and *y*
   must be of integer types, and *y* must be non-negative.  (This restriction was
   added in Python 2.2.  In Python 2.1 and before, floating 3-argument ``pow()``
   returned platform-dependent results depending on floating-point rounding
   accidents.)


.. function:: property([fget[, fset[, fdel[, doc]]]])

   Return a property attribute for new-style classes (classes that derive from
   :class:`object`).

   *fget* is a function for getting an attribute value, likewise *fset* is a
   function for setting, and *fdel* a function for del'ing, an attribute.  Typical
   use is to define a managed attribute x::

      class C(object):
          def __init__(self): self._x = None
          def getx(self): return self._x
          def setx(self, value): self._x = value
          def delx(self): del self._x
          x = property(getx, setx, delx, "I'm the 'x' property.")

   If given, *doc* will be the docstring of the property attribute. Otherwise, the
   property will copy *fget*'s docstring (if it exists).  This makes it possible to
   create read-only properties easily using :func:`property` as a decorator::

      class Parrot(object):
          def __init__(self):
              self._voltage = 100000

          @property
          def voltage(self):
              """Get the current voltage."""
              return self._voltage

   turns the :meth:`voltage` method into a "getter" for a read-only attribute with
   the same name.

   .. versionadded:: 2.2

   .. versionchanged:: 2.5
      Use *fget*'s docstring if no *doc* given.


.. function:: range([start,] stop[, step])

   This is a versatile function to create sequences containing arithmetic
   progressions.  It is most often used in :keyword:`for` loops.  The arguments
   must be plain integers.  If the *step* argument is omitted, it defaults to
   ``1``.  If the *start* argument is omitted, it defaults to ``0``.  The full form
   returns a list of plain integers ``[start, start + step, start + 2 * step,
   ...]``.  If *step* is positive, the last element is the largest ``start + i *
   step`` less than *stop*; if *step* is negative, the last element is the smallest
   ``start + i * step`` greater than *stop*.  *step* must not be zero (or else
   :exc:`ValueError` is raised).  Example::

      >>> list(range(10))
      [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
      >>> list(range(1, 11))
      [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
      >>> list(range(0, 30, 5))
      [0, 5, 10, 15, 20, 25]
      >>> list(range(0, 10, 3))
      [0, 3, 6, 9]
      >>> list(range(0, -10, -1))
      [0, -1, -2, -3, -4, -5, -6, -7, -8, -9]
      >>> list(range(0))
      []
      >>> list(range(1, 0))
      []


.. function:: repr(object)

   Return a string containing a printable representation of an object. This is the
   same value yielded by conversions (reverse quotes). It is sometimes useful to be
   able to access this operation as an ordinary function.  For many types, this
   function makes an attempt to return a string that would yield an object with the
   same value when passed to :func:`eval`.


.. function:: reversed(seq)

   Return a reverse iterator.  *seq* must be an object which supports the sequence
   protocol (the :meth:`__len__` method and the :meth:`__getitem__` method with
   integer arguments starting at ``0``).

   .. versionadded:: 2.4


.. function:: round(x[, n])

   Return the floating point value *x* rounded to *n* digits after the decimal
   point.  If *n* is omitted, it defaults to zero. The result is a floating point
   number.  Values are rounded to the closest multiple of 10 to the power minus
   *n*; if two multiples are equally close, rounding is done away from 0 (so. for
   example, ``round(0.5)`` is ``1.0`` and ``round(-0.5)`` is ``-1.0``).


.. function:: set([iterable])
   :noindex:

   Return a new set, optionally with elements are taken from *iterable*.
   The set type is described in :ref:`types-set`.

   For other containers see the built in :class:`dict`, :class:`list`, and
   :class:`tuple` classes, and the :mod:`collections` module.

   .. versionadded:: 2.4


.. function:: setattr(object, name, value)

   This is the counterpart of :func:`getattr`.  The arguments are an object, a
   string and an arbitrary value.  The string may name an existing attribute or a
   new attribute.  The function assigns the value to the attribute, provided the
   object allows it.  For example, ``setattr(x, 'foobar', 123)`` is equivalent to
   ``x.foobar = 123``.


.. function:: slice([start,] stop[, step])

   .. index:: single: Numerical Python

   Return a slice object representing the set of indices specified by
   ``range(start, stop, step)``.  The *start* and *step* arguments default to
   ``None``.  Slice objects have read-only data attributes :attr:`start`,
   :attr:`stop` and :attr:`step` which merely return the argument values (or their
   default).  They have no other explicit functionality; however they are used by
   Numerical Python and other third party extensions.  Slice objects are also
   generated when extended indexing syntax is used.  For example:
   ``a[start:stop:step]`` or ``a[start:stop, i]``.


.. function:: sorted(iterable[, cmp[, key[, reverse]]])

   Return a new sorted list from the items in *iterable*.

   The optional arguments *cmp*, *key*, and *reverse* have the same meaning as
   those for the :meth:`list.sort` method (described in section
   :ref:`typesseq-mutable`).

   *cmp* specifies a custom comparison function of two arguments (iterable
   elements) which should return a negative, zero or positive number depending on
   whether the first argument is considered smaller than, equal to, or larger than
   the second argument: ``cmp=lambda x,y: cmp(x.lower(), y.lower())``

   *key* specifies a function of one argument that is used to extract a comparison
   key from each list element: ``key=str.lower``

   *reverse* is a boolean value.  If set to ``True``, then the list elements are
   sorted as if each comparison were reversed.

   In general, the *key* and *reverse* conversion processes are much faster than
   specifying an equivalent *cmp* function.  This is because *cmp* is called
   multiple times for each list element while *key* and *reverse* touch each
   element only once.

   .. versionadded:: 2.4


.. function:: staticmethod(function)

   Return a static method for *function*.

   A static method does not receive an implicit first argument. To declare a static
   method, use this idiom::

      class C:
          @staticmethod
          def f(arg1, arg2, ...): ...

   The ``@staticmethod`` form is a function decorator -- see the description of
   function definitions in :ref:`function` for details.

   It can be called either on the class (such as ``C.f()``) or on an instance (such
   as ``C().f()``).  The instance is ignored except for its class.

   Static methods in Python are similar to those found in Java or C++. For a more
   advanced concept, see :func:`classmethod` in this section.

   For more information on static methods, consult the documentation on the
   standard type hierarchy in :ref:`types`.

   .. versionadded:: 2.2

   .. versionchanged:: 2.4
      Function decorator syntax added.


.. function:: str([object[, encoding[, errors]]])

   Return a string version of an object, using one of the following modes:
   
   If *encoding* and/or *errors* are given, :func:`str` will decode the
   *object* which can either be a byte string or a character buffer using
   the codec for *encoding*. The *encoding* parameter is a string giving
   the name of an encoding; if the encoding is not known, :exc:`LookupError`
   is raised.  Error handling is done according to *errors*; this specifies the
   treatment of characters which are invalid in the input encoding. If
   *errors* is ``'strict'`` (the default), a :exc:`ValueError` is raised on
   errors, while a value of ``'ignore'`` causes errors to be silently ignored,
   and a value of ``'replace'`` causes the official Unicode replacement character,
   U+FFFD, to be used to replace input characters which cannot be decoded.
   See also the :mod:`codecs` module. 

   When only *object* is given, this returns its nicely printable representation.
   For strings, this is the string itself.  The difference with ``repr(object)``
   is that ``str(object)`` does not always attempt to return a string that is
   acceptable to :func:`eval`; its goal is to return a printable string.
   With no arguments, this returns the empty string.

   Objects can specify what ``str(object)`` returns by defining a :meth:`__str__`
   special method.

   For more information on strings see :ref:`typesseq` which describes sequence
   functionality (strings are sequences), and also the string-specific methods
   described in the :ref:`string-methods` section. To output formatted strings
   use template strings or the ``%`` operator described in the
   :ref:`string-formatting` section. In addition see the :ref:`stringservices`
   section. See also :func:`unicode`.


.. function:: sum(iterable[, start])

   Sums *start* and the items of an *iterable* from left to right and returns the
   total.  *start* defaults to ``0``. The *iterable*'s items are normally numbers,
   and are not allowed to be strings.  The fast, correct way to concatenate a
   sequence of strings is by calling ``''.join(sequence)``.

   .. versionadded:: 2.3


.. function:: super(type[, object-or-type])

   Return the superclass of *type*.  If the second argument is omitted the super
   object returned is unbound.  If the second argument is an object,
   ``isinstance(obj, type)`` must be true.  If the second argument is a type,
   ``issubclass(type2, type)`` must be true. :func:`super` only works for new-style
   classes.

   A typical use for calling a cooperative superclass method is::

      class C(B):
          def meth(self, arg):
              super(C, self).meth(arg)

   Note that :func:`super` is implemented as part of the binding process for
   explicit dotted attribute lookups such as ``super(C, self).__getitem__(name)``.
   Accordingly, :func:`super` is undefined for implicit lookups using statements or
   operators such as ``super(C, self)[name]``.

   .. versionadded:: 2.2


.. function:: tuple([iterable])

   Return a tuple whose items are the same and in the same order as *iterable*'s
   items.  *iterable* may be a sequence, a container that supports iteration, or an
   iterator object. If *iterable* is already a tuple, it is returned unchanged.
   For instance, ``tuple('abc')`` returns ``('a', 'b', 'c')`` and ``tuple([1, 2,
   3])`` returns ``(1, 2, 3)``.  If no argument is given, returns a new empty
   tuple, ``()``.

   :class:`tuple` is an immutable sequence type, as documented in
   :ref:`typesseq`. For other containers see the built in :class:`dict`,
   :class:`list`, and :class:`set` classes, and the :mod:`collections` module.


.. function:: type(object)

   .. index:: object: type

   Return the type of an *object*.  The return value is a type object.  The
   :func:`isinstance` built-in function is recommended for testing the type of an
   object.

   With three arguments, :func:`type` functions as a constructor as detailed below.


.. function:: type(name, bases, dict)
   :noindex:

   Return a new type object.  This is essentially a dynamic form of the
   :keyword:`class` statement. The *name* string is the class name and becomes the
   :attr:`__name__` attribute; the *bases* tuple itemizes the base classes and
   becomes the :attr:`__bases__` attribute; and the *dict* dictionary is the
   namespace containing definitions for class body and becomes the :attr:`__dict__`
   attribute.  For example, the following two statements create identical
   :class:`type` objects::

      >>> class X(object):
      ...     a = 1
      ...     
      >>> X = type('X', (object,), dict(a=1))

   .. versionadded:: 2.2


.. function:: vars([object])

   Without arguments, return a dictionary corresponding to the current local symbol
   table.  With a module, class or class instance object as argument (or anything
   else that has a :attr:`__dict__` attribute), returns a dictionary corresponding
   to the object's symbol table.  The returned dictionary should not be modified:
   the effects on the corresponding symbol table are undefined. [#]_


.. function:: zip([iterable, ...])

   This function returns a list of tuples, where the *i*-th tuple contains the
   *i*-th element from each of the argument sequences or iterables. The returned
   list is truncated in length to the length of the shortest argument sequence.
   When there are multiple arguments which are all of the same length, :func:`zip`
   is similar to :func:`map` with an initial argument of ``None``. With a single
   sequence argument, it returns a list of 1-tuples. With no arguments, it returns
   an empty list.

   .. versionadded:: 2.0

   .. versionchanged:: 2.4
      Formerly, :func:`zip` required at least one argument and ``zip()`` raised a
      :exc:`TypeError` instead of returning an empty list.

.. % ---------------------------------------------------------------------------


.. _non-essential-built-in-funcs:

Non-essential Built-in Functions
================================

There are several built-in functions that are no longer essential to learn, know
or use in modern Python programming.  They have been kept here to maintain
backwards compatibility with programs written for older versions of Python.

Python programmers, trainers, students and bookwriters should feel free to
bypass these functions without concerns about missing something important.


.. function:: buffer(object[, offset[, size]])

   The *object* argument must be an object that supports the buffer call interface
   (such as strings, arrays, and buffers).  A new buffer object will be created
   which references the *object* argument. The buffer object will be a slice from
   the beginning of *object* (or from the specified *offset*). The slice will
   extend to the end of *object* (or will have a length given by the *size*
   argument).



.. rubric:: Footnotes

.. [#] Specifying a buffer size currently has no effect on systems that don't have
   :cfunc:`setvbuf`.  The interface to specify the buffer size is not done using a
   method that calls :cfunc:`setvbuf`, because that may dump core when called after
   any I/O has been performed, and there's no reliable way to determine whether
   this is the case.

.. [#] In the current implementation, local variable bindings cannot normally be
   affected this way, but variables retrieved from other scopes (such as modules)
   can be.  This may change.

