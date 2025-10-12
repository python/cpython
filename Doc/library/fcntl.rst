:mod:`!fcntl` --- The ``fcntl`` and ``ioctl`` system calls
==========================================================

.. module:: fcntl
   :platform: Unix
   :synopsis: The fcntl() and ioctl() system calls.

.. sectionauthor:: Jaap Vermeulen

.. index::
   pair: UNIX; file control
   pair: UNIX; I/O control

----------------

This module performs file and I/O control on file descriptors. It is an
interface to the :c:func:`fcntl` and :c:func:`ioctl` Unix routines.
See the :manpage:`fcntl(2)` and :manpage:`ioctl(2)` Unix manual pages
for full details.

.. availability:: Unix, not WASI.

All functions in this module take a file descriptor *fd* as their first
argument.  This can be an integer file descriptor, such as returned by
``sys.stdin.fileno()``, or an :class:`io.IOBase` object, such as ``sys.stdin``
itself, which provides a :meth:`~io.IOBase.fileno` that returns a genuine file
descriptor.

.. versionchanged:: 3.3
   Operations in this module used to raise an :exc:`IOError` where they now
   raise an :exc:`OSError`.

.. versionchanged:: 3.8
   The :mod:`!fcntl` module now contains ``F_ADD_SEALS``, ``F_GET_SEALS``, and
   ``F_SEAL_*`` constants for sealing of :func:`os.memfd_create` file
   descriptors.

.. versionchanged:: 3.9
   On macOS, the :mod:`!fcntl` module exposes the ``F_GETPATH`` constant,
   which obtains the path of a file from a file descriptor.
   On Linux(>=3.15), the :mod:`!fcntl` module exposes the ``F_OFD_GETLK``,
   ``F_OFD_SETLK`` and ``F_OFD_SETLKW`` constants, which are used when working
   with open file description locks.

.. versionchanged:: 3.10
   On Linux >= 2.6.11, the :mod:`!fcntl` module exposes the ``F_GETPIPE_SZ`` and
   ``F_SETPIPE_SZ`` constants, which allow to check and modify a pipe's size
   respectively.

.. versionchanged:: 3.11
   On FreeBSD, the :mod:`!fcntl` module exposes the ``F_DUP2FD`` and
   ``F_DUP2FD_CLOEXEC`` constants, which allow to duplicate a file descriptor,
   the latter setting ``FD_CLOEXEC`` flag in addition.

.. versionchanged:: 3.12
   On Linux >= 4.5, the :mod:`fcntl` module exposes the ``FICLONE`` and
   ``FICLONERANGE`` constants, which allow to share some data of one file with
   another file by reflinking on some filesystems (e.g., btrfs, OCFS2, and
   XFS). This behavior is commonly referred to as "copy-on-write".

.. versionchanged:: 3.13
   On Linux >= 2.6.32, the :mod:`!fcntl` module exposes the
   ``F_GETOWN_EX``, ``F_SETOWN_EX``, ``F_OWNER_TID``, ``F_OWNER_PID``, ``F_OWNER_PGRP`` constants, which allow to direct I/O availability signals
   to a specific thread, process, or process group.
   On Linux >= 4.13, the :mod:`!fcntl` module exposes the
   ``F_GET_RW_HINT``, ``F_SET_RW_HINT``, ``F_GET_FILE_RW_HINT``,
   ``F_SET_FILE_RW_HINT``, and ``RWH_WRITE_LIFE_*`` constants, which allow
   to inform the kernel about the relative expected lifetime of writes on
   a given inode or via a particular open file description.
   On Linux >= 5.1 and NetBSD, the :mod:`!fcntl` module exposes the
   ``F_SEAL_FUTURE_WRITE`` constant for use with ``F_ADD_SEALS`` and
   ``F_GET_SEALS`` operations.
   On FreeBSD, the :mod:`!fcntl` module exposes the ``F_READAHEAD``, ``F_ISUNIONSTACK``, and ``F_KINFO`` constants.
   On macOS and FreeBSD, the :mod:`!fcntl` module exposes the ``F_RDAHEAD``
   constant.
   On NetBSD and AIX, the :mod:`!fcntl` module exposes the ``F_CLOSEM``
   constant.
   On NetBSD, the :mod:`!fcntl` module exposes the ``F_MAXFD`` constant.
   On macOS and NetBSD, the :mod:`!fcntl` module exposes the ``F_GETNOSIGPIPE``
   and ``F_SETNOSIGPIPE`` constant.

