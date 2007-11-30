
:mod:`os` --- Miscellaneous operating system interfaces
=======================================================

.. module:: os
   :synopsis: Miscellaneous operating system interfaces.


This module provides a more portable way of using operating system dependent
functionality than importing a operating system dependent built-in module like
:mod:`posix` or :mod:`nt`. If you just want to read or write a file see
:func:`open`, if you want to manipulate paths, see the :mod:`os.path`
module, and if you want to read all the lines in all the files on the
command line see the :mod:`fileinput` module. For creating temporary
files and directories see the :mod:`tempfile` module, and for high-level
file and directory handling see the :mod:`shutil` module.

This module searches for an operating system dependent built-in module like
:mod:`mac` or :mod:`posix` and exports the same functions and data as found
there.  The design of all Python's built-in operating system dependent modules
is such that as long as the same functionality is available, it uses the same
interface; for example, the function ``os.stat(path)`` returns stat information
about *path* in the same format (which happens to have originated with the POSIX
interface).

Extensions peculiar to a particular operating system are also available through
the :mod:`os` module, but using them is of course a threat to portability!

Note that after the first time :mod:`os` is imported, there is *no* performance
penalty in using functions from :mod:`os` instead of directly from the operating
system dependent built-in module, so there should be *no* reason not to use
:mod:`os`!

The :mod:`os` module contains many functions and data values. The items below
and in the following sub-sections are all available directly from the :mod:`os`
module.

.. % Frank Stajano <fstajano@uk.research.att.com> complained that it
.. % wasn't clear that the entries described in the subsections were all
.. % available at the module level (most uses of subsections are
.. % different); I think this is only a problem for the HTML version,
.. % where the relationship may not be as clear.
.. % 


.. exception:: error

   .. index:: module: errno

   This exception is raised when a function returns a system-related error (not for
   illegal argument types or other incidental errors). This is also known as the
   built-in exception :exc:`OSError`.  The accompanying value is a pair containing
   the numeric error code from :cdata:`errno` and the corresponding string, as
   would be printed by the C function :cfunc:`perror`.  See the module
   :mod:`errno`, which contains names for the error codes defined by the underlying
   operating system.

   When exceptions are classes, this exception carries two attributes,
   :attr:`errno` and :attr:`strerror`.  The first holds the value of the C
   :cdata:`errno` variable, and the latter holds the corresponding error message
   from :cfunc:`strerror`.  For exceptions that involve a file system path (such as
   :func:`chdir` or :func:`unlink`), the exception instance will contain a third
   attribute, :attr:`filename`, which is the file name passed to the function.


.. data:: name

   The name of the operating system dependent module imported.  The following names
   have currently been registered: ``'posix'``, ``'nt'``, ``'mac'``, ``'os2'``,
   ``'ce'``, ``'java'``, ``'riscos'``.


.. data:: path

   The corresponding operating system dependent standard module for pathname
   operations, such as :mod:`posixpath` or :mod:`macpath`.  Thus, given the proper
   imports, ``os.path.split(file)`` is equivalent to but more portable than
   ``posixpath.split(file)``.  Note that this is also an importable module: it may
   be imported directly as :mod:`os.path`.


.. _os-procinfo:

Process Parameters
------------------

These functions and data items provide information and operate on the current
process and user.


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

   .. note::

      Calling :func:`putenv` directly does not change ``os.environ``, so it's better
      to modify ``os.environ``.

   .. note::

      On some platforms, including FreeBSD and Mac OS X, setting ``environ`` may cause
      memory leaks.  Refer to the system documentation for :cfunc:`putenv`.

   If :func:`putenv` is not provided, a modified copy of this mapping  may be
   passed to the appropriate process-creation functions to cause  child processes
   to use a modified environment.

   If the platform supports the :func:`unsetenv` function, you can delete items in
   this mapping to unset environment variables. :func:`unsetenv` will be called
   automatically when an item is deleted from ``os.environ``, and when
   one of the :meth:`pop` or :meth:`clear` methods is called.

   .. versionchanged:: 2.6
      Also unset environment variables when calling :meth:`os.environ.clear`
      and :meth:`os.environ.pop`.


.. function:: chdir(path)
              fchdir(fd)
              getcwd()
   :noindex:

   These functions are described in :ref:`os-file-dir`.


.. function:: ctermid()

   Return the filename corresponding to the controlling terminal of the process.
   Availability: Unix.


.. function:: getegid()

   Return the effective group id of the current process.  This corresponds to the
   'set id' bit on the file being executed in the current process. Availability:
   Unix.


.. function:: geteuid()

   .. index:: single: user; effective id

   Return the current process' effective user id. Availability: Unix.


.. function:: getgid()

   .. index:: single: process; group

   Return the real group id of the current process. Availability: Unix.


.. function:: getgroups()

   Return list of supplemental group ids associated with the current process.
   Availability: Unix.


.. function:: getlogin()

   Return the name of the user logged in on the controlling terminal of the
   process.  For most purposes, it is more useful to use the environment variable
   :envvar:`LOGNAME` to find out who the user is, or
   ``pwd.getpwuid(os.getuid())[0]`` to get the login name of the currently
   effective user ID. Availability: Unix.


.. function:: getpgid(pid)

   Return the process group id of the process with process id *pid*. If *pid* is 0,
   the process group id of the current process is returned. Availability: Unix.

   .. versionadded:: 2.3


.. function:: getpgrp()

   .. index:: single: process; group

   Return the id of the current process group. Availability: Unix.


.. function:: getpid()

   .. index:: single: process; id

   Return the current process id. Availability: Unix, Windows.


.. function:: getppid()

   .. index:: single: process; id of parent

   Return the parent's process id. Availability: Unix.


.. function:: getuid()

   .. index:: single: user; id

   Return the current process' user id. Availability: Unix.


.. function:: getenv(varname[, value])

   Return the value of the environment variable *varname* if it exists, or *value*
   if it doesn't.  *value* defaults to ``None``. Availability: most flavors of
   Unix, Windows.


.. function:: putenv(varname, value)

   .. index:: single: environment variables; setting

   Set the environment variable named *varname* to the string *value*.  Such
   changes to the environment affect subprocesses started with :func:`os.system`,
   :func:`popen` or :func:`fork` and :func:`execv`. Availability: most flavors of
   Unix, Windows.

   .. note::

      On some platforms, including FreeBSD and Mac OS X, setting ``environ`` may cause
      memory leaks. Refer to the system documentation for putenv.

   When :func:`putenv` is supported, assignments to items in ``os.environ`` are
   automatically translated into corresponding calls to :func:`putenv`; however,
   calls to :func:`putenv` don't update ``os.environ``, so it is actually
   preferable to assign to items of ``os.environ``.


.. function:: setegid(egid)

   Set the current process's effective group id. Availability: Unix.


.. function:: seteuid(euid)

   Set the current process's effective user id. Availability: Unix.


.. function:: setgid(gid)

   Set the current process' group id. Availability: Unix.


.. function:: setgroups(groups)

   Set the list of supplemental group ids associated with the current process to
   *groups*. *groups* must be a sequence, and each element must be an integer
   identifying a group. This operation is typical available only to the superuser.
   Availability: Unix.

   .. versionadded:: 2.2


.. function:: setpgrp()

   Calls the system call :cfunc:`setpgrp` or :cfunc:`setpgrp(0, 0)` depending on
   which version is implemented (if any).  See the Unix manual for the semantics.
   Availability: Unix.


.. function:: setpgid(pid, pgrp)

   Calls the system call :cfunc:`setpgid` to set the process group id of the
   process with id *pid* to the process group with id *pgrp*.  See the Unix manual
   for the semantics. Availability: Unix.


.. function:: setreuid(ruid, euid)

   Set the current process's real and effective user ids. Availability: Unix.


.. function:: setregid(rgid, egid)

   Set the current process's real and effective group ids. Availability: Unix.


.. function:: getsid(pid)

   Calls the system call :cfunc:`getsid`.  See the Unix manual for the semantics.
   Availability: Unix.

   .. versionadded:: 2.4


