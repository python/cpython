
:mod:`os.path` --- Common pathname manipulations
================================================

.. module:: os.path
   :synopsis: Operations on pathnames.


.. index:: single: path; operations

This module implements some useful functions on pathnames. To read or
write files see :func:`open`, and for accessing the filesystem see the
:mod:`os` module.

.. warning::

   On Windows, many of these functions do not properly support UNC pathnames.
   :func:`splitunc` and :func:`ismount` do handle them correctly.


.. function:: abspath(path)

   Return a normalized absolutized version of the pathname *path*. On most
   platforms, this is equivalent to ``normpath(join(os.getcwd(), path))``.

   .. versionadded:: 1.5.2


.. function:: basename(path)

   Return the base name of pathname *path*.  This is the second half of the pair
   returned by ``split(path)``.  Note that the result of this function is different
   from the Unix :program:`basename` program; where :program:`basename` for
   ``'/foo/bar/'`` returns ``'bar'``, the :func:`basename` function returns an
   empty string (``''``).


.. function:: commonprefix(list)

   Return the longest path prefix (taken character-by-character) that is a prefix
   of all paths in  *list*.  If *list* is empty, return the empty string (``''``).
   Note that this may return invalid paths because it works a character at a time.


.. function:: dirname(path)

   Return the directory name of pathname *path*.  This is the first half of the
   pair returned by ``split(path)``.


.. function:: exists(path)

   Return ``True`` if *path* refers to an existing path.  Returns ``False`` for
   broken symbolic links. On some platforms, this function may return ``False`` if
   permission is not granted to execute :func:`os.stat` on the requested file, even
   if the *path* physically exists.


.. function:: lexists(path)

   Return ``True`` if *path* refers to an existing path. Returns ``True`` for
   broken symbolic links.   Equivalent to :func:`exists` on platforms lacking
   :func:`os.lstat`.

   .. versionadded:: 2.4


.. function:: expanduser(path)

   On Unix and Windows, return the argument with an initial component of ``~`` or
   ``~user`` replaced by that *user*'s home directory.

   .. index:: module: pwd

   On Unix, an initial ``~`` is replaced by the environment variable :envvar:`HOME`
   if it is set; otherwise the current user's home directory is looked up in the
   password directory through the built-in module :mod:`pwd`. An initial ``~user``
   is looked up directly in the password directory.

   On Windows, :envvar:`HOME` and :envvar:`USERPROFILE` will be used if set,
   otherwise a combination of :envvar:`HOMEPATH` and :envvar:`HOMEDRIVE` will be
   used.  An initial ``~user`` is handled by stripping the last directory component
   from the created user path derived above.

   If the expansion fails or if the path does not begin with a tilde, the path is
   returned unchanged.


.. function:: expandvars(path)

   Return the argument with environment variables expanded.  Substrings of the form
   ``$name`` or ``${name}`` are replaced by the value of environment variable
   *name*.  Malformed variable names and references to non-existing variables are
   left unchanged.

   On Windows, ``%name%`` expansions are supported in addition to ``$name`` and
   ``${name}``.


.. function:: getatime(path)

   Return the time of last access of *path*.  The return value is a number giving
   the number of seconds since the epoch (see the  :mod:`time` module).  Raise
   :exc:`os.error` if the file does not exist or is inaccessible.

   .. versionadded:: 1.5.2

   .. versionchanged:: 2.3
      If :func:`os.stat_float_times` returns True, the result is a floating point
      number.


.. function:: getmtime(path)

   Return the time of last modification of *path*.  The return value is a number
   giving the number of seconds since the epoch (see the  :mod:`time` module).
   Raise :exc:`os.error` if the file does not exist or is inaccessible.

   .. versionadded:: 1.5.2

   .. versionchanged:: 2.3
      If :func:`os.stat_float_times` returns True, the result is a floating point
      number.


.. function:: getctime(path)

   Return the system's ctime which, on some systems (like Unix) is the time of the
   last change, and, on others (like Windows), is the creation time for *path*.
   The return value is a number giving the number of seconds since the epoch (see
   the  :mod:`time` module).  Raise :exc:`os.error` if the file does not exist or
   is inaccessible.

   .. versionadded:: 2.3


.. function:: getsize(path)

   Return the size, in bytes, of *path*.  Raise :exc:`os.error` if the file does
   not exist or is inaccessible.

   .. versionadded:: 1.5.2


.. function:: isabs(path)

   Return ``True`` if *path* is an absolute pathname (begins with a slash).


.. function:: isfile(path)

   Return ``True`` if *path* is an existing regular file.  This follows symbolic
   links, so both :func:`islink` and :func:`isfile` can be true for the same path.


.. function:: isdir(path)

   Return ``True`` if *path* is an existing directory.  This follows symbolic
   links, so both :func:`islink` and :func:`isdir` can be true for the same path.


.. function:: islink(path)

   Return ``True`` if *path* refers to a directory entry that is a symbolic link.
   Always ``False`` if symbolic links are not supported.


.. function:: ismount(path)

   Return ``True`` if pathname *path* is a :dfn:`mount point`: a point in a file
   system where a different file system has been mounted.  The function checks
   whether *path*'s parent, :file:`path/..`, is on a different device than *path*,
   or whether :file:`path/..` and *path* point to the same i-node on the same
   device --- this should detect mount points for all Unix and POSIX variants.


