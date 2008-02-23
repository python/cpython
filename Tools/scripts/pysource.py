#!/usr/bin/env python

"""\
List python source files.

There are three functions to check whether a file is a Python source, listed
here with increasing complexity:

- has_python_ext() checks whether a file name ends in '.py[w]'.
- look_like_python() checks whether the file is not binary and either has
  the '.py[w]' extension or the first line contains the word 'python'.
- can_be_compiled() checks whether the file can be compiled by compile().

The file also must be of appropriate size - not bigger than a megabyte.

walk_python_files() recursively lists all Python files under the given directories.
"""
__author__ = "Oleg Broytmann, Georg Brandl"

__all__ = ["has_python_ext", "looks_like_python", "can_be_compiled", "walk_python_files"]


import os, re

binary_re = re.compile('[\x00-\x08\x0E-\x1F\x7F]')

debug = False

def print_debug(msg):
    if debug: print msg


def _open(fullpath):
    try:
        size = os.stat(fullpath).st_size
    except OSError, err: # Permission denied - ignore the file
        print_debug("%s: permission denied: %s" % (fullpath, err))
        return None

    if size > 1024*1024: # too big
        print_debug("%s: the file is too big: %d bytes" % (fullpath, size))
        return None

    try:
        return open(fullpath, 'rU')
    except IOError, err: # Access denied, or a special file - ignore it
        print_debug("%s: access denied: %s" % (fullpath, err))
        return None

def has_python_ext(fullpath):
    return fullpath.endswith(".py") or fullpath.endswith(".pyw")

def looks_like_python(fullpath):
    infile = _open(fullpath)
    if infile is None:
        return False

    line = infile.readline()
    infile.close()

    if binary_re.search(line):
        # file appears to be binary
        print_debug("%s: appears to be binary" % fullpath)
        return False

    if fullpath.endswith(".py") or fullpath.endswith(".pyw"):
        return True
    elif "python" in line:
        # disguised Python script (e.g. CGI)
        return True

    return False

def can_be_compiled(fullpath):
    infile = _open(fullpath)
    if infile is None:
        return False

    code = infile.read()
    infile.close()

    try:
        compile(code, fullpath, "exec")
    except Exception, err:
        print_debug("%s: cannot compile: %s" % (fullpath, err))
        return False

    return True


def walk_python_files(paths, is_python=looks_like_python, exclude_dirs=None):
    """\
    Recursively yield all Python source files below the given paths.

    paths: a list of files and/or directories to be checked.
    is_python: a function that takes a file name and checks whether it is a
               Python source file
    exclude_dirs: a list of directory base names that should be excluded in
                  the search
    """
    if exclude_dirs is None:
        exclude_dirs=[]

    for path in paths:
        print_debug("testing: %s" % path)
        if os.path.isfile(path):
            if is_python(path):
                yield path
        elif os.path.isdir(path):
            print_debug("    it is a directory")
            for dirpath, dirnames, filenames in os.walk(path):
                for exclude in exclude_dirs:
                    if exclude in dirnames:
                        dirnames.remove(exclude)
                for filename in filenames:
                    fullpath = os.path.join(dirpath, filename)
                    print_debug("testing: %s" % fullpath)
                    if is_python(fullpath):
                        yield fullpath
        else:
            print_debug("    unknown type")


if __name__ == "__main__":
    # Two simple examples/tests
    for fullpath in walk_python_files(['.']):
        print fullpath
    print "----------"
    for fullpath in walk_python_files(['.'], is_python=can_be_compiled):
        print fullpath