.. function:: setsid()

   Calls the system call :cfunc:`setsid`.  See the Unix manual for the semantics.
   Availability: Unix.


.. function:: setuid(uid)

   .. index:: single: user; id, setting

   Set the current process' user id. Availability: Unix.

.. % placed in this section since it relates to errno.... a little weak


.. function:: strerror(code)

   Return the error message corresponding to the error code in *code*.
   Availability: Unix, Windows.


.. function:: umask(mask)

   Set the current numeric umask and returns the previous umask. Availability:
   Unix, Windows.


.. function:: uname()

   .. index::
      single: gethostname() (in module socket)
      single: gethostbyaddr() (in module socket)

   Return a 5-tuple containing information identifying the current operating
   system.  The tuple contains 5 strings: ``(sysname, nodename, release, version,
   machine)``.  Some systems truncate the nodename to 8 characters or to the
   leading component; a better way to get the hostname is
   :func:`socket.gethostname`  or even
   ``socket.gethostbyaddr(socket.gethostname())``. Availability: recent flavors of
   Unix.


.. function:: unsetenv(varname)

   .. index:: single: environment variables; deleting

   Unset (delete) the environment variable named *varname*. Such changes to the
   environment affect subprocesses started with :func:`os.system`, :func:`popen` or
   :func:`fork` and :func:`execv`. Availability: most flavors of Unix, Windows.

   When :func:`unsetenv` is supported, deletion of items in ``os.environ`` is
   automatically translated into a corresponding call to :func:`unsetenv`; however,
   calls to :func:`unsetenv` don't update ``os.environ``, so it is actually
   preferable to delete items of ``os.environ``.


.. _os-newstreams:

File Object Creation
--------------------

These functions create new file objects. (See also :func:`open`.)


.. function:: fdopen(fd[, mode[, bufsize]])

   .. index:: single: I/O control; buffering

   Return an open file object connected to the file descriptor *fd*.  The *mode*
   and *bufsize* arguments have the same meaning as the corresponding arguments to
   the built-in :func:`open` function. Availability: Macintosh, Unix, Windows.

   .. versionchanged:: 2.3
      When specified, the *mode* argument must now start with one of the letters
      ``'r'``, ``'w'``, or ``'a'``, otherwise a :exc:`ValueError` is raised.

   .. versionchanged:: 2.5
      On Unix, when the *mode* argument starts with ``'a'``, the *O_APPEND* flag is
      set on the file descriptor (which the :cfunc:`fdopen` implementation already
      does on most platforms).


.. function:: popen(command[, mode[, bufsize]])

   Open a pipe to or from *command*.  The return value is an open file object
   connected to the pipe, which can be read or written depending on whether *mode*
   is ``'r'`` (default) or ``'w'``. The *bufsize* argument has the same meaning as
   the corresponding argument to the built-in :func:`open` function.  The exit
   status of the command (encoded in the format specified for :func:`wait`) is
   available as the return value of the :meth:`close` method of the file object,
   except that when the exit status is zero (termination without errors), ``None``
   is returned. Availability: Macintosh, Unix, Windows.

   .. deprecated:: 2.6
      This function is obsolete.  Use the :mod:`subprocess` module.

   .. versionchanged:: 2.0
      This function worked unreliably under Windows in earlier versions of Python.
      This was due to the use of the :cfunc:`_popen` function from the libraries
      provided with Windows.  Newer versions of Python do not use the broken
      implementation from the Windows libraries.


.. function:: tmpfile()

   Return a new file object opened in update mode (``w+b``).  The file has no
   directory entries associated with it and will be automatically deleted once
   there are no file descriptors for the file. Availability: Macintosh, Unix,
   Windows.

There are a number of different :func:`popen\*` functions that provide slightly
different ways to create subprocesses.

.. deprecated:: 2.6
   All of the :func:`popen\*` functions are obsolete. Use the :mod:`subprocess`
   module.

For each of the :func:`popen\*` variants, if *bufsize* is specified, it
specifies the buffer size for the I/O pipes. *mode*, if provided, should be the
string ``'b'`` or ``'t'``; on Windows this is needed to determine whether the
file objects should be opened in binary or text mode.  The default value for
*mode* is ``'t'``.

Also, for each of these variants, on Unix, *cmd* may be a sequence, in which
case arguments will be passed directly to the program without shell intervention
(as with :func:`os.spawnv`). If *cmd* is a string it will be passed to the shell
(as with :func:`os.system`).

These methods do not make it possible to retrieve the exit status from the child
processes.  The only way to control the input and output streams and also
retrieve the return codes is to use the :mod:`subprocess` module; these are only
available on Unix.

For a discussion of possible deadlock conditions related to the use of these
functions, see :ref:`popen2-flow-control`.


.. function:: popen2(cmd[, mode[, bufsize]])

   Executes *cmd* as a sub-process.  Returns the file objects ``(child_stdin,
   child_stdout)``.

   .. deprecated:: 2.6
      All of the :func:`popen\*` functions are obsolete. Use the :mod:`subprocess`
      module.

   Availability: Macintosh, Unix, Windows.

   .. versionadded:: 2.0


.. function:: popen3(cmd[, mode[, bufsize]])

   Executes *cmd* as a sub-process.  Returns the file objects ``(child_stdin,
   child_stdout, child_stderr)``.

   .. deprecated:: 2.6
      All of the :func:`popen\*` functions are obsolete. Use the :mod:`subprocess`
      module.

   Availability: Macintosh, Unix, Windows.

   .. versionadded:: 2.0


.. function:: popen4(cmd[, mode[, bufsize]])

   Executes *cmd* as a sub-process.  Returns the file objects ``(child_stdin,
   child_stdout_and_stderr)``.

   .. deprecated:: 2.6
      All of the :func:`popen\*` functions are obsolete. Use the :mod:`subprocess`
      module.

   Availability: Macintosh, Unix, Windows.

   .. versionadded:: 2.0

(Note that ``child_stdin, child_stdout, and child_stderr`` are named from the
point of view of the child process, so *child_stdin* is the child's standard
input.)

This functionality is also available in the :mod:`popen2` module using functions
of the same names, but the return values of those functions have a different
order.


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


.. function:: close(fd)

   Close file descriptor *fd*. Availability: Macintosh, Unix, Windows.

   .. note::

      This function is intended for low-level I/O and must be applied to a file
      descriptor as returned by :func:`open` or :func:`pipe`.  To close a "file
      object" returned by the built-in function :func:`open` or by :func:`popen` or
      :func:`fdopen`, use its :meth:`close` method.


.. function:: dup(fd)

   Return a duplicate of file descriptor *fd*. Availability: Macintosh, Unix,
   Windows.


.. function:: dup2(fd, fd2)

   Duplicate file descriptor *fd* to *fd2*, closing the latter first if necessary.
   Availability: Macintosh, Unix, Windows.


.. function:: fchmod(fd, mode)

   Change the mode of the file given by *fd* to the numeric *mode*.  See the docs
   for :func:`chmod` for possible values of *mode*.  Availability: Unix.


.. function:: fchown(fd, uid, gid)

   Change the owner and group id of the file given by *fd* to the numeric *uid*
   and *gid*.  To leave one of the ids unchanged, set it to -1.
   Availability: Unix.


.. function:: fdatasync(fd)

   Force write of file with filedescriptor *fd* to disk. Does not force update of
   metadata. Availability: Unix.


.. function:: fpathconf(fd, name)

   Return system configuration information relevant to an open file. *name*
   specifies the configuration value to retrieve; it may be a string which is the
   name of a defined system value; these names are specified in a number of
   standards (POSIX.1, Unix 95, Unix 98, and others).  Some platforms define
   additional names as well.  The names known to the host operating system are
   given in the ``pathconf_names`` dictionary.  For configuration variables not
   included in that mapping, passing an integer for *name* is also accepted.
   Availability: Macintosh, Unix.

   If *name* is a string and is not known, :exc:`ValueError` is raised.  If a
   specific value for *name* is not supported by the host system, even if it is
   included in ``pathconf_names``, an :exc:`OSError` is raised with
   :const:`errno.EINVAL` for the error number.


