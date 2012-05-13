
/* Errno module */

#include "Python.h"

/* Windows socket errors (WSA*)  */
#ifdef MS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/* The following constants were added to errno.h in VS2010 but have
   preferred WSA equivalents. */
#undef EADDRINUSE
#undef EADDRNOTAVAIL
#undef EAFNOSUPPORT
#undef EALREADY
#undef ECONNABORTED
#undef ECONNREFUSED
#undef ECONNRESET
#undef EDESTADDRREQ
#undef EHOSTUNREACH
#undef EINPROGRESS
#undef EISCONN
#undef ELOOP
#undef EMSGSIZE
#undef ENETDOWN
#undef ENETRESET
#undef ENETUNREACH
#undef ENOBUFS
#undef ENOPROTOOPT
#undef ENOTCONN
#undef ENOTSOCK
#undef EOPNOTSUPP
#undef EPROTONOSUPPORT
#undef EPROTOTYPE
#undef ETIMEDOUT
#undef EWOULDBLOCK
#endif

/*
 * Pull in the system error definitions
 */

static PyMethodDef errno_methods[] = {
    {NULL,              NULL}
};

/* Helper function doing the dictionary inserting */

static void
_inscode(PyObject *d, PyObject *de, const char *name, int code)
{
    PyObject *u = PyUnicode_FromString(name);
    PyObject *v = PyLong_FromLong((long) code);

    /* Don't bother checking for errors; they'll be caught at the end
     * of the module initialization function by the caller of
     * initerrno().
     */
    if (u && v) {
        /* insert in modules dict */
        PyDict_SetItem(d, u, v);
        /* insert in errorcode dict */
        PyDict_SetItem(de, v, u);
    }
    Py_XDECREF(u);
    Py_XDECREF(v);
}

PyDoc_STRVAR(errno__doc__,
"This module makes available standard errno system symbols.\n\
\n\
The value of each symbol is the corresponding integer value,\n\
e.g., on most systems, errno.ENOENT equals the integer 2.\n\
\n\
The dictionary errno.errorcode maps numeric codes to symbol names,\n\
e.g., errno.errorcode[2] could be the string 'ENOENT'.\n\
\n\
Symbols that are not relevant to the underlying system are not defined.\n\
\n\
To map error codes to error messages, use the function os.strerror(),\n\
e.g. os.strerror(2) could return 'No such file or directory'.");

static struct PyModuleDef errnomodule = {
    PyModuleDef_HEAD_INIT,
    "errno",
    errno__doc__,
    -1,
    errno_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_errno(void)
{
    PyObject *m, *d, *de;
    m = PyModule_Create(&errnomodule);
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);
    de = PyDict_New();
    if (!d || !de || PyDict_SetItemString(d, "errorcode", de) < 0)
        return NULL;

/* Macro so I don't have to edit each and every line below... */
#define inscode(d, ds, de, name, code, comment) _inscode(d, de, name, code)

