:mod:`fcntl` --- The ``fcntl`` and ``ioctl`` system calls
=========================================================

.. module:: fcntl
   :platform: Unix
   :synopsis: The fcntl() and ioctl() system calls.
.. sectionauthor:: Jaap Vermeulen


.. index::
   pair: UNIX; file control
   pair: UNIX; I/O control

This module performs file control and I/O control on file descriptors. It is an
interface to the :c:func:`fcntl` and :c:func:`ioctl` Unix routines.  For a
complete description of these calls, see :manpage:`fcntl(2)` and
:manpage:`ioctl(2)` Unix manual pages.

All functions in this module take a file descriptor *fd* as their first
argument.  This can be an integer file descriptor, such as returned by
``sys.stdin.fileno()``, or a file object, such as ``sys.stdin`` itself, which
provides a :meth:`~io.IOBase.fileno` which returns a genuine file descriptor.

The module defines the following functions:


.. function:: fcntl(fd, op[, arg])

   Perform the operation *op* on file descriptor *fd* (file objects providing
   a :meth:`~io.IOBase.fileno` method are accepted as well).  The values used
   for for *op* are operating system dependent, and are available as constants
   in the :mod:`fcntl` module, using the same names as used in the relevant C
   header files.  The argument *arg* is optional, and defaults to the integer
   value ``0``.  When present, it can either be an integer value, or a string.
   With the argument missing or an integer value, the return value of this function
   is the integer return value of the C :c:func:`fcntl` call.  When the argument is
   a string it represents a binary structure, e.g. created by :func:`struct.pack`.
   The binary data is copied to a buffer whose address is passed to the C
   :c:func:`fcntl` call.  The return value after a successful call is the contents
   of the buffer, converted to a string object.  The length of the returned string
   will be the same as the length of the *arg* argument.  This is limited to 1024
   bytes.  If the information returned in the buffer by the operating system is
   larger than 1024 bytes, this is most likely to result in a segmentation
   violation or a more subtle data corruption.

   If the :c:func:`fcntl` fails, an :exc:`IOError` is raised.


.. function:: ioctl(fd, op[, arg[, mutate_flag]])

   This function is identical to the :func:`~fcntl.fcntl` function, except that the
   operations are typically defined in the library module :mod:`termios` and the
   argument handling is even more complicated.

   The op parameter is limited to values that can fit in 32-bits.
   Additional constants of interest for use as the *op* argument can be
   found in the :mod:`termios` module, under the same names as used in
   the relevant C header files.

   The parameter *arg* can be one of an integer, absent (treated identically to the
   integer ``0``), an object supporting the read-only buffer interface (most likely
   a plain Python string) or an object supporting the read-write buffer interface.

   In all but the last case, behaviour is as for the :func:`~fcntl.fcntl`
   function.

   If a mutable buffer is passed, then the behaviour is determined by the value of
   the *mutate_flag* parameter.

   If it is false, the buffer's mutability is ignored and behaviour is as for a
   read-only buffer, except that the 1024 byte limit mentioned above is avoided --
   so long as the buffer you pass is as least as long as what the operating system
   wants to put there, things should work.

   If *mutate_flag* is true, then the buffer is (in effect) passed to the
   underlying :func:`ioctl` system call, the latter's return code is passed back to
   the calling Python, and the buffer's new contents reflect the action of the
   :func:`ioctl`.  This is a slight simplification, because if the supplied buffer
   is less than 1024 bytes long it is first copied into a static buffer 1024 bytes
   long which is then passed to :func:`ioctl` and copied back into the supplied
   buffer.

   If *mutate_flag* is not supplied, then from Python 2.5 it defaults to true,
   which is a change from versions 2.3 and 2.4. Supply the argument explicitly if
   version portability is a priority.

   If the :c:func:`ioctl` fails, an :exc:`IOError` exception is raised.

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


.. function:: flock(fd, op)

   Perform the lock operation *op* on file descriptor *fd* (file objects providing
   a :meth:`~io.IOBase.fileno` method are accepted as well). See the Unix manual
   :manpage:`flock(2)` for details.  (On some systems, this function is emulated
   using :c:func:`fcntl`.)

   If the :c:func:`flock` fails, an :exc:`IOError` exception is raised.


.. function:: lockf(fd, operation, [length, [start, [whence]]])

   This is essentially a wrapper around the :func:`~fcntl.fcntl` locking calls.
   *fd* is the file descriptor of the file to lock or unlock, and *operation*
   is one of the following values:

   * :const:`LOCK_UN` -- unlock
   * :const:`LOCK_SH` -- acquire a shared lock
   * :const:`LOCK_EX` -- acquire an exclusive lock

   When *operation* is :const:`LOCK_SH` or :const:`LOCK_EX`, it can also be
   bitwise ORed with :const:`LOCK_NB` to avoid blocking on lock acquisition.
   If :const:`LOCK_NB` is used and the lock cannot be acquired, an
   :exc:`IOError` will be raised and the exception will have an *errno*
   attribute set to :const:`EACCES` or :const:`EAGAIN` (depending on the
   operating system; for portability, check for both values).  On at least some
   systems, :const:`LOCK_EX` can only be used if the file descriptor refers to a
   file opened for writing.

   *length* is the number of bytes to lock, *start* is the byte offset at
   which the lock starts, relative to *whence*, and *whence* is as with
   :func:`io.IOBase.seek`, specifically:

   * :const:`0` -- relative to the start of the file (:data:`os.SEEK_SET`)
   * :const:`1` -- relative to the current buffer position (:data:`os.SEEK_CUR`)
   * :const:`2` -- relative to the end of the file (:data:`os.SEEK_END`)

   The default for *start* is 0, which means to start at the beginning of the file.
   The default for *length* is 0 which means to lock to the end of the file.  The
   default for *whence* is also 0.

Examples (all on a SVR4 compliant system)::

   import struct, fcntl, os

   f = open(...)
   rv = fcntl.fcntl(f, fcntl.F_SETFL, os.O_NDELAY)

   lockdata = struct.pack('hhllhh', fcntl.F_WRLCK, 0, 0, 0, 0, 0)
   rv = fcntl.fcntl(f, fcntl.F_SETLKW, lockdata)

Note that in the first example the return value variable *rv* will hold an
integer value; in the second example it will hold a string value.  The structure
lay-out for the *lockdata* variable is system dependent --- therefore using the
:func:`flock` call may be better.


.. seealso::

   Module :mod:`os`
      If the locking flags :data:`~os.O_SHLOCK` and :data:`~os.O_EXLOCK` are
      present in the :mod:`os` module (on BSD only), the :func:`os.open`
      function provides an alternative to the :func:`lockf` and :func:`flock`
      functions.