.. function:: fstat(fd)

   Return status for file descriptor *fd*, like :func:`stat`. Availability:
   Macintosh, Unix, Windows.


.. function:: fstatvfs(fd)

   Return information about the filesystem containing the file associated with file
   descriptor *fd*, like :func:`statvfs`. Availability: Unix.


.. function:: fsync(fd)

   Force write of file with filedescriptor *fd* to disk.  On Unix, this calls the
   native :cfunc:`fsync` function; on Windows, the MS :cfunc:`_commit` function.

   If you're starting with a Python file object *f*, first do ``f.flush()``, and
   then do ``os.fsync(f.fileno())``, to ensure that all internal buffers associated
   with *f* are written to disk. Availability: Macintosh, Unix, and Windows
   starting in 2.2.3.


.. function:: ftruncate(fd, length)

   Truncate the file corresponding to file descriptor *fd*, so that it is at most
   *length* bytes in size. Availability: Macintosh, Unix.


.. function:: isatty(fd)

   Return ``True`` if the file descriptor *fd* is open and connected to a
   tty(-like) device, else ``False``. Availability: Macintosh, Unix.


.. function:: lchmod(path, mode)

   Change the mode of *path* to the numeric *mode*. If path is a symlink, this
   affects the symlink rather than the target. See the docs for :func:`chmod`
   for possible values of *mode*.  Availability: Unix.


.. function:: lseek(fd, pos, how)

   Set the current position of file descriptor *fd* to position *pos*, modified by
   *how*: ``0`` to set the position relative to the beginning of the file; ``1`` to
   set it relative to the current position; ``2`` to set it relative to the end of
   the file. Availability: Macintosh, Unix, Windows.


.. function:: open(file, flags[, mode])

   Open the file *file* and set various flags according to *flags* and possibly its
   mode according to *mode*. The default *mode* is ``0777`` (octal), and the
   current umask value is first masked out.  Return the file descriptor for the
   newly opened file. Availability: Macintosh, Unix, Windows.

   For a description of the flag and mode values, see the C run-time documentation;
   flag constants (like :const:`O_RDONLY` and :const:`O_WRONLY`) are defined in
   this module too (see below).

   .. note::

      This function is intended for low-level I/O.  For normal usage, use the built-in
      function :func:`open`, which returns a "file object" with :meth:`read` and
      :meth:`write` methods (and many more).  To wrap a file descriptor in a "file
      object", use :func:`fdopen`.


.. function:: openpty()

   .. index:: module: pty

   Open a new pseudo-terminal pair. Return a pair of file descriptors ``(master,
   slave)`` for the pty and the tty, respectively. For a (slightly) more portable
   approach, use the :mod:`pty` module. Availability: Macintosh, Some flavors of
   Unix.


.. function:: pipe()

   Create a pipe.  Return a pair of file descriptors ``(r, w)`` usable for reading
   and writing, respectively. Availability: Macintosh, Unix, Windows.


.. function:: read(fd, n)

   Read at most *n* bytes from file descriptor *fd*. Return a string containing the
   bytes read.  If the end of the file referred to by *fd* has been reached, an
   empty string is returned. Availability: Macintosh, Unix, Windows.

   .. note::

      This function is intended for low-level I/O and must be applied to a file
      descriptor as returned by :func:`open` or :func:`pipe`.  To read a "file object"
      returned by the built-in function :func:`open` or by :func:`popen` or
      :func:`fdopen`, or ``sys.stdin``, use its :meth:`read` or :meth:`readline`
      methods.


.. function:: tcgetpgrp(fd)

   Return the process group associated with the terminal given by *fd* (an open
   file descriptor as returned by :func:`open`). Availability: Macintosh, Unix.


.. function:: tcsetpgrp(fd, pg)

   Set the process group associated with the terminal given by *fd* (an open file
   descriptor as returned by :func:`open`) to *pg*. Availability: Macintosh, Unix.


.. function:: ttyname(fd)

   Return a string which specifies the terminal device associated with
   file descriptor *fd*.  If *fd* is not associated with a terminal device, an
   exception is raised. Availability:Macintosh, Unix.


.. function:: write(fd, str)

   Write the string *str* to file descriptor *fd*. Return the number of bytes
   actually written. Availability: Macintosh, Unix, Windows.

   .. note::

      This function is intended for low-level I/O and must be applied to a file
      descriptor as returned by :func:`open` or :func:`pipe`.  To write a "file
      object" returned by the built-in function :func:`open` or by :func:`popen` or
      :func:`fdopen`, or ``sys.stdout`` or ``sys.stderr``, use its :meth:`write`
      method.

The following data items are available for use in constructing the *flags*
parameter to the :func:`open` function.  Some items will not be available on all
platforms.  For descriptions of their availability and use, consult
:manpage:`open(2)`.


.. data:: O_RDONLY
          O_WRONLY
          O_RDWR
          O_APPEND
          O_CREAT
          O_EXCL
          O_TRUNC

   Options for the *flag* argument to the :func:`open` function. These can be
   bit-wise OR'd together. Availability: Macintosh, Unix, Windows.


.. data:: O_DSYNC
          O_RSYNC
          O_SYNC
          O_NDELAY
          O_NONBLOCK
          O_NOCTTY
          O_SHLOCK
          O_EXLOCK

   More options for the *flag* argument to the :func:`open` function. Availability:
   Macintosh, Unix.


.. data:: O_BINARY
          O_NOINHERIT
          O_SHORT_LIVED
          O_TEMPORARY
          O_RANDOM
          O_SEQUENTIAL
          O_TEXT

   Options for the *flag* argument to the :func:`open` function. These can be
   bit-wise OR'd together. Availability: Windows.


.. data:: O_DIRECT
          O_DIRECTORY
          O_NOFOLLOW
          O_NOATIME

   Options for the *flag* argument to the :func:`open` function. These are
   GNU extensions and not present if they are not defined by the C library.


.. data:: SEEK_SET
          SEEK_CUR
          SEEK_END

   Parameters to the :func:`lseek` function. Their values are 0, 1, and 2,
   respectively. Availability: Windows, Macintosh, Unix.

   .. versionadded:: 2.5


.. _os-file-dir:

Files and Directories
---------------------


.. function:: access(path, mode)

   Use the real uid/gid to test for access to *path*.  Note that most operations
   will use the effective uid/gid, therefore this routine can be used in a
   suid/sgid environment to test if the invoking user has the specified access to
   *path*.  *mode* should be :const:`F_OK` to test the existence of *path*, or it
   can be the inclusive OR of one or more of :const:`R_OK`, :const:`W_OK`, and
   :const:`X_OK` to test permissions.  Return :const:`True` if access is allowed,
   :const:`False` if not. See the Unix man page :manpage:`access(2)` for more
   information. Availability: Macintosh, Unix, Windows.

   .. note::

      Using :func:`access` to check if a user is authorized to e.g. open a file before
      actually doing so using :func:`open` creates a  security hole, because the user
      might exploit the short time interval  between checking and opening the file to
      manipulate it.

   .. note::

      I/O operations may fail even when :func:`access` indicates that they would
      succeed, particularly for operations on network filesystems which may have
      permissions semantics beyond the usual POSIX permission-bit model.


.. data:: F_OK

   Value to pass as the *mode* parameter of :func:`access` to test the existence of
   *path*.


.. data:: R_OK

   Value to include in the *mode* parameter of :func:`access` to test the
   readability of *path*.


.. data:: W_OK

   Value to include in the *mode* parameter of :func:`access` to test the
   writability of *path*.


.. data:: X_OK

   Value to include in the *mode* parameter of :func:`access` to determine if
   *path* can be executed.


.. function:: chdir(path)

   .. index:: single: directory; changing

   Change the current working directory to *path*. Availability: Macintosh, Unix,
   Windows.