    /*
     * The names and comments are borrowed from linux/include/errno.h,
     * which should be pretty all-inclusive.  However, the Solaris specific
     * names and comments are borrowed from sys/errno.h in Solaris.
     * MacOSX specific names and comments are borrowed from sys/errno.h in
     * MacOSX.
     */

#ifdef ENODEV
    inscode(d, ds, de, "ENODEV", ENODEV, "No such device");
#endif
#ifdef ENOCSI
    inscode(d, ds, de, "ENOCSI", ENOCSI, "No CSI structure available");
#endif
#ifdef EHOSTUNREACH
    inscode(d, ds, de, "EHOSTUNREACH", EHOSTUNREACH, "No route to host");
#else
#ifdef WSAEHOSTUNREACH
    inscode(d, ds, de, "EHOSTUNREACH", WSAEHOSTUNREACH, "No route to host");
#endif
#endif
#ifdef ENOMSG
    inscode(d, ds, de, "ENOMSG", ENOMSG, "No message of desired type");
#endif
#ifdef EUCLEAN
    inscode(d, ds, de, "EUCLEAN", EUCLEAN, "Structure needs cleaning");
#endif
#ifdef EL2NSYNC
    inscode(d, ds, de, "EL2NSYNC", EL2NSYNC, "Level 2 not synchronized");
#endif
#ifdef EL2HLT
    inscode(d, ds, de, "EL2HLT", EL2HLT, "Level 2 halted");
#endif
#ifdef ENODATA
    inscode(d, ds, de, "ENODATA", ENODATA, "No data available");
#endif
#ifdef ENOTBLK
    inscode(d, ds, de, "ENOTBLK", ENOTBLK, "Block device required");
#endif
#ifdef ENOSYS
    inscode(d, ds, de, "ENOSYS", ENOSYS, "Function not implemented");
#endif
#ifdef EPIPE
    inscode(d, ds, de, "EPIPE", EPIPE, "Broken pipe");
#endif
#ifdef EINVAL
    inscode(d, ds, de, "EINVAL", EINVAL, "Invalid argument");
#else
#ifdef WSAEINVAL
    inscode(d, ds, de, "EINVAL", WSAEINVAL, "Invalid argument");
#endif
#endif
#ifdef EOVERFLOW
    inscode(d, ds, de, "EOVERFLOW", EOVERFLOW, "Value too large for defined data type");
#endif
#ifdef EADV
    inscode(d, ds, de, "EADV", EADV, "Advertise error");
#endif
#ifdef EINTR
    inscode(d, ds, de, "EINTR", EINTR, "Interrupted system call");
#else
#ifdef WSAEINTR
    inscode(d, ds, de, "EINTR", WSAEINTR, "Interrupted system call");
#endif
#endif
#ifdef EUSERS
    inscode(d, ds, de, "EUSERS", EUSERS, "Too many users");
#else
#ifdef WSAEUSERS
    inscode(d, ds, de, "EUSERS", WSAEUSERS, "Too many users");
#endif
#endif
#ifdef ENOTEMPTY
    inscode(d, ds, de, "ENOTEMPTY", ENOTEMPTY, "Directory not empty");
#else
#ifdef WSAENOTEMPTY
    inscode(d, ds, de, "ENOTEMPTY", WSAENOTEMPTY, "Directory not empty");
#endif
#endif
#ifdef ENOBUFS
    inscode(d, ds, de, "ENOBUFS", ENOBUFS, "No buffer space available");
#else
#ifdef WSAENOBUFS
    inscode(d, ds, de, "ENOBUFS", WSAENOBUFS, "No buffer space available");
#endif
#endif
#ifdef EPROTO
    inscode(d, ds, de, "EPROTO", EPROTO, "Protocol error");
#endif
#ifdef EREMOTE
    inscode(d, ds, de, "EREMOTE", EREMOTE, "Object is remote");
#else
#ifdef WSAEREMOTE
    inscode(d, ds, de, "EREMOTE", WSAEREMOTE, "Object is remote");
#endif
#endif
#ifdef ENAVAIL
    inscode(d, ds, de, "ENAVAIL", ENAVAIL, "No XENIX semaphores available");
#endif
#ifdef ECHILD
    inscode(d, ds, de, "ECHILD", ECHILD, "No child processes");
#endif
#ifdef ELOOP
    inscode(d, ds, de, "ELOOP", ELOOP, "Too many symbolic links encountered");
#else
#ifdef WSAELOOP
    inscode(d, ds, de, "ELOOP", WSAELOOP, "Too many symbolic links encountered");
#endif
#endif
#ifdef EXDEV
    inscode(d, ds, de, "EXDEV", EXDEV, "Cross-device link");
#endif
#ifdef E2BIG
    inscode(d, ds, de, "E2BIG", E2BIG, "Arg list too long");
#endif
#ifdef ESRCH
    inscode(d, ds, de, "ESRCH", ESRCH, "No such process");
#endif
#ifdef EMSGSIZE
    inscode(d, ds, de, "EMSGSIZE", EMSGSIZE, "Message too long");
#else
#ifdef WSAEMSGSIZE
    inscode(d, ds, de, "EMSGSIZE", WSAEMSGSIZE, "Message too long");
#endif
#endif
#ifdef EAFNOSUPPORT
    inscode(d, ds, de, "EAFNOSUPPORT", EAFNOSUPPORT, "Address family not supported by protocol");
#else
#ifdef WSAEAFNOSUPPORT
    inscode(d, ds, de, "EAFNOSUPPORT", WSAEAFNOSUPPORT, "Address family not supported by protocol");
#endif
#endif
#ifdef EBADR
    inscode(d, ds, de, "EBADR", EBADR, "Invalid request descriptor");
#endif
#ifdef EHOSTDOWN
    inscode(d, ds, de, "EHOSTDOWN", EHOSTDOWN, "Host is down");
#else
#ifdef WSAEHOSTDOWN
    inscode(d, ds, de, "EHOSTDOWN", WSAEHOSTDOWN, "Host is down");
#endif
#endif
#ifdef EPFNOSUPPORT
    inscode(d, ds, de, "EPFNOSUPPORT", EPFNOSUPPORT, "Protocol family not supported");
#else
#ifdef WSAEPFNOSUPPORT
    inscode(d, ds, de, "EPFNOSUPPORT", WSAEPFNOSUPPORT, "Protocol family not supported");
#endif
#endif
#ifdef ENOPROTOOPT
    inscode(d, ds, de, "ENOPROTOOPT", ENOPROTOOPT, "Protocol not available");
#else
#ifdef WSAENOPROTOOPT
    inscode(d, ds, de, "ENOPROTOOPT", WSAENOPROTOOPT, "Protocol not available");
#endif
#endif
#ifdef EBUSY
    inscode(d, ds, de, "EBUSY", EBUSY, "Device or resource busy");
#endif
#ifdef EWOULDBLOCK
    inscode(d, ds, de, "EWOULDBLOCK", EWOULDBLOCK, "Operation would block");
#else
#ifdef WSAEWOULDBLOCK
    inscode(d, ds, de, "EWOULDBLOCK", WSAEWOULDBLOCK, "Operation would block");
#endif
#endif
#ifdef EBADFD
    inscode(d, ds, de, "EBADFD", EBADFD, "File descriptor in bad state");
#endif
#ifdef EDOTDOT
    inscode(d, ds, de, "EDOTDOT", EDOTDOT, "RFS specific error");
#endif
#ifdef EISCONN
    inscode(d, ds, de, "EISCONN", EISCONN, "Transport endpoint is already connected");
#else
#ifdef WSAEISCONN
    inscode(d, ds, de, "EISCONN", WSAEISCONN, "Transport endpoint is already connected");
#endif
#endif
#ifdef ENOANO
    inscode(d, ds, de, "ENOANO", ENOANO, "No anode");
#endif
#ifdef ESHUTDOWN
    inscode(d, ds, de, "ESHUTDOWN", ESHUTDOWN, "Cannot send after transport endpoint shutdown");
#else
#ifdef WSAESHUTDOWN
    inscode(d, ds, de, "ESHUTDOWN", WSAESHUTDOWN, "Cannot send after transport endpoint shutdown");
#endif
#endif
#ifdef ECHRNG
    inscode(d, ds, de, "ECHRNG", ECHRNG, "Channel number out of range");
#endif
#ifdef ELIBBAD
    inscode(d, ds, de, "ELIBBAD", ELIBBAD, "Accessing a corrupted shared library");
#endif
#ifdef ENONET
    inscode(d, ds, de, "ENONET", ENONET, "Machine is not on the network");
#endif
#ifdef EBADE
    inscode(d, ds, de, "EBADE", EBADE, "Invalid exchange");
#endif
#ifdef EBADF
    inscode(d, ds, de, "EBADF", EBADF, "Bad file number");
#else
#ifdef WSAEBADF
    inscode(d, ds, de, "EBADF", WSAEBADF, "Bad file number");
#endif
#endif
#ifdef EMULTIHOP
    inscode(d, ds, de, "EMULTIHOP", EMULTIHOP, "Multihop attempted");
#endif
#ifdef EIO
    inscode(d, ds, de, "EIO", EIO, "I/O error");
#endif
#ifdef EUNATCH
    inscode(d, ds, de, "EUNATCH", EUNATCH, "Protocol driver not attached");
#endif
#ifdef EPROTOTYPE
    inscode(d, ds, de, "EPROTOTYPE", EPROTOTYPE, "Protocol wrong type for socket");
#else
#ifdef WSAEPROTOTYPE
    inscode(d, ds, de, "EPROTOTYPE", WSAEPROTOTYPE, "Protocol wrong type for socket");
#endif
#endif
#ifdef ENOSPC
    inscode(d, ds, de, "ENOSPC", ENOSPC, "No space left on device");
#endif
#ifdef ENOEXEC
    inscode(d, ds, de, "ENOEXEC", ENOEXEC, "Exec format error");
#endif
#ifdef EALREADY
    inscode(d, ds, de, "EALREADY", EALREADY, "Operation already in progress");
#else
#ifdef WSAEALREADY
    inscode(d, ds, de, "EALREADY", WSAEALREADY, "Operation already in progress");
#endif
#endif
#ifdef ENETDOWN
    inscode(d, ds, de, "ENETDOWN", ENETDOWN, "Network is down");
#else
#ifdef WSAENETDOWN
    inscode(d, ds, de, "ENETDOWN", WSAENETDOWN, "Network is down");
#endif
#endif
#ifdef ENOTNAM
    inscode(d, ds, de, "ENOTNAM", ENOTNAM, "Not a XENIX named type file");
#endif
#ifdef EACCES
    inscode(d, ds, de, "EACCES", EACCES, "Permission denied");
#else
#ifdef WSAEACCES
    inscode(d, ds, de, "EACCES", WSAEACCES, "Permission denied");
#endif
#endif
#ifdef ELNRNG
    inscode(d, ds, de, "ELNRNG", ELNRNG, "Link number out of range");
#endif
#ifdef EILSEQ
    inscode(d, ds, de, "EILSEQ", EILSEQ, "Illegal byte sequence");
#endif
#ifdef ENOTDIR
    inscode(d, ds, de, "ENOTDIR", ENOTDIR, "Not a directory");
#endif
#ifdef ENOTUNIQ
    inscode(d, ds, de, "ENOTUNIQ", ENOTUNIQ, "Name not unique on network");
#endif
#ifdef EPERM
    inscode(d, ds, de, "EPERM", EPERM, "Operation not permitted");
#endif
#ifdef EDOM
    inscode(d, ds, de, "EDOM", EDOM, "Math argument out of domain of func");
#endif
#ifdef EXFULL
    inscode(d, ds, de, "EXFULL", EXFULL, "Exchange full");
#endif
#ifdef ECONNREFUSED
    inscode(d, ds, de, "ECONNREFUSED", ECONNREFUSED, "Connection refused");
#else
#ifdef WSAECONNREFUSED
    inscode(d, ds, de, "ECONNREFUSED", WSAECONNREFUSED, "Connection refused");
#endif
#endif
#ifdef EISDIR
    inscode(d, ds, de, "EISDIR", EISDIR, "Is a directory");
#endif
#ifdef EPROTONOSUPPORT
    inscode(d, ds, de, "EPROTONOSUPPORT", EPROTONOSUPPORT, "Protocol not supported");
#else
#ifdef WSAEPROTONOSUPPORT
    inscode(d, ds, de, "EPROTONOSUPPORT", WSAEPROTONOSUPPORT, "Protocol not supported");
#endif
#endif
#ifdef EROFS
    inscode(d, ds, de, "EROFS", EROFS, "Read-only file system");
#endif
#ifdef EADDRNOTAVAIL
    inscode(d, ds, de, "EADDRNOTAVAIL", EADDRNOTAVAIL, "Cannot assign requested address");
#else
#ifdef WSAEADDRNOTAVAIL
    inscode(d, ds, de, "EADDRNOTAVAIL", WSAEADDRNOTAVAIL, "Cannot assign requested address");
#endif
#endif
#ifdef EIDRM
    inscode(d, ds, de, "EIDRM", EIDRM, "Identifier removed");
#endif
#ifdef ECOMM
    inscode(d, ds, de, "ECOMM", ECOMM, "Communication error on send");
#endif
#ifdef ESRMNT
    inscode(d, ds, de, "ESRMNT", ESRMNT, "Srmount error");
#endif
#ifdef EREMOTEIO
    inscode(d, ds, de, "EREMOTEIO", EREMOTEIO, "Remote I/O error");
#endif
#ifdef EL3RST
    inscode(d, ds, de, "EL3RST", EL3RST, "Level 3 reset");
#endif
#ifdef EBADMSG
    inscode(d, ds, de, "EBADMSG", EBADMSG, "Not a data message");
#endif
#ifdef ENFILE
    inscode(d, ds, de, "ENFILE", ENFILE, "File table overflow");
#endif
#ifdef ELIBMAX
    inscode(d, ds, de, "ELIBMAX", ELIBMAX, "Attempting to link in too many shared libraries");
#endif
#ifdef ESPIPE
    inscode(d, ds, de, "ESPIPE", ESPIPE, "Illegal seek");
#endif
#ifdef ENOLINK
    inscode(d, ds, de, "ENOLINK", ENOLINK, "Link has been severed");
#endif
#ifdef ENETRESET
    inscode(d, ds, de, "ENETRESET", ENETRESET, "Network dropped connection because of reset");
#else
#ifdef WSAENETRESET
    inscode(d, ds, de, "ENETRESET", WSAENETRESET, "Network dropped connection because of reset");
#endif
#endif
#ifdef ETIMEDOUT
    inscode(d, ds, de, "ETIMEDOUT", ETIMEDOUT, "Connection timed out");
#else
#ifdef WSAETIMEDOUT
    inscode(d, ds, de, "ETIMEDOUT", WSAETIMEDOUT, "Connection timed out");
#endif
#endif
#ifdef ENOENT
    inscode(d, ds, de, "ENOENT", ENOENT, "No such file or directory");
#endif
#ifdef EEXIST
    inscode(d, ds, de, "EEXIST", EEXIST, "File exists");
#endif
#ifdef EDQUOT
    inscode(d, ds, de, "EDQUOT", EDQUOT, "Quota exceeded");
#else
#ifdef WSAEDQUOT
    inscode(d, ds, de, "EDQUOT", WSAEDQUOT, "Quota exceeded");
#endif
#endif
#ifdef ENOSTR
    inscode(d, ds, de, "ENOSTR", ENOSTR, "Device not a stream");
#endif
#ifdef EBADSLT
    inscode(d, ds, de, "EBADSLT", EBADSLT, "Invalid slot");
#endif
#ifdef EBADRQC
    inscode(d, ds, de, "EBADRQC", EBADRQC, "Invalid request code");
#endif
#ifdef ELIBACC
    inscode(d, ds, de, "ELIBACC", ELIBACC, "Can not access a needed shared library");
#endif
#ifdef EFAULT
    inscode(d, ds, de, "EFAULT", EFAULT, "Bad address");
#else
#ifdef WSAEFAULT
    inscode(d, ds, de, "EFAULT", WSAEFAULT, "Bad address");
#endif
#endif
#ifdef EFBIG
    inscode(d, ds, de, "EFBIG", EFBIG, "File too large");
#endif
#ifdef EDEADLK
    inscode(d, ds, de, "EDEADLK", EDEADLK, "Resource deadlock would occur");
#endif
#ifdef ENOTCONN
    inscode(d, ds, de, "ENOTCONN", ENOTCONN, "Transport endpoint is not connected");
#else
#ifdef WSAENOTCONN
    inscode(d, ds, de, "ENOTCONN", WSAENOTCONN, "Transport endpoint is not connected");
#endif
#endif
#ifdef EDESTADDRREQ
    inscode(d, ds, de, "EDESTADDRREQ", EDESTADDRREQ, "Destination address required");
#else
#ifdef WSAEDESTADDRREQ
    inscode(d, ds, de, "EDESTADDRREQ", WSAEDESTADDRREQ, "Destination address required");
#endif
#endif
#ifdef ELIBSCN
    inscode(d, ds, de, "ELIBSCN", ELIBSCN, ".lib section in a.out corrupted");
#endif
#ifdef ENOLCK
    inscode(d, ds, de, "ENOLCK", ENOLCK, "No record locks available");
#endif
#ifdef EISNAM
    inscode(d, ds, de, "EISNAM", EISNAM, "Is a named type file");
#endif
#ifdef ECONNABORTED
    inscode(d, ds, de, "ECONNABORTED", ECONNABORTED, "Software caused connection abort");
#else
#ifdef WSAECONNABORTED
    inscode(d, ds, de, "ECONNABORTED", WSAECONNABORTED, "Software caused connection abort");
#endif
#endif
#ifdef ENETUNREACH
    inscode(d, ds, de, "ENETUNREACH", ENETUNREACH, "Network is unreachable");
#else
#ifdef WSAENETUNREACH
    inscode(d, ds, de, "ENETUNREACH", WSAENETUNREACH, "Network is unreachable");
#endif
#endif
#ifdef ESTALE
    inscode(d, ds, de, "ESTALE", ESTALE, "Stale NFS file handle");
#else
#ifdef WSAESTALE
    inscode(d, ds, de, "ESTALE", WSAESTALE, "Stale NFS file handle");
#endif
#endif
#ifdef ENOSR
    inscode(d, ds, de, "ENOSR", ENOSR, "Out of streams resources");
#endif
#ifdef ENOMEM
    inscode(d, ds, de, "ENOMEM", ENOMEM, "Out of memory");
#endif
#ifdef ENOTSOCK
    inscode(d, ds, de, "ENOTSOCK", ENOTSOCK, "Socket operation on non-socket");
#else
#ifdef WSAENOTSOCK
    inscode(d, ds, de, "ENOTSOCK", WSAENOTSOCK, "Socket operation on non-socket");
#endif
#endif
#ifdef ESTRPIPE
    inscode(d, ds, de, "ESTRPIPE", ESTRPIPE, "Streams pipe error");
#endif
#ifdef EMLINK
    inscode(d, ds, de, "EMLINK", EMLINK, "Too many links");
#endif
#ifdef ERANGE
    inscode(d, ds, de, "ERANGE", ERANGE, "Math result not representable");
#endif
#ifdef ELIBEXEC
    inscode(d, ds, de, "ELIBEXEC", ELIBEXEC, "Cannot exec a shared library directly");
#endif
#ifdef EL3HLT
    inscode(d, ds, de, "EL3HLT", EL3HLT, "Level 3 halted");
#endif
#ifdef ECONNRESET
    inscode(d, ds, de, "ECONNRESET", ECONNRESET, "Connection reset by peer");
#else
#ifdef WSAECONNRESET
    inscode(d, ds, de, "ECONNRESET", WSAECONNRESET, "Connection reset by peer");
#endif
#endif
#ifdef EADDRINUSE
    inscode(d, ds, de, "EADDRINUSE", EADDRINUSE, "Address already in use");
#else
#ifdef WSAEADDRINUSE
    inscode(d, ds, de, "EADDRINUSE", WSAEADDRINUSE, "Address already in use");
#endif
#endif
#ifdef EOPNOTSUPP
    inscode(d, ds, de, "EOPNOTSUPP", EOPNOTSUPP, "Operation not supported on transport endpoint");
#else
#ifdef WSAEOPNOTSUPP
    inscode(d, ds, de, "EOPNOTSUPP", WSAEOPNOTSUPP, "Operation not supported on transport endpoint");
#endif
#endif
#ifdef EREMCHG
    inscode(d, ds, de, "EREMCHG", EREMCHG, "Remote address changed");
#endif
#ifdef EAGAIN
    inscode(d, ds, de, "EAGAIN", EAGAIN, "Try again");
#endif
#ifdef ENAMETOOLONG
    inscode(d, ds, de, "ENAMETOOLONG", ENAMETOOLONG, "File name too long");
#else
#ifdef WSAENAMETOOLONG
    inscode(d, ds, de, "ENAMETOOLONG", WSAENAMETOOLONG, "File name too long");
#endif
#endif
#ifdef ENOTTY
    inscode(d, ds, de, "ENOTTY", ENOTTY, "Not a typewriter");
#endif
#ifdef ERESTART
    inscode(d, ds, de, "ERESTART", ERESTART, "Interrupted system call should be restarted");
#endif
#ifdef ESOCKTNOSUPPORT
    inscode(d, ds, de, "ESOCKTNOSUPPORT", ESOCKTNOSUPPORT, "Socket type not supported");
#else
#ifdef WSAESOCKTNOSUPPORT
    inscode(d, ds, de, "ESOCKTNOSUPPORT", WSAESOCKTNOSUPPORT, "Socket type not supported");
#endif
#endif
#ifdef ETIME
    inscode(d, ds, de, "ETIME", ETIME, "Timer expired");
#endif
#ifdef EBFONT
    inscode(d, ds, de, "EBFONT", EBFONT, "Bad font file format");
#endif
#ifdef EDEADLOCK
    inscode(d, ds, de, "EDEADLOCK", EDEADLOCK, "Error EDEADLOCK");
#endif
#ifdef ETOOMANYREFS
    inscode(d, ds, de, "ETOOMANYREFS", ETOOMANYREFS, "Too many references: cannot splice");
#else
#ifdef WSAETOOMANYREFS
    inscode(d, ds, de, "ETOOMANYREFS", WSAETOOMANYREFS, "Too many references: cannot splice");
#endif
#endif
#ifdef EMFILE
    inscode(d, ds, de, "EMFILE", EMFILE, "Too many open files");
#else
#ifdef WSAEMFILE
    inscode(d, ds, de, "EMFILE", WSAEMFILE, "Too many open files");
#endif
#endif
#ifdef ETXTBSY
    inscode(d, ds, de, "ETXTBSY", ETXTBSY, "Text file busy");
#endif
#ifdef EINPROGRESS
    inscode(d, ds, de, "EINPROGRESS", EINPROGRESS, "Operation now in progress");
#else
#ifdef WSAEINPROGRESS
    inscode(d, ds, de, "EINPROGRESS", WSAEINPROGRESS, "Operation now in progress");
#endif
#endif
#ifdef ENXIO
    inscode(d, ds, de, "ENXIO", ENXIO, "No such device or address");
#endif
#ifdef ENOPKG
    inscode(d, ds, de, "ENOPKG", ENOPKG, "Package not installed");
#endif
#ifdef WSASY
    inscode(d, ds, de, "WSASY", WSASY, "Error WSASY");
#endif
#ifdef WSAEHOSTDOWN
    inscode(d, ds, de, "WSAEHOSTDOWN", WSAEHOSTDOWN, "Host is down");
#endif
#ifdef WSAENETDOWN
    inscode(d, ds, de, "WSAENETDOWN", WSAENETDOWN, "Network is down");
#endif
#ifdef WSAENOTSOCK
    inscode(d, ds, de, "WSAENOTSOCK", WSAENOTSOCK, "Socket operation on non-socket");
#endif
#ifdef WSAEHOSTUNREACH
    inscode(d, ds, de, "WSAEHOSTUNREACH", WSAEHOSTUNREACH, "No route to host");
#endif
#ifdef WSAELOOP
    inscode(d, ds, de, "WSAELOOP", WSAELOOP, "Too many symbolic links encountered");
#endif
#ifdef WSAEMFILE
    inscode(d, ds, de, "WSAEMFILE", WSAEMFILE, "Too many open files");
#endif
#ifdef WSAESTALE
    inscode(d, ds, de, "WSAESTALE", WSAESTALE, "Stale NFS file handle");
#endif
#ifdef WSAVERNOTSUPPORTED
    inscode(d, ds, de, "WSAVERNOTSUPPORTED", WSAVERNOTSUPPORTED, "Error WSAVERNOTSUPPORTED");
#endif
#ifdef WSAENETUNREACH
    inscode(d, ds, de, "WSAENETUNREACH", WSAENETUNREACH, "Network is unreachable");
#endif
#ifdef WSAEPROCLIM
    inscode(d, ds, de, "WSAEPROCLIM", WSAEPROCLIM, "Error WSAEPROCLIM");
#endif
#ifdef WSAEFAULT
    inscode(d, ds, de, "WSAEFAULT", WSAEFAULT, "Bad address");
#endif
#ifdef WSANOTINITIALISED
    inscode(d, ds, de, "WSANOTINITIALISED", WSANOTINITIALISED, "Error WSANOTINITIALISED");
#endif
#ifdef WSAEUSERS
    inscode(d, ds, de, "WSAEUSERS", WSAEUSERS, "Too many users");
#endif
#ifdef WSAMAKEASYNCREPL
    inscode(d, ds, de, "WSAMAKEASYNCREPL", WSAMAKEASYNCREPL, "Error WSAMAKEASYNCREPL");
#endif
#ifdef WSAENOPROTOOPT
    inscode(d, ds, de, "WSAENOPROTOOPT", WSAENOPROTOOPT, "Protocol not available");
#endif
#ifdef WSAECONNABORTED
    inscode(d, ds, de, "WSAECONNABORTED", WSAECONNABORTED, "Software caused connection abort");
#endif
#ifdef WSAENAMETOOLONG
    inscode(d, ds, de, "WSAENAMETOOLONG", WSAENAMETOOLONG, "File name too long");
#endif
#ifdef WSAENOTEMPTY
    inscode(d, ds, de, "WSAENOTEMPTY", WSAENOTEMPTY, "Directory not empty");
#endif
#ifdef WSAESHUTDOWN
    inscode(d, ds, de, "WSAESHUTDOWN", WSAESHUTDOWN, "Cannot send after transport endpoint shutdown");
#endif
#ifdef WSAEAFNOSUPPORT
    inscode(d, ds, de, "WSAEAFNOSUPPORT", WSAEAFNOSUPPORT, "Address family not supported by protocol");
#endif
#ifdef WSAETOOMANYREFS
    inscode(d, ds, de, "WSAETOOMANYREFS", WSAETOOMANYREFS, "Too many references: cannot splice");
#endif
#ifdef WSAEACCES
    inscode(d, ds, de, "WSAEACCES", WSAEACCES, "Permission denied");
#endif
#ifdef WSATR
    inscode(d, ds, de, "WSATR", WSATR, "Error WSATR");
#endif
#ifdef WSABASEERR
    inscode(d, ds, de, "WSABASEERR", WSABASEERR, "Error WSABASEERR");
#endif
#ifdef WSADESCRIPTIO
    inscode(d, ds, de, "WSADESCRIPTIO", WSADESCRIPTIO, "Error WSADESCRIPTIO");
#endif
#ifdef WSAEMSGSIZE
    inscode(d, ds, de, "WSAEMSGSIZE", WSAEMSGSIZE, "Message too long");
#endif
#ifdef WSAEBADF
    inscode(d, ds, de, "WSAEBADF", WSAEBADF, "Bad file number");
#endif
#ifdef WSAECONNRESET
    inscode(d, ds, de, "WSAECONNRESET", WSAECONNRESET, "Connection reset by peer");
#endif
#ifdef WSAGETSELECTERRO
    inscode(d, ds, de, "WSAGETSELECTERRO", WSAGETSELECTERRO, "Error WSAGETSELECTERRO");
#endif
#ifdef WSAETIMEDOUT
    inscode(d, ds, de, "WSAETIMEDOUT", WSAETIMEDOUT, "Connection timed out");
#endif
#ifdef WSAENOBUFS
    inscode(d, ds, de, "WSAENOBUFS", WSAENOBUFS, "No buffer space available");
#endif
#ifdef WSAEDISCON
    inscode(d, ds, de, "WSAEDISCON", WSAEDISCON, "Error WSAEDISCON");
#endif
#ifdef WSAEINTR
    inscode(d, ds, de, "WSAEINTR", WSAEINTR, "Interrupted system call");
#endif
#ifdef WSAEPROTOTYPE
    inscode(d, ds, de, "WSAEPROTOTYPE", WSAEPROTOTYPE, "Protocol wrong type for socket");
#endif
#ifdef WSAHOS
    inscode(d, ds, de, "WSAHOS", WSAHOS, "Error WSAHOS");
#endif
#ifdef WSAEADDRINUSE
    inscode(d, ds, de, "WSAEADDRINUSE", WSAEADDRINUSE, "Address already in use");
#endif
#ifdef WSAEADDRNOTAVAIL
    inscode(d, ds, de, "WSAEADDRNOTAVAIL", WSAEADDRNOTAVAIL, "Cannot assign requested address");
#endif
#ifdef WSAEALREADY
    inscode(d, ds, de, "WSAEALREADY", WSAEALREADY, "Operation already in progress");
#endif
#ifdef WSAEPROTONOSUPPORT
    inscode(d, ds, de, "WSAEPROTONOSUPPORT", WSAEPROTONOSUPPORT, "Protocol not supported");
#endif
#ifdef WSASYSNOTREADY
    inscode(d, ds, de, "WSASYSNOTREADY", WSASYSNOTREADY, "Error WSASYSNOTREADY");
#endif
#ifdef WSAEWOULDBLOCK
    inscode(d, ds, de, "WSAEWOULDBLOCK", WSAEWOULDBLOCK, "Operation would block");
#endif
#ifdef WSAEPFNOSUPPORT
    inscode(d, ds, de, "WSAEPFNOSUPPORT", WSAEPFNOSUPPORT, "Protocol family not supported");
#endif
#ifdef WSAEOPNOTSUPP
    inscode(d, ds, de, "WSAEOPNOTSUPP", WSAEOPNOTSUPP, "Operation not supported on transport endpoint");
#endif
#ifdef WSAEISCONN
    inscode(d, ds, de, "WSAEISCONN", WSAEISCONN, "Transport endpoint is already connected");
#endif
#ifdef WSAEDQUOT
    inscode(d, ds, de, "WSAEDQUOT", WSAEDQUOT, "Quota exceeded");
#endif
#ifdef WSAENOTCONN
    inscode(d, ds, de, "WSAENOTCONN", WSAENOTCONN, "Transport endpoint is not connected");
#endif
#ifdef WSAEREMOTE
    inscode(d, ds, de, "WSAEREMOTE", WSAEREMOTE, "Object is remote");
#endif
#ifdef WSAEINVAL
    inscode(d, ds, de, "WSAEINVAL", WSAEINVAL, "Invalid argument");
#endif
#ifdef WSAEINPROGRESS
    inscode(d, ds, de, "WSAEINPROGRESS", WSAEINPROGRESS, "Operation now in progress");
#endif
#ifdef WSAGETSELECTEVEN
    inscode(d, ds, de, "WSAGETSELECTEVEN", WSAGETSELECTEVEN, "Error WSAGETSELECTEVEN");
#endif
#ifdef WSAESOCKTNOSUPPORT
    inscode(d, ds, de, "WSAESOCKTNOSUPPORT", WSAESOCKTNOSUPPORT, "Socket type not supported");
#endif
#ifdef WSAGETASYNCERRO
    inscode(d, ds, de, "WSAGETASYNCERRO", WSAGETASYNCERRO, "Error WSAGETASYNCERRO");
#endif
#ifdef WSAMAKESELECTREPL
    inscode(d, ds, de, "WSAMAKESELECTREPL", WSAMAKESELECTREPL, "Error WSAMAKESELECTREPL");
#endif
#ifdef WSAGETASYNCBUFLE
    inscode(d, ds, de, "WSAGETASYNCBUFLE", WSAGETASYNCBUFLE, "Error WSAGETASYNCBUFLE");
#endif
#ifdef WSAEDESTADDRREQ
    inscode(d, ds, de, "WSAEDESTADDRREQ", WSAEDESTADDRREQ, "Destination address required");
#endif
#ifdef WSAECONNREFUSED
    inscode(d, ds, de, "WSAECONNREFUSED", WSAECONNREFUSED, "Connection refused");
#endif
#ifdef WSAENETRESET
    inscode(d, ds, de, "WSAENETRESET", WSAENETRESET, "Network dropped connection because of reset");
#endif
#ifdef WSAN
    inscode(d, ds, de, "WSAN", WSAN, "Error WSAN");
#endif
#ifdef ENOMEDIUM
    inscode(d, ds, de, "ENOMEDIUM", ENOMEDIUM, "No medium found");
#endif
#ifdef EMEDIUMTYPE
    inscode(d, ds, de, "EMEDIUMTYPE", EMEDIUMTYPE, "Wrong medium type");
#endif
#ifdef ECANCELED
    inscode(d, ds, de, "ECANCELED", ECANCELED, "Operation Canceled");
#endif
#ifdef ENOKEY
    inscode(d, ds, de, "ENOKEY", ENOKEY, "Required key not available");
#endif
#ifdef EKEYEXPIRED
    inscode(d, ds, de, "EKEYEXPIRED", EKEYEXPIRED, "Key has expired");
#endif
#ifdef EKEYREVOKED
    inscode(d, ds, de, "EKEYREVOKED", EKEYREVOKED, "Key has been revoked");
#endif
#ifdef EKEYREJECTED
    inscode(d, ds, de, "EKEYREJECTED", EKEYREJECTED, "Key was rejected by service");
#endif
#ifdef EOWNERDEAD
    inscode(d, ds, de, "EOWNERDEAD", EOWNERDEAD, "Owner died");
#endif
#ifdef ENOTRECOVERABLE
    inscode(d, ds, de, "ENOTRECOVERABLE", ENOTRECOVERABLE, "State not recoverable");
#endif
#ifdef ERFKILL
    inscode(d, ds, de, "ERFKILL", ERFKILL, "Operation not possible due to RF-kill");
#endif

