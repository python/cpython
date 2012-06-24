:mod:`os` --- Miscellaneous operating system interfaces
=======================================================

.. module:: os
   :synopsis: Miscellaneous operating system interfaces.


This module provides a portable way of using operating system dependent
functionality.  If you just want to read or write a file see :func:`open`, if
you want to manipulate paths, see the :mod:`os.path` module, and if you want to
read all the lines in all the files on the command line see the :mod:`fileinput`
module.  For creating temporary files and directories see the :mod:`tempfile`
module, and for high-level file and directory handling see the :mod:`shutil`
module.

Notes on the availability of these functions:

* The design of all built-in operating system dependent modules of Python is
  such that as long as the same functionality is available, it uses the same
  interface; for example, the function ``os.stat(path)`` returns stat
  information about *path* in the same format (which happens to have originated
  with the POSIX interface).

* Extensions peculiar to a particular operating system are also available
  through the :mod:`os` module, but using them is of course a threat to
  portability.

* All functions accepting path or file names accept both bytes and string
  objects, and result in an object of the same type, if a path or file name is
  returned.

* An "Availability: Unix" note means that this function is commonly found on
  Unix systems.  It does not make any claims about its existence on a specific
  operating system.

* If not separately noted, all functions that claim "Availability: Unix" are
  supported on Mac OS X, which builds on a Unix core.

.. Availability notes get their own line and occur at the end of the function
.. documentation.

.. note::

   All functions in this module raise :exc:`OSError` in the case of invalid or
   inaccessible file names and paths, or other arguments that have the correct
   type, but are not accepted by the operating system.

.. exception:: error

   An alias for the built-in :exc:`OSError` exception.


.. data:: name

   The name of the operating system dependent module imported.  The following
   names have currently been registered: ``'posix'``, ``'nt'``, ``'mac'``,
   ``'os2'``, ``'ce'``, ``'java'``.

   .. seealso::
      :attr:`sys.platform` has a finer granularity.  :func:`os.uname` gives
      system-dependent version information.

      The :mod:`platform` module provides detailed checks for the
      system's identity.


.. _os-filenames:

File Names, Command Line Arguments, and Environment Variables
-------------------------------------------------------------

In Python, file names, command line arguments, and environment variables are
represented using the string type. On some systems, decoding these strings to
and from bytes is necessary before passing them to the operating system. Python
uses the file system encoding to perform this conversion (see
:func:`sys.getfilesystemencoding`).

.. versionchanged:: 3.1
   On some systems, conversion using the file system encoding may fail. In this
   case, Python uses the ``surrogateescape`` encoding error handler, which means
   that undecodable bytes are replaced by a Unicode character U+DCxx on
   decoding, and these are again translated to the original byte on encoding.


The file system encoding must guarantee to successfully decode all bytes
below 128. If the file system encoding fails to provide this guarantee, API
functions may raise UnicodeErrors.


.. _os-procinfo:

Process Parameters
------------------

These functions and data items provide information and operate on the current
process and user.


.. function:: ctermid()

   Return the filename corresponding to the controlling terminal of the process.

   Availability: Unix.


.. data:: environ

   A mapping object representing the string environment. For example,
   ``environ['HOME']`` is the pathname of your home directory (on some platforms),
   and is equivalent to ``getenv("HOME")`` in C.

   This mapping is captured the first time the :mod:`os` module is imported,
   typically during Python startup as part of processing :file:`site.py`.  Changes
   to the environment made after this time are not reflected in ``os.environ``,
   except for changes made by modifying ``os.environ`` directly.

   If the platform supports the :func:`putenv` function, this mapping may be used
   to modify the environment as well as query the environment.  :func:`putenv` will
   be called automatically when the mapping is modified.

   On Unix, keys and values use :func:`sys.getfilesystemencoding` and
   ``'surrogateescape'`` error handler. Use :data:`environb` if you would like
   to use a different encoding.

   .. note::

      Calling :func:`putenv` directly does not change ``os.environ``, so it's better
      to modify ``os.environ``.

   .. note::

      On some platforms, including FreeBSD and Mac OS X, setting ``environ`` may
      cause memory leaks.  Refer to the system documentation for
      :c:func:`putenv`.

   If :func:`putenv` is not provided, a modified copy of this mapping  may be
   passed to the appropriate process-creation functions to cause  child processes
   to use a modified environment.

   If the platform supports the :func:`unsetenv` function, you can delete items in
   this mapping to unset environment variables. :func:`unsetenv` will be called
   automatically when an item is deleted from ``os.environ``, and when
   one of the :meth:`pop` or :meth:`clear` methods is called.


.. data:: environb

   Bytes version of :data:`environ`: a mapping object representing the
   environment as byte strings. :data:`environ` and :data:`environb` are
   synchronized (modify :data:`environb` updates :data:`environ`, and vice
   versa).

   :data:`environb` is only available if :data:`supports_bytes_environ` is
   True.

   .. versionadded:: 3.2


.. function:: chdir(path)
              fchdir(fd)
              getcwd()
   :noindex:

   These functions are described in :ref:`os-file-dir`.


.. function:: fsencode(filename)

   Encode *filename* to the filesystem encoding with ``'surrogateescape'``
   error handler, or ``'strict'`` on Windows; return :class:`bytes` unchanged.

   :func:`fsdecode` is the reverse function.

   .. versionadded:: 3.2


.. function:: fsdecode(filename)

   Decode *filename* from the filesystem encoding with ``'surrogateescape'``
   error handler, or ``'strict'`` on Windows; return :class:`str` unchanged.

   :func:`fsencode` is the reverse function.

   .. versionadded:: 3.2


.. function:: getenv(key, default=None)

   Return the value of the environment variable *key* if it exists, or
   *default* if it doesn't. *key*, *default* and the result are str.

   On Unix, keys and values are decoded with :func:`sys.getfilesystemencoding`
   and ``'surrogateescape'`` error handler. Use :func:`os.getenvb` if you
   would like to use a different encoding.

   Availability: most flavors of Unix, Windows.


.. function:: getenvb(key, default=None)

   Return the value of the environment variable *key* if it exists, or
   *default* if it doesn't. *key*, *default* and the result are bytes.

   Availability: most flavors of Unix.

   .. versionadded:: 3.2


.. function:: get_exec_path(env=None)

   Returns the list of directories that will be searched for a named
   executable, similar to a shell, when launching a process.
   *env*, when specified, should be an environment variable dictionary
   to lookup the PATH in.
   By default, when *env* is None, :data:`environ` is used.

   .. versionadded:: 3.2


.. function:: getegid()

   Return the effective group id of the current process.  This corresponds to the
   "set id" bit on the file being executed in the current process.

   Availability: Unix.


.. function:: geteuid()

   .. index:: single: user; effective id

   Return the current process's effective user id.

   Availability: Unix.


.. function:: getgid()

   .. index:: single: process; group

   Return the real group id of the current process.

   Availability: Unix.


.. function:: getgrouplist(user, group)

   Return list of group ids that *user* belongs to. If *group* is not in the
   list, it is included; typically, *group* is specified as the group ID
   field from the password record for *user*.

   Availability: Unix.

   .. versionadded:: 3.3


.. function:: getgroups()

   Return list of supplemental group ids associated with the current process.

   Availability: Unix.

   .. note:: On Mac OS X, :func:`getgroups` behavior differs somewhat from
      other Unix platforms. If the Python interpreter was built with a
      deployment target of :const:`10.5` or earlier, :func:`getgroups` returns
      the list of effective group ids associated with the current user process;
      this list is limited to a system-defined number of entries, typically 16,
      and may be modified by calls to :func:`setgroups` if suitably privileged.
      If built with a deployment target greater than :const:`10.5`,
      :func:`getgroups` returns the current group access list for the user
      associated with the effective user id of the process; the group access
      list may change over the lifetime of the process, it is not affected by
      calls to :func:`setgroups`, and its length is not limited to 16.  The
      deployment target value, :const:`MACOSX_DEPLOYMENT_TARGET`, can be
      obtained with :func:`sysconfig.get_config_var`.


.. function:: getlogin()

   Return the name of the user logged in on the controlling terminal of the
   process.  For most purposes, it is more useful to use the environment variables
   :envvar:`LOGNAME` or :envvar:`USERNAME` to find out who the user is, or
   ``pwd.getpwuid(os.getuid())[0]`` to get the login name of the currently
   effective user id.

   Availability: Unix, Windows.


.. function:: getpgid(pid)

   Return the process group id of the process with process id *pid*. If *pid* is 0,
   the process group id of the current process is returned.

   Availability: Unix.

.. function:: getpgrp()

   .. index:: single: process; group

   Return the id of the current process group.

   Availability: Unix.


.. function:: getpid()

   .. index:: single: process; id

   Return the current process id.

   Availability: Unix, Windows.


.. function:: getppid()

   .. index:: single: process; id of parent

   Return the parent's process id.  When the parent process has exited, on Unix
   the id returned is the one of the init process (1), on Windows it is still
   the same id, which may be already reused by another process.

   Availability: Unix, Windows

   .. versionchanged:: 3.2
      Added support for Windows.

.. function:: getpriority(which, who)

   .. index:: single: process; scheduling priority

   Get program scheduling priority.  The value *which* is one of
   :const:`PRIO_PROCESS`, :const:`PRIO_PGRP`, or :const:`PRIO_USER`, and *who*
   is interpreted relative to *which* (a process identifier for
   :const:`PRIO_PROCESS`, process group identifier for :const:`PRIO_PGRP`, and a
   user ID for :const:`PRIO_USER`).  A zero value for *who* denotes
   (respectively) the calling process, the process group of the calling process,
   or the real user ID of the calling process.

   Availability: Unix.

   .. versionadded:: 3.3


.. data:: PRIO_PROCESS
          PRIO_PGRP
          PRIO_USER

   Parameters for the :func:`getpriority` and :func:`setpriority` functions.

   Availability: Unix.

   .. versionadded:: 3.3


.. function:: getresuid()

   Return a tuple (ruid, euid, suid) denoting the current process's
   real, effective, and saved user ids.

   Availability: Unix.

   .. versionadded:: 3.2


.. function:: getresgid()

   Return a tuple (rgid, egid, sgid) denoting the current process's
   real, effective, and saved group ids.

   Availability: Unix.

   .. versionadded:: 3.2


.. function:: getuid()

   .. index:: single: user; id

   Return the current process's user id.

   Availability: Unix.


.. function:: initgroups(username, gid)

   Call the system initgroups() to initialize the group access list with all of
   the groups of which the specified username is a member, plus the specified
   group id.

   Availability: Unix.

   .. versionadded:: 3.2


.. function:: putenv(key, value)

   .. index:: single: environment variables; setting

   Set the environment variable named *key* to the string *value*.  Such
   changes to the environment affect subprocesses started with :func:`os.system`,
   :func:`popen` or :func:`fork` and :func:`execv`.

   Availability: most flavors of Unix, Windows.

   .. note::

      On some platforms, including FreeBSD and Mac OS X, setting ``environ`` may
      cause memory leaks. Refer to the system documentation for putenv.

   When :func:`putenv` is supported, assignments to items in ``os.environ`` are
   automatically translated into corresponding calls to :func:`putenv`; however,
   calls to :func:`putenv` don't update ``os.environ``, so it is actually
   preferable to assign to items of ``os.environ``.


.. function:: setegid(egid)

   Set the current process's effective group id.

   Availability: Unix.


.. function:: seteuid(euid)

   Set the current process's effective user id.

   Availability: Unix.


.. function:: setgid(gid)

   Set the current process' group id.

   Availability: Unix.


.. function:: setgroups(groups)

   Set the list of supplemental group ids associated with the current process to
   *groups*. *groups* must be a sequence, and each element must be an integer
   identifying a group. This operation is typically available only to the superuser.

   Availability: Unix.

   .. note:: On Mac OS X, the length of *groups* may not exceed the
      system-defined maximum number of effective group ids, typically 16.
      See the documentation for :func:`getgroups` for cases where it may not
      return the same group list set by calling setgroups().

.. function:: setpgrp()

   Call the system call :c:func:`setpgrp` or :c:func:`setpgrp(0, 0)` depending on
   which version is implemented (if any).  See the Unix manual for the semantics.

   Availability: Unix.


.. function:: setpgid(pid, pgrp)

   Call the system call :c:func:`setpgid` to set the process group id of the
   process with id *pid* to the process group with id *pgrp*.  See the Unix manual
   for the semantics.

   Availability: Unix.


