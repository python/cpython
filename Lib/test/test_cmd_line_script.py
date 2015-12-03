# Tests command line execution of scripts

import contextlib
import unittest
import os
import os.path
import test.test_support
from test.script_helper import (run_python,
                                temp_dir, make_script, compile_script,
                                assert_python_failure, make_pkg,
                                make_zip_script, make_zip_pkg)

verbose = test.test_support.verbose


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
print '__file__==%r' % __file__
print '__package__==%r' % __package__
# Check the sys module
import sys
assertIdentical(globals(), sys.modules[__name__].__dict__)
print 'sys.argv[0]==%r' % sys.argv[0]
"""

def _make_test_script(script_dir, script_basename, source=test_source):
    return make_script(script_dir, script_basename, source)

def _make_test_zip_pkg(zip_dir, zip_basename, pkg_name, script_basename,
                       source=test_source, depth=1):
    return make_zip_pkg(zip_dir, zip_basename, pkg_name, script_basename,
                        source, depth)

# There's no easy way to pass the script directory in to get
# -m to work (avoiding that is the whole point of making
# directories and zipfiles executable!)
# So we fake it for testing purposes with a custom launch script
launch_source = """\
import sys, os.path, runpy
sys.path.insert(0, %s)
runpy._run_module_as_main(%r)
"""

def _make_launch_script(script_dir, script_basename, module_name, path=None):
    if path is None:
        path = "os.path.dirname(__file__)"
    else:
        path = repr(path)
    source = launch_source % (path, module_name)
    return make_script(script_dir, script_basename, source)

class CmdLineTest(unittest.TestCase):
    def _check_script(self, script_name, expected_file,
                            expected_argv0, expected_package,
                            *cmd_line_switches):
        run_args = cmd_line_switches + (script_name,)
        exit_code, data = run_python(*run_args)
        if verbose:
            print 'Output from test script %r:' % script_name
            print data
        self.assertEqual(exit_code, 0)
        printed_file = '__file__==%r' % expected_file
        printed_argv0 = 'sys.argv[0]==%r' % expected_argv0
        printed_package = '__package__==%r' % expected_package
        if verbose:
            print 'Expected output:'
            print printed_file
            print printed_package
            print printed_argv0
        self.assertIn(printed_file, data)
        self.assertIn(printed_package, data)
        self.assertIn(printed_argv0, data)

    def _check_import_error(self, script_name, expected_msg,
                            *cmd_line_switches):
        run_args = cmd_line_switches + (script_name,)
        exit_code, data = run_python(*run_args)
        if verbose:
            print 'Output from test script %r:' % script_name
            print data
            print 'Expected output: %r' % expected_msg
        self.assertIn(expected_msg, data)

    def test_basic_script(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script')
            self._check_script(script_name, script_name, script_name, None)

    def test_script_compiled(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script')
            compiled_name = compile_script(script_name)
            os.remove(script_name)
            self._check_script(compiled_name, compiled_name, compiled_name, None)

    def test_directory(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            self._check_script(script_dir, script_name, script_dir, '')

    def test_directory_compiled(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            compiled_name = compile_script(script_name)
            os.remove(script_name)
            self._check_script(script_dir, compiled_name, script_dir, '')

    def test_directory_error(self):
        with temp_dir() as script_dir:
            msg = "can't find '__main__' module in %r" % script_dir
            self._check_import_error(script_dir, msg)

    def test_zipfile(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            zip_name, run_name = make_zip_script(script_dir, 'test_zip', script_name)
            self._check_script(zip_name, run_name, zip_name, '')

    def test_zipfile_compiled(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            compiled_name = compile_script(script_name)
            zip_name, run_name = make_zip_script(script_dir, 'test_zip', compiled_name)
            self._check_script(zip_name, run_name, zip_name, '')

    def test_zipfile_error(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'not_main')
            zip_name, run_name = make_zip_script(script_dir, 'test_zip', script_name)
            msg = "can't find '__main__' module in %r" % zip_name
            self._check_import_error(zip_name, msg)

    def test_module_in_package(self):
        with temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir)
            script_name = _make_test_script(pkg_dir, 'script')
            launch_name = _make_launch_script(script_dir, 'launch', 'test_pkg.script')
            self._check_script(launch_name, script_name, script_name, 'test_pkg')

    def test_module_in_package_in_zipfile(self):
        with temp_dir() as script_dir:
            zip_name, run_name = _make_test_zip_pkg(script_dir, 'test_zip', 'test_pkg', 'script')
            launch_name = _make_launch_script(script_dir, 'launch', 'test_pkg.script', zip_name)
            self._check_script(launch_name, run_name, run_name, 'test_pkg')

    def test_module_in_subpackage_in_zipfile(self):
        with temp_dir() as script_dir:
            zip_name, run_name = _make_test_zip_pkg(script_dir, 'test_zip', 'test_pkg', 'script', depth=2)
            launch_name = _make_launch_script(script_dir, 'launch', 'test_pkg.test_pkg.script', zip_name)
            self._check_script(launch_name, run_name, run_name, 'test_pkg.test_pkg')

    def test_package(self):
        with temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir)
            script_name = _make_test_script(pkg_dir, '__main__')
            launch_name = _make_launch_script(script_dir, 'launch', 'test_pkg')
            self._check_script(launch_name, script_name,
                               script_name, 'test_pkg')

    def test_package_compiled(self):
        with temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir)
            script_name = _make_test_script(pkg_dir, '__main__')
            compiled_name = compile_script(script_name)
            os.remove(script_name)
            launch_name = _make_launch_script(script_dir, 'launch', 'test_pkg')
            self._check_script(launch_name, compiled_name,
                               compiled_name, 'test_pkg')

    def test_package_error(self):
        with temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir)
            msg = ("'test_pkg' is a package and cannot "
                   "be directly executed")
            launch_name = _make_launch_script(script_dir, 'launch', 'test_pkg')
            self._check_import_error(launch_name, msg)

    def test_package_recursion(self):
        with temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir)
            main_dir = os.path.join(pkg_dir, '__main__')
            make_pkg(main_dir)
            msg = ("Cannot use package as __main__ module; "
                   "'test_pkg' is a package and cannot "
                   "be directly executed")
            launch_name = _make_launch_script(script_dir, 'launch', 'test_pkg')
            self._check_import_error(launch_name, msg)

    @contextlib.contextmanager
    def setup_test_pkg(self, *args):
        with temp_dir() as script_dir, \
                test.test_support.change_cwd(script_dir):
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir, *args)
            yield pkg_dir

    def check_dash_m_failure(self, *args):
        rc, out, err = assert_python_failure('-m', *args)
        if verbose > 1:
            print(out)
        self.assertEqual(rc, 1)
        return err

    def test_dash_m_error_code_is_one(self):
        # If a module is invoked with the -m command line flag
        # and results in an error that the return code to the
        # shell is '1'
        with self.setup_test_pkg() as pkg_dir:
            script_name = _make_test_script(pkg_dir, 'other', "if __name__ == '__main__': raise ValueError")
            err = self.check_dash_m_failure('test_pkg.other', *example_args)
            self.assertIn(b'ValueError', err)

    def test_dash_m_errors(self):
        # Exercise error reporting for various invalid package executions
        tests = (
            ('__builtin__', br'No code object available'),
            ('__builtin__.x', br'No module named'),
            ('__builtin__.x.y', br'No module named'),
            ('os.path', br'Loader.*cannot handle'),
            ('importlib', br'No module named.*'
                br'is a package and cannot be directly executed'),
            ('importlib.nonexistant', br'No module named'),
        )
        for name, regex in tests:
            rc, _, err = assert_python_failure('-m', name)
            self.assertEqual(rc, 1)
            self.assertRegexpMatches(err, regex)
            self.assertNotIn(b'Traceback', err)

    def test_dash_m_init_traceback(self):
        # These were wrapped in an ImportError and tracebacks were
        # suppressed; see Issue 14285
        exceptions = (ImportError, AttributeError, TypeError, ValueError)
        for exception in exceptions:
            exception = exception.__name__
            init = "raise {0}('Exception in __init__.py')".format(exception)
            with self.setup_test_pkg(init) as pkg_dir:
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


def test_main():
    test.test_support.run_unittest(CmdLineTest)
    test.test_support.reap_children()

if __name__ == '__main__':
    test_main()
