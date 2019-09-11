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
not by actually invoking a subshell.  Note that unlike :func:`fnmatch.fnmatch`,
:mod:`glob` treats filenames beginning with a dot (``.``) as special cases.
(For tilde and shell variable expansion, use :func:`os.path.expanduser` and
:func:`os.path.expandvars`.)

For a literal match, wrap the meta-characters in brackets.
For example, ``'[?]'`` matches the character ``'?'``.


.. seealso::
   The :mod:`pathlib` module offers high-level path objects.


.. function:: glob(pathname, *, recursive=False)

   Return a possibly-empty list of path names that match *pathname*, which must be
   a string containing a path specification. *pathname* can be either absolute
   (like :file:`/usr/src/Python-1.5/Makefile`) or relative (like
   :file:`../../Tools/\*/\*.gif`), and can contain shell-style wildcards. Broken
   symlinks are included in the results (as in the shell).

   .. index::
      single: **; in glob-style wildcards

   If *recursive* is true, the pattern "``**``" will match any files and zero or
   more directories, subdirectories and symbolic links to directories. If the
   pattern is followed by an :data:`os.sep` or :data:`os.altsep` then files will not
   match.

   .. note::
      Using the "``**``" pattern in large directory trees may consume
      an inordinate amount of time.

   .. versionchanged:: 3.5
      Support for recursive globs using "``**``".


.. function:: iglob(pathname, *, recursive=False)

   Return an :term:`iterator` which yields the same values as :func:`glob`
   without actually storing them all simultaneously.


.. function:: escape(pathname)

   Escape all special characters (``'?'``, ``'*'`` and ``'['``).
   This is useful if you want to match an arbitrary literal string that may
   have special characters in it.  Special characters in drive/UNC
   sharepoints are not escaped, e.g. on Windows
   ``escape('//?/c:/Quo vadis?.txt')`` returns ``'//?/c:/Quo vadis[?].txt'``.

   .. versionadded:: 3.4


For example, consider a directory containing the following files:
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

   Module :mod:`fnmatch`
      Shell-style filename (not path) expansion

