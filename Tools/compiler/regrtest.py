"""Run the Python regression test using the compiler

This test runs the standard Python test suite using bytecode generated
by this compiler instead of by the builtin compiler.

The regression test is run with the interpreter in verbose mode so
that import problems can be observed easily.
"""

from compiler import compile

import os
import sys
import test
import tempfile

def copy_test_suite():
    dest = tempfile.mktemp()
    os.mkdir(dest)
    os.system("cp -r %s/* %s" % (test.__path__[0], dest))
    print "Creating copy of test suite in", dest
    return dest

def compile_files(dir):
    print "Compiling",
    line_len = 10
    for file in os.listdir(dir):
        base, ext = os.path.splitext(file)
        if ext == '.py' and base[:4] == 'test':
            source = os.path.join(dir, file)
            line_len = line_len + len(file) + 1
            if line_len > 75:
                print "\n\t",
                line_len = len(source) + 9
            print file,
            compile(source)
            # make sure the .pyc file is not over-written
            os.chmod(source + "c", 444)
    print

def run_regrtest(test_dir):
    os.chdir(test_dir)
    os.system("%s -v regrtest.py" % sys.executable)

def cleanup(dir):
    os.system("rm -rf %s" % dir)

def main():
    test_dir = copy_test_suite()
    compile_files(test_dir)
    run_regrtest(test_dir)
    cleanup(test_dir)

if __name__ == "__main__":
    main()
