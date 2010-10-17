# tests command line execution of scripts

import unittest
import sys
import os
import os.path
import py_compile

from test import support
from test.script_helper import (
    make_pkg, make_script, make_zip_pkg, make_zip_script,
    assert_python_ok, assert_python_failure, temp_dir)

verbose = support.verbose

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
print('__file__==%a' % __file__)
assertEqual(__cached__, None)
print('__package__==%r' % __package__)
# Check the sys module
import sys
assertIdentical(globals(), sys.modules[__name__].__dict__)
print('sys.argv[0]==%a' % sys.argv[0])
print('sys.path[0]==%a' % sys.path[0])
# Check the working directory
import os
print('cwd==%a' % os.getcwd())
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
    def _check_output(self, script_name, exit_code, data,
                             expected_file, expected_argv0,
                             expected_path0, expected_package):
        if verbose > 1:
            print("Output from test script %r:" % script_name)
            print(data)
        self.assertEqual(exit_code, 0)
        printed_file = '__file__==%a' % expected_file
        printed_package = '__package__==%r' % expected_package
        printed_argv0 = 'sys.argv[0]==%a' % expected_argv0
        printed_path0 = 'sys.path[0]==%a' % expected_path0
        printed_cwd = 'cwd==%a' % os.getcwd()
        if verbose > 1:
            print('Expected output:')
            print(printed_file)
            print(printed_package)
            print(printed_argv0)
            print(printed_cwd)
        self.assertIn(printed_file.encode('utf-8'), data)
        self.assertIn(printed_package.encode('utf-8'), data)
        self.assertIn(printed_argv0.encode('utf-8'), data)
        self.assertIn(printed_path0.encode('utf-8'), data)
        self.assertIn(printed_cwd.encode('utf-8'), data)

    def _check_script(self, script_name, expected_file,
                            expected_argv0, expected_path0,
                            expected_package,
                            *cmd_line_switches):
        if not __debug__:
            cmd_line_switches += ('-' + 'O' * sys.flags.optimize,)
        run_args = cmd_line_switches + (script_name,)
        rc, out, err = assert_python_ok(*run_args)
        self._check_output(script_name, rc, out + err, expected_file,
                           expected_argv0, expected_path0, expected_package)

    def _check_import_error(self, script_name, expected_msg,
                            *cmd_line_switches):
        run_args = cmd_line_switches + (script_name,)
        rc, out, err = assert_python_failure(*run_args)
        if verbose > 1:
            print('Output from test script %r:' % script_name)
            print(err)
            print('Expected output: %r' % expected_msg)
        self.assertIn(expected_msg.encode('utf-8'), err)

    def test_basic_script(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script')
            self._check_script(script_name, script_name, script_name,
                               script_dir, None)

    def test_script_compiled(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script')
            py_compile.compile(script_name, doraise=True)
            os.remove(script_name)
            pyc_file = support.make_legacy_pyc(script_name)
            self._check_script(pyc_file, pyc_file,
                               pyc_file, script_dir, None)

    def test_directory(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            self._check_script(script_dir, script_name, script_dir,
                               script_dir, '')

    def test_directory_compiled(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            py_compile.compile(script_name, doraise=True)
            os.remove(script_name)
            pyc_file = support.make_legacy_pyc(script_name)
            self._check_script(script_dir, pyc_file, script_dir,
                               script_dir, '')

    def test_directory_error(self):
        with temp_dir() as script_dir:
            msg = "can't find '__main__' module in %r" % script_dir
            self._check_import_error(script_dir, msg)

    def test_zipfile(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            zip_name, run_name = make_zip_script(script_dir, 'test_zip', script_name)
            self._check_script(zip_name, run_name, zip_name, zip_name, '')

    def test_zipfile_compiled(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            compiled_name = py_compile.compile(script_name, doraise=True)
            zip_name, run_name = make_zip_script(script_dir, 'test_zip', compiled_name)
            self._check_script(zip_name, run_name, zip_name, zip_name, '')

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
            self._check_script(launch_name, script_name, script_name, script_dir, 'test_pkg')

    def test_module_in_package_in_zipfile(self):
        with temp_dir() as script_dir:
            zip_name, run_name = _make_test_zip_pkg(script_dir, 'test_zip', 'test_pkg', 'script')
            launch_name = _make_launch_script(script_dir, 'launch', 'test_pkg.script', zip_name)
            self._check_script(launch_name, run_name, run_name, zip_name, 'test_pkg')

    def test_module_in_subpackage_in_zipfile(self):
        with temp_dir() as script_dir:
            zip_name, run_name = _make_test_zip_pkg(script_dir, 'test_zip', 'test_pkg', 'script', depth=2)
            launch_name = _make_launch_script(script_dir, 'launch', 'test_pkg.test_pkg.script', zip_name)
            self._check_script(launch_name, run_name, run_name, zip_name, 'test_pkg.test_pkg')

    def test_package(self):
        with temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir)
            script_name = _make_test_script(pkg_dir, '__main__')
            launch_name = _make_launch_script(script_dir, 'launch', 'test_pkg')
            self._check_script(launch_name, script_name,
                               script_name, script_dir, 'test_pkg')

    def test_package_compiled(self):
        with temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            make_pkg(pkg_dir)
            script_name = _make_test_script(pkg_dir, '__main__')
            compiled_name = py_compile.compile(script_name, doraise=True)
            os.remove(script_name)
            pyc_file = support.make_legacy_pyc(script_name)
            launch_name = _make_launch_script(script_dir, 'launch', 'test_pkg')
            self._check_script(launch_name, pyc_file,
                               pyc_file, script_dir, 'test_pkg')

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

    def test_issue8202(self):
        # Make sure package __init__ modules see "-m" in sys.argv0 while
        # searching for the module to execute
        with temp_dir() as script_dir:
            with support.temp_cwd(path=script_dir):
                pkg_dir = os.path.join(script_dir, 'test_pkg')
                make_pkg(pkg_dir, "import sys; print('init_argv0==%r' % sys.argv[0])")
                script_name = _make_test_script(pkg_dir, 'script')
                rc, out, err = assert_python_ok('-m', 'test_pkg.script')
                if verbose > 1:
                    print(data)
                expected = "init_argv0==%r" % '-m'
                self.assertIn(expected.encode('utf-8'), out)
                self._check_output(script_name, rc, out,
                                   script_name, script_name, '', 'test_pkg')

    def test_issue8202_dash_c_file_ignored(self):
        # Make sure a "-c" file in the current directory
        # does not alter the value of sys.path[0]
        with temp_dir() as script_dir:
            with support.temp_cwd(path=script_dir):
                with open("-c", "w") as f:
                    f.write("data")
                    rc, out, err = assert_python_ok('-c',
                        'import sys; print("sys.path[0]==%r" % sys.path[0])')
                    if verbose > 1:
                        print(out)
                    expected = "sys.path[0]==%r" % ''
                    self.assertIn(expected.encode('utf-8'), out)

    def test_issue8202_dash_m_file_ignored(self):
        # Make sure a "-m" file in the current directory
        # does not alter the value of sys.path[0]
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'other')
            with support.temp_cwd(path=script_dir):
                with open("-m", "w") as f:
                    f.write("data")
                    rc, out, err = assert_python_ok('-m', 'other')
                    self._check_output(script_name, rc, out,
                                      script_name, script_name, '', '')

def test_main():
    support.run_unittest(CmdLineTest)
    support.reap_children()

if __name__ == '__main__':
    test_main()
