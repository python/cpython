"""
Test directories: listdir(), walk(), getcwd(), mkdir(), rmdir(), etc.
"""

import errno
import itertools
import os
import posix
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest
from test import support
from test.support import os_helper
from .utils import create_file


class WalkTests(unittest.TestCase):
    """Tests for os.walk()."""
    is_fwalk = False

    # Wrapper to hide minor differences between os.walk and os.fwalk
    # to tests both functions with the same code base
    def walk(self, top, **kwargs):
        if 'follow_symlinks' in kwargs:
            kwargs['followlinks'] = kwargs.pop('follow_symlinks')
        return os.walk(top, **kwargs)

    def setUp(self):
        join = os.path.join
        self.addCleanup(os_helper.rmtree, os_helper.TESTFN)

        # Build:
        #     TESTFN/
        #       TEST1/              a file kid and two directory kids
        #         tmp1
        #         SUB1/             a file kid and a directory kid
        #           tmp2
        #           SUB11/          no kids
        #         SUB2/             a file kid and a dirsymlink kid
        #           tmp3
        #           SUB21/          not readable
        #             tmp5
        #           link/           a symlink to TESTFN.2
        #           broken_link
        #           broken_link2
        #           broken_link3
        #       TEST2/
        #         tmp4              a lone file
        self.walk_path = join(os_helper.TESTFN, "TEST1")
        self.sub1_path = join(self.walk_path, "SUB1")
        self.sub11_path = join(self.sub1_path, "SUB11")
        sub2_path = join(self.walk_path, "SUB2")
        sub21_path = join(sub2_path, "SUB21")
        self.tmp1_path = join(self.walk_path, "tmp1")
        tmp2_path = join(self.sub1_path, "tmp2")
        tmp3_path = join(sub2_path, "tmp3")
        tmp5_path = join(sub21_path, "tmp3")
        self.link_path = join(sub2_path, "link")
        t2_path = join(os_helper.TESTFN, "TEST2")
        tmp4_path = join(os_helper.TESTFN, "TEST2", "tmp4")
        self.broken_link_path = join(sub2_path, "broken_link")
        broken_link2_path = join(sub2_path, "broken_link2")
        broken_link3_path = join(sub2_path, "broken_link3")

        # Create stuff.
        os.makedirs(self.sub11_path)
        os.makedirs(sub2_path)
        os.makedirs(sub21_path)
        os.makedirs(t2_path)

        for path in self.tmp1_path, tmp2_path, tmp3_path, tmp4_path, tmp5_path:
            with open(path, "x", encoding='utf-8') as f:
                f.write("I'm " + path + " and proud of it.  Blame test_os.\n")

        if os_helper.can_symlink():
            os.symlink(os.path.abspath(t2_path), self.link_path)
            os.symlink('broken', self.broken_link_path, True)
            os.symlink(join('tmp3', 'broken'), broken_link2_path, True)
            os.symlink(join('SUB21', 'tmp5'), broken_link3_path, True)
            self.sub2_tree = (sub2_path, ["SUB21", "link"],
                              ["broken_link", "broken_link2", "broken_link3",
                               "tmp3"])
        else:
            self.sub2_tree = (sub2_path, ["SUB21"], ["tmp3"])

        os.chmod(sub21_path, 0)
        try:
            os.listdir(sub21_path)
        except PermissionError:
            self.addCleanup(os.chmod, sub21_path, stat.S_IRWXU)
        else:
            os.chmod(sub21_path, stat.S_IRWXU)
            os.unlink(tmp5_path)
            os.rmdir(sub21_path)
            del self.sub2_tree[1][:1]

    def test_walk_topdown(self):
        # Walk top-down.
        all = list(self.walk(self.walk_path))

        self.assertEqual(len(all), 4)
        # We can't know which order SUB1 and SUB2 will appear in.
        # Not flipped:  TESTFN, SUB1, SUB11, SUB2
        #     flipped:  TESTFN, SUB2, SUB1, SUB11
        flipped = all[0][1][0] != "SUB1"
        all[0][1].sort()
        all[3 - 2 * flipped][-1].sort()
        all[3 - 2 * flipped][1].sort()
        self.assertEqual(all[0], (self.walk_path, ["SUB1", "SUB2"], ["tmp1"]))
        self.assertEqual(all[1 + flipped], (self.sub1_path, ["SUB11"], ["tmp2"]))
        self.assertEqual(all[2 + flipped], (self.sub11_path, [], []))
        self.assertEqual(all[3 - 2 * flipped], self.sub2_tree)

    def test_walk_prune(self, walk_path=None):
        if walk_path is None:
            walk_path = self.walk_path
        # Prune the search.
        all = []
        for root, dirs, files in self.walk(walk_path):
            all.append((root, dirs, files))
            # Don't descend into SUB1.
            if 'SUB1' in dirs:
                # Note that this also mutates the dirs we appended to all!
                dirs.remove('SUB1')

        self.assertEqual(len(all), 2)
        self.assertEqual(all[0], (self.walk_path, ["SUB2"], ["tmp1"]))

        all[1][-1].sort()
        all[1][1].sort()
        self.assertEqual(all[1], self.sub2_tree)

    def test_file_like_path(self):
        self.test_walk_prune(os_helper.FakePath(self.walk_path))

    def test_walk_bottom_up(self):
        # Walk bottom-up.
        all = list(self.walk(self.walk_path, topdown=False))

        self.assertEqual(len(all), 4, all)
        # We can't know which order SUB1 and SUB2 will appear in.
        # Not flipped:  SUB11, SUB1, SUB2, TESTFN
        #     flipped:  SUB2, SUB11, SUB1, TESTFN
        flipped = all[3][1][0] != "SUB1"
        all[3][1].sort()
        all[2 - 2 * flipped][-1].sort()
        all[2 - 2 * flipped][1].sort()
        self.assertEqual(all[3],
                         (self.walk_path, ["SUB1", "SUB2"], ["tmp1"]))
        self.assertEqual(all[flipped],
                         (self.sub11_path, [], []))
        self.assertEqual(all[flipped + 1],
                         (self.sub1_path, ["SUB11"], ["tmp2"]))
        self.assertEqual(all[2 - 2 * flipped],
                         self.sub2_tree)

    def test_walk_symlink(self):
        if not os_helper.can_symlink():
            self.skipTest("need symlink support")

        # Walk, following symlinks.
        walk_it = self.walk(self.walk_path, follow_symlinks=True)
        for root, dirs, files in walk_it:
            if root == self.link_path:
                self.assertEqual(dirs, [])
                self.assertEqual(files, ["tmp4"])
                break
        else:
            self.fail("Didn't follow symlink with followlinks=True")

        walk_it = self.walk(self.broken_link_path, follow_symlinks=True)
        if self.is_fwalk:
            self.assertRaises(FileNotFoundError, next, walk_it)
        self.assertRaises(StopIteration, next, walk_it)

    def test_walk_bad_dir(self):
        # Walk top-down.
        errors = []
        walk_it = self.walk(self.walk_path, onerror=errors.append)
        root, dirs, files = next(walk_it)
        self.assertEqual(errors, [])
        dir1 = 'SUB1'
        path1 = os.path.join(root, dir1)
        path1new = os.path.join(root, dir1 + '.new')
        os.rename(path1, path1new)
        try:
            roots = [r for r, d, f in walk_it]
            self.assertTrue(errors)
            self.assertNotIn(path1, roots)
            self.assertNotIn(path1new, roots)
            for dir2 in dirs:
                if dir2 != dir1:
                    self.assertIn(os.path.join(root, dir2), roots)
        finally:
            os.rename(path1new, path1)

    def test_walk_bad_dir2(self):
        walk_it = self.walk('nonexisting')
        if self.is_fwalk:
            self.assertRaises(FileNotFoundError, next, walk_it)
        self.assertRaises(StopIteration, next, walk_it)

        walk_it = self.walk('nonexisting', follow_symlinks=True)
        if self.is_fwalk:
            self.assertRaises(FileNotFoundError, next, walk_it)
        self.assertRaises(StopIteration, next, walk_it)

        walk_it = self.walk(self.tmp1_path)
        self.assertRaises(StopIteration, next, walk_it)

        walk_it = self.walk(self.tmp1_path, follow_symlinks=True)
        if self.is_fwalk:
            self.assertRaises(NotADirectoryError, next, walk_it)
        self.assertRaises(StopIteration, next, walk_it)

    @unittest.skipUnless(hasattr(os, "mkfifo"), 'requires os.mkfifo()')
    @unittest.skipIf(sys.platform == "vxworks",
                    "fifo requires special path on VxWorks")
    def test_walk_named_pipe(self):
        path = os_helper.TESTFN + '-pipe'
        os.mkfifo(path)
        self.addCleanup(os.unlink, path)

        walk_it = self.walk(path)
        self.assertRaises(StopIteration, next, walk_it)

        walk_it = self.walk(path, follow_symlinks=True)
        if self.is_fwalk:
            self.assertRaises(NotADirectoryError, next, walk_it)
        self.assertRaises(StopIteration, next, walk_it)

    @unittest.skipUnless(hasattr(os, "mkfifo"), 'requires os.mkfifo()')
    @unittest.skipIf(sys.platform == "vxworks",
                    "fifo requires special path on VxWorks")
    def test_walk_named_pipe2(self):
        path = os_helper.TESTFN + '-dir'
        os.mkdir(path)
        self.addCleanup(shutil.rmtree, path)
        os.mkfifo(os.path.join(path, 'mypipe'))

        errors = []
        walk_it = self.walk(path, onerror=errors.append)
        next(walk_it)
        self.assertRaises(StopIteration, next, walk_it)
        self.assertEqual(errors, [])

        errors = []
        walk_it = self.walk(path, onerror=errors.append)
        root, dirs, files = next(walk_it)
        self.assertEqual(root, path)
        self.assertEqual(dirs, [])
        self.assertEqual(files, ['mypipe'])
        dirs.extend(files)
        files.clear()
        if self.is_fwalk:
            self.assertRaises(NotADirectoryError, next, walk_it)
        self.assertRaises(StopIteration, next, walk_it)
        if self.is_fwalk:
            self.assertEqual(errors, [])
        else:
            self.assertEqual(len(errors), 1, errors)
            self.assertIsInstance(errors[0], NotADirectoryError)

    def test_walk_many_open_files(self):
        depth = 30
        base = os.path.join(os_helper.TESTFN, 'deep')
        p = os.path.join(base, *(['d']*depth))
        os.makedirs(p)

        iters = [self.walk(base, topdown=False) for j in range(100)]
        for i in range(depth + 1):
            expected = (p, ['d'] if i else [], [])
            for it in iters:
                self.assertEqual(next(it), expected)
            p = os.path.dirname(p)

        iters = [self.walk(base, topdown=True) for j in range(100)]
        p = base
        for i in range(depth + 1):
            expected = (p, ['d'] if i < depth else [], [])
            for it in iters:
                self.assertEqual(next(it), expected)
            p = os.path.join(p, 'd')

    def test_walk_above_recursion_limit(self):
        depth = 50
        os.makedirs(os.path.join(self.walk_path, *(['d'] * depth)))
        with support.infinite_recursion(depth - 5):
            all = list(self.walk(self.walk_path))

        sub2_path = self.sub2_tree[0]
        for root, dirs, files in all:
            if root == sub2_path:
                dirs.sort()
                files.sort()

        d_entries = []
        d_path = self.walk_path
        for _ in range(depth):
            d_path = os.path.join(d_path, "d")
            d_entries.append((d_path, ["d"], []))
        d_entries[-1][1].clear()

        # Sub-sequences where the order is known
        sections = {
            "SUB1": [
                (self.sub1_path, ["SUB11"], ["tmp2"]),
                (self.sub11_path, [], []),
            ],
            "SUB2": [self.sub2_tree],
            "d": d_entries,
        }

        # The ordering of sub-dirs is arbitrary but determines the order in
        # which sub-sequences appear
        dirs = all[0][1]
        expected = [(self.walk_path, dirs, ["tmp1"])]
        for d in dirs:
            expected.extend(sections[d])

        self.assertEqual(len(all), depth + 4)
        self.assertEqual(sorted(dirs), ["SUB1", "SUB2", "d"])
        self.assertEqual(all, expected)


