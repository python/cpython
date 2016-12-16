:mod:`shutil` --- High-level file operations
============================================

.. module:: shutil
   :synopsis: High-level file operations, including copying.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. partly based on the docstrings

.. index::
   single: file; copying
   single: copying files

**Source code:** :source:`Lib/shutil.py`

--------------

The :mod:`shutil` module offers a number of high-level operations on files and
collections of files.  In particular, functions are provided  which support file
copying and removal. For operations on individual files, see also the
:mod:`os` module.

.. warning::

   Even the higher-level file copying functions (:func:`shutil.copy`,
   :func:`shutil.copy2`) can't copy all file metadata.

   On POSIX platforms, this means that file owner and group are lost as well
   as ACLs.  On Mac OS, the resource fork and other metadata are not used.
   This means that resources will be lost and file type and creator codes will
   not be correct. On Windows, file owners, ACLs and alternate data streams
   are not copied.


.. _file-operations:

Directory and files operations
------------------------------

.. function:: copyfileobj(fsrc, fdst[, length])

   Copy the contents of the file-like object *fsrc* to the file-like object *fdst*.
   The integer *length*, if given, is the buffer size. In particular, a negative
   *length* value means to copy the data without looping over the source data in
   chunks; by default the data is read in chunks to avoid uncontrolled memory
   consumption. Note that if the current file position of the *fsrc* object is not
   0, only the contents from the current file position to the end of the file will
   be copied.


.. function:: copyfile(src, dst)

   Copy the contents (no metadata) of the file named *src* to a file named
   *dst*.  *dst* must be the complete target file name; look at
   :func:`shutil.copy` for a copy that accepts a target directory path.  If
   *src* and *dst* are the same files, :exc:`Error` is raised.
   The destination location must be writable; otherwise,  an :exc:`IOError` exception
   will be raised. If *dst* already exists, it will be replaced.   Special files
   such as character or block devices and pipes cannot be copied with this
   function.  *src* and *dst* are path names given as strings.


.. function:: copymode(src, dst)

   Copy the permission bits from *src* to *dst*.  The file contents, owner, and
   group are unaffected.  *src* and *dst* are path names given as strings.


.. function:: copystat(src, dst)

   Copy the permission bits, last access time, last modification time, and flags
   from *src* to *dst*.  The file contents, owner, and group are unaffected.  *src*
   and *dst* are path names given as strings.


.. function:: copy(src, dst)

   Copy the file *src* to the file or directory *dst*.  If *dst* is a directory, a
   file with the same basename as *src*  is created (or overwritten) in the
   directory specified.  Permission bits are copied.  *src* and *dst* are path
   names given as strings.


.. function:: copy2(src, dst)

   Similar to :func:`shutil.copy`, but metadata is copied as well -- in fact,
   this is just :func:`shutil.copy` followed by :func:`copystat`.  This is
   similar to the Unix command :program:`cp -p`.


.. function:: ignore_patterns(\*patterns)

   This factory function creates a function that can be used as a callable for
   :func:`copytree`\'s *ignore* argument, ignoring files and directories that
   match one of the glob-style *patterns* provided.  See the example below.

   .. versionadded:: 2.6


.. function:: copytree(src, dst, symlinks=False, ignore=None)

   Recursively copy an entire directory tree rooted at *src*.  The destination
   directory, named by *dst*, must not already exist; it will be created as
   well as missing parent directories.  Permissions and times of directories
   are copied with :func:`copystat`, individual files are copied using
   :func:`shutil.copy2`.

   If *symlinks* is true, symbolic links in the source tree are represented as
   symbolic links in the new tree, but the metadata of the original links is NOT
   copied; if false or omitted, the contents and metadata of the linked files
   are copied to the new tree.

   If *ignore* is given, it must be a callable that will receive as its
   arguments the directory being visited by :func:`copytree`, and a list of its
   contents, as returned by :func:`os.listdir`.  Since :func:`copytree` is
   called recursively, the *ignore* callable will be called once for each
   directory that is copied.  The callable must return a sequence of directory
   and file names relative to the current directory (i.e. a subset of the items
   in its second argument); these names will then be ignored in the copy
   process.  :func:`ignore_patterns` can be used to create such a callable that
   ignores names based on glob-style patterns.

   If exception(s) occur, an :exc:`Error` is raised with a list of reasons.

   The source code for this should be considered an example rather than the
   ultimate tool.

   .. versionchanged:: 2.3
      :exc:`Error` is raised if any exceptions occur during copying, rather than
      printing a message.

   .. versionchanged:: 2.5
      Create intermediate directories needed to create *dst*, rather than raising an
      error. Copy permissions and times of directories using :func:`copystat`.

   .. versionchanged:: 2.6
      Added the *ignore* argument to be able to influence what is being copied.


