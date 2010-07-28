import imp
import os
import sys
from test import test_support
import unittest

import trace
from trace import CoverageResults, Trace


#------------------------------- Utilities -----------------------------------#

def make_fake_module():
    """Creates a fake module named 'fakemodule'.

    The new module has a single function named 'foo', and it's placed in
    sys.modules
    The file this fake module "comes from" is fakefile.py

    """

    # Prepare the function to import from the fake module
    #
    _fake_foo_src = r'''
def foo(a_):
    b = a_ + 1
    return b + 2
'''.lstrip()

    _fake_foo = compile(_fake_foo_src, 'fakefile.py', 'exec')

    # Create a new module, place the function into it and add it to sys.modules
    #
    fakemodule = imp.new_module('fakemodule')
    exec _fake_foo in fakemodule.__dict__
    fakemodule.__file__ = 'fakefile.py'
    sys.modules['fakemodule'] = fakemodule


def modname(filename):
    """Infer a module name from a containing file name"""
    base = os.path.basename(filename)
    mod, ext = os.path.splitext(base)
    return mod


def my_file_and_modname():
    """The file and module name of this file (__file__)"""
    return __file__, modname(__file__)


#-------------------- Target functions for tracing ---------------------------#

def _traced_func_linear(a_, b_):
    a = a_
    b = b_
    c = a + b
    return c

def _traced_func_loop(a_, b_):
    c = a_
    for i in range(5):
        c += b_
    return c

# Expects the 'fakemodule' module to exist and have a 'foo' function in it
#
def _traced_func_importing(a_, b_):
    from fakemodule import foo
    return a_ + b_ + foo(1)

def _traced_func_simple_caller(a_):
    c = _traced_func_linear(a_, a_)
    return c + a_

def _traced_func_importing_caller(a_):
    k = _traced_func_simple_caller(a_)
    k += _traced_func_importing(k, a_)
    return k


#------------------------------ Test cases -----------------------------------#


class TestLineCounts(unittest.TestCase):
    """White-box testing of line-counting, via runfunc"""
    def setUp(self):
        self.tr = Trace(count=1, trace=0, countfuncs=0, countcallers=0)

    def test_traced_func_linear(self):
        result = self.tr.runfunc(_traced_func_linear, 2, 5)
        self.assertEqual(result, 7)

        # all lines are executed once
        expected = {}
        firstlineno = _traced_func_linear.__code__.co_firstlineno
        for i in range(1, 5):
            expected[(__file__, firstlineno +  i)] = 1

        self.assertEqual(self.tr.results().counts, expected)

    def test_traced_func_loop(self):
        self.tr.runfunc(_traced_func_loop, 2, 3)

        firstlineno = _traced_func_loop.__code__.co_firstlineno
        expected = {
            (__file__, firstlineno + 1): 1,
            (__file__, firstlineno + 2): 6,
            (__file__, firstlineno + 3): 5,
            (__file__, firstlineno + 4): 1,
        }
        self.assertEqual(self.tr.results().counts, expected)

    def test_traced_func_importing(self):
        make_fake_module()
        self.tr.runfunc(_traced_func_importing, 2, 5)

        firstlineno = _traced_func_importing.__code__.co_firstlineno
        expected = {
            (__file__, firstlineno + 1): 1,
            (__file__, firstlineno + 2): 1,
            ('fakefile.py', 2): 1,
            ('fakefile.py', 3): 1,
        }
        self.assertEqual(self.tr.results().counts, expected)


class TestRunExecCounts(unittest.TestCase):
    """A simple sanity test of line-counting, via run (exec)"""
    def test_tt(self):
        self.tr = Trace(count=1, trace=0, countfuncs=0, countcallers=0)
        code = r'''_traced_func_loop(2, 5)'''
        code = compile(code, __file__, 'exec')
        self.tr.run(code)

        firstlineno = _traced_func_loop.__code__.co_firstlineno
        expected = {
            (__file__, firstlineno + 1): 1,
            (__file__, firstlineno + 2): 6,
            (__file__, firstlineno + 3): 5,
            (__file__, firstlineno + 4): 1,
        }

        # When used through 'run', some other spurios counts are produced, like
        # the settrace of threading, which we ignore, just making sure that the
        # counts fo _traced_func_loop were right.
        #
        for k in expected.keys():
            self.assertEqual(self.tr.results().counts[k], expected[k])


class TestFuncs(unittest.TestCase):
    """White-box testing of funcs tracing"""
    def setUp(self):
        self.tr = Trace(count=0, trace=0, countfuncs=1)
        self.filemod = my_file_and_modname()

    def test_simple_caller(self):
        self.tr.runfunc(_traced_func_simple_caller, 1)

        expected = {
            self.filemod + ('_traced_func_simple_caller',): 1,
            self.filemod + ('_traced_func_linear',): 1,
        }
        self.assertEqual(self.tr.results().calledfuncs, expected)

    def test_loop_caller_importing(self):
        make_fake_module()
        self.tr.runfunc(_traced_func_importing_caller, 1)

        expected = {
            self.filemod + ('_traced_func_simple_caller',): 1,
            self.filemod + ('_traced_func_linear',): 1,
            self.filemod + ('_traced_func_importing_caller',): 1,
            self.filemod + ('_traced_func_importing',): 1,
            ('fakefile.py', 'fakefile', 'foo'): 1,
        }
        self.assertEqual(self.tr.results().calledfuncs, expected)


class TestCallers(unittest.TestCase):
    """White-box testing of callers tracing"""
    def setUp(self):
        self.tr = Trace(count=0, trace=0, countcallers=1)
        self.filemod = my_file_and_modname()

    def test_loop_caller_importing(self):
        make_fake_module()
        self.tr.runfunc(_traced_func_importing_caller, 1)

        expected = {
            ((os.path.splitext(trace.__file__)[0] + '.py', 'trace', 'Trace.runfunc'),
                (self.filemod + ('_traced_func_importing_caller',))): 1,
            ((self.filemod + ('_traced_func_simple_caller',)),
                (self.filemod + ('_traced_func_linear',))): 1,
            ((self.filemod + ('_traced_func_importing_caller',)),
                (self.filemod + ('_traced_func_simple_caller',))): 1,
            ((self.filemod + ('_traced_func_importing_caller',)),
                (self.filemod + ('_traced_func_importing',))): 1,
            ((self.filemod + ('_traced_func_importing',)),
                ('fakefile.py', 'fakefile', 'foo')): 1,
        }
        self.assertEqual(self.tr.results().callers, expected)


#------------------------------ Driver ---------------------------------------#

def test_main():
    print(__name__, type(__name__))
    test_support.run_unittest(__name__)


if __name__ == '__main__':
    test_main()
