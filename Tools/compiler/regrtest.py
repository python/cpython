"""Run the Python regression test using the compiler

This test runs the standard Python test suite using bytecode generated
by this compiler instead of by the builtin compiler.

The regression test is run with the interpreter in verbose mode so
that import problems can be observed easily.
"""

from compiler import compileFile

import os
import sys
import test
import tempfile

def copy_test_suite():
    dest = tempfile.mkdtemp()
    os.system("cp -r %s/* %s" % (test.__path__[0], dest))
    print "Creating copy of test suite in", dest
    return dest

def copy_library():
    dest = tempfile.mkdtemp()
    libdir = os.path.split(test.__path__[0])[0]
    print "Found standard library in", libdir
    print "Creating copy of standard library in", dest
    os.system("cp -r %s/* %s" % (libdir, dest))
    return dest

def compile_files(dir):
    print "Compiling", dir, "\n\t",
    line_len = 10
    for file in os.listdir(dir):
        base, ext = os.path.splitext(file)
        if ext == '.py':
            source = os.path.join(dir, file)
            line_len = line_len + len(file) + 1
            if line_len > 75:
                print "\n\t",
                line_len = len(source) + 9
            print file,
            try:
                compileFile(source)
            except SyntaxError, err:
                print err
                continue
            # make sure the .pyc file is not over-written
            os.chmod(source + "c", 444)
        elif file == 'CVS':
            pass
        else:
            path = os.path.join(dir, file)
            if os.path.isdir(path):
                print
                print
                compile_files(path)
                print "\t",
                line_len = 10
    print

def run_regrtest(lib_dir):
    test_dir = os.path.join(lib_dir, "test")
    os.chdir(test_dir)
    os.system("PYTHONPATH=%s %s -v regrtest.py" % (lib_dir, sys.executable))

def cleanup(dir):
    os.system("rm -rf %s" % dir)

def main():
    lib_dir = copy_library()
    compile_files(lib_dir)
    run_regrtest(lib_dir)
    raw_input("Cleanup?")
    cleanup(lib_dir)

if __name__ == "__main__":
    main()
