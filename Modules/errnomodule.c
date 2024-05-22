/* Errno module */

// Need limited C API version 3.13 for Py_mod_gil
#include "pyconfig.h"   // Py_GIL_DISABLED
#ifndef Py_GIL_DISABLED
#  define Py_LIMITED_API 0x030d0000
#endif

#include "Python.h"
#include <errno.h>                // EPIPE

/* Windows socket errors (WSA*)  */
#ifdef MS_WINDOWS
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>

   // The following constants were added to errno.h in VS2010 but have
   // preferred WSA equivalents.
#  undef EADDRINUSE
#  undef EADDRNOTAVAIL
#  undef EAFNOSUPPORT
#  undef EALREADY
#  undef ECONNABORTED
#  undef ECONNREFUSED
#  undef ECONNRESET
#  undef EDESTADDRREQ
#  undef EHOSTUNREACH
#  undef EINPROGRESS
#  undef EISCONN
#  undef ELOOP
#  undef EMSGSIZE
#  undef ENETDOWN
#  undef ENETRESET
#  undef ENETUNREACH
#  undef ENOBUFS
#  undef ENOPROTOOPT
#  undef ENOTCONN
#  undef ENOTSOCK
#  undef EOPNOTSUPP
#  undef EPROTONOSUPPORT
#  undef EPROTOTYPE
#  undef ETIMEDOUT
#  undef EWOULDBLOCK
#endif

/*
 * Pull in the system error definitions
 */

static PyMethodDef errno_methods[] = {
    {NULL,              NULL}
};

/* Helper function doing the dictionary inserting */

static int
_add_errcode(PyObject *module_dict, PyObject *error_dict, const char *name_str, int code_int)
{
    PyObject *name = PyUnicode_FromString(name_str);
    if (!name) {
        return -1;
    }

    PyObject *code = PyLong_FromLong(code_int);
    if (!code) {
        Py_DECREF(name);
        return -1;
    }

    int ret = -1;
    /* insert in modules dict */
    if (PyDict_SetItem(module_dict, name, code) < 0) {
        goto end;
    }
    /* insert in errorcode dict */
    if (PyDict_SetItem(error_dict, code, name) < 0) {
        goto end;
    }
    ret = 0;
end:
    Py_DECREF(name);
    Py_DECREF(code);
    return ret;
}