    /* Solaris-specific errnos */
#ifdef ECANCELED
    inscode(d, ds, de, "ECANCELED", ECANCELED, "Operation canceled");
#endif
#ifdef ENOTSUP
    inscode(d, ds, de, "ENOTSUP", ENOTSUP, "Operation not supported");
#endif
#ifdef EOWNERDEAD
    inscode(d, ds, de, "EOWNERDEAD", EOWNERDEAD, "Process died with the lock");
#endif
#ifdef ENOTRECOVERABLE
    inscode(d, ds, de, "ENOTRECOVERABLE", ENOTRECOVERABLE, "Lock is not recoverable");
#endif
#ifdef ELOCKUNMAPPED
    inscode(d, ds, de, "ELOCKUNMAPPED", ELOCKUNMAPPED, "Locked lock was unmapped");
#endif
#ifdef ENOTACTIVE
    inscode(d, ds, de, "ENOTACTIVE", ENOTACTIVE, "Facility is not active");
#endif

    /* MacOSX specific errnos */
#ifdef EAUTH
    inscode(d, ds, de, "EAUTH", EAUTH, "Authentication error");
#endif
#ifdef EBADARCH
    inscode(d, ds, de, "EBADARCH", EBADARCH, "Bad CPU type in executable");
#endif
#ifdef EBADEXEC
    inscode(d, ds, de, "EBADEXEC", EBADEXEC, "Bad executable (or shared library)");
#endif
#ifdef EBADMACHO
    inscode(d, ds, de, "EBADMACHO", EBADMACHO, "Malformed Mach-o file");
#endif
#ifdef EBADRPC
    inscode(d, ds, de, "EBADRPC", EBADRPC, "RPC struct is bad");
#endif
#ifdef EDEVERR
    inscode(d, ds, de, "EDEVERR", EDEVERR, "Device error");
#endif
#ifdef EFTYPE
    inscode(d, ds, de, "EFTYPE", EFTYPE, "Inappropriate file type or format");
#endif
#ifdef ENEEDAUTH
    inscode(d, ds, de, "ENEEDAUTH", ENEEDAUTH, "Need authenticator");
#endif
#ifdef ENOATTR
    inscode(d, ds, de, "ENOATTR", ENOATTR, "Attribute not found");
#endif
#ifdef ENOPOLICY
    inscode(d, ds, de, "ENOPOLICY", ENOPOLICY, "Policy not found");
#endif
#ifdef EPROCLIM
    inscode(d, ds, de, "EPROCLIM", EPROCLIM, "Too many processes");
#endif
#ifdef EPROCUNAVAIL
    inscode(d, ds, de, "EPROCUNAVAIL", EPROCUNAVAIL, "Bad procedure for program");
#endif
#ifdef EPROGMISMATCH
    inscode(d, ds, de, "EPROGMISMATCH", EPROGMISMATCH, "Program version wrong");
#endif
#ifdef EPROGUNAVAIL
    inscode(d, ds, de, "EPROGUNAVAIL", EPROGUNAVAIL, "RPC prog. not avail");
#endif
#ifdef EPWROFF
    inscode(d, ds, de, "EPWROFF", EPWROFF, "Device power is off");
#endif
#ifdef ERPCMISMATCH
    inscode(d, ds, de, "ERPCMISMATCH", ERPCMISMATCH, "RPC version wrong");
#endif
#ifdef ESHLIBVERS
    inscode(d, ds, de, "ESHLIBVERS", ESHLIBVERS, "Shared library version mismatch");
#endif

    Py_DECREF(de);
    return m;
}
