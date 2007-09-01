
:mod:`re` --- Regular expression operations
===========================================

.. module:: re
   :synopsis: Regular expression operations.
.. moduleauthor:: Fredrik Lundh <fredrik@pythonware.com>
.. sectionauthor:: Andrew M. Kuchling <amk@amk.ca>




This module provides regular expression matching operations similar to
those found in Perl. Both patterns and strings to be searched can be
Unicode strings as well as 8-bit strings.  The :mod:`re` module is
always available.

Regular expressions use the backslash character (``'\'``) to indicate
special forms or to allow special characters to be used without invoking
their special meaning.  This collides with Python's usage of the same
character for the same purpose in string literals; for example, to match
a literal backslash, one might have to write ``'\\\\'`` as the pattern
string, because the regular expression must be ``\\``, and each
backslash must be expressed as ``\\`` inside a regular Python string
literal.

The solution is to use Python's raw string notation for regular expression
patterns; backslashes are not handled in any special way in a string literal
prefixed with ``'r'``.  So ``r"\n"`` is a two-character string containing
``'\'`` and ``'n'``, while ``"\n"`` is a one-character string containing a
newline. Usually patterns will be expressed in Python code using this raw string
notation.

.. seealso::

   Mastering Regular Expressions
      Book on regular expressions by Jeffrey Friedl, published by O'Reilly.  The
      second  edition of the book no longer covers Python at all,  but the first
      edition covered writing good regular expression patterns in great detail.


.. _re-syntax:

Regular Expression Syntax
-------------------------

A regular expression (or RE) specifies a set of strings that matches it; the
functions in this module let you check if a particular string matches a given
regular expression (or if a given regular expression matches a particular
string, which comes down to the same thing).

Regular expressions can be concatenated to form new regular expressions; if *A*
and *B* are both regular expressions, then *AB* is also a regular expression.
In general, if a string *p* matches *A* and another string *q* matches *B*, the
string *pq* will match AB.  This holds unless *A* or *B* contain low precedence
operations; boundary conditions between *A* and *B*; or have numbered group
references.  Thus, complex expressions can easily be constructed from simpler
primitive expressions like the ones described here.  For details of the theory
and implementation of regular expressions, consult the Friedl book referenced
above, or almost any textbook about compiler construction.

A brief explanation of the format of regular expressions follows.  For further
information and a gentler presentation, consult the Regular Expression HOWTO,
accessible from http://www.python.org/doc/howto/.

Regular expressions can contain both special and ordinary characters. Most
ordinary characters, like ``'A'``, ``'a'``, or ``'0'``, are the simplest regular
expressions; they simply match themselves.  You can concatenate ordinary
characters, so ``last`` matches the string ``'last'``.  (In the rest of this
section, we'll write RE's in ``this special style``, usually without quotes, and
strings to be matched ``'in single quotes'``.)

Some characters, like ``'|'`` or ``'('``, are special. Special
characters either stand for classes of ordinary characters, or affect
how the regular expressions around them are interpreted. Regular
expression pattern strings may not contain null bytes, but can specify
the null byte using the ``\number`` notation, e.g., ``'\x00'``.


The special characters are:

.. % 

``'.'``
   (Dot.)  In the default mode, this matches any character except a newline.  If
   the :const:`DOTALL` flag has been specified, this matches any character
   including a newline.

``'^'``
   (Caret.)  Matches the start of the string, and in :const:`MULTILINE` mode also
   matches immediately after each newline.

``'$'``
   Matches the end of the string or just before the newline at the end of the
   string, and in :const:`MULTILINE` mode also matches before a newline.  ``foo``
   matches both 'foo' and 'foobar', while the regular expression ``foo$`` matches
   only 'foo'.  More interestingly, searching for ``foo.$`` in ``'foo1\nfoo2\n'``
   matches 'foo2' normally, but 'foo1' in :const:`MULTILINE` mode.

``'*'``
   Causes the resulting RE to match 0 or more repetitions of the preceding RE, as
   many repetitions as are possible.  ``ab*`` will match 'a', 'ab', or 'a' followed
   by any number of 'b's.

``'+'``
   Causes the resulting RE to match 1 or more repetitions of the preceding RE.
   ``ab+`` will match 'a' followed by any non-zero number of 'b's; it will not
   match just 'a'.

``'?'``
   Causes the resulting RE to match 0 or 1 repetitions of the preceding RE.
   ``ab?`` will match either 'a' or 'ab'.

``*?``, ``+?``, ``??``
   The ``'*'``, ``'+'``, and ``'?'`` qualifiers are all :dfn:`greedy`; they match
   as much text as possible.  Sometimes this behaviour isn't desired; if the RE
   ``<.*>`` is matched against ``'<H1>title</H1>'``, it will match the entire
   string, and not just ``'<H1>'``.  Adding ``'?'`` after the qualifier makes it
   perform the match in :dfn:`non-greedy` or :dfn:`minimal` fashion; as *few*
   characters as possible will be matched.  Using ``.*?`` in the previous
   expression will match only ``'<H1>'``.

``{m}``
   Specifies that exactly *m* copies of the previous RE should be matched; fewer
   matches cause the entire RE not to match.  For example, ``a{6}`` will match
   exactly six ``'a'`` characters, but not five.

``{m,n}``
   Causes the resulting RE to match from *m* to *n* repetitions of the preceding
   RE, attempting to match as many repetitions as possible.  For example,
   ``a{3,5}`` will match from 3 to 5 ``'a'`` characters.  Omitting *m* specifies a
   lower bound of zero,  and omitting *n* specifies an infinite upper bound.  As an
   example, ``a{4,}b`` will match ``aaaab`` or a thousand ``'a'`` characters
   followed by a ``b``, but not ``aaab``. The comma may not be omitted or the
   modifier would be confused with the previously described form.

``{m,n}?``
   Causes the resulting RE to match from *m* to *n* repetitions of the preceding
   RE, attempting to match as *few* repetitions as possible.  This is the
   non-greedy version of the previous qualifier.  For example, on the
   6-character string ``'aaaaaa'``, ``a{3,5}`` will match 5 ``'a'`` characters,
   while ``a{3,5}?`` will only match 3 characters.

``'\'``
   Either escapes special characters (permitting you to match characters like
   ``'*'``, ``'?'``, and so forth), or signals a special sequence; special
   sequences are discussed below.

   If you're not using a raw string to express the pattern, remember that Python
   also uses the backslash as an escape sequence in string literals; if the escape
   sequence isn't recognized by Python's parser, the backslash and subsequent
   character are included in the resulting string.  However, if Python would
   recognize the resulting sequence, the backslash should be repeated twice.  This
   is complicated and hard to understand, so it's highly recommended that you use
   raw strings for all but the simplest expressions.

``[]``
   Used to indicate a set of characters.  Characters can be listed individually, or
   a range of characters can be indicated by giving two characters and separating
   them by a ``'-'``.  Special characters are not active inside sets.  For example,
   ``[akm$]`` will match any of the characters ``'a'``, ``'k'``,
   ``'m'``, or ``'$'``; ``[a-z]`` will match any lowercase letter, and
   ``[a-zA-Z0-9]`` matches any letter or digit.  Character classes such
   as ``\w`` or ``\S`` (defined below) are also acceptable inside a
   range, although the characters they match depends on whether :const:`LOCALE`
   or  :const:`UNICODE` mode is in force.  If you want to include a
   ``']'`` or a ``'-'`` inside a set, precede it with a backslash, or
   place it as the first character.  The pattern ``[]]`` will match
   ``']'``, for example.

   You can match the characters not within a range by :dfn:`complementing` the set.
   This is indicated by including a ``'^'`` as the first character of the set;
   ``'^'`` elsewhere will simply match the ``'^'`` character.  For example,
   ``[^5]`` will match any character except ``'5'``, and ``[^^]`` will match any
   character except ``'^'``.