.. function:: fchdir(fd)

   Change the current working directory to the directory represented by the file
   descriptor *fd*.  The descriptor must refer to an opened directory, not an open
   file. Availability: Unix.

   .. versionadded:: 2.3


.. function:: getcwd()

   Return a string representing the current working directory. Availability:
   Macintosh, Unix, Windows.


.. function:: getcwdu()

   Return a Unicode object representing the current working directory.
   Availability: Macintosh, Unix, Windows.

   .. versionadded:: 2.3


.. function:: chflags(path, flags)

   Set the flags of *path* to the numeric *flags*. *flags* may take a combination
   (bitwise OR) of the following values (as defined in the :mod:`stat` module):

   * ``UF_NODUMP``
   * ``UF_IMMUTABLE``
   * ``UF_APPEND``
   * ``UF_OPAQUE``
   * ``UF_NOUNLINK``
   * ``SF_ARCHIVED``
   * ``SF_IMMUTABLE``
   * ``SF_APPEND``
   * ``SF_NOUNLINK``
   * ``SF_SNAPSHOT``

   Availability: Macintosh, Unix.

   .. versionadded:: 2.6


.. function:: chroot(path)

   Change the root directory of the current process to *path*. Availability:
   Macintosh, Unix.

   .. versionadded:: 2.2


.. function:: chmod(path, mode)

   Change the mode of *path* to the numeric *mode*. *mode* may take one of the
   following values (as defined in the :mod:`stat` module) or bitwise or-ed
   combinations of them:


   * ``stat.S_ISUID``
   * ``stat.S_ISGID``
   * ``stat.S_ENFMT``
   * ``stat.S_ISVTX``
   * ``stat.S_IREAD``
   * ``stat.S_IWRITE``
   * ``stat.S_IEXEC``
   * ``stat.S_IRWXU``
   * ``stat.S_IRUSR``
   * ``stat.S_IWUSR``
   * ``stat.S_IXUSR``
   * ``stat.S_IRWXG``
   * ``stat.S_IRGRP``
   * ``stat.S_IWGRP``
   * ``stat.S_IXGRP``
   * ``stat.S_IRWXO``
   * ``stat.S_IROTH``
   * ``stat.S_IWOTH``
   * ``stat.S_IXOTH``

   Availability: Macintosh, Unix, Windows.

   .. note::

      Although Windows supports :func:`chmod`, you can only  set the file's read-only
      flag with it (via the ``stat.S_IWRITE``  and ``stat.S_IREAD``
      constants or a corresponding integer value).  All other bits are
      ignored.


.. function:: chown(path, uid, gid)

   Change the owner and group id of *path* to the numeric *uid* and *gid*. To leave
   one of the ids unchanged, set it to -1. Availability: Macintosh, Unix.


.. function:: lchflags(path, flags)

   Set the flags of *path* to the numeric *flags*, like :func:`chflags`, but do not
   follow symbolic links. Availability: Unix.

   .. versionadded:: 2.6


.. function:: lchown(path, uid, gid)

   Change the owner and group id of *path* to the numeric *uid* and gid. This
   function will not follow symbolic links. Availability: Macintosh, Unix.

   .. versionadded:: 2.3


.. function:: link(src, dst)

   Create a hard link pointing to *src* named *dst*. Availability: Macintosh, Unix.


.. function:: listdir(path)

   Return a list containing the names of the entries in the directory. The list is
   in arbitrary order.  It does not include the special entries ``'.'`` and
   ``'..'`` even if they are present in the directory. Availability: Macintosh,
   Unix, Windows.

   .. versionchanged:: 2.3
      On Windows NT/2k/XP and Unix, if *path* is a Unicode object, the result will be
      a list of Unicode objects.


.. function:: lstat(path)

   Like :func:`stat`, but do not follow symbolic links.  This is an alias for
   :func:`stat` on platforms that do not support symbolic links, such as
   Windows.


.. function:: mkfifo(path[, mode])

   Create a FIFO (a named pipe) named *path* with numeric mode *mode*.  The default
   *mode* is ``0666`` (octal).  The current umask value is first masked out from
   the mode. Availability: Macintosh, Unix.

   FIFOs are pipes that can be accessed like regular files.  FIFOs exist until they
   are deleted (for example with :func:`os.unlink`). Generally, FIFOs are used as
   rendezvous between "client" and "server" type processes: the server opens the
   FIFO for reading, and the client opens it for writing.  Note that :func:`mkfifo`
   doesn't open the FIFO --- it just creates the rendezvous point.


.. function:: mknod(filename[, mode=0600, device])

   Create a filesystem node (file, device special file or named pipe) named
   *filename*. *mode* specifies both the permissions to use and the type of node to
   be created, being combined (bitwise OR) with one of ``stat.S_IFREG``,
   ``stat.S_IFCHR``, ``stat.S_IFBLK``,
   and ``stat.S_IFIFO`` (those constants are available in :mod:`stat`).
   For ``stat.S_IFCHR`` and
   ``stat.S_IFBLK``, *device* defines the newly created device special file (probably using
   :func:`os.makedev`), otherwise it is ignored.

   .. versionadded:: 2.3


.. function:: major(device)

   Extracts the device major number from a raw device number (usually the
   :attr:`st_dev` or :attr:`st_rdev` field from :ctype:`stat`).

   .. versionadded:: 2.3


.. function:: minor(device)

   Extracts the device minor number from a raw device number (usually the
   :attr:`st_dev` or :attr:`st_rdev` field from :ctype:`stat`).

   .. versionadded:: 2.3


.. function:: makedev(major, minor)

   Composes a raw device number from the major and minor device numbers.

   .. versionadded:: 2.3


.. function:: mkdir(path[, mode])

   Create a directory named *path* with numeric mode *mode*. The default *mode* is
   ``0777`` (octal).  On some systems, *mode* is ignored.  Where it is used, the
   current umask value is first masked out. Availability: Macintosh, Unix, Windows.

   It is also possible to create temporary directories; see the
   :mod:`tempfile` module's :func:`tempfile.mkdtemp` function.


.. function:: makedirs(path[, mode])

   .. index::
      single: directory; creating
      single: UNC paths; and os.makedirs()

   Recursive directory creation function.  Like :func:`mkdir`, but makes all
   intermediate-level directories needed to contain the leaf directory.  Throws an
   :exc:`error` exception if the leaf directory already exists or cannot be
   created.  The default *mode* is ``0777`` (octal).  On some systems, *mode* is
   ignored. Where it is used, the current umask value is first masked out.

   .. note::

      :func:`makedirs` will become confused if the path elements to create include
      *os.pardir*.

   .. versionadded:: 1.5.2

   .. versionchanged:: 2.3
      This function now handles UNC paths correctly.


.. function:: pathconf(path, name)

   Return system configuration information relevant to a named file. *name*
   specifies the configuration value to retrieve; it may be a string which is the
   name of a defined system value; these names are specified in a number of
   standards (POSIX.1, Unix 95, Unix 98, and others).  Some platforms define
   additional names as well.  The names known to the host operating system are
   given in the ``pathconf_names`` dictionary.  For configuration variables not
   included in that mapping, passing an integer for *name* is also accepted.
   Availability: Macintosh, Unix.

   If *name* is a string and is not known, :exc:`ValueError` is raised.  If a
   specific value for *name* is not supported by the host system, even if it is
   included in ``pathconf_names``, an :exc:`OSError` is raised with
   :const:`errno.EINVAL` for the error number.


.. data:: pathconf_names

   Dictionary mapping names accepted by :func:`pathconf` and :func:`fpathconf` to
   the integer values defined for those names by the host operating system.  This
   can be used to determine the set of names known to the system. Availability:
   Macintosh, Unix.


.. function:: readlink(path)

   Return a string representing the path to which the symbolic link points.  The
   result may be either an absolute or relative pathname; if it is relative, it may
   be converted to an absolute pathname using ``os.path.join(os.path.dirname(path),
   result)``.

   .. versionchanged:: 2.6
      If the *path* is a Unicode object the result will also be a Unicode object.

   Availability: Macintosh, Unix.


