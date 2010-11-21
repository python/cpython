# As a test suite for the os module, this is woefully inadequate, but this
# does add tests for a few functions which have been determined to be more
# portable than they had been thought to be.

import os
import errno
import unittest
import warnings
import sys
import shutil
from test import support

# Detect whether we're on a Linux system that uses the (now outdated
# and unmaintained) linuxthreads threading library.  There's an issue
# when combining linuxthreads with a failed execv call: see
# http://bugs.python.org/issue4970.
if (hasattr(os, "confstr_names") and
    "CS_GNU_LIBPTHREAD_VERSION" in os.confstr_names):
    libpthread = os.confstr("CS_GNU_LIBPTHREAD_VERSION")
    USING_LINUXTHREADS= libpthread.startswith("linuxthreads")
else:
    USING_LINUXTHREADS= False

# Tests creating TESTFN
class FileTests(unittest.TestCase):
    def setUp(self):
        if os.path.exists(support.TESTFN):
            os.unlink(support.TESTFN)
    tearDown = setUp

    def test_access(self):
        f = os.open(support.TESTFN, os.O_CREAT|os.O_RDWR)
        os.close(f)
        self.assertTrue(os.access(support.TESTFN, os.W_OK))

    def test_closerange(self):
        first = os.open(support.TESTFN, os.O_CREAT|os.O_RDWR)
        # We must allocate two consecutive file descriptors, otherwise
        # it will mess up other file descriptors (perhaps even the three
        # standard ones).
        second = os.dup(first)
        try:
            retries = 0
            while second != first + 1:
                os.close(first)
                retries += 1
                if retries > 10:
                    # XXX test skipped
                    self.skipTest("couldn't allocate two consecutive fds")
                first, second = second, os.dup(second)
        finally:
            os.close(second)
        # close a fd that is open, and one that isn't
        os.closerange(first, first + 2)
        self.assertRaises(OSError, os.write, first, b"a")

    def test_rename(self):
        path = support.TESTFN
        old = sys.getrefcount(path)
        self.assertRaises(TypeError, os.rename, path, 0)
        new = sys.getrefcount(path)
        self.assertEqual(old, new)

    def test_read(self):
        with open(support.TESTFN, "w+b") as fobj:
            fobj.write(b"spam")
            fobj.flush()
            fd = fobj.fileno()
            os.lseek(fd, 0, 0)
            s = os.read(fd, 4)
            self.assertEqual(type(s), bytes)
            self.assertEqual(s, b"spam")

    def test_write(self):
        # os.write() accepts bytes- and buffer-like objects but not strings
        fd = os.open(support.TESTFN, os.O_CREAT | os.O_WRONLY)
        self.assertRaises(TypeError, os.write, fd, "beans")
        os.write(fd, b"bacon\n")
        os.write(fd, bytearray(b"eggs\n"))
        os.write(fd, memoryview(b"spam\n"))
        os.close(fd)
        with open(support.TESTFN, "rb") as fobj:
            self.assertEqual(fobj.read().splitlines(),
                [b"bacon", b"eggs", b"spam"])


