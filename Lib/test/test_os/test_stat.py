"""
Test os.stat(), os.fstat(), etc.
"""

import errno
import os
import pickle
import posix
import stat
import subprocess
import sys
import unittest
from test.support import os_helper
from .utils import create_file


# Test attributes on return values from os.*stat* family.
class StatAttributeTests(unittest.TestCase):
    def setUp(self):
        self.fname = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, self.fname)
        create_file(self.fname, b"ABC")

    def check_stat_attributes(self, fname):
        result = os.stat(fname)

        # Make sure direct access works
        self.assertEqual(result[stat.ST_SIZE], 3)
        self.assertEqual(result.st_size, 3)

        # Make sure all the attributes are there
        members = dir(result)
        for name in dir(stat):
            if name[:3] == 'ST_':
                attr = name.lower()
                if name.endswith("TIME"):
                    def trunc(x): return int(x)
                else:
                    def trunc(x): return x
                self.assertEqual(trunc(getattr(result, attr)),
                                  result[getattr(stat, name)])
                self.assertIn(attr, members)

        # Make sure that the st_?time and st_?time_ns fields roughly agree
        # (they should always agree up to around tens-of-microseconds)
        for name in 'st_atime st_mtime st_ctime'.split():
            floaty = int(getattr(result, name) * 100000)
            nanosecondy = getattr(result, name + "_ns") // 10000
            self.assertAlmostEqual(floaty, nanosecondy, delta=2)

        # Ensure both birthtime and birthtime_ns roughly agree, if present
        try:
            floaty = int(result.st_birthtime * 100000)
            nanosecondy = result.st_birthtime_ns // 10000
        except AttributeError:
            pass
        else:
            self.assertAlmostEqual(floaty, nanosecondy, delta=2)

        try:
            result[200]
            self.fail("No exception raised")
        except IndexError:
            pass

        # Make sure that assignment fails
        try:
            result.st_mode = 1
            self.fail("No exception raised")
        except AttributeError:
            pass

        try:
            result.st_rdev = 1
            self.fail("No exception raised")
        except (AttributeError, TypeError):
            pass

        try:
            result.parrot = 1
            self.fail("No exception raised")
        except AttributeError:
            pass

        # Use the stat_result constructor with a too-short tuple.
        try:
            result2 = os.stat_result((10,))
            self.fail("No exception raised")
        except TypeError:
            pass

        # Use the constructor with a too-long tuple.
        try:
            result2 = os.stat_result((0,1,2,3,4,5,6,7,8,9,10,11,12,13,14))
        except TypeError:
            pass

    def test_stat_attributes(self):
        self.check_stat_attributes(self.fname)

    def test_stat_attributes_bytes(self):
        try:
            fname = self.fname.encode(sys.getfilesystemencoding())
        except UnicodeEncodeError:
            self.skipTest("cannot encode %a for the filesystem" % self.fname)
        self.check_stat_attributes(fname)

    def test_stat_result_pickle(self):
        result = os.stat(self.fname)
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(f'protocol {proto}'):
                p = pickle.dumps(result, proto)
                self.assertIn(b'stat_result', p)
                if proto < 4:
                    self.assertIn(b'cos\nstat_result\n', p)
                unpickled = pickle.loads(p)
                self.assertEqual(result, unpickled)

    @unittest.skipUnless(hasattr(os, 'statvfs'), 'test needs os.statvfs()')
    def test_statvfs_attributes(self):
        result = os.statvfs(self.fname)

        # Make sure direct access works
        self.assertEqual(result.f_bfree, result[3])

        # Make sure all the attributes are there.
        members = ('bsize', 'frsize', 'blocks', 'bfree', 'bavail', 'files',
                    'ffree', 'favail', 'flag', 'namemax')
        for value, member in enumerate(members):
            self.assertEqual(getattr(result, 'f_' + member), result[value])

        self.assertTrue(isinstance(result.f_fsid, int))

        # Test that the size of the tuple doesn't change
        self.assertEqual(len(result), 10)

        # Make sure that assignment really fails
        try:
            result.f_bfree = 1
            self.fail("No exception raised")
        except AttributeError:
            pass

        try:
            result.parrot = 1
            self.fail("No exception raised")
        except AttributeError:
            pass

        # Use the constructor with a too-short tuple.
        try:
            result2 = os.statvfs_result((10,))
            self.fail("No exception raised")
        except TypeError:
            pass

        # Use the constructor with a too-long tuple.
        try:
            result2 = os.statvfs_result((0,1,2,3,4,5,6,7,8,9,10,11,12,13,14))
        except TypeError:
            pass

    @unittest.skipUnless(hasattr(os, 'statvfs'),
                         "need os.statvfs()")
    def test_statvfs_result_pickle(self):
        result = os.statvfs(self.fname)

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            p = pickle.dumps(result, proto)
            self.assertIn(b'statvfs_result', p)
            if proto < 4:
                self.assertIn(b'cos\nstatvfs_result\n', p)
            unpickled = pickle.loads(p)
            self.assertEqual(result, unpickled)

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
    def test_1686475(self):
        # Verify that an open file can be stat'ed
        try:
            os.stat(r"c:\pagefile.sys")
        except FileNotFoundError:
            self.skipTest(r'c:\pagefile.sys does not exist')
        except OSError as e:
            self.fail("Could not stat pagefile.sys")

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_15261(self):
        # Verify that stat'ing a closed fd does not cause crash
        r, w = os.pipe()
        try:
            os.stat(r)          # should not raise error
        finally:
            os.close(r)
            os.close(w)
        with self.assertRaises(OSError) as ctx:
            os.stat(r)
        self.assertEqual(ctx.exception.errno, errno.EBADF)

    def check_file_attributes(self, result):
        self.assertHasAttr(result, 'st_file_attributes')
        self.assertTrue(isinstance(result.st_file_attributes, int))
        self.assertTrue(0 <= result.st_file_attributes <= 0xFFFFFFFF)

    @unittest.skipUnless(sys.platform == "win32",
                         "st_file_attributes is Win32 specific")
    def test_file_attributes(self):
        # test file st_file_attributes (FILE_ATTRIBUTE_DIRECTORY not set)
        result = os.stat(self.fname)
        self.check_file_attributes(result)
        self.assertEqual(
            result.st_file_attributes & stat.FILE_ATTRIBUTE_DIRECTORY,
            0)

        # test directory st_file_attributes (FILE_ATTRIBUTE_DIRECTORY set)
        dirname = os_helper.TESTFN + "dir"
        os.mkdir(dirname)
        self.addCleanup(os.rmdir, dirname)

        result = os.stat(dirname)
        self.check_file_attributes(result)
        self.assertEqual(
            result.st_file_attributes & stat.FILE_ATTRIBUTE_DIRECTORY,
            stat.FILE_ATTRIBUTE_DIRECTORY)

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
    def test_access_denied(self):
        # Default to FindFirstFile WIN32_FIND_DATA when access is
        # denied. See issue 28075.
        # os.environ['TEMP'] should be located on a volume that
        # supports file ACLs.
        fname = os.path.join(os.environ['TEMP'], self.fname + "_access")
        self.addCleanup(os_helper.unlink, fname)
        create_file(fname, b'ABC')
        # Deny the right to [S]YNCHRONIZE on the file to
        # force CreateFile to fail with ERROR_ACCESS_DENIED.
        DETACHED_PROCESS = 8
        subprocess.check_call(
            # bpo-30584: Use security identifier *S-1-5-32-545 instead
            # of localized "Users" to not depend on the locale.
            ['icacls.exe', fname, '/deny', '*S-1-5-32-545:(S)'],
            creationflags=DETACHED_PROCESS
        )
        result = os.stat(fname)
        self.assertNotEqual(result.st_size, 0)
        self.assertTrue(os.path.isfile(fname))

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
    def test_stat_block_device(self):
        # bpo-38030: os.stat fails for block devices
        # Test a filename like "//./C:"
        fname = "//./" + os.path.splitdrive(os.getcwd())[0]
        result = os.stat(fname)
        self.assertEqual(result.st_mode, stat.S_IFBLK)


