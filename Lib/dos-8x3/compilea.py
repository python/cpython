"""Module/script to "compile" all .py files to .pyc (or .pyo) file.

When called as a script with arguments, this compiles the directories
given as arguments recursively; the -l option prevents it from
recursing into directories.

Without arguments, if compiles all modules on sys.path, without
recursing into subdirectories.  (Even though it should do so for
packages -- for now, you'll have to deal with packages separately.)

See module py_compile for details of the actual byte-compilation.

"""

import os
import sys
import py_compile

def compile_dir(dir, maxlevels=10, ddir=None):
    """Byte-compile all modules in the given directory tree.

    Arguments (only dir is required):

    dir:       the directory to byte-compile
    maxlevels: maximum recursion level (default 10)
    ddir:      if given, purported directory name (this is the
               directory name that will show up in error messages)

    """
    print 'Listing', dir, '...'
    try:
        names = os.listdir(dir)
    except os.error:
        print "Can't list", dir
        names = []
    names.sort()
    for name in names:
        fullname = os.path.join(dir, name)
        if ddir:
            dfile = os.path.join(ddir, name)
        else:
            dfile = None
        if os.path.isfile(fullname):
            head, tail = name[:-3], name[-3:]
            if tail == '.py':
                print 'Compiling', fullname, '...'
                try:
                    py_compile.compile(fullname, None, dfile)
                except KeyboardInterrupt:
                    raise KeyboardInterrupt
                except:
                    if type(sys.exc_type) == type(''):
                        exc_type_name = sys.exc_type
                    else: exc_type_name = sys.exc_type.__name__
                    print 'Sorry:', exc_type_name + ':',
                    print sys.exc_value
        elif maxlevels > 0 and \
             name != os.curdir and name != os.pardir and \
             os.path.isdir(fullname) and \
             not os.path.islink(fullname):
            compile_dir(fullname, maxlevels - 1, dfile)

def compile_path(skip_curdir=1, maxlevels=0):
    """Byte-compile all module on sys.path.

    Arguments (all optional):

    skip_curdir: if true, skip current directory (default true)
    maxlevels:   max recursion level (default 0)

    """
    for dir in sys.path:
        if (not dir or dir == os.curdir) and skip_curdir:
            print 'Skipping current directory'
        else:
            compile_dir(dir, maxlevels)

def main():
    """Script main program."""
    import getopt
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'ld:')
    except getopt.error, msg:
        print msg
        print "usage: compileall [-l] [-d destdir] [directory ...]"
        print "-l: don't recurse down"
        print "-d destdir: purported directory name for error messages"
        print "if no arguments, -l sys.path is assumed"
        sys.exit(2)
    maxlevels = 10
    ddir = None
    for o, a in opts:
        if o == '-l': maxlevels = 0
        if o == '-d': ddir = a
    if ddir:
        if len(args) != 1:
            print "-d destdir require exactly one directory argument"
            sys.exit(2)
    try:
        if args:
            for dir in args:
                compile_dir(dir, maxlevels, ddir)
        else:
            compile_path()
    except KeyboardInterrupt:
        print "\n[interrupt]"

if __name__ == '__main__':
    main()