``'|'``
   ``A|B``, where A and B can be arbitrary REs, creates a regular expression that
   will match either A or B.  An arbitrary number of REs can be separated by the
   ``'|'`` in this way.  This can be used inside groups (see below) as well.  As
   the target string is scanned, REs separated by ``'|'`` are tried from left to
   right. When one pattern completely matches, that branch is accepted. This means
   that once ``A`` matches, ``B`` will not be tested further, even if it would
   produce a longer overall match.  In other words, the ``'|'`` operator is never
   greedy.  To match a literal ``'|'``, use ``\|``, or enclose it inside a
   character class, as in ``[|]``.

``(...)``
   Matches whatever regular expression is inside the parentheses, and indicates the
   start and end of a group; the contents of a group can be retrieved after a match
   has been performed, and can be matched later in the string with the ``\number``
   special sequence, described below.  To match the literals ``'('`` or ``')'``,
   use ``\(`` or ``\)``, or enclose them inside a character class: ``[(] [)]``.

``(?...)``
   This is an extension notation (a ``'?'`` following a ``'('`` is not meaningful
   otherwise).  The first character after the ``'?'`` determines what the meaning
   and further syntax of the construct is. Extensions usually do not create a new
   group; ``(?P<name>...)`` is the only exception to this rule. Following are the
   currently supported extensions.