@unittest.skipUnless(hasattr(os, 'fwalk'), "Test needs os.fwalk()")
class FwalkTests(WalkTests):
    """Tests for os.fwalk()."""
    is_fwalk = True

    def walk(self, top, **kwargs):
        for root, dirs, files, root_fd in self.fwalk(top, **kwargs):
            yield (root, dirs, files)

    def fwalk(self, *args, **kwargs):
        return os.fwalk(*args, **kwargs)

    def _compare_to_walk(self, walk_kwargs, fwalk_kwargs):
        """
        compare with walk() results.
        """
        walk_kwargs = walk_kwargs.copy()
        fwalk_kwargs = fwalk_kwargs.copy()
        for topdown, follow_symlinks in itertools.product((True, False), repeat=2):
            walk_kwargs.update(topdown=topdown, followlinks=follow_symlinks)
            fwalk_kwargs.update(topdown=topdown, follow_symlinks=follow_symlinks)

            expected = {}
            for root, dirs, files in os.walk(**walk_kwargs):
                expected[root] = (set(dirs), set(files))

            for root, dirs, files, rootfd in self.fwalk(**fwalk_kwargs):
                self.assertIn(root, expected)
                self.assertEqual(expected[root], (set(dirs), set(files)))

    def test_compare_to_walk(self):
        kwargs = {'top': os_helper.TESTFN}
        self._compare_to_walk(kwargs, kwargs)

    def test_dir_fd(self):
        try:
            fd = os.open(".", os.O_RDONLY)
            walk_kwargs = {'top': os_helper.TESTFN}
            fwalk_kwargs = walk_kwargs.copy()
            fwalk_kwargs['dir_fd'] = fd
            self._compare_to_walk(walk_kwargs, fwalk_kwargs)
        finally:
            os.close(fd)

    def test_yields_correct_dir_fd(self):
        # check returned file descriptors
        for topdown, follow_symlinks in itertools.product((True, False), repeat=2):
            args = os_helper.TESTFN, topdown, None
            for root, dirs, files, rootfd in self.fwalk(*args, follow_symlinks=follow_symlinks):
                # check that the FD is valid
                os.fstat(rootfd)
                # redundant check
                os.stat(rootfd)
                # check that listdir() returns consistent information
                self.assertEqual(set(os.listdir(rootfd)), set(dirs) | set(files))

    @unittest.skipIf(
        support.is_android, "dup return value is unpredictable on Android"
    )
    def test_fd_leak(self):
        # Since we're opening a lot of FDs, we must be careful to avoid leaks:
        # we both check that calling fwalk() a large number of times doesn't
        # yield EMFILE, and that the minimum allocated FD hasn't changed.
        minfd = os.dup(1)
        os.close(minfd)
        for i in range(256):
            for x in self.fwalk(os_helper.TESTFN):
                pass
        newfd = os.dup(1)
        self.addCleanup(os.close, newfd)
        self.assertEqual(newfd, minfd)

    @unittest.skipIf(
        support.is_android, "dup return value is unpredictable on Android"
    )
    def test_fd_finalization(self):
        # Check that close()ing the fwalk() generator closes FDs
        def getfd():
            fd = os.dup(1)
            os.close(fd)
            return fd
        for topdown in (False, True):
            old_fd = getfd()
            it = self.fwalk(os_helper.TESTFN, topdown=topdown)
            self.assertEqual(getfd(), old_fd)
            next(it)
            self.assertGreater(getfd(), old_fd)
            it.close()
            self.assertEqual(getfd(), old_fd)

    # fwalk() keeps file descriptors open
    test_walk_many_open_files = None


