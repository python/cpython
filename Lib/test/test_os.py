# As a test suite for the os module, this is woefully inadequate, but this
# does add tests for a few functions which have been determined to be more
# portable than they had been thought to be.

import os
import unittest
import warnings
import sys
from test import test_support

# Tests creating TESTFN
class FileTests(unittest.TestCase):
    def setUp(self):
        if os.path.exists(test_support.TESTFN):
            os.unlink(test_support.TESTFN)
    tearDown = setUp

    def test_access(self):
        f = os.open(test_support.TESTFN, os.O_CREAT|os.O_RDWR)
        os.close(f)
        self.assert_(os.access(test_support.TESTFN, os.W_OK))


# Test attributes on return values from os.*stat* family.
class StatAttributeTests(unittest.TestCase):
    def setUp(self):
        os.mkdir(test_support.TESTFN)
        self.fname = os.path.join(test_support.TESTFN, "f1")
        f = open(self.fname, 'wb')
        f.write(b"ABC")
        f.close()

    def tearDown(self):
        os.unlink(self.fname)
        os.rmdir(test_support.TESTFN)

    def test_stat_attributes(self):
        if not hasattr(os, "stat"):
            return

        import stat
        result = os.stat(self.fname)

        # Make sure direct access works
        self.assertEquals(result[stat.ST_SIZE], 3)
        self.assertEquals(result.st_size, 3)

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
                self.assertEquals(trunc(getattr(result, attr)),
                                  result[getattr(stat, name)])
                self.assert_(attr in members)

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

        import statvfs
        try:
            result = os.statvfs(self.fname)
        except OSError as e:
            # On AtheOS, glibc always returns ENOSYS
            import errno
            if e.errno == errno.ENOSYS:
                return

        # Make sure direct access works
        self.assertEquals(result.f_bfree, result[statvfs.F_BFREE])

        # Make sure all the attributes are there
        members = dir(result)
        for name in dir(statvfs):
            if name[:2] == 'F_':
                attr = name.lower()
                self.assertEquals(getattr(result, attr),
                                  result[getattr(statvfs, name)])
                self.assert_(attr in members)

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
        st = os.stat(test_support.TESTFN)
        # round to int, because some systems may support sub-second
        # time stamps in stat, but not in utime.
        os.utime(test_support.TESTFN, (st.st_atime, int(st.st_mtime-delta)))
        st2 = os.stat(test_support.TESTFN)
        self.assertEquals(st2.st_mtime, int(st.st_mtime-delta))

    # Restrict test to Win32, since there is no guarantee other
    # systems support centiseconds
    if sys.platform == 'win32':
        def get_file_system(path):
            import os
            root = os.path.splitdrive(os.path.realpath("."))[0] + '\\'
            import ctypes
            kernel32 = ctypes.windll.kernel32
            buf = ctypes.create_string_buffer("", 100)
            if kernel32.GetVolumeInformationA(root, None, 0, None, None, None, buf, len(buf)):
                return buf.value

        if get_file_system(test_support.TESTFN) == "NTFS":
            def test_1565150(self):
                t1 = 1159195039.25
                os.utime(self.fname, (t1, t1))
                self.assertEquals(os.stat(self.fname).st_mtime, t1)

        def test_1686475(self):
            # Verify that an open file can be stat'ed
            try:
                os.stat(r"c:\pagefile.sys")
            except WindowsError as e:
                if e == 2: # file does not exist; cannot run test
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
            value = os.popen("/bin/sh -c 'echo $HELLO'").read().strip()
            self.assertEquals(value, "World")

    # Verify environ keys and values from the OS are of the
    # correct str type.
    def test_keyvalue_types(self):
        for key, val in os.environ.items():
            self.assertEquals(type(key), str)
            self.assertEquals(type(val), str)

    def test_items(self):
        for key, value in self._reference().items():
            self.assertEqual(os.environ.get(key), value)

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

class MakedirTests(unittest.TestCase):
    def setUp(self):
        os.mkdir(test_support.TESTFN)

    def test_makedir(self):
        base = test_support.TESTFN
        path = os.path.join(base, 'dir1', 'dir2', 'dir3')
        os.makedirs(path)             # Should work
        path = os.path.join(base, 'dir1', 'dir2', 'dir3', 'dir4')
        os.makedirs(path)

        # Try paths with a '.' in them
        self.failUnlessRaises(OSError, os.makedirs, os.curdir)
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
    def test_execvpe_with_bad_program(self):
        self.assertRaises(OSError, os.execvpe, 'no such app-', ['no such app-'], None)

    def test_execvpe_with_bad_arglist(self):
        self.assertRaises(ValueError, os.execvpe, 'notepad', [], None)

class Win32ErrorTests(unittest.TestCase):
    def test_rename(self):
        self.assertRaises(WindowsError, os.rename, test_support.TESTFN, test_support.TESTFN+".bak")

    def test_remove(self):
        self.assertRaises(WindowsError, os.remove, test_support.TESTFN)

    def test_chdir(self):
        self.assertRaises(WindowsError, os.chdir, test_support.TESTFN)

    def test_mkdir(self):
        self.assertRaises(WindowsError, os.chdir, test_support.TESTFN)

    def test_utime(self):
        self.assertRaises(WindowsError, os.utime, test_support.TESTFN, None)

    def test_access(self):
        self.assertRaises(WindowsError, os.utime, test_support.TESTFN, 0)

    def test_chmod(self):
        self.assertRaises(WindowsError, os.utime, test_support.TESTFN, 0)

if sys.platform != 'win32':
    class Win32ErrorTests(unittest.TestCase):
        pass

def test_main():
    test_support.run_unittest(
        FileTests,
        StatAttributeTests,
        EnvironTests,
        WalkTests,
        MakedirTests,
        DevNullTests,
        URandomTests,
        ExecTests,
        Win32ErrorTests
    )

if __name__ == "__main__":
    test_main()