.. function:: remove(path)

   Remove the file *path*.  If *path* is a directory, :exc:`OSError` is raised; see
   :func:`rmdir` below to remove a directory.  This is identical to the
   :func:`unlink` function documented below.  On Windows, attempting to remove a
   file that is in use causes an exception to be raised; on Unix, the directory
   entry is removed but the storage allocated to the file is not made available
   until the original file is no longer in use. Availability: Macintosh, Unix,
   Windows.


.. function:: removedirs(path)

   .. index:: single: directory; deleting

   Removes directories recursively.  Works like :func:`rmdir` except that, if the
   leaf directory is successfully removed, :func:`removedirs`  tries to
   successively remove every parent directory mentioned in  *path* until an error
   is raised (which is ignored, because it generally means that a parent directory
   is not empty). For example, ``os.removedirs('foo/bar/baz')`` will first remove
   the directory ``'foo/bar/baz'``, and then remove ``'foo/bar'`` and ``'foo'`` if
   they are empty. Raises :exc:`OSError` if the leaf directory could not be
   successfully removed.

   .. versionadded:: 1.5.2


.. function:: rename(src, dst)

   Rename the file or directory *src* to *dst*.  If *dst* is a directory,
   :exc:`OSError` will be raised.  On Unix, if *dst* exists and is a file, it will
   be removed silently if the user has permission.  The operation may fail on some
   Unix flavors if *src* and *dst* are on different filesystems.  If successful,
   the renaming will be an atomic operation (this is a POSIX requirement).  On
   Windows, if *dst* already exists, :exc:`OSError` will be raised even if it is a
   file; there may be no way to implement an atomic rename when *dst* names an
   existing file. Availability: Macintosh, Unix, Windows.


.. function:: renames(old, new)

   Recursive directory or file renaming function. Works like :func:`rename`, except
   creation of any intermediate directories needed to make the new pathname good is
   attempted first. After the rename, directories corresponding to rightmost path
   segments of the old name will be pruned away using :func:`removedirs`.

   .. versionadded:: 1.5.2

   .. note::

      This function can fail with the new directory structure made if you lack
      permissions needed to remove the leaf directory or file.


.. function:: rmdir(path)

   Remove the directory *path*. Availability: Macintosh, Unix, Windows.


.. function:: stat(path)

   Perform a :cfunc:`stat` system call on the given path.  The return value is an
   object whose attributes correspond to the members of the :ctype:`stat`
   structure, namely: :attr:`st_mode` (protection bits), :attr:`st_ino` (inode
   number), :attr:`st_dev` (device), :attr:`st_nlink` (number of hard links),
   :attr:`st_uid` (user ID of owner), :attr:`st_gid` (group ID of owner),
   :attr:`st_size` (size of file, in bytes), :attr:`st_atime` (time of most recent
   access), :attr:`st_mtime` (time of most recent content modification),
   :attr:`st_ctime` (platform dependent; time of most recent metadata change on
   Unix, or the time of creation on Windows)::

      >>> import os
      >>> statinfo = os.stat('somefile.txt')
      >>> statinfo
      (33188, 422511L, 769L, 1, 1032, 100, 926L, 1105022698,1105022732, 1105022732)
      >>> statinfo.st_size
      926L
      >>>

   .. versionchanged:: 2.3
      If :func:`stat_float_times` returns true, the time values are floats, measuring
      seconds. Fractions of a second may be reported if the system supports that. On
      Mac OS, the times are always floats. See :func:`stat_float_times` for further
      discussion.

   On some Unix systems (such as Linux), the following attributes may also be
   available: :attr:`st_blocks` (number of blocks allocated for file),
   :attr:`st_blksize` (filesystem blocksize), :attr:`st_rdev` (type of device if an
   inode device). :attr:`st_flags` (user defined flags for file).

   On other Unix systems (such as FreeBSD), the following attributes may be
   available (but may be only filled out if root tries to use them): :attr:`st_gen`
   (file generation number), :attr:`st_birthtime` (time of file creation).

   On Mac OS systems, the following attributes may also be available:
   :attr:`st_rsize`, :attr:`st_creator`, :attr:`st_type`.

   On RISCOS systems, the following attributes are also available: :attr:`st_ftype`
   (file type), :attr:`st_attrs` (attributes), :attr:`st_obtype` (object type).

   .. index:: module: stat

   For backward compatibility, the return value of :func:`stat` is also accessible
   as a tuple of at least 10 integers giving the most important (and portable)
   members of the :ctype:`stat` structure, in the order :attr:`st_mode`,
   :attr:`st_ino`, :attr:`st_dev`, :attr:`st_nlink`, :attr:`st_uid`,
   :attr:`st_gid`, :attr:`st_size`, :attr:`st_atime`, :attr:`st_mtime`,
   :attr:`st_ctime`. More items may be added at the end by some implementations.
   The standard module :mod:`stat` defines functions and constants that are useful
   for extracting information from a :ctype:`stat` structure. (On Windows, some
   items are filled with dummy values.)

   .. note::

      The exact meaning and resolution of the :attr:`st_atime`, :attr:`st_mtime`, and
      :attr:`st_ctime` members depends on the operating system and the file system.
      For example, on Windows systems using the FAT or FAT32 file systems,
      :attr:`st_mtime` has 2-second resolution, and :attr:`st_atime` has only 1-day
      resolution.  See your operating system documentation for details.

   Availability: Macintosh, Unix, Windows.

   .. versionchanged:: 2.2
      Added access to values as attributes of the returned object.

   .. versionchanged:: 2.5
      Added st_gen, st_birthtime.


.. function:: stat_float_times([newvalue])

   Determine whether :class:`stat_result` represents time stamps as float objects.
   If *newvalue* is ``True``, future calls to :func:`stat` return floats, if it is
   ``False``, future calls return ints. If *newvalue* is omitted, return the
   current setting.

   For compatibility with older Python versions, accessing :class:`stat_result` as
   a tuple always returns integers.

   .. versionchanged:: 2.5
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


.. function:: statvfs(path)

   Perform a :cfunc:`statvfs` system call on the given path.  The return value is
   an object whose attributes describe the filesystem on the given path, and
   correspond to the members of the :ctype:`statvfs` structure, namely:
   :attr:`f_bsize`, :attr:`f_frsize`, :attr:`f_blocks`, :attr:`f_bfree`,
   :attr:`f_bavail`, :attr:`f_files`, :attr:`f_ffree`, :attr:`f_favail`,
   :attr:`f_flag`, :attr:`f_namemax`. Availability: Unix.

   .. index:: module: statvfs

   For backward compatibility, the return value is also accessible as a tuple whose
   values correspond to the attributes, in the order given above. The standard
   module :mod:`statvfs` defines constants that are useful for extracting
   information from a :ctype:`statvfs` structure when accessing it as a sequence;
   this remains useful when writing code that needs to work with versions of Python
   that don't support accessing the fields as attributes.

   .. versionchanged:: 2.2
      Added access to values as attributes of the returned object.


.. function:: symlink(src, dst)

   Create a symbolic link pointing to *src* named *dst*. Availability: Unix.


.. function:: tempnam([dir[, prefix]])

   Return a unique path name that is reasonable for creating a temporary file.
   This will be an absolute path that names a potential directory entry in the
   directory *dir* or a common location for temporary files if *dir* is omitted or
   ``None``.  If given and not ``None``, *prefix* is used to provide a short prefix
   to the filename.  Applications are responsible for properly creating and
   managing files created using paths returned by :func:`tempnam`; no automatic
   cleanup is provided. On Unix, the environment variable :envvar:`TMPDIR`
   overrides *dir*, while on Windows the :envvar:`TMP` is used.  The specific
   behavior of this function depends on the C library implementation; some aspects
   are underspecified in system documentation.

   .. warning::

      Use of :func:`tempnam` is vulnerable to symlink attacks; consider using
      :func:`tmpfile` (section :ref:`os-newstreams`) instead.

   Availability: Macintosh, Unix, Windows.