static int
errno_exec(PyObject *module)
{
    PyObject *module_dict = PyModule_GetDict(module);  // Borrowed ref.
    if (module_dict == NULL) {
        return -1;
    }
    PyObject *error_dict = PyDict_New();
    if (error_dict == NULL) {
        return -1;
    }
    if (PyDict_SetItemString(module_dict, "errorcode", error_dict) < 0) {
        Py_DECREF(error_dict);
        return -1;
    }

/* Macro so I don't have to edit each and every line below... */
#define add_errcode(name, code, comment)                               \
    do {                                                               \
        if (_add_errcode(module_dict, error_dict, name, code) < 0) {   \
            Py_DECREF(error_dict);                                     \
            return -1;                                                 \
        }                                                              \
    } while (0);

    /*
     * The names and comments are borrowed from linux/include/errno.h,
     * which should be pretty all-inclusive.  However, the Solaris specific
     * names and comments are borrowed from sys/errno.h in Solaris.
     * MacOSX specific names and comments are borrowed from sys/errno.h in
     * MacOSX.
     */

#ifdef ENODEV
    add_errcode("ENODEV", ENODEV, "No such device");
#endif
#ifdef ENOCSI
    add_errcode("ENOCSI", ENOCSI, "No CSI structure available");
#endif
#ifdef EHOSTUNREACH
    add_errcode("EHOSTUNREACH", EHOSTUNREACH, "No route to host");
#else
#ifdef WSAEHOSTUNREACH
    add_errcode("EHOSTUNREACH", WSAEHOSTUNREACH, "No route to host");
#endif
#endif
#ifdef ENOMSG
    add_errcode("ENOMSG", ENOMSG, "No message of desired type");
#endif
#ifdef EUCLEAN
    add_errcode("EUCLEAN", EUCLEAN, "Structure needs cleaning");
#endif
#ifdef EL2NSYNC
    add_errcode("EL2NSYNC", EL2NSYNC, "Level 2 not synchronized");
#endif
#ifdef EL2HLT
    add_errcode("EL2HLT", EL2HLT, "Level 2 halted");
#endif
#ifdef ENODATA
    add_errcode("ENODATA", ENODATA, "No data available");
#endif
#ifdef ENOTBLK
    add_errcode("ENOTBLK", ENOTBLK, "Block device required");
#endif
#ifdef ENOSYS
    add_errcode("ENOSYS", ENOSYS, "Function not implemented");
#endif
#ifdef EPIPE
    add_errcode("EPIPE", EPIPE, "Broken pipe");
#endif
#ifdef EINVAL
    add_errcode("EINVAL", EINVAL, "Invalid argument");
#else
#ifdef WSAEINVAL
    add_errcode("EINVAL", WSAEINVAL, "Invalid argument");
#endif
#endif
#ifdef EOVERFLOW
    add_errcode("EOVERFLOW", EOVERFLOW, "Value too large for defined data type");
#endif
#ifdef EADV
    add_errcode("EADV", EADV, "Advertise error");
#endif
#ifdef EINTR
    add_errcode("EINTR", EINTR, "Interrupted system call");
#else
#ifdef WSAEINTR
    add_errcode("EINTR", WSAEINTR, "Interrupted system call");
#endif
#endif
#ifdef EUSERS
    add_errcode("EUSERS", EUSERS, "Too many users");
#else
#ifdef WSAEUSERS
    add_errcode("EUSERS", WSAEUSERS, "Too many users");
#endif
#endif
#ifdef ENOTEMPTY
    add_errcode("ENOTEMPTY", ENOTEMPTY, "Directory not empty");
#else
#ifdef WSAENOTEMPTY
    add_errcode("ENOTEMPTY", WSAENOTEMPTY, "Directory not empty");
#endif
#endif
#ifdef ENOBUFS
    add_errcode("ENOBUFS", ENOBUFS, "No buffer space available");
#else
#ifdef WSAENOBUFS
    add_errcode("ENOBUFS", WSAENOBUFS, "No buffer space available");
#endif
#endif
#ifdef EPROTO
    add_errcode("EPROTO", EPROTO, "Protocol error");
#endif
#ifdef EREMOTE
    add_errcode("EREMOTE", EREMOTE, "Object is remote");
#else
#ifdef WSAEREMOTE
    add_errcode("EREMOTE", WSAEREMOTE, "Object is remote");
#endif
#endif
#ifdef ENAVAIL
    add_errcode("ENAVAIL", ENAVAIL, "No XENIX semaphores available");
#endif
#ifdef ECHILD
    add_errcode("ECHILD", ECHILD, "No child processes");
#endif
#ifdef ELOOP
    add_errcode("ELOOP", ELOOP, "Too many symbolic links encountered");
#else
#ifdef WSAELOOP
    add_errcode("ELOOP", WSAELOOP, "Too many symbolic links encountered");
#endif
#endif
#ifdef EXDEV
    add_errcode("EXDEV", EXDEV, "Cross-device link");
#endif
#ifdef E2BIG
    add_errcode("E2BIG", E2BIG, "Arg list too long");
#endif
#ifdef ESRCH
    add_errcode("ESRCH", ESRCH, "No such process");
#endif
#ifdef EMSGSIZE
    add_errcode("EMSGSIZE", EMSGSIZE, "Message too long");
#else
#ifdef WSAEMSGSIZE
    add_errcode("EMSGSIZE", WSAEMSGSIZE, "Message too long");
#endif
#endif
#ifdef EAFNOSUPPORT
    add_errcode("EAFNOSUPPORT", EAFNOSUPPORT, "Address family not supported by protocol");
#else
#ifdef WSAEAFNOSUPPORT
    add_errcode("EAFNOSUPPORT", WSAEAFNOSUPPORT, "Address family not supported by protocol");
#endif
#endif
#ifdef EBADR
    add_errcode("EBADR", EBADR, "Invalid request descriptor");
#endif
#ifdef EHOSTDOWN
    add_errcode("EHOSTDOWN", EHOSTDOWN, "Host is down");
#else
#ifdef WSAEHOSTDOWN
    add_errcode("EHOSTDOWN", WSAEHOSTDOWN, "Host is down");
#endif
#endif
#ifdef EPFNOSUPPORT
    add_errcode("EPFNOSUPPORT", EPFNOSUPPORT, "Protocol family not supported");
#else
#ifdef WSAEPFNOSUPPORT
    add_errcode("EPFNOSUPPORT", WSAEPFNOSUPPORT, "Protocol family not supported");
#endif
#endif
#ifdef ENOPROTOOPT
    add_errcode("ENOPROTOOPT", ENOPROTOOPT, "Protocol not available");
#else
#ifdef WSAENOPROTOOPT
    add_errcode("ENOPROTOOPT", WSAENOPROTOOPT, "Protocol not available");
#endif
#endif
#ifdef EBUSY
    add_errcode("EBUSY", EBUSY, "Device or resource busy");
#endif
#ifdef EWOULDBLOCK
    add_errcode("EWOULDBLOCK", EWOULDBLOCK, "Operation would block");
#else
#ifdef WSAEWOULDBLOCK
    add_errcode("EWOULDBLOCK", WSAEWOULDBLOCK, "Operation would block");
#endif
#endif
#ifdef EBADFD
    add_errcode("EBADFD", EBADFD, "File descriptor in bad state");
#endif
#ifdef EDOTDOT
    add_errcode("EDOTDOT", EDOTDOT, "RFS specific error");
#endif
#ifdef EISCONN
    add_errcode("EISCONN", EISCONN, "Transport endpoint is already connected");
#else
#ifdef WSAEISCONN
    add_errcode("EISCONN", WSAEISCONN, "Transport endpoint is already connected");
#endif
#endif
#ifdef ENOANO
    add_errcode("ENOANO", ENOANO, "No anode");
#endif
#if defined(__wasi__) && !defined(ESHUTDOWN)
    // WASI SDK 16 does not have ESHUTDOWN, shutdown results in EPIPE.
    #define ESHUTDOWN EPIPE
#endif
#ifdef ESHUTDOWN
    add_errcode("ESHUTDOWN", ESHUTDOWN, "Cannot send after transport endpoint shutdown");
#else
#ifdef WSAESHUTDOWN
    add_errcode("ESHUTDOWN", WSAESHUTDOWN, "Cannot send after transport endpoint shutdown");
#endif
#endif
#ifdef ECHRNG
    add_errcode("ECHRNG", ECHRNG, "Channel number out of range");
#endif
#ifdef ELIBBAD
    add_errcode("ELIBBAD", ELIBBAD, "Accessing a corrupted shared library");
#endif
#ifdef ENONET
    add_errcode("ENONET", ENONET, "Machine is not on the network");
#endif
#ifdef EBADE
    add_errcode("EBADE", EBADE, "Invalid exchange");
#endif
#ifdef EBADF
    add_errcode("EBADF", EBADF, "Bad file number");
#else
#ifdef WSAEBADF
    add_errcode("EBADF", WSAEBADF, "Bad file number");
#endif
#endif
#ifdef EMULTIHOP
    add_errcode("EMULTIHOP", EMULTIHOP, "Multihop attempted");
#endif
#ifdef EIO
    add_errcode("EIO", EIO, "I/O error");
#endif
#ifdef EUNATCH
    add_errcode("EUNATCH", EUNATCH, "Protocol driver not attached");
#endif
#ifdef EPROTOTYPE
    add_errcode("EPROTOTYPE", EPROTOTYPE, "Protocol wrong type for socket");
#else
#ifdef WSAEPROTOTYPE
    add_errcode("EPROTOTYPE", WSAEPROTOTYPE, "Protocol wrong type for socket");
#endif
#endif
#ifdef ENOSPC
    add_errcode("ENOSPC", ENOSPC, "No space left on device");
#endif
#ifdef ENOEXEC
    add_errcode("ENOEXEC", ENOEXEC, "Exec format error");
#endif
#ifdef EALREADY
    add_errcode("EALREADY", EALREADY, "Operation already in progress");
#else
#ifdef WSAEALREADY
    add_errcode("EALREADY", WSAEALREADY, "Operation already in progress");
#endif
#endif
#ifdef ENETDOWN
    add_errcode("ENETDOWN", ENETDOWN, "Network is down");
#else
#ifdef WSAENETDOWN
    add_errcode("ENETDOWN", WSAENETDOWN, "Network is down");
#endif
#endif
#ifdef ENOTNAM
    add_errcode("ENOTNAM", ENOTNAM, "Not a XENIX named type file");
#endif
#ifdef EACCES
    add_errcode("EACCES", EACCES, "Permission denied");
#else
#ifdef WSAEACCES
    add_errcode("EACCES", WSAEACCES, "Permission denied");
#endif
#endif
#ifdef ELNRNG
    add_errcode("ELNRNG", ELNRNG, "Link number out of range");
#endif
#ifdef EILSEQ
    add_errcode("EILSEQ", EILSEQ, "Illegal byte sequence");
#endif
#ifdef ENOTDIR
    add_errcode("ENOTDIR", ENOTDIR, "Not a directory");
#endif
#ifdef ENOTUNIQ
    add_errcode("ENOTUNIQ", ENOTUNIQ, "Name not unique on network");
#endif
#ifdef EPERM
    add_errcode("EPERM", EPERM, "Operation not permitted");
#endif
#ifdef EDOM
    add_errcode("EDOM", EDOM, "Math argument out of domain of func");
#endif
#ifdef EXFULL
    add_errcode("EXFULL", EXFULL, "Exchange full");
#endif
#ifdef ECONNREFUSED
    add_errcode("ECONNREFUSED", ECONNREFUSED, "Connection refused");
#else
#ifdef WSAECONNREFUSED
    add_errcode("ECONNREFUSED", WSAECONNREFUSED, "Connection refused");
#endif
#endif
#ifdef EISDIR
    add_errcode("EISDIR", EISDIR, "Is a directory");
#endif
#ifdef EPROTONOSUPPORT
    add_errcode("EPROTONOSUPPORT", EPROTONOSUPPORT, "Protocol not supported");
#else
#ifdef WSAEPROTONOSUPPORT
    add_errcode("EPROTONOSUPPORT", WSAEPROTONOSUPPORT, "Protocol not supported");
#endif
#endif
#ifdef EROFS
    add_errcode("EROFS", EROFS, "Read-only file system");
#endif
#ifdef EADDRNOTAVAIL
    add_errcode("EADDRNOTAVAIL", EADDRNOTAVAIL, "Cannot assign requested address");
#else
#ifdef WSAEADDRNOTAVAIL
    add_errcode("EADDRNOTAVAIL", WSAEADDRNOTAVAIL, "Cannot assign requested address");
#endif
#endif
#ifdef EIDRM
    add_errcode("EIDRM", EIDRM, "Identifier removed");
#endif
#ifdef ECOMM
    add_errcode("ECOMM", ECOMM, "Communication error on send");
#endif
#ifdef ESRMNT
    add_errcode("ESRMNT", ESRMNT, "Srmount error");
#endif
#ifdef EREMOTEIO
    add_errcode("EREMOTEIO", EREMOTEIO, "Remote I/O error");
#endif
#ifdef EL3RST
    add_errcode("EL3RST", EL3RST, "Level 3 reset");
#endif
#ifdef EBADMSG
    add_errcode("EBADMSG", EBADMSG, "Not a data message");
#endif
#ifdef ENFILE
    add_errcode("ENFILE", ENFILE, "File table overflow");
#endif
#ifdef ELIBMAX
    add_errcode("ELIBMAX", ELIBMAX, "Attempting to link in too many shared libraries");
#endif
#ifdef ESPIPE
    add_errcode("ESPIPE", ESPIPE, "Illegal seek");
#endif
#ifdef ENOLINK
    add_errcode("ENOLINK", ENOLINK, "Link has been severed");
#endif
#ifdef ENETRESET
    add_errcode("ENETRESET", ENETRESET, "Network dropped connection because of reset");
#else
#ifdef WSAENETRESET
    add_errcode("ENETRESET", WSAENETRESET, "Network dropped connection because of reset");
#endif
#endif
#ifdef ETIMEDOUT
    add_errcode("ETIMEDOUT", ETIMEDOUT, "Connection timed out");
#else
#ifdef WSAETIMEDOUT
    add_errcode("ETIMEDOUT", WSAETIMEDOUT, "Connection timed out");
#endif
#endif
#ifdef ENOENT
    add_errcode("ENOENT", ENOENT, "No such file or directory");
#endif
#ifdef EEXIST
    add_errcode("EEXIST", EEXIST, "File exists");
#endif
#ifdef EDQUOT
    add_errcode("EDQUOT", EDQUOT, "Quota exceeded");
#else
#ifdef WSAEDQUOT
    add_errcode("EDQUOT", WSAEDQUOT, "Quota exceeded");
#endif
#endif
#ifdef ENOSTR
    add_errcode("ENOSTR", ENOSTR, "Device not a stream");
#endif
#ifdef EBADSLT
    add_errcode("EBADSLT", EBADSLT, "Invalid slot");
#endif
#ifdef EBADRQC
    add_errcode("EBADRQC", EBADRQC, "Invalid request code");
#endif
#ifdef ELIBACC
    add_errcode("ELIBACC", ELIBACC, "Can not access a needed shared library");
#endif
#ifdef EFAULT
    add_errcode("EFAULT", EFAULT, "Bad address");
#else
#ifdef WSAEFAULT
    add_errcode("EFAULT", WSAEFAULT, "Bad address");
#endif
#endif
#ifdef EFBIG
    add_errcode("EFBIG", EFBIG, "File too large");
#endif
#ifdef EDEADLK
    add_errcode("EDEADLK", EDEADLK, "Resource deadlock would occur");
#endif
#ifdef ENOTCONN
    add_errcode("ENOTCONN", ENOTCONN, "Transport endpoint is not connected");
#else
#ifdef WSAENOTCONN
    add_errcode("ENOTCONN", WSAENOTCONN, "Transport endpoint is not connected");
#endif
#endif
#ifdef EDESTADDRREQ
    add_errcode("EDESTADDRREQ", EDESTADDRREQ, "Destination address required");
#else
#ifdef WSAEDESTADDRREQ
    add_errcode("EDESTADDRREQ", WSAEDESTADDRREQ, "Destination address required");
#endif
#endif
#ifdef ELIBSCN
    add_errcode("ELIBSCN", ELIBSCN, ".lib section in a.out corrupted");
#endif
#ifdef ENOLCK
    add_errcode("ENOLCK", ENOLCK, "No record locks available");
#endif
#ifdef EISNAM
    add_errcode("EISNAM", EISNAM, "Is a named type file");
#endif
#ifdef ECONNABORTED
    add_errcode("ECONNABORTED", ECONNABORTED, "Software caused connection abort");
#else
#ifdef WSAECONNABORTED
    add_errcode("ECONNABORTED", WSAECONNABORTED, "Software caused connection abort");
#endif
#endif
#ifdef ENETUNREACH
    add_errcode("ENETUNREACH", ENETUNREACH, "Network is unreachable");
#else
#ifdef WSAENETUNREACH
    add_errcode("ENETUNREACH", WSAENETUNREACH, "Network is unreachable");
#endif
#endif
#ifdef ESTALE
    add_errcode("ESTALE", ESTALE, "Stale NFS file handle");
#else
#ifdef WSAESTALE
    add_errcode("ESTALE", WSAESTALE, "Stale NFS file handle");
#endif
#endif
#ifdef ENOSR
    add_errcode("ENOSR", ENOSR, "Out of streams resources");
#endif
#ifdef ENOMEM
    add_errcode("ENOMEM", ENOMEM, "Out of memory");
#endif
#ifdef ENOTSOCK
    add_errcode("ENOTSOCK", ENOTSOCK, "Socket operation on non-socket");
#else
#ifdef WSAENOTSOCK
    add_errcode("ENOTSOCK", WSAENOTSOCK, "Socket operation on non-socket");
#endif
#endif
#ifdef ESTRPIPE
    add_errcode("ESTRPIPE", ESTRPIPE, "Streams pipe error");
#endif
#ifdef EMLINK
    add_errcode("EMLINK", EMLINK, "Too many links");
#endif
#ifdef ERANGE
    add_errcode("ERANGE", ERANGE, "Math result not representable");
#endif
#ifdef ELIBEXEC
    add_errcode("ELIBEXEC", ELIBEXEC, "Cannot exec a shared library directly");
#endif
#ifdef EL3HLT
    add_errcode("EL3HLT", EL3HLT, "Level 3 halted");
#endif
#ifdef ECONNRESET
    add_errcode("ECONNRESET", ECONNRESET, "Connection reset by peer");
#else
#ifdef WSAECONNRESET
    add_errcode("ECONNRESET", WSAECONNRESET, "Connection reset by peer");
#endif
#endif
#ifdef EADDRINUSE
    add_errcode("EADDRINUSE", EADDRINUSE, "Address already in use");
#else
#ifdef WSAEADDRINUSE
    add_errcode("EADDRINUSE", WSAEADDRINUSE, "Address already in use");
#endif
#endif
#ifdef EOPNOTSUPP
    add_errcode("EOPNOTSUPP", EOPNOTSUPP, "Operation not supported on transport endpoint");
#else
#ifdef WSAEOPNOTSUPP
    add_errcode("EOPNOTSUPP", WSAEOPNOTSUPP, "Operation not supported on transport endpoint");
#endif
#endif
#ifdef EREMCHG
    add_errcode("EREMCHG", EREMCHG, "Remote address changed");
#endif
#ifdef EAGAIN
    add_errcode("EAGAIN", EAGAIN, "Try again");
#endif
#ifdef ENAMETOOLONG
    add_errcode("ENAMETOOLONG", ENAMETOOLONG, "File name too long");
#else
#ifdef WSAENAMETOOLONG
    add_errcode("ENAMETOOLONG", WSAENAMETOOLONG, "File name too long");
#endif
#endif
#ifdef ENOTTY
    add_errcode("ENOTTY", ENOTTY, "Not a typewriter");
#endif
#ifdef ERESTART
    add_errcode("ERESTART", ERESTART, "Interrupted system call should be restarted");
#endif
#ifdef ESOCKTNOSUPPORT
    add_errcode("ESOCKTNOSUPPORT", ESOCKTNOSUPPORT, "Socket type not supported");
#else
#ifdef WSAESOCKTNOSUPPORT
    add_errcode("ESOCKTNOSUPPORT", WSAESOCKTNOSUPPORT, "Socket type not supported");
#endif
#endif
#ifdef ETIME
    add_errcode("ETIME", ETIME, "Timer expired");
#endif
#ifdef EBFONT
    add_errcode("EBFONT", EBFONT, "Bad font file format");
#endif
#ifdef EDEADLOCK
    add_errcode("EDEADLOCK", EDEADLOCK, "Error EDEADLOCK");
#endif
#ifdef ETOOMANYREFS
    add_errcode("ETOOMANYREFS", ETOOMANYREFS, "Too many references: cannot splice");
#else
#ifdef WSAETOOMANYREFS
    add_errcode("ETOOMANYREFS", WSAETOOMANYREFS, "Too many references: cannot splice");
#endif
#endif
#ifdef EMFILE
    add_errcode("EMFILE", EMFILE, "Too many open files");
#else
#ifdef WSAEMFILE
    add_errcode("EMFILE", WSAEMFILE, "Too many open files");
#endif
#endif
#ifdef ETXTBSY
    add_errcode("ETXTBSY", ETXTBSY, "Text file busy");
#endif
#ifdef EINPROGRESS
    add_errcode("EINPROGRESS", EINPROGRESS, "Operation now in progress");
#else
#ifdef WSAEINPROGRESS
    add_errcode("EINPROGRESS", WSAEINPROGRESS, "Operation now in progress");
#endif
#endif
#ifdef ENXIO
    add_errcode("ENXIO", ENXIO, "No such device or address");
#endif
#ifdef ENOPKG
    add_errcode("ENOPKG", ENOPKG, "Package not installed");
#endif
#ifdef WSASY
    add_errcode("WSASY", WSASY, "Error WSASY");
#endif
#ifdef WSAEHOSTDOWN
    add_errcode("WSAEHOSTDOWN", WSAEHOSTDOWN, "Host is down");
#endif
#ifdef WSAENETDOWN
    add_errcode("WSAENETDOWN", WSAENETDOWN, "Network is down");
#endif
#ifdef WSAENOTSOCK
    add_errcode("WSAENOTSOCK", WSAENOTSOCK, "Socket operation on non-socket");
#endif
#ifdef WSAEHOSTUNREACH
    add_errcode("WSAEHOSTUNREACH", WSAEHOSTUNREACH, "No route to host");
#endif
#ifdef WSAELOOP
    add_errcode("WSAELOOP", WSAELOOP, "Too many symbolic links encountered");
#endif
#ifdef WSAEMFILE
    add_errcode("WSAEMFILE", WSAEMFILE, "Too many open files");
#endif
#ifdef WSAESTALE
    add_errcode("WSAESTALE", WSAESTALE, "Stale NFS file handle");
#endif
#ifdef WSAVERNOTSUPPORTED
    add_errcode("WSAVERNOTSUPPORTED", WSAVERNOTSUPPORTED, "Error WSAVERNOTSUPPORTED");
#endif
#ifdef WSAENETUNREACH
    add_errcode("WSAENETUNREACH", WSAENETUNREACH, "Network is unreachable");
#endif
#ifdef WSAEPROCLIM
    add_errcode("WSAEPROCLIM", WSAEPROCLIM, "Error WSAEPROCLIM");
#endif
#ifdef WSAEFAULT
    add_errcode("WSAEFAULT", WSAEFAULT, "Bad address");
#endif
#ifdef WSANOTINITIALISED
    add_errcode("WSANOTINITIALISED", WSANOTINITIALISED, "Error WSANOTINITIALISED");
#endif
#ifdef WSAEUSERS
    add_errcode("WSAEUSERS", WSAEUSERS, "Too many users");
#endif
#ifdef WSAMAKEASYNCREPL
    add_errcode("WSAMAKEASYNCREPL", WSAMAKEASYNCREPL, "Error WSAMAKEASYNCREPL");
#endif
#ifdef WSAENOPROTOOPT
    add_errcode("WSAENOPROTOOPT", WSAENOPROTOOPT, "Protocol not available");
#endif
#ifdef WSAECONNABORTED
    add_errcode("WSAECONNABORTED", WSAECONNABORTED, "Software caused connection abort");
#endif
#ifdef WSAENAMETOOLONG
    add_errcode("WSAENAMETOOLONG", WSAENAMETOOLONG, "File name too long");
#endif
#ifdef WSAENOTEMPTY
    add_errcode("WSAENOTEMPTY", WSAENOTEMPTY, "Directory not empty");
#endif
#ifdef WSAESHUTDOWN
    add_errcode("WSAESHUTDOWN", WSAESHUTDOWN, "Cannot send after transport endpoint shutdown");
#endif
#ifdef WSAEAFNOSUPPORT
    add_errcode("WSAEAFNOSUPPORT", WSAEAFNOSUPPORT, "Address family not supported by protocol");
#endif
#ifdef WSAETOOMANYREFS
    add_errcode("WSAETOOMANYREFS", WSAETOOMANYREFS, "Too many references: cannot splice");
#endif
#ifdef WSAEACCES
    add_errcode("WSAEACCES", WSAEACCES, "Permission denied");
#endif
#ifdef WSATR
    add_errcode("WSATR", WSATR, "Error WSATR");
#endif
#ifdef WSABASEERR
    add_errcode("WSABASEERR", WSABASEERR, "Error WSABASEERR");
#endif
#ifdef WSADESCRIPTIO
    add_errcode("WSADESCRIPTIO", WSADESCRIPTIO, "Error WSADESCRIPTIO");
#endif
#ifdef WSAEMSGSIZE
    add_errcode("WSAEMSGSIZE", WSAEMSGSIZE, "Message too long");
#endif
#ifdef WSAEBADF
    add_errcode("WSAEBADF", WSAEBADF, "Bad file number");
#endif
#ifdef WSAECONNRESET
    add_errcode("WSAECONNRESET", WSAECONNRESET, "Connection reset by peer");
#endif
#ifdef WSAGETSELECTERRO
    add_errcode("WSAGETSELECTERRO", WSAGETSELECTERRO, "Error WSAGETSELECTERRO");
#endif
#ifdef WSAETIMEDOUT
    add_errcode("WSAETIMEDOUT", WSAETIMEDOUT, "Connection timed out");
#endif
#ifdef WSAENOBUFS
    add_errcode("WSAENOBUFS", WSAENOBUFS, "No buffer space available");
#endif
#ifdef WSAEDISCON
    add_errcode("WSAEDISCON", WSAEDISCON, "Error WSAEDISCON");
#endif
#ifdef WSAEINTR
    add_errcode("WSAEINTR", WSAEINTR, "Interrupted system call");
#endif
#ifdef WSAEPROTOTYPE
    add_errcode("WSAEPROTOTYPE", WSAEPROTOTYPE, "Protocol wrong type for socket");
#endif
#ifdef WSAHOS
    add_errcode("WSAHOS", WSAHOS, "Error WSAHOS");
#endif
#ifdef WSAEADDRINUSE
    add_errcode("WSAEADDRINUSE", WSAEADDRINUSE, "Address already in use");
#endif
#ifdef WSAEADDRNOTAVAIL
    add_errcode("WSAEADDRNOTAVAIL", WSAEADDRNOTAVAIL, "Cannot assign requested address");
#endif
#ifdef WSAEALREADY
    add_errcode("WSAEALREADY", WSAEALREADY, "Operation already in progress");
#endif
#ifdef WSAEPROTONOSUPPORT
    add_errcode("WSAEPROTONOSUPPORT", WSAEPROTONOSUPPORT, "Protocol not supported");
#endif
#ifdef WSASYSNOTREADY
    add_errcode("WSASYSNOTREADY", WSASYSNOTREADY, "Error WSASYSNOTREADY");
#endif
#ifdef WSAEWOULDBLOCK
    add_errcode("WSAEWOULDBLOCK", WSAEWOULDBLOCK, "Operation would block");
#endif
#ifdef WSAEPFNOSUPPORT
    add_errcode("WSAEPFNOSUPPORT", WSAEPFNOSUPPORT, "Protocol family not supported");
#endif
#ifdef WSAEOPNOTSUPP
    add_errcode("WSAEOPNOTSUPP", WSAEOPNOTSUPP, "Operation not supported on transport endpoint");
#endif
#ifdef WSAEISCONN
    add_errcode("WSAEISCONN", WSAEISCONN, "Transport endpoint is already connected");
#endif
#ifdef WSAEDQUOT
    add_errcode("WSAEDQUOT", WSAEDQUOT, "Quota exceeded");
#endif
#ifdef WSAENOTCONN
    add_errcode("WSAENOTCONN", WSAENOTCONN, "Transport endpoint is not connected");
#endif
#ifdef WSAEREMOTE
    add_errcode("WSAEREMOTE", WSAEREMOTE, "Object is remote");
#endif
#ifdef WSAEINVAL
    add_errcode("WSAEINVAL", WSAEINVAL, "Invalid argument");
#endif
#ifdef WSAEINPROGRESS
    add_errcode("WSAEINPROGRESS", WSAEINPROGRESS, "Operation now in progress");
#endif
#ifdef WSAGETSELECTEVEN
    add_errcode("WSAGETSELECTEVEN", WSAGETSELECTEVEN, "Error WSAGETSELECTEVEN");
#endif
#ifdef WSAESOCKTNOSUPPORT
    add_errcode("WSAESOCKTNOSUPPORT", WSAESOCKTNOSUPPORT, "Socket type not supported");
#endif
#ifdef WSAGETASYNCERRO
    add_errcode("WSAGETASYNCERRO", WSAGETASYNCERRO, "Error WSAGETASYNCERRO");
#endif
#ifdef WSAMAKESELECTREPL
    add_errcode("WSAMAKESELECTREPL", WSAMAKESELECTREPL, "Error WSAMAKESELECTREPL");
#endif
#ifdef WSAGETASYNCBUFLE
    add_errcode("WSAGETASYNCBUFLE", WSAGETASYNCBUFLE, "Error WSAGETASYNCBUFLE");
#endif
#ifdef WSAEDESTADDRREQ
    add_errcode("WSAEDESTADDRREQ", WSAEDESTADDRREQ, "Destination address required");
#endif
#ifdef WSAECONNREFUSED
    add_errcode("WSAECONNREFUSED", WSAECONNREFUSED, "Connection refused");
#endif
#ifdef WSAENETRESET
    add_errcode("WSAENETRESET", WSAENETRESET, "Network dropped connection because of reset");
#endif
#ifdef WSAN
    add_errcode("WSAN", WSAN, "Error WSAN");
#endif
#ifdef ENOMEDIUM
    add_errcode("ENOMEDIUM", ENOMEDIUM, "No medium found");
#endif
#ifdef EMEDIUMTYPE
    add_errcode("EMEDIUMTYPE", EMEDIUMTYPE, "Wrong medium type");
#endif
#ifdef ECANCELED
    add_errcode("ECANCELED", ECANCELED, "Operation Canceled");
#endif
#ifdef ENOKEY
    add_errcode("ENOKEY", ENOKEY, "Required key not available");
#endif
#ifdef EKEYEXPIRED
    add_errcode("EKEYEXPIRED", EKEYEXPIRED, "Key has expired");
#endif
#ifdef EKEYREVOKED
    add_errcode("EKEYREVOKED", EKEYREVOKED, "Key has been revoked");
#endif
#ifdef EKEYREJECTED
    add_errcode("EKEYREJECTED", EKEYREJECTED, "Key was rejected by service");
#endif
#ifdef EOWNERDEAD
    add_errcode("EOWNERDEAD", EOWNERDEAD, "Owner died");
#endif
#ifdef ENOTRECOVERABLE
    add_errcode("ENOTRECOVERABLE", ENOTRECOVERABLE, "State not recoverable");
#endif
#ifdef ERFKILL
    add_errcode("ERFKILL", ERFKILL, "Operation not possible due to RF-kill");
#endif

    /* Solaris-specific errnos */
#ifdef ECANCELED
    add_errcode("ECANCELED", ECANCELED, "Operation canceled");
#endif
#ifdef ENOTSUP
    add_errcode("ENOTSUP", ENOTSUP, "Operation not supported");
#endif
#ifdef EOWNERDEAD
    add_errcode("EOWNERDEAD", EOWNERDEAD, "Process died with the lock");
#endif
#ifdef ENOTRECOVERABLE
    add_errcode("ENOTRECOVERABLE", ENOTRECOVERABLE, "Lock is not recoverable");
#endif
#ifdef ELOCKUNMAPPED
    add_errcode("ELOCKUNMAPPED", ELOCKUNMAPPED, "Locked lock was unmapped");
#endif
#ifdef ENOTACTIVE
    add_errcode("ENOTACTIVE", ENOTACTIVE, "Facility is not active");
#endif

    /* MacOSX specific errnos */
#ifdef EAUTH
    add_errcode("EAUTH", EAUTH, "Authentication error");
#endif
#ifdef EBADARCH
    add_errcode("EBADARCH", EBADARCH, "Bad CPU type in executable");
#endif
#ifdef EBADEXEC
    add_errcode("EBADEXEC", EBADEXEC, "Bad executable (or shared library)");
#endif
#ifdef EBADMACHO
    add_errcode("EBADMACHO", EBADMACHO, "Malformed Mach-o file");
#endif
#ifdef EBADRPC
    add_errcode("EBADRPC", EBADRPC, "RPC struct is bad");
#endif
#ifdef EDEVERR
    add_errcode("EDEVERR", EDEVERR, "Device error");
#endif
#ifdef EFTYPE
    add_errcode("EFTYPE", EFTYPE, "Inappropriate file type or format");
#endif
#ifdef ENEEDAUTH
    add_errcode("ENEEDAUTH", ENEEDAUTH, "Need authenticator");
#endif
#ifdef ENOATTR
    add_errcode("ENOATTR", ENOATTR, "Attribute not found");
#endif
#ifdef ENOPOLICY
    add_errcode("ENOPOLICY", ENOPOLICY, "Policy not found");
#endif
#ifdef EPROCLIM
    add_errcode("EPROCLIM", EPROCLIM, "Too many processes");
#endif
#ifdef EPROCUNAVAIL
    add_errcode("EPROCUNAVAIL", EPROCUNAVAIL, "Bad procedure for program");
#endif
#ifdef EPROGMISMATCH
    add_errcode("EPROGMISMATCH", EPROGMISMATCH, "Program version wrong");
#endif
#ifdef EPROGUNAVAIL
    add_errcode("EPROGUNAVAIL", EPROGUNAVAIL, "RPC prog. not avail");
#endif
#ifdef EPWROFF
    add_errcode("EPWROFF", EPWROFF, "Device power is off");
#endif
#ifdef ERPCMISMATCH
    add_errcode("ERPCMISMATCH", ERPCMISMATCH, "RPC version wrong");
#endif
#ifdef ESHLIBVERS
    add_errcode("ESHLIBVERS", ESHLIBVERS, "Shared library version mismatch");
#endif
#ifdef EQFULL
    add_errcode("EQFULL", EQFULL, "Interface output queue is full");
#endif
#ifdef ENOTCAPABLE
    // WASI extension
    add_errcode("ENOTCAPABLE", ENOTCAPABLE, "Capabilities insufficient");
#endif

    Py_DECREF(error_dict);
    return 0;
}

static PyModuleDef_Slot errno_slots[] = {
    {Py_mod_exec, errno_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
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
