"""Support functions for testing scripts in the Tools directory."""
import os
import unittest
import importlib
from test import support
from fnmatch import fnmatch

basepath = os.path.dirname(                 # <src/install dir>
                os.path.dirname(                # Lib
                    os.path.dirname(                # test
                        os.path.dirname(__file__))))    # test_tools

toolsdir = os.path.join(basepath, 'Tools')
scriptsdir = os.path.join(toolsdir, 'scripts')

def skip_if_missing():
    if not os.path.isdir(scriptsdir):
        raise unittest.SkipTest('scripts directory could not be found')

def import_tool(toolname):
    with support.DirsOnSysPath(scriptsdir):
        return importlib.import_module(toolname)

def load_tests(*args):
    return support.load_package_tests(os.path.dirname(__file__), *args)
