# As a test suite for the os module, this is woefully inadequate, but this
# does add tests for a few functions which have been determined to be more
# portable than they had been thought to be.

import os
import errno
import unittest
import warnings
import sys
import signal
import subprocess
import sysconfig
import textwrap
import time
try:
    import resource
except ImportError:
    resource = None

from test import test_support
from test.script_helper import assert_python_ok
import mmap
import uuid

warnings.filterwarnings("ignore", "tempnam", RuntimeWarning, __name__)
warnings.filterwarnings("ignore", "tmpnam", RuntimeWarning, __name__)

# Tests creating TESTFN
class FileTests(unittest.TestCase):
    def setUp(self):
        if os.path.exists(test_support.TESTFN):
            os.unlink(test_support.TESTFN)
    tearDown = setUp

    def test_access(self):
        f = os.open(test_support.TESTFN, os.O_CREAT|os.O_RDWR)
        os.close(f)
        self.assertTrue(os.access(test_support.TESTFN, os.W_OK))

    def test_closerange(self):
        first = os.open(test_support.TESTFN, os.O_CREAT|os.O_RDWR)
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
        self.assertRaises(OSError, os.write, first, "a")

    @test_support.cpython_only
    def test_rename(self):
        path = unicode(test_support.TESTFN)
        old = sys.getrefcount(path)
        self.assertRaises(TypeError, os.rename, path, 0)
        new = sys.getrefcount(path)
        self.assertEqual(old, new)


class TemporaryFileTests(unittest.TestCase):
    def setUp(self):
        self.files = []
        os.mkdir(test_support.TESTFN)

    def tearDown(self):
        for name in self.files:
            os.unlink(name)
        os.rmdir(test_support.TESTFN)

    def check_tempfile(self, name):
        # make sure it doesn't already exist:
        self.assertFalse(os.path.exists(name),
                    "file already exists for temporary file")
        # make sure we can create the file
        open(name, "w")
        self.files.append(name)

    @unittest.skipUnless(hasattr(os, 'tempnam'), 'test needs os.tempnam()')
    def test_tempnam(self):
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", "tempnam", RuntimeWarning,
                                    r"test_os$")
            warnings.filterwarnings("ignore", "tempnam", DeprecationWarning)
            self.check_tempfile(os.tempnam())

            name = os.tempnam(test_support.TESTFN)
            self.check_tempfile(name)

            name = os.tempnam(test_support.TESTFN, "pfx")
            self.assertTrue(os.path.basename(name)[:3] == "pfx")
            self.check_tempfile(name)

    @unittest.skipUnless(hasattr(os, 'tmpfile'), 'test needs os.tmpfile()')
    def test_tmpfile(self):
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
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", "tmpfile", DeprecationWarning)

            if sys.platform == 'win32':
                name = '\\python_test_os_test_tmpfile.txt'
                if os.path.exists(name):
                    os.remove(name)
                try:
                    fp = open(name, 'w')
                except IOError, first:
                    # open() failed, assert tmpfile() fails in the same way.
                    # Although open() raises an IOError and os.tmpfile() raises an
                    # OSError(), 'args' will be (13, 'Permission denied') in both
                    # cases.
                    try:
                        fp = os.tmpfile()
                    except OSError, second:
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

    @unittest.skipUnless(hasattr(os, 'tmpnam'), 'test needs os.tmpnam()')
    def test_tmpnam(self):
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", "tmpnam", RuntimeWarning,
                                    r"test_os$")
            warnings.filterwarnings("ignore", "tmpnam", DeprecationWarning)

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

