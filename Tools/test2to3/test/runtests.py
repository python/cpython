# Fictitious test runner for the project

import sys, os

if sys.version_info > (3,):
    # copy test suite over to "build/lib" and convert it
    from distutils.util import copydir_run_2to3
    testroot = os.path.dirname(__file__)
    newroot = os.path.join(testroot, '..', 'build/lib/test')
    copydir_run_2to3(testroot, newroot)
    # in the following imports, pick up the converted modules
    sys.path[0] = newroot

# run the tests here...

from test_foo import FooTest

import unittest
unittest.main()
