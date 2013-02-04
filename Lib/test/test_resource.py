import unittest
from test import test_support
import time

resource = test_support.import_module('resource')

# This test is checking a few specific problem spots with the resource module.

class ResourceTest(unittest.TestCase):

    def test_args(self):
        self.assertRaises(TypeError, resource.getrlimit)
        self.assertRaises(TypeError, resource.getrlimit, 42, 42)
        self.assertRaises(TypeError, resource.setrlimit)
        self.assertRaises(TypeError, resource.setrlimit, 42, 42, 42)

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
                f = open(test_support.TESTFN, "wb")
                try:
                    f.write("X" * 1024)
                    try:
                        f.write("Y")
                        f.flush()
                        # On some systems (e.g., Ubuntu on hppa) the flush()
                        # doesn't always cause the exception, but the close()
                        # does eventually.  Try flushing several times in
                        # an attempt to ensure the file is really synced and
                        # the exception raised.
                        for i in range(5):
                            time.sleep(.1)
                            f.flush()
                    except IOError:
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
                test_support.unlink(test_support.TESTFN)

    def test_fsize_toobig(self):
        # Be sure that setrlimit is checking for really large values
        too_big = 10L**50
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

    # Issue 6083: Reference counting bug
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

def test_main(verbose=None):
    test_support.run_unittest(ResourceTest)

if __name__ == "__main__":
    test_main()