# Test attributes on return values from os.*stat* family.
class StatAttributeTests(unittest.TestCase):
    def setUp(self):
        os.mkdir(test_support.TESTFN)
        self.fname = os.path.join(test_support.TESTFN, "f1")
        f = open(self.fname, 'wb')
        f.write("ABC")
        f.close()

    def tearDown(self):
        os.unlink(self.fname)
        os.rmdir(test_support.TESTFN)

    @unittest.skipUnless(hasattr(os, 'stat'), 'test needs os.stat()')
    def test_stat_attributes(self):
        import stat
        result = os.stat(self.fname)

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

        try:
            result[200]
            self.fail("No exception raised")
        except IndexError:
            pass

        # Make sure that assignment fails
        try:
            result.st_mode = 1
            self.fail("No exception raised")
        except (AttributeError, TypeError):
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


    @unittest.skipUnless(hasattr(os, 'statvfs'), 'test needs os.statvfs()')
    def test_statvfs_attributes(self):
        try:
            result = os.statvfs(self.fname)
        except OSError, e:
            # On AtheOS, glibc always returns ENOSYS
            if e.errno == errno.ENOSYS:
                self.skipTest('glibc always returns ENOSYS on AtheOS')

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
            self.fail("No exception raised")
        except TypeError:
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

    def test_utime_dir(self):
        delta = 1000000
        st = os.stat(test_support.TESTFN)
        # round to int, because some systems may support sub-second
        # time stamps in stat, but not in utime.
        os.utime(test_support.TESTFN, (st.st_atime, int(st.st_mtime-delta)))
        st2 = os.stat(test_support.TESTFN)
        self.assertEqual(st2.st_mtime, int(st.st_mtime-delta))

    # Restrict tests to Win32, since there is no guarantee other
    # systems support centiseconds
    def get_file_system(path):
        if sys.platform == 'win32':
            root = os.path.splitdrive(os.path.abspath(path))[0] + '\\'
            import ctypes
            kernel32 = ctypes.windll.kernel32
            buf = ctypes.create_string_buffer("", 100)
            if kernel32.GetVolumeInformationA(root, None, 0, None, None, None, buf, len(buf)):
                return buf.value

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
    @unittest.skipUnless(get_file_system(test_support.TESTFN) == "NTFS",
                         "requires NTFS")
    def test_1565150(self):
        t1 = 1159195039.25
        os.utime(self.fname, (t1, t1))
        self.assertEqual(os.stat(self.fname).st_mtime, t1)

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
    @unittest.skipUnless(get_file_system(test_support.TESTFN) == "NTFS",
                         "requires NTFS")
    def test_large_time(self):
        t1 = 5000000000 # some day in 2128
        os.utime(self.fname, (t1, t1))
        self.assertEqual(os.stat(self.fname).st_mtime, t1)

    @unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
    def test_1686475(self):
        # Verify that an open file can be stat'ed
        try:
            os.stat(r"c:\pagefile.sys")
        except WindowsError, e:
            if e.errno == 2: # file does not exist; cannot run test
                self.skipTest(r'c:\pagefile.sys does not exist')
            self.fail("Could not stat pagefile.sys")

from test import mapping_tests