.. versionchanged:: 3.14
   On Linux >= 6.1, the :mod:`!fcntl` module exposes the ``F_DUPFD_QUERY``
   to query a file descriptor pointing to the same file.

The module defines the following functions:


.. function:: fcntl(fd, cmd, arg=0, /)

   Perform the operation *cmd* on file descriptor *fd* (file objects providing
   a :meth:`~io.IOBase.fileno` method are accepted as well).  The values used
   for *cmd* are operating system dependent, and are available as constants
   in the :mod:`fcntl` module, using the same names as used in the relevant C
   header files. The argument *arg* can either be an integer value, a
   :term:`bytes-like object`, or a string.
   The type and size of *arg* must match the type and size of
   the argument of the operation as specified in the relevant C documentation.

   When *arg* is an integer, the function returns the integer
   return value of the C :c:func:`fcntl` call.

   When the argument is bytes-like object, it represents a binary structure,
   for example, created by :func:`struct.pack`.
   A string value is encoded to binary using the UTF-8 encoding.
   The binary data is copied to a buffer whose address is
   passed to the C :c:func:`fcntl` call.  The return value after a successful
   call is the contents of the buffer, converted to a :class:`bytes` object.
   The length of the returned object will be the same as the length of the
   *arg* argument.

   If the :c:func:`fcntl` call fails, an :exc:`OSError` is raised.

   .. note::
      If the type or size of *arg* does not match the type or size
      of the operation's argument (for example, if an integer is
      passed when a pointer is expected, or the information returned in
      the buffer by the operating system is larger than the size of *arg*),
      this is most likely to result in a segmentation violation or
      a more subtle data corruption.

   .. audit-event:: fcntl.fcntl fd,cmd,arg fcntl.fcntl

   .. versionchanged:: 3.14
      Add support of arbitrary :term:`bytes-like objects <bytes-like object>`,
      not only :class:`bytes`.

   .. versionchanged:: next
      The size of bytes-like objects is no longer limited to 1024 bytes.


.. function:: ioctl(fd, request, arg=0, mutate_flag=True, /)

   This function is identical to the :func:`~fcntl.fcntl` function, except
   that the argument handling is even more complicated.

   The *request* parameter is limited to values that can fit in 32-bits
   or 64-bits, depending on the platform.
   Additional constants of interest for use as the *request* argument can be
   found in the :mod:`termios` module, under the same names as used in
   the relevant C header files.

   The parameter *arg* can be an integer, a :term:`bytes-like object`,
   or a string.
   The type and size of *arg* must match the type and size of
   the argument of the operation as specified in the relevant C documentation.

   If *arg* does not support the read-write buffer interface or
   the *mutate_flag* is false, behavior is as for the :func:`~fcntl.fcntl`
   function.

   If *arg* supports the read-write buffer interface (like :class:`bytearray`)
   and *mutate_flag* is true (the default), then the buffer is (in effect) passed
   to the underlying :c:func:`!ioctl` system call, the latter's return code is
   passed back to the calling Python, and the buffer's new contents reflect the
   action of the :c:func:`ioctl`.  This is a slight simplification, because if the
   supplied buffer is less than 1024 bytes long it is first copied into a static
   buffer 1024 bytes long which is then passed to :func:`ioctl` and copied back
   into the supplied buffer.

   If the :c:func:`ioctl` call fails, an :exc:`OSError` exception is raised.

   .. note::
      If the type or size of *arg* does not match the type or size
      of the operation's argument (for example, if an integer is
      passed when a pointer is expected, or the information returned in
      the buffer by the operating system is larger than the size of *arg*),
      this is most likely to result in a segmentation violation or
      a more subtle data corruption.

   An example::

      >>> import array, fcntl, struct, termios, os
      >>> os.getpgrp()
      13341
      >>> struct.unpack('h', fcntl.ioctl(0, termios.TIOCGPGRP, "  "))[0]
      13341
      >>> buf = array.array('h', [0])
      >>> fcntl.ioctl(0, termios.TIOCGPGRP, buf, 1)
      0
      >>> buf
      array('h', [13341])

   .. audit-event:: fcntl.ioctl fd,request,arg fcntl.ioctl

   .. versionchanged:: 3.14
      The GIL is always released during a system call.
      System calls failing with EINTR are automatically retried.

   .. versionchanged:: next
      The size of not mutated bytes-like objects is no longer
      limited to 1024 bytes.

