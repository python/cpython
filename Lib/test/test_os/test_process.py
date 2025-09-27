"""
Tests processes: spawn(), exec(), fork(), waitpid(), etc.
"""

import contextlib
import errno
import os
import platform
import signal
import stat
import subprocess
import sys
import tempfile
import textwrap
import unittest
import uuid
from test import support
from test.support import os_helper
from test.support import warnings_helper
from test.support.os_helper import FakePath
from test.support.script_helper import assert_python_ok
from .utils import requires_sched

try:
    import posix
except ImportError:
    import nt as posix
try:
    import _testcapi
except ImportError:
    _testcapi = None


# Detect whether we're on a Linux system that uses the (now outdated
# and unmaintained) linuxthreads threading library.  There's an issue
# when combining linuxthreads with a failed execv call: see
# http://bugs.python.org/issue4970.
if hasattr(sys, 'thread_info') and sys.thread_info.version:
    USING_LINUXTHREADS = sys.thread_info.version.startswith("linuxthreads")
else:
    USING_LINUXTHREADS = False

def requires_os_func(name):
    return unittest.skipUnless(hasattr(os, name), 'requires os.%s' % name)


def tearDownModule():
    support.reap_children()


@contextlib.contextmanager
def _execvpe_mockup(defpath=None):
    """
    Stubs out execv and execve functions when used as context manager.
    Records exec calls. The mock execv and execve functions always raise an
    exception as they would normally never return.
    """
    # A list of tuples containing (function name, first arg, args)
    # of calls to execv or execve that have been made.
    calls = []

    def mock_execv(name, *args):
        calls.append(('execv', name, args))
        raise RuntimeError("execv called")

    def mock_execve(name, *args):
        calls.append(('execve', name, args))
        raise OSError(errno.ENOTDIR, "execve called")

    try:
        orig_execv = os.execv
        orig_execve = os.execve
        orig_defpath = os.defpath
        os.execv = mock_execv
        os.execve = mock_execve
        if defpath is not None:
            os.defpath = defpath
        yield calls
    finally:
        os.execv = orig_execv
        os.execve = orig_execve
        os.defpath = orig_defpath


@unittest.skipUnless(hasattr(os, 'execv'),
                     "need os.execv()")
class ExecTests(unittest.TestCase):
    @unittest.skipIf(USING_LINUXTHREADS,
                     "avoid triggering a linuxthreads bug: see issue #4970")
    def test_execvpe_with_bad_program(self):
        self.assertRaises(OSError, os.execvpe, 'no such app-',
                          ['no such app-'], None)

    def test_execv_with_bad_arglist(self):
        self.assertRaises(ValueError, os.execv, 'notepad', ())
        self.assertRaises(ValueError, os.execv, 'notepad', [])
        self.assertRaises(ValueError, os.execv, 'notepad', ('',))
        self.assertRaises(ValueError, os.execv, 'notepad', [''])

    def test_execvpe_with_bad_arglist(self):
        self.assertRaises(ValueError, os.execvpe, 'notepad', [], None)
        self.assertRaises(ValueError, os.execvpe, 'notepad', [], {})
        self.assertRaises(ValueError, os.execvpe, 'notepad', [''], {})

    @unittest.skipUnless(hasattr(os, '_execvpe'),
                         "No internal os._execvpe function to test.")
    def _test_internal_execvpe(self, test_type):
        program_path = os.sep + 'absolutepath'
        if test_type is bytes:
            program = b'executable'
            fullpath = os.path.join(os.fsencode(program_path), program)
            native_fullpath = fullpath
            arguments = [b'progname', 'arg1', 'arg2']
        else:
            program = 'executable'
            arguments = ['progname', 'arg1', 'arg2']
            fullpath = os.path.join(program_path, program)
            if os.name != "nt":
                native_fullpath = os.fsencode(fullpath)
            else:
                native_fullpath = fullpath
        env = {'spam': 'beans'}

        # test os._execvpe() with an absolute path
        with _execvpe_mockup() as calls:
            self.assertRaises(RuntimeError,
                os._execvpe, fullpath, arguments)
            self.assertEqual(len(calls), 1)
            self.assertEqual(calls[0], ('execv', fullpath, (arguments,)))

        # test os._execvpe() with a relative path:
        # os.get_exec_path() returns defpath
        with _execvpe_mockup(defpath=program_path) as calls:
            self.assertRaises(OSError,
                os._execvpe, program, arguments, env=env)
            self.assertEqual(len(calls), 1)
            self.assertSequenceEqual(calls[0],
                ('execve', native_fullpath, (arguments, env)))

        # test os._execvpe() with a relative path:
        # os.get_exec_path() reads the 'PATH' variable
        with _execvpe_mockup() as calls:
            env_path = env.copy()
            if test_type is bytes:
                env_path[b'PATH'] = program_path
            else:
                env_path['PATH'] = program_path
            self.assertRaises(OSError,
                os._execvpe, program, arguments, env=env_path)
            self.assertEqual(len(calls), 1)
            self.assertSequenceEqual(calls[0],
                ('execve', native_fullpath, (arguments, env_path)))

    def test_internal_execvpe_str(self):
        self._test_internal_execvpe(str)
        if os.name != "nt":
            self._test_internal_execvpe(bytes)

    def test_execve_invalid_env(self):
        args = [sys.executable, '-c', 'pass']

        # null character in the environment variable name
        newenv = os.environ.copy()
        newenv["FRUIT\0VEGETABLE"] = "cabbage"
        with self.assertRaises(ValueError):
            os.execve(args[0], args, newenv)

        # null character in the environment variable value
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange\0VEGETABLE=cabbage"
        with self.assertRaises(ValueError):
            os.execve(args[0], args, newenv)

        # equal character in the environment variable name
        newenv = os.environ.copy()
        newenv["FRUIT=ORANGE"] = "lemon"
        with self.assertRaises(ValueError):
            os.execve(args[0], args, newenv)

    @unittest.skipUnless(sys.platform == "win32", "Win32-specific test")
    def test_execve_with_empty_path(self):
        # bpo-32890: Check GetLastError() misuse
        try:
            os.execve('', ['arg'], {})
        except OSError as e:
            self.assertTrue(e.winerror is None or e.winerror != 0)
        else:
            self.fail('No OSError raised')


