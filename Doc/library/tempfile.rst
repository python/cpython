
:mod:`tempfile` --- Generate temporary files and directories
============================================================

.. sectionauthor:: Zack Weinberg <zack@codesourcery.com>


.. module:: tempfile
   :synopsis: Generate temporary files and directories.


.. index::
   pair: temporary; file name
   pair: temporary; file

This module generates temporary files and directories.  It works on all
supported platforms.

In version 2.3 of Python, this module was overhauled for enhanced security.  It
now provides three new functions, :func:`NamedTemporaryFile`, :func:`mkstemp`,
and :func:`mkdtemp`, which should eliminate all remaining need to use the
insecure :func:`mktemp` function.  Temporary file names created by this module
no longer contain the process ID; instead a string of six random characters is
used.

Also, all the user-callable functions now take additional arguments which allow
direct control over the location and name of temporary files.  It is no longer
necessary to use the global *tempdir* and *template* variables.  To maintain
backward compatibility, the argument order is somewhat odd; it is recommended to
use keyword arguments for clarity.

The module defines the following user-callable functions:


.. function:: TemporaryFile([mode='w+b'[, bufsize=-1[, suffix[, prefix[, dir]]]]])

   Return a file-like object that can be used as a temporary storage
   area.  The file is created using :func:`mkstemp`. It will be destroyed as soon
   as it is closed (including an implicit close when the object is garbage
   collected).  Under Unix, the directory entry for the file is removed immediately
   after the file is created.  Other platforms do not support this; your code
   should not rely on a temporary file created using this function having or not
   having a visible name in the file system.

   The *mode* parameter defaults to ``'w+b'`` so that the file created can be read
   and written without being closed.  Binary mode is used so that it behaves
   consistently on all platforms without regard for the data that is stored.
   *bufsize* defaults to ``-1``, meaning that the operating system default is used.

   The *dir*, *prefix* and *suffix* parameters are passed to :func:`mkstemp`.

   The returned object is a true file object on POSIX platforms.  On other
   platforms, it is a file-like object whose :attr:`file` attribute is the
   underlying true file object.


.. function:: NamedTemporaryFile([mode='w+b'[, bufsize=-1[, suffix[, prefix[, dir[, delete]]]]]])

   This function operates exactly as :func:`TemporaryFile` does, except that the
   file is guaranteed to have a visible name in the file system (on Unix, the
   directory entry is not unlinked).  That name can be retrieved from the
   :attr:`name` member of the file object.  Whether the name can be used to open
   the file a second time, while the named temporary file is still open, varies
   across platforms (it can be so used on Unix; it cannot on Windows NT or later).
   If *delete* is true (the default), the file is deleted as soon as it is closed.
   The returned object is always a file-like object whose :attr:`file` attribute
   is the underlying true file object.


.. function:: SpooledTemporaryFile([max_size=0, [mode='w+b'[, bufsize=-1[, suffix[, prefix[, dir]]]]]])

   This function operates exactly as :func:`TemporaryFile` does, except that data
   is spooled in memory until the file size exceeds *max_size*, or until the file's
   :func:`fileno` method is called, at which point the contents are written to disk
   and operation proceeds as with :func:`TemporaryFile`.

   The resulting file has one additional method, :func:`rollover`, which causes the
   file to roll over to an on-disk file regardless of its size.

   The returned object is a file-like object whose :attr:`_file` attribute
   is either a :class:`StringIO` object or a true file object, depending on
   whether :func:`rollover` has been called.