.. function:: setpriority(which, who, priority)

   .. index:: single: process; scheduling priority

   Set program scheduling priority. The value *which* is one of
   :const:`PRIO_PROCESS`, :const:`PRIO_PGRP`, or :const:`PRIO_USER`, and *who*
   is interpreted relative to *which* (a process identifier for
   :const:`PRIO_PROCESS`, process group identifier for :const:`PRIO_PGRP`, and a
   user ID for :const:`PRIO_USER`). A zero value for *who* denotes
   (respectively) the calling process, the process group of the calling process,
   or the real user ID of the calling process.
   *priority* is a value in the range -20 to 19. The default priority is 0;
   lower priorities cause more favorable scheduling.

   Availability: Unix

   .. versionadded:: 3.3


.. function:: setregid(rgid, egid)

   Set the current process's real and effective group ids.

   Availability: Unix.


.. function:: setresgid(rgid, egid, sgid)

   Set the current process's real, effective, and saved group ids.

   Availability: Unix.

   .. versionadded:: 3.2


.. function:: setresuid(ruid, euid, suid)

   Set the current process's real, effective, and saved user ids.

   Availability: Unix.

   .. versionadded:: 3.2


.. function:: setreuid(ruid, euid)

   Set the current process's real and effective user ids.

   Availability: Unix.


.. function:: getsid(pid)

   Call the system call :c:func:`getsid`.  See the Unix manual for the semantics.

   Availability: Unix.


.. function:: setsid()

   Call the system call :c:func:`setsid`.  See the Unix manual for the semantics.

   Availability: Unix.


.. function:: setuid(uid)

   .. index:: single: user; id, setting

   Set the current process's user id.

   Availability: Unix.


.. placed in this section since it relates to errno.... a little weak
.. function:: strerror(code)

   Return the error message corresponding to the error code in *code*.
   On platforms where :c:func:`strerror` returns ``NULL`` when given an unknown
   error number, :exc:`ValueError` is raised.

   Availability: Unix, Windows.


.. data:: supports_bytes_environ

   True if the native OS type of the environment is bytes (eg. False on
   Windows).

   .. versionadded:: 3.2


.. function:: umask(mask)

   Set the current numeric umask and return the previous umask.

   Availability: Unix, Windows.


.. function:: uname()

   .. index::
      single: gethostname() (in module socket)
      single: gethostbyaddr() (in module socket)

   Return a 5-tuple containing information identifying the current operating
   system.  The tuple contains 5 strings: ``(sysname, nodename, release, version,
   machine)``.  Some systems truncate the nodename to 8 characters or to the
   leading component; a better way to get the hostname is
   :func:`socket.gethostname`  or even
   ``socket.gethostbyaddr(socket.gethostname())``.

   Availability: recent flavors of Unix.


.. function:: unsetenv(key)

   .. index:: single: environment variables; deleting

   Unset (delete) the environment variable named *key*. Such changes to the
   environment affect subprocesses started with :func:`os.system`, :func:`popen` or
   :func:`fork` and :func:`execv`.

   When :func:`unsetenv` is supported, deletion of items in ``os.environ`` is
   automatically translated into a corresponding call to :func:`unsetenv`; however,
   calls to :func:`unsetenv` don't update ``os.environ``, so it is actually
   preferable to delete items of ``os.environ``.

   Availability: most flavors of Unix, Windows.


.. _os-newstreams:

File Object Creation
--------------------

These functions create new :term:`file objects <file object>`. (See also :func:`open`.)


.. function:: fdopen(fd, *args, **kwargs)

   Return an open file object connected to the file descriptor *fd*.
   This is an alias of :func:`open` and accepts the same arguments.
   The only difference is that the first argument of :func:`fdopen`
   must always be an integer.


.. _os-fd-ops:

File Descriptor Operations
--------------------------

These functions operate on I/O streams referenced using file descriptors.

File descriptors are small integers corresponding to a file that has been opened
by the current process.  For example, standard input is usually file descriptor
0, standard output is 1, and standard error is 2.  Further files opened by a
process will then be assigned 3, 4, 5, and so forth.  The name "file descriptor"
is slightly deceptive; on Unix platforms, sockets and pipes are also referenced
by file descriptors.

The :meth:`~file.fileno` method can be used to obtain the file descriptor
associated with a :term:`file object` when required.  Note that using the file
descriptor directly will bypass the file object methods, ignoring aspects such
as internal buffering of data.


.. function:: close(fd)

   Close file descriptor *fd*.

   Availability: Unix, Windows.

   .. note::

      This function is intended for low-level I/O and must be applied to a file
      descriptor as returned by :func:`os.open` or :func:`pipe`.  To close a "file
      object" returned by the built-in function :func:`open` or by :func:`popen` or
      :func:`fdopen`, use its :meth:`~file.close` method.


.. function:: closerange(fd_low, fd_high)

   Close all file descriptors from *fd_low* (inclusive) to *fd_high* (exclusive),
   ignoring errors. Equivalent to (but much faster than)::

      for fd in range(fd_low, fd_high):
          try:
              os.close(fd)
          except OSError:
              pass

   Availability: Unix, Windows.


.. function:: device_encoding(fd)

   Return a string describing the encoding of the device associated with *fd*
   if it is connected to a terminal; else return :const:`None`.


.. function:: dup(fd)

   Return a duplicate of file descriptor *fd*.

   Availability: Unix, Windows.


.. function:: dup2(fd, fd2)

   Duplicate file descriptor *fd* to *fd2*, closing the latter first if necessary.

   Availability: Unix, Windows.


.. function:: fchmod(fd, mode)

   Change the mode of the file given by *fd* to the numeric *mode*.  See the
   docs for :func:`chmod` for possible values of *mode*.  From Python 3.3, this
   is equivalent to ``os.chmod(fd, mode)``.

   Availability: Unix.


.. function:: fchown(fd, uid, gid)

   Change the owner and group id of the file given by *fd* to the numeric *uid*
   and *gid*.  To leave one of the ids unchanged, set it to -1.  See
   :func:`chown`.  From Python 3.3, this is equivalent to ``os.chown(fd, uid,
   gid)``.

   Availability: Unix.


.. function:: fdatasync(fd)

   Force write of file with filedescriptor *fd* to disk. Does not force update of
   metadata.

   Availability: Unix.

   .. note::
      This function is not available on MacOS.


.. function:: fpathconf(fd, name)

   Return system configuration information relevant to an open file. *name*
   specifies the configuration value to retrieve; it may be a string which is the
   name of a defined system value; these names are specified in a number of
   standards (POSIX.1, Unix 95, Unix 98, and others).  Some platforms define
   additional names as well.  The names known to the host operating system are
   given in the ``pathconf_names`` dictionary.  For configuration variables not
   included in that mapping, passing an integer for *name* is also accepted.

   If *name* is a string and is not known, :exc:`ValueError` is raised.  If a
   specific value for *name* is not supported by the host system, even if it is
   included in ``pathconf_names``, an :exc:`OSError` is raised with
   :const:`errno.EINVAL` for the error number.

   Availability: Unix.


.. function:: fstat(fd)

   Return status for file descriptor *fd*, like :func:`~os.stat`.  From Python
   3.3, this is equivalent to ``os.stat(fd)``.

   Availability: Unix, Windows.


.. function:: fstatvfs(fd)

   Return information about the filesystem containing the file associated with
   file descriptor *fd*, like :func:`statvfs`.  From Python 3.3, this is
   equivalent to ``os.statvfs(fd)``.

   Availability: Unix.


.. function:: fsync(fd)

   Force write of file with filedescriptor *fd* to disk.  On Unix, this calls the
   native :c:func:`fsync` function; on Windows, the MS :c:func:`_commit` function.

   If you're starting with a buffered Python :term:`file object` *f*, first do
   ``f.flush()``, and then do ``os.fsync(f.fileno())``, to ensure that all internal
   buffers associated with *f* are written to disk.

   Availability: Unix, and Windows.


.. function:: ftruncate(fd, length)

   Truncate the file corresponding to file descriptor *fd*, so that it is at most
   *length* bytes in size.

   Availability: Unix.


.. function:: isatty(fd)

   Return ``True`` if the file descriptor *fd* is open and connected to a
   tty(-like) device, else ``False``.

   Availability: Unix.


.. function:: lockf(fd, cmd, len)

   Apply, test or remove a POSIX lock on an open file descriptor.
   *fd* is an open file descriptor.
   *cmd* specifies the command to use - one of :data:`F_LOCK`, :data:`F_TLOCK`,
   :data:`F_ULOCK` or :data:`F_TEST`.
   *len* specifies the section of the file to lock.

   Availability: Unix.

   .. versionadded:: 3.3


.. data:: F_LOCK
          F_TLOCK
          F_ULOCK
          F_TEST

   Flags that specify what action :func:`lockf` will take.

   Availability: Unix.

   .. versionadded:: 3.3

.. function:: lseek(fd, pos, how)

   Set the current position of file descriptor *fd* to position *pos*, modified
   by *how*: :const:`SEEK_SET` or ``0`` to set the position relative to the
   beginning of the file; :const:`SEEK_CUR` or ``1`` to set it relative to the
   current position; :const:`os.SEEK_END` or ``2`` to set it relative to the end of
   the file. Return the new cursor position in bytes, starting from the beginning.

   Availability: Unix, Windows.


.. data:: SEEK_SET
          SEEK_CUR
          SEEK_END

   Parameters to the :func:`lseek` function. Their values are 0, 1, and 2,
   respectively. Availability: Windows, Unix.

   .. versionadded:: 3.3
      Some operating systems could support additional values, like
      :data:`os.SEEK_HOLE` or :data:`os.SEEK_DATA`.


.. function:: open(file, flags, mode=0o777, *, dir_fd=None)

   Open the file *file* and set various flags according to *flags* and possibly
   its mode according to *mode*.  When computing *mode*, the current umask value
   is first masked out.  Return the file descriptor for the newly opened file.

   For a description of the flag and mode values, see the C run-time documentation;
   flag constants (like :const:`O_RDONLY` and :const:`O_WRONLY`) are defined in
   this module too (see :ref:`open-constants`).  In particular, on Windows adding
   :const:`O_BINARY` is needed to open files in binary mode.

   This function can support :ref:`paths relative to directory descriptors
   <dir_fd>`.

   Availability: Unix, Windows.

   .. note::

      This function is intended for low-level I/O.  For normal usage, use the
      built-in function :func:`open`, which returns a :term:`file object` with
      :meth:`~file.read` and :meth:`~file.write` methods (and many more).  To
      wrap a file descriptor in a file object, use :func:`fdopen`.

   .. versionadded:: 3.3
      The *dir_fd* argument.


.. function:: openpty()

   .. index:: module: pty

   Open a new pseudo-terminal pair. Return a pair of file descriptors ``(master,
   slave)`` for the pty and the tty, respectively. For a (slightly) more portable
   approach, use the :mod:`pty` module.

   Availability: some flavors of Unix.


.. function:: pipe()

   Create a pipe.  Return a pair of file descriptors ``(r, w)`` usable for reading
   and writing, respectively.

   Availability: Unix, Windows.


.. function:: pipe2(flags)

   Create a pipe with *flags* set atomically.
   *flags* can be constructed by ORing together one or more of these values:
   :data:`O_NONBLOCK`, :data:`O_CLOEXEC`.
   Return a pair of file descriptors ``(r, w)`` usable for reading and writing,
   respectively.

   Availability: some flavors of Unix.

   .. versionadded:: 3.3


.. function:: posix_fallocate(fd, offset, len)

   Ensures that enough disk space is allocated for the file specified by *fd*
   starting from *offset* and continuing for *len* bytes.

   Availability: Unix.

   .. versionadded:: 3.3


.. function:: posix_fadvise(fd, offset, len, advice)

   Announces an intention to access data in a specific pattern thus allowing
   the kernel to make optimizations.
   The advice applies to the region of the file specified by *fd* starting at
   *offset* and continuing for *len* bytes.
   *advice* is one of :data:`POSIX_FADV_NORMAL`, :data:`POSIX_FADV_SEQUENTIAL`,
   :data:`POSIX_FADV_RANDOM`, :data:`POSIX_FADV_NOREUSE`,
   :data:`POSIX_FADV_WILLNEED` or :data:`POSIX_FADV_DONTNEED`.

   Availability: Unix.

   .. versionadded:: 3.3


.. data:: POSIX_FADV_NORMAL
          POSIX_FADV_SEQUENTIAL
          POSIX_FADV_RANDOM
          POSIX_FADV_NOREUSE
          POSIX_FADV_WILLNEED
          POSIX_FADV_DONTNEED

   Flags that can be used in *advice* in :func:`posix_fadvise` that specify
   the access pattern that is likely to be used.

   Availability: Unix.

   .. versionadded:: 3.3


.. function:: pread(fd, buffersize, offset)

   Read from a file descriptor, *fd*, at a position of *offset*. It will read up
   to *buffersize* number of bytes. The file offset remains unchanged.

   Availability: Unix.

   .. versionadded:: 3.3