@support.requires_subprocess()
class SpawnTests(unittest.TestCase):
    @staticmethod
    def quote_args(args):
        # On Windows, os.spawn* simply joins arguments with spaces:
        # arguments need to be quoted
        if os.name != 'nt':
            return args
        return [f'"{arg}"' if " " in arg.strip() else arg for arg in args]

    def create_args(self, *, with_env=False, use_bytes=False):
        self.exitcode = 17

        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)

        if not with_env:
            code = 'import sys; sys.exit(%s)' % self.exitcode
        else:
            self.env = dict(os.environ)
            # create an unique key
            self.key = str(uuid.uuid4())
            self.env[self.key] = self.key
            # read the variable from os.environ to check that it exists
            code = ('import sys, os; magic = os.environ[%r]; sys.exit(%s)'
                    % (self.key, self.exitcode))

        with open(filename, "w", encoding="utf-8") as fp:
            fp.write(code)

        program = sys.executable
        args = self.quote_args([program, filename])
        if use_bytes:
            program = os.fsencode(program)
            args = [os.fsencode(a) for a in args]
            self.env = {os.fsencode(k): os.fsencode(v)
                        for k, v in self.env.items()}

        return program, args

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnl')
    def test_spawnl(self):
        program, args = self.create_args()
        exitcode = os.spawnl(os.P_WAIT, program, *args)
        self.assertEqual(exitcode, self.exitcode)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnle')
    def test_spawnle(self):
        program, args = self.create_args(with_env=True)
        exitcode = os.spawnle(os.P_WAIT, program, *args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnlp')
    def test_spawnlp(self):
        program, args = self.create_args()
        exitcode = os.spawnlp(os.P_WAIT, program, *args)
        self.assertEqual(exitcode, self.exitcode)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnlpe')
    def test_spawnlpe(self):
        program, args = self.create_args(with_env=True)
        exitcode = os.spawnlpe(os.P_WAIT, program, *args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnv')
    def test_spawnv(self):
        program, args = self.create_args()
        exitcode = os.spawnv(os.P_WAIT, program, args)
        self.assertEqual(exitcode, self.exitcode)

        # Test for PyUnicode_FSConverter()
        exitcode = os.spawnv(os.P_WAIT, FakePath(program), args)
        self.assertEqual(exitcode, self.exitcode)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnve')
    def test_spawnve(self):
        program, args = self.create_args(with_env=True)
        exitcode = os.spawnve(os.P_WAIT, program, args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnvp')
    def test_spawnvp(self):
        program, args = self.create_args()
        exitcode = os.spawnvp(os.P_WAIT, program, args)
        self.assertEqual(exitcode, self.exitcode)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnvpe')
    def test_spawnvpe(self):
        program, args = self.create_args(with_env=True)
        exitcode = os.spawnvpe(os.P_WAIT, program, args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnv')
    def test_nowait(self):
        program, args = self.create_args()
        pid = os.spawnv(os.P_NOWAIT, program, args)
        support.wait_process(pid, exitcode=self.exitcode)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnve')
    def test_spawnve_bytes(self):
        # Test bytes handling in parse_arglist and parse_envlist (#28114)
        program, args = self.create_args(with_env=True, use_bytes=True)
        exitcode = os.spawnve(os.P_WAIT, program, args, self.env)
        self.assertEqual(exitcode, self.exitcode)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnl')
    def test_spawnl_noargs(self):
        program, __ = self.create_args()
        self.assertRaises(ValueError, os.spawnl, os.P_NOWAIT, program)
        self.assertRaises(ValueError, os.spawnl, os.P_NOWAIT, program, '')

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnle')
    def test_spawnle_noargs(self):
        program, __ = self.create_args()
        self.assertRaises(ValueError, os.spawnle, os.P_NOWAIT, program, {})
        self.assertRaises(ValueError, os.spawnle, os.P_NOWAIT, program, '', {})

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnv')
    def test_spawnv_noargs(self):
        program, __ = self.create_args()
        self.assertRaises(ValueError, os.spawnv, os.P_NOWAIT, program, ())
        self.assertRaises(ValueError, os.spawnv, os.P_NOWAIT, program, [])
        self.assertRaises(ValueError, os.spawnv, os.P_NOWAIT, program, ('',))
        self.assertRaises(ValueError, os.spawnv, os.P_NOWAIT, program, [''])

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnve')
    def test_spawnve_noargs(self):
        program, __ = self.create_args()
        self.assertRaises(ValueError, os.spawnve, os.P_NOWAIT, program, (), {})
        self.assertRaises(ValueError, os.spawnve, os.P_NOWAIT, program, [], {})
        self.assertRaises(ValueError, os.spawnve, os.P_NOWAIT, program, ('',), {})
        self.assertRaises(ValueError, os.spawnve, os.P_NOWAIT, program, [''], {})

    def _test_invalid_env(self, spawn):
        program = sys.executable
        args = self.quote_args([program, '-c', 'pass'])

        # null character in the environment variable name
        newenv = os.environ.copy()
        newenv["FRUIT\0VEGETABLE"] = "cabbage"
        try:
            exitcode = spawn(os.P_WAIT, program, args, newenv)
        except ValueError:
            pass
        else:
            self.assertEqual(exitcode, 127)

        # null character in the environment variable value
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange\0VEGETABLE=cabbage"
        try:
            exitcode = spawn(os.P_WAIT, program, args, newenv)
        except ValueError:
            pass
        else:
            self.assertEqual(exitcode, 127)

        # equal character in the environment variable name
        newenv = os.environ.copy()
        newenv["FRUIT=ORANGE"] = "lemon"
        try:
            exitcode = spawn(os.P_WAIT, program, args, newenv)
        except ValueError:
            pass
        else:
            self.assertEqual(exitcode, 127)

        # equal character in the environment variable value
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)
        with open(filename, "w", encoding="utf-8") as fp:
            fp.write('import sys, os\n'
                     'if os.getenv("FRUIT") != "orange=lemon":\n'
                     '    raise AssertionError')

        args = self.quote_args([program, filename])
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange=lemon"
        exitcode = spawn(os.P_WAIT, program, args, newenv)
        self.assertEqual(exitcode, 0)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnve')
    def test_spawnve_invalid_env(self):
        self._test_invalid_env(os.spawnve)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @requires_os_func('spawnvpe')
    def test_spawnvpe_invalid_env(self):
        self._test_invalid_env(os.spawnvpe)


class PosixTester(unittest.TestCase):

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @unittest.skipUnless(getattr(os, 'execve', None) in os.supports_fd, "test needs execve() to support the fd parameter")
    @support.requires_fork()
    def test_fexecve(self):
        fp = os.open(sys.executable, os.O_RDONLY)
        try:
            pid = os.fork()
            if pid == 0:
                os.chdir(os.path.split(sys.executable)[0])
                posix.execve(fp, [sys.executable, '-c', 'pass'], os.environ)
            else:
                support.wait_process(pid, exitcode=0)
        finally:
            os.close(fp)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @unittest.skipUnless(hasattr(posix, 'waitid'), "test needs posix.waitid()")
    @support.requires_fork()
    def test_waitid(self):
        pid = os.fork()
        if pid == 0:
            os.chdir(os.path.split(sys.executable)[0])
            posix.execve(sys.executable, [sys.executable, '-c', 'pass'], os.environ)
        else:
            res = posix.waitid(posix.P_PID, pid, posix.WEXITED)
            self.assertEqual(pid, res.si_pid)

    @support.requires_fork()
    def test_register_at_fork(self):
        with self.assertRaises(TypeError, msg="Positional args not allowed"):
            os.register_at_fork(lambda: None)
        with self.assertRaises(TypeError, msg="Args must be callable"):
            os.register_at_fork(before=2)
        with self.assertRaises(TypeError, msg="Args must be callable"):
            os.register_at_fork(after_in_child="three")
        with self.assertRaises(TypeError, msg="Args must be callable"):
            os.register_at_fork(after_in_parent=b"Five")
        with self.assertRaises(TypeError, msg="Args must not be None"):
            os.register_at_fork(before=None)
        with self.assertRaises(TypeError, msg="Args must not be None"):
            os.register_at_fork(after_in_child=None)
        with self.assertRaises(TypeError, msg="Args must not be None"):
            os.register_at_fork(after_in_parent=None)
        with self.assertRaises(TypeError, msg="Invalid arg was allowed"):
            # Ensure a combination of valid and invalid is an error.
            os.register_at_fork(before=None, after_in_parent=lambda: 3)
        with self.assertRaises(TypeError, msg="At least one argument is required"):
            # when no arg is passed
            os.register_at_fork()
        with self.assertRaises(TypeError, msg="Invalid arg was allowed"):
            # Ensure a combination of valid and invalid is an error.
            os.register_at_fork(before=lambda: None, after_in_child='')
        # We test actual registrations in their own process so as not to
        # pollute this one.  There is no way to unregister for cleanup.
        code = """if 1:
            import os

            r, w = os.pipe()
            fin_r, fin_w = os.pipe()

            os.register_at_fork(before=lambda: os.write(w, b'A'))
            os.register_at_fork(after_in_parent=lambda: os.write(w, b'C'))
            os.register_at_fork(after_in_child=lambda: os.write(w, b'E'))
            os.register_at_fork(before=lambda: os.write(w, b'B'),
                                after_in_parent=lambda: os.write(w, b'D'),
                                after_in_child=lambda: os.write(w, b'F'))

            pid = os.fork()
            if pid == 0:
                # At this point, after-forkers have already been executed
                os.close(w)
                # Wait for parent to tell us to exit
                os.read(fin_r, 1)
                os._exit(0)
            else:
                try:
                    os.close(w)
                    with open(r, "rb") as f:
                        data = f.read()
                        assert len(data) == 6, data
                        # Check before-fork callbacks
                        assert data[:2] == b'BA', data
                        # Check after-fork callbacks
                        assert sorted(data[2:]) == list(b'CDEF'), data
                        assert data.index(b'C') < data.index(b'D'), data
                        assert data.index(b'E') < data.index(b'F'), data
                finally:
                    os.write(fin_w, b'!')
            """
        assert_python_ok('-c', code)



class _PosixSpawnMixin:
    # Program which does nothing and exits with status 0 (success)
    NOOP_PROGRAM = (sys.executable, '-I', '-S', '-c', 'pass')
    spawn_func = None

    def python_args(self, *args):
        # Disable site module to avoid side effects. For example,
        # on Fedora 28, if the HOME environment variable is not set,
        # site._getuserbase() calls pwd.getpwuid() which opens
        # /var/lib/sss/mc/passwd but then leaves the file open which makes
        # test_close_file() to fail.
        return (sys.executable, '-I', '-S', *args)

    def test_returns_pid(self):
        pidfile = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, pidfile)
        script = f"""if 1:
            import os
            with open({pidfile!r}, "w") as pidfile:
                pidfile.write(str(os.getpid()))
            """
        args = self.python_args('-c', script)
        pid = self.spawn_func(args[0], args, os.environ)
        support.wait_process(pid, exitcode=0)
        with open(pidfile, encoding="utf-8") as f:
            self.assertEqual(f.read(), str(pid))

    def test_no_such_executable(self):
        no_such_executable = 'no_such_executable'
        try:
            pid = self.spawn_func(no_such_executable,
                                  [no_such_executable],
                                  os.environ)
        # bpo-35794: PermissionError can be raised if there are
        # directories in the $PATH that are not accessible.
        except (FileNotFoundError, PermissionError) as exc:
            self.assertEqual(exc.filename, no_such_executable)
        else:
            pid2, status = os.waitpid(pid, 0)
            self.assertEqual(pid2, pid)
            self.assertNotEqual(status, 0)

    def test_specify_environment(self):
        envfile = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, envfile)
        script = f"""if 1:
            import os
            with open({envfile!r}, "w", encoding="utf-8") as envfile:
                envfile.write(os.environ['foo'])
        """
        args = self.python_args('-c', script)
        pid = self.spawn_func(args[0], args,
                              {**os.environ, 'foo': 'bar'})
        support.wait_process(pid, exitcode=0)
        with open(envfile, encoding="utf-8") as f:
            self.assertEqual(f.read(), 'bar')

    def test_none_file_actions(self):
        pid = self.spawn_func(
            self.NOOP_PROGRAM[0],
            self.NOOP_PROGRAM,
            os.environ,
            file_actions=None
        )
        support.wait_process(pid, exitcode=0)

    def test_empty_file_actions(self):
        pid = self.spawn_func(
            self.NOOP_PROGRAM[0],
            self.NOOP_PROGRAM,
            os.environ,
            file_actions=[]
        )
        support.wait_process(pid, exitcode=0)

    def test_resetids_explicit_default(self):
        pid = self.spawn_func(
            sys.executable,
            [sys.executable, '-c', 'pass'],
            os.environ,
            resetids=False
        )
        support.wait_process(pid, exitcode=0)

    def test_resetids(self):
        pid = self.spawn_func(
            sys.executable,
            [sys.executable, '-c', 'pass'],
            os.environ,
            resetids=True
        )
        support.wait_process(pid, exitcode=0)

    def test_setpgroup(self):
        pid = self.spawn_func(
            sys.executable,
            [sys.executable, '-c', 'pass'],
            os.environ,
            setpgroup=os.getpgrp()
        )
        support.wait_process(pid, exitcode=0)

    def test_setpgroup_wrong_type(self):
        with self.assertRaises(TypeError):
            self.spawn_func(sys.executable,
                            [sys.executable, "-c", "pass"],
                            os.environ, setpgroup="023")

    @unittest.skipUnless(hasattr(signal, 'pthread_sigmask'),
                           'need signal.pthread_sigmask()')
    def test_setsigmask(self):
        code = textwrap.dedent("""\
            import signal
            signal.raise_signal(signal.SIGUSR1)""")

        pid = self.spawn_func(
            sys.executable,
            [sys.executable, '-c', code],
            os.environ,
            setsigmask=[signal.SIGUSR1]
        )
        support.wait_process(pid, exitcode=0)

    def test_setsigmask_wrong_type(self):
        with self.assertRaises(TypeError):
            self.spawn_func(sys.executable,
                            [sys.executable, "-c", "pass"],
                            os.environ, setsigmask=34)
        with self.assertRaises(TypeError):
            self.spawn_func(sys.executable,
                            [sys.executable, "-c", "pass"],
                            os.environ, setsigmask=["j"])
        with self.assertRaises(ValueError):
            self.spawn_func(sys.executable,
                            [sys.executable, "-c", "pass"],
                            os.environ, setsigmask=[signal.NSIG,
                                                    signal.NSIG+1])

    def test_setsid(self):
        rfd, wfd = os.pipe()
        self.addCleanup(os.close, rfd)
        try:
            os.set_inheritable(wfd, True)

            code = textwrap.dedent(f"""
                import os
                fd = {wfd}
                sid = os.getsid(0)
                os.write(fd, str(sid).encode())
            """)

            try:
                pid = self.spawn_func(sys.executable,
                                      [sys.executable, "-c", code],
                                      os.environ, setsid=True)
            except NotImplementedError as exc:
                self.skipTest(f"setsid is not supported: {exc!r}")
            except PermissionError as exc:
                self.skipTest(f"setsid failed with: {exc!r}")
        finally:
            os.close(wfd)

        support.wait_process(pid, exitcode=0)

        output = os.read(rfd, 100)
        child_sid = int(output)
        parent_sid = os.getsid(os.getpid())
        self.assertNotEqual(parent_sid, child_sid)

    @unittest.skipUnless(hasattr(signal, 'pthread_sigmask'),
                         'need signal.pthread_sigmask()')
    def test_setsigdef(self):
        original_handler = signal.signal(signal.SIGUSR1, signal.SIG_IGN)
        code = textwrap.dedent("""\
            import signal
            signal.raise_signal(signal.SIGUSR1)""")
        try:
            pid = self.spawn_func(
                sys.executable,
                [sys.executable, '-c', code],
                os.environ,
                setsigdef=[signal.SIGUSR1]
            )
        finally:
            signal.signal(signal.SIGUSR1, original_handler)

        support.wait_process(pid, exitcode=-signal.SIGUSR1)

    def test_setsigdef_wrong_type(self):
        with self.assertRaises(TypeError):
            self.spawn_func(sys.executable,
                            [sys.executable, "-c", "pass"],
                            os.environ, setsigdef=34)
        with self.assertRaises(TypeError):
            self.spawn_func(sys.executable,
                            [sys.executable, "-c", "pass"],
                            os.environ, setsigdef=["j"])
        with self.assertRaises(ValueError):
            self.spawn_func(sys.executable,
                            [sys.executable, "-c", "pass"],
                            os.environ, setsigdef=[signal.NSIG, signal.NSIG+1])

    @requires_sched
    @unittest.skipIf(sys.platform.startswith(('freebsd', 'netbsd')),
                     "bpo-34685: test can fail on BSD")
    def test_setscheduler_only_param(self):
        policy = os.sched_getscheduler(0)
        priority = os.sched_get_priority_min(policy)
        code = textwrap.dedent(f"""\
            import os, sys
            if os.sched_getscheduler(0) != {policy}:
                sys.exit(101)
            if os.sched_getparam(0).sched_priority != {priority}:
                sys.exit(102)""")
        pid = self.spawn_func(
            sys.executable,
            [sys.executable, '-c', code],
            os.environ,
            scheduler=(None, os.sched_param(priority))
        )
        support.wait_process(pid, exitcode=0)

    @requires_sched
    @unittest.skipIf(sys.platform.startswith(('freebsd', 'netbsd')),
                     "bpo-34685: test can fail on BSD")
    @unittest.skipIf(platform.libc_ver()[0] == 'glibc' and
                     os.sched_getscheduler(0) in [
                        os.SCHED_BATCH,
                        os.SCHED_IDLE,
                        os.SCHED_DEADLINE],
                     "Skip test due to glibc posix_spawn policy")
    def test_setscheduler_with_policy(self):
        policy = os.sched_getscheduler(0)
        priority = os.sched_get_priority_min(policy)
        code = textwrap.dedent(f"""\
            import os, sys
            if os.sched_getscheduler(0) != {policy}:
                sys.exit(101)
            if os.sched_getparam(0).sched_priority != {priority}:
                sys.exit(102)""")
        pid = self.spawn_func(
            sys.executable,
            [sys.executable, '-c', code],
            os.environ,
            scheduler=(policy, os.sched_param(priority))
        )
        support.wait_process(pid, exitcode=0)

    def test_multiple_file_actions(self):
        file_actions = [
            (os.POSIX_SPAWN_OPEN, 3, os.path.realpath(__file__), os.O_RDONLY, 0),
            (os.POSIX_SPAWN_CLOSE, 0),
            (os.POSIX_SPAWN_DUP2, 1, 4),
        ]
        pid = self.spawn_func(self.NOOP_PROGRAM[0],
                              self.NOOP_PROGRAM,
                              os.environ,
                              file_actions=file_actions)
        support.wait_process(pid, exitcode=0)

    def test_bad_file_actions(self):
        args = self.NOOP_PROGRAM
        with self.assertRaises(TypeError):
            self.spawn_func(args[0], args, os.environ,
                            file_actions=[None])
        with self.assertRaises(TypeError):
            self.spawn_func(args[0], args, os.environ,
                            file_actions=[()])
        with self.assertRaises(TypeError):
            self.spawn_func(args[0], args, os.environ,
                            file_actions=[(None,)])
        with self.assertRaises(TypeError):
            self.spawn_func(args[0], args, os.environ,
                            file_actions=[(12345,)])
        with self.assertRaises(TypeError):
            self.spawn_func(args[0], args, os.environ,
                            file_actions=[(os.POSIX_SPAWN_CLOSE,)])
        with self.assertRaises(TypeError):
            self.spawn_func(args[0], args, os.environ,
                            file_actions=[(os.POSIX_SPAWN_CLOSE, 1, 2)])
        with self.assertRaises(TypeError):
            self.spawn_func(args[0], args, os.environ,
                            file_actions=[(os.POSIX_SPAWN_CLOSE, None)])
        with self.assertRaises(ValueError):
            self.spawn_func(args[0], args, os.environ,
                            file_actions=[(os.POSIX_SPAWN_OPEN,
                                           3, __file__ + '\0',
                                           os.O_RDONLY, 0)])

    def test_open_file(self):
        outfile = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, outfile)
        script = """if 1:
            import sys
            sys.stdout.write("hello")
            """
        file_actions = [
            (os.POSIX_SPAWN_OPEN, 1, outfile,
                os.O_WRONLY | os.O_CREAT | os.O_TRUNC,
                stat.S_IRUSR | stat.S_IWUSR),
        ]
        args = self.python_args('-c', script)
        pid = self.spawn_func(args[0], args, os.environ,
                              file_actions=file_actions)

        support.wait_process(pid, exitcode=0)
        with open(outfile, encoding="utf-8") as f:
            self.assertEqual(f.read(), 'hello')

    def test_close_file(self):
        closefile = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, closefile)
        script = f"""if 1:
            import os
            try:
                os.fstat(0)
            except OSError as e:
                with open({closefile!r}, 'w', encoding='utf-8') as closefile:
                    closefile.write('is closed %d' % e.errno)
            """
        args = self.python_args('-c', script)
        pid = self.spawn_func(args[0], args, os.environ,
                              file_actions=[(os.POSIX_SPAWN_CLOSE, 0)])

        support.wait_process(pid, exitcode=0)
        with open(closefile, encoding="utf-8") as f:
            self.assertEqual(f.read(), 'is closed %d' % errno.EBADF)

    def test_dup2(self):
        dupfile = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, dupfile)
        script = """if 1:
            import sys
            sys.stdout.write("hello")
            """
        with open(dupfile, "wb") as childfile:
            file_actions = [
                (os.POSIX_SPAWN_DUP2, childfile.fileno(), 1),
            ]
            args = self.python_args('-c', script)
            pid = self.spawn_func(args[0], args, os.environ,
                                  file_actions=file_actions)
            support.wait_process(pid, exitcode=0)
        with open(dupfile, encoding="utf-8") as f:
            self.assertEqual(f.read(), 'hello')


