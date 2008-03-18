#! /usr/bin/env python
"""Test the errno module
   Roger E. Masse
"""

import errno
from test import test_support
import unittest

errors = ['E2BIG', 'EACCES', 'EADDRINUSE', 'EADDRNOTAVAIL', 'EADV',
          'EAFNOSUPPORT', 'EAGAIN', 'EALREADY', 'EBADE', 'EBADF',
          'EBADFD', 'EBADMSG', 'EBADR', 'EBADRQC', 'EBADSLT',
          'EBFONT', 'EBUSY', 'ECHILD', 'ECHRNG', 'ECOMM',
          'ECONNABORTED', 'ECONNREFUSED', 'ECONNRESET',
          'EDEADLK', 'EDEADLOCK', 'EDESTADDRREQ', 'EDOTDOT', 'EDOM',
          'EDQUOT', 'EEXIST', 'EFAULT', 'EFBIG', 'EHOSTDOWN',
          'EHOSTUNREACH', 'EIDRM', 'EILSEQ', 'EINPROGRESS',
          'EINTR', 'EINVAL', 'EIO', 'EISCONN', 'EISDIR', 'EISNAM',
          'EL2HLT', 'EL2NSYNC', 'EL3HLT', 'EL3RST', 'ELIBACC',
          'ELIBBAD', 'ELIBEXEC', 'ELIBMAX', 'ELIBSCN', 'ELNRNG',
          'ELOOP', 'EMFILE', 'EMLINK', 'EMSGSIZE', 'EMULTIHOP',
          'ENAMETOOLONG', 'ENAVAIL', 'ENETDOWN', 'ENETRESET', 'ENETUNREACH',
          'ENFILE', 'ENOANO', 'ENOBUFS', 'ENOCSI', 'ENODATA',
          'ENODEV', 'ENOENT', 'ENOEXEC', 'ENOLCK', 'ENOLINK',
          'ENOMEM', 'ENOMSG', 'ENONET', 'ENOPKG', 'ENOPROTOOPT',
          'ENOSPC', 'ENOSR', 'ENOSTR', 'ENOSYS', 'ENOTBLK',
          'ENOTCONN', 'ENOTDIR', 'ENOTEMPTY', 'ENOTNAM', 'ENOTOBACCO', 'ENOTSOCK',
          'ENOTTY', 'ENOTUNIQ', 'ENXIO', 'EOPNOTSUPP',
          'EOVERFLOW', 'EPERM', 'EPFNOSUPPORT', 'EPIPE',
          'EPROTO', 'EPROTONOSUPPORT', 'EPROTOTYPE',
          'ERANGE', 'EREMCHG', 'EREMOTE', 'EREMOTEIO', 'ERESTART',
          'EROFS', 'ESHUTDOWN', 'ESOCKTNOSUPPORT', 'ESPIPE',
          'ESRCH', 'ESRMNT', 'ESTALE', 'ESTRPIPE', 'ETIME',
          'ETIMEDOUT', 'ETOOMANYREFS', 'ETXTBSY', 'EUCLEAN', 'EUNATCH',
          'EUSERS', 'EWOULDBLOCK', 'EXDEV', 'EXFULL']


class ErrnoAttributeTests(unittest.TestCase):

    def test_for_improper_attributes(self):
        # No unexpected attributes should be on the module.
        errors_set = set(errors)
        for attribute in errno.__dict__.iterkeys():
            if attribute.isupper():
                self.assert_(attribute in errors_set,
                                "%s is an unexpected error value" % attribute)

    def test_using_errorcode(self):
        # Every key value in errno.errorcode should be on the module.
        for value in errno.errorcode.itervalues():
            self.assert_(hasattr(errno, value), 'no %s attr in errno' % value)


class ErrorcodeTests(unittest.TestCase):

    def test_attributes_in_errorcode(self):
        for attribute in errno.__dict__.iterkeys():
            if attribute.isupper():
                self.assert_(getattr(errno, attribute) in errno.errorcode,
                             'no %s attr in errno.errorcode' % attribute)


def test_main():
    test_support.run_unittest(ErrnoAttributeTests, ErrorcodeTests)


if __name__ == '__main__':
    test_main()