.. function:: flock(fd, operation, /)

   Perform the lock operation *operation* on file descriptor *fd* (file objects providing
   a :meth:`~io.IOBase.fileno` method are accepted as well). See the Unix manual
   :manpage:`flock(2)` for details.  (On some systems, this function is emulated
   using :c:func:`fcntl`.)

   If the :c:func:`flock` call fails, an :exc:`OSError` exception is raised.

   .. audit-event:: fcntl.flock fd,operation fcntl.flock


.. function:: lockf(fd, cmd, len=0, start=0, whence=0, /)

   This is essentially a wrapper around the :func:`~fcntl.fcntl` locking calls.
   *fd* is the file descriptor (file objects providing a :meth:`~io.IOBase.fileno`
   method are accepted as well) of the file to lock or unlock, and *cmd*
   is one of the following values:

   .. data:: LOCK_UN

      Release an existing lock.

   .. data:: LOCK_SH

      Acquire a shared lock.

   .. data:: LOCK_EX

      Acquire an exclusive lock.

   .. data:: LOCK_NB

      Bitwise OR with any of the other three ``LOCK_*`` constants to make
      the request non-blocking.

   If :const:`!LOCK_NB` is used and the lock cannot be acquired, an
   :exc:`OSError` will be raised and the exception will have an *errno*
   attribute set to :const:`~errno.EACCES` or :const:`~errno.EAGAIN` (depending on the
   operating system; for portability, check for both values).  On at least some
   systems, :const:`!LOCK_EX` can only be used if the file descriptor refers to a
   file opened for writing.

   *len* is the number of bytes to lock, *start* is the byte offset at
   which the lock starts, relative to *whence*, and *whence* is as with
   :func:`io.IOBase.seek`, specifically:

   * ``0`` -- relative to the start of the file (:const:`os.SEEK_SET`)
   * ``1`` -- relative to the current buffer position (:const:`os.SEEK_CUR`)
   * ``2`` -- relative to the end of the file (:const:`os.SEEK_END`)

   The default for *start* is 0, which means to start at the beginning of the file.
   The default for *len* is 0 which means to lock to the end of the file.  The
   default for *whence* is also 0.

   .. audit-event:: fcntl.lockf fd,cmd,len,start,whence fcntl.lockf

Examples (all on a SVR4 compliant system)::

   import struct, fcntl, os

   f = open(...)
   rv = fcntl.fcntl(f, fcntl.F_SETFL, os.O_NDELAY)

   lockdata = struct.pack('hhllhh', fcntl.F_WRLCK, 0, 0, 0, 0, 0)
   rv = fcntl.fcntl(f, fcntl.F_SETLKW, lockdata)

Note that in the first example the return value variable *rv* will hold an
integer value; in the second example it will hold a :class:`bytes` object.  The
structure lay-out for the *lockdata* variable is system dependent --- therefore
using the :func:`flock` call may be better.


.. seealso::

   Module :mod:`os`
      If the locking flags :const:`~os.O_SHLOCK` and :const:`~os.O_EXLOCK` are
      present in the :mod:`os` module (on BSD only), the :func:`os.open`
      function provides an alternative to the :func:`lockf` and :func:`flock`
      functions.