.. function:: pwrite(fd, string, offset)

   Write *string* to a file descriptor, *fd*, from *offset*, leaving the file
   offset unchanged.

   Availability: Unix.

   .. versionadded:: 3.3


.. function:: read(fd, n)

   Read at most *n* bytes from file descriptor *fd*. Return a bytestring containing the
   bytes read.  If the end of the file referred to by *fd* has been reached, an
   empty bytes object is returned.

   Availability: Unix, Windows.

   .. note::

      This function is intended for low-level I/O and must be applied to a file
      descriptor as returned by :func:`os.open` or :func:`pipe`.  To read a "file object"
      returned by the built-in function :func:`open` or by :func:`popen` or
      :func:`fdopen`, or :data:`sys.stdin`, use its :meth:`~file.read` or
      :meth:`~file.readline` methods.


.. function:: sendfile(out, in, offset, nbytes)
              sendfile(out, in, offset, nbytes, headers=None, trailers=None, flags=0)

   Copy *nbytes* bytes from file descriptor *in* to file descriptor *out*
   starting at *offset*.
   Return the number of bytes sent. When EOF is reached return 0.

   The first function notation is supported by all platforms that define
   :func:`sendfile`.

   On Linux, if *offset* is given as ``None``, the bytes are read from the
   current position of *in* and the position of *in* is updated.

   The second case may be used on Mac OS X and FreeBSD where *headers* and
   *trailers* are arbitrary sequences of buffers that are written before and
   after the data from *in* is written. It returns the same as the first case.

   On Mac OS X and FreeBSD, a value of 0 for *nbytes* specifies to send until
   the end of *in* is reached.

   On Solaris, *out* may be the file descriptor of a regular file or the file
   descriptor of a socket. On all other platforms, *out* must be the file
   descriptor of an open socket.

   Availability: Unix.

   .. versionadded:: 3.3


.. data:: SF_NODISKIO
          SF_MNOWAIT
          SF_SYNC

   Parameters to the :func:`sendfile` function, if the implementation supports
   them.

   Availability: Unix.

   .. versionadded:: 3.3


.. function:: readv(fd, buffers)

   Read from a file descriptor into a number of writable buffers. *buffers* is
   an arbitrary sequence of writable buffers. Returns the total number of bytes
   read.

   Availability: Unix.

   .. versionadded:: 3.3


.. function:: tcgetpgrp(fd)

   Return the process group associated with the terminal given by *fd* (an open
   file descriptor as returned by :func:`os.open`).

   Availability: Unix.


.. function:: tcsetpgrp(fd, pg)

   Set the process group associated with the terminal given by *fd* (an open file
   descriptor as returned by :func:`os.open`) to *pg*.

   Availability: Unix.


.. function:: ttyname(fd)

   Return a string which specifies the terminal device associated with
   file descriptor *fd*.  If *fd* is not associated with a terminal device, an
   exception is raised.

   Availability: Unix.


.. function:: write(fd, str)

   Write the bytestring in *str* to file descriptor *fd*. Return the number of
   bytes actually written.

   Availability: Unix, Windows.

   .. note::

      This function is intended for low-level I/O and must be applied to a file
      descriptor as returned by :func:`os.open` or :func:`pipe`.  To write a "file
      object" returned by the built-in function :func:`open` or by :func:`popen` or
      :func:`fdopen`, or :data:`sys.stdout` or :data:`sys.stderr`, use its
      :meth:`~file.write` method.


.. function:: writev(fd, buffers)

   Write the contents of *buffers* to file descriptor *fd*, where *buffers*
   is an arbitrary sequence of buffers.
   Returns the total number of bytes written.

   Availability: Unix.

   .. versionadded:: 3.3


.. _open-constants:

``open()`` flag constants
~~~~~~~~~~~~~~~~~~~~~~~~~

The following constants are options for the *flags* parameter to the
:func:`~os.open` function.  They can be combined using the bitwise OR operator
``|``.  Some of them are not available on all platforms.  For descriptions of
their availability and use, consult the :manpage:`open(2)` manual page on Unix
or `the MSDN <http://msdn.microsoft.com/en-us/library/z0kc8e3z.aspx>`_ on Windows.


.. data:: O_RDONLY
          O_WRONLY
          O_RDWR
          O_APPEND
          O_CREAT
          O_EXCL
          O_TRUNC

   These constants are available on Unix and Windows.


.. data:: O_DSYNC
          O_RSYNC
          O_SYNC
          O_NDELAY
          O_NONBLOCK
          O_NOCTTY
          O_SHLOCK
          O_EXLOCK
          O_CLOEXEC

   These constants are only available on Unix.

   .. versionchanged:: 3.3
      Add :data:`O_CLOEXEC` constant.

.. data:: O_BINARY
          O_NOINHERIT
          O_SHORT_LIVED
          O_TEMPORARY
          O_RANDOM
          O_SEQUENTIAL
          O_TEXT

   These constants are only available on Windows.


.. data:: O_ASYNC
          O_DIRECT
          O_DIRECTORY
          O_NOFOLLOW
          O_NOATIME

   These constants are GNU extensions and not present if they are not defined by
   the C library.


.. data:: RTLD_LAZY
          RTLD_NOW
          RTLD_GLOBAL
          RTLD_LOCAL
          RTLD_NODELETE
          RTLD_NOLOAD
          RTLD_DEEPBIND

   See the Unix manual page :manpage:`dlopen(3)`.

   .. versionadded:: 3.3


.. _terminal-size:

Querying the size of a terminal
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. versionadded:: 3.3

.. function:: get_terminal_size(fd=STDOUT_FILENO)

   Return the size of the terminal window as ``(columns, lines)``,
   tuple of type :class:`terminal_size`.

   The optional argument ``fd`` (default ``STDOUT_FILENO``, or standard
   output) specifies which file descriptor should be queried.

   If the file descriptor is not connected to a terminal, an :exc:`OSError`
   is thrown.

   :func:`shutil.get_terminal_size` is the high-level function which
   should normally be used, ``os.get_terminal_size`` is the low-level
   implementation.

   Availability: Unix, Windows.

.. class:: terminal_size(tuple)

   A tuple of ``(columns, lines)`` for holding terminal window size.

   .. attribute:: columns

      Width of the terminal window in characters.

   .. attribute:: lines

      Height of the terminal window in characters.


.. _os-file-dir:

Files and Directories
---------------------

On some Unix platforms, many of these functions support one or more of these
features:

.. _path_fd:

* For some functions, the *path* argument can be not only a string giving a path
  name, but also a file descriptor.  The function will then operate on the file
  referred to by the descriptor.  (For POSIX systems, this will use the ``f...``
  versions of the function.)

  You can check whether or not *path* can be specified as a file descriptor on
  your platform using :data:`os.supports_fd`.  If it is unavailable, using it
  will raise a :exc:`NotImplementedError`.

  If the function also supports *dir_fd* or *follow_symlinks* arguments, it is
  an error to specify one of those when supplying *path* as a file descriptor.

.. _dir_fd:

* For functions with a *dir_fd* parameter: If *dir_fd* is not ``None``, it
  should be a file descriptor referring to a directory, and the path to operate
  on should be relative; path will then be relative to that directory.  If the
  path is absolute, *dir_fd* is ignored.  (For POSIX systems, this will use the
  ``f...at`` versions of the function.)

  You can check whether or not *dir_fd* is supported on your platform using
  :data:`os.supports_dir_fd`.  If it is unavailable, using it will raise a
  :exc:`NotImplementedError`.

.. _follow_symlinks:

* For functions ith a *follow_symlinks* parameter: If *follow_symlinks* is
  ``False``, and the last element of the path to operate on is a symbolic link,
  the function will operate on the symbolic link itself instead of the file the
  link points to.  (For POSIX systems, this will use the ``l...`` versions of
  the function.)

  You can check whether or not *follow_symlinks* is supported on your platform
  using :data:`os.supports_follow_symlinks`.  If it is unavailable, using it
  will raise a :exc:`NotImplementedError`.



.. function:: access(path, mode, *, dir_fd=None, effective_ids=False, follow_symlinks=True)

   Use the real uid/gid to test for access to *path*.  Note that most operations
   will use the effective uid/gid, therefore this routine can be used in a
   suid/sgid environment to test if the invoking user has the specified access to
   *path*.  *mode* should be :const:`F_OK` to test the existence of *path*, or it
   can be the inclusive OR of one or more of :const:`R_OK`, :const:`W_OK`, and
   :const:`X_OK` to test permissions.  Return :const:`True` if access is allowed,
   :const:`False` if not. See the Unix man page :manpage:`access(2)` for more
   information.

   This function can support specifying :ref:`paths relative to directory
   descriptors <dir_fd>` and :ref:`not following symlinks <follow_symlinks>`.

   If *effective_ids* is ``True``, :func:`access` will perform its access
   checks using the effective uid/gid instead of the real uid/gid.
   *effective_ids* may not be supported on your platform; you can check whether
   or not it is available using :data:`os.supports_effective_ids`.  If it is
   unavailable, using it will raise a :exc:`NotImplementedError`.

   Availability: Unix, Windows.

   .. note::

      Using :func:`access` to check if a user is authorized to e.g. open a file
      before actually doing so using :func:`open` creates a security hole,
      because the user might exploit the short time interval between checking
      and opening the file to manipulate it. It's preferable to use :term:`EAFP`
      techniques. For example::

         if os.access("myfile", os.R_OK):
             with open("myfile") as fp:
                 return fp.read()
         return "some default data"

      is better written as::

         try:
             fp = open("myfile")
         except PermissionError:
             return "some default data"
         else:
             with fp:
                 return fp.read()

   .. note::

      I/O operations may fail even when :func:`access` indicates that they would
      succeed, particularly for operations on network filesystems which may have
      permissions semantics beyond the usual POSIX permission-bit model.

   .. versionchanged:: 3.3
      Added the *dir_fd*, *effective_ids*, and *follow_symlinks* parameters.


.. data:: F_OK
          R_OK
          W_OK
          X_OK

   Values to pass as the *mode* parameter of :func:`access` to test the
   existence, readability, writability and executability of *path*,
   respectively.


.. function:: chdir(path)

   .. index:: single: directory; changing

   Change the current working directory to *path*.

   This function can support :ref:`working on a file descriptor <path_fd>`.  The
   descriptor must refer to an opened directory, not an open file.

   Availability: Unix, Windows.

   .. versionadded:: 3.3
      Added support for specifying *path* as a file descriptor
      on some platforms.


.. function:: chflags(path, flags, *, follow_symlinks=True)

   Set the flags of *path* to the numeric *flags*. *flags* may take a combination
   (bitwise OR) of the following values (as defined in the :mod:`stat` module):

   * :data:`stat.UF_NODUMP`
   * :data:`stat.UF_IMMUTABLE`
   * :data:`stat.UF_APPEND`
   * :data:`stat.UF_OPAQUE`
   * :data:`stat.UF_NOUNLINK`
   * :data:`stat.UF_COMPRESSED`
   * :data:`stat.UF_HIDDEN`
   * :data:`stat.SF_ARCHIVED`
   * :data:`stat.SF_IMMUTABLE`
   * :data:`stat.SF_APPEND`
   * :data:`stat.SF_NOUNLINK`
   * :data:`stat.SF_SNAPSHOT`

   This function can support :ref:`not following symlinks <follow_symlinks>`.

   Availability: Unix.

   .. versionadded:: 3.3
      The *follow_symlinks* argument.


.. function:: chmod(path, mode, *, dir_fd=None, follow_symlinks=True)

   Change the mode of *path* to the numeric *mode*. *mode* may take one of the
   following values (as defined in the :mod:`stat` module) or bitwise ORed
   combinations of them:

   * :data:`stat.S_ISUID`
   * :data:`stat.S_ISGID`
   * :data:`stat.S_ENFMT`
   * :data:`stat.S_ISVTX`
   * :data:`stat.S_IREAD`
   * :data:`stat.S_IWRITE`
   * :data:`stat.S_IEXEC`
   * :data:`stat.S_IRWXU`
   * :data:`stat.S_IRUSR`
   * :data:`stat.S_IWUSR`
   * :data:`stat.S_IXUSR`
   * :data:`stat.S_IRWXG`
   * :data:`stat.S_IRGRP`
   * :data:`stat.S_IWGRP`
   * :data:`stat.S_IXGRP`
   * :data:`stat.S_IRWXO`
   * :data:`stat.S_IROTH`
   * :data:`stat.S_IWOTH`
   * :data:`stat.S_IXOTH`

   This function can support :ref:`specifying a file descriptor <path_fd>`,
   :ref:`paths relative to directory descriptors <dir_fd>` and :ref:`not
   following symlinks <follow_symlinks>`.

   Availability: Unix, Windows.

   .. note::

      Although Windows supports :func:`chmod`, you can only set the file's
      read-only flag with it (via the ``stat.S_IWRITE`` and ``stat.S_IREAD``
      constants or a corresponding integer value).  All other bits are ignored.

   .. versionadded:: 3.3
      Added support for specifying *path* as an open file descriptor,
      and the *dir_fd* and *follow_symlinks* arguments.