class TemporaryFileTests(unittest.TestCase):
    def setUp(self):
        self.files = []
        os.mkdir(support.TESTFN)

    def tearDown(self):
        for name in self.files:
            os.unlink(name)
        os.rmdir(support.TESTFN)

    def check_tempfile(self, name):
        # make sure it doesn't already exist:
        self.assertFalse(os.path.exists(name),
                    "file already exists for temporary file")
        # make sure we can create the file
        open(name, "w")
        self.files.append(name)

    def test_tempnam(self):
        if not hasattr(os, "tempnam"):
            return
        warnings.filterwarnings("ignore", "tempnam", RuntimeWarning,
                                r"test_os$")
        self.check_tempfile(os.tempnam())

        name = os.tempnam(support.TESTFN)
        self.check_tempfile(name)

        name = os.tempnam(support.TESTFN, "pfx")
        self.assertTrue(os.path.basename(name)[:3] == "pfx")
        self.check_tempfile(name)

    def test_tmpfile(self):
        if not hasattr(os, "tmpfile"):
            return
        # As with test_tmpnam() below, the Windows implementation of tmpfile()
        # attempts to create a file in the root directory of the current drive.
        # On Vista and Server 2008, this test will always fail for normal users
        # as writing to the root directory requires elevated privileges.  With
        # XP and below, the semantics of tmpfile() are the same, but the user
        # running the test is more likely to have administrative privileges on
        # their account already.  If that's the case, then os.tmpfile() should
        # work.  In order to make this test as useful as possible, rather than
        # trying to detect Windows versions or whether or not the user has the
        # right permissions, just try and create a file in the root directory
        # and see if it raises a 'Permission denied' OSError.  If it does, then
        # test that a subsequent call to os.tmpfile() raises the same error. If
        # it doesn't, assume we're on XP or below and the user running the test
        # has administrative privileges, and proceed with the test as normal.
        if sys.platform == 'win32':
            name = '\\python_test_os_test_tmpfile.txt'
            if os.path.exists(name):
                os.remove(name)
            try:
                fp = open(name, 'w')
            except IOError as first:
                # open() failed, assert tmpfile() fails in the same way.
                # Although open() raises an IOError and os.tmpfile() raises an
                # OSError(), 'args' will be (13, 'Permission denied') in both
                # cases.
                try:
                    fp = os.tmpfile()
                except OSError as second:
                    self.assertEqual(first.args, second.args)
                else:
                    self.fail("expected os.tmpfile() to raise OSError")
                return
            else:
                # open() worked, therefore, tmpfile() should work.  Close our
                # dummy file and proceed with the test as normal.
                fp.close()
                os.remove(name)

        fp = os.tmpfile()
        fp.write("foobar")
        fp.seek(0,0)
        s = fp.read()
        fp.close()
        self.assertTrue(s == "foobar")

    def test_tmpnam(self):
        import sys
        if not hasattr(os, "tmpnam"):
            return
        warnings.filterwarnings("ignore", "tmpnam", RuntimeWarning,
                                r"test_os$")
        name = os.tmpnam()
        if sys.platform in ("win32",):
            # The Windows tmpnam() seems useless.  From the MS docs:
            #
            #     The character string that tmpnam creates consists of
            #     the path prefix, defined by the entry P_tmpdir in the
            #     file STDIO.H, followed by a sequence consisting of the
            #     digit characters '0' through '9'; the numerical value
            #     of this string is in the range 1 - 65,535.  Changing the
            #     definitions of L_tmpnam or P_tmpdir in STDIO.H does not
            #     change the operation of tmpnam.
            #
            # The really bizarre part is that, at least under MSVC6,
            # P_tmpdir is "\\".  That is, the path returned refers to
            # the root of the current drive.  That's a terrible place to
            # put temp files, and, depending on privileges, the user
            # may not even be able to open a file in the root directory.
            self.assertFalse(os.path.exists(name),
                        "file already exists for temporary file")
        else:
            self.check_tempfile(name)

    def fdopen_helper(self, *args):
        fd = os.open(support.TESTFN, os.O_RDONLY)
        fp2 = os.fdopen(fd, *args)
        fp2.close()

    def test_fdopen(self):
        self.fdopen_helper()
        self.fdopen_helper('r')
        self.fdopen_helper('r', 100)