.. function:: join(path1[, path2[, ...]])

   Join one or more path components intelligently.  If any component is an absolute
   path, all previous components (on Windows, including the previous drive letter,
   if there was one) are thrown away, and joining continues.  The return value is
   the concatenation of *path1*, and optionally *path2*, etc., with exactly one
   directory separator (``os.sep``) inserted between components, unless *path2* is
   empty.  Note that on Windows, since there is a current directory for each drive,
   ``os.path.join("c:", "foo")`` represents a path relative to the current
   directory on drive :file:`C:` (:file:`c:foo`), not :file:`c:\\foo`.


.. function:: normcase(path)

   Normalize the case of a pathname.  On Unix, this returns the path unchanged; on
   case-insensitive filesystems, it converts the path to lowercase.  On Windows, it
   also converts forward slashes to backward slashes.


.. function:: normpath(path)

   Normalize a pathname.  This collapses redundant separators and up-level
   references so that ``A//B``, ``A/./B`` and ``A/foo/../B`` all become ``A/B``.
   It does not normalize the case (use :func:`normcase` for that).  On Windows, it
   converts forward slashes to backward slashes. It should be understood that this
   may change the meaning of the path if it contains symbolic links!


.. function:: realpath(path)

   Return the canonical path of the specified filename, eliminating any symbolic
   links encountered in the path (if they are supported by the operating system).

   .. versionadded:: 2.2


.. function:: relpath(path[, start])

   Return a relative filepath to *path* either from the current directory or from
   an optional *start* point.

   *start* defaults to :attr:`os.curdir`. Availability:  Windows, Unix.

   .. versionadded:: 2.6


.. function:: samefile(path1, path2)

   Return ``True`` if both pathname arguments refer to the same file or directory
   (as indicated by device number and i-node number). Raise an exception if a
   :func:`os.stat` call on either pathname fails. Availability:  Macintosh, Unix.


.. function:: sameopenfile(fp1, fp2)

   Return ``True`` if the file descriptors *fp1* and *fp2* refer to the same file.
   Availability:  Macintosh, Unix.


.. function:: samestat(stat1, stat2)

   Return ``True`` if the stat tuples *stat1* and *stat2* refer to the same file.
   These structures may have been returned by :func:`fstat`, :func:`lstat`, or
   :func:`stat`.  This function implements the underlying comparison used by
   :func:`samefile` and :func:`sameopenfile`. Availability:  Macintosh, Unix.


.. function:: split(path)

   Split the pathname *path* into a pair, ``(head, tail)`` where *tail* is the last
   pathname component and *head* is everything leading up to that.  The *tail* part
   will never contain a slash; if *path* ends in a slash, *tail* will be empty.  If
   there is no slash in *path*, *head* will be empty.  If *path* is empty, both
   *head* and *tail* are empty.  Trailing slashes are stripped from *head* unless
   it is the root (one or more slashes only).  In nearly all cases, ``join(head,
   tail)`` equals *path* (the only exception being when there were multiple slashes
   separating *head* from *tail*).


.. function:: splitdrive(path)

   Split the pathname *path* into a pair ``(drive, tail)`` where *drive* is either
   a drive specification or the empty string.  On systems which do not use drive
   specifications, *drive* will always be the empty string.  In all cases, ``drive
   + tail`` will be the same as *path*.

   .. versionadded:: 1.3


.. function:: splitext(path)

   Split the pathname *path* into a pair ``(root, ext)``  such that ``root + ext ==
   path``, and *ext* is empty or begins with a period and contains at most one
   period. Leading periods on the basename are  ignored; ``splitext('.cshrc')``
   returns  ``('.cshrc', '')``.

   .. versionchanged:: 2.6
      Earlier versions could produce an empty root when the only period was the
      first character.


.. function:: splitunc(path)

   Split the pathname *path* into a pair ``(unc, rest)`` so that *unc* is the UNC
   mount point (such as ``r'\\host\mount'``), if present, and *rest* the rest of
   the path (such as  ``r'\path\file.ext'``).  For paths containing drive letters,
   *unc* will always be the empty string. Availability:  Windows.


.. function:: walk(path, visit, arg)

   Calls the function *visit* with arguments ``(arg, dirname, names)`` for each
   directory in the directory tree rooted at *path* (including *path* itself, if it
   is a directory).  The argument *dirname* specifies the visited directory, the
   argument *names* lists the files in the directory (gotten from
   ``os.listdir(dirname)``). The *visit* function may modify *names* to influence
   the set of directories visited below *dirname*, e.g. to avoid visiting certain
   parts of the tree.  (The object referred to by *names* must be modified in
   place, using :keyword:`del` or slice assignment.)

   .. note::

      Symbolic links to directories are not treated as subdirectories, and that
      :func:`walk` therefore will not visit them. To visit linked directories you must
      identify them with ``os.path.islink(file)`` and ``os.path.isdir(file)``, and
      invoke :func:`walk` as necessary.

   .. note::

      The newer :func:`os.walk` generator supplies similar functionality and can be
      easier to use.


.. data:: supports_unicode_filenames

   True if arbitrary Unicode strings can be used as file names (within limitations
   imposed by the file system), and if :func:`os.listdir` returns Unicode strings
   for a Unicode argument.

   .. versionadded:: 2.3