class BytesWalkTests(WalkTests):
    """Tests for os.walk() with bytes."""
    def walk(self, top, **kwargs):
        if 'follow_symlinks' in kwargs:
            kwargs['followlinks'] = kwargs.pop('follow_symlinks')
        for broot, bdirs, bfiles in os.walk(os.fsencode(top), **kwargs):
            root = os.fsdecode(broot)
            dirs = list(map(os.fsdecode, bdirs))
            files = list(map(os.fsdecode, bfiles))
            yield (root, dirs, files)
            bdirs[:] = list(map(os.fsencode, dirs))
            bfiles[:] = list(map(os.fsencode, files))

@unittest.skipUnless(hasattr(os, 'fwalk'), "Test needs os.fwalk()")
class BytesFwalkTests(FwalkTests):
    """Tests for os.walk() with bytes."""
    def fwalk(self, top='.', *args, **kwargs):
        for broot, bdirs, bfiles, topfd in os.fwalk(os.fsencode(top), *args, **kwargs):
            root = os.fsdecode(broot)
            dirs = list(map(os.fsdecode, bdirs))
            files = list(map(os.fsdecode, bfiles))
            yield (root, dirs, files, topfd)
            bdirs[:] = list(map(os.fsencode, dirs))
            bfiles[:] = list(map(os.fsencode, files))