.. function:: tmpnam()

   Return a unique path name that is reasonable for creating a temporary file.
   This will be an absolute path that names a potential directory entry in a common
   location for temporary files.  Applications are responsible for properly
   creating and managing files created using paths returned by :func:`tmpnam`; no
   automatic cleanup is provided.

   .. warning::

      Use of :func:`tmpnam` is vulnerable to symlink attacks; consider using
      :func:`tmpfile` (section :ref:`os-newstreams`) instead.

   Availability: Unix, Windows.  This function probably shouldn't be used on
   Windows, though: Microsoft's implementation of :func:`tmpnam` always creates a
   name in the root directory of the current drive, and that's generally a poor
   location for a temp file (depending on privileges, you may not even be able to
   open a file using this name).


.. data:: TMP_MAX

   The maximum number of unique names that :func:`tmpnam` will generate before
   reusing names.


.. function:: unlink(path)

   Remove the file *path*.  This is the same function as :func:`remove`; the
   :func:`unlink` name is its traditional Unix name. Availability: Macintosh, Unix,
   Windows.


.. function:: utime(path, times)

   Set the access and modified times of the file specified by *path*. If *times* is
   ``None``, then the file's access and modified times are set to the current time.
   Otherwise, *times* must be a 2-tuple of numbers, of the form ``(atime, mtime)``
   which is used to set the access and modified times, respectively. Whether a
   directory can be given for *path* depends on whether the operating system
   implements directories as files (for example, Windows does not).  Note that the
   exact times you set here may not be returned by a subsequent :func:`stat` call,
   depending on the resolution with which your operating system records access and
   modification times; see :func:`stat`.

   .. versionchanged:: 2.0
      Added support for ``None`` for *times*.

   Availability: Macintosh, Unix, Windows.


.. function:: walk(top[, topdown=True [, onerror=None[, followlinks=False]]])

   .. index::
      single: directory; walking
      single: directory; traversal

   :func:`walk` generates the file names in a directory tree, by walking the tree
   either top down or bottom up. For each directory in the tree rooted at directory
   *top* (including *top* itself), it yields a 3-tuple ``(dirpath, dirnames,
   filenames)``.

   *dirpath* is a string, the path to the directory.  *dirnames* is a list of the
   names of the subdirectories in *dirpath* (excluding ``'.'`` and ``'..'``).
   *filenames* is a list of the names of the non-directory files in *dirpath*.
   Note that the names in the lists contain no path components.  To get a full path
   (which begins with *top*) to a file or directory in *dirpath*, do
   ``os.path.join(dirpath, name)``.

   If optional argument *topdown* is true or not specified, the triple for a
   directory is generated before the triples for any of its subdirectories
   (directories are generated top down).  If *topdown* is false, the triple for a
   directory is generated after the triples for all of its subdirectories
   (directories are generated bottom up).

   When *topdown* is true, the caller can modify the *dirnames* list in-place
   (perhaps using :keyword:`del` or slice assignment), and :func:`walk` will only
   recurse into the subdirectories whose names remain in *dirnames*; this can be
   used to prune the search, impose a specific order of visiting, or even to inform
   :func:`walk` about directories the caller creates or renames before it resumes
   :func:`walk` again.  Modifying *dirnames* when *topdown* is false is
   ineffective, because in bottom-up mode the directories in *dirnames* are
   generated before *dirpath* itself is generated.

   By default errors from the ``os.listdir()`` call are ignored.  If optional
   argument *onerror* is specified, it should be a function; it will be called with
   one argument, an :exc:`OSError` instance.  It can report the error to continue
   with the walk, or raise the exception to abort the walk.  Note that the filename
   is available as the ``filename`` attribute of the exception object.

   By default, :func:`walk` will not walk down into symbolic links that resolve to
   directories. Set *followlinks* to True to visit directories pointed to by
   symlinks, on systems that support them.

   .. versionadded:: 2.6
      The *followlinks* parameter.

   .. note::

      Be aware that setting *followlinks* to true can lead to infinite recursion if a
      link points to a parent directory of itself. :func:`walk` does not keep track of
      the directories it visited already.

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
          print root, "consumes",
          print sum(getsize(join(root, name)) for name in files),
          print "bytes in", len(files), "non-directory files"
          if 'CVS' in dirs:
              dirs.remove('CVS')  # don't visit CVS directories

   In the next example, walking the tree bottom up is essential: :func:`rmdir`
   doesn't allow deleting a directory before the directory is empty::

      # Delete everything reachable from the directory named in 'top',
      # assuming there are no symbolic links.
      # CAUTION:  This is dangerous!  For example, if top == '/', it
      # could delete all your disk files.
      import os
      for root, dirs, files in os.walk(top, topdown=False):
          for name in files:
              os.remove(os.path.join(root, name))
          for name in dirs:
              os.rmdir(os.path.join(root, name))

   .. versionadded:: 2.3


.. _os-process:

Process Management
------------------

These functions may be used to create and manage processes.

The various :func:`exec\*` functions take a list of arguments for the new
program loaded into the process.  In each case, the first of these arguments is
passed to the new program as its own name rather than as an argument a user may
have typed on a command line.  For the C programmer, this is the ``argv[0]``
passed to a program's :cfunc:`main`.  For example, ``os.execv('/bin/echo',
['foo', 'bar'])`` will only print ``bar`` on standard output; ``foo`` will seem
to be ignored.


.. function:: abort()

   Generate a :const:`SIGABRT` signal to the current process.  On Unix, the default
   behavior is to produce a core dump; on Windows, the process immediately returns
   an exit code of ``3``.  Be aware that programs which use :func:`signal.signal`
   to register a handler for :const:`SIGABRT` will behave differently.
   Availability: Macintosh, Unix, Windows.


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
   and will have the same process ID as the caller.  Errors will be reported as
   :exc:`OSError` exceptions.

   The ``'l'`` and ``'v'`` variants of the :func:`exec\*` functions differ in how
   command-line arguments are passed.  The ``'l'`` variants are perhaps the easiest
   to work with if the number of parameters is fixed when the code is written; the
   individual parameters simply become additional parameters to the :func:`execl\*`
   functions.  The ``'v'`` variants are good when the number of parameters is
   variable, with the arguments being passed in a list or tuple as the *args*
   parameter.  In either case, the arguments to the child process should start with
   the name of the command being run, but this is not enforced.

   The variants which include a ``'p'`` near the end (:func:`execlp`,
   :func:`execlpe`, :func:`execvp`, and :func:`execvpe`) will use the
   :envvar:`PATH` environment variable to locate the program *file*.  When the
   environment is being replaced (using one of the :func:`exec\*e` variants,
   discussed in the next paragraph), the new environment is used as the source of
   the :envvar:`PATH` variable. The other variants, :func:`execl`, :func:`execle`,
   :func:`execv`, and :func:`execve`, will not use the :envvar:`PATH` variable to
   locate the executable; *path* must contain an appropriate absolute or relative
   path.

   For :func:`execle`, :func:`execlpe`, :func:`execve`, and :func:`execvpe` (note
   that these all end in ``'e'``), the *env* parameter must be a mapping which is
   used to define the environment variables for the new process; the :func:`execl`,
   :func:`execlp`, :func:`execv`, and :func:`execvp` all cause the new process to
   inherit the environment of the current process. Availability: Macintosh, Unix,
   Windows.


.. function:: _exit(n)

   Exit to the system with status *n*, without calling cleanup handlers, flushing
   stdio buffers, etc. Availability: Macintosh, Unix, Windows.

   .. note::

      The standard way to exit is ``sys.exit(n)``. :func:`_exit` should normally only
      be used in the child process after a :func:`fork`.

The following exit codes are a defined, and can be used with :func:`_exit`,
although they are not required.  These are typically used for system programs
written in Python, such as a mail server's external command delivery program.

.. note::

   Some of these may not be available on all Unix platforms, since there is some
   variation.  These constants are defined where they are defined by the underlying
   platform.