class PosixTester(unittest.TestCase):

    def setUp(self):
        # create empty file
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        create_file(os_helper.TESTFN, b'')

    def test_stat(self):
        self.assertTrue(posix.stat(os_helper.TESTFN))
        self.assertTrue(posix.stat(os.fsencode(os_helper.TESTFN)))

        self.assertRaisesRegex(TypeError,
                'should be string, bytes, os.PathLike or integer, not',
                posix.stat, bytearray(os.fsencode(os_helper.TESTFN)))
        self.assertRaisesRegex(TypeError,
                'should be string, bytes, os.PathLike or integer, not',
                posix.stat, None)
        self.assertRaisesRegex(TypeError,
                'should be string, bytes, os.PathLike or integer, not',
                posix.stat, list(os_helper.TESTFN))
        self.assertRaisesRegex(TypeError,
                'should be string, bytes, os.PathLike or integer, not',
                posix.stat, list(os.fsencode(os_helper.TESTFN)))

    @unittest.skipUnless(hasattr(posix, 'fstat'),
                         'test needs posix.fstat()')
    def test_fstat(self):
        fp = open(os_helper.TESTFN)
        try:
            self.assertTrue(posix.fstat(fp.fileno()))
            self.assertTrue(posix.stat(fp.fileno()))

            self.assertRaisesRegex(TypeError,
                    'should be string, bytes, os.PathLike or integer, not',
                    posix.stat, float(fp.fileno()))
        finally:
            fp.close()


if __name__ == "__main__":
    unittest.main()
