:mod:`glob` --- Unix style pathname pattern expansion
=====================================================

.. module:: glob
   :synopsis: Unix shell style pathname pattern expansion.

**Source code:** :source:`Lib/glob.py`

.. index:: single: filenames; pathname expansion

--------------

.. index::
   single: * (asterisk); in glob-style wildcards
   single: ? (question mark); in glob-style wildcards
   single: [] (square brackets); in glob-style wildcards
   single: ! (exclamation); in glob-style wildcards
   single: - (minus); in glob-style wildcards
   single: . (dot); in glob-style wildcards

The :mod:`glob` module finds all the pathnames matching a specified pattern
according to the rules used by the Unix shell, although results are returned in
arbitrary order.  No tilde expansion is done, but ``*``, ``?``, and character
ranges expressed with ``[]`` will be correctly matched.  This is done by using
the :func:`os.scandir` and :func:`fnmatch.fnmatch` functions in concert, and
not by actually invoking a subshell.

Note that files beginning with a dot (``.``) can only be matched by
patterns that also start with a dot,
unlike :func:`fnmatch.fnmatch` or :func:`pathlib.Path.glob`.
(For tilde and shell variable expansion, use :func:`os.path.expanduser` and
:func:`os.path.expandvars`.)

For a literal match, wrap the meta-characters in brackets.
For example, ``'[?]'`` matches the character ``'?'``.

The :mod:`glob` module defines the following functions:


.. function:: glob(pathname, *, root_dir=None, dir_fd=None, recursive=False, \
                   include_hidden=False)

   Return a possibly empty list of path names that match *pathname*, which must be
   a string containing a path specification. *pathname* can be either absolute
   (like :file:`/usr/src/Python-1.5/Makefile`) or relative (like
   :file:`../../Tools/\*/\*.gif`), and can contain shell-style wildcards. Broken
   symlinks are included in the results (as in the shell). Whether or not the
   results are sorted depends on the file system.  If a file that satisfies
   conditions is removed or added during the call of this function, whether
   a path name for that file will be included is unspecified.

   If *root_dir* is not ``None``, it should be a :term:`path-like object`
   specifying the root directory for searching.  It has the same effect on
   :func:`glob` as changing the current directory before calling it.  If
   *pathname* is relative, the result will contain paths relative to
   *root_dir*.

   This function can support :ref:`paths relative to directory descriptors
   <dir_fd>` with the *dir_fd* parameter.

   .. index::
      single: **; in glob-style wildcards

   If *recursive* is true, the pattern "``**``" will match any files and zero or
   more directories, subdirectories and symbolic links to directories. If the
   pattern is followed by an :data:`os.sep` or :data:`os.altsep` then files will not
   match.

   If *include_hidden* is true, "``**``" pattern will match hidden directories.

   .. audit-event:: glob.glob pathname,recursive glob.glob
   .. audit-event:: glob.glob/2 pathname,recursive,root_dir,dir_fd glob.glob

   .. note::
      Using the "``**``" pattern in large directory trees may consume
      an inordinate amount of time.

   .. versionchanged:: 3.5
      Support for recursive globs using "``**``".

   .. versionchanged:: 3.10
      Added the *root_dir* and *dir_fd* parameters.

   .. versionchanged:: 3.11
      Added the *include_hidden* parameter.


.. function:: iglob(pathname, *, root_dir=None, dir_fd=None, recursive=False, \
                    include_hidden=False)

   Return an :term:`iterator` which yields the same values as :func:`glob`
   without actually storing them all simultaneously.

   .. audit-event:: glob.glob pathname,recursive glob.iglob
   .. audit-event:: glob.glob/2 pathname,recursive,root_dir,dir_fd glob.iglob

   .. versionchanged:: 3.5
      Support for recursive globs using "``**``".

   .. versionchanged:: 3.10
      Added the *root_dir* and *dir_fd* parameters.

   .. versionchanged:: 3.11
      Added the *include_hidden* parameter.


.. function:: escape(pathname)

   Escape all special characters (``'?'``, ``'*'`` and ``'['``).
   This is useful if you want to match an arbitrary literal string that may
   have special characters in it.  Special characters in drive/UNC
   sharepoints are not escaped, e.g. on Windows
   ``escape('//?/c:/Quo vadis?.txt')`` returns ``'//?/c:/Quo vadis[?].txt'``.

   .. versionadded:: 3.4


.. function:: translate(pathname, *, recursive=False, include_hidden=False, seps=None)

   Convert the given path specification to a regular expression for use with
   :func:`re.match`. The path specification can contain shell-style wildcards.

   For example:

      >>> import glob, re
      >>>
      >>> regex = glob.translate('**/*.txt', recursive=True, include_hidden=True)
      >>> regex
      '(?s:(?:.+/)?[^/]*\\.txt)\\Z'
      >>> reobj = re.compile(regex)
      >>> reobj.match('foo/bar/baz.txt')
      <re.Match object; span=(0, 15), match='foo/bar/baz.txt'>

   Path separators and segments are meaningful to this function, unlike
   :func:`fnmatch.translate`. By default wildcards do not match path
   separators, and ``*`` pattern segments match precisely one path segment.

   If *recursive* is true, the pattern segment "``**``" will match any number
   of path segments. If "``**``" occurs in any position other than a full
   pattern segment, :exc:`ValueError` is raised.

   If *include_hidden* is true, wildcards can match path segments that start
   with a dot (``.``).

   A sequence of path separators may be supplied to the *seps* argument. If
   not given, :data:`os.sep` and :data:`~os.altsep` (if available) are used.

   .. seealso::

     :meth:`pathlib.PurePath.match` and :meth:`pathlib.Path.glob` methods,
     which call this function to implement pattern matching and globbing.

   .. versionadded:: 3.13


Examples
--------

Consider a directory containing the following files:
:file:`1.gif`, :file:`2.txt`, :file:`card.gif` and a subdirectory :file:`sub`
which contains only the file :file:`3.txt`.  :func:`glob` will produce
the following results.  Notice how any leading components of the path are
preserved. ::

   >>> import glob
   >>> glob.glob('./[0-9].*')
   ['./1.gif', './2.txt']
   >>> glob.glob('*.gif')
   ['1.gif', 'card.gif']
   >>> glob.glob('?.gif')
   ['1.gif']
   >>> glob.glob('**/*.txt', recursive=True)
   ['2.txt', 'sub/3.txt']
   >>> glob.glob('./**/', recursive=True)
   ['./', './sub/']

If the directory contains files starting with ``.`` they won't be matched by
default. For example, consider a directory containing :file:`card.gif` and
:file:`.card.gif`::

   >>> import glob
   >>> glob.glob('*.gif')
   ['card.gif']
   >>> glob.glob('.c*')
   ['.card.gif']

.. seealso::
   The :mod:`fnmatch` module offers shell-style filename (not path) expansion.

.. seealso::
   The :mod:`pathlib` module offers high-level path objects.
