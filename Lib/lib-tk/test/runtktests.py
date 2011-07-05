"""
Use this module to get and run all tk tests.

Tkinter tests should live in a package inside the directory where this file
lives, like test_tkinter.
Extensions also should live in packages following the same rule as above.
"""

import os
import sys
import unittest
import importlib
import subprocess
import test.test_support

this_dir_path = os.path.abspath(os.path.dirname(__file__))

_tk_available = None

def check_tk_availability():
    """Check that Tk is installed and available."""
    global _tk_available

    if _tk_available is not None:
        return

    if sys.platform == 'darwin':
        # The Aqua Tk implementations on OS X can abort the process if
        # being called in an environment where a window server connection
        # cannot be made, for instance when invoked by a buildbot or ssh
        # process not running under the same user id as the current console
        # user.  Instead, try to initialize Tk under a subprocess.
        p = subprocess.Popen(
                [sys.executable, '-c', 'import Tkinter; Tkinter.Button()'],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stderr = test.test_support.strip_python_stderr(p.communicate()[1])
        if stderr or p.returncode:
            raise unittest.SkipTest("tk cannot be initialized: %s" % stderr)
    else:
        import Tkinter
        try:
            Tkinter.Button()
        except Tkinter.TclError as msg:
            # assuming tk is not available
            raise unittest.SkipTest("tk not available: %s" % msg)

    _tk_available = True
    return

def is_package(path):
    for name in os.listdir(path):
        if name in ('__init__.py', '__init__.pyc', '__init.pyo'):
            return True
    return False

def get_tests_modules(basepath=this_dir_path, gui=True, packages=None):
    """This will import and yield modules whose names start with test_
    and are inside packages found in the path starting at basepath.

    If packages is specified it should contain package names that want
    their tests colleted.
    """
    py_ext = '.py'

    for dirpath, dirnames, filenames in os.walk(basepath):
        for dirname in list(dirnames):
            if dirname[0] == '.':
                dirnames.remove(dirname)

        if is_package(dirpath) and filenames:
            pkg_name = dirpath[len(basepath) + len(os.sep):].replace('/', '.')
            if packages and pkg_name not in packages:
                continue

            filenames = filter(
                    lambda x: x.startswith('test_') and x.endswith(py_ext),
                    filenames)

            for name in filenames:
                try:
                    yield importlib.import_module(
                            ".%s" % name[:-len(py_ext)], pkg_name)
                except test.test_support.ResourceDenied:
                    if gui:
                        raise

def get_tests(text=True, gui=True, packages=None):
    """Yield all the tests in the modules found by get_tests_modules.

    If nogui is True, only tests that do not require a GUI will be
    returned."""
    attrs = []
    if text:
        attrs.append('tests_nogui')
    if gui:
        attrs.append('tests_gui')
    for module in get_tests_modules(gui=gui, packages=packages):
        for attr in attrs:
            for test in getattr(module, attr, ()):
                yield test

if __name__ == "__main__":
    test.test_support.use_resources = ['gui']
    test.test_support.run_unittest(*get_tests())
