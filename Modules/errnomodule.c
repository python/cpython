/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Errno module */

#include "Python.h"

/* Mac with GUSI has more errors than those in errno.h */
#ifdef USE_GUSI
#include <sys/errno.h>
#endif

/*
 * Pull in the system error definitions
 */ 

static PyMethodDef errno_methods[] = {
  {NULL,	NULL}
};

/*
 * Convenience routine to export an integer value.
 * For simplicity, errors (which are unlikely anyway) are ignored.
 */

static void
insint(d, name, value)
	PyObject * d;
	char * name;
	int value;
{
	PyObject *v = PyInt_FromLong((long) value);
	if (v == NULL) {
		/* Don't bother reporting this error */
		PyErr_Clear();
	}
	else {
		PyDict_SetItemString(d, name, v);
		Py_DECREF(v);
	}
}

void
initerrno()
{
	PyObject *m, *d;
	m = Py_InitModule("errno", errno_methods);
	d = PyModule_GetDict(m);

	/*
	 * The names and comments are borrowed from linux/include/errno.h,
	 * which should be pretty all-inclusive
	 */ 

#ifdef EPERM
	/* Operation not permitted */
	insint(d, "EPERM", EPERM);
#endif
#ifdef ENOENT
	/* No such file or directory */
	insint(d, "ENOENT", ENOENT);
#endif
#ifdef ESRCH
	/* No such process */
	insint(d, "ESRCH", ESRCH);
#endif
#ifdef EINTR
	/* Interrupted system call */
	insint(d, "EINTR", EINTR);
#endif
#ifdef EIO
	/* I/O error */
	insint(d, "EIO", EIO);
#endif
#ifdef ENXIO
	/* No such device or address */
	insint(d, "ENXIO", ENXIO);
#endif
#ifdef E2BIG
	/* Arg list too long */
	insint(d, "E2BIG", E2BIG);
#endif
#ifdef ENOEXEC
	/* Exec format error */
	insint(d, "ENOEXEC", ENOEXEC);
#endif
#ifdef EBADF
	/* Bad file number */
	insint(d, "EBADF", EBADF);
#endif
#ifdef ECHILD
	/* No child processes */
	insint(d, "ECHILD", ECHILD);
#endif
#ifdef EAGAIN
	/* Try again */
	insint(d, "EAGAIN", EAGAIN);
#endif
#ifdef ENOMEM
	/* Out of memory */
	insint(d, "ENOMEM", ENOMEM);
#endif
#ifdef EACCES
	/* Permission denied */
	insint(d, "EACCES", EACCES);
#endif
#ifdef EFAULT
	/* Bad address */
	insint(d, "EFAULT", EFAULT);
#endif
#ifdef ENOTBLK
	/* Block device required */
	insint(d, "ENOTBLK", ENOTBLK);
#endif
#ifdef EBUSY
	/* Device or resource busy */
	insint(d, "EBUSY", EBUSY);
#endif
#ifdef EEXIST
	/* File exists */
	insint(d, "EEXIST", EEXIST);
#endif
#ifdef EXDEV
	/* Cross-device link */
	insint(d, "EXDEV", EXDEV);
#endif
#ifdef ENODEV
	/* No such device */
	insint(d, "ENODEV", ENODEV);
#endif
#ifdef ENOTDIR
	/* Not a directory */
	insint(d, "ENOTDIR", ENOTDIR);
#endif
#ifdef EISDIR
	/* Is a directory */
	insint(d, "EISDIR", EISDIR);
#endif
#ifdef EINVAL
	/* Invalid argument */
	insint(d, "EINVAL", EINVAL);
#endif
#ifdef ENFILE
	/* File table overflow */
	insint(d, "ENFILE", ENFILE);
#endif
#ifdef EMFILE
	/* Too many open files */
	insint(d, "EMFILE", EMFILE);
#endif
#ifdef ENOTTY
	/* Not a typewriter */
	insint(d, "ENOTTY", ENOTTY);
#endif
#ifdef ETXTBSY
	/* Text file busy */
	insint(d, "ETXTBSY", ETXTBSY);
#endif
#ifdef EFBIG
	/* File too large */
	insint(d, "EFBIG", EFBIG);
#endif
#ifdef ENOSPC
	/* No space left on device */
	insint(d, "ENOSPC", ENOSPC);
#endif
#ifdef ESPIPE
	/* Illegal seek */
	insint(d, "ESPIPE", ESPIPE);
#endif
#ifdef EROFS
	/* Read-only file system */
	insint(d, "EROFS", EROFS);
#endif
#ifdef EMLINK
	/* Too many links */
	insint(d, "EMLINK", EMLINK);
#endif
#ifdef EPIPE
	/* Broken pipe */
	insint(d, "EPIPE", EPIPE);
#endif
#ifdef EDOM
	/* Math argument out of domain of func */
	insint(d, "EDOM", EDOM);
#endif
#ifdef ERANGE
	/* Math result not representable */
	insint(d, "ERANGE", ERANGE);
#endif
#ifdef EDEADLK
	/* Resource deadlock would occur */
	insint(d, "EDEADLK", EDEADLK);
#endif
#ifdef ENAMETOOLONG
	/* File name too long */
	insint(d, "ENAMETOOLONG", ENAMETOOLONG);
#endif
#ifdef ENOLCK
	/* No record locks available */
	insint(d, "ENOLCK", ENOLCK);
#endif
#ifdef ENOSYS
	/* Function not implemented */
	insint(d, "ENOSYS", ENOSYS);
#endif
#ifdef ENOTEMPTY
	/* Directory not empty */
	insint(d, "ENOTEMPTY", ENOTEMPTY);
#endif
#ifdef ELOOP
	/* Too many symbolic links encountered */
	insint(d, "ELOOP", ELOOP);
#endif
#ifdef EWOULDBLOCK
	/* Operation would block */
	insint(d, "EWOULDBLOCK", EWOULDBLOCK);
#endif
#ifdef ENOMSG
	/* No message of desired type */
	insint(d, "ENOMSG", ENOMSG);
#endif
#ifdef EIDRM
	/* Identifier removed */
	insint(d, "EIDRM", EIDRM);
#endif
#ifdef ECHRNG
	/* Channel number out of range */
	insint(d, "ECHRNG", ECHRNG);
#endif
#ifdef EL2NSYNC
	/* Level 2 not synchronized */
	insint(d, "EL2NSYNC", EL2NSYNC);
#endif
#ifdef EL3HLT
	/* Level 3 halted */
	insint(d, "EL3HLT", EL3HLT);
#endif
#ifdef EL3RST
	/* Level 3 reset */
	insint(d, "EL3RST", EL3RST);
#endif
#ifdef ELNRNG
	/* Link number out of range */
	insint(d, "ELNRNG", ELNRNG);
#endif
#ifdef EUNATCH
	/* Protocol driver not attached */
	insint(d, "EUNATCH", EUNATCH);
#endif
#ifdef ENOCSI
	/* No CSI structure available */
	insint(d, "ENOCSI", ENOCSI);
#endif
#ifdef EL2HLT
	/* Level 2 halted */
	insint(d, "EL2HLT", EL2HLT);
#endif
#ifdef EBADE
	/* Invalid exchange */
	insint(d, "EBADE", EBADE);
#endif
#ifdef EBADR
	/* Invalid request descriptor */
	insint(d, "EBADR", EBADR);
#endif
#ifdef EXFULL
	/* Exchange full */
	insint(d, "EXFULL", EXFULL);
#endif
#ifdef ENOANO
	/* No anode */
	insint(d, "ENOANO", ENOANO);
#endif
#ifdef EBADRQC
	/* Invalid request code */
	insint(d, "EBADRQC", EBADRQC);
#endif
#ifdef EBADSLT
	/* Invalid slot */
	insint(d, "EBADSLT", EBADSLT);
#endif
#ifdef EDEADLOCK
	/* File locking deadlock error */
	insint(d, "EDEADLOCK", EDEADLOCK);
#endif
#ifdef EBFONT
	/* Bad font file format */
	insint(d, "EBFONT", EBFONT);
#endif
#ifdef ENOSTR
	/* Device not a stream */
	insint(d, "ENOSTR", ENOSTR);
#endif
#ifdef ENODATA
	/* No data available */
	insint(d, "ENODATA", ENODATA);
#endif
#ifdef ETIME
	/* Timer expired */
	insint(d, "ETIME", ETIME);
#endif
#ifdef ENOSR
	/* Out of streams resources */
	insint(d, "ENOSR", ENOSR);
#endif
#ifdef ENONET
	/* Machine is not on the network */
	insint(d, "ENONET", ENONET);
#endif
#ifdef ENOPKG
	/* Package not installed */
	insint(d, "ENOPKG", ENOPKG);
#endif
#ifdef EREMOTE
	/* Object is remote */
	insint(d, "EREMOTE", EREMOTE);
#endif
#ifdef ENOLINK
	/* Link has been severed */
	insint(d, "ENOLINK", ENOLINK);
#endif
#ifdef EADV
	/* Advertise error */
	insint(d, "EADV", EADV);
#endif
#ifdef ESRMNT
	/* Srmount error */
	insint(d, "ESRMNT", ESRMNT);
#endif
#ifdef ECOMM
	/* Communication error on send */
	insint(d, "ECOMM", ECOMM);
#endif
#ifdef EPROTO
	/* Protocol error */
	insint(d, "EPROTO", EPROTO);
#endif
#ifdef EMULTIHOP
	/* Multihop attempted */
	insint(d, "EMULTIHOP", EMULTIHOP);
#endif
#ifdef EDOTDOT
	/* RFS specific error */
	insint(d, "EDOTDOT", EDOTDOT);
#endif
#ifdef EBADMSG
	/* Not a data message */
	insint(d, "EBADMSG", EBADMSG);
#endif
#ifdef EOVERFLOW
	/* Value too large for defined data type */
	insint(d, "EOVERFLOW", EOVERFLOW);
#endif
#ifdef ENOTUNIQ
	/* Name not unique on network */
	insint(d, "ENOTUNIQ", ENOTUNIQ);
#endif
#ifdef EBADFD
	/* File descriptor in bad state */
	insint(d, "EBADFD", EBADFD);
#endif
#ifdef EREMCHG
	/* Remote address changed */
	insint(d, "EREMCHG", EREMCHG);
#endif
#ifdef ELIBACC
	/* Can not access a needed shared library */
	insint(d, "ELIBACC", ELIBACC);
#endif
#ifdef ELIBBAD
	/* Accessing a corrupted shared library */
	insint(d, "ELIBBAD", ELIBBAD);
#endif
#ifdef ELIBSCN
	/* .lib section in a.out corrupted */
	insint(d, "ELIBSCN", ELIBSCN);
#endif
#ifdef ELIBMAX
	/* Attempting to link in too many shared libraries */
	insint(d, "ELIBMAX", ELIBMAX);
#endif
#ifdef ELIBEXEC
	/* Cannot exec a shared library directly */
	insint(d, "ELIBEXEC", ELIBEXEC);
#endif
#ifdef EILSEQ
	/* Illegal byte sequence */
	insint(d, "EILSEQ", EILSEQ);
#endif
#ifdef ERESTART
	/* Interrupted system call should be restarted */
	insint(d, "ERESTART", ERESTART);
#endif
#ifdef ESTRPIPE
	/* Streams pipe error */
	insint(d, "ESTRPIPE", ESTRPIPE);
#endif
#ifdef EUSERS
	/* Too many users */
	insint(d, "EUSERS", EUSERS);
#endif
#ifdef ENOTSOCK
	/* Socket operation on non-socket */
	insint(d, "ENOTSOCK", ENOTSOCK);
#endif
#ifdef EDESTADDRREQ
	/* Destination address required */
	insint(d, "EDESTADDRREQ", EDESTADDRREQ);
#endif
#ifdef EMSGSIZE
	/* Message too long */
	insint(d, "EMSGSIZE", EMSGSIZE);
#endif
#ifdef EPROTOTYPE
	/* Protocol wrong type for socket */
	insint(d, "EPROTOTYPE", EPROTOTYPE);
#endif
#ifdef ENOPROTOOPT
	/* Protocol not available */
	insint(d, "ENOPROTOOPT", ENOPROTOOPT);
#endif
#ifdef EPROTONOSUPPORT
	/* Protocol not supported */
	insint(d, "EPROTONOSUPPORT", EPROTONOSUPPORT);
#endif
#ifdef ESOCKTNOSUPPORT
	/* Socket type not supported */
	insint(d, "ESOCKTNOSUPPORT", ESOCKTNOSUPPORT);
#endif
#ifdef EOPNOTSUPP
	/* Operation not supported on transport endpoint */
	insint(d, "EOPNOTSUPP", EOPNOTSUPP);
#endif
#ifdef EPFNOSUPPORT
	/* Protocol family not supported */
	insint(d, "EPFNOSUPPORT", EPFNOSUPPORT);
#endif
#ifdef EAFNOSUPPORT
	/* Address family not supported by protocol */
	insint(d, "EAFNOSUPPORT", EAFNOSUPPORT);
#endif
#ifdef EADDRINUSE
	/* Address already in use */
	insint(d, "EADDRINUSE", EADDRINUSE);
#endif
#ifdef EADDRNOTAVAIL
	/* Cannot assign requested address */
	insint(d, "EADDRNOTAVAIL", EADDRNOTAVAIL);
#endif
#ifdef ENETDOWN
	/* Network is down */
	insint(d, "ENETDOWN", ENETDOWN);
#endif
#ifdef ENETUNREACH
	/* Network is unreachable */
	insint(d, "ENETUNREACH", ENETUNREACH);
#endif
#ifdef ENETRESET
	/* Network dropped connection because of reset */
	insint(d, "ENETRESET", ENETRESET);
#endif
#ifdef ECONNABORTED
	/* Software caused connection abort */
	insint(d, "ECONNABORTED", ECONNABORTED);
#endif
#ifdef ECONNRESET
	/* Connection reset by peer */
	insint(d, "ECONNRESET", ECONNRESET);
#endif
#ifdef ENOBUFS
	/* No buffer space available */
	insint(d, "ENOBUFS", ENOBUFS);
#endif
#ifdef EISCONN
	/* Transport endpoint is already connected */
	insint(d, "EISCONN", EISCONN);
#endif
#ifdef ENOTCONN
	/* Transport endpoint is not connected */
	insint(d, "ENOTCONN", ENOTCONN);
#endif
#ifdef ESHUTDOWN
	/* Cannot send after transport endpoint shutdown */
	insint(d, "ESHUTDOWN", ESHUTDOWN);
#endif
#ifdef ETOOMANYREFS
	/* Too many references: cannot splice */
	insint(d, "ETOOMANYREFS", ETOOMANYREFS);
#endif
#ifdef ETIMEDOUT
	/* Connection timed out */
	insint(d, "ETIMEDOUT", ETIMEDOUT);
#endif
#ifdef ECONNREFUSED
	/* Connection refused */
	insint(d, "ECONNREFUSED", ECONNREFUSED);
#endif
#ifdef EHOSTDOWN
	/* Host is down */
	insint(d, "EHOSTDOWN", EHOSTDOWN);
#endif
#ifdef EHOSTUNREACH
	/* No route to host */
	insint(d, "EHOSTUNREACH", EHOSTUNREACH);
#endif
#ifdef EALREADY
	/* Operation already in progress */
	insint(d, "EALREADY", EALREADY);
#endif
#ifdef EINPROGRESS
	/* Operation now in progress */
	insint(d, "EINPROGRESS", EINPROGRESS);
#endif
#ifdef ESTALE
	/* Stale NFS file handle */
	insint(d, "ESTALE", ESTALE);
#endif
#ifdef EUCLEAN
	/* Structure needs cleaning */
	insint(d, "EUCLEAN", EUCLEAN);
#endif
#ifdef ENOTNAM
	/* Not a XENIX named type file */
	insint(d, "ENOTNAM", ENOTNAM);
#endif
#ifdef ENAVAIL
	/* No XENIX semaphores available */
	insint(d, "ENAVAIL", ENAVAIL);
#endif
#ifdef EISNAM
	/* Is a named type file */
	insint(d, "EISNAM", EISNAM);
#endif
#ifdef EREMOTEIO
	/* Remote I/O error */
	insint(d, "EREMOTEIO", EREMOTEIO);
#endif
#ifdef EDQUOT
	/* Quota exceeded */
	insint(d, "EDQUOT", EDQUOT);
#endif
}
