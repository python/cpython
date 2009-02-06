"""
Use this module to get and run all tk tests.

tkinter tests should live in a package inside the directory where this file
lives, like test_tkinter.
Extensions also should live in packages following the same rule as above.
"""

import os
import sys
import unittest
import importlib
import test.support

this_dir_path = os.path.abspath(os.path.dirname(__file__))

def is_package(path):
    for name in os.listdir(path):
        if name in ('__init__.py', '__init__.pyc', '__init.pyo'):
            return True
    return False

def get_tests_modules(basepath=this_dir_path, gui=True, packages=None):
    """This will import and yield modules whose names start with test_
    and are inside packages found in the path starting at basepath.

    If packages is specified it should contain package names that
    want their tests collected.
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
                        ".%s.%s" % (pkg_name, name[:-len(py_ext)]),
                        "tkinter.test")
                except test.support.ResourceDenied:
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
    test.support.use_resources = ['gui']
    test.support.run_unittest(*get_tests())