``(?iLmsux)``
   (One or more letters from the set ``'i'``, ``'L'``, ``'m'``, ``'s'``,
   ``'u'``, ``'x'``.)  The group matches the empty string; the letters
   set the corresponding flags: :const:`re.I` (ignore case),
   :const:`re.L` (locale dependent), :const:`re.M` (multi-line),
   :const:`re.S` (dot matches all), :const:`re.U` (Unicode dependent),
   and :const:`re.X` (verbose), for the entire regular expression. (The
   flags are described in :ref:`contents-of-module-re`.) This
   is useful if you wish to include the flags as part of the regular
   expression, instead of passing a *flag* argument to the
   :func:`compile` function.

   Note that the ``(?x)`` flag changes how the expression is parsed. It should be
   used first in the expression string, or after one or more whitespace characters.
   If there are non-whitespace characters before the flag, the results are
   undefined.

``(?:...)``
   A non-grouping version of regular parentheses. Matches whatever regular
   expression is inside the parentheses, but the substring matched by the group
   *cannot* be retrieved after performing a match or referenced later in the
   pattern.

``(?P<name>...)``
   Similar to regular parentheses, but the substring matched by the group is
   accessible via the symbolic group name *name*.  Group names must be valid Python
   identifiers, and each group name must be defined only once within a regular
   expression.  A symbolic group is also a numbered group, just as if the group
   were not named.  So the group named 'id' in the example below can also be
   referenced as the numbered group 1.

   For example, if the pattern is ``(?P<id>[a-zA-Z_]\w*)``, the group can be
   referenced by its name in arguments to methods of match objects, such as
   ``m.group('id')`` or ``m.end('id')``, and also by name in pattern text (for
   example, ``(?P=id)``) and replacement text (such as ``\g<id>``).

``(?P=name)``
   Matches whatever text was matched by the earlier group named *name*.