@unittest.skipUnless(hasattr(os, 'posix_spawn'), "test needs os.posix_spawn")
@support.requires_subprocess()
class TestPosixSpawn(unittest.TestCase, _PosixSpawnMixin):
    spawn_func = getattr(os, 'posix_spawn', None)


@unittest.skipUnless(hasattr(os, 'posix_spawnp'), "test needs os.posix_spawnp")
@support.requires_subprocess()
class TestPosixSpawnP(unittest.TestCase, _PosixSpawnMixin):
    spawn_func = getattr(os, 'posix_spawnp', None)

    @os_helper.skip_unless_symlink
    def test_posix_spawnp(self):
        # Use a symlink to create a program in its own temporary directory
        temp_dir = tempfile.mkdtemp()
        self.addCleanup(os_helper.rmtree, temp_dir)

        program = 'posix_spawnp_test_program.exe'
        program_fullpath = os.path.join(temp_dir, program)
        os.symlink(sys.executable, program_fullpath)

        try:
            path = os.pathsep.join((temp_dir, os.environ['PATH']))
        except KeyError:
            path = temp_dir   # PATH is not set

        spawn_args = (program, '-I', '-S', '-c', 'pass')
        code = textwrap.dedent("""
            import os
            from test import support

            args = %a
            pid = os.posix_spawnp(args[0], args, os.environ)

            support.wait_process(pid, exitcode=0)
        """ % (spawn_args,))

        # Use a subprocess to test os.posix_spawnp() with a modified PATH
        # environment variable: posix_spawnp() uses the current environment
        # to locate the program, not its environment argument.
        args = ('-c', code)
        assert_python_ok(*args, PATH=path)


