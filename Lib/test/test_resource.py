import contextlib
import sys
import unittest
from test import support
from test.support import import_helper
from test.support import os_helper
import time

resource = import_helper.import_module('resource')

# This test is checking a few specific problem spots with the resource module.

class ResourceTest(unittest.TestCase):

    def test_args(self):
        self.assertRaises(TypeError, resource.getrlimit)
        self.assertRaises(TypeError, resource.getrlimit, 42, 42)
        self.assertRaises(TypeError, resource.setrlimit)
        self.assertRaises(TypeError, resource.setrlimit, 42, 42, 42)

    @unittest.skipIf(sys.platform == "vxworks",
                     "setting RLIMIT_FSIZE is not supported on VxWorks")
    def test_fsize_ismax(self):
        try:
            (cur, max) = resource.getrlimit(resource.RLIMIT_FSIZE)
        except AttributeError:
            pass
        else:
            # RLIMIT_FSIZE should be RLIM_INFINITY, which will be a really big
            # number on a platform with large file support.  On these platforms,
            # we need to test that the get/setrlimit functions properly convert
            # the number to a C long long and that the conversion doesn't raise
            # an error.
            self.assertEqual(resource.RLIM_INFINITY, max)
            resource.setrlimit(resource.RLIMIT_FSIZE, (cur, max))

    def test_fsize_enforced(self):
        try:
            (cur, max) = resource.getrlimit(resource.RLIMIT_FSIZE)
        except AttributeError:
            pass
        else:
            # Check to see what happens when the RLIMIT_FSIZE is small.  Some
            # versions of Python were terminated by an uncaught SIGXFSZ, but
            # pythonrun.c has been fixed to ignore that exception.  If so, the
            # write() should return EFBIG when the limit is exceeded.

            # At least one platform has an unlimited RLIMIT_FSIZE and attempts
            # to change it raise ValueError instead.
            try:
                try:
                    resource.setrlimit(resource.RLIMIT_FSIZE, (1024, max))
                    limit_set = True
                except ValueError:
                    limit_set = False
                f = open(os_helper.TESTFN, "wb")
                try:
                    f.write(b"X" * 1024)
                    try:
                        f.write(b"Y")
                        f.flush()
                        # On some systems (e.g., Ubuntu on hppa) the flush()
                        # doesn't always cause the exception, but the close()
                        # does eventually.  Try flushing several times in
                        # an attempt to ensure the file is really synced and
                        # the exception raised.
                        for i in range(5):
                            time.sleep(.1)
                            f.flush()
                    except OSError:
                        if not limit_set:
                            raise
                    if limit_set:
                        # Close will attempt to flush the byte we wrote
                        # Restore limit first to avoid getting a spurious error
                        resource.setrlimit(resource.RLIMIT_FSIZE, (cur, max))
                finally:
                    f.close()
            finally:
                if limit_set:
                    resource.setrlimit(resource.RLIMIT_FSIZE, (cur, max))
                os_helper.unlink(os_helper.TESTFN)

    def test_fsize_toobig(self):
        # Be sure that setrlimit is checking for really large values
        too_big = 10**50
        try:
            (cur, max) = resource.getrlimit(resource.RLIMIT_FSIZE)
        except AttributeError:
            pass
        else:
            try:
                resource.setrlimit(resource.RLIMIT_FSIZE, (too_big, max))
            except (OverflowError, ValueError):
                pass
            try:
                resource.setrlimit(resource.RLIMIT_FSIZE, (max, too_big))
            except (OverflowError, ValueError):
                pass

    @unittest.skipUnless(hasattr(resource, "getrusage"), "needs getrusage")
    def test_getrusage(self):
        self.assertRaises(TypeError, resource.getrusage)
        self.assertRaises(TypeError, resource.getrusage, 42, 42)
        usageself = resource.getrusage(resource.RUSAGE_SELF)
        usagechildren = resource.getrusage(resource.RUSAGE_CHILDREN)
        # May not be available on all systems.
        try:
            usageboth = resource.getrusage(resource.RUSAGE_BOTH)
        except (ValueError, AttributeError):
            pass
        try:
            usage_thread = resource.getrusage(resource.RUSAGE_THREAD)
        except (ValueError, AttributeError):
            pass

    # Issue 6083: Reference counting bug
    @unittest.skipIf(sys.platform == "vxworks",
                     "setting RLIMIT_CPU is not supported on VxWorks")
    def test_setrusage_refcount(self):
        try:
            limits = resource.getrlimit(resource.RLIMIT_CPU)
        except AttributeError:
            pass
        else:
            class BadSequence:
                def __len__(self):
                    return 2
                def __getitem__(self, key):
                    if key in (0, 1):
                        return len(tuple(range(1000000)))
                    raise IndexError

            resource.setrlimit(resource.RLIMIT_CPU, BadSequence())

    def test_pagesize(self):
        pagesize = resource.getpagesize()
        self.assertIsInstance(pagesize, int)
        self.assertGreaterEqual(pagesize, 0)

    @unittest.skipUnless(sys.platform in ('linux', 'android'), 'Linux only')
    def test_linux_constants(self):
        for attr in ['MSGQUEUE', 'NICE', 'RTPRIO', 'RTTIME', 'SIGPENDING']:
            with contextlib.suppress(AttributeError):
                self.assertIsInstance(getattr(resource, 'RLIMIT_' + attr), int)

    def test_freebsd_contants(self):
        for attr in ['SWAP', 'SBSIZE', 'NPTS']:
            with contextlib.suppress(AttributeError):
                self.assertIsInstance(getattr(resource, 'RLIMIT_' + attr), int)

    @unittest.skipUnless(hasattr(resource, 'prlimit'), 'no prlimit')
    @support.requires_linux_version(2, 6, 36)
    def test_prlimit(self):
        self.assertRaises(TypeError, resource.prlimit)
        self.assertRaises(ProcessLookupError, resource.prlimit,
                          -1, resource.RLIMIT_AS)
        limit = resource.getrlimit(resource.RLIMIT_AS)
        self.assertEqual(resource.prlimit(0, resource.RLIMIT_AS), limit)
        self.assertEqual(resource.prlimit(0, resource.RLIMIT_AS, limit),
                         limit)

    # Issue 20191: Reference counting bug
    @unittest.skipUnless(hasattr(resource, 'prlimit'), 'no prlimit')
    @support.requires_linux_version(2, 6, 36)
    def test_prlimit_refcount(self):
        class BadSeq:
            def __len__(self):
                return 2
            def __getitem__(self, key):
                return limits[key] - 1  # new reference

        limits = resource.getrlimit(resource.RLIMIT_AS)
        self.assertEqual(resource.prlimit(0, resource.RLIMIT_AS, BadSeq()),
                         limits)


if __name__ == "__main__":
    unittest.main()
