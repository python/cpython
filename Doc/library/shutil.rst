
:mod:`shutil` --- High-level file operations
============================================

.. module:: shutil
   :synopsis: High-level file operations, including copying.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


.. % partly based on the docstrings

.. index::
   single: file; copying
   single: copying files

The :mod:`shutil` module offers a number of high-level operations on files and
collections of files.  In particular, functions are provided  which support file
copying and removal.

.. warning::
   
   On MacOS, the resource fork and other metadata are not used.  For file copies,
   this means that resources will be lost and  file type and creator codes will
   not be correct.


.. function:: copyfile(src, dst)

   Copy the contents of the file named *src* to a file named *dst*.  The
   destination location must be writable; otherwise,  an :exc:`IOError` exception
   will be raised. If *dst* already exists, it will be replaced.   Special files
   such as character or block devices and pipes cannot be copied with this
   function.  *src* and *dst* are path names given as strings.


.. function:: copyfileobj(fsrc, fdst[, length])

   Copy the contents of the file-like object *fsrc* to the file-like object *fdst*.
   The integer *length*, if given, is the buffer size. In particular, a negative
   *length* value means to copy the data without looping over the source data in
   chunks; by default the data is read in chunks to avoid uncontrolled memory
   consumption. Note that if the current file position of the *fsrc* object is not
   0, only the contents from the current file position to the end of the file will
   be copied.


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

   Similar to :func:`copy`, but last access time and last modification time are
   copied as well.  This is similar to the Unix command :program:`cp -p`.


.. function:: copytree(src, dst[, symlinks])

   Recursively copy an entire directory tree rooted at *src*.  The destination
   directory, named by *dst*, must not already exist; it will be created as well as
   missing parent directories. Permissions and times of directories are copied with
   :func:`copystat`, individual files are copied using :func:`copy2`.   If
   *symlinks* is true, symbolic links in the source tree are represented as
   symbolic links in the new tree; if false or omitted, the contents of the linked
   files are copied to the new tree.  If exception(s) occur, an :exc:`Error` is
   raised with a list of reasons.

   The source code for this should be considered an example rather than  a tool.

   .. versionchanged:: 2.3
      :exc:`Error` is raised if any exceptions occur during copying, rather than
      printing a message.

   .. versionchanged:: 2.5
      Create intermediate directories needed to create *dst*, rather than raising an
      error. Copy permissions and times of directories using :func:`copystat`.


.. function:: rmtree(path[, ignore_errors[, onerror]])

   .. index:: single: directory; deleting

   Delete an entire directory tree (*path* must point to a directory). If
   *ignore_errors* is true, errors resulting from failed removals will be ignored;
   if false or omitted, such errors are handled by calling a handler specified by
   *onerror* or, if that is omitted, they raise an exception.

   If *onerror* is provided, it must be a callable that accepts three parameters:
   *function*, *path*, and *excinfo*. The first parameter, *function*, is the
   function which raised the exception; it will be :func:`os.listdir`,
   :func:`os.remove` or :func:`os.rmdir`.  The second parameter, *path*, will be
   the path name passed to *function*.  The third parameter, *excinfo*, will be the
   exception information return by :func:`sys.exc_info`.  Exceptions raised by
   *onerror* will not be caught.


.. function:: move(src, dst)

   Recursively move a file or directory to another location.

   If the destination is on our current filesystem, then simply use rename.
   Otherwise, copy src to the dst and then remove src.

   .. versionadded:: 2.3


.. exception:: Error

   This exception collects exceptions that raised during a mult-file operation. For
   :func:`copytree`, the exception argument is a list of 3-tuples (*srcname*,
   *dstname*, *exception*).

   .. versionadded:: 2.3


.. _shutil-example:

Example
-------

This example is the implementation of the :func:`copytree` function, described
above, with the docstring omitted.  It demonstrates many of the other functions
provided by this module. ::

   def copytree(src, dst, symlinks=False):
       names = os.listdir(src)
       os.makedirs(dst)
       errors = []
       for name in names:
           srcname = os.path.join(src, name)
           dstname = os.path.join(dst, name)
           try:
               if symlinks and os.path.islink(srcname):
                   linkto = os.readlink(srcname)
                   os.symlink(linkto, dstname)
               elif os.path.isdir(srcname):
                   copytree(srcname, dstname, symlinks)
               else:
                   copy2(srcname, dstname)
               # XXX What about devices, sockets etc.?
           except (IOError, os.error), why:
               errors.append((srcname, dstname, str(why)))
           # catch the Error from the recursive copytree so that we can
           # continue with other files
           except Error, err:
               errors.extend(err.args[0])
       try:
           copystat(src, dst)
       except WindowsError:
           # can't copy file access times on Windows
           pass
       except OSError, why:
           errors.extend((src, dst, str(why)))
       if errors:
           raise Error, errors
