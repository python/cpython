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

Of the following list, symbols that are not used on the current platform are not
defined by the module.  The specific list of defined symbols is available as
``errno.errorcode.keys()``.  Symbols available can include:


.. data:: EPERM

   Operation not permitted. This error is mapped to the exception
   :exc:`PermissionError`.


.. data:: ENOENT

   No such file or directory. This error is mapped to the exception
   :exc:`FileNotFoundError`.


.. data:: ESRCH

   No such process. This error is mapped to the exception
   :exc:`ProcessLookupError`.


.. data:: EINTR

   Interrupted system call. This error is mapped to the exception
   :exc:`InterruptedError`.


.. data:: EIO

   I/O error


.. data:: ENXIO

   No such device or address


.. data:: E2BIG

   Arg list too long


.. data:: ENOEXEC

   Exec format error


.. data:: EBADF

   Bad file number


.. data:: ECHILD

   No child processes. This error is mapped to the exception
   :exc:`ChildProcessError`.


.. data:: EAGAIN

   Try again. This error is mapped to the exception :exc:`BlockingIOError`.


.. data:: ENOMEM

   Out of memory


.. data:: EACCES

   Permission denied.  This error is mapped to the exception
   :exc:`PermissionError`.


.. data:: EFAULT

   Bad address


.. data:: ENOTBLK

   Block device required


.. data:: EBUSY

   Device or resource busy


.. data:: EEXIST

   File exists. This error is mapped to the exception
   :exc:`FileExistsError`.


.. data:: EXDEV

   Cross-device link


.. data:: ENODEV

   No such device


.. data:: ENOTDIR

   Not a directory. This error is mapped to the exception
   :exc:`NotADirectoryError`.


.. data:: EISDIR

   Is a directory. This error is mapped to the exception
   :exc:`IsADirectoryError`.


.. data:: EINVAL

   Invalid argument


.. data:: ENFILE

   File table overflow


.. data:: EMFILE

   Too many open files


.. data:: ENOTTY

   Not a typewriter


.. data:: ETXTBSY

   Text file busy


.. data:: EFBIG

   File too large


.. data:: ENOSPC

   No space left on device


.. data:: ESPIPE

   Illegal seek


.. data:: EROFS

   Read-only file system


.. data:: EMLINK

   Too many links


.. data:: EPIPE

   Broken pipe. This error is mapped to the exception
   :exc:`BrokenPipeError`.


.. data:: EDOM

   Math argument out of domain of func


.. data:: ERANGE

   Math result not representable


.. data:: EDEADLK

   Resource deadlock would occur


.. data:: ENAMETOOLONG

   File name too long


.. data:: ENOLCK

   No record locks available


.. data:: ENOSYS

   Function not implemented


.. data:: ENOTEMPTY

   Directory not empty


.. data:: ELOOP

   Too many symbolic links encountered


.. data:: EWOULDBLOCK

   Operation would block. This error is mapped to the exception
   :exc:`BlockingIOError`.


.. data:: ENOMSG

   No message of desired type


.. data:: EIDRM

   Identifier removed


.. data:: ECHRNG

   Channel number out of range


.. data:: EL2NSYNC

   Level 2 not synchronized


.. data:: EL3HLT

   Level 3 halted


.. data:: EL3RST

   Level 3 reset


.. data:: ELNRNG

   Link number out of range


.. data:: EUNATCH

   Protocol driver not attached


.. data:: ENOCSI

   No CSI structure available


.. data:: EL2HLT

   Level 2 halted


.. data:: EBADE

   Invalid exchange


.. data:: EBADR

   Invalid request descriptor


.. data:: EXFULL

   Exchange full


.. data:: ENOANO

   No anode


.. data:: EBADRQC

   Invalid request code


.. data:: EBADSLT

   Invalid slot


.. data:: EDEADLOCK

   File locking deadlock error


.. data:: EBFONT

   Bad font file format


.. data:: ENOSTR

   Device not a stream


.. data:: ENODATA

   No data available


.. data:: ETIME

   Timer expired


.. data:: ENOSR

   Out of streams resources


.. data:: ENONET

   Machine is not on the network


.. data:: ENOPKG

   Package not installed


.. data:: EREMOTE

   Object is remote


.. data:: ENOLINK

   Link has been severed


.. data:: EADV

   Advertise error


.. data:: ESRMNT

   Srmount error


