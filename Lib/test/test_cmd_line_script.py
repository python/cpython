# Tests command line execution of scripts
from __future__ import with_statement

import unittest
import os
import os.path
import sys
import test.test_support
import tempfile
import subprocess
import py_compile
import contextlib
import shutil
import zipfile

verbose = test.test_support.verbose

# XXX ncoghlan: Should we consider moving these to test_support?
from test.test_cmd_line import _spawn_python, _kill_python

def _run_python(*args):
    if __debug__:
        p = _spawn_python(*args)
    else:
        p = _spawn_python('-O', *args)
    stdout_data = _kill_python(p)
    return p.wait(), stdout_data.decode()

@contextlib.contextmanager
def temp_dir():
    dirname = tempfile.mkdtemp()
    dirname = os.path.realpath(dirname)
    try:
        yield dirname
    finally:
        shutil.rmtree(dirname)

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
print('__file__==%r' % __file__)
print('__package__==%r' % __package__)
# Check the sys module
import sys
assertIdentical(globals(), sys.modules[__name__].__dict__)
print('sys.argv[0]==%r' % sys.argv[0])
"""

def _make_test_script(script_dir, script_basename, source=test_source):
    script_filename = script_basename+os.path.extsep+'py'
    script_name = os.path.join(script_dir, script_filename)
    script_file = open(script_name, 'w')
    script_file.write(source)
    script_file.close()
    return script_name

def _compile_test_script(script_name):
    py_compile.compile(script_name, doraise=True)
    if __debug__:
        compiled_name = script_name + 'c'
    else:
        compiled_name = script_name + 'o'
    return compiled_name

def _make_test_zip(zip_dir, zip_basename, script_name):
    zip_filename = zip_basename+os.path.extsep+"zip"
    zip_name = os.path.join(zip_dir, zip_filename)
    zip_file = zipfile.ZipFile(zip_name, 'w')
    zip_file.write(script_name, os.path.basename(script_name))
    zip_file.close()
    # if verbose:
    #    zip_file = zipfile.ZipFile(zip_name, 'r')
    #    print("Contents of %r:" % zip_name)
    #    zip_file.printdir()
    #    zip_file.close()
    return zip_name

def _make_test_pkg(pkg_dir):
    os.mkdir(pkg_dir)
    _make_test_script(pkg_dir, '__init__', '')

# There's no easy way to pass the script directory in to get
# -m to work (avoiding that is the whole point of making
# directories and zipfiles executable!)
# So we fake it for testing purposes with a custom launch script
launch_source = """\
import sys, os.path, runpy
sys.path[0:0] = os.path.dirname(__file__)
runpy._run_module_as_main(%r)
"""

def _make_launch_script(script_dir, script_basename, module_name):
    return _make_test_script(script_dir, script_basename,
                             launch_source % module_name)

class CmdLineTest(unittest.TestCase):
    def _check_script(self, script_name, expected_file,
                            expected_argv0, expected_package,
                            *cmd_line_switches):
        run_args = cmd_line_switches + (script_name,)
        exit_code, data = _run_python(*run_args)
        if verbose:
            print("Output from test script %r:" % script_name)
            print(data)
        self.assertEqual(exit_code, 0)
        printed_file = '__file__==%r' % expected_file
        printed_argv0 = 'sys.argv[0]==%r' % expected_argv0
        printed_package = '__package__==%r' % expected_package
        if verbose:
            print('Expected output:')
            print(printed_file)
            print(printed_package)
            print(printed_argv0)
        self.assert_(printed_file in data)
        self.assert_(printed_package in data)
        self.assert_(printed_argv0 in data)

    def test_basic_script(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script')
            self._check_script(script_name, script_name, script_name, None)

    def test_script_compiled(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, 'script')
            compiled_name = _compile_test_script(script_name)
            os.remove(script_name)
            self._check_script(compiled_name, compiled_name, compiled_name, None)

    def test_directory(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            self._check_script(script_dir, script_name, script_dir, '')

    def test_directory_compiled(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            compiled_name = _compile_test_script(script_name)
            os.remove(script_name)
            self._check_script(script_dir, compiled_name, script_dir, '')

    def test_zipfile(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            zip_name = _make_test_zip(script_dir, 'test_zip', script_name)
            self._check_script(zip_name, None, zip_name, '')

    def test_zipfile_compiled(self):
        with temp_dir() as script_dir:
            script_name = _make_test_script(script_dir, '__main__')
            compiled_name = _compile_test_script(script_name)
            zip_name = _make_test_zip(script_dir, 'test_zip', compiled_name)
            self._check_script(zip_name, None, zip_name, '')

    def test_module_in_package(self):
        with temp_dir() as script_dir:
            pkg_dir = os.path.join(script_dir, 'test_pkg')
            _make_test_pkg(pkg_dir)
            script_name = _make_test_script(pkg_dir, 'script')
            launch_name = _make_launch_script(script_dir, 'launch', 'test_pkg.script')
            self._check_script(launch_name, script_name,
                               script_name, 'test_pkg')


def test_main():
    test.test_support.run_unittest(CmdLineTest)
    test.test_support.reap_children()

if __name__ == '__main__':
    test_main()
