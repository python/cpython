import contextlib
import errno
import importlib
import io
import logging
import os
import shutil
import signal
import socket
import stat
import subprocess
import sys
import sysconfig
import tempfile
import textwrap
import unittest
import warnings

from test import support
from test.support import import_helper
from test.support import os_helper
from test.support import script_helper
from test.support import socket_helper
from test.support import warnings_helper

TESTFN = os_helper.TESTFN


class LogCaptureHandler(logging.StreamHandler):
    # Inspired by pytest's caplog
    def __init__(self):
        super().__init__(io.StringIO())
        self.records = []

    def emit(self, record) -> None:
        self.records.append(record)
        super().emit(record)

    def handleError(self, record):
        raise


@contextlib.contextmanager
def _caplog():
    handler = LogCaptureHandler()
    root_logger = logging.getLogger()
    root_logger.addHandler(handler)
    try:
        yield handler
    finally:
        root_logger.removeHandler(handler)


class TestSupport(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        orig_filter_len = len(warnings._get_filters())
        cls._warnings_helper_token = support.ignore_deprecations_from(
            "test.support.warnings_helper", like=".*used in test_support.*"
        )
        cls._test_support_token = support.ignore_deprecations_from(
            __name__, like=".*You should NOT be seeing this.*"
        )
        assert len(warnings._get_filters()) == orig_filter_len + 2

    @classmethod
    def tearDownClass(cls):
        orig_filter_len = len(warnings._get_filters())
        support.clear_ignored_deprecations(
            cls._warnings_helper_token,
            cls._test_support_token,
        )
        assert len(warnings._get_filters()) == orig_filter_len - 2

    def test_ignored_deprecations_are_silent(self):
        """Test support.ignore_deprecations_from() silences warnings"""
        with warnings.catch_warnings(record=True) as warning_objs:
            warnings_helper._warn_about_deprecation()
            warnings.warn("You should NOT be seeing this.", DeprecationWarning)
            messages = [str(w.message) for w in warning_objs]
        self.assertEqual(len(messages), 0, messages)

    def test_import_module(self):
        import_helper.import_module("ftplib")
        self.assertRaises(unittest.SkipTest,
                          import_helper.import_module, "foo")

    def test_import_fresh_module(self):
        import_helper.import_fresh_module("ftplib")

    def test_get_attribute(self):
        self.assertEqual(support.get_attribute(self, "test_get_attribute"),
                        self.test_get_attribute)
        self.assertRaises(unittest.SkipTest, support.get_attribute, self, "foo")

    @unittest.skip("failing buildbots")
    def test_get_original_stdout(self):
        self.assertEqual(support.get_original_stdout(), sys.stdout)

    def test_unload(self):
        import sched  # noqa: F401
        self.assertIn("sched", sys.modules)
        import_helper.unload("sched")
        self.assertNotIn("sched", sys.modules)

    def test_unlink(self):
        with open(TESTFN, "w", encoding="utf-8") as f:
            pass
        os_helper.unlink(TESTFN)
        self.assertFalse(os.path.exists(TESTFN))
        os_helper.unlink(TESTFN)

    def test_rmtree(self):
        dirpath = os_helper.TESTFN + 'd'
        subdirpath = os.path.join(dirpath, 'subdir')
        os.mkdir(dirpath)
        os.mkdir(subdirpath)
        os_helper.rmtree(dirpath)
        self.assertFalse(os.path.exists(dirpath))
        with support.swap_attr(support, 'verbose', 0):
            os_helper.rmtree(dirpath)

        os.mkdir(dirpath)
        os.mkdir(subdirpath)
        os.chmod(dirpath, stat.S_IRUSR|stat.S_IXUSR)
        with support.swap_attr(support, 'verbose', 0):
            os_helper.rmtree(dirpath)
        self.assertFalse(os.path.exists(dirpath))

        os.mkdir(dirpath)
        os.mkdir(subdirpath)
        os.chmod(dirpath, 0)
        with support.swap_attr(support, 'verbose', 0):
            os_helper.rmtree(dirpath)
        self.assertFalse(os.path.exists(dirpath))

    def test_forget(self):
        mod_filename = TESTFN + '.py'
        with open(mod_filename, 'w', encoding="utf-8") as f:
            print('foo = 1', file=f)
        sys.path.insert(0, os.curdir)
        importlib.invalidate_caches()
        try:
            mod = __import__(TESTFN)
            self.assertIn(TESTFN, sys.modules)

            import_helper.forget(TESTFN)
            self.assertNotIn(TESTFN, sys.modules)
        finally:
            del sys.path[0]
            os_helper.unlink(mod_filename)
            os_helper.rmtree('__pycache__')

    @support.requires_working_socket()
    def test_HOST(self):
        s = socket.create_server((socket_helper.HOST, 0))
        s.close()

    @support.requires_working_socket()
    def test_find_unused_port(self):
        port = socket_helper.find_unused_port()
        s = socket.create_server((socket_helper.HOST, port))
        s.close()

    @support.requires_working_socket()
    def test_bind_port(self):
        s = socket.socket()
        socket_helper.bind_port(s)
        s.listen()
        s.close()

    # Tests for temp_dir()

    def test_temp_dir(self):
        """Test that temp_dir() creates and destroys its directory."""
        parent_dir = tempfile.mkdtemp()
        parent_dir = os.path.realpath(parent_dir)

        try:
            path = os.path.join(parent_dir, 'temp')
            self.assertFalse(os.path.isdir(path))
            with os_helper.temp_dir(path) as temp_path:
                self.assertEqual(temp_path, path)
                self.assertTrue(os.path.isdir(path))
            self.assertFalse(os.path.isdir(path))
        finally:
            os_helper.rmtree(parent_dir)

    def test_temp_dir__path_none(self):
        """Test passing no path."""
        with os_helper.temp_dir() as temp_path:
            self.assertTrue(os.path.isdir(temp_path))
        self.assertFalse(os.path.isdir(temp_path))

    def test_temp_dir__existing_dir__quiet_default(self):
        """Test passing a directory that already exists."""
        def call_temp_dir(path):
            with os_helper.temp_dir(path) as temp_path:
                raise Exception("should not get here")

        path = tempfile.mkdtemp()
        path = os.path.realpath(path)
        try:
            self.assertTrue(os.path.isdir(path))
            self.assertRaises(FileExistsError, call_temp_dir, path)
            # Make sure temp_dir did not delete the original directory.
            self.assertTrue(os.path.isdir(path))
        finally:
            shutil.rmtree(path)

    def test_temp_dir__existing_dir__quiet_true(self):
        """Test passing a directory that already exists with quiet=True."""
        path = tempfile.mkdtemp()
        path = os.path.realpath(path)

        try:
            with warnings_helper.check_warnings() as recorder, _caplog() as caplog:
                with os_helper.temp_dir(path, quiet=True) as temp_path:
                    self.assertEqual(path, temp_path)
                warnings = [str(w.message) for w in recorder.warnings]
            # Make sure temp_dir did not delete the original directory.
            self.assertTrue(os.path.isdir(path))
        finally:
            shutil.rmtree(path)

        self.assertListEqual(warnings, [])
        self.assertEqual(len(caplog.records), 1)
        record = caplog.records[0]
        self.assertStartsWith(
            record.getMessage(),
            f'tests may fail, unable to create '
            f'temporary directory {path!r}: '
        )

    @support.requires_fork()
    def test_temp_dir__forked_child(self):
        """Test that a forked child process does not remove the directory."""
        # See bpo-30028 for details.
        # Run the test as an external script, because it uses fork.
        script_helper.assert_python_ok("-c", textwrap.dedent("""
            import os
            from test import support
            from test.support import os_helper
            with os_helper.temp_cwd() as temp_path:
                pid = os.fork()
                if pid != 0:
                    # parent process

                    # wait for the child to terminate
                    support.wait_process(pid, exitcode=0)

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

        with os_helper.temp_dir() as temp_path:
            with os_helper.change_cwd(temp_path) as new_cwd:
                self.assertEqual(new_cwd, temp_path)
                self.assertEqual(os.getcwd(), new_cwd)

        self.assertEqual(os.getcwd(), original_cwd)

    def test_change_cwd__non_existent_dir(self):
        """Test passing a non-existent directory."""
        original_cwd = os.getcwd()

        def call_change_cwd(path):
            with os_helper.change_cwd(path) as new_cwd:
                raise Exception("should not get here")

        with os_helper.temp_dir() as parent_dir:
            non_existent_dir = os.path.join(parent_dir, 'does_not_exist')
            self.assertRaises(FileNotFoundError, call_change_cwd,
                              non_existent_dir)

        self.assertEqual(os.getcwd(), original_cwd)

    def test_change_cwd__non_existent_dir__quiet_true(self):
        """Test passing a non-existent directory with quiet=True."""
        original_cwd = os.getcwd()

        with os_helper.temp_dir() as parent_dir:
            bad_dir = os.path.join(parent_dir, 'does_not_exist')
            with warnings_helper.check_warnings() as recorder, _caplog() as caplog:
                with os_helper.change_cwd(bad_dir, quiet=True) as new_cwd:
                    self.assertEqual(new_cwd, original_cwd)
                    self.assertEqual(os.getcwd(), new_cwd)
                warnings = [str(w.message) for w in recorder.warnings]

        self.assertListEqual(warnings, [])
        self.assertEqual(len(caplog.records), 1)
        record = caplog.records[0]
        self.assertStartsWith(
            record.getMessage(),
            f'tests may fail, unable to change '
            f'the current working directory '
            f'to {bad_dir!r}: '
        )

    # Tests for change_cwd()

    def test_change_cwd__chdir_warning(self):
        """Check the warning message when os.chdir() fails."""
        path = TESTFN + '_does_not_exist'
        with warnings_helper.check_warnings() as recorder, _caplog() as caplog:
            with os_helper.change_cwd(path=path, quiet=True):
                pass
            messages = [str(w.message) for w in recorder.warnings]

        self.assertListEqual(messages, [])
        self.assertEqual(len(caplog.records), 1)
        record = caplog.records[0]
        self.assertStartsWith(
            record.getMessage(),
            f'tests may fail, unable to change '
            f'the current working directory '
            f'to {path!r}: ',
        )

    # Tests for temp_cwd()

    def test_temp_cwd(self):
        here = os.getcwd()
        with os_helper.temp_cwd(name=TESTFN):
            self.assertEqual(os.path.basename(os.getcwd()), TESTFN)
        self.assertFalse(os.path.exists(TESTFN))
        self.assertEqual(os.getcwd(), here)


    def test_temp_cwd__name_none(self):
        """Test passing None to temp_cwd()."""
        original_cwd = os.getcwd()
        with os_helper.temp_cwd(name=None) as new_cwd:
            self.assertNotEqual(new_cwd, original_cwd)
            self.assertTrue(os.path.isdir(new_cwd))
            self.assertEqual(os.getcwd(), new_cwd)
        self.assertEqual(os.getcwd(), original_cwd)

    def test_sortdict(self):
        self.assertEqual(support.sortdict({3:3, 2:2, 1:1}), "{1: 1, 2: 2, 3: 3}")

    def test_make_bad_fd(self):
        fd = os_helper.make_bad_fd()
        with self.assertRaises(OSError) as cm:
            os.write(fd, b"foo")
        self.assertEqual(cm.exception.errno, errno.EBADF)

    def test_check_syntax_error(self):
        support.check_syntax_error(self, "def class", lineno=1, offset=5)
        with self.assertRaises(AssertionError):
            support.check_syntax_error(self, "x=1")

    def test_CleanImport(self):
        import importlib
        with import_helper.CleanImport("pprint"):
            importlib.import_module("pprint")

    def test_DirsOnSysPath(self):
        with import_helper.DirsOnSysPath('foo', 'bar'):
            self.assertIn("foo", sys.path)
            self.assertIn("bar", sys.path)
        self.assertNotIn("foo", sys.path)
        self.assertNotIn("bar", sys.path)

    def test_captured_stdout(self):
        with support.captured_stdout() as stdout:
            print("hello")
        self.assertEqual(stdout.getvalue(), "hello\n")

    def test_captured_stderr(self):
        with support.captured_stderr() as stderr:
            print("hello", file=sys.stderr)
        self.assertEqual(stderr.getvalue(), "hello\n")

    def test_captured_stdin(self):
        with support.captured_stdin() as stdin:
            stdin.write('hello\n')
            stdin.seek(0)
            # call test code that consumes from sys.stdin
            captured = input()
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
        self.assertNotHasAttr(obj, 'y')
        with support.swap_attr(obj, "y", 5):
            del obj.y
        self.assertNotHasAttr(obj, 'y')

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

    class RefClass:
        attribute1 = None
        attribute2 = None
        _hidden_attribute1 = None
        __magic_1__ = None

    class OtherClass:
        attribute2 = None
        attribute3 = None
        __magic_1__ = None
        __magic_2__ = None

    def test_detect_api_mismatch(self):
        missing_items = support.detect_api_mismatch(self.RefClass,
                                                    self.OtherClass)
        self.assertEqual({'attribute1'}, missing_items)

        missing_items = support.detect_api_mismatch(self.OtherClass,
                                                    self.RefClass)
        self.assertEqual({'attribute3', '__magic_2__'}, missing_items)

    def test_detect_api_mismatch__ignore(self):
        ignore = ['attribute1', 'attribute3', '__magic_2__', 'not_in_either']

        missing_items = support.detect_api_mismatch(
                self.RefClass, self.OtherClass, ignore=ignore)
        self.assertEqual(set(), missing_items)

        missing_items = support.detect_api_mismatch(
                self.OtherClass, self.RefClass, ignore=ignore)
        self.assertEqual(set(), missing_items)

    def test_check__all__(self):
        extra = {'tempdir'}
        not_exported = {'template'}
        support.check__all__(self,
                             tempfile,
                             extra=extra,
                             not_exported=not_exported)

        extra = {
            'TextTestResult',
            'installHandler',
        }
        not_exported = {'load_tests', "TestProgram", "BaseTestSuite"}
        support.check__all__(self,
                             unittest,
                             ("unittest.result", "unittest.case",
                              "unittest.suite", "unittest.loader",
                              "unittest.main", "unittest.runner",
                              "unittest.signals", "unittest.async_case"),
                             extra=extra,
                             not_exported=not_exported)

        self.assertRaises(AssertionError, support.check__all__, self, unittest)

    @unittest.skipUnless(hasattr(os, 'waitpid') and hasattr(os, 'WNOHANG'),
                         'need os.waitpid() and os.WNOHANG')
    @support.requires_fork()
    def test_reap_children(self):
        # Make sure that there is no other pending child process
        support.reap_children()

        # Create a child process
        pid = os.fork()
        if pid == 0:
            # child process: do nothing, just exit
            os._exit(0)

        was_altered = support.environment_altered
        try:
            support.environment_altered = False
            stderr = io.StringIO()

            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                with support.swap_attr(support.print_warning, 'orig_stderr', stderr):
                    support.reap_children()

                # Use environment_altered to check if reap_children() found
                # the child process
                if support.environment_altered:
                    break

            msg = "Warning -- reap_children() reaped child process %s" % pid
            self.assertIn(msg, stderr.getvalue())
            self.assertTrue(support.environment_altered)
        finally:
            support.environment_altered = was_altered

        # Just in case, check again that there is no other
        # pending child process
        support.reap_children()

    @support.requires_subprocess()
    def check_options(self, args, func, expected=None):
        code = f'from test.support import {func}; print(repr({func}()))'
        cmd = [sys.executable, *args, '-c', code]
        env = {key: value for key, value in os.environ.items()
               if not key.startswith('PYTHON')}
        proc = subprocess.run(cmd,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.DEVNULL,
                              universal_newlines=True,
                              env=env)
        if expected is None:
            expected = args
        self.assertEqual(proc.stdout.rstrip(), repr(expected))
        self.assertEqual(proc.returncode, 0)

    @support.requires_resource('cpu')
    def test_args_from_interpreter_flags(self):
        # Test test.support.args_from_interpreter_flags()
        for opts in (
            # no option
            [],
            # single option
            ['-B'],
            ['-s'],
            ['-S'],
            ['-E'],
            ['-v'],
            ['-b'],
            ['-P'],
            ['-q'],
            ['-I'],
            # same option multiple times
            ['-bb'],
            ['-vvv'],
            # -W options
            ['-Wignore'],
            # -X options
            ['-X', 'dev'],
            ['-Wignore', '-X', 'dev'],
            ['-X', 'faulthandler'],
            ['-X', 'importtime'],
            ['-X', 'importtime=2'],
            ['-X', 'showrefcount'],
            ['-X', 'tracemalloc'],
            ['-X', 'tracemalloc=3'],
        ):
            with self.subTest(opts=opts):
                self.check_options(opts, 'args_from_interpreter_flags')

        self.check_options(['-I', '-E', '-s', '-P'],
                           'args_from_interpreter_flags',
                           ['-I'])

    def test_optim_args_from_interpreter_flags(self):
        # Test test.support.optim_args_from_interpreter_flags()
        for opts in (
            # no option
            [],
            ['-O'],
            ['-OO'],
            ['-OOOO'],
        ):
            with self.subTest(opts=opts):
                self.check_options(opts, 'optim_args_from_interpreter_flags')

    @unittest.skipIf(support.is_apple_mobile, "Unstable on Apple Mobile")
    @unittest.skipIf(support.is_wasi, "Unavailable on WASI")
    def test_fd_count(self):
        # We cannot test the absolute value of fd_count(): on old Linux kernel
        # or glibc versions, os.urandom() keeps a FD open on /dev/urandom
        # device and Python has 4 FD opens instead of 3. Test is unstable on
        # Emscripten and Apple Mobile platforms; these platforms start and stop
        # background threads that use pipes and epoll fds.
        start = os_helper.fd_count()
        fd = os.open(__file__, os.O_RDONLY)
        try:
            more = os_helper.fd_count()
        finally:
            os.close(fd)
        self.assertEqual(more - start, 1)

    def check_print_warning(self, msg, expected):
        stderr = io.StringIO()
        with support.swap_attr(support.print_warning, 'orig_stderr', stderr):
            support.print_warning(msg)
        self.assertEqual(stderr.getvalue(), expected)

    def test_print_warning(self):
        self.check_print_warning("msg",
                                 "Warning -- msg\n")
        self.check_print_warning("a\nb",
                                 'Warning -- a\nWarning -- b\n')

    def test_has_strftime_extensions(self):
        if sys.platform == "win32":
            self.assertFalse(support.has_strftime_extensions)
        else:
            self.assertTrue(support.has_strftime_extensions)

    def test_get_recursion_depth(self):
        # test support.get_recursion_depth()
        code = textwrap.dedent("""
            from test import support
            import sys

            def check(cond):
                if not cond:
                    raise AssertionError("test failed")

            # depth 1
            check(support.get_recursion_depth() == 1)

            # depth 2
            def test_func():
                check(support.get_recursion_depth() == 2)
            test_func()

            def test_recursive(depth, limit):
                if depth >= limit:
                    # cannot call get_recursion_depth() at this depth,
                    # it can raise RecursionError
                    return
                get_depth = support.get_recursion_depth()
                print(f"test_recursive: {depth}/{limit}: "
                      f"get_recursion_depth() says {get_depth}")
                check(get_depth == depth)
                test_recursive(depth + 1, limit)

            # depth up to 25
            with support.infinite_recursion(max_depth=25):
                limit = sys.getrecursionlimit()
                print(f"test with sys.getrecursionlimit()={limit}")
                test_recursive(2, limit)

            # depth up to 500
            with support.infinite_recursion(max_depth=500):
                limit = sys.getrecursionlimit()
                print(f"test with sys.getrecursionlimit()={limit}")
                test_recursive(2, limit)
        """)
        script_helper.assert_python_ok("-c", code)

    def test_recursion(self):
        # Test infinite_recursion() and get_recursion_available() functions.
        def recursive_function(depth):
            if depth:
                recursive_function(depth - 1)

        for max_depth in (5, 25, 250, 2500):
            with support.infinite_recursion(max_depth):
                available = support.get_recursion_available()

                # Recursion up to 'available' additional frames should be OK.
                recursive_function(available)

                # Recursion up to 'available+1' additional frames must raise
                # RecursionError. Avoid self.assertRaises(RecursionError) which
                # can consume more than 3 frames and so raises RecursionError.
                try:
                    recursive_function(available + 1)
                except RecursionError:
                    pass
                else:
                    self.fail("RecursionError was not raised")

        # Test the bare minimumum: max_depth=3
        with support.infinite_recursion(3):
            try:
                recursive_function(3)
            except RecursionError:
                pass
            else:
                self.fail("RecursionError was not raised")

    def test_parse_memlimit(self):
        parse = support._parse_memlimit
        KiB = 1024
        MiB = KiB * 1024
        GiB = MiB * 1024
        TiB = GiB * 1024
        self.assertEqual(parse('0k'), 0)
        self.assertEqual(parse('3k'), 3 * KiB)
        self.assertEqual(parse('2.4m'), int(2.4 * MiB))
        self.assertEqual(parse('4g'), int(4 * GiB))
        self.assertEqual(parse('1t'), TiB)

        for limit in ('', '3', '3.5.10k', '10x'):
            with self.subTest(limit=limit):
                with self.assertRaises(ValueError):
                    parse(limit)

    def test_set_memlimit(self):
        _4GiB = 4 * 1024 ** 3
        TiB = 1024 ** 4
        old_max_memuse = support.max_memuse
        old_real_max_memuse = support.real_max_memuse
        try:
            if sys.maxsize > 2**32:
                support.set_memlimit('4g')
                self.assertEqual(support.max_memuse, _4GiB)
                self.assertEqual(support.real_max_memuse, _4GiB)

                big = 2**100 // TiB
                support.set_memlimit(f'{big}t')
                self.assertEqual(support.max_memuse, sys.maxsize)
                self.assertEqual(support.real_max_memuse, big * TiB)
            else:
                support.set_memlimit('4g')
                self.assertEqual(support.max_memuse, sys.maxsize)
                self.assertEqual(support.real_max_memuse, _4GiB)
        finally:
            support.max_memuse = old_max_memuse
            support.real_max_memuse = old_real_max_memuse

    def test_copy_python_src_ignore(self):
        # Get source directory
        src_dir = sysconfig.get_config_var('abs_srcdir')
        if not src_dir:
            src_dir = sysconfig.get_config_var('srcdir')
        src_dir = os.path.abspath(src_dir)

        # Check that the source code is available
        if not os.path.exists(src_dir):
            self.skipTest(f"cannot access Python source code directory:"
                          f" {src_dir!r}")
        # Check that the landmark copy_python_src_ignore() expects is available
        # (Previously we looked for 'Lib\os.py', which is always present on Windows.)
        landmark = os.path.join(src_dir, 'Modules')
        if not os.path.exists(landmark):
            self.skipTest(f"cannot access Python source code directory:"
                          f" {landmark!r} landmark is missing")

        # Test support.copy_python_src_ignore()

        # Source code directory
        ignored = {'.git', '__pycache__'}
        names = os.listdir(src_dir)
        self.assertEqual(support.copy_python_src_ignore(src_dir, names),
                         ignored | {'build'})

        # Doc/ directory
        path = os.path.join(src_dir, 'Doc')
        self.assertEqual(support.copy_python_src_ignore(path, os.listdir(path)),
                         ignored | {'build', 'venv'})

        # Another directory
        path = os.path.join(src_dir, 'Objects')
        self.assertEqual(support.copy_python_src_ignore(path, os.listdir(path)),
                         ignored)

    def test_get_signal_name(self):
        for exitcode, expected in (
            (-int(signal.SIGINT), 'SIGINT'),
            (-int(signal.SIGSEGV), 'SIGSEGV'),
            (128 + int(signal.SIGABRT), 'SIGABRT'),
            (3221225477, "STATUS_ACCESS_VIOLATION"),
            (0xC00000FD, "STATUS_STACK_OVERFLOW"),
        ):
            self.assertEqual(support.get_signal_name(exitcode), expected,
                             exitcode)

    def test_linked_to_musl(self):
        linked = support.linked_to_musl()
        self.assertIsNotNone(linked)
        if support.is_wasm32:
            self.assertTrue(linked)
        # The value is cached, so make sure it returns the same value again.
        self.assertIs(linked, support.linked_to_musl())
        # The unlike libc, the musl version is a triple.
        if linked:
            self.assertIsInstance(linked, tuple)
            self.assertEqual(3, len(linked))
            for v in linked:
                self.assertIsInstance(v, int)


    # XXX -follows a list of untested API
    # make_legacy_pyc
    # is_resource_enabled
    # requires
    # fcmp
    # umaks
    # findfile
    # check_warnings
    # EnvironmentVarGuard
    # transient_internet
    # run_with_locale
    # bigmemtest
    # precisionbigmemtest
    # bigaddrspacetest
    # requires_resource
    # threading_cleanup
    # reap_threads
    # can_symlink
    # skip_unless_symlink
    # SuppressCrashReport


if __name__ == '__main__':
    unittest.main()
