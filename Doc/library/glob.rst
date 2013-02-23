:mod:`glob` --- Unix style pathname pattern expansion
=====================================================

.. module:: glob
   :synopsis: Unix shell style pathname pattern expansion.


.. index:: single: filenames; pathname expansion

**Source code:** :source:`Lib/glob.py`

--------------

The :mod:`glob` module finds all the pathnames matching a specified pattern
according to the rules used by the Unix shell.  No tilde expansion is done, but
``*``, ``?``, and character ranges expressed with ``[]`` will be correctly
matched.  This is done by using the :func:`os.listdir` and
:func:`fnmatch.fnmatch` functions in concert, and not by actually invoking a
subshell.  Note that unlike :func:`fnmatch.fnmatch`, :mod:`glob` treats
filenames beginning with a dot (``.``) as special cases.  (For tilde and shell
variable expansion, use :func:`os.path.expanduser` and
:func:`os.path.expandvars`.)

For a literal match, wrap the meta-characters in brackets.
For example, ``'[?]'`` matches the character ``'?'``.


.. function:: glob(pathname)

   Return a possibly-empty list of path names that match *pathname*, which must be
   a string containing a path specification. *pathname* can be either absolute
   (like :file:`/usr/src/Python-1.5/Makefile`) or relative (like
   :file:`../../Tools/\*/\*.gif`), and can contain shell-style wildcards. Broken
   symlinks are included in the results (as in the shell).


.. function:: iglob(pathname)

   Return an :term:`iterator` which yields the same values as :func:`glob`
   without actually storing them all simultaneously.

   .. versionadded:: 2.5

For example, consider a directory containing only the following files:
:file:`1.gif`, :file:`2.txt`, and :file:`card.gif`.  :func:`glob` will produce
the following results.  Notice how any leading components of the path are
preserved. ::

   >>> import glob
   >>> glob.glob('./[0-9].*')
   ['./1.gif', './2.txt']
   >>> glob.glob('*.gif')
   ['1.gif', 'card.gif']
   >>> glob.glob('?.gif')
   ['1.gif']

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