class PosixTester(unittest.TestCase):

    def setUp(self):
        # create empty file
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        create_file(os_helper.TESTFN, b'')

    def test_listdir(self):
        self.assertIn(os_helper.TESTFN, posix.listdir(os.curdir))

    def test_listdir_default(self):
        # When listdir is called without argument,
        # it's the same as listdir(os.curdir).
        self.assertIn(os_helper.TESTFN, posix.listdir())

    def test_listdir_bytes(self):
        # When listdir is called with a bytes object,
        # the returned strings are of type bytes.
        self.assertIn(os.fsencode(os_helper.TESTFN), posix.listdir(b'.'))

    def test_listdir_bytes_like(self):
        for cls in bytearray, memoryview:
            with self.assertRaises(TypeError):
                posix.listdir(cls(b'.'))

    @unittest.skipUnless(posix.listdir in os.supports_fd,
                         "test needs fd support for posix.listdir()")
    def test_listdir_fd(self):
        f = posix.open(posix.getcwd(), posix.O_RDONLY)
        self.addCleanup(posix.close, f)
        self.assertEqual(
            sorted(posix.listdir('.')),
            sorted(posix.listdir(f))
            )
        # Check that the fd offset was reset (issue #13739)
        self.assertEqual(
            sorted(posix.listdir('.')),
            sorted(posix.listdir(f))
            )

    @unittest.skipUnless(hasattr(posix, 'chdir'), 'test needs posix.chdir()')
    def test_chdir(self):
        posix.chdir(os.curdir)
        self.assertRaises(OSError, posix.chdir, os_helper.TESTFN)