# Test attributes on return values from os.*stat* family.
class StatAttributeTests(unittest.TestCase):
    def setUp(self):
        os.mkdir(support.TESTFN)
        self.fname = os.path.join(support.TESTFN, "f1")
        f = open(self.fname, 'wb')
        f.write(b"ABC")
        f.close()

    def tearDown(self):
        os.unlink(self.fname)
        os.rmdir(support.TESTFN)

    def test_stat_attributes(self):
        if not hasattr(os, "stat"):
            return

        import stat
        result = os.stat(self.fname)

        # Make sure direct access works
        self.assertEqual(result[stat.ST_SIZE], 3)
        self.assertEqual(result.st_size, 3)

        import sys

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
                self.assertTrue(attr in members)

        try:
            result[200]
            self.fail("No exception thrown")
        except IndexError:
            pass

        # Make sure that assignment fails
        try:
            result.st_mode = 1
            self.fail("No exception thrown")
        except AttributeError:
            pass

        try:
            result.st_rdev = 1
            self.fail("No exception thrown")
        except (AttributeError, TypeError):
            pass

        try:
            result.parrot = 1
            self.fail("No exception thrown")
        except AttributeError:
            pass

        # Use the stat_result constructor with a too-short tuple.
        try:
            result2 = os.stat_result((10,))
            self.fail("No exception thrown")
        except TypeError:
            pass

        # Use the constructr with a too-long tuple.
        try:
            result2 = os.stat_result((0,1,2,3,4,5,6,7,8,9,10,11,12,13,14))
        except TypeError:
            pass


    def test_statvfs_attributes(self):
        if not hasattr(os, "statvfs"):
            return

        try:
            result = os.statvfs(self.fname)
        except OSError as e:
            # On AtheOS, glibc always returns ENOSYS
            if e.errno == errno.ENOSYS:
                return

        # Make sure direct access works
        self.assertEqual(result.f_bfree, result[3])

        # Make sure all the attributes are there.
        members = ('bsize', 'frsize', 'blocks', 'bfree', 'bavail', 'files',
                    'ffree', 'favail', 'flag', 'namemax')
        for value, member in enumerate(members):
            self.assertEqual(getattr(result, 'f_' + member), result[value])

        # Make sure that assignment really fails
        try:
            result.f_bfree = 1
            self.fail("No exception thrown")
        except AttributeError:
            pass

        try:
            result.parrot = 1
            self.fail("No exception thrown")
        except AttributeError:
            pass

        # Use the constructor with a too-short tuple.
        try:
            result2 = os.statvfs_result((10,))
            self.fail("No exception thrown")
        except TypeError:
            pass

        # Use the constructr with a too-long tuple.
        try:
            result2 = os.statvfs_result((0,1,2,3,4,5,6,7,8,9,10,11,12,13,14))
        except TypeError:
            pass

    def test_utime_dir(self):
        delta = 1000000
        st = os.stat(support.TESTFN)
        # round to int, because some systems may support sub-second
        # time stamps in stat, but not in utime.
        os.utime(support.TESTFN, (st.st_atime, int(st.st_mtime-delta)))
        st2 = os.stat(support.TESTFN)
        self.assertEqual(st2.st_mtime, int(st.st_mtime-delta))

    # Restrict test to Win32, since there is no guarantee other
    # systems support centiseconds
    if sys.platform == 'win32':
        def get_file_system(path):
            root = os.path.splitdrive(os.path.abspath(path))[0] + '\\'
            import ctypes
            kernel32 = ctypes.windll.kernel32
            buf = ctypes.create_unicode_buffer("", 100)
            if kernel32.GetVolumeInformationW(root, None, 0, None, None, None, buf, len(buf)):
                return buf.value

        if get_file_system(support.TESTFN) == "NTFS":
            def test_1565150(self):
                t1 = 1159195039.25
                os.utime(self.fname, (t1, t1))
                self.assertEqual(os.stat(self.fname).st_mtime, t1)

        def test_1686475(self):
            # Verify that an open file can be stat'ed
            try:
                os.stat(r"c:\pagefile.sys")
            except WindowsError as e:
                if e.errno == 2: # file does not exist; cannot run test
                    return
                self.fail("Could not stat pagefile.sys")

from test import mapping_tests