.. data:: EX_OK

   Exit code that means no error occurred. Availability: Macintosh, Unix.

   .. versionadded:: 2.3


.. data:: EX_USAGE

   Exit code that means the command was used incorrectly, such as when the wrong
   number of arguments are given. Availability: Macintosh, Unix.

   .. versionadded:: 2.3


.. data:: EX_DATAERR

   Exit code that means the input data was incorrect. Availability: Macintosh,
   Unix.

   .. versionadded:: 2.3


.. data:: EX_NOINPUT

   Exit code that means an input file did not exist or was not readable.
   Availability: Macintosh, Unix.

   .. versionadded:: 2.3


.. data:: EX_NOUSER

   Exit code that means a specified user did not exist. Availability: Macintosh,
   Unix.

   .. versionadded:: 2.3


.. data:: EX_NOHOST

   Exit code that means a specified host did not exist. Availability: Macintosh,
   Unix.

   .. versionadded:: 2.3


.. data:: EX_UNAVAILABLE

   Exit code that means that a required service is unavailable. Availability:
   Macintosh, Unix.

   .. versionadded:: 2.3


.. data:: EX_SOFTWARE

   Exit code that means an internal software error was detected. Availability:
   Macintosh, Unix.

   .. versionadded:: 2.3


.. data:: EX_OSERR

   Exit code that means an operating system error was detected, such as the
   inability to fork or create a pipe. Availability: Macintosh, Unix.

   .. versionadded:: 2.3


.. data:: EX_OSFILE

   Exit code that means some system file did not exist, could not be opened, or had
   some other kind of error. Availability: Macintosh, Unix.

   .. versionadded:: 2.3


.. data:: EX_CANTCREAT

   Exit code that means a user specified output file could not be created.
   Availability: Macintosh, Unix.

   .. versionadded:: 2.3


.. data:: EX_IOERR

   Exit code that means that an error occurred while doing I/O on some file.
   Availability: Macintosh, Unix.

   .. versionadded:: 2.3


.. data:: EX_TEMPFAIL

   Exit code that means a temporary failure occurred.  This indicates something
   that may not really be an error, such as a network connection that couldn't be
   made during a retryable operation. Availability: Macintosh, Unix.

   .. versionadded:: 2.3


.. data:: EX_PROTOCOL

   Exit code that means that a protocol exchange was illegal, invalid, or not
   understood. Availability: Macintosh, Unix.

   .. versionadded:: 2.3


.. data:: EX_NOPERM

   Exit code that means that there were insufficient permissions to perform the
   operation (but not intended for file system problems). Availability: Macintosh,
   Unix.

   .. versionadded:: 2.3


.. data:: EX_CONFIG

   Exit code that means that some kind of configuration error occurred.
   Availability: Macintosh, Unix.

   .. versionadded:: 2.3


.. data:: EX_NOTFOUND

   Exit code that means something like "an entry was not found". Availability:
   Macintosh, Unix.

   .. versionadded:: 2.3


.. function:: fork()

   Fork a child process.  Return ``0`` in the child, the child's process id in the
   parent. Availability: Macintosh, Unix.


.. function:: forkpty()

   Fork a child process, using a new pseudo-terminal as the child's controlling
   terminal. Return a pair of ``(pid, fd)``, where *pid* is ``0`` in the child, the
   new child's process id in the parent, and *fd* is the file descriptor of the
   master end of the pseudo-terminal.  For a more portable approach, use the
   :mod:`pty` module. Availability: Macintosh, Some flavors of Unix.


.. function:: kill(pid, sig)

   .. index::
      single: process; killing
      single: process; signalling

   Send signal *sig* to the process *pid*.  Constants for the specific signals
   available on the host platform are defined in the :mod:`signal` module.
   Availability: Macintosh, Unix.


.. function:: killpg(pgid, sig)

   .. index::
      single: process; killing
      single: process; signalling

   Send the signal *sig* to the process group *pgid*. Availability: Macintosh,
   Unix.

   .. versionadded:: 2.3


.. function:: nice(increment)

   Add *increment* to the process's "niceness".  Return the new niceness.
   Availability: Macintosh, Unix.


.. function:: plock(op)

   Lock program segments into memory.  The value of *op* (defined in
   ``<sys/lock.h>``) determines which segments are locked. Availability: Macintosh,
   Unix.


.. function:: popen(...)
              popen2(...)
              popen3(...)
              popen4(...)
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
   preferable to using these functions.)

   If *mode* is :const:`P_NOWAIT`, this function returns the process ID of the new
   process; if *mode* is :const:`P_WAIT`, returns the process's exit code if it
   exits normally, or ``-signal``, where *signal* is the signal that killed the
   process.  On Windows, the process ID will actually be the process handle, so can
   be used with the :func:`waitpid` function.

   The ``'l'`` and ``'v'`` variants of the :func:`spawn\*` functions differ in how
   command-line arguments are passed.  The ``'l'`` variants are perhaps the easiest
   to work with if the number of parameters is fixed when the code is written; the
   individual parameters simply become additional parameters to the
   :func:`spawnl\*` functions.  The ``'v'`` variants are good when the number of
   parameters is variable, with the arguments being passed in a list or tuple as
   the *args* parameter.  In either case, the arguments to the child process must
   start with the name of the command being run.

   The variants which include a second ``'p'`` near the end (:func:`spawnlp`,
   :func:`spawnlpe`, :func:`spawnvp`, and :func:`spawnvpe`) will use the
   :envvar:`PATH` environment variable to locate the program *file*.  When the
   environment is being replaced (using one of the :func:`spawn\*e` variants,
   discussed in the next paragraph), the new environment is used as the source of
   the :envvar:`PATH` variable.  The other variants, :func:`spawnl`,
   :func:`spawnle`, :func:`spawnv`, and :func:`spawnve`, will not use the
   :envvar:`PATH` variable to locate the executable; *path* must contain an
   appropriate absolute or relative path.

   For :func:`spawnle`, :func:`spawnlpe`, :func:`spawnve`, and :func:`spawnvpe`
   (note that these all end in ``'e'``), the *env* parameter must be a mapping
   which is used to define the environment variables for the new process; the
   :func:`spawnl`, :func:`spawnlp`, :func:`spawnv`, and :func:`spawnvp` all cause
   the new process to inherit the environment of the current process.

   As an example, the following calls to :func:`spawnlp` and :func:`spawnvpe` are
   equivalent::

      import os
      os.spawnlp(os.P_WAIT, 'cp', 'cp', 'index.html', '/dev/null')

      L = ['cp', 'index.html', '/dev/null']
      os.spawnvpe(os.P_WAIT, 'cp', L, os.environ)

   Availability: Unix, Windows.  :func:`spawnlp`, :func:`spawnlpe`, :func:`spawnvp`
   and :func:`spawnvpe` are not available on Windows.

   .. versionadded:: 1.6


.. data:: P_NOWAIT
          P_NOWAITO

   Possible values for the *mode* parameter to the :func:`spawn\*` family of
   functions.  If either of these values is given, the :func:`spawn\*` functions
   will return as soon as the new process has been created, with the process ID as
   the return value. Availability: Macintosh, Unix, Windows.

   .. versionadded:: 1.6


.. data:: P_WAIT

   Possible value for the *mode* parameter to the :func:`spawn\*` family of
   functions.  If this is given as *mode*, the :func:`spawn\*` functions will not
   return until the new process has run to completion and will return the exit code
   of the process the run is successful, or ``-signal`` if a signal kills the
   process. Availability: Macintosh, Unix, Windows.

   .. versionadded:: 1.6


.. data:: P_DETACH
          P_OVERLAY

   Possible values for the *mode* parameter to the :func:`spawn\*` family of
   functions.  These are less portable than those listed above. :const:`P_DETACH`
   is similar to :const:`P_NOWAIT`, but the new process is detached from the
   console of the calling process. If :const:`P_OVERLAY` is used, the current
   process will be replaced; the :func:`spawn\*` function will not return.
   Availability: Windows.

   .. versionadded:: 1.6


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
   is not a slash (``'/'``); the underlying Win32 :cfunc:`ShellExecute` function
   doesn't work if it is.  Use the :func:`os.path.normpath` function to ensure that
   the path is properly encoded for Win32. Availability: Windows.

   .. versionadded:: 2.0

   .. versionadded:: 2.5
      The *operation* parameter.


