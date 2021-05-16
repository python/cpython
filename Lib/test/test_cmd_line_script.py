# tests command line execution of scripts

import contextlib
import importlib
import importlib.machinery
import zipimport
import unittest
import sys
import os
import os.path
import py_compile
import subprocess
import io

import textwrap
from test import support
from test.support import import_helper
from test.support import os_helper
from test.support.script_helper import (
    make_pkg, make_script, make_zip_pkg, make_zip_script,
    assert_python_ok, assert_python_failure, spawn_python, kill_python)

verbose = support.verbose

example_args = ['test1', 'test2', 'test3']

test_source = """\
# Script may be run with optimisation enabled, so don't rely on assert
# statements being executed
def assertEqual(lhs, rhs):
    if lhs != rhs:
        raise AssertionError('%r != %r' % (lhs, rhs))
def assertIdentical(lhs, rhs):
    if lhs is not rhs:
        raise AssertionError('%r is not %r' % (lhs, rhs))
# Check basic code execution
result = ['Top level assignment']
def f():
    result.append('Lower level reference')
f()
assertEqual(result, ['Top level assignment', 'Lower level reference'])
# Check population of magic variables
assertEqual(__name__, '__main__')
from importlib.machinery import BuiltinImporter
_loader = __loader__ if __loader__ is BuiltinImporter else type(__loader__)
print('__loader__==%a' % _loader)
print('__file__==%a' % __file__)
print('__cached__==%a' % __cached__)
print('__package__==%r' % __package__)
# Check PEP 451 details
import os.path
if __package__ is not None:
    print('__main__ was located through the import system')
    assertIdentical(__spec__.loader, __loader__)
    expected_spec_name = os.path.splitext(os.path.basename(__file__))[0]
    if __package__:
        expected_spec_name = __package__ + "." + expected_spec_name
    assertEqual(__spec__.name, expected_spec_name)
    assertEqual(__spec__.parent, __package__)
    assertIdentical(__spec__.submodule_search_locations, None)
    assertEqual(__spec__.origin, __file__)
    if __spec__.cached is not None:
        assertEqual(__spec__.cached, __cached__)
# Check the sys module
import sys
assertIdentical(globals(), sys.modules[__name__].__dict__)
if __spec__ is not None:
    # XXX: We're not currently making __main__ available under its real name
    pass # assertIdentical(globals(), sys.modules[__spec__.name].__dict__)
from test import test_cmd_line_script
example_args_list = test_cmd_line_script.example_args
assertEqual(sys.argv[1:], example_args_list)
print('sys.argv[0]==%a' % sys.argv[0])
print('sys.path[0]==%a' % sys.path[0])
# Check the working directory
import os
print('cwd==%a' % os.getcwd())
"""

def _make_test_script(script_dir, script_basename, source=test_source):
    to_return = make_script(script_dir, script_basename, source)
    importlib.invalidate_caches()
    return to_return

def _make_test_zip_pkg(zip_dir, zip_basename, pkg_name, script_basename,
                       source=test_source, depth=1):
    to_return = make_zip_pkg(zip_dir, zip_basename, pkg_name, script_basename,
                             source, depth)
    importlib.invalidate_caches()
    return to_return

