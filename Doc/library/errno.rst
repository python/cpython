:mod:`!errno` --- Standard errno system symbols
===============================================

.. module:: errno
   :synopsis: Standard errno system symbols.

----------------

This module makes available standard ``errno`` system symbols. The value of each
symbol is the corresponding integer value. The names and descriptions are
borrowed from :file:`linux/include/errno.h`, which should be
all-inclusive.


.. data:: errorcode

   Dictionary providing a mapping from the errno value to the string name in the
   underlying system.  For instance, ``errno.errorcode[errno.EPERM]`` maps to
   ``'EPERM'``.

To translate a numeric error code to an error message, use :func:`os.strerror`.

Of the following table, symbols that are not used on the current platform are not
defined by the module.  The specific list of defined symbols is available as
``errno.errorcode.keys()``.  Symbols available can include:


.. list-table::
   :header-rows: 1

   * - Name
     - Description

   * - .. data:: EPERM
     - Operation not permitted. [#PermissionError]_

   * - .. data:: ENOENT
     - No such file or directory. [#FileNotFoundError]_

   * - .. data:: ESRCH
     - No such process. [#ProcessLookupError]_

   * - .. data:: EINTR
     - Interrupted system call. [#InterruptedError]_

   * - .. data:: EIO
     - I/O error

   * - .. data:: ENXIO
     - No such device or address

   * - .. data:: E2BIG
     - Arg list too long

   * - .. data:: ENOEXEC
     - Exec format error

   * - .. data:: EBADF
     - Bad file number

   * - .. data:: ECHILD
     - No child processes. [#ChildProcessError]_

   * - .. data:: EAGAIN
     - Try again. [#BlockingIOError]_

   * - .. data:: ENOMEM
     - Out of memory

   * - .. data:: EACCES
     - Permission denied. [#PermissionError]_

   * - .. data:: EFAULT
     - Bad address

   * - .. data:: ENOTBLK
     - Block device required

   * - .. data:: EBUSY
     - Device or resource busy

   * - .. data:: EEXIST
     - File exists. [#FileExistsError]_

   * - .. data:: EXDEV
     - Cross-device link

   * - .. data:: ENODEV
     - No such device

   * - .. data:: ENOTDIR
     - Not a directory. [#NotADirectoryError]_

   * - .. data:: EISDIR
     - Is a directory. [#IsADirectoryError]_

   * - .. data:: EINVAL
     - Invalid argument

   * - .. data:: ENFILE
     - File table overflow

   * - .. data:: EMFILE
     - Too many open files

   * - .. data:: ENOTTY
     - Not a typewriter

   * - .. data:: ETXTBSY
     - Text file busy

   * - .. data:: EFBIG
     - File too large

   * - .. data:: ENOSPC
     - No space left on device

   * - .. data:: ESPIPE
     - Illegal seek

   * - .. data:: EROFS
     - Read-only file system

   * - .. data:: EMLINK
     - Too many links

   * - .. data:: EPIPE
     - Broken pipe. [#BrokenPipeError]_

   * - .. data:: EDOM
     - Math argument out of domain of func

   * - .. data:: ERANGE
     - Math result not representable

   * - .. data:: EDEADLK
     - Resource deadlock would occur

   * - .. data:: ENAMETOOLONG
     - File name too long

   * - .. data:: ENOLCK
     - No record locks available

   * - .. data:: ENOSYS
     - Function not implemented

   * - .. data:: ENOTEMPTY
     - Directory not empty

   * - .. data:: ELOOP
     - Too many symbolic links encountered

   * - .. data:: EWOULDBLOCK
     - Operation would block. [#BlockingIOError]_

   * - .. data:: ENOMSG
     - No message of desired type

   * - .. data:: EIDRM
     - Identifier removed

   * - .. data:: ECHRNG
     - Channel number out of range

   * - .. data:: EL2NSYNC
     - Level 2 not synchronized

   * - .. data:: EL3HLT
     - Level 3 halted

   * - .. data:: EL3RST
     - Level 3 reset

   * - .. data:: ELNRNG
     - Link number out of range

   * - .. data:: EUNATCH
     - Protocol driver not attached

   * - .. data:: ENOCSI
     - No CSI structure available

   * - .. data:: EL2HLT
     - Level 2 halted

   * - .. data:: EBADE
     - Invalid exchange

   * - .. data:: EBADR
     - Invalid request descriptor

   * - .. data:: EXFULL
     - Exchange full

   * - .. data:: ENOANO
     - No anode

   * - .. data:: EBADRQC
     - Invalid request code

   * - .. data:: EBADSLT
     - Invalid slot

   * - .. data:: EDEADLOCK
     - File locking deadlock error

   * - .. data:: EBFONT
     - Bad font file format

   * - .. data:: ENOSTR
     - Device not a stream

   * - .. data:: ENODATA
     - No data available

   * - .. data:: ETIME
     - Timer expired

   * - .. data:: ENOSR
     - Out of streams resources

   * - .. data:: ENONET
     - Machine is not on the network

   * - .. data:: ENOPKG
     - Package not installed

   * - .. data:: EREMOTE
     - Object is remote

   * - .. data:: ENOLINK
     - Link has been severed

   * - .. data:: EADV
     - Advertise error

   * - .. data:: ESRMNT
     - Srmount error

   * - .. data:: ECOMM
     - Communication error on send

   * - .. data:: EPROTO
     - Protocol error

   * - .. data:: EMULTIHOP
     - Multihop attempted

   * - .. data:: EDOTDOT
     - RFS specific error

   * - .. data:: EBADMSG
     - Not a data message

   * - .. data:: EOVERFLOW
     - Value too large for defined data type

   * - .. data:: ENOTUNIQ
     - Name not unique on network

   * - .. data:: EBADFD
     - File descriptor in bad state

   * - .. data:: EREMCHG
     - Remote address changed

   * - .. data:: ELIBACC
     - Can not access a needed shared library

   * - .. data:: ELIBBAD
     - Accessing a corrupted shared library

   * - .. data:: ELIBSCN
     - .lib section in a.out corrupted

   * - .. data:: ELIBMAX
     - Attempting to link in too many shared libraries

   * - .. data:: ELIBEXEC
     - Cannot exec a shared library directly

   * - .. data:: EILSEQ
     - Illegal byte sequence

   * - .. data:: ERESTART
     - Interrupted system call should be restarted

   * - .. data:: ESTRPIPE
     - Streams pipe error

   * - .. data:: EUSERS
     - Too many users

   * - .. data:: ENOTSOCK
     - Socket operation on non-socket

   * - .. data:: EDESTADDRREQ
     - Destination address required

   * - .. data:: EMSGSIZE
     - Message too long

   * - .. data:: EPROTOTYPE
     - Protocol wrong type for socket

   * - .. data:: ENOPROTOOPT
     - Protocol not available

   * - .. data:: EPROTONOSUPPORT
     - Protocol not supported

   * - .. data:: ESOCKTNOSUPPORT
     - Socket type not supported

   * - .. data:: EOPNOTSUPP
     - Operation not supported on transport endpoint

   * - .. data:: ENOTSUP
     - Operation not supported

   * - .. data:: EPFNOSUPPORT
     - Protocol family not supported

   * - .. data:: EAFNOSUPPORT
     - Address family not supported by protocol

   * - .. data:: EADDRINUSE
     - Address already in use

   * - .. data:: EADDRNOTAVAIL
     - Cannot assign requested address

   * - .. data:: ENETDOWN
     - Network is down

   * - .. data:: ENETUNREACH
     - Network is unreachable

   * - .. data:: ENETRESET
     - Network dropped connection because of reset

   * - .. data:: ECONNABORTED
     - Software caused connection abort. [#ConnectionAbortedError]_

   * - .. data:: ECONNRESET
     - Connection reset by peer. [#ConnectionResetError]_

   * - .. data:: ENOBUFS
     - No buffer space available

   * - .. data:: EISCONN
     - Transport endpoint is already connected

   * - .. data:: ENOTCONN
     - Transport endpoint is not connected

   * - .. data:: ESHUTDOWN
     - Cannot send after transport endpoint shutdown. [#BrokenPipeError]_

   * - .. data:: ETOOMANYREFS
     - Too many references: cannot splice

   * - .. data:: ETIMEDOUT
     - Connection timed out. [#TimeoutError]_

   * - .. data:: ECONNREFUSED
     - Connection refused. [#ConnectionRefusedError]_

   * - .. data:: EHOSTDOWN
     - Host is down

   * - .. data:: EHOSTUNREACH
     - No route to host

   * - .. data:: EALREADY
     - Operation already in progress. [#BlockingIOError]_

   * - .. data:: EINPROGRESS
     - Operation now in progress. [#BlockingIOError]_

   * - .. data:: ESTALE
     - Stale NFS file handle

   * - .. data:: EUCLEAN
     - Structure needs cleaning

   * - .. data:: ENOTNAM
     - Not a XENIX named type file

   * - .. data:: ENAVAIL
     - No XENIX semaphores available

   * - .. data:: EISNAM
     - Is a named type file

   * - .. data:: EREMOTEIO
     - Remote I/O error

   * - .. data:: EDQUOT
     - Quota exceeded

   * - .. data:: EQFULL
     - Interface output queue is full

   * - .. data:: ENOTCAPABLE
     - Capabilities insufficient. [#PermissionError]_

   * - .. data:: ECANCELED
     - Operation canceled

   * - .. data:: EOWNERDEAD
     - Owner died

   * - .. data:: ENOTRECOVERABLE
     - State not recoverable

.. versionadded:: 3.2

   * :data:`errno.ENOTSUP`
   * :data:`errno.ECANCELED`
   * :data:`errno.EOWNERDEAD`
   * :data:`errno.ENOTRECOVERABLE`

.. versionadded:: 3.11

   * :data:`errno.EQFULL`

.. versionadded:: 3.11.1

   * :data:`errno.ENOTCAPABLE`

.. rubric:: Footnotes

.. [#BlockingIOError] This error is mapped to the exception :exc:`BlockingIOError`.
.. [#BrokenPipeError] This error is mapped to the exception :exc:`BrokenPipeError`.
.. [#ChildProcessError] This error is mapped to the exception :exc:`ChildProcessError`.
.. [#ConnectionAbortedError] This error is mapped to the exception :exc:`ConnectionAbortedError`.
.. [#ConnectionRefusedError] This error is mapped to the exception :exc:`ConnectionRefusedError`.
.. [#ConnectionResetError] This error is mapped to the exception :exc:`ConnectionResetError`.
.. [#FileExistsError] This error is mapped to the exception :exc:`FileExistsError`.
.. [#FileNotFoundError] This error is mapped to the exception :exc:`FileNotFoundError`.
.. [#InterruptedError] This error is mapped to the exception :exc:`InterruptedError`.
.. [#IsADirectoryError] This error is mapped to the exception :exc:`IsADirectoryError`.
.. [#NotADirectoryError] This error is mapped to the exception :exc:`NotADirectoryError`.
.. [#PermissionError] This error is mapped to the exception :exc:`PermissionError`.
.. [#ProcessLookupError] This error is mapped to the exception :exc:`ProcessLookupError`.
.. [#TimeoutError] This error is mapped to the exception :exc:`TimeoutError`.