class MiscTests(unittest.TestCase):
    def test_getcwd(self):
        cwd = os.getcwd()
        self.assertIsInstance(cwd, str)

    def test_getcwd_long_path(self):
        # bpo-37412: On Linux, PATH_MAX is usually around 4096 bytes. On
        # Windows, MAX_PATH is defined as 260 characters, but Windows supports
        # longer path if longer paths support is enabled. Internally, the os
        # module uses MAXPATHLEN which is at least 1024.
        #
        # Use a directory name of 200 characters to fit into Windows MAX_PATH
        # limit.
        #
        # On Windows, the test can stop when trying to create a path longer
        # than MAX_PATH if long paths support is disabled:
        # see RtlAreLongPathsEnabled().
        min_len = 2000   # characters
        # On VxWorks, PATH_MAX is defined as 1024 bytes. Creating a path
        # longer than PATH_MAX will fail.
        if sys.platform == 'vxworks':
            min_len = 1000
        dirlen = 200     # characters
        dirname = 'python_test_dir_'
        dirname = dirname + ('a' * (dirlen - len(dirname)))

        with tempfile.TemporaryDirectory() as tmpdir:
            with os_helper.change_cwd(tmpdir) as path:
                expected = path

                while True:
                    cwd = os.getcwd()
                    self.assertEqual(cwd, expected)

                    need = min_len - (len(cwd) + len(os.path.sep))
                    if need <= 0:
                        break
                    if len(dirname) > need and need > 0:
                        dirname = dirname[:need]

                    path = os.path.join(path, dirname)
                    try:
                        os.mkdir(path)
                        # On Windows, chdir() can fail
                        # even if mkdir() succeeded
                        os.chdir(path)
                    except FileNotFoundError:
                        # On Windows, catch ERROR_PATH_NOT_FOUND (3) and
                        # ERROR_FILENAME_EXCED_RANGE (206) errors
                        # ("The filename or extension is too long")
                        break
                    except OSError as exc:
                        if exc.errno == errno.ENAMETOOLONG:
                            break
                        else:
                            raise

                    expected = path

                if support.verbose:
                    print(f"Tested current directory length: {len(cwd)}")

    @unittest.skipUnless(hasattr(posix, 'getcwd'), 'test needs posix.getcwd()')
    def test_getcwd_long_pathnames(self):
        dirname = 'getcwd-test-directory-0123456789abcdef-01234567890abcdef'
        curdir = os.getcwd()
        base_path = os.path.abspath(os_helper.TESTFN) + '.getcwd'

        try:
            os.mkdir(base_path)
            os.chdir(base_path)
        except:
            #  Just returning nothing instead of the SkipTest exception, because
            #  the test results in Error in that case.  Is that ok?
            #  raise unittest.SkipTest("cannot create directory for testing")
            return

            def _create_and_do_getcwd(dirname, current_path_length = 0):
                try:
                    os.mkdir(dirname)
                except:
                    raise unittest.SkipTest("mkdir cannot create directory sufficiently deep for getcwd test")

                os.chdir(dirname)
                try:
                    os.getcwd()
                    if current_path_length < 1027:
                        _create_and_do_getcwd(dirname, current_path_length + len(dirname) + 1)
                finally:
                    os.chdir('..')
                    os.rmdir(dirname)

            _create_and_do_getcwd(dirname)

        finally:
            os.chdir(curdir)
            os_helper.rmtree(base_path)

    def test_getcwdb(self):
        cwd = os.getcwdb()
        self.assertIsInstance(cwd, bytes)
        self.assertEqual(os.fsdecode(cwd), os.getcwd())


