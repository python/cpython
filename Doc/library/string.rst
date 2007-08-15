
:mod:`string` --- Common string operations
==========================================

.. module:: string
   :synopsis: Common string operations.


.. index:: module: re

The :mod:`string` module contains a number of useful constants and
classes, as well as some deprecated legacy functions that are also
available as methods on strings. In addition, Python's built-in string
classes support the sequence type methods described in the
:ref:`typesseq` section, and also the string-specific methods described
in the :ref:`string-methods` section. To output formatted strings use
template strings or the ``%`` operator described in the
:ref:`string-formatting` section. Also, see the :mod:`re` module for
string functions based on regular expressions.


String constants
----------------

The constants defined in this module are:


.. data:: ascii_letters

   The concatenation of the :const:`ascii_lowercase` and :const:`ascii_uppercase`
   constants described below.  This value is not locale-dependent.


.. data:: ascii_lowercase

   The lowercase letters ``'abcdefghijklmnopqrstuvwxyz'``.  This value is not
   locale-dependent and will not change.


.. data:: ascii_uppercase

   The uppercase letters ``'ABCDEFGHIJKLMNOPQRSTUVWXYZ'``.  This value is not
   locale-dependent and will not change.


.. data:: digits

   The string ``'0123456789'``.


.. data:: hexdigits

   The string ``'0123456789abcdefABCDEF'``.


.. data:: octdigits

   The string ``'01234567'``.


.. data:: punctuation

   String of ASCII characters which are considered punctuation characters
   in the ``C`` locale.


.. data:: printable

   String of ASCII characters which are considered printable.  This is a
   combination of :const:`digits`, :const:`ascii_letters`, :const:`punctuation`,
   and :const:`whitespace`.


.. data:: whitespace

   A string containing all characters that are considered whitespace.
   This includes the characters space, tab, linefeed, return, formfeed, and
   vertical tab.


Template strings
----------------

Templates provide simpler string substitutions as described in :pep:`292`.
Instead of the normal ``%``\ -based substitutions, Templates support ``$``\
-based substitutions, using the following rules:

* ``$$`` is an escape; it is replaced with a single ``$``.

* ``$identifier`` names a substitution placeholder matching a mapping key of
  ``"identifier"``.  By default, ``"identifier"`` must spell a Python
  identifier.  The first non-identifier character after the ``$`` character
  terminates this placeholder specification.

* ``${identifier}`` is equivalent to ``$identifier``.  It is required when valid
  identifier characters follow the placeholder but are not part of the
  placeholder, such as ``"${noun}ification"``.

Any other appearance of ``$`` in the string will result in a :exc:`ValueError`
being raised.

.. versionadded:: 2.4

The :mod:`string` module provides a :class:`Template` class that implements
these rules.  The methods of :class:`Template` are:


.. class:: Template(template)

   The constructor takes a single argument which is the template string.


.. method:: Template.substitute(mapping[, **kws])

   Performs the template substitution, returning a new string.  *mapping* is any
   dictionary-like object with keys that match the placeholders in the template.
   Alternatively, you can provide keyword arguments, where the keywords are the
   placeholders.  When both *mapping* and *kws* are given and there are duplicates,
   the placeholders from *kws* take precedence.


.. method:: Template.safe_substitute(mapping[, **kws])

   Like :meth:`substitute`, except that if placeholders are missing from *mapping*
   and *kws*, instead of raising a :exc:`KeyError` exception, the original
   placeholder will appear in the resulting string intact.  Also, unlike with
   :meth:`substitute`, any other appearances of the ``$`` will simply return ``$``
   instead of raising :exc:`ValueError`.

   While other exceptions may still occur, this method is called "safe" because
   substitutions always tries to return a usable string instead of raising an
   exception.  In another sense, :meth:`safe_substitute` may be anything other than
   safe, since it will silently ignore malformed templates containing dangling
   delimiters, unmatched braces, or placeholders that are not valid Python
   identifiers.

:class:`Template` instances also provide one public data attribute:


.. attribute:: string.template

   This is the object passed to the constructor's *template* argument.  In general,
   you shouldn't change it, but read-only access is not enforced.

Here is an example of how to use a Template::

   >>> from string import Template
   >>> s = Template('$who likes $what')
   >>> s.substitute(who='tim', what='kung pao')
   'tim likes kung pao'
   >>> d = dict(who='tim')
   >>> Template('Give $who $100').substitute(d)
   Traceback (most recent call last):
   [...]
   ValueError: Invalid placeholder in string: line 1, col 10
   >>> Template('$who likes $what').substitute(d)
   Traceback (most recent call last):
   [...]
   KeyError: 'what'
   >>> Template('$who likes $what').safe_substitute(d)
   'tim likes $what'

Advanced usage: you can derive subclasses of :class:`Template` to customize the
placeholder syntax, delimiter character, or the entire regular expression used
to parse template strings.  To do this, you can override these class attributes:

