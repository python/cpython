import compiler
import os
import test.test_support
import unittest

class CompilerTest(unittest.TestCase):

    def testCompileLibrary(self):
        # A simple but large test.  Compile all the code in the
        # standard library and its test suite.  This doesn't verify
        # that any of the code is correct, merely the compiler is able
        # to generate some kind of code for it.

        libdir = os.path.dirname(unittest.__file__)
        testdir = os.path.dirname(test.test_support.__file__)

        for dir in [libdir, testdir]:
            for path in os.listdir(dir):
                if not path.endswith(".py"):
                    continue
                f = open(os.path.join(dir, path), "r")
                buf = f.read()
                f.close()
                compiler.compile(buf, path, "exec")

def test_main():
    test.test_support.requires("compiler")
    test.test_support.run_unittest(CompilerTest)

if __name__ == "__main__":
    test_main()


        