class EnvironTests(mapping_tests.BasicTestMappingProtocol):
    """check that os.environ object conform to mapping protocol"""
    type2test = None

    def setUp(self):
        self.__save = dict(os.environ)
        for key, value in self._reference().items():
            os.environ[key] = value

    def tearDown(self):
        os.environ.clear()
        os.environ.update(self.__save)

    def _reference(self):
        return {"KEY1":"VALUE1", "KEY2":"VALUE2", "KEY3":"VALUE3"}

    def _empty_mapping(self):
        os.environ.clear()
        return os.environ

    # Bug 1110478
    def test_update2(self):
        os.environ.clear()
        if os.path.exists("/bin/sh"):
            os.environ.update(HELLO="World")
            with os.popen("/bin/sh -c 'echo $HELLO'") as popen:
                value = popen.read().strip()
                self.assertEqual(value, "World")

    def test_os_popen_iter(self):
        if os.path.exists("/bin/sh"):
            with os.popen(
                "/bin/sh -c 'echo \"line1\nline2\nline3\"'") as popen:
                it = iter(popen)
                self.assertEqual(next(it), "line1\n")
                self.assertEqual(next(it), "line2\n")
                self.assertEqual(next(it), "line3\n")
                self.assertRaises(StopIteration, next, it)

    # Verify environ keys and values from the OS are of the
    # correct str type.
    def test_keyvalue_types(self):
        for key, val in os.environ.items():
            self.assertEqual(type(key), str)
            self.assertEqual(type(val), str)

    def test_items(self):
        for key, value in self._reference().items():
            self.assertEqual(os.environ.get(key), value)

    # Issue 7310
    def test___repr__(self):
        """Check that the repr() of os.environ looks like environ({...})."""
        env = os.environ
        self.assertTrue(isinstance(env.data, dict))
        self.assertEqual(repr(env), 'environ({!r})'.format(env.data))