.. function:: chown(path, uid, gid, *, dir_fd=None, follow_symlinks=True)

   Change the owner and group id of *path* to the numeric *uid* and *gid*.  To
   leave one of the ids unchanged, set it to -1.

   This function can support :ref:`specifying a file descriptor <path_fd>`,
   :ref:`paths relative to directory descriptors <dir_fd>` and :ref:`not
   following symlinks <follow_symlinks>`.

   See :func:`shutil.chown` for a higher-level function that accepts names in
   addition to numeric ids.

   Availability: Unix.

   .. versionadded:: 3.3
      Added support for specifying an open file descriptor for *path*,
      and the *dir_fd* and *follow_symlinks* arguments.


.. function:: chroot(path)

   Change the root directory of the current process to *path*. Availability:
   Unix.


.. function:: fchdir(fd)

   Change the current working directory to the directory represented by the file
   descriptor *fd*.  The descriptor must refer to an opened directory, not an
   open file.  From Python 3.3, this is equivalent to ``os.chdir(fd)``.

   Availability: Unix.


.. function:: getcwd()

   Return a string representing the current working directory.

   Availability: Unix, Windows.


.. function:: getcwdb()

   Return a bytestring representing the current working directory.

   Availability: Unix, Windows.


.. function:: lchflags(path, flags)

   Set the flags of *path* to the numeric *flags*, like :func:`chflags`, but do
   not follow symbolic links.  From Python 3.3, this is equivalent to
   ``os.chflags(path, flags, follow_symlinks=False)``.

   Availability: Unix.


.. function:: lchmod(path, mode)

   Change the mode of *path* to the numeric *mode*. If path is a symlink, this
   affects the symlink rather than the target.  See the docs for :func:`chmod`
   for possible values of *mode*.  From Python 3.3, this is equivalent to
   ``os.chmod(path, mode, follow_symlinks=False)``.

   Availability: Unix.


.. function:: lchown(path, uid, gid)

   Change the owner and group id of *path* to the numeric *uid* and *gid*.  This
   function will not follow symbolic links.  From Python 3.3, this is equivalent
   to ``os.chown(path, uid, gid, follow_symlinks=False)``.

   Availability: Unix.


.. function:: link(src, dst, *, src_dir_fd=None, dst_dir_fd=None, follow_symlinks=True)

   Create a hard link pointing to *src* named *dst*.

   If either *src_dir_fd* or *dst_dir_fd* is not ``None``, it should be a file
   descriptor referring to a directory, and the corresponding path (*src* or
   *dst*) should be relative; that path will then be relative to that directory.
   (If *src* is absolute, *src_dir_fd* is ignored; the same goes for *dst* and
   *dst_dir_fd*.)  *src_dir_fd* and *dst_dir_fd* may not be supported on your
   platform; you can check whether or not they are available using
   :data:`os.supports_dir_fd`.  If they are unavailable, using either will raise
   a :exc:`NotImplementedError`.

   This function can also support :ref:`not following symlinks
   <follow_symlinks>`.

   Availability: Unix, Windows.

   .. versionchanged:: 3.2
      Added Windows support.

   .. versionadded:: 3.3
      Added the *src_dir_fd*, *dst_dir_fd*, and *follow_symlinks* arguments.


.. function:: listdir(path='.')

   Return a list containing the names of the entries in the directory given by
   *path* (default: ``'.'``).  The list is in arbitrary order.  It does not
   include the special entries ``'.'`` and ``'..'`` even if they are present in
   the directory.

   This function can be called with a bytes or string argument, and returns
   filenames of the same datatype.

   This function can also support :ref:`specifying an open file descriptor
   <path_fd>` (referring to a directory).

   Availability: Unix, Windows.

   .. versionchanged:: 3.2
      The *path* parameter became optional.

   .. versionadded:: 3.3
      Added support for specifying an open file descriptor for *path*.


.. function:: lstat(path, *, dir_fd=None)

   Perform the equivalent of an :c:func:`lstat` system call on the given path.
   Similar to :func:`~os.stat`, but does not follow symbolic links.  On
   platforms that do not support symbolic links, this is an alias for
   :func:`~os.stat`.  From Python 3.3, this is equivalent to ``os.stat(path,
   dir_fd=dir_fd, follow_symlinks=False)``.

   This function can also support :ref:`paths relative to directory descriptors
   <dir_fd>`.

   .. versionchanged:: 3.2
      Added support for Windows 6.0 (Vista) symbolic links.

   .. versionchanged:: 3.3
      Added the *dir_fd* parameter.


.. function:: mkdir(path, mode=0o777, *, dir_fd=None)

   Create a directory named *path* with numeric mode *mode*.

   On some systems, *mode* is ignored.  Where it is used, the current umask
   value is first masked out.  If the directory already exists, :exc:`OSError`
   is raised.

   This function can also support :ref:`paths relative to directory descriptors
   <dir_fd>`.

   It is also possible to create temporary directories; see the
   :mod:`tempfile` module's :func:`tempfile.mkdtemp` function.

   Availability: Unix, Windows.

   .. versionadded:: 3.3
      The *dir_fd* argument.


.. function:: makedirs(path, mode=0o777, exist_ok=False)

   .. index::
      single: directory; creating
      single: UNC paths; and os.makedirs()

   Recursive directory creation function.  Like :func:`mkdir`, but makes all
   intermediate-level directories needed to contain the leaf directory.  If
   the target directory with the same mode as specified already exists,
   raises an :exc:`OSError` exception if *exist_ok* is False, otherwise no
   exception is raised.  If the directory cannot be created in other cases,
   raises an :exc:`OSError` exception.  The default *mode* is ``0o777`` (octal).
   On some systems, *mode* is ignored.  Where it is used, the current umask
   value is first masked out.

   .. note::

      :func:`makedirs` will become confused if the path elements to create
      include :data:`pardir`.

   This function handles UNC paths correctly.

   .. versionadded:: 3.2
      The *exist_ok* parameter.


.. function:: mkfifo(path, mode=0o666, *, dir_fd=None)

   Create a FIFO (a named pipe) named *path* with numeric mode *mode*.
   The current umask value is first masked out from the mode.

   This function can also support :ref:`paths relative to directory descriptors
   <dir_fd>`.

   FIFOs are pipes that can be accessed like regular files.  FIFOs exist until they
   are deleted (for example with :func:`os.unlink`). Generally, FIFOs are used as
   rendezvous between "client" and "server" type processes: the server opens the
   FIFO for reading, and the client opens it for writing.  Note that :func:`mkfifo`
   doesn't open the FIFO --- it just creates the rendezvous point.

   Availability: Unix.

   .. versionadded:: 3.3
      The *dir_fd* argument.


.. function:: mknod(filename, mode=0o600, device=0, *, dir_fd=None)

   Create a filesystem node (file, device special file or named pipe) named
   *filename*. *mode* specifies both the permissions to use and the type of node
   to be created, being combined (bitwise OR) with one of ``stat.S_IFREG``,
   ``stat.S_IFCHR``, ``stat.S_IFBLK``, and ``stat.S_IFIFO`` (those constants are
   available in :mod:`stat`).  For ``stat.S_IFCHR`` and ``stat.S_IFBLK``,
   *device* defines the newly created device special file (probably using
   :func:`os.makedev`), otherwise it is ignored.

   This function can also support :ref:`paths relative to directory descriptors
   <dir_fd>`.

   .. versionadded:: 3.3
      The *dir_fd* argument.


.. function:: major(device)

   Extract the device major number from a raw device number (usually the
   :attr:`st_dev` or :attr:`st_rdev` field from :c:type:`stat`).


.. function:: minor(device)

   Extract the device minor number from a raw device number (usually the
   :attr:`st_dev` or :attr:`st_rdev` field from :c:type:`stat`).


.. function:: makedev(major, minor)

   Compose a raw device number from the major and minor device numbers.


.. function:: pathconf(path, name)

   Return system configuration information relevant to a named file. *name*
   specifies the configuration value to retrieve; it may be a string which is the
   name of a defined system value; these names are specified in a number of
   standards (POSIX.1, Unix 95, Unix 98, and others).  Some platforms define
   additional names as well.  The names known to the host operating system are
   given in the ``pathconf_names`` dictionary.  For configuration variables not
   included in that mapping, passing an integer for *name* is also accepted.

   If *name* is a string and is not known, :exc:`ValueError` is raised.  If a
   specific value for *name* is not supported by the host system, even if it is
   included in ``pathconf_names``, an :exc:`OSError` is raised with
   :const:`errno.EINVAL` for the error number.

   Availability: Unix.


.. data:: pathconf_names

   Dictionary mapping names accepted by :func:`pathconf` and :func:`fpathconf` to
   the integer values defined for those names by the host operating system.  This
   can be used to determine the set of names known to the system. Availability:
   Unix.


.. function:: readlink(path, *, dir_fd=None)

   Return a string representing the path to which the symbolic link points.  The
   result may be either an absolute or relative pathname; if it is relative, it
   may be converted to an absolute pathname using
   ``os.path.join(os.path.dirname(path), result)``.

   If the *path* is a string object, the result will also be a string object,
   and the call may raise an UnicodeDecodeError. If the *path* is a bytes
   object, the result will be a bytes object.

   This function can also support :ref:`paths relative to directory descriptors
   <dir_fd>`.

   Availability: Unix, Windows

   .. versionchanged:: 3.2
      Added support for Windows 6.0 (Vista) symbolic links.

   .. versionadded:: 3.3
      The *dir_fd* argument.


.. function:: remove(path, *, dir_fd=None)

   Remove (delete) the file *path*.  If *path* is a directory, :exc:`OSError` is
   raised.  Use :func:`rmdir` to remove directories.

   This function can support :ref:`paths relative to directory descriptors
   <dir_fd>`.

   On Windows, attempting to remove a file that is in use causes an exception to
   be raised; on Unix, the directory entry is removed but the storage allocated
   to the file is not made available until the original file is no longer in use.

   This function is identical to :func:`unlink`.

   Availability: Unix, Windows.

   .. versionadded:: 3.3
      The *dir_fd* argument.


.. function:: removedirs(path)

   .. index:: single: directory; deleting

   Remove directories recursively.  Works like :func:`rmdir` except that, if the
   leaf directory is successfully removed, :func:`removedirs`  tries to
   successively remove every parent directory mentioned in  *path* until an error
   is raised (which is ignored, because it generally means that a parent directory
   is not empty). For example, ``os.removedirs('foo/bar/baz')`` will first remove
   the directory ``'foo/bar/baz'``, and then remove ``'foo/bar'`` and ``'foo'`` if
   they are empty. Raises :exc:`OSError` if the leaf directory could not be
   successfully removed.


.. function:: rename(src, dst, *, src_dir_fd=None, dst_dir_fd=None)

   Rename the file or directory *src* to *dst*.  If *dst* is a directory,
   :exc:`OSError` will be raised.  On Unix, if *dst* exists and is a file, it will
   be replaced silently if the user has permission.  The operation may fail on some
   Unix flavors if *src* and *dst* are on different filesystems.  If successful,
   the renaming will be an atomic operation (this is a POSIX requirement).  On
   Windows, if *dst* already exists, :exc:`OSError` will be raised even if it is a
   file.

   If either *src_dir_fd* or *dst_dir_fd* is not ``None``, it should be a
   file descriptor referring to a directory, and the corresponding path
   (*src* or *dst*) should be relative; that path will then be relative to
   that directory.  (If *src* is absolute, *src_dir_fd* is ignored; the same
   goes for *dst* and *dst_dir_fd*.)
   *src_dir_fd* and *dst_dir_fd* may not be supported on your platform;
   you can check whether or not they are available using :data:`os.supports_dir_fd`.
   If they are unavailable, using either will raise a :exc:`NotImplementedError`.

   If you want cross-platform overwriting of the destination, use :func:`replace`.

   Availability: Unix, Windows.

   .. versionadded:: 3.3
      The *src_dir_fd* and *dst_dir_fd* arguments.


.. function:: renames(old, new)

   Recursive directory or file renaming function. Works like :func:`rename`, except
   creation of any intermediate directories needed to make the new pathname good is
   attempted first. After the rename, directories corresponding to rightmost path
   segments of the old name will be pruned away using :func:`removedirs`.

   .. note::

      This function can fail with the new directory structure made if you lack
      permissions needed to remove the leaf directory or file.