* *delimiter* -- This is the literal string describing a placeholder introducing
  delimiter.  The default value ``$``.  Note that this should *not* be a regular
  expression, as the implementation will call :meth:`re.escape` on this string as
  needed.

* *idpattern* -- This is the regular expression describing the pattern for
  non-braced placeholders (the braces will be added automatically as
  appropriate).  The default value is the regular expression
  ``[_a-z][_a-z0-9]*``.

Alternatively, you can provide the entire regular expression pattern by
overriding the class attribute *pattern*.  If you do this, the value must be a
regular expression object with four named capturing groups.  The capturing
groups correspond to the rules given above, along with the invalid placeholder
rule:

* *escaped* -- This group matches the escape sequence, e.g. ``$$``, in the
  default pattern.

* *named* -- This group matches the unbraced placeholder name; it should not
  include the delimiter in capturing group.

* *braced* -- This group matches the brace enclosed placeholder name; it should
  not include either the delimiter or braces in the capturing group.

* *invalid* -- This group matches any other delimiter pattern (usually a single
  delimiter), and it should appear last in the regular expression.


String functions
----------------

The following functions are available to operate on string and Unicode objects.
They are not available as string methods.


.. function:: capwords(s)

   Split the argument into words using :func:`split`, capitalize each word using
   :func:`capitalize`, and join the capitalized words using :func:`join`.  Note
   that this replaces runs of whitespace characters by a single space, and removes
   leading and trailing whitespace.


.. function:: maketrans(from, to)

   Return a translation table suitable for passing to :func:`translate`, that will
   map each character in *from* into the character at the same position in *to*;
   *from* and *to* must have the same length.

   .. warning::

      Don't use strings derived from :const:`lowercase` and :const:`uppercase` as
      arguments; in some locales, these don't have the same length.  For case
      conversions, always use :func:`lower` and :func:`upper`.


Deprecated string functions
---------------------------

The following list of functions are also defined as methods of string and
Unicode objects; see section :ref:`string-methods` for more information on
those.  You should consider these functions as deprecated, although they will
not be removed until Python 3.0.  The functions defined in this module are:


.. function:: atof(s)

   .. deprecated:: 2.0
      Use the :func:`float` built-in function.

   .. index:: builtin: float

   Convert a string to a floating point number.  The string must have the standard
   syntax for a floating point literal in Python, optionally preceded by a sign
   (``+`` or ``-``).  Note that this behaves identical to the built-in function
   :func:`float` when passed a string.

   .. note::

      .. index::
         single: NaN
         single: Infinity

      When passing in a string, values for NaN and Infinity may be returned, depending
      on the underlying C library.  The specific set of strings accepted which cause
      these values to be returned depends entirely on the C library and is known to
      vary.


.. function:: atoi(s[, base])

   .. deprecated:: 2.0
      Use the :func:`int` built-in function.

   .. index:: builtin: eval

   Convert string *s* to an integer in the given *base*.  The string must consist
   of one or more digits, optionally preceded by a sign (``+`` or ``-``).  The
   *base* defaults to 10.  If it is 0, a default base is chosen depending on the
   leading characters of the string (after stripping the sign): ``0x`` or ``0X``
   means 16, ``0`` means 8, anything else means 10.  If *base* is 16, a leading
   ``0x`` or ``0X`` is always accepted, though not required.  This behaves
   identically to the built-in function :func:`int` when passed a string.  (Also
   note: for a more flexible interpretation of numeric literals, use the built-in
   function :func:`eval`.)


.. function:: atol(s[, base])

   .. deprecated:: 2.0
      Use the :func:`long` built-in function.

   .. index:: builtin: long

   Convert string *s* to a long integer in the given *base*. The string must
   consist of one or more digits, optionally preceded by a sign (``+`` or ``-``).
   The *base* argument has the same meaning as for :func:`atoi`.  A trailing ``l``
   or ``L`` is not allowed, except if the base is 0.  Note that when invoked
   without *base* or with *base* set to 10, this behaves identical to the built-in
   function :func:`long` when passed a string.


.. function:: capitalize(word)

   Return a copy of *word* with only its first character capitalized.


.. function:: expandtabs(s[, tabsize])

   Expand tabs in a string replacing them by one or more spaces, depending on the
   current column and the given tab size.  The column number is reset to zero after
   each newline occurring in the string. This doesn't understand other non-printing
   characters or escape sequences.  The tab size defaults to 8.


.. function:: find(s, sub[, start[,end]])

   Return the lowest index in *s* where the substring *sub* is found such that
   *sub* is wholly contained in ``s[start:end]``.  Return ``-1`` on failure.
   Defaults for *start* and *end* and interpretation of negative values is the same
   as for slices.


.. function:: rfind(s, sub[, start[, end]])

   Like :func:`find` but find the highest index.


.. function:: index(s, sub[, start[, end]])

   Like :func:`find` but raise :exc:`ValueError` when the substring is not found.


.. function:: rindex(s, sub[, start[, end]])

   Like :func:`rfind` but raise :exc:`ValueError` when the substring is not found.