class EnvironTests(mapping_tests.BasicTestMappingProtocol):
    """check that os.environ object conform to mapping protocol"""
    type2test = None
    def _reference(self):
        return {"KEY1":"VALUE1", "KEY2":"VALUE2", "KEY3":"VALUE3"}
    def _empty_mapping(self):
        os.environ.clear()
        return os.environ
    def setUp(self):
        self.__save = dict(os.environ)
        os.environ.clear()
    def tearDown(self):
        os.environ.clear()
        os.environ.update(self.__save)

    # Bug 1110478
    def test_update2(self):
        if os.path.exists("/bin/sh"):
            os.environ.update(HELLO="World")
            with os.popen("/bin/sh -c 'echo $HELLO'") as popen:
                value = popen.read().strip()
                self.assertEqual(value, "World")

    # On FreeBSD < 7 and OS X < 10.6, unsetenv() doesn't return a value (issue
    # #13415).
    @unittest.skipIf(sys.platform.startswith(('freebsd', 'darwin')),
                     "due to known OS bug: see issue #13415")
    def test_unset_error(self):
        if sys.platform == "win32":
            # an environment variable is limited to 32,767 characters
            key = 'x' * 50000
            self.assertRaises(ValueError, os.environ.__delitem__, key)
        else:
            # "=" is not allowed in a variable name
            key = 'key='
            self.assertRaises(OSError, os.environ.__delitem__, key)

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
        walk_path = join(test_support.TESTFN, "TEST1")
        sub1_path = join(walk_path, "SUB1")
        sub11_path = join(sub1_path, "SUB11")
        sub2_path = join(walk_path, "SUB2")
        tmp1_path = join(walk_path, "tmp1")
        tmp2_path = join(sub1_path, "tmp2")
        tmp3_path = join(sub2_path, "tmp3")
        link_path = join(sub2_path, "link")
        t2_path = join(test_support.TESTFN, "TEST2")
        tmp4_path = join(test_support.TESTFN, "TEST2", "tmp4")

        # Create stuff.
        os.makedirs(sub11_path)
        os.makedirs(sub2_path)
        os.makedirs(t2_path)
        for path in tmp1_path, tmp2_path, tmp3_path, tmp4_path:
            f = file(path, "w")
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
        for root, dirs, files in os.walk(test_support.TESTFN, topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                dirname = os.path.join(root, name)
                if not os.path.islink(dirname):
                    os.rmdir(dirname)
                else:
                    os.remove(dirname)
        os.rmdir(test_support.TESTFN)

class MakedirTests (unittest.TestCase):
    def setUp(self):
        os.mkdir(test_support.TESTFN)

    def test_makedir(self):
        base = test_support.TESTFN
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
        path = os.path.join(test_support.TESTFN, 'dir1', 'dir2', 'dir3',
                            'dir4', 'dir5', 'dir6')
        # If the tests failed, the bottom-most directory ('../dir6')
        # may not have been created, so we look for the outermost directory
        # that exists.
        while not os.path.exists(path) and path != test_support.TESTFN:
            path = os.path.dirname(path)

        os.removedirs(path)

class DevNullTests (unittest.TestCase):
    def test_devnull(self):
        f = file(os.devnull, 'w')
        f.write('hello')
        f.close()
        f = file(os.devnull, 'r')
        self.assertEqual(f.read(), '')
        f.close()

class URandomTests (unittest.TestCase):

    def test_urandom_length(self):
        self.assertEqual(len(os.urandom(0)), 0)
        self.assertEqual(len(os.urandom(1)), 1)
        self.assertEqual(len(os.urandom(10)), 10)
        self.assertEqual(len(os.urandom(100)), 100)
        self.assertEqual(len(os.urandom(1000)), 1000)

    def test_urandom_value(self):
        data1 = os.urandom(16)
        data2 = os.urandom(16)
        self.assertNotEqual(data1, data2)

    def get_urandom_subprocess(self, count):
        # We need to use repr() and eval() to avoid line ending conversions
        # under Windows.
        code = '\n'.join((
            'import os, sys',
            'data = os.urandom(%s)' % count,
            'sys.stdout.write(repr(data))',
            'sys.stdout.flush()',
            'print >> sys.stderr, (len(data), data)'))
        cmd_line = [sys.executable, '-c', code]
        p = subprocess.Popen(cmd_line, stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p.communicate()
        self.assertEqual(p.wait(), 0, (p.wait(), err))
        out = eval(out)
        self.assertEqual(len(out), count, err)
        return out

    def test_urandom_subprocess(self):
        data1 = self.get_urandom_subprocess(16)
        data2 = self.get_urandom_subprocess(16)
        self.assertNotEqual(data1, data2)


HAVE_GETENTROPY = (sysconfig.get_config_var('HAVE_GETENTROPY') == 1)

@unittest.skipIf(HAVE_GETENTROPY,
                 "getentropy() does not use a file descriptor")
class URandomFDTests(unittest.TestCase):
    @unittest.skipUnless(resource, "test requires the resource module")
    def test_urandom_failure(self):
        # Check urandom() failing when it is not able to open /dev/random.
        # We spawn a new process to make the test more robust (if getrlimit()
        # failed to restore the file descriptor limit after this, the whole
        # test suite would crash; this actually happened on the OS X Tiger
        # buildbot).
        code = """if 1:
            import errno
            import os
            import resource

            soft_limit, hard_limit = resource.getrlimit(resource.RLIMIT_NOFILE)
            resource.setrlimit(resource.RLIMIT_NOFILE, (1, hard_limit))
            try:
                os.urandom(16)
            except OSError as e:
                assert e.errno == errno.EMFILE, e.errno
            else:
                raise AssertionError("OSError not raised")
            """
        assert_python_ok('-c', code)


class ExecTests(unittest.TestCase):

    def test_execvpe_with_bad_arglist(self):
        self.assertRaises(ValueError, os.execvpe, 'notepad', [], None)

    def test_execve_invalid_env(self):
        args = [sys.executable, '-c', 'pass']

        # null character in the enviroment variable name
        newenv = os.environ.copy()
        newenv["FRUIT\0VEGETABLE"] = "cabbage"
        with self.assertRaises(TypeError):
            os.execve(args[0], args, newenv)

        # null character in the enviroment variable value
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange\0VEGETABLE=cabbage"
        with self.assertRaises(TypeError):
            os.execve(args[0], args, newenv)

        # equal character in the enviroment variable name
        newenv = os.environ.copy()
        newenv["FRUIT=ORANGE"] = "lemon"
        with self.assertRaises(ValueError):
            os.execve(args[0], args, newenv)


@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
class Win32ErrorTests(unittest.TestCase):
    def test_rename(self):
        self.assertRaises(WindowsError, os.rename, test_support.TESTFN, test_support.TESTFN+".bak")

    def test_remove(self):
        self.assertRaises(WindowsError, os.remove, test_support.TESTFN)

    def test_chdir(self):
        self.assertRaises(WindowsError, os.chdir, test_support.TESTFN)

    def test_mkdir(self):
        f = open(test_support.TESTFN, "w")
        try:
            self.assertRaises(WindowsError, os.mkdir, test_support.TESTFN)
        finally:
            f.close()
            os.unlink(test_support.TESTFN)

    def test_utime(self):
        self.assertRaises(WindowsError, os.utime, test_support.TESTFN, None)

    def test_chmod(self):
        self.assertRaises(WindowsError, os.chmod, test_support.TESTFN, 0)

class TestInvalidFD(unittest.TestCase):
    singles = ["fchdir", "fdopen", "dup", "fdatasync", "fstat",
               "fstatvfs", "fsync", "tcgetpgrp", "ttyname"]
    #singles.append("close")
    #We omit close because it doesn't raise an exception on some platforms
    def get_single(f):
        def helper(self):
            if  hasattr(os, f):
                self.check(getattr(os, f))
        return helper
    for f in singles:
        locals()["test_"+f] = get_single(f)

    def check(self, f, *args):
        try:
            f(test_support.make_bad_fd(), *args)
        except OSError as e:
            self.assertEqual(e.errno, errno.EBADF)
        else:
            self.fail("%r didn't raise an OSError with a bad file descriptor"
                      % f)

    @unittest.skipUnless(hasattr(os, 'isatty'), 'test needs os.isatty()')
    def test_isatty(self):
        self.assertEqual(os.isatty(test_support.make_bad_fd()), False)

    @unittest.skipUnless(hasattr(os, 'closerange'), 'test needs os.closerange()')
    def test_closerange(self):
        fd = test_support.make_bad_fd()
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

    @unittest.skipUnless(hasattr(os, 'dup2'), 'test needs os.dup2()')
    def test_dup2(self):
        self.check(os.dup2, 20)

    @unittest.skipUnless(hasattr(os, 'fchmod'), 'test needs os.fchmod()')
    def test_fchmod(self):
        self.check(os.fchmod, 0)

    @unittest.skipUnless(hasattr(os, 'fchown'), 'test needs os.fchown()')
    def test_fchown(self):
        self.check(os.fchown, -1, -1)

    @unittest.skipUnless(hasattr(os, 'fpathconf'), 'test needs os.fpathconf()')
    def test_fpathconf(self):
        self.check(os.fpathconf, "PC_NAME_MAX")

    @unittest.skipUnless(hasattr(os, 'ftruncate'), 'test needs os.ftruncate()')
    def test_ftruncate(self):
        self.check(os.ftruncate, 0)

    @unittest.skipUnless(hasattr(os, 'lseek'), 'test needs os.lseek()')
    def test_lseek(self):
        self.check(os.lseek, 0, 0)

    @unittest.skipUnless(hasattr(os, 'read'), 'test needs os.read()')
    def test_read(self):
        self.check(os.read, 1)

    @unittest.skipUnless(hasattr(os, 'tcsetpgrp'), 'test needs os.tcsetpgrp()')
    def test_tcsetpgrpt(self):
        self.check(os.tcsetpgrp, 0)

    @unittest.skipUnless(hasattr(os, 'write'), 'test needs os.write()')
    def test_write(self):
        self.check(os.write, " ")

@unittest.skipIf(sys.platform == "win32", "Posix specific tests")
class PosixUidGidTests(unittest.TestCase):
    @unittest.skipUnless(hasattr(os, 'setuid'), 'test needs os.setuid()')
    def test_setuid(self):
        if os.getuid() != 0:
            self.assertRaises(os.error, os.setuid, 0)
        self.assertRaises(TypeError, os.setuid, 'not an int')
        self.assertRaises(OverflowError, os.setuid, 1<<32)

    @unittest.skipUnless(hasattr(os, 'setgid'), 'test needs os.setgid()')
    def test_setgid(self):
        if os.getuid() != 0:
            self.assertRaises(os.error, os.setgid, 0)
        self.assertRaises(TypeError, os.setgid, 'not an int')
        self.assertRaises(OverflowError, os.setgid, 1<<32)

    @unittest.skipUnless(hasattr(os, 'seteuid'), 'test needs os.seteuid()')
    def test_seteuid(self):
        if os.getuid() != 0:
            self.assertRaises(os.error, os.seteuid, 0)
        self.assertRaises(TypeError, os.seteuid, 'not an int')
        self.assertRaises(OverflowError, os.seteuid, 1<<32)

    @unittest.skipUnless(hasattr(os, 'setegid'), 'test needs os.setegid()')
    def test_setegid(self):
        if os.getuid() != 0:
            self.assertRaises(os.error, os.setegid, 0)
        self.assertRaises(TypeError, os.setegid, 'not an int')
        self.assertRaises(OverflowError, os.setegid, 1<<32)

    @unittest.skipUnless(hasattr(os, 'setreuid'), 'test needs os.setreuid()')
    def test_setreuid(self):
        if os.getuid() != 0:
            self.assertRaises(os.error, os.setreuid, 0, 0)
        self.assertRaises(TypeError, os.setreuid, 'not an int', 0)
        self.assertRaises(TypeError, os.setreuid, 0, 'not an int')
        self.assertRaises(OverflowError, os.setreuid, 1<<32, 0)
        self.assertRaises(OverflowError, os.setreuid, 0, 1<<32)

    @unittest.skipUnless(hasattr(os, 'setreuid'), 'test needs os.setreuid()')
    def test_setreuid_neg1(self):
        # Needs to accept -1.  We run this in a subprocess to avoid
        # altering the test runner's process state (issue8045).
        subprocess.check_call([
                sys.executable, '-c',
                'import os,sys;os.setreuid(-1,-1);sys.exit(0)'])

    @unittest.skipUnless(hasattr(os, 'setregid'), 'test needs os.setregid()')
    def test_setregid(self):
        if os.getuid() != 0:
            self.assertRaises(os.error, os.setregid, 0, 0)
        self.assertRaises(TypeError, os.setregid, 'not an int', 0)
        self.assertRaises(TypeError, os.setregid, 0, 'not an int')
        self.assertRaises(OverflowError, os.setregid, 1<<32, 0)
        self.assertRaises(OverflowError, os.setregid, 0, 1<<32)

    @unittest.skipUnless(hasattr(os, 'setregid'), 'test needs os.setregid()')
    def test_setregid_neg1(self):
        # Needs to accept -1.  We run this in a subprocess to avoid
        # altering the test runner's process state (issue8045).
        subprocess.check_call([
                sys.executable, '-c',
                'import os,sys;os.setregid(-1,-1);sys.exit(0)'])


@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
class Win32KillTests(unittest.TestCase):
    def _kill(self, sig):
        # Start sys.executable as a subprocess and communicate from the
        # subprocess to the parent that the interpreter is ready. When it
        # becomes ready, send *sig* via os.kill to the subprocess and check
        # that the return code is equal to *sig*.
        import ctypes
        from ctypes import wintypes
        import msvcrt

        # Since we can't access the contents of the process' stdout until the
        # process has exited, use PeekNamedPipe to see what's inside stdout
        # without waiting. This is done so we can tell that the interpreter
        # is started and running at a point where it could handle a signal.
        PeekNamedPipe = ctypes.windll.kernel32.PeekNamedPipe
        PeekNamedPipe.restype = wintypes.BOOL
        PeekNamedPipe.argtypes = (wintypes.HANDLE, # Pipe handle
                                  ctypes.POINTER(ctypes.c_char), # stdout buf
                                  wintypes.DWORD, # Buffer size
                                  ctypes.POINTER(wintypes.DWORD), # bytes read
                                  ctypes.POINTER(wintypes.DWORD), # bytes avail
                                  ctypes.POINTER(wintypes.DWORD)) # bytes left
        msg = "running"
        proc = subprocess.Popen([sys.executable, "-c",
                                 "import sys;"
                                 "sys.stdout.write('{}');"
                                 "sys.stdout.flush();"
                                 "input()".format(msg)],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                stdin=subprocess.PIPE)
        self.addCleanup(proc.stdout.close)
        self.addCleanup(proc.stderr.close)
        self.addCleanup(proc.stdin.close)

        count, max = 0, 100
        while count < max and proc.poll() is None:
            # Create a string buffer to store the result of stdout from the pipe
            buf = ctypes.create_string_buffer(len(msg))
            # Obtain the text currently in proc.stdout
            # Bytes read/avail/left are left as NULL and unused
            rslt = PeekNamedPipe(msvcrt.get_osfhandle(proc.stdout.fileno()),
                                 buf, ctypes.sizeof(buf), None, None, None)
            self.assertNotEqual(rslt, 0, "PeekNamedPipe failed")
            if buf.value:
                self.assertEqual(msg, buf.value)
                break
            time.sleep(0.1)
            count += 1
        else:
            self.fail("Did not receive communication from the subprocess")

        os.kill(proc.pid, sig)
        self.assertEqual(proc.wait(), sig)

    def test_kill_sigterm(self):
        # SIGTERM doesn't mean anything special, but make sure it works
        self._kill(signal.SIGTERM)

    def test_kill_int(self):
        # os.kill on Windows can take an int which gets set as the exit code
        self._kill(100)

    def _kill_with_event(self, event, name):
        tagname = "test_os_%s" % uuid.uuid1()
        m = mmap.mmap(-1, 1, tagname)
        m[0] = '0'
        # Run a script which has console control handling enabled.
        proc = subprocess.Popen([sys.executable,
                   os.path.join(os.path.dirname(__file__),
                                "win_console_handler.py"), tagname],
                   creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)
        # Let the interpreter startup before we send signals. See #3137.
        count, max = 0, 20
        while count < max and proc.poll() is None:
            if m[0] == '1':
                break
            time.sleep(0.5)
            count += 1
        else:
            self.fail("Subprocess didn't finish initialization")
        os.kill(proc.pid, event)
        # proc.send_signal(event) could also be done here.
        # Allow time for the signal to be passed and the process to exit.
        time.sleep(0.5)
        if not proc.poll():
            # Forcefully kill the process if we weren't able to signal it.
            os.kill(proc.pid, signal.SIGINT)
            self.fail("subprocess did not stop on {}".format(name))

    @unittest.skip("subprocesses aren't inheriting Ctrl+C property")
    def test_CTRL_C_EVENT(self):
        from ctypes import wintypes
        import ctypes

        # Make a NULL value by creating a pointer with no argument.
        NULL = ctypes.POINTER(ctypes.c_int)()
        SetConsoleCtrlHandler = ctypes.windll.kernel32.SetConsoleCtrlHandler
        SetConsoleCtrlHandler.argtypes = (ctypes.POINTER(ctypes.c_int),
                                          wintypes.BOOL)
        SetConsoleCtrlHandler.restype = wintypes.BOOL

        # Calling this with NULL and FALSE causes the calling process to
        # handle Ctrl+C, rather than ignore it. This property is inherited
        # by subprocesses.
        SetConsoleCtrlHandler(NULL, 0)

        self._kill_with_event(signal.CTRL_C_EVENT, "CTRL_C_EVENT")

    def test_CTRL_BREAK_EVENT(self):
        self._kill_with_event(signal.CTRL_BREAK_EVENT, "CTRL_BREAK_EVENT")


@unittest.skipUnless(sys.platform == "win32", "Win32 specific tests")
class Win32ListdirTests(unittest.TestCase):
    """Test listdir on Windows."""

    def setUp(self):
        self.created_paths = []
        for i in range(2):
            dir_name = 'SUB%d' % i
            dir_path = os.path.join(support.TESTFN, dir_name)
            file_name = 'FILE%d' % i
            file_path = os.path.join(support.TESTFN, file_name)
            os.makedirs(dir_path)
            with open(file_path, 'w') as f:
                f.write("I'm %s and proud of it. Blame test_os.\n" % file_path)
            self.created_paths.extend([dir_name, file_name])
        self.created_paths.sort()

    def tearDown(self):
        shutil.rmtree(support.TESTFN)

    def test_listdir_no_extended_path(self):
        """Test when the path is not an "extended" path."""
        # unicode
        fs_encoding = sys.getfilesystemencoding()
        self.assertEqual(
                sorted(os.listdir(support.TESTFN.decode(fs_encoding))),
                [path.decode(fs_encoding) for path in self.created_paths])

        # bytes
        self.assertEqual(
                sorted(os.listdir(os.fsencode(support.TESTFN))),
                self.created_paths)

    def test_listdir_extended_path(self):
        """Test when the path starts with '\\\\?\\'."""
        # See: http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx#maxpath
        # unicode
        fs_encoding = sys.getfilesystemencoding()
        path = u'\\\\?\\' + os.path.abspath(support.TESTFN.decode(fs_encoding))
        self.assertEqual(
                sorted(os.listdir(path)),
                [path.decode(fs_encoding) for path in self.created_paths])

        # bytes
        path = b'\\\\?\\' + os.path.abspath(support.TESTFN)
        self.assertEqual(
                sorted(os.listdir(path)),
                self.created_paths)


class SpawnTests(unittest.TestCase):
    def _test_invalid_env(self, spawn):
        args = [sys.executable, '-c', 'pass']

        # null character in the enviroment variable name
        newenv = os.environ.copy()
        newenv["FRUIT\0VEGETABLE"] = "cabbage"
        try:
            exitcode = spawn(os.P_WAIT, args[0], args, newenv)
        except TypeError:
            pass
        else:
            self.assertEqual(exitcode, 127)

        # null character in the enviroment variable value
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange\0VEGETABLE=cabbage"
        try:
            exitcode = spawn(os.P_WAIT, args[0], args, newenv)
        except TypeError:
            pass
        else:
            self.assertEqual(exitcode, 127)

        # equal character in the enviroment variable name
        newenv = os.environ.copy()
        newenv["FRUIT=ORANGE"] = "lemon"
        try:
            exitcode = spawn(os.P_WAIT, args[0], args, newenv)
        except ValueError:
            pass
        else:
            self.assertEqual(exitcode, 127)

        # equal character in the enviroment variable value
        filename = test_support.TESTFN
        self.addCleanup(test_support.unlink, filename)
        with open(filename, "w") as fp:
            fp.write('import sys, os\n'
                     'if os.getenv("FRUIT") != "orange=lemon":\n'
                     '    raise AssertionError')
        args = [sys.executable, filename]
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange=lemon"
        exitcode = spawn(os.P_WAIT, args[0], args, newenv)
        self.assertEqual(exitcode, 0)

    @unittest.skipUnless(hasattr(os, 'spawnve'), 'test needs os.spawnve()')
    def test_spawnve_invalid_env(self):
        self._test_invalid_env(os.spawnve)

    @unittest.skipUnless(hasattr(os, 'spawnvpe'), 'test needs os.spawnvpe()')
    def test_spawnvpe_invalid_env(self):
        self._test_invalid_env(os.spawnvpe)


def test_main():
    test_support.run_unittest(
        FileTests,
        TemporaryFileTests,
        StatAttributeTests,
        EnvironTests,
        WalkTests,
        MakedirTests,
        DevNullTests,
        URandomTests,
        URandomFDTests,
        ExecTests,
        Win32ErrorTests,
        TestInvalidFD,
        PosixUidGidTests,
        Win32KillTests,
        SpawnTests,
    )

if __name__ == "__main__":
    test_main()