.. function:: replace(src, dst, *, src_dir_fd=None, dst_dir_fd=None)

   Rename the file or directory *src* to *dst*.  If *dst* is a directory,
   :exc:`OSError` will be raised.  If *dst* exists and is a file, it will
   be replaced silently if the user has permission.  The operation may fail
   if *src* and *dst* are on different filesystems.  If successful,
   the renaming will be an atomic operation (this is a POSIX requirement).

   If either *src_dir_fd* or *dst_dir_fd* is not ``None``, it should be a
   file descriptor referring to a directory, and the corresponding path
   (*src* or *dst*) should be relative; that path will then be relative to
   that directory.  (If *src* is absolute, *src_dir_fd* is ignored; the same
   goes for *dst* and *dst_dir_fd*.)
   *src_dir_fd* and *dst_dir_fd* may not be supported on your platform;
   you can check whether or not they are available using :data:`os.supports_dir_fd`.
   If they are unavailable, using either will raise a :exc:`NotImplementedError`.

   Availability: Unix, Windows

   .. versionadded:: 3.3


.. function:: rmdir(path, *, dir_fd=None)

   Remove (delete) the directory *path*.  Only works when the directory is
   empty, otherwise, :exc:`OSError` is raised.  In order to remove whole
   directory trees, :func:`shutil.rmtree` can be used.

   This function can support :ref:`paths relative to directory descriptors
   <dir_fd>`.

   Availability: Unix, Windows.

   .. versionadded:: 3.3
      The *dir_fd* parameter.


.. function:: stat(path, *, dir_fd=None, follow_symlinks=True)

   Perform the equivalent of a :c:func:`stat` system call on the given path.
   *path* may be specified as either a string or as an open file descriptor.
   (This function normally follows symlinks; to stat a symlink add the argument
   ``follow_symlinks=False``, or use :func:`lstat`.)

   The return value is an object whose attributes correspond roughly
   to the members of the :c:type:`stat` structure, namely:

   * :attr:`st_mode` - protection bits,
   * :attr:`st_ino` - inode number,
   * :attr:`st_dev` - device,
   * :attr:`st_nlink` - number of hard links,
   * :attr:`st_uid` - user id of owner,
   * :attr:`st_gid` - group id of owner,
   * :attr:`st_size` - size of file, in bytes,
   * :attr:`st_atime` - time of most recent access expressed in seconds,
   * :attr:`st_mtime` - time of most recent content modification
     expressed in seconds,
   * :attr:`st_ctime` - platform dependent; time of most recent metadata
     change on Unix, or the time of creation on Windows, expressed in seconds
   * :attr:`st_atime_ns` - time of most recent access
     expressed in nanoseconds as an integer,
   * :attr:`st_mtime_ns` - time of most recent content modification
     expressed in nanoseconds as an integer,
   * :attr:`st_ctime_ns` - platform dependent; time of most recent metadata
     change on Unix, or the time of creation on Windows,
     expressed in nanoseconds as an integer

   On some Unix systems (such as Linux), the following attributes may also be
   available:

   * :attr:`st_blocks` - number of blocks allocated for file
   * :attr:`st_blksize` - filesystem blocksize
   * :attr:`st_rdev` - type of device if an inode device
   * :attr:`st_flags` - user defined flags for file

   On other Unix systems (such as FreeBSD), the following attributes may be
   available (but may be only filled out if root tries to use them):

   * :attr:`st_gen` - file generation number
   * :attr:`st_birthtime` - time of file creation

   On Mac OS systems, the following attributes may also be available:

   * :attr:`st_rsize`
   * :attr:`st_creator`
   * :attr:`st_type`

   .. note::

      The exact meaning and resolution of the :attr:`st_atime`,
      :attr:`st_mtime`, and :attr:`st_ctime` attributes depend on the operating
      system and the file system. For example, on Windows systems using the FAT
      or FAT32 file systems, :attr:`st_mtime` has 2-second resolution, and
      :attr:`st_atime` has only 1-day resolution.  See your operating system
      documentation for details.
      Similarly, although :attr:`st_atime_ns`, :attr:`st_mtime_ns`,
      and :attr:`st_ctime_ns` are always expressed in nanoseconds, many
      systems do not provide nanosecond precision.  On systems that do
      provide nanosecond precision, the floating-point object used to
      store :attr:`st_atime`, :attr:`st_mtime`, and :attr:`st_ctime`
      cannot preserve all of it, and as such will be slightly inexact.
      If you need the exact timestamps you should always use
      :attr:`st_atime_ns`, :attr:`st_mtime_ns`, and :attr:`st_ctime_ns`.

   For backward compatibility, the return value of :func:`~os.stat` is also
   accessible as a tuple of at least 10 integers giving the most important (and
   portable) members of the :c:type:`stat` structure, in the order
   :attr:`st_mode`, :attr:`st_ino`, :attr:`st_dev`, :attr:`st_nlink`,
   :attr:`st_uid`, :attr:`st_gid`, :attr:`st_size`, :attr:`st_atime`,
   :attr:`st_mtime`, :attr:`st_ctime`. More items may be added at the end by
   some implementations.

   This function can support :ref:`specifying an open file descriptor
   <path_fd>`, :ref:`specifying a file descriptor <path_fd>` and :ref:`not
   following symlinks <follow_symlinks>`.

   .. index:: module: stat

   The standard module :mod:`stat` defines functions and constants that are useful
   for extracting information from a :c:type:`stat` structure. (On Windows, some
   items are filled with dummy values.)

   Example::

      >>> import os
      >>> statinfo = os.stat('somefile.txt')
      >>> statinfo
      posix.stat_result(st_mode=33188, st_ino=7876932, st_dev=234881026,
      st_nlink=1, st_uid=501, st_gid=501, st_size=264, st_atime=1297230295,
      st_mtime=1297230027, st_ctime=1297230027)
      >>> statinfo.st_size
      264

   Availability: Unix, Windows.

   .. versionadded:: 3.3
      Added the *dir_fd* and *follow_symlinks* arguments,
      specifying a file descriptor instead of a path,
      and the :attr:`st_atime_ns`, :attr:`st_mtime_ns`,
      and :attr:`st_ctime_ns` members.


.. function:: stat_float_times([newvalue])

   Determine whether :class:`stat_result` represents time stamps as float objects.
   If *newvalue* is ``True``, future calls to :func:`~os.stat` return floats, if it is
   ``False``, future calls return ints. If *newvalue* is omitted, return the
   current setting.

   For compatibility with older Python versions, accessing :class:`stat_result` as
   a tuple always returns integers.

   Python now returns float values by default. Applications which do not work
   correctly with floating point time stamps can use this function to restore the
   old behaviour.

   The resolution of the timestamps (that is the smallest possible fraction)
   depends on the system. Some systems only support second resolution; on these
   systems, the fraction will always be zero.

   It is recommended that this setting is only changed at program startup time in
   the *__main__* module; libraries should never change this setting. If an
   application uses a library that works incorrectly if floating point time stamps
   are processed, this application should turn the feature off until the library
   has been corrected.

   .. deprecated:: 3.3


.. function:: statvfs(path)

   Perform a :c:func:`statvfs` system call on the given path.  The return value is
   an object whose attributes describe the filesystem on the given path, and
   correspond to the members of the :c:type:`statvfs` structure, namely:
   :attr:`f_bsize`, :attr:`f_frsize`, :attr:`f_blocks`, :attr:`f_bfree`,
   :attr:`f_bavail`, :attr:`f_files`, :attr:`f_ffree`, :attr:`f_favail`,
   :attr:`f_flag`, :attr:`f_namemax`.

   Two module-level constants are defined for the :attr:`f_flag` attribute's
   bit-flags: if :const:`ST_RDONLY` is set, the filesystem is mounted
   read-only, and if :const:`ST_NOSUID` is set, the semantics of
   setuid/setgid bits are disabled or not supported.

   This function can support :ref:`specifying a file descriptor <path_fd>`.

   .. versionchanged:: 3.2
      The :const:`ST_RDONLY` and :const:`ST_NOSUID` constants were added.

   Availability: Unix.

   .. versionadded:: 3.3
      Added support for specifying an open file descriptor for *path*.


.. data:: supports_dir_fd

   An object implementing collections.Set indicating which functions in the
   :mod:`os` permit use of their *dir_fd* parameter.  Different platforms
   provide different functionality, and an option that might work on one might
   be unsupported on another.  For consistency's sakes, functions that support
   *dir_fd* always allow specifying the parameter, but will throw an exception
   if the functionality is not actually available.

   To check whether a particular function permits use of its *dir_fd*
   parameter, use the ``in`` operator on ``supports_dir_fd``.  As an example,
   this expression determines whether the *dir_fd* parameter of :func:`os.stat`
   is locally available::

       os.stat in os.supports_dir_fd

   Currently *dir_fd* parameters only work on UNIX platforms;
   none of them work on Windows.

   .. versionadded:: 3.3


.. data:: supports_effective_ids

   An object implementing collections.Set indicating which functions in the
   :mod:`os` permit use of the *effective_ids* parameter for :func:`os.access`.
   If the local platform supports it, the collection will contain
   :func:`os.access`, otherwise it will be empty.

   To check whether you can use the *effective_ids* parameter for
   :func:`os.access`, use the ``in`` operator on ``supports_dir_fd``, like so::

       os.access in os.supports_effective_ids

   Currently *effective_ids* only works on Unix platforms; it does not work on
   Windows.

   .. versionadded:: 3.3


.. data:: supports_fd

   An object implementing collections.Set indicating which functions in the
   :mod:`os` permit specifying their *path* parameter as an open file
   descriptor.  Different platforms provide different functionality, and an
   option that might work on one might be unsupported on another.  For
   consistency's sakes, functions that support *fd* always allow specifying
   the parameter, but will throw an exception if the functionality is not
   actually available.

   To check whether a particular function permits specifying an open file
   descriptor for its *path* parameter, use the ``in`` operator on
   ``supports_fd``. As an example, this expression determines whether
   :func:`os.chdir` accepts open file descriptors when called on your local
   platform::

       os.chdir in os.supports_fd

   .. versionadded:: 3.3


.. data:: supports_follow_symlinks

   An object implementing collections.Set indicating which functions in the
   :mod:`os` permit use of their *follow_symlinks* parameter.  Different
   platforms provide different functionality, and an option that might work on
   one might be unsupported on another.  For consistency's sakes, functions that
   support *follow_symlinks* always allow specifying the parameter, but will
   throw an exception if the functionality is not actually available.

   To check whether a particular function permits use of its *follow_symlinks*
   parameter, use the ``in`` operator on ``supports_follow_symlinks``.  As an
   example, this expression determines whether the *follow_symlinks* parameter
   of :func:`os.stat` is locally available::

       os.stat in os.supports_follow_symlinks

   .. versionadded:: 3.3


.. function:: symlink(source, link_name, target_is_directory=False, *, dir_fd=None)

   Create a symbolic link pointing to *source* named *link_name*.

   On Windows, a symlink represents either a file or a directory, and does not
   morph to the target dynamically.  If *target_is_directory* is set to ``True``,
   the symlink will be created as a directory symlink, otherwise as a file symlink
   (the default).  On non-Window platforms, *target_is_directory* is ignored.

   Symbolic link support was introduced in Windows 6.0 (Vista).  :func:`symlink`
   will raise a :exc:`NotImplementedError` on Windows versions earlier than 6.0.

   This function can support :ref:`paths relative to directory descriptors
   <dir_fd>`.

   .. note::

      On Windows, the *SeCreateSymbolicLinkPrivilege* is required in order to
      successfully create symlinks. This privilege is not typically granted to
      regular users but is available to accounts which can escalate privileges
      to the administrator level. Either obtaining the privilege or running your
      application as an administrator are ways to successfully create symlinks.

      :exc:`OSError` is raised when the function is called by an unprivileged
      user.

   Availability: Unix, Windows.

   .. versionchanged:: 3.2
      Added support for Windows 6.0 (Vista) symbolic links.

   .. versionadded:: 3.3
      Added the *dir_fd* argument, and now allow *target_is_directory*
      on non-Windows platforms.


.. function:: sync()

   Force write of everything to disk.

   Availability: Unix.

   .. versionadded:: 3.3


.. function:: truncate(path, length)

   Truncate the file corresponding to *path*, so that it is at most
   *length* bytes in size.

   Availability: Unix.

   .. versionadded:: 3.3


.. function:: unlink(path, *, dir_fd=None)

   Remove (delete) the file *path*.  This function is identical to
   :func:`remove`; the ``unlink`` name is its traditional Unix
   name.  Please see the documentation for :func:`remove` for
   further information.

   Availability: Unix, Windows.

   .. versionadded:: 3.3
      The *dir_fd* parameter.


