import compiler
import os
import test.test_support
import unittest
from random import random

class CompilerTest(unittest.TestCase):

    def testCompileLibrary(self):
        # A simple but large test.  Compile all the code in the
        # standard library and its test suite.  This doesn't verify
        # that any of the code is correct, merely the compiler is able
        # to generate some kind of code for it.

        libdir = os.path.dirname(unittest.__file__)
        testdir = os.path.dirname(test.test_support.__file__)

        for dir in [libdir, testdir]:
            for basename in os.listdir(dir):
                if not basename.endswith(".py"):
                    continue
                if not TEST_ALL and random() < 0.98:
                    continue
                path = os.path.join(dir, basename)
                if test.test_support.verbose:
                    print "compiling", path
                f = open(path, "U")
                buf = f.read()
                f.close()
                if "badsyntax" in basename:
                    self.assertRaises(SyntaxError, compiler.compile,
                                      buf, basename, "exec")
                else:
                    compiler.compile(buf, basename, "exec")

    def testLineNo(self):
        # Test that all nodes except Module have a correct lineno attribute.
        filename = __file__
        if filename.endswith(".pyc") or filename.endswith(".pyo"):
            filename = filename[:-1]
        tree = compiler.parseFile(filename)
        self.check_lineno(tree)

    def check_lineno(self, node):
        try:
            self._check_lineno(node)
        except AssertionError:
            print node.__class__, node.lineno
            raise

    def _check_lineno(self, node):
        if not node.__class__ in NOLINENO:
            self.assert_(isinstance(node.lineno, int),
                "lineno=%s on %s" % (node.lineno, node.__class__))
            self.assert_(node.lineno > 0,
                "lineno=%s on %s" % (node.lineno, node.__class__))
        for child in node.getChildNodes():
            self.check_lineno(child)

NOLINENO = (compiler.ast.Module, compiler.ast.Stmt, compiler.ast.Discard)

###############################################################################
# code below is just used to trigger some possible errors, for the benefit of
# testLineNo
###############################################################################

class Toto:
    """docstring"""
    pass

a, b = 2, 3
[c, d] = 5, 6
l = [(x, y) for x, y in zip(range(5), range(5,10))]
l[0]
l[3:4]
if l:
    pass
else:
    a, b = b, a

try:
    print yo
except:
    yo = 3
else:
    yo += 3

try:
    a += b
finally:
    b = 0

from math import *

###############################################################################

def test_main():
    global TEST_ALL
    TEST_ALL = test.test_support.is_resource_enabled("compiler")
    test.test_support.run_unittest(CompilerTest)

if __name__ == "__main__":
    test_main()
