#! /usr/bin/env python
"""Test the errno module
   Roger E. Masse
"""

import errno
from test.test_support import verbose

errors = ['E2BIG', 'EACCES', 'EADDRINUSE', 'EADDRNOTAVAIL', 'EADV',
          'EAFNOSUPPORT', 'EAGAIN', 'EALREADY', 'EBADE', 'EBADF',
          'EBADFD', 'EBADMSG', 'EBADR', 'EBADRQC', 'EBADSLT',
          'EBFONT', 'EBUSY', 'ECHILD', 'ECHRNG', 'ECOMM',
          'ECONNABORTED', 'ECONNREFUSED', 'ECONNRESET',
          'EDEADLK', 'EDEADLOCK', 'EDESTADDRREQ', 'EDOM',
          'EDQUOT', 'EEXIST', 'EFAULT', 'EFBIG', 'EHOSTDOWN',
          'EHOSTUNREACH', 'EIDRM', 'EILSEQ', 'EINPROGRESS',
          'EINTR', 'EINVAL', 'EIO', 'EISCONN', 'EISDIR',
          'EL2HLT', 'EL2NSYNC', 'EL3HLT', 'EL3RST', 'ELIBACC',
          'ELIBBAD', 'ELIBEXEC', 'ELIBMAX', 'ELIBSCN', 'ELNRNG',
          'ELOOP', 'EMFILE', 'EMLINK', 'EMSGSIZE', 'EMULTIHOP',
          'ENAMETOOLONG', 'ENETDOWN', 'ENETRESET', 'ENETUNREACH',
          'ENFILE', 'ENOANO', 'ENOBUFS', 'ENOCSI', 'ENODATA',
          'ENODEV', 'ENOENT', 'ENOEXEC', 'ENOLCK', 'ENOLINK',
          'ENOMEM', 'ENOMSG', 'ENONET', 'ENOPKG', 'ENOPROTOOPT',
          'ENOSPC', 'ENOSR', 'ENOSTR', 'ENOSYS', 'ENOTBLK',
          'ENOTCONN', 'ENOTDIR', 'ENOTEMPTY', 'ENOTOBACCO', 'ENOTSOCK',
          'ENOTTY', 'ENOTUNIQ', 'ENXIO', 'EOPNOTSUPP',
          'EOVERFLOW', 'EPERM', 'EPFNOSUPPORT', 'EPIPE',
          'EPROTO', 'EPROTONOSUPPORT', 'EPROTOTYPE',
          'ERANGE', 'EREMCHG', 'EREMOTE', 'ERESTART',
          'EROFS', 'ESHUTDOWN', 'ESOCKTNOSUPPORT', 'ESPIPE',
          'ESRCH', 'ESRMNT', 'ESTALE', 'ESTRPIPE', 'ETIME',
          'ETIMEDOUT', 'ETOOMANYREFS', 'ETXTBSY', 'EUNATCH',
          'EUSERS', 'EWOULDBLOCK', 'EXDEV', 'EXFULL']

#
# This is a wee bit bogus since the module only conditionally adds
# errno constants if they have been defined by errno.h  However, this
# test seems to work on SGI, Sparc & intel Solaris, and linux.
#
for error in errors:
    try:
        a = getattr(errno, error)
    except AttributeError:
        if verbose:
            print '%s: not found' % error
    else:
        if verbose:
            print '%s: %d' % (error, a)
