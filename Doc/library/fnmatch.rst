:mod:`!fnmatch` --- Unix filename pattern matching
==================================================

.. module:: fnmatch
   :synopsis: Unix shell style filename pattern matching.

**Source code:** :source:`Lib/fnmatch.py`

.. index:: single: filenames; wildcard expansion

.. index:: pair: module; re

--------------

This module provides support for Unix shell-style wildcards, which are *not* the
same as regular expressions (which are documented in the :mod:`re` module).  The
special characters used in shell-style wildcards are:

.. index::
   single: * (asterisk); in glob-style wildcards
   single: ? (question mark); in glob-style wildcards
   single: [] (square brackets); in glob-style wildcards
   single: ! (exclamation); in glob-style wildcards
   single: - (minus); in glob-style wildcards

+------------+------------------------------------+
| Pattern    | Meaning                            |
+============+====================================+
| ``*``      | matches everything                 |
+------------+------------------------------------+
| ``?``      | matches any single character       |
+------------+------------------------------------+
| ``[seq]``  | matches any character in *seq*     |
+------------+------------------------------------+
| ``[!seq]`` | matches any character not in *seq* |
+------------+------------------------------------+

For a literal match, wrap the meta-characters in brackets.
For example, ``'[?]'`` matches the character ``'?'``.

.. index:: pair: module; glob

Note that the filename separator (``'/'`` on Unix) is *not* special to this
module.  See module :mod:`glob` for pathname expansion (:mod:`glob` uses
:func:`.filter` to match pathname segments).  Similarly, filenames starting with
a period are not special for this module, and are matched by the ``*`` and ``?``
patterns.

Unless stated otherwise, "filename string" and "pattern string" either refer to
:class:`str` or ``ISO-8859-1`` encoded :class:`bytes` objects. Note that the
functions documented below do not allow to mix a :class:`!bytes` pattern with
a :class:`!str` filename, and vice-versa.

Finally, note that :func:`functools.lru_cache` with a *maxsize* of 32768
is used to cache the (typed) compiled regex patterns in the following
functions: :func:`fnmatch`, :func:`fnmatchcase`, :func:`.filter`, :func:`.filterfalse`.


.. function:: fnmatch(name, pat)

   Test whether the filename string *name* matches the pattern string *pat*,
   returning ``True`` or ``False``.  Both parameters are case-normalized
   using :func:`os.path.normcase`. :func:`fnmatchcase` can be used to perform a
   case-sensitive comparison, regardless of whether that's standard for the
   operating system.

   This example will print all file names in the current directory with the
   extension ``.txt``::

      import fnmatch
      import os

      for file in os.listdir('.'):
          if fnmatch.fnmatch(file, '*.txt'):
              print(file)


.. function:: fnmatchcase(name, pat)

   Test whether the filename string *name* matches the pattern string *pat*,
   returning ``True`` or ``False``;
   the comparison is case-sensitive and does not apply :func:`os.path.normcase`.


.. function:: filter(names, pat)

   Construct a list from those elements of the :term:`iterable` of filename
   strings *names* that match the pattern string *pat*.
   It is the same as ``[n for n in names if fnmatch(n, pat)]``,
   but implemented more efficiently.


.. function:: filterfalse(names, pat)

   Construct a list from those elements of the :term:`iterable` of filename
   strings *names* that do not match the pattern string *pat*.
   It is the same as ``[n for n in names if not fnmatch(n, pat)]``,
   but implemented more efficiently.

   .. versionadded:: 3.14


.. function:: translate(pat)

   Return the shell-style pattern *pat* converted to a regular expression for
   using with :func:`re.match`. The pattern is expected to be a :class:`str`.

   Example:

      >>> import fnmatch, re
      >>>
      >>> regex = fnmatch.translate('*.txt')
      >>> regex
      '(?s:.*\\.txt)\\z'
      >>> reobj = re.compile(regex)
      >>> reobj.match('foobar.txt')
      <re.Match object; span=(0, 10), match='foobar.txt'>


.. seealso::

   Module :mod:`glob`
      Unix shell-style path expansion.