``(?#...)``
   A comment; the contents of the parentheses are simply ignored.

``(?=...)``
   Matches if ``...`` matches next, but doesn't consume any of the string.  This is
   called a lookahead assertion.  For example, ``Isaac (?=Asimov)`` will match
   ``'Isaac '`` only if it's followed by ``'Asimov'``.

``(?!...)``
   Matches if ``...`` doesn't match next.  This is a negative lookahead assertion.
   For example, ``Isaac (?!Asimov)`` will match ``'Isaac '`` only if it's *not*
   followed by ``'Asimov'``.

``(?<=...)``
   Matches if the current position in the string is preceded by a match for ``...``
   that ends at the current position.  This is called a :dfn:`positive lookbehind
   assertion`. ``(?<=abc)def`` will find a match in ``abcdef``, since the
   lookbehind will back up 3 characters and check if the contained pattern matches.
   The contained pattern must only match strings of some fixed length, meaning that
   ``abc`` or ``a|b`` are allowed, but ``a*`` and ``a{3,4}`` are not.  Note that
   patterns which start with positive lookbehind assertions will never match at the
   beginning of the string being searched; you will most likely want to use the
   :func:`search` function rather than the :func:`match` function::

      >>> import re
      >>> m = re.search('(?<=abc)def', 'abcdef')
      >>> m.group(0)
      'def'

   This example looks for a word following a hyphen::

      >>> m = re.search('(?<=-)\w+', 'spam-egg')
      >>> m.group(0)
      'egg'

``(?<!...)``
   Matches if the current position in the string is not preceded by a match for
   ``...``.  This is called a :dfn:`negative lookbehind assertion`.  Similar to
   positive lookbehind assertions, the contained pattern must only match strings of
   some fixed length.  Patterns which start with negative lookbehind assertions may
   match at the beginning of the string being searched.

``(?(id/name)yes-pattern|no-pattern)``
   Will try to match with ``yes-pattern`` if the group with given *id* or *name*
   exists, and with ``no-pattern`` if it doesn't. ``no-pattern`` is optional and
   can be omitted. For example,  ``(<)?(\w+@\w+(?:\.\w+)+)(?(1)>)`` is a poor email
   matching pattern, which will match with ``'<user@host.com>'`` as well as
   ``'user@host.com'``, but not with ``'<user@host.com'``.


The special sequences consist of ``'\'`` and a character from the list below.
If the ordinary character is not on the list, then the resulting RE will match
the second character.  For example, ``\$`` matches the character ``'$'``.

.. % 

``\number``
   Matches the contents of the group of the same number.  Groups are numbered
   starting from 1.  For example, ``(.+) \1`` matches ``'the the'`` or ``'55 55'``,
   but not ``'the end'`` (note the space after the group).  This special sequence
   can only be used to match one of the first 99 groups.  If the first digit of
   *number* is 0, or *number* is 3 octal digits long, it will not be interpreted as
   a group match, but as the character with octal value *number*. Inside the
   ``'['`` and ``']'`` of a character class, all numeric escapes are treated as
   characters.

``\A``
   Matches only at the start of the string.

``\b``
   Matches the empty string, but only at the beginning or end of a word.  A word is
   defined as a sequence of alphanumeric or underscore characters, so the end of a
   word is indicated by whitespace or a non-alphanumeric, non-underscore character.
   Note that  ``\b`` is defined as the boundary between ``\w`` and ``\ W``, so the
   precise set of characters deemed to be alphanumeric depends on the values of the
   ``UNICODE`` and ``LOCALE`` flags.  Inside a character range, ``\b`` represents
   the backspace character, for compatibility with Python's string literals.

``\B``
   Matches the empty string, but only when it is *not* at the beginning or end of a
   word.  This is just the opposite of ``\b``, so is also subject to the settings
   of ``LOCALE`` and ``UNICODE``.

``\d``
   When the :const:`UNICODE` flag is not specified, matches any decimal digit; this
   is equivalent to the set ``[0-9]``.  With :const:`UNICODE`, it will match
   whatever is classified as a digit in the Unicode character properties database.

``\D``
   When the :const:`UNICODE` flag is not specified, matches any non-digit
   character; this is equivalent to the set  ``[^0-9]``.  With :const:`UNICODE`, it
   will match  anything other than character marked as digits in the Unicode
   character  properties database.

``\s``
   When the :const:`LOCALE` and :const:`UNICODE` flags are not specified, matches
   any whitespace character; this is equivalent to the set ``[ \t\n\r\f\v]``. With
   :const:`LOCALE`, it will match this set plus whatever characters are defined as
   space for the current locale. If :const:`UNICODE` is set, this will match the
   characters ``[ \t\n\r\f\v]`` plus whatever is classified as space in the Unicode
   character properties database.

``\S``
   When the :const:`LOCALE` and :const:`UNICODE` flags are not specified, matches
   any non-whitespace character; this is equivalent to the set ``[^ \t\n\r\f\v]``
   With :const:`LOCALE`, it will match any character not in this set, and not
   defined as space in the current locale. If :const:`UNICODE` is set, this will
   match anything other than ``[ \t\n\r\f\v]`` and characters marked as space in
   the Unicode character properties database.

``\w``
   When the :const:`LOCALE` and :const:`UNICODE` flags are not specified, matches
   any alphanumeric character and the underscore; this is equivalent to the set
   ``[a-zA-Z0-9_]``.  With :const:`LOCALE`, it will match the set ``[0-9_]`` plus
   whatever characters are defined as alphanumeric for the current locale.  If
   :const:`UNICODE` is set, this will match the characters ``[0-9_]`` plus whatever
   is classified as alphanumeric in the Unicode character properties database.

``\W``
   When the :const:`LOCALE` and :const:`UNICODE` flags are not specified, matches
   any non-alphanumeric character; this is equivalent to the set ``[^a-zA-Z0-9_]``.
   With :const:`LOCALE`, it will match any character not in the set ``[0-9_]``, and
   not defined as alphanumeric for the current locale. If :const:`UNICODE` is set,
   this will match anything other than ``[0-9_]`` and characters marked as
   alphanumeric in the Unicode character properties database.

``\Z``
   Matches only at the end of the string.

Most of the standard escapes supported by Python string literals are also
accepted by the regular expression parser::

   \a      \b      \f      \n
   \r      \t      \v      \x
   \\

Octal escapes are included in a limited form: If the first digit is a 0, or if
there are three octal digits, it is considered an octal escape. Otherwise, it is
a group reference.  As for string literals, octal escapes are always at most
three digits in length.

.. % Note the lack of a period in the section title; it causes problems
.. % with readers of the GNU info version.  See http://www.python.org/sf/581414.


.. _matching-searching:

Matching vs Searching
---------------------

.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


Python offers two different primitive operations based on regular expressions:
**match** checks for a match only at the beginning of the string, while
**search** checks for a match anywhere in the string (this is what Perl does
by default).

Note that match may differ from search even when using a regular expression
beginning with ``'^'``: ``'^'`` matches only at the start of the string, or in
:const:`MULTILINE` mode also immediately following a newline.  The "match"
operation succeeds only if the pattern matches at the start of the string
regardless of mode, or at the starting position given by the optional *pos*
argument regardless of whether a newline precedes it.

.. % Examples from Tim Peters:

::

   re.compile("a").match("ba", 1)           # succeeds
   re.compile("^a").search("ba", 1)         # fails; 'a' not at start
   re.compile("^a").search("\na", 1)        # fails; 'a' not at start
   re.compile("^a", re.M).search("\na", 1)  # succeeds
   re.compile("^a", re.M).search("ba", 1)   # fails; no preceding \n


.. _contents-of-module-re:

Module Contents
---------------

The module defines several functions, constants, and an exception. Some of the
functions are simplified versions of the full featured methods for compiled
regular expressions.  Most non-trivial applications always use the compiled
form.


.. function:: compile(pattern[, flags])

   Compile a regular expression pattern into a regular expression object, which can
   be used for matching using its :func:`match` and :func:`search` methods,
   described below.

   The expression's behaviour can be modified by specifying a *flags* value.
   Values can be any of the following variables, combined using bitwise OR (the
   ``|`` operator).

   The sequence ::

      prog = re.compile(pat)
      result = prog.match(str)

   is equivalent to ::

      result = re.match(pat, str)

   but the version using :func:`compile` is more efficient when the expression will
   be used several times in a single program.

   .. % (The compiled version of the last pattern passed to
   .. % \function{re.match()} or \function{re.search()} is cached, so
   .. % programs that use only a single regular expression at a time needn't
   .. % worry about compiling regular expressions.)


.. data:: I
          IGNORECASE

   Perform case-insensitive matching; expressions like ``[A-Z]`` will match
   lowercase letters, too.  This is not affected by the current locale.


.. data:: L
          LOCALE

   Make ``\w``, ``\W``, ``\b``, ``\B``, ``\s`` and ``\S`` dependent on the current
   locale.


.. data:: M
          MULTILINE

   When specified, the pattern character ``'^'`` matches at the beginning of the
   string and at the beginning of each line (immediately following each newline);
   and the pattern character ``'$'`` matches at the end of the string and at the
   end of each line (immediately preceding each newline).  By default, ``'^'``
   matches only at the beginning of the string, and ``'$'`` only at the end of the
   string and immediately before the newline (if any) at the end of the string.


.. data:: S
          DOTALL

   Make the ``'.'`` special character match any character at all, including a
   newline; without this flag, ``'.'`` will match anything *except* a newline.


.. data:: U
          UNICODE

   Make ``\w``, ``\W``, ``\b``, ``\B``, ``\d``, ``\D``, ``\s`` and ``\S`` dependent
   on the Unicode character properties database.


.. data:: X
          VERBOSE

   This flag allows you to write regular expressions that look nicer. Whitespace
   within the pattern is ignored, except when in a character class or preceded by
   an unescaped backslash, and, when a line contains a ``'#'`` neither in a
   character class or preceded by an unescaped backslash, all characters from the
   leftmost such ``'#'`` through the end of the line are ignored.

   This means that the two following regular expression objects are equal::

      re.compile(r""" [a-z]+   # some letters
                      \.\.     # two dots
                      [a-z]*   # perhaps more letters""")
      re.compile(r"[a-z]+\.\.[a-z]*")


.. function:: search(pattern, string[, flags])

   Scan through *string* looking for a location where the regular expression
   *pattern* produces a match, and return a corresponding :class:`MatchObject`
   instance. Return ``None`` if no position in the string matches the pattern; note
   that this is different from finding a zero-length match at some point in the
   string.


.. function:: match(pattern, string[, flags])

   If zero or more characters at the beginning of *string* match the regular
   expression *pattern*, return a corresponding :class:`MatchObject` instance.
   Return ``None`` if the string does not match the pattern; note that this is
   different from a zero-length match.

   .. note::

      If you want to locate a match anywhere in *string*, use :meth:`search` instead.


.. function:: split(pattern, string[, maxsplit=0])

   Split *string* by the occurrences of *pattern*.  If capturing parentheses are
   used in *pattern*, then the text of all groups in the pattern are also returned
   as part of the resulting list. If *maxsplit* is nonzero, at most *maxsplit*
   splits occur, and the remainder of the string is returned as the final element
   of the list.  (Incompatibility note: in the original Python 1.5 release,
   *maxsplit* was ignored.  This has been fixed in later releases.) ::

      >>> re.split('\W+', 'Words, words, words.')
      ['Words', 'words', 'words', '']
      >>> re.split('(\W+)', 'Words, words, words.')
      ['Words', ', ', 'words', ', ', 'words', '.', '']
      >>> re.split('\W+', 'Words, words, words.', 1)
      ['Words', 'words, words.']


.. function:: findall(pattern, string[, flags])

   Return a list of all non-overlapping matches of *pattern* in *string*.  If one
   or more groups are present in the pattern, return a list of groups; this will be
   a list of tuples if the pattern has more than one group.  Empty matches are
   included in the result unless they touch the beginning of another match.


.. function:: finditer(pattern, string[, flags])

   Return an iterator over all non-overlapping matches for the RE *pattern* in
   *string*.  For each match, the iterator returns a match object.  Empty matches
   are included in the result unless they touch the beginning of another match.


.. function:: sub(pattern, repl, string[, count])

   Return the string obtained by replacing the leftmost non-overlapping occurrences
   of *pattern* in *string* by the replacement *repl*.  If the pattern isn't found,
   *string* is returned unchanged.  *repl* can be a string or a function; if it is
   a string, any backslash escapes in it are processed.  That is, ``\n`` is
   converted to a single newline character, ``\r`` is converted to a linefeed, and
   so forth.  Unknown escapes such as ``\j`` are left alone.  Backreferences, such
   as ``\6``, are replaced with the substring matched by group 6 in the pattern.
   For example::

      >>> re.sub(r'def\s+([a-zA-Z_][a-zA-Z_0-9]*)\s*\(\s*\):',
      ...        r'static PyObject*\npy_\1(void)\n{',
      ...        'def myfunc():')
      'static PyObject*\npy_myfunc(void)\n{'

   If *repl* is a function, it is called for every non-overlapping occurrence of
   *pattern*.  The function takes a single match object argument, and returns the
   replacement string.  For example::

      >>> def dashrepl(matchobj):
      ...     if matchobj.group(0) == '-': return ' '
      ...     else: return '-'
      >>> re.sub('-{1,2}', dashrepl, 'pro----gram-files')
      'pro--gram files'

   The pattern may be a string or an RE object; if you need to specify regular
   expression flags, you must use a RE object, or use embedded modifiers in a
   pattern; for example, ``sub("(?i)b+", "x", "bbbb BBBB")`` returns ``'x x'``.

   The optional argument *count* is the maximum number of pattern occurrences to be
   replaced; *count* must be a non-negative integer.  If omitted or zero, all
   occurrences will be replaced. Empty matches for the pattern are replaced only
   when not adjacent to a previous match, so ``sub('x*', '-', 'abc')`` returns
   ``'-a-b-c-'``.

   In addition to character escapes and backreferences as described above,
   ``\g<name>`` will use the substring matched by the group named ``name``, as
   defined by the ``(?P<name>...)`` syntax. ``\g<number>`` uses the corresponding
   group number; ``\g<2>`` is therefore equivalent to ``\2``, but isn't ambiguous
   in a replacement such as ``\g<2>0``.  ``\20`` would be interpreted as a
   reference to group 20, not a reference to group 2 followed by the literal
   character ``'0'``.  The backreference ``\g<0>`` substitutes in the entire
   substring matched by the RE.


.. function:: subn(pattern, repl, string[, count])

   Perform the same operation as :func:`sub`, but return a tuple ``(new_string,
   number_of_subs_made)``.


.. function:: escape(string)

   Return *string* with all non-alphanumerics backslashed; this is useful if you
   want to match an arbitrary literal string that may have regular expression
   metacharacters in it.


.. exception:: error

   Exception raised when a string passed to one of the functions here is not a
   valid regular expression (for example, it might contain unmatched parentheses)
   or when some other error occurs during compilation or matching.  It is never an
   error if a string contains no match for a pattern.


.. _re-objects:

Regular Expression Objects
--------------------------

Compiled regular expression objects support the following methods and
attributes:


.. method:: RegexObject.match(string[, pos[, endpos]])

   If zero or more characters at the beginning of *string* match this regular
   expression, return a corresponding :class:`MatchObject` instance.  Return
   ``None`` if the string does not match the pattern; note that this is different
   from a zero-length match.

   .. note::

      If you want to locate a match anywhere in *string*, use :meth:`search` instead.

   The optional second parameter *pos* gives an index in the string where the
   search is to start; it defaults to ``0``.  This is not completely equivalent to
   slicing the string; the ``'^'`` pattern character matches at the real beginning
   of the string and at positions just after a newline, but not necessarily at the
   index where the search is to start.

   The optional parameter *endpos* limits how far the string will be searched; it
   will be as if the string is *endpos* characters long, so only the characters
   from *pos* to ``endpos - 1`` will be searched for a match.  If *endpos* is less
   than *pos*, no match will be found, otherwise, if *rx* is a compiled regular
   expression object, ``rx.match(string, 0, 50)`` is equivalent to
   ``rx.match(string[:50], 0)``.


.. method:: RegexObject.search(string[, pos[, endpos]])

   Scan through *string* looking for a location where this regular expression
   produces a match, and return a corresponding :class:`MatchObject` instance.
   Return ``None`` if no position in the string matches the pattern; note that this
   is different from finding a zero-length match at some point in the string.

   The optional *pos* and *endpos* parameters have the same meaning as for the
   :meth:`match` method.


.. method:: RegexObject.split(string[, maxsplit=0])

   Identical to the :func:`split` function, using the compiled pattern.


.. method:: RegexObject.findall(string[, pos[, endpos]])

   Identical to the :func:`findall` function, using the compiled pattern.


.. method:: RegexObject.finditer(string[, pos[, endpos]])

   Identical to the :func:`finditer` function, using the compiled pattern.


.. method:: RegexObject.sub(repl, string[, count=0])

   Identical to the :func:`sub` function, using the compiled pattern.


.. method:: RegexObject.subn(repl, string[, count=0])

   Identical to the :func:`subn` function, using the compiled pattern.


.. attribute:: RegexObject.flags

   The flags argument used when the RE object was compiled, or ``0`` if no flags
   were provided.


.. attribute:: RegexObject.groupindex

   A dictionary mapping any symbolic group names defined by ``(?P<id>)`` to group
   numbers.  The dictionary is empty if no symbolic groups were used in the
   pattern.


.. attribute:: RegexObject.pattern

   The pattern string from which the RE object was compiled.


.. _match-objects:

Match Objects
-------------

:class:`MatchObject` instances support the following methods and attributes:


.. method:: MatchObject.expand(template)

   Return the string obtained by doing backslash substitution on the template
   string *template*, as done by the :meth:`sub` method. Escapes such as ``\n`` are
   converted to the appropriate characters, and numeric backreferences (``\1``,
   ``\2``) and named backreferences (``\g<1>``, ``\g<name>``) are replaced by the
   contents of the corresponding group.


.. method:: MatchObject.group([group1, ...])

   Returns one or more subgroups of the match.  If there is a single argument, the
   result is a single string; if there are multiple arguments, the result is a
   tuple with one item per argument. Without arguments, *group1* defaults to zero
   (the whole match is returned). If a *groupN* argument is zero, the corresponding
   return value is the entire matching string; if it is in the inclusive range
   [1..99], it is the string matching the corresponding parenthesized group.  If a
   group number is negative or larger than the number of groups defined in the
   pattern, an :exc:`IndexError` exception is raised. If a group is contained in a
   part of the pattern that did not match, the corresponding result is ``None``.
   If a group is contained in a part of the pattern that matched multiple times,
   the last match is returned.

   If the regular expression uses the ``(?P<name>...)`` syntax, the *groupN*
   arguments may also be strings identifying groups by their group name.  If a
   string argument is not used as a group name in the pattern, an :exc:`IndexError`
   exception is raised.

   A moderately complicated example::

      m = re.match(r"(?P<int>\d+)\.(\d*)", '3.14')

   After performing this match, ``m.group(1)`` is ``'3'``, as is
   ``m.group('int')``, and ``m.group(2)`` is ``'14'``.


.. method:: MatchObject.groups([default])

   Return a tuple containing all the subgroups of the match, from 1 up to however
   many groups are in the pattern.  The *default* argument is used for groups that
   did not participate in the match; it defaults to ``None``.  (Incompatibility
   note: in the original Python 1.5 release, if the tuple was one element long, a
   string would be returned instead.  In later versions (from 1.5.1 on), a
   singleton tuple is returned in such cases.)


.. method:: MatchObject.groupdict([default])

   Return a dictionary containing all the *named* subgroups of the match, keyed by
   the subgroup name.  The *default* argument is used for groups that did not
   participate in the match; it defaults to ``None``.


.. method:: MatchObject.start([group])
            MatchObject.end([group])

   Return the indices of the start and end of the substring matched by *group*;
   *group* defaults to zero (meaning the whole matched substring). Return ``-1`` if
   *group* exists but did not contribute to the match.  For a match object *m*, and
   a group *g* that did contribute to the match, the substring matched by group *g*
   (equivalent to ``m.group(g)``) is ::

      m.string[m.start(g):m.end(g)]

   Note that ``m.start(group)`` will equal ``m.end(group)`` if *group* matched a
   null string.  For example, after ``m = re.search('b(c?)', 'cba')``,
   ``m.start(0)`` is 1, ``m.end(0)`` is 2, ``m.start(1)`` and ``m.end(1)`` are both
   2, and ``m.start(2)`` raises an :exc:`IndexError` exception.


.. method:: MatchObject.span([group])

   For :class:`MatchObject` *m*, return the 2-tuple ``(m.start(group),
   m.end(group))``. Note that if *group* did not contribute to the match, this is
   ``(-1, -1)``.  Again, *group* defaults to zero.


.. attribute:: MatchObject.pos

   The value of *pos* which was passed to the :func:`search` or :func:`match`
   method of the :class:`RegexObject`.  This is the index into the string at which
   the RE engine started looking for a match.


.. attribute:: MatchObject.endpos

   The value of *endpos* which was passed to the :func:`search` or :func:`match`
   method of the :class:`RegexObject`.  This is the index into the string beyond
   which the RE engine will not go.


.. attribute:: MatchObject.lastindex

   The integer index of the last matched capturing group, or ``None`` if no group
   was matched at all. For example, the expressions ``(a)b``, ``((a)(b))``, and
   ``((ab))`` will have ``lastindex == 1`` if applied to the string ``'ab'``, while
   the expression ``(a)(b)`` will have ``lastindex == 2``, if applied to the same
   string.


.. attribute:: MatchObject.lastgroup

   The name of the last matched capturing group, or ``None`` if the group didn't
   have a name, or if no group was matched at all.


.. attribute:: MatchObject.re

   The regular expression object whose :meth:`match` or :meth:`search` method
   produced this :class:`MatchObject` instance.


.. attribute:: MatchObject.string

   The string passed to :func:`match` or :func:`search`.


Examples
--------

**Simulating scanf()**

.. index:: single: scanf()

Python does not currently have an equivalent to :cfunc:`scanf`.  Regular
expressions are generally more powerful, though also more verbose, than
:cfunc:`scanf` format strings.  The table below offers some more-or-less
equivalent mappings between :cfunc:`scanf` format tokens and regular
expressions.

+--------------------------------+---------------------------------------------+
| :cfunc:`scanf` Token           | Regular Expression                          |
+================================+=============================================+
| ``%c``                         | ``.``                                       |
+--------------------------------+---------------------------------------------+
| ``%5c``                        | ``.{5}``                                    |
+--------------------------------+---------------------------------------------+
| ``%d``                         | ``[-+]?\d+``                                |
+--------------------------------+---------------------------------------------+
| ``%e``, ``%E``, ``%f``, ``%g`` | ``[-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?`` |
+--------------------------------+---------------------------------------------+
| ``%i``                         | ``[-+]?(0[xX][\dA-Fa-f]+|0[0-7]*|\d+)``     |
+--------------------------------+---------------------------------------------+
| ``%o``                         | ``0[0-7]*``                                 |
+--------------------------------+---------------------------------------------+
| ``%s``                         | ``\S+``                                     |
+--------------------------------+---------------------------------------------+
| ``%u``                         | ``\d+``                                     |
+--------------------------------+---------------------------------------------+
| ``%x``, ``%X``                 | ``0[xX][\dA-Fa-f]+``                        |
+--------------------------------+---------------------------------------------+

To extract the filename and numbers from a string like ::

   /usr/sbin/sendmail - 0 errors, 4 warnings

you would use a :cfunc:`scanf` format like ::

   %s - %d errors, %d warnings

The equivalent regular expression would be ::

   (\S+) - (\d+) errors, (\d+) warnings

**Avoiding recursion**

If you create regular expressions that require the engine to perform a lot of
recursion, you may encounter a :exc:`RuntimeError` exception with the message
``maximum recursion limit`` exceeded. For example, ::

   >>> import re
   >>> s = 'Begin ' + 1000*'a very long string ' + 'end'
   >>> re.match('Begin (\w| )*? end', s).end()
   Traceback (most recent call last):
     File "<stdin>", line 1, in ?
     File "/usr/local/lib/python2.5/re.py", line 132, in match
       return _compile(pattern, flags).match(string)
   RuntimeError: maximum recursion limit exceeded

You can often restructure your regular expression to avoid recursion.

Starting with Python 2.3, simple uses of the ``*?`` pattern are special-cased to
avoid recursion.  Thus, the above regular expression can avoid recursion by
being recast as ``Begin [a-zA-Z0-9_ ]*?end``.  As a further benefit, such
regular expressions will run faster than their recursive equivalents.