.. function:: rmtree(path[, ignore_errors[, onerror]])

   .. index:: single: directory; deleting

   Delete an entire directory tree; *path* must point to a directory (but not a
   symbolic link to a directory).  If *ignore_errors* is true, errors resulting
   from failed removals will be ignored; if false or omitted, such errors are
   handled by calling a handler specified by *onerror* or, if that is omitted,
   they raise an exception.

   If *onerror* is provided, it must be a callable that accepts three
   parameters: *function*, *path*, and *excinfo*. The first parameter,
   *function*, is the function which raised the exception; it will be
   :func:`os.path.islink`, :func:`os.listdir`, :func:`os.remove` or
   :func:`os.rmdir`.  The second parameter, *path*, will be the path name passed
   to *function*.  The third parameter, *excinfo*, will be the exception
   information return by :func:`sys.exc_info`.  Exceptions raised by *onerror*
   will not be caught.

   .. versionchanged:: 2.6
      Explicitly check for *path* being a symbolic link and raise :exc:`OSError`
      in that case.


.. function:: move(src, dst)

   Recursively move a file or directory (*src*) to another location (*dst*).

   If the destination is an existing directory, then *src* is moved inside that
   directory. If the destination already exists but is not a directory, it may
   be overwritten depending on :func:`os.rename` semantics.

   If the destination is on the current filesystem, then :func:`os.rename` is
   used.  Otherwise, *src* is copied (using :func:`shutil.copy2`) to *dst* and
   then removed.

   .. versionadded:: 2.3


.. exception:: Error

   This exception collects exceptions that are raised during a multi-file
   operation. For :func:`copytree`, the exception argument is a list of 3-tuples
   (*srcname*, *dstname*, *exception*).

   .. versionadded:: 2.3


.. _copytree-example:

copytree example
::::::::::::::::

This example is the implementation of the :func:`copytree` function, described
above, with the docstring omitted.  It demonstrates many of the other functions
provided by this module. ::

   def copytree(src, dst, symlinks=False, ignore=None):
       names = os.listdir(src)
       if ignore is not None:
           ignored_names = ignore(src, names)
       else:
           ignored_names = set()

       os.makedirs(dst)
       errors = []
       for name in names:
           if name in ignored_names:
               continue
           srcname = os.path.join(src, name)
           dstname = os.path.join(dst, name)
           try:
               if symlinks and os.path.islink(srcname):
                   linkto = os.readlink(srcname)
                   os.symlink(linkto, dstname)
               elif os.path.isdir(srcname):
                   copytree(srcname, dstname, symlinks, ignore)
               else:
                   copy2(srcname, dstname)
               # XXX What about devices, sockets etc.?
           except (IOError, os.error) as why:
               errors.append((srcname, dstname, str(why)))
           # catch the Error from the recursive copytree so that we can
           # continue with other files
           except Error as err:
               errors.extend(err.args[0])
       try:
           copystat(src, dst)
       except WindowsError:
           # can't copy file access times on Windows
           pass
       except OSError as why:
           errors.extend((src, dst, str(why)))
       if errors:
           raise Error(errors)

Another example that uses the :func:`ignore_patterns` helper::

   from shutil import copytree, ignore_patterns

   copytree(source, destination, ignore=ignore_patterns('*.pyc', 'tmp*'))

This will copy everything except ``.pyc`` files and files or directories whose
name starts with ``tmp``.