class CmdLineTest(unittest.TestCase):
    def _check_output(self, script_name, exit_code, data,
                             expected_file, expected_argv0,
                             expected_path0, expected_package,
                             expected_loader, expected_cwd=None):
        if verbose > 1:
            print("Output from test script %r:" % script_name)
            print(repr(data))
        self.assertEqual(exit_code, 0)
        printed_loader = '__loader__==%a' % expected_loader
        printed_file = '__file__==%a' % expected_file
        printed_package = '__package__==%r' % expected_package
        printed_argv0 = 'sys.argv[0]==%a' % expected_argv0
        printed_path0 = 'sys.path[0]==%a' % expected_path0
        if expected_cwd is None:
            expected_cwd = os.getcwd()
        printed_cwd = 'cwd==%a' % expected_cwd
        if verbose > 1:
            print('Expected output:')
            print(printed_file)
            print(printed_package)
            print(printed_argv0)
            print(printed_cwd)
        self.assertIn(printed_loader.encode('utf-8'), data)
        self.assertIn(printed_file.encode('utf-8'), data)
        self.assertIn(printed_package.encode('utf-8'), data)
        self.assertIn(printed_argv0.encode('utf-8'), data)
        self.assertIn(printed_path0.encode('utf-8'), data)
        self.assertIn(printed_cwd.encode('utf-8'), data)

    def _check_script(self, script_exec_args, expected_file,
                            expected_argv0, expected_path0,
                            expected_package, expected_loader,
                            *cmd_line_switches, cwd=None, **env_vars):
        if isinstance(script_exec_args, str):
            script_exec_args = [script_exec_args]
        run_args = [*support.optim_args_from_interpreter_flags(),
                    *cmd_line_switches, *script_exec_args, *example_args]
        rc, out, err = assert_python_ok(
            *run_args, __isolated=False, __cwd=cwd, **env_vars
        )
        self._check_output(script_exec_args, rc, out + err, expected_file,
                           expected_argv0, expected_path0,
                           expected_package, expected_loader, cwd)

    def _check_import_error(self, script_exec_args, expected_msg,
                            *cmd_line_switches, cwd=None, **env_vars):
        if isinstance(script_exec_args, str):
            script_exec_args = (script_exec_args,)
        else:
            script_exec_args = tuple(script_exec_args)
        run_args = cmd_line_switches + script_exec_args
        rc, out, err = assert_python_failure(
            *run_args, __isolated=False, __cwd=cwd, **env_vars
        )
        if verbose > 1:
            print(f'Output from test script {script_exec_args!r:}')
            print(repr(err))
            print('Expected output: %r' % expected_msg)
        self.assertIn(expected_msg.encode('utf-8'), err)

    def test_dash_c_loader(self):
        rc, out, err = assert_python_ok("-c", "print(__loader__)")
        expected = repr(importlib.machinery.BuiltinImporter).encode("utf-8")
        self.assertIn(expected, out)

    def test_stdin_loader(self):
        # Unfortunately, there's no way to automatically test the fully
        # interactive REPL, since that code path only gets executed when
        # stdin is an interactive tty.
        p = spawn_python()
        try:
            p.stdin.write(b"print(__loader__)\n")
            p.stdin.flush()
        finally:
            out = kill_python(p)
        expected = repr(importlib.machinery.BuiltinImporter).encode("utf-8")
        self.assertIn(expected, out)

    @contextlib.contextmanager
    def interactive_python(self, separate_stderr=False):
        if separate_stderr:
            p = spawn_python('-i', stderr=subprocess.PIPE)
            stderr = p.stderr
        else:
            p = spawn_python('-i', stderr=subprocess.STDOUT)
            stderr = p.stdout
        try:
            # Drain stderr until prompt
            while True:
                data = stderr.read(4)
                if data == b">>> ":
                    break
                stderr.readline()
            yield p
        finally:
            kill_python(p)
            stderr.close()

    def check_repl_stdout_flush(self, separate_stderr=False):
        with self.interactive_python(separate_stderr) as p:
            p.stdin.write(b"print('foo')\n")
            p.stdin.flush()
            self.assertEqual(b'foo', p.stdout.readline().strip())

    def check_repl_stderr_flush(self, separate_stderr=False):
        with self.interactive_python(separate_stderr) as p:
            p.stdin.write(b"1/0\n")
            p.stdin.flush()
            stderr = p.stderr if separate_stderr else p.stdout
            self.assertIn(b'Traceback ', stderr.readline())
            self.assertIn(b'File "<stdin>"', stderr.readline())
            self.assertIn(b'ZeroDivisionError', stderr.readline())

    def test_repl_stdout_flush(self):
        self.check_repl_stdout_flush()

    def test_repl_stdout_flush_separate_stderr(self):
        self.check_repl_stdout_flush(True)

    def test_repl_stderr_flush(self):
        self.check_repl_stderr_flush()

    def test_repl_stderr_flush_separate_stderr(self):
        self.check_repl_stderr_flush(True)

    def test_basic_script(self):
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script')
            self._check_script(script_name, script_name, script_name,
                               script_dir, None,
                               importlib.machinery.SourceFileLoader,
                               expected_cwd=script_dir)

    def test_script_abspath(self):
        # pass the script using the relative path, expect the absolute path
        # in __file__
        with os_helper.temp_cwd() as script_dir:
            self.assertTrue(os.path.isabs(script_dir), script_dir)

            script_name = _make_test_script(script_dir, 'script')
            relative_name = os.path.basename(script_name)
            self._check_script(relative_name, script_name, relative_name,
                               script_dir, None,
                               importlib.machinery.SourceFileLoader)

    def test_script_compiled(self):
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script')
            py_compile.compile(script_name, doraise=True)
            os.remove(script_name)
            pyc_file = import_helper.make_legacy_pyc(script_name)
            self._check_script(pyc_file, pyc_file,
                               pyc_file, script_dir, None,
                               importlib.machinery.SourcelessFileLoader)

    def test_directory(self):
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            self._check_script(script_dir, script_name, script_dir,
                               script_dir, '',
                               importlib.machinery.SourceFileLoader)

    def test_directory_compiled(self):
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            py_compile.compile(script_name, doraise=True)
            os.remove(script_name)
            pyc_file = import_helper.make_legacy_pyc(script_name)
            self._check_script(script_dir, pyc_file, script_dir,
                               script_dir, '',
                               importlib.machinery.SourcelessFileLoader)

    def test_directory_error(self):
        with os_helper.temp_dir() as script_dir:
            msg = "can't find '__main__' module in %r" % script_dir
            self._check_import_error(script_dir, msg)

    def test_zipfile(self):
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            zip_name, run_name = make_zip_script(script_dir, 'test_zip', script_name)
            self._check_script(zip_name, run_name, zip_name, zip_name, '',
                               zipimport.zipimporter)

    def test_zipfile_compiled_timestamp(self):
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            compiled_name = py_compile.compile(
                script_name, doraise=True,
                invalidation_mode=py_compile.PycInvalidationMode.TIMESTAMP)
            zip_name, run_name = make_zip_script(script_dir, 'test_zip', compiled_name)
            self._check_script(zip_name, run_name, zip_name, zip_name, '',
                               zipimport.zipimporter)

    def test_zipfile_compiled_checked_hash(self):
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            compiled_name = py_compile.compile(
                script_name, doraise=True,
                invalidation_mode=py_compile.PycInvalidationMode.CHECKED_HASH)
            zip_name, run_name = make_zip_script(script_dir, 'test_zip', compiled_name)
            self._check_script(zip_name, run_name, zip_name, zip_name, '',
                               zipimport.zipimporter)

    def test_zipfile_compiled_unchecked_hash(self):
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            compiled_name = py_compile.compile(
                script_name, doraise=True,
                invalidation_mode=py_compile.PycInvalidationMode.UNCHECKED_HASH)
            zip_name, run_name = make_zip_script(script_dir, 'test_zip', compiled_name)
            self._check_script(zip_name, run_name, zip_name, zip_name, '',
                               zipimport.zipimporter)

    def test_zipfile_error(self):
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'not_main')
            zip_name, run_name = make_zip_script(script_dir, 'test_zip', script_name)
            msg = "can't find '__main__' module in %r" % zip_name
            self._check_import_error(zip_name, msg)

    def test_module_in_package(self):
        with os_helper.temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir)
            script_name = _make_test_script(pkg_dir, 'script')
            self._check_script(["-m", "test_pkg.script"], script_name, script_name,
                               script_dir, 'test_pkg',
                               importlib.machinery.SourceFileLoader,
                               cwd=script_dir)

    def test_module_in_package_in_zipfile(self):
        with os_helper.temp_dir() as script_dir:
            zip_name, run_name = _make_test_zip_pkg(script_dir, 'test_zip', 'test_pkg', 'script')
            self._check_script(["-m", "test_pkg.script"], run_name, run_name,
                               script_dir, 'test_pkg', zipimport.zipimporter,
                               PYTHONPATH=zip_name, cwd=script_dir)

    def test_module_in_subpackage_in_zipfile(self):
        with os_helper.temp_dir() as script_dir:
            zip_name, run_name = _make_test_zip_pkg(script_dir, 'test_zip', 'test_pkg', 'script', depth=2)
            self._check_script(["-m", "test_pkg.test_pkg.script"], run_name, run_name,
                               script_dir, 'test_pkg.test_pkg',
                               zipimport.zipimporter,
                               PYTHONPATH=zip_name, cwd=script_dir)

    def test_package(self):
        with os_helper.temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir)
            script_name = _make_test_script(pkg_dir, '__main__')
            self._check_script(["-m", "test_pkg"], script_name,
                               script_name, script_dir, 'test_pkg',
                               importlib.machinery.SourceFileLoader,
                               cwd=script_dir)

    def test_package_compiled(self):
        with os_helper.temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir)
            script_name = _make_test_script(pkg_dir, '__main__')
            compiled_name = py_compile.compile(script_name, doraise=True)
            os.remove(script_name)
            pyc_file = import_helper.make_legacy_pyc(script_name)
            self._check_script(["-m", "test_pkg"], pyc_file,
                               pyc_file, script_dir, 'test_pkg',
                               importlib.machinery.SourcelessFileLoader,
                               cwd=script_dir)

    def test_package_error(self):
        with os_helper.temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir)
            msg = ("'test_pkg' is a package and cannot "
                   "be directly executed")
            self._check_import_error(["-m", "test_pkg"], msg, cwd=script_dir)

    def test_package_recursion(self):
        with os_helper.temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir)
            main_dir = os.path.join(pkg_dir, '__main__')
            make_pkg(main_dir)
            msg = ("Cannot use package as __main__ module; "
                   "'test_pkg' is a package and cannot "
                   "be directly executed")
            self._check_import_error(["-m", "test_pkg"], msg, cwd=script_dir)

    def test_issue8202(self):
        # Make sure package __init__ modules see "-m" in sys.argv0 while
        # searching for the module to execute
        with os_helper.temp_dir() as script_dir:
            with os_helper.change_cwd(path=script_dir):
                pkg_dir = os.path.join(script_dir, 'test_pkg')
                make_pkg(pkg_dir, "import sys; print('init_argv0==%r' % sys.argv[0])")
                script_name = _make_test_script(pkg_dir, 'script')
                rc, out, err = assert_python_ok('-m', 'test_pkg.script', *example_args, __isolated=False)
                if verbose > 1:
                    print(repr(out))
                expected = "init_argv0==%r" % '-m'
                self.assertIn(expected.encode('utf-8'), out)
                self._check_output(script_name, rc, out,
                                   script_name, script_name, script_dir, 'test_pkg',
                                   importlib.machinery.SourceFileLoader)

    def test_issue8202_dash_c_file_ignored(self):
        # Make sure a "-c" file in the current directory
        # does not alter the value of sys.path[0]
        with os_helper.temp_dir() as script_dir:
            with os_helper.change_cwd(path=script_dir):
                with open("-c", "w", encoding="utf-8") as f:
                    f.write("data")
                    rc, out, err = assert_python_ok('-c',
                        'import sys; print("sys.path[0]==%r" % sys.path[0])',
                        __isolated=False)
                    if verbose > 1:
                        print(repr(out))
                    expected = "sys.path[0]==%r" % ''
                    self.assertIn(expected.encode('utf-8'), out)

    def test_issue8202_dash_m_file_ignored(self):
        # Make sure a "-m" file in the current directory
        # does not alter the value of sys.path[0]
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'other')
            with os_helper.change_cwd(path=script_dir):
                with open("-m", "w", encoding="utf-8") as f:
                    f.write("data")
                    rc, out, err = assert_python_ok('-m', 'other', *example_args,
                                                    __isolated=False)
                    self._check_output(script_name, rc, out,
                                      script_name, script_name, script_dir, '',
                                      importlib.machinery.SourceFileLoader)

    def test_issue20884(self):
        # On Windows, script with encoding cookie and LF line ending
        # will be failed.
        with os_helper.temp_dir() as script_dir:
            script_name = os.path.join(script_dir, "issue20884.py")
            with open(script_name, "w", encoding="latin1", newline='\n') as f:
                f.write("#coding: iso-8859-1\n")
                f.write('"""\n')
                for _ in range(30):
                    f.write('x'*80 + '\n')
                f.write('"""\n')

            with os_helper.change_cwd(path=script_dir):
                rc, out, err = assert_python_ok(script_name)
            self.assertEqual(b"", out)
            self.assertEqual(b"", err)

    @contextlib.contextmanager
    def setup_test_pkg(self, *args):
        with os_helper.temp_dir() as script_dir, \
                os_helper.change_cwd(path=script_dir):
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir, *args)
            yield pkg_dir

    def check_dash_m_failure(self, *args):
        rc, out, err = assert_python_failure('-m', *args, __isolated=False)
        if verbose > 1:
            print(repr(out))
        self.assertEqual(rc, 1)
        return err

    def test_dash_m_error_code_is_one(self):
        # If a module is invoked with the -m command line flag
        # and results in an error that the return code to the
        # shell is '1'
        with self.setup_test_pkg() as pkg_dir:
            script_name = _make_test_script(pkg_dir, 'other',
                                            "if __name__ == '__main__': raise ValueError")
            err = self.check_dash_m_failure('test_pkg.other', *example_args)
            self.assertIn(b'ValueError', err)

    def test_dash_m_errors(self):
        # Exercise error reporting for various invalid package executions
        tests = (
            ('builtins', br'No code object available'),
            ('builtins.x', br'Error while finding module specification.*'
                br'ModuleNotFoundError'),
            ('builtins.x.y', br'Error while finding module specification.*'
                br'ModuleNotFoundError.*No module named.*not a package'),
            ('os.path', br'loader.*cannot handle'),
            ('importlib', br'No module named.*'
                br'is a package and cannot be directly executed'),
            ('importlib.nonexistent', br'No module named'),
            ('.unittest', br'Relative module names not supported'),
        )
        for name, regex in tests:
            with self.subTest(name):
                rc, _, err = assert_python_failure('-m', name)
                self.assertEqual(rc, 1)
                self.assertRegex(err, regex)
                self.assertNotIn(b'Traceback', err)

    def test_dash_m_bad_pyc(self):
        with os_helper.temp_dir() as script_dir, \
                os_helper.change_cwd(path=script_dir):
            os.mkdir('test_pkg')
            # Create invalid *.pyc as empty file
            with open('test_pkg/__init__.pyc', 'wb'):
                pass
            err = self.check_dash_m_failure('test_pkg')
            self.assertRegex(err,
                br'Error while finding module specification.*'
                br'ImportError.*bad magic number')
            self.assertNotIn(b'is a package', err)
            self.assertNotIn(b'Traceback', err)

    def test_hint_when_triying_to_import_a_py_file(self):
        with os_helper.temp_dir() as script_dir, \
                os_helper.change_cwd(path=script_dir):
            # Create invalid *.pyc as empty file
            with open('asyncio.py', 'wb'):
                pass
            err = self.check_dash_m_failure('asyncio.py')
            self.assertIn(b"Try using 'asyncio' instead "
                          b"of 'asyncio.py' as the module name", err)

    def test_dash_m_init_traceback(self):
        # These were wrapped in an ImportError and tracebacks were
        # suppressed; see Issue 14285
        exceptions = (ImportError, AttributeError, TypeError, ValueError)
        for exception in exceptions:
            exception = exception.__name__
            init = "raise {0}('Exception in __init__.py')".format(exception)
            with self.subTest(exception), \
                    self.setup_test_pkg(init) as pkg_dir:
                err = self.check_dash_m_failure('test_pkg')
                self.assertIn(exception.encode('ascii'), err)
                self.assertIn(b'Exception in __init__.py', err)
                self.assertIn(b'Traceback', err)

    def test_dash_m_main_traceback(self):
        # Ensure that an ImportError's traceback is reported
        with self.setup_test_pkg() as pkg_dir:
            main = "raise ImportError('Exception in __main__ module')"
            _make_test_script(pkg_dir, '__main__', main)
            err = self.check_dash_m_failure('test_pkg')
            self.assertIn(b'ImportError', err)
            self.assertIn(b'Exception in __main__ module', err)
            self.assertIn(b'Traceback', err)

    def test_pep_409_verbiage(self):
        # Make sure PEP 409 syntax properly suppresses
        # the context of an exception
        script = textwrap.dedent("""\
            try:
                raise ValueError
            except:
                raise NameError from None
            """)
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script', script)
            exitcode, stdout, stderr = assert_python_failure(script_name)
            text = stderr.decode('ascii').split('\n')
            self.assertEqual(len(text), 5)
            self.assertTrue(text[0].startswith('Traceback'))
            self.assertTrue(text[1].startswith('  File '))
            self.assertTrue(text[3].startswith('NameError'))

    def test_non_ascii(self):
        # Mac OS X denies the creation of a file with an invalid UTF-8 name.
        # Windows allows creating a name with an arbitrary bytes name, but
        # Python cannot a undecodable bytes argument to a subprocess.
        if (os_helper.TESTFN_UNDECODABLE
        and sys.platform not in ('win32', 'darwin')):
            name = os.fsdecode(os_helper.TESTFN_UNDECODABLE)
        elif os_helper.TESTFN_NONASCII:
            name = os_helper.TESTFN_NONASCII
        else:
            self.skipTest("need os_helper.TESTFN_NONASCII")

        # Issue #16218
        source = 'print(ascii(__file__))\n'
        script_name = _make_test_script(os.getcwd(), name, source)
        self.addCleanup(os_helper.unlink, script_name)
        rc, stdout, stderr = assert_python_ok(script_name)
        self.assertEqual(
            ascii(script_name),
            stdout.rstrip().decode('ascii'),
            'stdout=%r stderr=%r' % (stdout, stderr))
        self.assertEqual(0, rc)

    def test_issue20500_exit_with_exception_value(self):
        script = textwrap.dedent("""\
            import sys
            error = None
            try:
                raise ValueError('some text')
            except ValueError as err:
                error = err

            if error:
                sys.exit(error)
            """)
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script', script)
            exitcode, stdout, stderr = assert_python_failure(script_name)
            text = stderr.decode('ascii')
            self.assertEqual(text.rstrip(), "some text")

    def test_syntaxerror_unindented_caret_position(self):
        script = "1 + 1 = 2\n"
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script', script)
            exitcode, stdout, stderr = assert_python_failure(script_name)
            text = io.TextIOWrapper(io.BytesIO(stderr), 'ascii').read()
            # Confirm that the caret is located under the '=' sign
            self.assertIn("\n    ^^^^^\n", text)

    def test_syntaxerror_indented_caret_position(self):
        script = textwrap.dedent("""\
            if True:
                1 + 1 = 2
            """)
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script', script)
            exitcode, stdout, stderr = assert_python_failure(script_name)
            text = io.TextIOWrapper(io.BytesIO(stderr), 'ascii').read()
            # Confirm that the caret starts under the first 1 character
            self.assertIn("\n    1 + 1 = 2\n    ^^^^^\n", text)

            # Try the same with a form feed at the start of the indented line
            script = (
                "if True:\n"
                "\f    1 + 1 = 2\n"
            )
            script_name = _make_test_script(script_dir, "script", script)
            exitcode, stdout, stderr = assert_python_failure(script_name)
            text = io.TextIOWrapper(io.BytesIO(stderr), "ascii").read()
            self.assertNotIn("\f", text)
            self.assertIn("\n    1 + 1 = 2\n    ^^^^^\n", text)

    def test_syntaxerror_multi_line_fstring(self):
        script = 'foo = f"""{}\nfoo"""\n'
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script', script)
            exitcode, stdout, stderr = assert_python_failure(script_name)
            self.assertEqual(
                stderr.splitlines()[-3:],
                [
                    b'    foo"""',
                    b'          ^',
                    b'SyntaxError: f-string: empty expression not allowed',
                ],
            )

    def test_syntaxerror_invalid_escape_sequence_multi_line(self):
        script = 'foo = """\\q"""\n'
        with os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script', script)
            exitcode, stdout, stderr = assert_python_failure(
                '-Werror', script_name,
            )
            self.assertEqual(
                stderr.splitlines()[-3:],
                [   b'    foo = """\\q"""',
                    b'          ^^^^^^^^',
                    b'SyntaxError: invalid escape sequence \\q'
                ],
            )

    def test_consistent_sys_path_for_direct_execution(self):
        # This test case ensures that the following all give the same
        # sys.path configuration:
        #
        #    ./python -s script_dir/__main__.py
        #    ./python -s script_dir
        #    ./python -I script_dir
        script = textwrap.dedent("""\
            import sys
            for entry in sys.path:
                print(entry)
            """)
        # Always show full path diffs on errors
        self.maxDiff = None
        with os_helper.temp_dir() as work_dir, os_helper.temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__', script)
            # Reference output comes from directly executing __main__.py
            # We omit PYTHONPATH and user site to align with isolated mode
            p = spawn_python("-Es", script_name, cwd=work_dir)
            out_by_name = kill_python(p).decode().splitlines()
            self.assertEqual(out_by_name[0], script_dir)
            self.assertNotIn(work_dir, out_by_name)
            # Directory execution should give the same output
            p = spawn_python("-Es", script_dir, cwd=work_dir)
            out_by_dir = kill_python(p).decode().splitlines()
            self.assertEqual(out_by_dir, out_by_name)
            # As should directory execution in isolated mode
            p = spawn_python("-I", script_dir, cwd=work_dir)
            out_by_dir_isolated = kill_python(p).decode().splitlines()
            self.assertEqual(out_by_dir_isolated, out_by_dir, out_by_name)

    def test_consistent_sys_path_for_module_execution(self):
        # This test case ensures that the following both give the same
        # sys.path configuration:
        #    ./python -sm script_pkg.__main__
        #    ./python -sm script_pkg
        #
        # And that this fails as unable to find the package:
        #    ./python -Im script_pkg
        script = textwrap.dedent("""\
            import sys
            for entry in sys.path:
                print(entry)
            """)
        # Always show full path diffs on errors
        self.maxDiff = None
        with os_helper.temp_dir() as work_dir:
            script_dir = os.path.join(work_dir, "script_pkg")
            os.mkdir(script_dir)
            script_name = _make_test_script(script_dir, '__main__', script)
            # Reference output comes from `-m script_pkg.__main__`
            # We omit PYTHONPATH and user site to better align with the
            # direct execution test cases
            p = spawn_python("-sm", "script_pkg.__main__", cwd=work_dir)
            out_by_module = kill_python(p).decode().splitlines()
            self.assertEqual(out_by_module[0], work_dir)
            self.assertNotIn(script_dir, out_by_module)
            # Package execution should give the same output
            p = spawn_python("-sm", "script_pkg", cwd=work_dir)
            out_by_package = kill_python(p).decode().splitlines()
            self.assertEqual(out_by_package, out_by_module)
            # Isolated mode should fail with an import error
            exitcode, stdout, stderr = assert_python_failure(
                "-Im", "script_pkg", cwd=work_dir
            )
            traceback_lines = stderr.decode().splitlines()
            self.assertIn("No module named script_pkg", traceback_lines[-1])

    def test_nonexisting_script(self):
        # bpo-34783: "./python script.py" must not crash
        # if the script file doesn't exist.
        # (Skip test for macOS framework builds because sys.executable name
        #  is not the actual Python executable file name.
        script = 'nonexistingscript.py'
        self.assertFalse(os.path.exists(script))

        proc = spawn_python(script, text=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
        out, err = proc.communicate()
        self.assertIn(": can't open file ", err)
        self.assertNotEqual(proc.returncode, 0)


def test_main():
    support.run_unittest(CmdLineTest)
    support.reap_children()

if __name__ == '__main__':
    test_main()