.. function:: count(s, sub[, start[, end]])

   Return the number of (non-overlapping) occurrences of substring *sub* in string
   ``s[start:end]``. Defaults for *start* and *end* and interpretation of negative
   values are the same as for slices.


.. function:: lower(s)

   Return a copy of *s*, but with upper case letters converted to lower case.


.. function:: split(s[, sep[, maxsplit]])

   Return a list of the words of the string *s*.  If the optional second argument
   *sep* is absent or ``None``, the words are separated by arbitrary strings of
   whitespace characters (space, tab,  newline, return, formfeed).  If the second
   argument *sep* is present and not ``None``, it specifies a string to be used as
   the  word separator.  The returned list will then have one more item than the
   number of non-overlapping occurrences of the separator in the string.  The
   optional third argument *maxsplit* defaults to 0.  If it is nonzero, at most
   *maxsplit* number of splits occur, and the remainder of the string is returned
   as the final element of the list (thus, the list will have at most
   ``maxsplit+1`` elements).

   The behavior of split on an empty string depends on the value of *sep*. If *sep*
   is not specified, or specified as ``None``, the result will be an empty list.
   If *sep* is specified as any string, the result will be a list containing one
   element which is an empty string.


.. function:: rsplit(s[, sep[, maxsplit]])

   Return a list of the words of the string *s*, scanning *s* from the end.  To all
   intents and purposes, the resulting list of words is the same as returned by
   :func:`split`, except when the optional third argument *maxsplit* is explicitly
   specified and nonzero.  When *maxsplit* is nonzero, at most *maxsplit* number of
   splits -- the *rightmost* ones -- occur, and the remainder of the string is
   returned as the first element of the list (thus, the list will have at most
   ``maxsplit+1`` elements).

   .. versionadded:: 2.4


.. function:: splitfields(s[, sep[, maxsplit]])

   This function behaves identically to :func:`split`.  (In the past, :func:`split`
   was only used with one argument, while :func:`splitfields` was only used with
   two arguments.)


.. function:: join(words[, sep])

   Concatenate a list or tuple of words with intervening occurrences of  *sep*.
   The default value for *sep* is a single space character.  It is always true that
   ``string.join(string.split(s, sep), sep)`` equals *s*.


.. function:: joinfields(words[, sep])

   This function behaves identically to :func:`join`.  (In the past,  :func:`join`
   was only used with one argument, while :func:`joinfields` was only used with two
   arguments.) Note that there is no :meth:`joinfields` method on string objects;
   use the :meth:`join` method instead.


.. function:: lstrip(s[, chars])

   Return a copy of the string with leading characters removed.  If *chars* is
   omitted or ``None``, whitespace characters are removed.  If given and not
   ``None``, *chars* must be a string; the characters in the string will be
   stripped from the beginning of the string this method is called on.

   .. versionchanged:: 2.2.3
      The *chars* parameter was added.  The *chars* parameter cannot be passed in
      earlier 2.2 versions.


.. function:: rstrip(s[, chars])

   Return a copy of the string with trailing characters removed.  If *chars* is
   omitted or ``None``, whitespace characters are removed.  If given and not
   ``None``, *chars* must be a string; the characters in the string will be
   stripped from the end of the string this method is called on.

   .. versionchanged:: 2.2.3
      The *chars* parameter was added.  The *chars* parameter cannot be passed in
      earlier 2.2 versions.


.. function:: strip(s[, chars])

   Return a copy of the string with leading and trailing characters removed.  If
   *chars* is omitted or ``None``, whitespace characters are removed.  If given and
   not ``None``, *chars* must be a string; the characters in the string will be
   stripped from the both ends of the string this method is called on.

   .. versionchanged:: 2.2.3
      The *chars* parameter was added.  The *chars* parameter cannot be passed in
      earlier 2.2 versions.


.. function:: swapcase(s)

   Return a copy of *s*, but with lower case letters converted to upper case and
   vice versa.


.. function:: translate(s, table[, deletechars])

   Delete all characters from *s* that are in *deletechars* (if  present), and then
   translate the characters using *table*, which  must be a 256-character string
   giving the translation for each character value, indexed by its ordinal.  If
   *table* is ``None``, then only the character deletion step is performed.


.. function:: upper(s)

   Return a copy of *s*, but with lower case letters converted to upper case.


.. function:: ljust(s, width)
              rjust(s, width)
              center(s, width)

   These functions respectively left-justify, right-justify and center a string in
   a field of given width.  They return a string that is at least *width*
   characters wide, created by padding the string *s* with spaces until the given
   width on the right, left or both sides.  The string is never truncated.


.. function:: zfill(s, width)

   Pad a numeric string on the left with zero digits until the given width is
   reached.  Strings starting with a sign are handled correctly.


.. function:: replace(str, old, new[, maxreplace])

   Return a copy of string *str* with all occurrences of substring *old* replaced
   by *new*.  If the optional argument *maxreplace* is given, the first
   *maxreplace* occurrences are replaced.