.. function:: utime(path, times=None, *, ns=None, dir_fd=None, follow_symlinks=True)

   Set the access and modified times of the file specified by *path*.

   :func:`utime` takes two optional parameters, *times* and *ns*.
   These specify the times set on *path* and are used as follows:

   - If *ns* is not ``None``,
     it must be a 2-tuple of the form ``(atime_ns, mtime_ns)``
     where each member is an int expressing nanoseconds.
   - If *times* is not ``None``,
     it must be a 2-tuple of the form ``(atime, mtime)``
     where each member is an int or float expressing seconds.
   - If *times* and *ns* are both ``None``,
     this is equivalent to specifying ``ns=(atime_ns, mtime_ns)``
     where both times are the current time.
     (The effect is similar to running the Unix program
     :program:`touch` on *path*.)

   It is an error to specify tuples for both *times* and *ns*.

   Whether a directory can be given for *path*
   depends on whether the operating system implements directories as files
   (for example, Windows does not).  Note that the exact times you set here may
   not be returned by a subsequent :func:`~os.stat` call, depending on the
   resolution with which your operating system records access and modification
   times; see :func:`~os.stat`.  The best way to preserve exact times is to
   use the *st_atime_ns* and *st_mtime_ns* fields from the :func:`os.stat`
   result object with the *ns* parameter to `utime`.

   This function can support :ref:`specifying a file descriptor <path_fd>`,
   :ref:`paths relative to directory descriptors <dir_fd>` and :ref:`not
   following symlinks <follow_symlinks>`.

   Availability: Unix, Windows.

   .. versionadded:: 3.3
      Added support for specifying an open file descriptor for *path*,
      and the *dir_fd*, *follow_symlinks*, and *ns* parameters.


.. function:: walk(top, topdown=True, onerror=None, followlinks=False)

   .. index::
      single: directory; walking
      single: directory; traversal

   Generate the file names in a directory tree by walking the tree
   either top-down or bottom-up. For each directory in the tree rooted at directory
   *top* (including *top* itself), it yields a 3-tuple ``(dirpath, dirnames,
   filenames)``.

   *dirpath* is a string, the path to the directory.  *dirnames* is a list of the
   names of the subdirectories in *dirpath* (excluding ``'.'`` and ``'..'``).
   *filenames* is a list of the names of the non-directory files in *dirpath*.
   Note that the names in the lists contain no path components.  To get a full path
   (which begins with *top*) to a file or directory in *dirpath*, do
   ``os.path.join(dirpath, name)``.

   If optional argument *topdown* is ``True`` or not specified, the triple for a
   directory is generated before the triples for any of its subdirectories
   (directories are generated top-down).  If *topdown* is ``False``, the triple for a
   directory is generated after the triples for all of its subdirectories
   (directories are generated bottom-up).

   When *topdown* is ``True``, the caller can modify the *dirnames* list in-place
   (perhaps using :keyword:`del` or slice assignment), and :func:`walk` will only
   recurse into the subdirectories whose names remain in *dirnames*; this can be
   used to prune the search, impose a specific order of visiting, or even to inform
   :func:`walk` about directories the caller creates or renames before it resumes
   :func:`walk` again.  Modifying *dirnames* when *topdown* is ``False`` is
   ineffective, because in bottom-up mode the directories in *dirnames* are
   generated before *dirpath* itself is generated.

   By default, errors from the :func:`listdir` call are ignored.  If optional
   argument *onerror* is specified, it should be a function; it will be called with
   one argument, an :exc:`OSError` instance.  It can report the error to continue
   with the walk, or raise the exception to abort the walk.  Note that the filename
   is available as the ``filename`` attribute of the exception object.

   By default, :func:`walk` will not walk down into symbolic links that resolve to
   directories. Set *followlinks* to ``True`` to visit directories pointed to by
   symlinks, on systems that support them.

   .. note::

      Be aware that setting *followlinks* to ``True`` can lead to infinite
      recursion if a link points to a parent directory of itself. :func:`walk`
      does not keep track of the directories it visited already.

   .. note::

      If you pass a relative pathname, don't change the current working directory
      between resumptions of :func:`walk`.  :func:`walk` never changes the current
      directory, and assumes that its caller doesn't either.

   This example displays the number of bytes taken by non-directory files in each
   directory under the starting directory, except that it doesn't look under any
   CVS subdirectory::

      import os
      from os.path import join, getsize
      for root, dirs, files in os.walk('python/Lib/email'):
          print(root, "consumes", end=" ")
          print(sum(getsize(join(root, name)) for name in files), end=" ")
          print("bytes in", len(files), "non-directory files")
          if 'CVS' in dirs:
              dirs.remove('CVS')  # don't visit CVS directories

   In the next example, walking the tree bottom-up is essential: :func:`rmdir`
   doesn't allow deleting a directory before the directory is empty::

      # Delete everything reachable from the directory named in "top",
      # assuming there are no symbolic links.
      # CAUTION:  This is dangerous!  For example, if top == '/', it
      # could delete all your disk files.
      import os
      for root, dirs, files in os.walk(top, topdown=False):
          for name in files:
              os.remove(os.path.join(root, name))
          for name in dirs:
              os.rmdir(os.path.join(root, name))


.. function:: fwalk(top, topdown=True, onerror=None, followlinks=False)

   .. index::
      single: directory; walking
      single: directory; traversal

   This behaves exactly like :func:`walk`, except that it yields a 4-tuple
   ``(dirpath, dirnames, filenames, dirfd)``.

   *dirpath*, *dirnames* and *filenames* are identical to :func:`walk` output,
   and *dirfd* is a file descriptor referring to the directory *dirpath*.

   .. note::

      Since :func:`fwalk` yields file descriptors, those are only valid until
      the next iteration step, so you should duplicate them (e.g. with
      :func:`dup`) if you want to keep them longer.

   This example displays the number of bytes taken by non-directory files in each
   directory under the starting directory, except that it doesn't look under any
   CVS subdirectory::

      import os
      for root, dirs, files, rootfd in os.fwalk('python/Lib/email'):
          print(root, "consumes", end="")
          print(sum([os.fstatat(rootfd, name).st_size for name in files]),
                end="")
          print("bytes in", len(files), "non-directory files")
          if 'CVS' in dirs:
              dirs.remove('CVS')  # don't visit CVS directories

   In the next example, walking the tree bottom-up is essential:
   :func:`unlinkat` doesn't allow deleting a directory before the directory is
   empty::

      # Delete everything reachable from the directory named in "top",
      # assuming there are no symbolic links.
      # CAUTION:  This is dangerous!  For example, if top == '/', it
      # could delete all your disk files.
      import os
      for root, dirs, files, rootfd in os.fwalk(top, topdown=False):
          for name in files:
              os.unlinkat(rootfd, name)
          for name in dirs:
              os.unlinkat(rootfd, name, os.AT_REMOVEDIR)

   Availability: Unix.

   .. versionadded:: 3.3


Linux extended attributes
~~~~~~~~~~~~~~~~~~~~~~~~~

.. versionadded:: 3.3

These functions are all available on Linux only.

.. function:: getxattr(path, attribute, *, follow_symlinks=True)

   Return the value of the extended filesystem attribute *attribute* for
   *path*. *attribute* can be bytes or str. If it is str, it is encoded
   with the filesystem encoding.

   This function can support :ref:`specifying a file descriptor <path_fd>` and
   :ref:`not following symlinks <follow_symlinks>`.


.. function:: listxattr(path=None, *, follow_symlinks=True)

   Return a list of the extended filesystem attributes on *path*.  The
   attributes in the list are represented as strings decoded with the filesystem
   encoding.  If *path* is ``None``, :func:`listxattr` will examine the current
   directory.

   This function can support :ref:`specifying a file descriptor <path_fd>` and
   :ref:`not following symlinks <follow_symlinks>`.


.. function:: removexattr(path, attribute, *, follow_symlinks=True)

   Removes the extended filesystem attribute *attribute* from *path*.
   *attribute* should be bytes or str. If it is a string, it is encoded
   with the filesystem encoding.

   This function can support :ref:`specifying a file descriptor <path_fd>` and
   :ref:`not following symlinks <follow_symlinks>`.


.. function:: setxattr(path, attribute, value, flags=0, *, follow_symlinks=True)

   Set the extended filesystem attribute *attribute* on *path* to *value*.
   *attribute* must be a bytes or str with no embedded NULs. If it is a str,
   it is encoded with the filesystem encoding.  *flags* may be
   :data:`XATTR_REPLACE` or :data:`XATTR_CREATE`. If :data:`XATTR_REPLACE` is
   given and the attribute does not exist, ``EEXISTS`` will be raised.
   If :data:`XATTR_CREATE` is given and the attribute already exists, the
   attribute will not be created and ``ENODATA`` will be raised.

   This function can support :ref:`specifying a file descriptor <path_fd>` and
   :ref:`not following symlinks <follow_symlinks>`.

   .. note::

      A bug in Linux kernel versions less than 2.6.39 caused the flags argument
      to be ignored on some filesystems.


.. data:: XATTR_SIZE_MAX

   The maximum size the value of an extended attribute can be. Currently, this
   is 64 kilobytes on Linux.


.. data:: XATTR_CREATE

   This is a possible value for the flags argument in :func:`setxattr`. It
   indicates the operation must create an attribute.


.. data:: XATTR_REPLACE

   This is a possible value for the flags argument in :func:`setxattr`. It
   indicates the operation must replace an existing attribute.


.. _os-process:

Process Management
------------------

These functions may be used to create and manage processes.

The various :func:`exec\*` functions take a list of arguments for the new
program loaded into the process.  In each case, the first of these arguments is
passed to the new program as its own name rather than as an argument a user may
have typed on a command line.  For the C programmer, this is the ``argv[0]``
passed to a program's :c:func:`main`.  For example, ``os.execv('/bin/echo',
['foo', 'bar'])`` will only print ``bar`` on standard output; ``foo`` will seem
to be ignored.


.. function:: abort()

   Generate a :const:`SIGABRT` signal to the current process.  On Unix, the default
   behavior is to produce a core dump; on Windows, the process immediately returns
   an exit code of ``3``.  Be aware that calling this function will not call the
   Python signal handler registered for :const:`SIGABRT` with
   :func:`signal.signal`.

   Availability: Unix, Windows.