Another example that uses the *ignore* argument to add a logging call::

   from shutil import copytree
   import logging

   def _logpath(path, names):
       logging.info('Working in %s' % path)
       return []   # nothing will be ignored

   copytree(source, destination, ignore=_logpath)


.. _archiving-operations:

Archiving operations
--------------------

High-level utilities to create and read compressed and archived files are also
provided.  They rely on the :mod:`zipfile` and :mod:`tarfile` modules.

.. function:: make_archive(base_name, format, [root_dir, [base_dir, [verbose, [dry_run, [owner, [group, [logger]]]]]]])

   Create an archive file (eg. zip or tar) and returns its name.

   *base_name* is the name of the file to create, including the path, minus
   any format-specific extension. *format* is the archive format: one of
   "zip" (if the :mod:`zlib` module or external ``zip`` executable is
   available), "tar", "gztar" (if the :mod:`zlib` module is available), or
   "bztar" (if the :mod:`bz2` module is available).

   *root_dir* is a directory that will be the root directory of the
   archive; ie. we typically chdir into *root_dir* before creating the
   archive.

   *base_dir* is the directory where we start archiving from;
   ie. *base_dir* will be the common prefix of all files and
   directories in the archive.

   *root_dir* and *base_dir* both default to the current directory.

   *owner* and *group* are used when creating a tar archive. By default,
   uses the current owner and group.

   *logger* must be an object compatible with :pep:`282`, usually an instance of
   :class:`logging.Logger`.

   .. versionadded:: 2.7


.. function:: get_archive_formats()

   Return a list of supported formats for archiving.
   Each element of the returned sequence is a tuple ``(name, description)``.

   By default :mod:`shutil` provides these formats:

   - *zip*: ZIP file (if the :mod:`zlib` module or external ``zip``
     executable is available).
   - *tar*: uncompressed tar file.
   - *gztar*: gzip'ed tar-file (if the :mod:`zlib` module is available).
   - *bztar*: bzip2'ed tar-file (if the :mod:`bz2` module is available).

   You can register new formats or provide your own archiver for any existing
   formats, by using :func:`register_archive_format`.

   .. versionadded:: 2.7


.. function:: register_archive_format(name, function, [extra_args, [description]])

   Register an archiver for the format *name*. *function* is a callable that
   will be used to invoke the archiver.

   If given, *extra_args* is a sequence of ``(name, value)`` that will be
   used as extra keywords arguments when the archiver callable is used.

   *description* is used by :func:`get_archive_formats` which returns the
   list of archivers. Defaults to an empty list.

   .. versionadded:: 2.7


.. function::  unregister_archive_format(name)

   Remove the archive format *name* from the list of supported formats.

   .. versionadded:: 2.7


.. _archiving-example:

Archiving example
:::::::::::::::::

In this example, we create a gzip'ed tar-file archive containing all files
found in the :file:`.ssh` directory of the user::

    >>> from shutil import make_archive
    >>> import os
    >>> archive_name = os.path.expanduser(os.path.join('~', 'myarchive'))
    >>> root_dir = os.path.expanduser(os.path.join('~', '.ssh'))
    >>> make_archive(archive_name, 'gztar', root_dir)
    '/Users/tarek/myarchive.tar.gz'

The resulting archive contains:

.. code-block:: shell-session

    $ tar -tzvf /Users/tarek/myarchive.tar.gz
    drwx------ tarek/staff       0 2010-02-01 16:23:40 ./
    -rw-r--r-- tarek/staff     609 2008-06-09 13:26:54 ./authorized_keys
    -rwxr-xr-x tarek/staff      65 2008-06-09 13:26:54 ./config
    -rwx------ tarek/staff     668 2008-06-09 13:26:54 ./id_dsa
    -rwxr-xr-x tarek/staff     609 2008-06-09 13:26:54 ./id_dsa.pub
    -rw------- tarek/staff    1675 2008-06-09 13:26:54 ./id_rsa
    -rw-r--r-- tarek/staff     397 2008-06-09 13:26:54 ./id_rsa.pub
    -rw-r--r-- tarek/staff   37192 2010-02-06 18:23:10 ./known_hosts