class WalkTests(unittest.TestCase):
    """Tests for os.walk()."""

    def test_traversal(self):
        import os
        from os.path import join

        # Build:
        #     TESTFN/
        #       TEST1/              a file kid and two directory kids
        #         tmp1
        #         SUB1/             a file kid and a directory kid
        #           tmp2
        #           SUB11/          no kids
        #         SUB2/             a file kid and a dirsymlink kid
        #           tmp3
        #           link/           a symlink to TESTFN.2
        #       TEST2/
        #         tmp4              a lone file
        walk_path = join(support.TESTFN, "TEST1")
        sub1_path = join(walk_path, "SUB1")
        sub11_path = join(sub1_path, "SUB11")
        sub2_path = join(walk_path, "SUB2")
        tmp1_path = join(walk_path, "tmp1")
        tmp2_path = join(sub1_path, "tmp2")
        tmp3_path = join(sub2_path, "tmp3")
        link_path = join(sub2_path, "link")
        t2_path = join(support.TESTFN, "TEST2")
        tmp4_path = join(support.TESTFN, "TEST2", "tmp4")

        # Create stuff.
        os.makedirs(sub11_path)
        os.makedirs(sub2_path)
        os.makedirs(t2_path)
        for path in tmp1_path, tmp2_path, tmp3_path, tmp4_path:
            f = open(path, "w")
            f.write("I'm " + path + " and proud of it.  Blame test_os.\n")
            f.close()
        if hasattr(os, "symlink"):
            os.symlink(os.path.abspath(t2_path), link_path)
            sub2_tree = (sub2_path, ["link"], ["tmp3"])
        else:
            sub2_tree = (sub2_path, [], ["tmp3"])

        # Walk top-down.
        all = list(os.walk(walk_path))
        self.assertEqual(len(all), 4)
        # We can't know which order SUB1 and SUB2 will appear in.
        # Not flipped:  TESTFN, SUB1, SUB11, SUB2
        #     flipped:  TESTFN, SUB2, SUB1, SUB11
        flipped = all[0][1][0] != "SUB1"
        all[0][1].sort()
        self.assertEqual(all[0], (walk_path, ["SUB1", "SUB2"], ["tmp1"]))
        self.assertEqual(all[1 + flipped], (sub1_path, ["SUB11"], ["tmp2"]))
        self.assertEqual(all[2 + flipped], (sub11_path, [], []))
        self.assertEqual(all[3 - 2 * flipped], sub2_tree)

        # Prune the search.
        all = []
        for root, dirs, files in os.walk(walk_path):
            all.append((root, dirs, files))
            # Don't descend into SUB1.
            if 'SUB1' in dirs:
                # Note that this also mutates the dirs we appended to all!
                dirs.remove('SUB1')
        self.assertEqual(len(all), 2)
        self.assertEqual(all[0], (walk_path, ["SUB2"], ["tmp1"]))
        self.assertEqual(all[1], sub2_tree)

        # Walk bottom-up.
        all = list(os.walk(walk_path, topdown=False))
        self.assertEqual(len(all), 4)
        # We can't know which order SUB1 and SUB2 will appear in.
        # Not flipped:  SUB11, SUB1, SUB2, TESTFN
        #     flipped:  SUB2, SUB11, SUB1, TESTFN
        flipped = all[3][1][0] != "SUB1"
        all[3][1].sort()
        self.assertEqual(all[3], (walk_path, ["SUB1", "SUB2"], ["tmp1"]))
        self.assertEqual(all[flipped], (sub11_path, [], []))
        self.assertEqual(all[flipped + 1], (sub1_path, ["SUB11"], ["tmp2"]))
        self.assertEqual(all[2 - 2 * flipped], sub2_tree)

        if hasattr(os, "symlink"):
            # Walk, following symlinks.
            for root, dirs, files in os.walk(walk_path, followlinks=True):
                if root == link_path:
                    self.assertEqual(dirs, [])
                    self.assertEqual(files, ["tmp4"])
                    break
            else:
                self.fail("Didn't follow symlink with followlinks=True")

    def tearDown(self):
        # Tear everything down.  This is a decent use for bottom-up on
        # Windows, which doesn't have a recursive delete command.  The
        # (not so) subtlety is that rmdir will fail unless the dir's
        # kids are removed first, so bottom up is essential.
        for root, dirs, files in os.walk(support.TESTFN, topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                dirname = os.path.join(root, name)
                if not os.path.islink(dirname):
                    os.rmdir(dirname)
                else:
                    os.remove(dirname)
        os.rmdir(support.TESTFN)

class MakedirTests(unittest.TestCase):
    def setUp(self):
        os.mkdir(support.TESTFN)

    def test_makedir(self):
        base = support.TESTFN
        path = os.path.join(base, 'dir1', 'dir2', 'dir3')
        os.makedirs(path)             # Should work
        path = os.path.join(base, 'dir1', 'dir2', 'dir3', 'dir4')
        os.makedirs(path)

        # Try paths with a '.' in them
        self.assertRaises(OSError, os.makedirs, os.curdir)
        path = os.path.join(base, 'dir1', 'dir2', 'dir3', 'dir4', 'dir5', os.curdir)
        os.makedirs(path)
        path = os.path.join(base, 'dir1', os.curdir, 'dir2', 'dir3', 'dir4',
                            'dir5', 'dir6')
        os.makedirs(path)

    def tearDown(self):
        path = os.path.join(support.TESTFN, 'dir1', 'dir2', 'dir3',
                            'dir4', 'dir5', 'dir6')
        # If the tests failed, the bottom-most directory ('../dir6')
        # may not have been created, so we look for the outermost directory
        # that exists.
        while not os.path.exists(path) and path != support.TESTFN:
            path = os.path.dirname(path)

        os.removedirs(path)

class DevNullTests(unittest.TestCase):
    def test_devnull(self):
        f = open(os.devnull, 'w')
        f.write('hello')
        f.close()
        f = open(os.devnull, 'r')
        self.assertEqual(f.read(), '')
        f.close()

class URandomTests(unittest.TestCase):
    def test_urandom(self):
        try:
            self.assertEqual(len(os.urandom(1)), 1)
            self.assertEqual(len(os.urandom(10)), 10)
            self.assertEqual(len(os.urandom(100)), 100)
            self.assertEqual(len(os.urandom(1000)), 1000)
        except NotImplementedError:
            pass

class ExecTests(unittest.TestCase):
    @unittest.skipIf(USING_LINUXTHREADS,
                     "avoid triggering a linuxthreads bug: see issue #4970")
    def test_execvpe_with_bad_program(self):
        self.assertRaises(OSError, os.execvpe, 'no such app-',
                          ['no such app-'], None)

    def test_execvpe_with_bad_arglist(self):
        self.assertRaises(ValueError, os.execvpe, 'notepad', [], None)

class ArgTests(unittest.TestCase):
    def test_bytearray(self):
        # Issue #7561: posix module didn't release bytearray exports properly.
        b = bytearray(os.sep.encode('ascii'))
        self.assertRaises(OSError, os.mkdir, b)
        # Check object is still resizable.
        b[:] = b''

class Win32ErrorTests(unittest.TestCase):
    def test_rename(self):
        self.assertRaises(WindowsError, os.rename, support.TESTFN, support.TESTFN+".bak")

    def test_remove(self):
        self.assertRaises(WindowsError, os.remove, support.TESTFN)

    def test_chdir(self):
        self.assertRaises(WindowsError, os.chdir, support.TESTFN)

    def test_mkdir(self):
        f = open(support.TESTFN, "w")
        try:
            self.assertRaises(WindowsError, os.mkdir, support.TESTFN)
        finally:
            f.close()
            os.unlink(support.TESTFN)

    def test_utime(self):
        self.assertRaises(WindowsError, os.utime, support.TESTFN, None)

    def test_chmod(self):
        self.assertRaises(WindowsError, os.chmod, support.TESTFN, 0)

class TestInvalidFD(unittest.TestCase):
    singles = ["fchdir", "dup", "fdopen", "fdatasync", "fstat",
               "fstatvfs", "fsync", "tcgetpgrp", "ttyname"]
    #singles.append("close")
    #We omit close because it doesn'r raise an exception on some platforms
    def get_single(f):
        def helper(self):
            if  hasattr(os, f):
                self.check(getattr(os, f))
        return helper
    for f in singles:
        locals()["test_"+f] = get_single(f)

    def check(self, f, *args):
        try:
            f(support.make_bad_fd(), *args)
        except OSError as e:
            self.assertEqual(e.errno, errno.EBADF)
        else:
            self.fail("%r didn't raise a OSError with a bad file descriptor"
                      % f)

    def test_isatty(self):
        if hasattr(os, "isatty"):
            self.assertEqual(os.isatty(support.make_bad_fd()), False)

    def test_closerange(self):
        if hasattr(os, "closerange"):
            fd = support.make_bad_fd()
            # Make sure none of the descriptors we are about to close are
            # currently valid (issue 6542).
            for i in range(10):
                try: os.fstat(fd+i)
                except OSError:
                    pass
                else:
                    break
            if i < 2:
                raise unittest.SkipTest(
                    "Unable to acquire a range of invalid file descriptors")
            self.assertEqual(os.closerange(fd, fd + i-1), None)

    def test_dup2(self):
        if hasattr(os, "dup2"):
            self.check(os.dup2, 20)

    def test_fchmod(self):
        if hasattr(os, "fchmod"):
            self.check(os.fchmod, 0)

    def test_fchown(self):
        if hasattr(os, "fchown"):
            self.check(os.fchown, -1, -1)

    def test_fpathconf(self):
        if hasattr(os, "fpathconf"):
            self.check(os.fpathconf, "PC_NAME_MAX")

    def test_ftruncate(self):
        if hasattr(os, "ftruncate"):
            self.check(os.ftruncate, 0)

    def test_lseek(self):
        if hasattr(os, "lseek"):
            self.check(os.lseek, 0, 0)

    def test_read(self):
        if hasattr(os, "read"):
            self.check(os.read, 1)

    def test_tcsetpgrpt(self):
        if hasattr(os, "tcsetpgrp"):
            self.check(os.tcsetpgrp, 0)

    def test_write(self):
        if hasattr(os, "write"):
            self.check(os.write, b" ")

if sys.platform != 'win32':
    class Win32ErrorTests(unittest.TestCase):
        pass

    class PosixUidGidTests(unittest.TestCase):
        if hasattr(os, 'setuid'):
            def test_setuid(self):
                if os.getuid() != 0:
                    self.assertRaises(os.error, os.setuid, 0)
                self.assertRaises(OverflowError, os.setuid, 1<<32)

        if hasattr(os, 'setgid'):
            def test_setgid(self):
                if os.getuid() != 0:
                    self.assertRaises(os.error, os.setgid, 0)
                self.assertRaises(OverflowError, os.setgid, 1<<32)

        if hasattr(os, 'seteuid'):
            def test_seteuid(self):
                if os.getuid() != 0:
                    self.assertRaises(os.error, os.seteuid, 0)
                self.assertRaises(OverflowError, os.seteuid, 1<<32)

        if hasattr(os, 'setegid'):
            def test_setegid(self):
                if os.getuid() != 0:
                    self.assertRaises(os.error, os.setegid, 0)
                self.assertRaises(OverflowError, os.setegid, 1<<32)

        if hasattr(os, 'setreuid'):
            def test_setreuid(self):
                if os.getuid() != 0:
                    self.assertRaises(os.error, os.setreuid, 0, 0)
                self.assertRaises(OverflowError, os.setreuid, 1<<32, 0)
                self.assertRaises(OverflowError, os.setreuid, 0, 1<<32)

            def test_setreuid_neg1(self):
                # Needs to accept -1.  We run this in a subprocess to avoid
                # altering the test runner's process state (issue8045).
                import subprocess
                subprocess.check_call([
                        sys.executable, '-c',
                        'import os,sys;os.setreuid(-1,-1);sys.exit(0)'])

        if hasattr(os, 'setregid'):
            def test_setregid(self):
                if os.getuid() != 0:
                    self.assertRaises(os.error, os.setregid, 0, 0)
                self.assertRaises(OverflowError, os.setregid, 1<<32, 0)
                self.assertRaises(OverflowError, os.setregid, 0, 1<<32)

            def test_setregid_neg1(self):
                # Needs to accept -1.  We run this in a subprocess to avoid
                # altering the test runner's process state (issue8045).
                import subprocess
                subprocess.check_call([
                        sys.executable, '-c',
                        'import os,sys;os.setregid(-1,-1);sys.exit(0)'])

    @unittest.skipIf(sys.platform == 'darwin', "tests don't apply to OS X")
    class Pep383Tests(unittest.TestCase):
        filenames = [b'foo\xf6bar', 'foo\xf6bar'.encode("utf-8")]

        def setUp(self):
            self.fsencoding = sys.getfilesystemencoding()
            sys.setfilesystemencoding("utf-8")
            self.dir = support.TESTFN
            self.bdir = self.dir.encode("utf-8", "surrogateescape")
            os.mkdir(self.dir)
            self.unicodefn = []
            for fn in self.filenames:
                f = open(os.path.join(self.bdir, fn), "w")
                f.close()
                self.unicodefn.append(fn.decode("utf-8", "surrogateescape"))

        def tearDown(self):
            shutil.rmtree(self.dir)
            sys.setfilesystemencoding(self.fsencoding)

        def test_listdir(self):
            expected = set(self.unicodefn)
            found = set(os.listdir(support.TESTFN))
            self.assertEqual(found, expected)

        def test_open(self):
            for fn in self.unicodefn:
                f = open(os.path.join(self.dir, fn))
                f.close()

        def test_stat(self):
            for fn in self.unicodefn:
                os.stat(os.path.join(self.dir, fn))
else:
    class PosixUidGidTests(unittest.TestCase):
        pass
    class Pep383Tests(unittest.TestCase):
        pass

def test_main():
    support.run_unittest(
        ArgTests,
        FileTests,
        StatAttributeTests,
        EnvironTests,
        WalkTests,
        MakedirTests,
        DevNullTests,
        URandomTests,
        ExecTests,
        Win32ErrorTests,
        TestInvalidFD,
        PosixUidGidTests,
        Pep383Tests
    )

if __name__ == "__main__":
    test_main()