.. function:: execl(path, arg0, arg1, ...)
              execle(path, arg0, arg1, ..., env)
              execlp(file, arg0, arg1, ...)
              execlpe(file, arg0, arg1, ..., env)
              execv(path, args)
              execve(path, args, env)
              execvp(file, args)
              execvpe(file, args, env)

   These functions all execute a new program, replacing the current process; they
   do not return.  On Unix, the new executable is loaded into the current process,
   and will have the same process id as the caller.  Errors will be reported as
   :exc:`OSError` exceptions.

   The current process is replaced immediately. Open file objects and
   descriptors are not flushed, so if there may be data buffered
   on these open files, you should flush them using
   :func:`sys.stdout.flush` or :func:`os.fsync` before calling an
   :func:`exec\*` function.

   The "l" and "v" variants of the :func:`exec\*` functions differ in how
   command-line arguments are passed.  The "l" variants are perhaps the easiest
   to work with if the number of parameters is fixed when the code is written; the
   individual parameters simply become additional parameters to the :func:`execl\*`
   functions.  The "v" variants are good when the number of parameters is
   variable, with the arguments being passed in a list or tuple as the *args*
   parameter.  In either case, the arguments to the child process should start with
   the name of the command being run, but this is not enforced.

   The variants which include a "p" near the end (:func:`execlp`,
   :func:`execlpe`, :func:`execvp`, and :func:`execvpe`) will use the
   :envvar:`PATH` environment variable to locate the program *file*.  When the
   environment is being replaced (using one of the :func:`exec\*e` variants,
   discussed in the next paragraph), the new environment is used as the source of
   the :envvar:`PATH` variable. The other variants, :func:`execl`, :func:`execle`,
   :func:`execv`, and :func:`execve`, will not use the :envvar:`PATH` variable to
   locate the executable; *path* must contain an appropriate absolute or relative
   path.

   For :func:`execle`, :func:`execlpe`, :func:`execve`, and :func:`execvpe` (note
   that these all end in "e"), the *env* parameter must be a mapping which is
   used to define the environment variables for the new process (these are used
   instead of the current process' environment); the functions :func:`execl`,
   :func:`execlp`, :func:`execv`, and :func:`execvp` all cause the new process to
   inherit the environment of the current process.

   For :func:`execve` on some platforms, *path* may also be specified as an open
   file descriptor.  This functionality may not be supported on your platform;
   you can check whether or not it is available using :data:`os.supports_fd`.
   If it is unavailable, using it will raise a :exc:`NotImplementedError`.

   Availability: Unix, Windows.

   .. versionadded:: 3.3
      Added support for specifying an open file descriptor for *path*
      for :func:`execve`.

.. function:: _exit(n)

   Exit the process with status *n*, without calling cleanup handlers, flushing
   stdio buffers, etc.

   Availability: Unix, Windows.

   .. note::

      The standard way to exit is ``sys.exit(n)``.  :func:`_exit` should
      normally only be used in the child process after a :func:`fork`.

The following exit codes are defined and can be used with :func:`_exit`,
although they are not required.  These are typically used for system programs
written in Python, such as a mail server's external command delivery program.

.. note::

   Some of these may not be available on all Unix platforms, since there is some
   variation.  These constants are defined where they are defined by the underlying
   platform.


.. data:: EX_OK

   Exit code that means no error occurred.

   Availability: Unix.


.. data:: EX_USAGE

   Exit code that means the command was used incorrectly, such as when the wrong
   number of arguments are given.

   Availability: Unix.


.. data:: EX_DATAERR

   Exit code that means the input data was incorrect.

   Availability: Unix.


.. data:: EX_NOINPUT

   Exit code that means an input file did not exist or was not readable.

   Availability: Unix.


.. data:: EX_NOUSER

   Exit code that means a specified user did not exist.

   Availability: Unix.


.. data:: EX_NOHOST

   Exit code that means a specified host did not exist.

   Availability: Unix.


.. data:: EX_UNAVAILABLE

   Exit code that means that a required service is unavailable.

   Availability: Unix.


.. data:: EX_SOFTWARE

   Exit code that means an internal software error was detected.

   Availability: Unix.


.. data:: EX_OSERR

   Exit code that means an operating system error was detected, such as the
   inability to fork or create a pipe.

   Availability: Unix.


.. data:: EX_OSFILE

   Exit code that means some system file did not exist, could not be opened, or had
   some other kind of error.

   Availability: Unix.


.. data:: EX_CANTCREAT

   Exit code that means a user specified output file could not be created.

   Availability: Unix.


.. data:: EX_IOERR

   Exit code that means that an error occurred while doing I/O on some file.

   Availability: Unix.


.. data:: EX_TEMPFAIL

   Exit code that means a temporary failure occurred.  This indicates something
   that may not really be an error, such as a network connection that couldn't be
   made during a retryable operation.

   Availability: Unix.


.. data:: EX_PROTOCOL

   Exit code that means that a protocol exchange was illegal, invalid, or not
   understood.

   Availability: Unix.


.. data:: EX_NOPERM

   Exit code that means that there were insufficient permissions to perform the
   operation (but not intended for file system problems).

   Availability: Unix.


.. data:: EX_CONFIG

   Exit code that means that some kind of configuration error occurred.

   Availability: Unix.


.. data:: EX_NOTFOUND

   Exit code that means something like "an entry was not found".

   Availability: Unix.


.. function:: fork()

   Fork a child process.  Return ``0`` in the child and the child's process id in the
   parent.  If an error occurs :exc:`OSError` is raised.

   Note that some platforms including FreeBSD <= 6.3, Cygwin and OS/2 EMX have
   known issues when using fork() from a thread.

   Availability: Unix.


.. function:: forkpty()

   Fork a child process, using a new pseudo-terminal as the child's controlling
   terminal. Return a pair of ``(pid, fd)``, where *pid* is ``0`` in the child, the
   new child's process id in the parent, and *fd* is the file descriptor of the
   master end of the pseudo-terminal.  For a more portable approach, use the
   :mod:`pty` module.  If an error occurs :exc:`OSError` is raised.

   Availability: some flavors of Unix.


.. function:: kill(pid, sig)

   .. index::
      single: process; killing
      single: process; signalling

   Send signal *sig* to the process *pid*.  Constants for the specific signals
   available on the host platform are defined in the :mod:`signal` module.

   Windows: The :data:`signal.CTRL_C_EVENT` and
   :data:`signal.CTRL_BREAK_EVENT` signals are special signals which can
   only be sent to console processes which share a common console window,
   e.g., some subprocesses. Any other value for *sig* will cause the process
   to be unconditionally killed by the TerminateProcess API, and the exit code
   will be set to *sig*. The Windows version of :func:`kill` additionally takes
   process handles to be killed.

   See also :func:`signal.pthread_kill`.

   .. versionadded:: 3.2
      Windows support.


.. function:: killpg(pgid, sig)

   .. index::
      single: process; killing
      single: process; signalling

   Send the signal *sig* to the process group *pgid*.

   Availability: Unix.


.. function:: nice(increment)

   Add *increment* to the process's "niceness".  Return the new niceness.

   Availability: Unix.


.. function:: plock(op)

   Lock program segments into memory.  The value of *op* (defined in
   ``<sys/lock.h>``) determines which segments are locked.

   Availability: Unix.


.. function:: popen(...)
   :noindex:

   Run child processes, returning opened pipes for communications.  These functions
   are described in section :ref:`os-newstreams`.


.. function:: spawnl(mode, path, ...)
              spawnle(mode, path, ..., env)
              spawnlp(mode, file, ...)
              spawnlpe(mode, file, ..., env)
              spawnv(mode, path, args)
              spawnve(mode, path, args, env)
              spawnvp(mode, file, args)
              spawnvpe(mode, file, args, env)

   Execute the program *path* in a new process.

   (Note that the :mod:`subprocess` module provides more powerful facilities for
   spawning new processes and retrieving their results; using that module is
   preferable to using these functions.  Check especially the
   :ref:`subprocess-replacements` section.)

   If *mode* is :const:`P_NOWAIT`, this function returns the process id of the new
   process; if *mode* is :const:`P_WAIT`, returns the process's exit code if it
   exits normally, or ``-signal``, where *signal* is the signal that killed the
   process.  On Windows, the process id will actually be the process handle, so can
   be used with the :func:`waitpid` function.

   The "l" and "v" variants of the :func:`spawn\*` functions differ in how
   command-line arguments are passed.  The "l" variants are perhaps the easiest
   to work with if the number of parameters is fixed when the code is written; the
   individual parameters simply become additional parameters to the
   :func:`spawnl\*` functions.  The "v" variants are good when the number of
   parameters is variable, with the arguments being passed in a list or tuple as
   the *args* parameter.  In either case, the arguments to the child process must
   start with the name of the command being run.

   The variants which include a second "p" near the end (:func:`spawnlp`,
   :func:`spawnlpe`, :func:`spawnvp`, and :func:`spawnvpe`) will use the
   :envvar:`PATH` environment variable to locate the program *file*.  When the
   environment is being replaced (using one of the :func:`spawn\*e` variants,
   discussed in the next paragraph), the new environment is used as the source of
   the :envvar:`PATH` variable.  The other variants, :func:`spawnl`,
   :func:`spawnle`, :func:`spawnv`, and :func:`spawnve`, will not use the
   :envvar:`PATH` variable to locate the executable; *path* must contain an
   appropriate absolute or relative path.

   For :func:`spawnle`, :func:`spawnlpe`, :func:`spawnve`, and :func:`spawnvpe`
   (note that these all end in "e"), the *env* parameter must be a mapping
   which is used to define the environment variables for the new process (they are
   used instead of the current process' environment); the functions
   :func:`spawnl`, :func:`spawnlp`, :func:`spawnv`, and :func:`spawnvp` all cause
   the new process to inherit the environment of the current process.  Note that
   keys and values in the *env* dictionary must be strings; invalid keys or
   values will cause the function to fail, with a return value of ``127``.

   As an example, the following calls to :func:`spawnlp` and :func:`spawnvpe` are
   equivalent::

      import os
      os.spawnlp(os.P_WAIT, 'cp', 'cp', 'index.html', '/dev/null')

      L = ['cp', 'index.html', '/dev/null']
      os.spawnvpe(os.P_WAIT, 'cp', L, os.environ)

   Availability: Unix, Windows.  :func:`spawnlp`, :func:`spawnlpe`, :func:`spawnvp`
   and :func:`spawnvpe` are not available on Windows.  :func:`spawnle` and
   :func:`spawnve` are not thread-safe on Windows; we advise you to use the
   :mod:`subprocess` module instead.


.. data:: P_NOWAIT
          P_NOWAITO

   Possible values for the *mode* parameter to the :func:`spawn\*` family of
   functions.  If either of these values is given, the :func:`spawn\*` functions
   will return as soon as the new process has been created, with the process id as
   the return value.

   Availability: Unix, Windows.


.. data:: P_WAIT

   Possible value for the *mode* parameter to the :func:`spawn\*` family of
   functions.  If this is given as *mode*, the :func:`spawn\*` functions will not
   return until the new process has run to completion and will return the exit code
   of the process the run is successful, or ``-signal`` if a signal kills the
   process.

   Availability: Unix, Windows.


.. data:: P_DETACH
          P_OVERLAY

   Possible values for the *mode* parameter to the :func:`spawn\*` family of
   functions.  These are less portable than those listed above. :const:`P_DETACH`
   is similar to :const:`P_NOWAIT`, but the new process is detached from the
   console of the calling process. If :const:`P_OVERLAY` is used, the current
   process will be replaced; the :func:`spawn\*` function will not return.

   Availability: Windows.


.. function:: startfile(path[, operation])

   Start a file with its associated application.

   When *operation* is not specified or ``'open'``, this acts like double-clicking
   the file in Windows Explorer, or giving the file name as an argument to the
   :program:`start` command from the interactive command shell: the file is opened
   with whatever application (if any) its extension is associated.

   When another *operation* is given, it must be a "command verb" that specifies
   what should be done with the file. Common verbs documented by Microsoft are
   ``'print'`` and  ``'edit'`` (to be used on files) as well as ``'explore'`` and
   ``'find'`` (to be used on directories).

   :func:`startfile` returns as soon as the associated application is launched.
   There is no option to wait for the application to close, and no way to retrieve
   the application's exit status.  The *path* parameter is relative to the current
   directory.  If you want to use an absolute path, make sure the first character
   is not a slash (``'/'``); the underlying Win32 :c:func:`ShellExecute` function
   doesn't work if it is.  Use the :func:`os.path.normpath` function to ensure that
   the path is properly encoded for Win32.

   Availability: Windows.


.. function:: system(command)

   Execute the command (a string) in a subshell.  This is implemented by calling
   the Standard C function :c:func:`system`, and has the same limitations.
   Changes to :data:`sys.stdin`, etc. are not reflected in the environment of
   the executed command. If *command* generates any output, it will be sent to
   the interpreter standard output stream.

   On Unix, the return value is the exit status of the process encoded in the
   format specified for :func:`wait`.  Note that POSIX does not specify the
   meaning of the return value of the C :c:func:`system` function, so the return
   value of the Python function is system-dependent.

   On Windows, the return value is that returned by the system shell after
   running *command*.  The shell is given by the Windows environment variable
   :envvar:`COMSPEC`: it is usually :program:`cmd.exe`, which returns the exit
   status of the command run; on systems using a non-native shell, consult your
   shell documentation.

   The :mod:`subprocess` module provides more powerful facilities for spawning
   new processes and retrieving their results; using that module is preferable
   to using this function.  See the :ref:`subprocess-replacements` section in
   the :mod:`subprocess` documentation for some helpful recipes.

   Availability: Unix, Windows.


.. function:: times()

   Return a 5-tuple of floating point numbers indicating accumulated (processor
   or other) times, in seconds.  The items are: user time, system time,
   children's user time, children's system time, and elapsed real time since a
   fixed point in the past, in that order.  See the Unix manual page
   :manpage:`times(2)` or the corresponding Windows Platform API documentation.
   On Windows, only the first two items are filled, the others are zero.

   Availability: Unix, Windows


.. function:: wait()

   Wait for completion of a child process, and return a tuple containing its pid
   and exit status indication: a 16-bit number, whose low byte is the signal number
   that killed the process, and whose high byte is the exit status (if the signal
   number is zero); the high bit of the low byte is set if a core file was
   produced.

   Availability: Unix.

.. function:: waitid(idtype, id, options)

   Wait for the completion of one or more child processes.
   *idtype* can be :data:`P_PID`, :data:`P_PGID` or :data:`P_ALL`.
   *id* specifies the pid to wait on.
   *options* is constructed from the ORing of one or more of :data:`WEXITED`,
   :data:`WSTOPPED` or :data:`WCONTINUED` and additionally may be ORed with
   :data:`WNOHANG` or :data:`WNOWAIT`. The return value is an object
   representing the data contained in the :c:type:`siginfo_t` structure, namely:
   :attr:`si_pid`, :attr:`si_uid`, :attr:`si_signo`, :attr:`si_status`,
   :attr:`si_code` or ``None`` if :data:`WNOHANG` is specified and there are no
   children in a waitable state.

   Availability: Unix.

   .. versionadded:: 3.3

.. data:: P_PID
          P_PGID
          P_ALL

   These are the possible values for *idtype* in :func:`waitid`. They affect
   how *id* is interpreted.

   Availability: Unix.

   .. versionadded:: 3.3

.. data:: WEXITED
          WSTOPPED
          WNOWAIT

   Flags that can be used in *options* in :func:`waitid` that specify what
   child signal to wait for.

   Availability: Unix.

   .. versionadded:: 3.3


.. data:: CLD_EXITED
          CLD_DUMPED
          CLD_TRAPPED
          CLD_CONTINUED

   These are the possible values for :attr:`si_code` in the result returned by
   :func:`waitid`.

   Availability: Unix.

   .. versionadded:: 3.3


.. function:: waitpid(pid, options)

   The details of this function differ on Unix and Windows.

   On Unix: Wait for completion of a child process given by process id *pid*, and
   return a tuple containing its process id and exit status indication (encoded as
   for :func:`wait`).  The semantics of the call are affected by the value of the
   integer *options*, which should be ``0`` for normal operation.

   If *pid* is greater than ``0``, :func:`waitpid` requests status information for
   that specific process.  If *pid* is ``0``, the request is for the status of any
   child in the process group of the current process.  If *pid* is ``-1``, the
   request pertains to any child of the current process.  If *pid* is less than
   ``-1``, status is requested for any process in the process group ``-pid`` (the
   absolute value of *pid*).

   An :exc:`OSError` is raised with the value of errno when the syscall
   returns -1.

   On Windows: Wait for completion of a process given by process handle *pid*, and
   return a tuple containing *pid*, and its exit status shifted left by 8 bits
   (shifting makes cross-platform use of the function easier). A *pid* less than or
   equal to ``0`` has no special meaning on Windows, and raises an exception. The
   value of integer *options* has no effect. *pid* can refer to any process whose
   id is known, not necessarily a child process. The :func:`spawn` functions called
   with :const:`P_NOWAIT` return suitable process handles.


.. function:: wait3([options])

   Similar to :func:`waitpid`, except no process id argument is given and a
   3-element tuple containing the child's process id, exit status indication, and
   resource usage information is returned.  Refer to :mod:`resource`.\
   :func:`getrusage` for details on resource usage information.  The option
   argument is the same as that provided to :func:`waitpid` and :func:`wait4`.

   Availability: Unix.


.. function:: wait4(pid, options)

   Similar to :func:`waitpid`, except a 3-element tuple, containing the child's
   process id, exit status indication, and resource usage information is returned.
   Refer to :mod:`resource`.\ :func:`getrusage` for details on resource usage
   information.  The arguments to :func:`wait4` are the same as those provided to
   :func:`waitpid`.

   Availability: Unix.


.. data:: WNOHANG

   The option for :func:`waitpid` to return immediately if no child process status
   is available immediately. The function returns ``(0, 0)`` in this case.

   Availability: Unix.


.. data:: WCONTINUED

   This option causes child processes to be reported if they have been continued
   from a job control stop since their status was last reported.

   Availability: Some Unix systems.


.. data:: WUNTRACED

   This option causes child processes to be reported if they have been stopped but
   their current state has not been reported since they were stopped.

   Availability: Unix.


The following functions take a process status code as returned by
:func:`system`, :func:`wait`, or :func:`waitpid` as a parameter.  They may be
used to determine the disposition of a process.

.. function:: WCOREDUMP(status)

   Return ``True`` if a core dump was generated for the process, otherwise
   return ``False``.

   Availability: Unix.


.. function:: WIFCONTINUED(status)

   Return ``True`` if the process has been continued from a job control stop,
   otherwise return ``False``.

   Availability: Unix.


.. function:: WIFSTOPPED(status)

   Return ``True`` if the process has been stopped, otherwise return
   ``False``.

   Availability: Unix.


.. function:: WIFSIGNALED(status)

   Return ``True`` if the process exited due to a signal, otherwise return
   ``False``.

   Availability: Unix.


.. function:: WIFEXITED(status)

   Return ``True`` if the process exited using the :manpage:`exit(2)` system call,
   otherwise return ``False``.

   Availability: Unix.


.. function:: WEXITSTATUS(status)

   If ``WIFEXITED(status)`` is true, return the integer parameter to the
   :manpage:`exit(2)` system call.  Otherwise, the return value is meaningless.

   Availability: Unix.


.. function:: WSTOPSIG(status)

   Return the signal which caused the process to stop.

   Availability: Unix.


.. function:: WTERMSIG(status)

   Return the signal which caused the process to exit.

   Availability: Unix.


Interface to the scheduler
--------------------------

These functions control how a process is allocated CPU time by the operating
system. They are only available on some Unix platforms. For more detailed
information, consult your Unix manpages.

.. versionadded:: 3.3

The following scheduling policies are exposed if they are a supported by the
operating system.

.. data:: SCHED_OTHER

   The default scheduling policy.

.. data:: SCHED_BATCH

   Scheduling policy for CPU-intensive processes that tries to preserve
   interactivity on the rest of the computer.

.. data:: SCHED_IDLE

   Scheduling policy for extremely low priority background tasks.

.. data:: SCHED_SPORADIC

   Scheduling policy for sporadic server programs.

.. data:: SCHED_FIFO

   A First In First Out scheduling policy.

.. data:: SCHED_RR

   A round-robin scheduling policy.

.. data:: SCHED_RESET_ON_FORK

   This flag can OR'ed with any other scheduling policy. When a process with
   this flag set forks, its child's scheduling policy and priority are reset to
   the default.


.. class:: sched_param(sched_priority)

   This class represents tunable scheduling parameters used in
   :func:`sched_setparam`, :func:`sched_setscheduler`, and
   :func:`sched_getparam`. It is immutable.

   At the moment, there is only one possible parameter:

   .. attribute:: sched_priority

      The scheduling priority for a scheduling policy.


.. function:: sched_get_priority_min(policy)

   Get the minimum priority value for *policy*. *policy* is one of the
   scheduling policy constants above.


.. function:: sched_get_priority_max(policy)

   Get the maximum priority value for *policy*. *policy* is one of the
   scheduling policy constants above.


.. function:: sched_setscheduler(pid, policy, param)

   Set the scheduling policy for the process with PID *pid*. A *pid* of 0 means
   the calling process. *policy* is one of the scheduling policy constants
   above. *param* is a :class:`sched_param` instance.


.. function:: sched_getscheduler(pid)

   Return the scheduling policy for the process with PID *pid*. A *pid* of 0
   means the calling process. The result is one of the scheduling policy
   constants above.


.. function:: sched_setparam(pid, param)

   Set a scheduling parameters for the process with PID *pid*. A *pid* of 0 means
   the calling process. *param* is a :class:`sched_param` instance.


.. function:: sched_getparam(pid)

   Return the scheduling parameters as a :class:`sched_param` instance for the
   process with PID *pid*. A *pid* of 0 means the calling process.


.. function:: sched_rr_get_interval(pid)

   Return the round-robin quantum in seconds for the process with PID *pid*. A
   *pid* of 0 means the calling process.


.. function:: sched_yield()

   Voluntarily relinquish the CPU.


.. class:: cpu_set(ncpus)

   :class:`cpu_set` represents a set of CPUs on which a process is eligible to
   run. *ncpus* is the number of CPUs the set should describe. Methods on
   :class:`cpu_set` allow CPUs to be add or removed.

   :class:`cpu_set` supports the AND, OR, and XOR bitwise operations. For
   example, given two cpu_sets, ``one`` and ``two``, ``one | two`` returns a
   :class:`cpu_set` containing the cpus enabled both in ``one`` and ``two``.

   .. method:: set(i)

      Enable CPU *i*.

   .. method:: clear(i)

      Remove CPU *i*.

   .. method:: isset(i)

      Return ``True`` if CPU *i* is enabled in the set.

   .. method:: count()

      Return the number of enabled CPUs in the set.

   .. method:: zero()

      Clear the set completely.


.. function:: sched_setaffinity(pid, mask)

   Restrict the process with PID *pid* to a set of CPUs. *mask* is a
   :class:`cpu_set` instance.


.. function:: sched_getaffinity(pid, size)

   Return the :class:`cpu_set` the process with PID *pid* is restricted to. The
   result will contain *size* CPUs.


.. _os-path:

Miscellaneous System Information
--------------------------------


.. function:: confstr(name)

   Return string-valued system configuration values. *name* specifies the
   configuration value to retrieve; it may be a string which is the name of a
   defined system value; these names are specified in a number of standards (POSIX,
   Unix 95, Unix 98, and others).  Some platforms define additional names as well.
   The names known to the host operating system are given as the keys of the
   ``confstr_names`` dictionary.  For configuration variables not included in that
   mapping, passing an integer for *name* is also accepted.

   If the configuration value specified by *name* isn't defined, ``None`` is
   returned.

   If *name* is a string and is not known, :exc:`ValueError` is raised.  If a
   specific value for *name* is not supported by the host system, even if it is
   included in ``confstr_names``, an :exc:`OSError` is raised with
   :const:`errno.EINVAL` for the error number.

   Availability: Unix


.. data:: confstr_names

   Dictionary mapping names accepted by :func:`confstr` to the integer values
   defined for those names by the host operating system. This can be used to
   determine the set of names known to the system.

   Availability: Unix.


.. function:: getloadavg()

   Return the number of processes in the system run queue averaged over the last
   1, 5, and 15 minutes or raises :exc:`OSError` if the load average was
   unobtainable.

   Availability: Unix.


.. function:: sysconf(name)

   Return integer-valued system configuration values. If the configuration value
   specified by *name* isn't defined, ``-1`` is returned.  The comments regarding
   the *name* parameter for :func:`confstr` apply here as well; the dictionary that
   provides information on the known names is given by ``sysconf_names``.

   Availability: Unix.


.. data:: sysconf_names

   Dictionary mapping names accepted by :func:`sysconf` to the integer values
   defined for those names by the host operating system. This can be used to
   determine the set of names known to the system.

   Availability: Unix.

The following data values are used to support path manipulation operations.  These
are defined for all platforms.

Higher-level operations on pathnames are defined in the :mod:`os.path` module.


.. data:: curdir

   The constant string used by the operating system to refer to the current
   directory. This is ``'.'`` for Windows and POSIX. Also available via
   :mod:`os.path`.


.. data:: pardir

   The constant string used by the operating system to refer to the parent
   directory. This is ``'..'`` for Windows and POSIX. Also available via
   :mod:`os.path`.


.. data:: sep

   The character used by the operating system to separate pathname components.
   This is ``'/'`` for POSIX and ``'\\'`` for Windows.  Note that knowing this
   is not sufficient to be able to parse or concatenate pathnames --- use
   :func:`os.path.split` and :func:`os.path.join` --- but it is occasionally
   useful. Also available via :mod:`os.path`.


.. data:: altsep

   An alternative character used by the operating system to separate pathname
   components, or ``None`` if only one separator character exists.  This is set to
   ``'/'`` on Windows systems where ``sep`` is a backslash. Also available via
   :mod:`os.path`.


.. data:: extsep

   The character which separates the base filename from the extension; for example,
   the ``'.'`` in :file:`os.py`. Also available via :mod:`os.path`.


.. data:: pathsep

   The character conventionally used by the operating system to separate search
   path components (as in :envvar:`PATH`), such as ``':'`` for POSIX or ``';'`` for
   Windows. Also available via :mod:`os.path`.


.. data:: defpath

   The default search path used by :func:`exec\*p\*` and :func:`spawn\*p\*` if the
   environment doesn't have a ``'PATH'`` key. Also available via :mod:`os.path`.


.. data:: linesep

   The string used to separate (or, rather, terminate) lines on the current
   platform.  This may be a single character, such as ``'\n'`` for POSIX, or
   multiple characters, for example, ``'\r\n'`` for Windows. Do not use
   *os.linesep* as a line terminator when writing files opened in text mode (the
   default); use a single ``'\n'`` instead, on all platforms.


.. data:: devnull

   The file path of the null device. For example: ``'/dev/null'`` for
   POSIX, ``'nul'`` for Windows.  Also available via :mod:`os.path`.


.. _os-miscfunc:

Miscellaneous Functions
-----------------------


.. function:: urandom(n)

   Return a string of *n* random bytes suitable for cryptographic use.

   This function returns random bytes from an OS-specific randomness source.  The
   returned data should be unpredictable enough for cryptographic applications,
   though its exact quality depends on the OS implementation.  On a UNIX-like
   system this will query /dev/urandom, and on Windows it will use CryptGenRandom.
   If a randomness source is not found, :exc:`NotImplementedError` will be raised.