@support.requires_fork()
class ForkTests(unittest.TestCase):
    def test_fork(self):
        # bpo-42540: ensure os.fork() with non-default memory allocator does
        # not crash on exit.
        code = """if 1:
            import os
            from test import support
            pid = os.fork()
            if pid != 0:
                support.wait_process(pid, exitcode=0)
        """
        assert_python_ok("-c", code)
        if support.Py_GIL_DISABLED:
            assert_python_ok("-c", code, PYTHONMALLOC="mimalloc_debug")
        else:
            assert_python_ok("-c", code, PYTHONMALLOC="malloc_debug")

    @unittest.skipUnless(sys.platform in ("linux", "android", "darwin"),
                         "Only Linux and macOS detect this today.")
    @unittest.skipIf(_testcapi is None, "requires _testcapi")
    def test_fork_warns_when_non_python_thread_exists(self):
        code = """if 1:
            import os, threading, warnings
            from _testcapi import _spawn_pthread_waiter, _end_spawned_pthread
            _spawn_pthread_waiter()
            try:
                with warnings.catch_warnings(record=True) as ws:
                    warnings.filterwarnings(
                            "always", category=DeprecationWarning)
                    if os.fork() == 0:
                        assert not ws, f"unexpected warnings in child: {ws}"
                        os._exit(0)  # child
                    else:
                        assert ws[0].category == DeprecationWarning, ws[0]
                        assert 'fork' in str(ws[0].message), ws[0]
                        # Waiting allows an error in the child to hit stderr.
                        exitcode = os.wait()[1]
                        assert exitcode == 0, f"child exited {exitcode}"
                assert threading.active_count() == 1, threading.enumerate()
            finally:
                _end_spawned_pthread()
        """
        _, out, err = assert_python_ok("-c", code, PYTHONOPTIMIZE='0')
        self.assertEqual(err.decode("utf-8"), "")
        self.assertEqual(out.decode("utf-8"), "")

    def test_fork_at_finalization(self):
        code = """if 1:
            import atexit
            import os

            class AtFinalization:
                def __del__(self):
                    print("OK")
                    pid = os.fork()
                    if pid != 0:
                        print("shouldn't be printed")
            at_finalization = AtFinalization()
        """
        _, out, err = assert_python_ok("-c", code)
        self.assertEqual(b"OK\n", out)
        self.assertIn(b"can't fork at interpreter shutdown", err)


