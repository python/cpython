import importlib
import shutil
import stat
import sys
import os
import unittest
import socket
import tempfile
import textwrap
import errno
from test import support
from test.support import script_helper

TESTFN = support.TESTFN


class ClassicClass:
    pass

class NewStyleClass(object):
    pass


class TestSupport(unittest.TestCase):

    def test_import_module(self):
        support.import_module("ftplib")
        self.assertRaises(unittest.SkipTest, support.import_module, "foo")

    def test_import_fresh_module(self):
        support.import_fresh_module("ftplib")

    def test_get_attribute(self):
        self.assertEqual(support.get_attribute(self, "test_get_attribute"),
                        self.test_get_attribute)
        self.assertRaises(unittest.SkipTest, support.get_attribute, self, "foo")
        with self.assertRaisesRegexp(unittest.SkipTest, 'unittest'):
            support.get_attribute(unittest, 'foo')
        with self.assertRaisesRegexp(unittest.SkipTest, 'ClassicClass'):
            support.get_attribute(ClassicClass, 'foo')
        with self.assertRaisesRegexp(unittest.SkipTest, 'ClassicClass'):
            support.get_attribute(ClassicClass(), 'foo')
        with self.assertRaisesRegexp(unittest.SkipTest, 'NewStyleClass'):
            support.get_attribute(NewStyleClass, 'foo')
        with self.assertRaisesRegexp(unittest.SkipTest, 'NewStyleClass'):
            support.get_attribute(NewStyleClass(), 'foo')

    @unittest.skip("failing buildbots")
    def test_get_original_stdout(self):
        self.assertEqual(support.get_original_stdout(), sys.stdout)

    def test_unload(self):
        import sched
        self.assertIn("sched", sys.modules)
        support.unload("sched")
        self.assertNotIn("sched", sys.modules)

    def test_unlink(self):
        with open(TESTFN, "w") as f:
            pass
        support.unlink(TESTFN)
        self.assertFalse(os.path.exists(TESTFN))
        support.unlink(TESTFN)

    def test_rmtree(self):
        dirpath = support.TESTFN + 'd'
        subdirpath = os.path.join(dirpath, 'subdir')
        os.mkdir(dirpath)
        os.mkdir(subdirpath)
        support.rmtree(dirpath)
        self.assertFalse(os.path.exists(dirpath))
        with support.swap_attr(support, 'verbose', 0):
            support.rmtree(dirpath)

        os.mkdir(dirpath)
        os.mkdir(subdirpath)
        os.chmod(dirpath, stat.S_IRUSR|stat.S_IXUSR)
        with support.swap_attr(support, 'verbose', 0):
            support.rmtree(dirpath)
        self.assertFalse(os.path.exists(dirpath))

        os.mkdir(dirpath)
        os.mkdir(subdirpath)
        os.chmod(dirpath, 0)
        with support.swap_attr(support, 'verbose', 0):
            support.rmtree(dirpath)
        self.assertFalse(os.path.exists(dirpath))

    def test_forget(self):
        mod_filename = TESTFN + '.py'
        with open(mod_filename, 'wt') as f:
            f.write('foo = 1\n')
        sys.path.insert(0, os.curdir)
        try:
            mod = __import__(TESTFN)
            self.assertIn(TESTFN, sys.modules)

            support.forget(TESTFN)
            self.assertNotIn(TESTFN, sys.modules)
        finally:
            del sys.path[0]
            support.unlink(mod_filename)
            support.rmtree('__pycache__')

    def test_HOST(self):
        s = socket.socket()
        s.bind((support.HOST, 0))
        s.close()

    def test_find_unused_port(self):
        port = support.find_unused_port()
        s = socket.socket()
        s.bind((support.HOST, port))
        s.close()

    def test_bind_port(self):
        s = socket.socket()
        support.bind_port(s)
        s.listen(5)
        s.close()

    # Tests for temp_dir()

    def test_temp_dir(self):
        """Test that temp_dir() creates and destroys its directory."""
        parent_dir = tempfile.mkdtemp()
        parent_dir = os.path.realpath(parent_dir)

        try:
            path = os.path.join(parent_dir, 'temp')
            self.assertFalse(os.path.isdir(path))
            with support.temp_dir(path) as temp_path:
                self.assertEqual(temp_path, path)
                self.assertTrue(os.path.isdir(path))
            self.assertFalse(os.path.isdir(path))
        finally:
            support.rmtree(parent_dir)

    def test_temp_dir__path_none(self):
        """Test passing no path."""
        with support.temp_dir() as temp_path:
            self.assertTrue(os.path.isdir(temp_path))
        self.assertFalse(os.path.isdir(temp_path))

    def test_temp_dir__existing_dir__quiet_default(self):
        """Test passing a directory that already exists."""
        def call_temp_dir(path):
            with support.temp_dir(path) as temp_path:
                raise Exception("should not get here")

        path = tempfile.mkdtemp()
        path = os.path.realpath(path)
        try:
            self.assertTrue(os.path.isdir(path))
            with self.assertRaises(OSError) as cm:
                call_temp_dir(path)
            self.assertEqual(cm.exception.errno, errno.EEXIST)
            # Make sure temp_dir did not delete the original directory.
            self.assertTrue(os.path.isdir(path))
        finally:
            shutil.rmtree(path)

    def test_temp_dir__existing_dir__quiet_true(self):
        """Test passing a directory that already exists with quiet=True."""
        path = tempfile.mkdtemp()
        path = os.path.realpath(path)

        try:
            with support.check_warnings() as recorder:
                with support.temp_dir(path, quiet=True) as temp_path:
                    self.assertEqual(path, temp_path)
                warnings = [str(w.message) for w in recorder.warnings]
            # Make sure temp_dir did not delete the original directory.
            self.assertTrue(os.path.isdir(path))
        finally:
            shutil.rmtree(path)

        expected = ['tests may fail, unable to create temp dir: ' + path]
        self.assertEqual(warnings, expected)

    @unittest.skipUnless(hasattr(os, "fork"), "test requires os.fork")
    def test_temp_dir__forked_child(self):
        """Test that a forked child process does not remove the directory."""
        # See bpo-30028 for details.
        # Run the test as an external script, because it uses fork.
        script_helper.assert_python_ok("-c", textwrap.dedent("""
            import os
            from test import support
            with support.temp_cwd() as temp_path:
                pid = os.fork()
                if pid != 0:
                    # parent process (child has pid == 0)

                    # wait for the child to terminate
                    (pid, status) = os.waitpid(pid, 0)
                    if status != 0:
                        raise AssertionError("Child process failed with exit "
                                             "status indication "
                                             "0x{:x}.".format(status))

                    # Make sure that temp_path is still present. When the child
                    # process leaves the 'temp_cwd'-context, the __exit__()-
                    # method of the context must not remove the temporary
                    # directory.
                    if not os.path.isdir(temp_path):
                        raise AssertionError("Child removed temp_path.")
        """))

    # Tests for change_cwd()

    def test_change_cwd(self):
        original_cwd = os.getcwd()

        with support.temp_dir() as temp_path:
            with support.change_cwd(temp_path) as new_cwd:
                self.assertEqual(new_cwd, temp_path)
                self.assertEqual(os.getcwd(), new_cwd)

        self.assertEqual(os.getcwd(), original_cwd)

    def test_change_cwd__non_existent_dir(self):
        """Test passing a non-existent directory."""
        original_cwd = os.getcwd()

        def call_change_cwd(path):
            with support.change_cwd(path) as new_cwd:
                raise Exception("should not get here")

        with support.temp_dir() as parent_dir:
            non_existent_dir = os.path.join(parent_dir, 'does_not_exist')
            with self.assertRaises(OSError) as cm:
                call_change_cwd(non_existent_dir)
            self.assertEqual(cm.exception.errno, errno.ENOENT)

        self.assertEqual(os.getcwd(), original_cwd)

    def test_change_cwd__non_existent_dir__quiet_true(self):
        """Test passing a non-existent directory with quiet=True."""
        original_cwd = os.getcwd()

        with support.temp_dir() as parent_dir:
            bad_dir = os.path.join(parent_dir, 'does_not_exist')
            with support.check_warnings() as recorder:
                with support.change_cwd(bad_dir, quiet=True) as new_cwd:
                    self.assertEqual(new_cwd, original_cwd)
                    self.assertEqual(os.getcwd(), new_cwd)
                warnings = [str(w.message) for w in recorder.warnings]

        expected = ['tests may fail, unable to change CWD to: ' + bad_dir]
        self.assertEqual(warnings, expected)

    # Tests for change_cwd()

    def test_change_cwd__chdir_warning(self):
        """Check the warning message when os.chdir() fails."""
        path = TESTFN + '_does_not_exist'
        with support.check_warnings() as recorder:
            with support.change_cwd(path=path, quiet=True):
                pass
            messages = [str(w.message) for w in recorder.warnings]
        self.assertEqual(messages, ['tests may fail, unable to change CWD to: ' + path])

    # Tests for temp_cwd()

    def test_temp_cwd(self):
        here = os.getcwd()
        with support.temp_cwd(name=TESTFN):
            self.assertEqual(os.path.basename(os.getcwd()), TESTFN)
        self.assertFalse(os.path.exists(TESTFN))
        self.assertEqual(os.getcwd(), here)


    def test_temp_cwd__name_none(self):
        """Test passing None to temp_cwd()."""
        original_cwd = os.getcwd()
        with support.temp_cwd(name=None) as new_cwd:
            self.assertNotEqual(new_cwd, original_cwd)
            self.assertTrue(os.path.isdir(new_cwd))
            self.assertEqual(os.getcwd(), new_cwd)
        self.assertEqual(os.getcwd(), original_cwd)

    def test_sortdict(self):
        self.assertEqual(support.sortdict({3:3, 2:2, 1:1}), "{1: 1, 2: 2, 3: 3}")

    def test_make_bad_fd(self):
        fd = support.make_bad_fd()
        with self.assertRaises(OSError) as cm:
            os.write(fd, b"foo")
        self.assertEqual(cm.exception.errno, errno.EBADF)

    def test_check_syntax_error(self):
        support.check_syntax_error(self, "def class", lineno=1, offset=9)
        with self.assertRaises(AssertionError):
            support.check_syntax_error(self, "x=1")

    def test_CleanImport(self):
        import importlib
        with support.CleanImport("asyncore"):
            importlib.import_module("asyncore")

    def test_DirsOnSysPath(self):
        with support.DirsOnSysPath('foo', 'bar'):
            self.assertIn("foo", sys.path)
            self.assertIn("bar", sys.path)
        self.assertNotIn("foo", sys.path)
        self.assertNotIn("bar", sys.path)

    def test_captured_stdout(self):
        with support.captured_stdout() as stdout:
            print "hello"
        self.assertEqual(stdout.getvalue(), "hello\n")

    def test_captured_stderr(self):
        with support.captured_stderr() as stderr:
            print >>sys.stderr, "hello"
        self.assertEqual(stderr.getvalue(), "hello\n")

    def test_captured_stdin(self):
        with support.captured_stdin() as stdin:
            stdin.write('hello\n')
            stdin.seek(0)
            # call test code that consumes from sys.stdin
            captured = raw_input()
        self.assertEqual(captured, "hello")

    def test_gc_collect(self):
        support.gc_collect()

    def test_python_is_optimized(self):
        self.assertIsInstance(support.python_is_optimized(), bool)

    def test_swap_attr(self):
        class Obj:
            pass
        obj = Obj()
        obj.x = 1
        with support.swap_attr(obj, "x", 5) as x:
            self.assertEqual(obj.x, 5)
            self.assertEqual(x, 1)
        self.assertEqual(obj.x, 1)
        with support.swap_attr(obj, "y", 5) as y:
            self.assertEqual(obj.y, 5)
            self.assertIsNone(y)
        self.assertFalse(hasattr(obj, 'y'))
        with support.swap_attr(obj, "y", 5):
            del obj.y
        self.assertFalse(hasattr(obj, 'y'))

    def test_swap_item(self):
        D = {"x":1}
        with support.swap_item(D, "x", 5) as x:
            self.assertEqual(D["x"], 5)
            self.assertEqual(x, 1)
        self.assertEqual(D["x"], 1)
        with support.swap_item(D, "y", 5) as y:
            self.assertEqual(D["y"], 5)
            self.assertIsNone(y)
        self.assertNotIn("y", D)
        with support.swap_item(D, "y", 5):
            del D["y"]
        self.assertNotIn("y", D)

    def test_match_test(self):
        class Test:
            def __init__(self, test_id):
                self.test_id = test_id

            def id(self):
                return self.test_id

        test_access = Test('test.test_os.FileTests.test_access')
        test_chdir = Test('test.test_os.Win32ErrorTests.test_chdir')

        with support.swap_attr(support, '_match_test_func', None):
            # match all
            support.set_match_tests([])
            self.assertTrue(support.match_test(test_access))
            self.assertTrue(support.match_test(test_chdir))

            # match all using None
            support.set_match_tests(None)
            self.assertTrue(support.match_test(test_access))
            self.assertTrue(support.match_test(test_chdir))

            # match the full test identifier
            support.set_match_tests([test_access.id()])
            self.assertTrue(support.match_test(test_access))
            self.assertFalse(support.match_test(test_chdir))

            # match the module name
            support.set_match_tests(['test_os'])
            self.assertTrue(support.match_test(test_access))
            self.assertTrue(support.match_test(test_chdir))

            # Test '*' pattern
            support.set_match_tests(['test_*'])
            self.assertTrue(support.match_test(test_access))
            self.assertTrue(support.match_test(test_chdir))

            # Test case sensitivity
            support.set_match_tests(['filetests'])
            self.assertFalse(support.match_test(test_access))
            support.set_match_tests(['FileTests'])
            self.assertTrue(support.match_test(test_access))

            # Test pattern containing '.' and a '*' metacharacter
            support.set_match_tests(['*test_os.*.test_*'])
            self.assertTrue(support.match_test(test_access))
            self.assertTrue(support.match_test(test_chdir))

            # Multiple patterns
            support.set_match_tests([test_access.id(), test_chdir.id()])
            self.assertTrue(support.match_test(test_access))
            self.assertTrue(support.match_test(test_chdir))

            support.set_match_tests(['test_access', 'DONTMATCH'])
            self.assertTrue(support.match_test(test_access))
            self.assertFalse(support.match_test(test_chdir))

    def test_fd_count(self):
        # We cannot test the absolute value of fd_count(): on old Linux
        # kernel or glibc versions, os.urandom() keeps a FD open on
        # /dev/urandom device and Python has 4 FD opens instead of 3.
        start = support.fd_count()
        fd = os.open(__file__, os.O_RDONLY)
        try:
            more = support.fd_count()
        finally:
            os.close(fd)
        self.assertEqual(more - start, 1)

    # XXX -follows a list of untested API
    # make_legacy_pyc
    # is_resource_enabled
    # requires
    # fcmp
    # umaks
    # findfile
    # check_warnings
    # EnvironmentVarGuard
    # TransientResource
    # transient_internet
    # run_with_locale
    # set_memlimit
    # bigmemtest
    # precisionbigmemtest
    # bigaddrspacetest
    # requires_resource
    # run_doctest
    # threading_cleanup
    # reap_threads
    # reap_children
    # strip_python_stderr
    # args_from_interpreter_flags
    # can_symlink
    # skip_unless_symlink
    # SuppressCrashReport


def test_main():
    tests = [TestSupport]
    support.run_unittest(*tests)

if __name__ == '__main__':
    test_main()
