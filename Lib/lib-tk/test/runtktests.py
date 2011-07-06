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
import test.test_support

this_dir_path = os.path.abspath(os.path.dirname(__file__))

_tk_unavailable = None

def check_tk_availability():
    """Check that Tk is installed and available."""
    global _tk_unavailable

    if _tk_unavailable is None:
        _tk_unavailable = False
        if sys.platform == 'darwin':
            # The Aqua Tk implementations on OS X can abort the process if
            # being called in an environment where a window server connection
            # cannot be made, for instance when invoked by a buildbot or ssh
            # process not running under the same user id as the current console
            # user.  To avoid that, raise an exception if the window manager
            # connection is not available.
            from ctypes import cdll, c_int, pointer, Structure
            from ctypes.util import find_library

            app_services = cdll.LoadLibrary(find_library("ApplicationServices"))

            if app_services.CGMainDisplayID() == 0:
                _tk_unavailable = "cannot run without OS X window manager"
            else:
                class ProcessSerialNumber(Structure):
                    _fields_ = [("highLongOfPSN", c_int),
                                ("lowLongOfPSN", c_int)]
                psn = ProcessSerialNumber()
                psn_p = pointer(psn)
                if (  (app_services.GetCurrentProcess(psn_p) < 0) or
                      (app_services.SetFrontProcess(psn_p) < 0) ):
                    _tk_unavailable = "cannot run without OS X gui process"
        else:   # not OS X
            import Tkinter
            try:
                Tkinter.Button()
            except Tkinter.TclError as msg:
                # assuming tk is not available
                _tk_unavailable = "tk not available: %s" % msg

    if _tk_unavailable:
        raise unittest.SkipTest(_tk_unavailable)
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