@support.requires_subprocess()
class PidTests(unittest.TestCase):
    @unittest.skipUnless(hasattr(os, 'getppid'), "test needs os.getppid")
    def test_getppid(self):
        p = subprocess.Popen([sys._base_executable, '-c',
                              'import os; print(os.getppid())'],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        stdout, error = p.communicate()
        # We are the parent of our subprocess
        self.assertEqual(error, b'')
        self.assertEqual(int(stdout), os.getpid())

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def check_waitpid(self, code, exitcode, callback=None):
        if sys.platform == 'win32':
            # On Windows, os.spawnv() simply joins arguments with spaces:
            # arguments need to be quoted
            args = [f'"{sys.executable}"', '-c', f'"{code}"']
        else:
            args = [sys.executable, '-c', code]
        pid = os.spawnv(os.P_NOWAIT, sys.executable, args)

        if callback is not None:
            callback(pid)

        # don't use support.wait_process() to test directly os.waitpid()
        # and os.waitstatus_to_exitcode()
        pid2, status = os.waitpid(pid, 0)
        self.assertEqual(os.waitstatus_to_exitcode(status), exitcode)
        self.assertEqual(pid2, pid)

    def test_waitpid(self):
        self.check_waitpid(code='pass', exitcode=0)

    def test_waitstatus_to_exitcode(self):
        exitcode = 23
        code = f'import sys; sys.exit({exitcode})'
        self.check_waitpid(code, exitcode=exitcode)

        with self.assertRaises(TypeError):
            os.waitstatus_to_exitcode(0.0)

    @unittest.skipUnless(sys.platform == 'win32', 'win32-specific test')
    def test_waitstatus_to_exitcode_windows(self):
        max_exitcode = 2 ** 32 - 1
        for exitcode in (0, 1, 5, max_exitcode):
            self.assertEqual(os.waitstatus_to_exitcode(exitcode << 8),
                             exitcode)

        # invalid values
        with self.assertRaises(ValueError):
            os.waitstatus_to_exitcode((max_exitcode + 1) << 8)
        with self.assertRaises(OverflowError):
            os.waitstatus_to_exitcode(-1)

    # Skip the test on Windows
    @unittest.skipUnless(hasattr(signal, 'SIGKILL'), 'need signal.SIGKILL')
    def test_waitstatus_to_exitcode_kill(self):
        code = f'import time; time.sleep({support.LONG_TIMEOUT})'
        signum = signal.SIGKILL

        def kill_process(pid):
            os.kill(pid, signum)

        self.check_waitpid(code, exitcode=-signum, callback=kill_process)


class TimesTests(unittest.TestCase):
    def test_times(self):
        times = os.times()
        self.assertIsInstance(times, os.times_result)

        for field in ('user', 'system', 'children_user', 'children_system',
                      'elapsed'):
            value = getattr(times, field)
            self.assertIsInstance(value, float)

        if os.name == 'nt':
            self.assertEqual(times.children_user, 0)
            self.assertEqual(times.children_system, 0)
            self.assertEqual(times.elapsed, 0)


if __name__ == '__main__':
    unittest.main()
