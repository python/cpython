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
                f = open(path)
                buf = f.read()
                f.close()
                if "badsyntax" in basename:
                    self.assertRaises(SyntaxError, compiler.compile,
                                      buf, basename, "exec")
                else:
                    compiler.compile(buf, basename, "exec")

def test_main():
    global TEST_ALL
    TEST_ALL = test.test_support.is_resource_enabled("compiler")
    test.test_support.run_unittest(CompilerTest)

if __name__ == "__main__":
    test_main()