.. function:: mkstemp([suffix[, prefix[, dir[, text]]]])

   Creates a temporary file in the most secure manner possible.  There are no
   race conditions in the file's creation, assuming that the platform properly
   implements the :const:`os.O_EXCL` flag for :func:`os.open`.  The file is
   readable and writable only by the creating user ID.  If the platform uses
   permission bits to indicate whether a file is executable, the file is
   executable by no one.  The file descriptor is not inherited by child
   processes.

   Unlike :func:`TemporaryFile`, the user of :func:`mkstemp` is responsible for
   deleting the temporary file when done with it.

   If *suffix* is specified, the file name will end with that suffix, otherwise
   there will be no suffix.  :func:`mkstemp` does not put a dot between the file
   name and the suffix; if you need one, put it at the beginning of *suffix*.

   If *prefix* is specified, the file name will begin with that prefix; otherwise,
   a default prefix is used.

   If *dir* is specified, the file will be created in that directory; otherwise,
   a default directory is used.  The default directory is chosen from a
   platform-dependent list, but the user of the application can control the
   directory location by setting the *TMPDIR*, *TEMP* or *TMP* environment
   variables.  There is thus no guarantee that the generated filename will have
   any nice properties, such as not requiring quoting when passed to external
   commands via ``os.popen()``.

   If *text* is specified, it indicates whether to open the file in binary mode
   (the default) or text mode.  On some platforms, this makes no difference.

   :func:`mkstemp` returns a tuple containing an OS-level handle to an open file
   (as would be returned by :func:`os.open`) and the absolute pathname of that
   file, in that order.


.. function:: mkdtemp([suffix[, prefix[, dir]]])

   Creates a temporary directory in the most secure manner possible. There are no
   race conditions in the directory's creation.  The directory is readable,
   writable, and searchable only by the creating user ID.

   The user of :func:`mkdtemp` is responsible for deleting the temporary directory
   and its contents when done with it.

   The *prefix*, *suffix*, and *dir* arguments are the same as for :func:`mkstemp`.

   :func:`mkdtemp` returns the absolute pathname of the new directory.


.. function:: mktemp([suffix[, prefix[, dir]]])

   .. deprecated:: 2.3
      Use :func:`mkstemp` instead.

   Return an absolute pathname of a file that did not exist at the time the call is
   made.  The *prefix*, *suffix*, and *dir* arguments are the same as for
   :func:`mkstemp`.

   .. warning::

      Use of this function may introduce a security hole in your program.  By the time
      you get around to doing anything with the file name it returns, someone else may
      have beaten you to the punch.

The module uses two global variables that tell it how to construct a temporary
name.  They are initialized at the first call to any of the functions above.
The caller may change them, but this is discouraged; use the appropriate
function arguments, instead.


.. data:: tempdir

   When set to a value other than ``None``, this variable defines the default value
   for the *dir* argument to all the functions defined in this module.

   If ``tempdir`` is unset or ``None`` at any call to any of the above functions,
   Python searches a standard list of directories and sets *tempdir* to the first
   one which the calling user can create files in.  The list is:

   #. The directory named by the :envvar:`TMPDIR` environment variable.

   #. The directory named by the :envvar:`TEMP` environment variable.

   #. The directory named by the :envvar:`TMP` environment variable.

   #. A platform-specific location:

      * On Windows, the directories :file:`C:\\TEMP`, :file:`C:\\TMP`,
        :file:`\\TEMP`, and :file:`\\TMP`, in that order.

      * On all other platforms, the directories :file:`/tmp`, :file:`/var/tmp`, and
        :file:`/usr/tmp`, in that order.

   #. As a last resort, the current working directory.


.. function:: gettempdir()

   Return the directory currently selected to create temporary files in. If
   :data:`tempdir` is not ``None``, this simply returns its contents; otherwise,
   the search described above is performed, and the result returned.


.. data:: template

   .. deprecated:: 2.0
      Use :func:`gettempprefix` instead.

   When set to a value other than ``None``, this variable defines the prefix of the
   final component of the filenames returned by :func:`mktemp`.  A string of six
   random letters and digits is appended to the prefix to make the filename unique.
   On Windows, the default prefix is :file:`~T`; on all other systems it is
   :file:`tmp`.

   Older versions of this module used to require that ``template`` be set to
   ``None`` after a call to :func:`os.fork`; this has not been necessary since
   version 1.5.2.


.. function:: gettempprefix()

   Return the filename prefix used to create temporary files.  This does not
   contain the directory component.  Using this function is preferred over reading
   the *template* variable directly.