class MakedirTests(unittest.TestCase):
    def setUp(self):
        os.mkdir(os_helper.TESTFN)

    def test_makedir(self):
        base = os_helper.TESTFN
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

    @unittest.skipIf(
        support.is_wasi,
        "WASI's umask is a stub."
    )
    def test_mode(self):
        # Note: in some cases, the umask might already be 2 in which case this
        # will pass even if os.umask is actually broken.
        with os_helper.temp_umask(0o002):
            base = os_helper.TESTFN
            parent = os.path.join(base, 'dir1')
            path = os.path.join(parent, 'dir2')
            os.makedirs(path, 0o555)
            self.assertTrue(os.path.exists(path))
            self.assertTrue(os.path.isdir(path))
            if os.name != 'nt':
                self.assertEqual(os.stat(path).st_mode & 0o777, 0o555)
                self.assertEqual(os.stat(parent).st_mode & 0o777, 0o775)

    @unittest.skipIf(
        support.is_wasi,
        "WASI's umask is a stub."
    )
    def test_exist_ok_existing_directory(self):
        path = os.path.join(os_helper.TESTFN, 'dir1')
        mode = 0o777
        old_mask = os.umask(0o022)
        os.makedirs(path, mode)
        self.assertRaises(OSError, os.makedirs, path, mode)
        self.assertRaises(OSError, os.makedirs, path, mode, exist_ok=False)
        os.makedirs(path, 0o776, exist_ok=True)
        os.makedirs(path, mode=mode, exist_ok=True)
        os.umask(old_mask)

        # Issue #25583: A drive root could raise PermissionError on Windows
        os.makedirs(os.path.abspath('/'), exist_ok=True)

    @unittest.skipIf(
        support.is_wasi,
        "WASI's umask is a stub."
    )
    def test_exist_ok_s_isgid_directory(self):
        path = os.path.join(os_helper.TESTFN, 'dir1')
        S_ISGID = stat.S_ISGID
        mode = 0o777
        old_mask = os.umask(0o022)
        try:
            existing_testfn_mode = stat.S_IMODE(
                    os.lstat(os_helper.TESTFN).st_mode)
            try:
                os.chmod(os_helper.TESTFN, existing_testfn_mode | S_ISGID)
            except PermissionError:
                raise unittest.SkipTest('Cannot set S_ISGID for dir.')
            if (os.lstat(os_helper.TESTFN).st_mode & S_ISGID != S_ISGID):
                raise unittest.SkipTest('No support for S_ISGID dir mode.')
            # The os should apply S_ISGID from the parent dir for us, but
            # this test need not depend on that behavior.  Be explicit.
            os.makedirs(path, mode | S_ISGID)
            # http://bugs.python.org/issue14992
            # Should not fail when the bit is already set.
            os.makedirs(path, mode, exist_ok=True)
            # remove the bit.
            os.chmod(path, stat.S_IMODE(os.lstat(path).st_mode) & ~S_ISGID)
            # May work even when the bit is not already set when demanded.
            os.makedirs(path, mode | S_ISGID, exist_ok=True)
        finally:
            os.umask(old_mask)

    def test_exist_ok_existing_regular_file(self):
        base = os_helper.TESTFN
        path = os.path.join(os_helper.TESTFN, 'dir1')
        with open(path, 'w', encoding='utf-8') as f:
            f.write('abc')
        self.assertRaises(OSError, os.makedirs, path)
        self.assertRaises(OSError, os.makedirs, path, exist_ok=False)
        self.assertRaises(OSError, os.makedirs, path, exist_ok=True)
        os.remove(path)

    @unittest.skipUnless(os.name == 'nt', "requires Windows")
    def test_win32_mkdir_700(self):
        base = os_helper.TESTFN
        path = os.path.abspath(os.path.join(os_helper.TESTFN, 'dir'))
        os.mkdir(path, mode=0o700)
        out = subprocess.check_output(["cacls.exe", path, "/s"], encoding="oem")
        os.rmdir(path)
        out = out.strip().rsplit(" ", 1)[1]
        self.assertEqual(
            out,
            '"D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;FA;;;OW)"',
        )

    def tearDown(self):
        path = os.path.join(os_helper.TESTFN, 'dir1', 'dir2', 'dir3',
                            'dir4', 'dir5', 'dir6')
        # If the tests failed, the bottom-most directory ('../dir6')
        # may not have been created, so we look for the outermost directory
        # that exists.
        while not os.path.exists(path) and path != os_helper.TESTFN:
            path = os.path.dirname(path)

        os.removedirs(path)