.. function:: system(command)

   Execute the command (a string) in a subshell.  This is implemented by calling
   the Standard C function :cfunc:`system`, and has the same limitations.  Changes
   to ``posix.environ``, ``sys.stdin``, etc. are not reflected in the environment
   of the executed command.

   On Unix, the return value is the exit status of the process encoded in the
   format specified for :func:`wait`.  Note that POSIX does not specify the meaning
   of the return value of the C :cfunc:`system` function, so the return value of
   the Python function is system-dependent.

   On Windows, the return value is that returned by the system shell after running
   *command*, given by the Windows environment variable :envvar:`COMSPEC`: on
   :program:`command.com` systems (Windows 95, 98 and ME) this is always ``0``; on
   :program:`cmd.exe` systems (Windows NT, 2000 and XP) this is the exit status of
   the command run; on systems using a non-native shell, consult your shell
   documentation.

   Availability: Macintosh, Unix, Windows.

   The :mod:`subprocess` module provides more powerful facilities for spawning new
   processes and retrieving their results; using that module is preferable to using
   this function.


.. function:: times()

   Return a 5-tuple of floating point numbers indicating accumulated (processor or
   other) times, in seconds.  The items are: user time, system time, children's
   user time, children's system time, and elapsed real time since a fixed point in
   the past, in that order.  See the Unix manual page :manpage:`times(2)` or the
   corresponding Windows Platform API documentation. Availability: Macintosh, Unix,
   Windows.


.. function:: wait()

   Wait for completion of a child process, and return a tuple containing its pid
   and exit status indication: a 16-bit number, whose low byte is the signal number
   that killed the process, and whose high byte is the exit status (if the signal
   number is zero); the high bit of the low byte is set if a core file was
   produced. Availability: Macintosh, Unix.


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

   .. versionadded:: 2.5


.. function:: wait4(pid, options)

   Similar to :func:`waitpid`, except a 3-element tuple, containing the child's
   process id, exit status indication, and resource usage information is returned.
   Refer to :mod:`resource`.\ :func:`getrusage` for details on resource usage
   information.  The arguments to :func:`wait4` are the same as those provided to
   :func:`waitpid`. Availability: Unix.

   .. versionadded:: 2.5


.. data:: WNOHANG

   The option for :func:`waitpid` to return immediately if no child process status
   is available immediately. The function returns ``(0, 0)`` in this case.
   Availability: Macintosh, Unix.


.. data:: WCONTINUED

   This option causes child processes to be reported if they have been continued
   from a job control stop since their status was last reported. Availability: Some
   Unix systems.

   .. versionadded:: 2.3


.. data:: WUNTRACED

   This option causes child processes to be reported if they have been stopped but
   their current state has not been reported since they were stopped. Availability:
   Macintosh, Unix.

   .. versionadded:: 2.3

The following functions take a process status code as returned by
:func:`system`, :func:`wait`, or :func:`waitpid` as a parameter.  They may be
used to determine the disposition of a process.


.. function:: WCOREDUMP(status)

   Returns ``True`` if a core dump was generated for the process, otherwise it
   returns ``False``. Availability: Macintosh, Unix.

   .. versionadded:: 2.3


.. function:: WIFCONTINUED(status)

   Returns ``True`` if the process has been continued from a job control stop,
   otherwise it returns ``False``. Availability: Unix.

   .. versionadded:: 2.3


.. function:: WIFSTOPPED(status)

   Returns ``True`` if the process has been stopped, otherwise it returns
   ``False``. Availability: Unix.


.. function:: WIFSIGNALED(status)

   Returns ``True`` if the process exited due to a signal, otherwise it returns
   ``False``. Availability: Macintosh, Unix.


.. function:: WIFEXITED(status)

   Returns ``True`` if the process exited using the :manpage:`exit(2)` system call,
   otherwise it returns ``False``. Availability: Macintosh, Unix.


.. function:: WEXITSTATUS(status)

   If ``WIFEXITED(status)`` is true, return the integer parameter to the
   :manpage:`exit(2)` system call.  Otherwise, the return value is meaningless.
   Availability: Macintosh, Unix.


.. function:: WSTOPSIG(status)

   Return the signal which caused the process to stop. Availability: Macintosh,
   Unix.


.. function:: WTERMSIG(status)

   Return the signal which caused the process to exit. Availability: Macintosh,
   Unix.


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
   mapping, passing an integer for *name* is also accepted. Availability:
   Macintosh, Unix.

   If the configuration value specified by *name* isn't defined, ``None`` is
   returned.

   If *name* is a string and is not known, :exc:`ValueError` is raised.  If a
   specific value for *name* is not supported by the host system, even if it is
   included in ``confstr_names``, an :exc:`OSError` is raised with
   :const:`errno.EINVAL` for the error number.


.. data:: confstr_names

   Dictionary mapping names accepted by :func:`confstr` to the integer values
   defined for those names by the host operating system. This can be used to
   determine the set of names known to the system. Availability: Macintosh, Unix.


.. function:: getloadavg()

   Return the number of processes in the system run queue averaged over the last 1,
   5, and 15 minutes or raises :exc:`OSError` if the load  average was
   unobtainable.

   .. versionadded:: 2.3


.. function:: sysconf(name)

   Return integer-valued system configuration values. If the configuration value
   specified by *name* isn't defined, ``-1`` is returned.  The comments regarding
   the *name* parameter for :func:`confstr` apply here as well; the dictionary that
   provides information on the known names is given by ``sysconf_names``.
   Availability: Macintosh, Unix.


.. data:: sysconf_names

   Dictionary mapping names accepted by :func:`sysconf` to the integer values
   defined for those names by the host operating system. This can be used to
   determine the set of names known to the system. Availability: Macintosh, Unix.

The follow data values are used to support path manipulation operations.  These
are defined for all platforms.

Higher-level operations on pathnames are defined in the :mod:`os.path` module.


.. data:: curdir

   The constant string used by the operating system to refer to the current
   directory. For example: ``'.'`` for POSIX or ``':'`` for Mac OS 9. Also
   available via :mod:`os.path`.


.. data:: pardir

   The constant string used by the operating system to refer to the parent
   directory. For example: ``'..'`` for POSIX or ``'::'`` for Mac OS 9. Also
   available via :mod:`os.path`.


.. data:: sep

   The character used by the operating system to separate pathname components, for
   example, ``'/'`` for POSIX or ``':'`` for Mac OS 9.  Note that knowing this is
   not sufficient to be able to parse or concatenate pathnames --- use
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

   .. versionadded:: 2.2


.. data:: pathsep

   The character conventionally used by the operating system to separate search
   path components (as in :envvar:`PATH`), such as ``':'`` for POSIX or ``';'`` for
   Windows. Also available via :mod:`os.path`.


.. data:: defpath

   The default search path used by :func:`exec\*p\*` and :func:`spawn\*p\*` if the
   environment doesn't have a ``'PATH'`` key. Also available via :mod:`os.path`.


.. data:: linesep

   The string used to separate (or, rather, terminate) lines on the current
   platform.  This may be a single character, such as  ``'\n'`` for POSIX or
   ``'\r'`` for Mac OS, or multiple  characters, for example, ``'\r\n'`` for
   Windows. Do not use *os.linesep* as a line terminator when writing files  opened
   in text mode (the default); use a single ``'\n'`` instead,  on all platforms.


.. data:: devnull

   The file path of the null device. For example: ``'/dev/null'`` for POSIX or
   ``'Dev:Nul'`` for Mac OS 9. Also available via :mod:`os.path`.

   .. versionadded:: 2.4


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

   .. versionadded:: 2.4

