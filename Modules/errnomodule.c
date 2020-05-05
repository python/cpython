
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

static int
errno_exec(PyObject *module)
{
    PyObject *module_dict = PyModule_GetDict(module);
    PyObject *error_dict = PyDict_New();
    if (!module_dict || !error_dict || PyDict_SetItemString(module_dict, "errorcode", error_dict) < 0) {
        return -1;
    }

/* Macro so I don't have to edit each and every line below... */
#define inscode(module_dict, error_dict, name, code, comment) _inscode(module_dict, error_dict, name, code)

    /*
     * The names and comments are borrowed from linux/include/errno.h,
     * which should be pretty all-inclusive.  However, the Solaris specific
     * names and comments are borrowed from sys/errno.h in Solaris.
     * MacOSX specific names and comments are borrowed from sys/errno.h in
     * MacOSX.
     */

#ifdef ENODEV
    inscode(module_dict, error_dict, "ENODEV", ENODEV, "No such device");
#endif
#ifdef ENOCSI
    inscode(module_dict, error_dict, "ENOCSI", ENOCSI, "No CSI structure available");
#endif
#ifdef EHOSTUNREACH
    inscode(module_dict, error_dict, "EHOSTUNREACH", EHOSTUNREACH, "No route to host");
#else
#ifdef WSAEHOSTUNREACH
    inscode(module_dict, error_dict, "EHOSTUNREACH", WSAEHOSTUNREACH, "No route to host");
#endif
#endif
#ifdef ENOMSG
    inscode(module_dict, error_dict, "ENOMSG", ENOMSG, "No message of desired type");
#endif
#ifdef EUCLEAN
    inscode(module_dict, error_dict, "EUCLEAN", EUCLEAN, "Structure needs cleaning");
#endif
#ifdef EL2NSYNC
    inscode(module_dict, error_dict, "EL2NSYNC", EL2NSYNC, "Level 2 not synchronized");
#endif
#ifdef EL2HLT
    inscode(module_dict, error_dict, "EL2HLT", EL2HLT, "Level 2 halted");
#endif
#ifdef ENODATA
    inscode(module_dict, error_dict, "ENODATA", ENODATA, "No data available");
#endif
#ifdef ENOTBLK
    inscode(module_dict, error_dict, "ENOTBLK", ENOTBLK, "Block device required");
#endif
#ifdef ENOSYS
    inscode(module_dict, error_dict, "ENOSYS", ENOSYS, "Function not implemented");
#endif
#ifdef EPIPE
    inscode(module_dict, error_dict, "EPIPE", EPIPE, "Broken pipe");
#endif
#ifdef EINVAL
    inscode(module_dict, error_dict, "EINVAL", EINVAL, "Invalid argument");
#else
#ifdef WSAEINVAL
    inscode(module_dict, error_dict, "EINVAL", WSAEINVAL, "Invalid argument");
#endif
#endif
#ifdef EOVERFLOW
    inscode(module_dict, error_dict, "EOVERFLOW", EOVERFLOW, "Value too large for defined data type");
#endif
#ifdef EADV
    inscode(module_dict, error_dict, "EADV", EADV, "Advertise error");
#endif
#ifdef EINTR
    inscode(module_dict, error_dict, "EINTR", EINTR, "Interrupted system call");
#else
#ifdef WSAEINTR
    inscode(module_dict, error_dict, "EINTR", WSAEINTR, "Interrupted system call");
#endif
#endif
#ifdef EUSERS
    inscode(module_dict, error_dict, "EUSERS", EUSERS, "Too many users");
#else
#ifdef WSAEUSERS
    inscode(module_dict, error_dict, "EUSERS", WSAEUSERS, "Too many users");
#endif
#endif
#ifdef ENOTEMPTY
    inscode(module_dict, error_dict, "ENOTEMPTY", ENOTEMPTY, "Directory not empty");
#else
#ifdef WSAENOTEMPTY
    inscode(module_dict, error_dict, "ENOTEMPTY", WSAENOTEMPTY, "Directory not empty");
#endif
#endif
#ifdef ENOBUFS
    inscode(module_dict, error_dict, "ENOBUFS", ENOBUFS, "No buffer space available");
#else
#ifdef WSAENOBUFS
    inscode(module_dict, error_dict, "ENOBUFS", WSAENOBUFS, "No buffer space available");
#endif
#endif
#ifdef EPROTO
    inscode(module_dict, error_dict, "EPROTO", EPROTO, "Protocol error");
#endif
#ifdef EREMOTE
    inscode(module_dict, error_dict, "EREMOTE", EREMOTE, "Object is remote");
#else
#ifdef WSAEREMOTE
    inscode(module_dict, error_dict, "EREMOTE", WSAEREMOTE, "Object is remote");
#endif
#endif
#ifdef ENAVAIL
    inscode(module_dict, error_dict, "ENAVAIL", ENAVAIL, "No XENIX semaphores available");
#endif
#ifdef ECHILD
    inscode(module_dict, error_dict, "ECHILD", ECHILD, "No child processes");
#endif
#ifdef ELOOP
    inscode(module_dict, error_dict, "ELOOP", ELOOP, "Too many symbolic links encountered");
#else
#ifdef WSAELOOP
    inscode(module_dict, error_dict, "ELOOP", WSAELOOP, "Too many symbolic links encountered");
#endif
#endif
#ifdef EXDEV
    inscode(module_dict, error_dict, "EXDEV", EXDEV, "Cross-device link");
#endif
#ifdef E2BIG
    inscode(module_dict, error_dict, "E2BIG", E2BIG, "Arg list too long");
#endif
#ifdef ESRCH
    inscode(module_dict, error_dict, "ESRCH", ESRCH, "No such process");
#endif
#ifdef EMSGSIZE
    inscode(module_dict, error_dict, "EMSGSIZE", EMSGSIZE, "Message too long");
#else
#ifdef WSAEMSGSIZE
    inscode(module_dict, error_dict, "EMSGSIZE", WSAEMSGSIZE, "Message too long");
#endif
#endif
#ifdef EAFNOSUPPORT
    inscode(module_dict, error_dict, "EAFNOSUPPORT", EAFNOSUPPORT, "Address family not supported by protocol");
#else
#ifdef WSAEAFNOSUPPORT
    inscode(module_dict, error_dict, "EAFNOSUPPORT", WSAEAFNOSUPPORT, "Address family not supported by protocol");
#endif
#endif
#ifdef EBADR
    inscode(module_dict, error_dict, "EBADR", EBADR, "Invalid request descriptor");
#endif
#ifdef EHOSTDOWN
    inscode(module_dict, error_dict, "EHOSTDOWN", EHOSTDOWN, "Host is down");
#else
#ifdef WSAEHOSTDOWN
    inscode(module_dict, error_dict, "EHOSTDOWN", WSAEHOSTDOWN, "Host is down");
#endif
#endif
#ifdef EPFNOSUPPORT
    inscode(module_dict, error_dict, "EPFNOSUPPORT", EPFNOSUPPORT, "Protocol family not supported");
#else
#ifdef WSAEPFNOSUPPORT
    inscode(module_dict, error_dict, "EPFNOSUPPORT", WSAEPFNOSUPPORT, "Protocol family not supported");
#endif
#endif
#ifdef ENOPROTOOPT
    inscode(module_dict, error_dict, "ENOPROTOOPT", ENOPROTOOPT, "Protocol not available");
#else
#ifdef WSAENOPROTOOPT
    inscode(module_dict, error_dict, "ENOPROTOOPT", WSAENOPROTOOPT, "Protocol not available");
#endif
#endif
#ifdef EBUSY
    inscode(module_dict, error_dict, "EBUSY", EBUSY, "Device or resource busy");
#endif
#ifdef EWOULDBLOCK
    inscode(module_dict, error_dict, "EWOULDBLOCK", EWOULDBLOCK, "Operation would block");
#else
#ifdef WSAEWOULDBLOCK
    inscode(module_dict, error_dict, "EWOULDBLOCK", WSAEWOULDBLOCK, "Operation would block");
#endif
#endif
#ifdef EBADFD
    inscode(module_dict, error_dict, "EBADFD", EBADFD, "File descriptor in bad state");
#endif
#ifdef EDOTDOT
    inscode(module_dict, error_dict, "EDOTDOT", EDOTDOT, "RFS specific error");
#endif
#ifdef EISCONN
    inscode(module_dict, error_dict, "EISCONN", EISCONN, "Transport endpoint is already connected");
#else
#ifdef WSAEISCONN
    inscode(module_dict, error_dict, "EISCONN", WSAEISCONN, "Transport endpoint is already connected");
#endif
#endif
#ifdef ENOANO
    inscode(module_dict, error_dict, "ENOANO", ENOANO, "No anode");
#endif
#ifdef ESHUTDOWN
    inscode(module_dict, error_dict, "ESHUTDOWN", ESHUTDOWN, "Cannot send after transport endpoint shutdown");
#else
#ifdef WSAESHUTDOWN
    inscode(module_dict, error_dict, "ESHUTDOWN", WSAESHUTDOWN, "Cannot send after transport endpoint shutdown");
#endif
#endif
#ifdef ECHRNG
    inscode(module_dict, error_dict, "ECHRNG", ECHRNG, "Channel number out of range");
#endif
#ifdef ELIBBAD
    inscode(module_dict, error_dict, "ELIBBAD", ELIBBAD, "Accessing a corrupted shared library");
#endif
#ifdef ENONET
    inscode(module_dict, error_dict, "ENONET", ENONET, "Machine is not on the network");
#endif
#ifdef EBADE
    inscode(module_dict, error_dict, "EBADE", EBADE, "Invalid exchange");
#endif
#ifdef EBADF
    inscode(module_dict, error_dict, "EBADF", EBADF, "Bad file number");
#else
#ifdef WSAEBADF
    inscode(module_dict, error_dict, "EBADF", WSAEBADF, "Bad file number");
#endif
#endif
#ifdef EMULTIHOP
    inscode(module_dict, error_dict, "EMULTIHOP", EMULTIHOP, "Multihop attempted");
#endif
#ifdef EIO
    inscode(module_dict, error_dict, "EIO", EIO, "I/O error");
#endif
#ifdef EUNATCH
    inscode(module_dict, error_dict, "EUNATCH", EUNATCH, "Protocol driver not attached");
#endif
#ifdef EPROTOTYPE
    inscode(module_dict, error_dict, "EPROTOTYPE", EPROTOTYPE, "Protocol wrong type for socket");
#else
#ifdef WSAEPROTOTYPE
    inscode(module_dict, error_dict, "EPROTOTYPE", WSAEPROTOTYPE, "Protocol wrong type for socket");
#endif
#endif
#ifdef ENOSPC
    inscode(module_dict, error_dict, "ENOSPC", ENOSPC, "No space left on device");
#endif
#ifdef ENOEXEC
    inscode(module_dict, error_dict, "ENOEXEC", ENOEXEC, "Exec format error");
#endif
#ifdef EALREADY
    inscode(module_dict, error_dict, "EALREADY", EALREADY, "Operation already in progress");
#else
#ifdef WSAEALREADY
    inscode(module_dict, error_dict, "EALREADY", WSAEALREADY, "Operation already in progress");
#endif
#endif
#ifdef ENETDOWN
    inscode(module_dict, error_dict, "ENETDOWN", ENETDOWN, "Network is down");
#else
#ifdef WSAENETDOWN
    inscode(module_dict, error_dict, "ENETDOWN", WSAENETDOWN, "Network is down");
#endif
#endif
#ifdef ENOTNAM
    inscode(module_dict, error_dict, "ENOTNAM", ENOTNAM, "Not a XENIX named type file");
#endif
#ifdef EACCES
    inscode(module_dict, error_dict, "EACCES", EACCES, "Permission denied");
#else
#ifdef WSAEACCES
    inscode(module_dict, error_dict, "EACCES", WSAEACCES, "Permission denied");
#endif
#endif
#ifdef ELNRNG
    inscode(module_dict, error_dict, "ELNRNG", ELNRNG, "Link number out of range");
#endif
#ifdef EILSEQ
    inscode(module_dict, error_dict, "EILSEQ", EILSEQ, "Illegal byte sequence");
#endif
#ifdef ENOTDIR
    inscode(module_dict, error_dict, "ENOTDIR", ENOTDIR, "Not a directory");
#endif
#ifdef ENOTUNIQ
    inscode(module_dict, error_dict, "ENOTUNIQ", ENOTUNIQ, "Name not unique on network");
#endif
#ifdef EPERM
    inscode(module_dict, error_dict, "EPERM", EPERM, "Operation not permitted");
#endif
#ifdef EDOM
    inscode(module_dict, error_dict, "EDOM", EDOM, "Math argument out of domain of func");
#endif
#ifdef EXFULL
    inscode(module_dict, error_dict, "EXFULL", EXFULL, "Exchange full");
#endif
#ifdef ECONNREFUSED
    inscode(module_dict, error_dict, "ECONNREFUSED", ECONNREFUSED, "Connection refused");
#else
#ifdef WSAECONNREFUSED
    inscode(module_dict, error_dict, "ECONNREFUSED", WSAECONNREFUSED, "Connection refused");
#endif
#endif
#ifdef EISDIR
    inscode(module_dict, error_dict, "EISDIR", EISDIR, "Is a directory");
#endif
#ifdef EPROTONOSUPPORT
    inscode(module_dict, error_dict, "EPROTONOSUPPORT", EPROTONOSUPPORT, "Protocol not supported");
#else
#ifdef WSAEPROTONOSUPPORT
    inscode(module_dict, error_dict, "EPROTONOSUPPORT", WSAEPROTONOSUPPORT, "Protocol not supported");
#endif
#endif
#ifdef EROFS
    inscode(module_dict, error_dict, "EROFS", EROFS, "Read-only file system");
#endif
#ifdef EADDRNOTAVAIL
    inscode(module_dict, error_dict, "EADDRNOTAVAIL", EADDRNOTAVAIL, "Cannot assign requested address");
#else
#ifdef WSAEADDRNOTAVAIL
    inscode(module_dict, error_dict, "EADDRNOTAVAIL", WSAEADDRNOTAVAIL, "Cannot assign requested address");
#endif
#endif
#ifdef EIDRM
    inscode(module_dict, error_dict, "EIDRM", EIDRM, "Identifier removed");
#endif
#ifdef ECOMM
    inscode(module_dict, error_dict, "ECOMM", ECOMM, "Communication error on send");
#endif
#ifdef ESRMNT
    inscode(module_dict, error_dict, "ESRMNT", ESRMNT, "Srmount error");
#endif
#ifdef EREMOTEIO
    inscode(module_dict, error_dict, "EREMOTEIO", EREMOTEIO, "Remote I/O error");
#endif
#ifdef EL3RST
    inscode(module_dict, error_dict, "EL3RST", EL3RST, "Level 3 reset");
#endif
#ifdef EBADMSG
    inscode(module_dict, error_dict, "EBADMSG", EBADMSG, "Not a data message");
#endif
#ifdef ENFILE
    inscode(module_dict, error_dict, "ENFILE", ENFILE, "File table overflow");
#endif
#ifdef ELIBMAX
    inscode(module_dict, error_dict, "ELIBMAX", ELIBMAX, "Attempting to link in too many shared libraries");
#endif
#ifdef ESPIPE
    inscode(module_dict, error_dict, "ESPIPE", ESPIPE, "Illegal seek");
#endif
#ifdef ENOLINK
    inscode(module_dict, error_dict, "ENOLINK", ENOLINK, "Link has been severed");
#endif
#ifdef ENETRESET
    inscode(module_dict, error_dict, "ENETRESET", ENETRESET, "Network dropped connection because of reset");
#else
#ifdef WSAENETRESET
    inscode(module_dict, error_dict, "ENETRESET", WSAENETRESET, "Network dropped connection because of reset");
#endif
#endif
#ifdef ETIMEDOUT
    inscode(module_dict, error_dict, "ETIMEDOUT", ETIMEDOUT, "Connection timed out");
#else
#ifdef WSAETIMEDOUT
    inscode(module_dict, error_dict, "ETIMEDOUT", WSAETIMEDOUT, "Connection timed out");
#endif
#endif
#ifdef ENOENT
    inscode(module_dict, error_dict, "ENOENT", ENOENT, "No such file or directory");
#endif
#ifdef EEXIST
    inscode(module_dict, error_dict, "EEXIST", EEXIST, "File exists");
#endif
#ifdef EDQUOT
    inscode(module_dict, error_dict, "EDQUOT", EDQUOT, "Quota exceeded");
#else
#ifdef WSAEDQUOT
    inscode(module_dict, error_dict, "EDQUOT", WSAEDQUOT, "Quota exceeded");
#endif
#endif
#ifdef ENOSTR
    inscode(module_dict, error_dict, "ENOSTR", ENOSTR, "Device not a stream");
#endif
#ifdef EBADSLT
    inscode(module_dict, error_dict, "EBADSLT", EBADSLT, "Invalid slot");
#endif
#ifdef EBADRQC
    inscode(module_dict, error_dict, "EBADRQC", EBADRQC, "Invalid request code");
#endif
#ifdef ELIBACC
    inscode(module_dict, error_dict, "ELIBACC", ELIBACC, "Can not access a needed shared library");
#endif
#ifdef EFAULT
    inscode(module_dict, error_dict, "EFAULT", EFAULT, "Bad address");
#else
#ifdef WSAEFAULT
    inscode(module_dict, error_dict, "EFAULT", WSAEFAULT, "Bad address");
#endif
#endif
#ifdef EFBIG
    inscode(module_dict, error_dict, "EFBIG", EFBIG, "File too large");
#endif
#ifdef EDEADLK
    inscode(module_dict, error_dict, "EDEADLK", EDEADLK, "Resource deadlock would occur");
#endif
#ifdef ENOTCONN
    inscode(module_dict, error_dict, "ENOTCONN", ENOTCONN, "Transport endpoint is not connected");
#else
#ifdef WSAENOTCONN
    inscode(module_dict, error_dict, "ENOTCONN", WSAENOTCONN, "Transport endpoint is not connected");
#endif
#endif
#ifdef EDESTADDRREQ
    inscode(module_dict, error_dict, "EDESTADDRREQ", EDESTADDRREQ, "Destination address required");
#else
#ifdef WSAEDESTADDRREQ
    inscode(module_dict, error_dict, "EDESTADDRREQ", WSAEDESTADDRREQ, "Destination address required");
#endif
#endif
#ifdef ELIBSCN
    inscode(module_dict, error_dict, "ELIBSCN", ELIBSCN, ".lib section in a.out corrupted");
#endif
#ifdef ENOLCK
    inscode(module_dict, error_dict, "ENOLCK", ENOLCK, "No record locks available");
#endif
#ifdef EISNAM
    inscode(module_dict, error_dict, "EISNAM", EISNAM, "Is a named type file");
#endif
#ifdef ECONNABORTED
    inscode(module_dict, error_dict, "ECONNABORTED", ECONNABORTED, "Software caused connection abort");
#else
#ifdef WSAECONNABORTED
    inscode(module_dict, error_dict, "ECONNABORTED", WSAECONNABORTED, "Software caused connection abort");
#endif
#endif
#ifdef ENETUNREACH
    inscode(module_dict, error_dict, "ENETUNREACH", ENETUNREACH, "Network is unreachable");
#else
#ifdef WSAENETUNREACH
    inscode(module_dict, error_dict, "ENETUNREACH", WSAENETUNREACH, "Network is unreachable");
#endif
#endif
#ifdef ESTALE
    inscode(module_dict, error_dict, "ESTALE", ESTALE, "Stale NFS file handle");
#else
#ifdef WSAESTALE
    inscode(module_dict, error_dict, "ESTALE", WSAESTALE, "Stale NFS file handle");
#endif
#endif
#ifdef ENOSR
    inscode(module_dict, error_dict, "ENOSR", ENOSR, "Out of streams resources");
#endif
#ifdef ENOMEM
    inscode(module_dict, error_dict, "ENOMEM", ENOMEM, "Out of memory");
#endif
#ifdef ENOTSOCK
    inscode(module_dict, error_dict, "ENOTSOCK", ENOTSOCK, "Socket operation on non-socket");
#else
#ifdef WSAENOTSOCK
    inscode(module_dict, error_dict, "ENOTSOCK", WSAENOTSOCK, "Socket operation on non-socket");
#endif
#endif
#ifdef ESTRPIPE
    inscode(module_dict, error_dict, "ESTRPIPE", ESTRPIPE, "Streams pipe error");
#endif
#ifdef EMLINK
    inscode(module_dict, error_dict, "EMLINK", EMLINK, "Too many links");
#endif
#ifdef ERANGE
    inscode(module_dict, error_dict, "ERANGE", ERANGE, "Math result not representable");
#endif
#ifdef ELIBEXEC
    inscode(module_dict, error_dict, "ELIBEXEC", ELIBEXEC, "Cannot exec a shared library directly");
#endif
#ifdef EL3HLT
    inscode(module_dict, error_dict, "EL3HLT", EL3HLT, "Level 3 halted");
#endif
#ifdef ECONNRESET
    inscode(module_dict, error_dict, "ECONNRESET", ECONNRESET, "Connection reset by peer");
#else
#ifdef WSAECONNRESET
    inscode(module_dict, error_dict, "ECONNRESET", WSAECONNRESET, "Connection reset by peer");
#endif
#endif
#ifdef EADDRINUSE
    inscode(module_dict, error_dict, "EADDRINUSE", EADDRINUSE, "Address already in use");
#else
#ifdef WSAEADDRINUSE
    inscode(module_dict, error_dict, "EADDRINUSE", WSAEADDRINUSE, "Address already in use");
#endif
#endif
#ifdef EOPNOTSUPP
    inscode(module_dict, error_dict, "EOPNOTSUPP", EOPNOTSUPP, "Operation not supported on transport endpoint");
#else
#ifdef WSAEOPNOTSUPP
    inscode(module_dict, error_dict, "EOPNOTSUPP", WSAEOPNOTSUPP, "Operation not supported on transport endpoint");
#endif
#endif
#ifdef EREMCHG
    inscode(module_dict, error_dict, "EREMCHG", EREMCHG, "Remote address changed");
#endif
#ifdef EAGAIN
    inscode(module_dict, error_dict, "EAGAIN", EAGAIN, "Try again");
#endif
#ifdef ENAMETOOLONG
    inscode(module_dict, error_dict, "ENAMETOOLONG", ENAMETOOLONG, "File name too long");
#else
#ifdef WSAENAMETOOLONG
    inscode(module_dict, error_dict, "ENAMETOOLONG", WSAENAMETOOLONG, "File name too long");
#endif
#endif
#ifdef ENOTTY
    inscode(module_dict, error_dict, "ENOTTY", ENOTTY, "Not a typewriter");
#endif
#ifdef ERESTART
    inscode(module_dict, error_dict, "ERESTART", ERESTART, "Interrupted system call should be restarted");
#endif
#ifdef ESOCKTNOSUPPORT
    inscode(module_dict, error_dict, "ESOCKTNOSUPPORT", ESOCKTNOSUPPORT, "Socket type not supported");
#else
#ifdef WSAESOCKTNOSUPPORT
    inscode(module_dict, error_dict, "ESOCKTNOSUPPORT", WSAESOCKTNOSUPPORT, "Socket type not supported");
#endif
#endif
#ifdef ETIME
    inscode(module_dict, error_dict, "ETIME", ETIME, "Timer expired");
#endif
#ifdef EBFONT
    inscode(module_dict, error_dict, "EBFONT", EBFONT, "Bad font file format");
#endif
#ifdef EDEADLOCK
    inscode(module_dict, error_dict, "EDEADLOCK", EDEADLOCK, "Error EDEADLOCK");
#endif
#ifdef ETOOMANYREFS
    inscode(module_dict, error_dict, "ETOOMANYREFS", ETOOMANYREFS, "Too many references: cannot splice");
#else
#ifdef WSAETOOMANYREFS
    inscode(module_dict, error_dict, "ETOOMANYREFS", WSAETOOMANYREFS, "Too many references: cannot splice");
#endif
#endif
#ifdef EMFILE
    inscode(module_dict, error_dict, "EMFILE", EMFILE, "Too many open files");
#else
#ifdef WSAEMFILE
    inscode(module_dict, error_dict, "EMFILE", WSAEMFILE, "Too many open files");
#endif
#endif
#ifdef ETXTBSY
    inscode(module_dict, error_dict, "ETXTBSY", ETXTBSY, "Text file busy");
#endif
#ifdef EINPROGRESS
    inscode(module_dict, error_dict, "EINPROGRESS", EINPROGRESS, "Operation now in progress");
#else
#ifdef WSAEINPROGRESS
    inscode(module_dict, error_dict, "EINPROGRESS", WSAEINPROGRESS, "Operation now in progress");
#endif
#endif
#ifdef ENXIO
    inscode(module_dict, error_dict, "ENXIO", ENXIO, "No such device or address");
#endif
#ifdef ENOPKG
    inscode(module_dict, error_dict, "ENOPKG", ENOPKG, "Package not installed");
#endif
#ifdef WSASY
    inscode(module_dict, error_dict, "WSASY", WSASY, "Error WSASY");
#endif
#ifdef WSAEHOSTDOWN
    inscode(module_dict, error_dict, "WSAEHOSTDOWN", WSAEHOSTDOWN, "Host is down");
#endif
#ifdef WSAENETDOWN
    inscode(module_dict, error_dict, "WSAENETDOWN", WSAENETDOWN, "Network is down");
#endif
#ifdef WSAENOTSOCK
    inscode(module_dict, error_dict, "WSAENOTSOCK", WSAENOTSOCK, "Socket operation on non-socket");
#endif
#ifdef WSAEHOSTUNREACH
    inscode(module_dict, error_dict, "WSAEHOSTUNREACH", WSAEHOSTUNREACH, "No route to host");
#endif
#ifdef WSAELOOP
    inscode(module_dict, error_dict, "WSAELOOP", WSAELOOP, "Too many symbolic links encountered");
#endif
#ifdef WSAEMFILE
    inscode(module_dict, error_dict, "WSAEMFILE", WSAEMFILE, "Too many open files");
#endif
#ifdef WSAESTALE
    inscode(module_dict, error_dict, "WSAESTALE", WSAESTALE, "Stale NFS file handle");
#endif
#ifdef WSAVERNOTSUPPORTED
    inscode(module_dict, error_dict, "WSAVERNOTSUPPORTED", WSAVERNOTSUPPORTED, "Error WSAVERNOTSUPPORTED");
#endif
#ifdef WSAENETUNREACH
    inscode(module_dict, error_dict, "WSAENETUNREACH", WSAENETUNREACH, "Network is unreachable");
#endif
#ifdef WSAEPROCLIM
    inscode(module_dict, error_dict, "WSAEPROCLIM", WSAEPROCLIM, "Error WSAEPROCLIM");
#endif
#ifdef WSAEFAULT
    inscode(module_dict, error_dict, "WSAEFAULT", WSAEFAULT, "Bad address");
#endif
#ifdef WSANOTINITIALISED
    inscode(module_dict, error_dict, "WSANOTINITIALISED", WSANOTINITIALISED, "Error WSANOTINITIALISED");
#endif
#ifdef WSAEUSERS
    inscode(module_dict, error_dict, "WSAEUSERS", WSAEUSERS, "Too many users");
#endif
#ifdef WSAMAKEASYNCREPL
    inscode(module_dict, error_dict, "WSAMAKEASYNCREPL", WSAMAKEASYNCREPL, "Error WSAMAKEASYNCREPL");
#endif
#ifdef WSAENOPROTOOPT
    inscode(module_dict, error_dict, "WSAENOPROTOOPT", WSAENOPROTOOPT, "Protocol not available");
#endif
#ifdef WSAECONNABORTED
    inscode(module_dict, error_dict, "WSAECONNABORTED", WSAECONNABORTED, "Software caused connection abort");
#endif
#ifdef WSAENAMETOOLONG
    inscode(module_dict, error_dict, "WSAENAMETOOLONG", WSAENAMETOOLONG, "File name too long");
#endif
#ifdef WSAENOTEMPTY
    inscode(module_dict, error_dict, "WSAENOTEMPTY", WSAENOTEMPTY, "Directory not empty");
#endif
#ifdef WSAESHUTDOWN
    inscode(module_dict, error_dict, "WSAESHUTDOWN", WSAESHUTDOWN, "Cannot send after transport endpoint shutdown");
#endif
#ifdef WSAEAFNOSUPPORT
    inscode(module_dict, error_dict, "WSAEAFNOSUPPORT", WSAEAFNOSUPPORT, "Address family not supported by protocol");
#endif
#ifdef WSAETOOMANYREFS
    inscode(module_dict, error_dict, "WSAETOOMANYREFS", WSAETOOMANYREFS, "Too many references: cannot splice");
#endif
#ifdef WSAEACCES
    inscode(module_dict, error_dict, "WSAEACCES", WSAEACCES, "Permission denied");
#endif
#ifdef WSATR
    inscode(module_dict, error_dict, "WSATR", WSATR, "Error WSATR");
#endif
#ifdef WSABASEERR
    inscode(module_dict, error_dict, "WSABASEERR", WSABASEERR, "Error WSABASEERR");
#endif
#ifdef WSADESCRIPTIO
    inscode(module_dict, error_dict, "WSADESCRIPTIO", WSADESCRIPTIO, "Error WSADESCRIPTIO");
#endif
#ifdef WSAEMSGSIZE
    inscode(module_dict, error_dict, "WSAEMSGSIZE", WSAEMSGSIZE, "Message too long");
#endif
#ifdef WSAEBADF
    inscode(module_dict, error_dict, "WSAEBADF", WSAEBADF, "Bad file number");
#endif
#ifdef WSAECONNRESET
    inscode(module_dict, error_dict, "WSAECONNRESET", WSAECONNRESET, "Connection reset by peer");
#endif
#ifdef WSAGETSELECTERRO
    inscode(module_dict, error_dict, "WSAGETSELECTERRO", WSAGETSELECTERRO, "Error WSAGETSELECTERRO");
#endif
#ifdef WSAETIMEDOUT
    inscode(module_dict, error_dict, "WSAETIMEDOUT", WSAETIMEDOUT, "Connection timed out");
#endif
#ifdef WSAENOBUFS
    inscode(module_dict, error_dict, "WSAENOBUFS", WSAENOBUFS, "No buffer space available");
#endif
#ifdef WSAEDISCON
    inscode(module_dict, error_dict, "WSAEDISCON", WSAEDISCON, "Error WSAEDISCON");
#endif
#ifdef WSAEINTR
    inscode(module_dict, error_dict, "WSAEINTR", WSAEINTR, "Interrupted system call");
#endif
#ifdef WSAEPROTOTYPE
    inscode(module_dict, error_dict, "WSAEPROTOTYPE", WSAEPROTOTYPE, "Protocol wrong type for socket");
#endif
#ifdef WSAHOS
    inscode(module_dict, error_dict, "WSAHOS", WSAHOS, "Error WSAHOS");
#endif
#ifdef WSAEADDRINUSE
    inscode(module_dict, error_dict, "WSAEADDRINUSE", WSAEADDRINUSE, "Address already in use");
#endif
#ifdef WSAEADDRNOTAVAIL
    inscode(module_dict, error_dict, "WSAEADDRNOTAVAIL", WSAEADDRNOTAVAIL, "Cannot assign requested address");
#endif
#ifdef WSAEALREADY
    inscode(module_dict, error_dict, "WSAEALREADY", WSAEALREADY, "Operation already in progress");
#endif
#ifdef WSAEPROTONOSUPPORT
    inscode(module_dict, error_dict, "WSAEPROTONOSUPPORT", WSAEPROTONOSUPPORT, "Protocol not supported");
#endif
#ifdef WSASYSNOTREADY
    inscode(module_dict, error_dict, "WSASYSNOTREADY", WSASYSNOTREADY, "Error WSASYSNOTREADY");
#endif
#ifdef WSAEWOULDBLOCK
    inscode(module_dict, error_dict, "WSAEWOULDBLOCK", WSAEWOULDBLOCK, "Operation would block");
#endif
#ifdef WSAEPFNOSUPPORT
    inscode(module_dict, error_dict, "WSAEPFNOSUPPORT", WSAEPFNOSUPPORT, "Protocol family not supported");
#endif
#ifdef WSAEOPNOTSUPP
    inscode(module_dict, error_dict, "WSAEOPNOTSUPP", WSAEOPNOTSUPP, "Operation not supported on transport endpoint");
#endif
#ifdef WSAEISCONN
    inscode(module_dict, error_dict, "WSAEISCONN", WSAEISCONN, "Transport endpoint is already connected");
#endif
#ifdef WSAEDQUOT
    inscode(module_dict, error_dict, "WSAEDQUOT", WSAEDQUOT, "Quota exceeded");
#endif
#ifdef WSAENOTCONN
    inscode(module_dict, error_dict, "WSAENOTCONN", WSAENOTCONN, "Transport endpoint is not connected");
#endif
#ifdef WSAEREMOTE
    inscode(module_dict, error_dict, "WSAEREMOTE", WSAEREMOTE, "Object is remote");
#endif
#ifdef WSAEINVAL
    inscode(module_dict, error_dict, "WSAEINVAL", WSAEINVAL, "Invalid argument");
#endif
#ifdef WSAEINPROGRESS
    inscode(module_dict, error_dict, "WSAEINPROGRESS", WSAEINPROGRESS, "Operation now in progress");
#endif
#ifdef WSAGETSELECTEVEN
    inscode(module_dict, error_dict, "WSAGETSELECTEVEN", WSAGETSELECTEVEN, "Error WSAGETSELECTEVEN");
#endif
#ifdef WSAESOCKTNOSUPPORT
    inscode(module_dict, error_dict, "WSAESOCKTNOSUPPORT", WSAESOCKTNOSUPPORT, "Socket type not supported");
#endif
#ifdef WSAGETASYNCERRO
    inscode(module_dict, error_dict, "WSAGETASYNCERRO", WSAGETASYNCERRO, "Error WSAGETASYNCERRO");
#endif
#ifdef WSAMAKESELECTREPL
    inscode(module_dict, error_dict, "WSAMAKESELECTREPL", WSAMAKESELECTREPL, "Error WSAMAKESELECTREPL");
#endif
#ifdef WSAGETASYNCBUFLE
    inscode(module_dict, error_dict, "WSAGETASYNCBUFLE", WSAGETASYNCBUFLE, "Error WSAGETASYNCBUFLE");
#endif
#ifdef WSAEDESTADDRREQ
    inscode(module_dict, error_dict, "WSAEDESTADDRREQ", WSAEDESTADDRREQ, "Destination address required");
#endif
#ifdef WSAECONNREFUSED
    inscode(module_dict, error_dict, "WSAECONNREFUSED", WSAECONNREFUSED, "Connection refused");
#endif
#ifdef WSAENETRESET
    inscode(module_dict, error_dict, "WSAENETRESET", WSAENETRESET, "Network dropped connection because of reset");
#endif
#ifdef WSAN
    inscode(module_dict, error_dict, "WSAN", WSAN, "Error WSAN");
#endif
#ifdef ENOMEDIUM
    inscode(module_dict, error_dict, "ENOMEDIUM", ENOMEDIUM, "No medium found");
#endif
#ifdef EMEDIUMTYPE
    inscode(module_dict, error_dict, "EMEDIUMTYPE", EMEDIUMTYPE, "Wrong medium type");
#endif
#ifdef ECANCELED
    inscode(module_dict, error_dict, "ECANCELED", ECANCELED, "Operation Canceled");
#endif
#ifdef ENOKEY
    inscode(module_dict, error_dict, "ENOKEY", ENOKEY, "Required key not available");
#endif
#ifdef EKEYEXPIRED
    inscode(module_dict, error_dict, "EKEYEXPIRED", EKEYEXPIRED, "Key has expired");
#endif
#ifdef EKEYREVOKED
    inscode(module_dict, error_dict, "EKEYREVOKED", EKEYREVOKED, "Key has been revoked");
#endif
#ifdef EKEYREJECTED
    inscode(module_dict, error_dict, "EKEYREJECTED", EKEYREJECTED, "Key was rejected by service");
#endif
#ifdef EOWNERDEAD
    inscode(module_dict, error_dict, "EOWNERDEAD", EOWNERDEAD, "Owner died");
#endif
#ifdef ENOTRECOVERABLE
    inscode(module_dict, error_dict, "ENOTRECOVERABLE", ENOTRECOVERABLE, "State not recoverable");
#endif
#ifdef ERFKILL
    inscode(module_dict, error_dict, "ERFKILL", ERFKILL, "Operation not possible due to RF-kill");
#endif

    /* Solaris-specific errnos */
#ifdef ECANCELED
    inscode(module_dict, error_dict, "ECANCELED", ECANCELED, "Operation canceled");
#endif
#ifdef ENOTSUP
    inscode(module_dict, error_dict, "ENOTSUP", ENOTSUP, "Operation not supported");
#endif
#ifdef EOWNERDEAD
    inscode(module_dict, error_dict, "EOWNERDEAD", EOWNERDEAD, "Process died with the lock");
#endif
#ifdef ENOTRECOVERABLE
    inscode(module_dict, error_dict, "ENOTRECOVERABLE", ENOTRECOVERABLE, "Lock is not recoverable");
#endif
#ifdef ELOCKUNMAPPED
    inscode(module_dict, error_dict, "ELOCKUNMAPPED", ELOCKUNMAPPED, "Locked lock was unmapped");
#endif
#ifdef ENOTACTIVE
    inscode(module_dict, error_dict, "ENOTACTIVE", ENOTACTIVE, "Facility is not active");
#endif

    /* MacOSX specific errnos */
#ifdef EAUTH
    inscode(module_dict, error_dict, "EAUTH", EAUTH, "Authentication error");
#endif
#ifdef EBADARCH
    inscode(module_dict, error_dict, "EBADARCH", EBADARCH, "Bad CPU type in executable");
#endif
#ifdef EBADEXEC
    inscode(module_dict, error_dict, "EBADEXEC", EBADEXEC, "Bad executable (or shared library)");
#endif
#ifdef EBADMACHO
    inscode(module_dict, error_dict, "EBADMACHO", EBADMACHO, "Malformed Mach-o file");
#endif
#ifdef EBADRPC
    inscode(module_dict, error_dict, "EBADRPC", EBADRPC, "RPC struct is bad");
#endif
#ifdef EDEVERR
    inscode(module_dict, error_dict, "EDEVERR", EDEVERR, "Device error");
#endif
#ifdef EFTYPE
    inscode(module_dict, error_dict, "EFTYPE", EFTYPE, "Inappropriate file type or format");
#endif
#ifdef ENEEDAUTH
    inscode(module_dict, error_dict, "ENEEDAUTH", ENEEDAUTH, "Need authenticator");
#endif
#ifdef ENOATTR
    inscode(module_dict, error_dict, "ENOATTR", ENOATTR, "Attribute not found");
#endif
#ifdef ENOPOLICY
    inscode(module_dict, error_dict, "ENOPOLICY", ENOPOLICY, "Policy not found");
#endif
#ifdef EPROCLIM
    inscode(module_dict, error_dict, "EPROCLIM", EPROCLIM, "Too many processes");
#endif
#ifdef EPROCUNAVAIL
    inscode(module_dict, error_dict, "EPROCUNAVAIL", EPROCUNAVAIL, "Bad procedure for program");
#endif
#ifdef EPROGMISMATCH
    inscode(module_dict, error_dict, "EPROGMISMATCH", EPROGMISMATCH, "Program version wrong");
#endif
#ifdef EPROGUNAVAIL
    inscode(module_dict, error_dict, "EPROGUNAVAIL", EPROGUNAVAIL, "RPC prog. not avail");
#endif
#ifdef EPWROFF
    inscode(module_dict, error_dict, "EPWROFF", EPWROFF, "Device power is off");
#endif
#ifdef ERPCMISMATCH
    inscode(module_dict, error_dict, "ERPCMISMATCH", ERPCMISMATCH, "RPC version wrong");
#endif
#ifdef ESHLIBVERS
    inscode(module_dict, error_dict, "ESHLIBVERS", ESHLIBVERS, "Shared library version mismatch");
#endif

    Py_DECREF(error_dict);
    return 0;
}

static PyModuleDef_Slot errno_slots[] = {
    {Py_mod_exec, errno_exec},
    {0, NULL}
};

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
    .m_name = "errno",
    .m_doc = errno__doc__,
    .m_size = 0,
    .m_methods = errno_methods,
    .m_slots = errno_slots,
};

PyMODINIT_FUNC
PyInit_errno(void)
{
    return PyModuleDef_Init(&errnomodule);
}