.. data:: ECOMM

   Communication error on send


.. data:: EPROTO

   Protocol error


.. data:: EMULTIHOP

   Multihop attempted


.. data:: EDOTDOT

   RFS specific error


.. data:: EBADMSG

   Not a data message


.. data:: EOVERFLOW

   Value too large for defined data type


.. data:: ENOTUNIQ

   Name not unique on network


.. data:: EBADFD

   File descriptor in bad state


.. data:: EREMCHG

   Remote address changed


.. data:: ELIBACC

   Can not access a needed shared library


.. data:: ELIBBAD

   Accessing a corrupted shared library


.. data:: ELIBSCN

   .lib section in a.out corrupted


.. data:: ELIBMAX

   Attempting to link in too many shared libraries


.. data:: ELIBEXEC

   Cannot exec a shared library directly


.. data:: EILSEQ

   Illegal byte sequence


.. data:: ERESTART

   Interrupted system call should be restarted


.. data:: ESTRPIPE

   Streams pipe error


.. data:: EUSERS

   Too many users


.. data:: ENOTSOCK

   Socket operation on non-socket


.. data:: EDESTADDRREQ

   Destination address required


.. data:: EMSGSIZE

   Message too long


.. data:: EPROTOTYPE

   Protocol wrong type for socket


.. data:: ENOPROTOOPT

   Protocol not available


.. data:: EPROTONOSUPPORT

   Protocol not supported


.. data:: ESOCKTNOSUPPORT

   Socket type not supported


.. data:: EOPNOTSUPP

   Operation not supported on transport endpoint


.. data:: ENOTSUP

   Operation not supported

   .. versionadded:: 3.2


.. data:: EPFNOSUPPORT

   Protocol family not supported


.. data:: EAFNOSUPPORT

   Address family not supported by protocol


.. data:: EADDRINUSE

   Address already in use


.. data:: EADDRNOTAVAIL

   Cannot assign requested address


.. data:: ENETDOWN

   Network is down


.. data:: ENETUNREACH

   Network is unreachable


.. data:: ENETRESET

   Network dropped connection because of reset


.. data:: ECONNABORTED

   Software caused connection abort. This error is mapped to the
   exception :exc:`ConnectionAbortedError`.


.. data:: ECONNRESET

   Connection reset by peer. This error is mapped to the exception
   :exc:`ConnectionResetError`.


.. data:: ENOBUFS

   No buffer space available


.. data:: EISCONN

   Transport endpoint is already connected


.. data:: ENOTCONN

   Transport endpoint is not connected


.. data:: ESHUTDOWN

   Cannot send after transport endpoint shutdown. This error is mapped
   to the exception :exc:`BrokenPipeError`.


.. data:: ETOOMANYREFS

   Too many references: cannot splice


.. data:: ETIMEDOUT

   Connection timed out. This error is mapped to the exception
   :exc:`TimeoutError`.


.. data:: ECONNREFUSED

   Connection refused. This error is mapped to the exception
   :exc:`ConnectionRefusedError`.


.. data:: EHOSTDOWN

   Host is down


.. data:: EHOSTUNREACH

   No route to host


.. data:: EALREADY

   Operation already in progress. This error is mapped to the
   exception :exc:`BlockingIOError`.


.. data:: EINPROGRESS

   Operation now in progress. This error is mapped to the exception
   :exc:`BlockingIOError`.


.. data:: ESTALE

   Stale NFS file handle


.. data:: EUCLEAN

   Structure needs cleaning


.. data:: ENOTNAM

   Not a XENIX named type file


.. data:: ENAVAIL

   No XENIX semaphores available


.. data:: EISNAM

   Is a named type file


.. data:: EREMOTEIO

   Remote I/O error


.. data:: EDQUOT

   Quota exceeded

.. data:: EQFULL

   Interface output queue is full

   .. versionadded:: 3.11

.. data:: ENOTCAPABLE

   Capabilities insufficient. This error is mapped to the exception
   :exc:`PermissionError`.

   .. availability:: WASI, FreeBSD

   .. versionadded:: 3.11.1


.. data:: ECANCELED

   Operation canceled

   .. versionadded:: 3.2


.. data:: EOWNERDEAD

   Owner died

   .. versionadded:: 3.2


.. data:: ENOTRECOVERABLE

   State not recoverable

   .. versionadded:: 3.2