class RemoveDirsTests(unittest.TestCase):
    def setUp(self):
        os.makedirs(os_helper.TESTFN)

    def tearDown(self):
        os_helper.rmtree(os_helper.TESTFN)

    def test_remove_all(self):
        dira = os.path.join(os_helper.TESTFN, 'dira')
        os.mkdir(dira)
        dirb = os.path.join(dira, 'dirb')
        os.mkdir(dirb)
        os.removedirs(dirb)
        self.assertFalse(os.path.exists(dirb))
        self.assertFalse(os.path.exists(dira))
        self.assertFalse(os.path.exists(os_helper.TESTFN))

    def test_remove_partial(self):
        dira = os.path.join(os_helper.TESTFN, 'dira')
        os.mkdir(dira)
        dirb = os.path.join(dira, 'dirb')
        os.mkdir(dirb)
        create_file(os.path.join(dira, 'file.txt'))
        os.removedirs(dirb)
        self.assertFalse(os.path.exists(dirb))
        self.assertTrue(os.path.exists(dira))
        self.assertTrue(os.path.exists(os_helper.TESTFN))

    def test_remove_nothing(self):
        dira = os.path.join(os_helper.TESTFN, 'dira')
        os.mkdir(dira)
        dirb = os.path.join(dira, 'dirb')
        os.mkdir(dirb)
        create_file(os.path.join(dirb, 'file.txt'))
        with self.assertRaises(OSError):
            os.removedirs(dirb)
        self.assertTrue(os.path.exists(dirb))
        self.assertTrue(os.path.exists(dira))
        self.assertTrue(os.path.exists(os_helper.TESTFN))


if __name__ == "__main__":
    unittest.main()
