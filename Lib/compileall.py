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
import stat
import sys
import py_compile

__all__ = ["compile_dir","compile_path"]

def compile_dir(dir, maxlevels=10, ddir=None, force=0, rx=None):
    """Byte-compile all modules in the given directory tree.

    Arguments (only dir is required):

    dir:       the directory to byte-compile
    maxlevels: maximum recursion level (default 10)
    ddir:      if given, purported directory name (this is the
               directory name that will show up in error messages)
    force:     if 1, force compilation, even if timestamps are up-to-date

    """
    print 'Listing', dir, '...'
    try:
        names = os.listdir(dir)
    except os.error:
        print "Can't list", dir
        names = []
    names.sort()
    success = 1
    for name in names:
        fullname = os.path.join(dir, name)
        if ddir:
            dfile = os.path.join(ddir, name)
        else:
            dfile = None
        if rx:
            mo = rx.search(fullname)
            if mo:
                continue
        if os.path.isfile(fullname):
            head, tail = name[:-3], name[-3:]
            if tail == '.py':
                cfile = fullname + (__debug__ and 'c' or 'o')
                ftime = os.stat(fullname)[stat.ST_MTIME]
                try: ctime = os.stat(cfile)[stat.ST_MTIME]
                except os.error: ctime = 0
                if (ctime > ftime) and not force: continue
                print 'Compiling', fullname, '...'
                try:
                    ok = py_compile.compile(fullname, None, dfile)
                except KeyboardInterrupt:
                    raise KeyboardInterrupt
                except:
                    # XXX py_compile catches SyntaxErrors
                    if type(sys.exc_type) == type(''):
                        exc_type_name = sys.exc_type
                    else: exc_type_name = sys.exc_type.__name__
                    print 'Sorry:', exc_type_name + ':',
                    print sys.exc_value
                    success = 0
                else:
                    if ok == 0:
                        success = 0
        elif maxlevels > 0 and \
             name != os.curdir and name != os.pardir and \
             os.path.isdir(fullname) and \
             not os.path.islink(fullname):
            if not compile_dir(fullname, maxlevels - 1, dfile, force, rx):
                success = 0
    return success

def compile_path(skip_curdir=1, maxlevels=0, force=0):
    """Byte-compile all module on sys.path.

    Arguments (all optional):

    skip_curdir: if true, skip current directory (default true)
    maxlevels:   max recursion level (default 0)
    force: as for compile_dir() (default 0)

    """
    success = 1
    for dir in sys.path:
        if (not dir or dir == os.curdir) and skip_curdir:
            print 'Skipping current directory'
        else:
            success = success and compile_dir(dir, maxlevels, None, force)
    return success

def main():
    """Script main program."""
    import getopt
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'lfd:x:')
    except getopt.error, msg:
        print msg
        print "usage: python compileall.py [-l] [-f] [-d destdir] " \
              "[-s regexp] [directory ...]"
        print "-l: don't recurse down"
        print "-f: force rebuild even if timestamps are up-to-date"
        print "-d destdir: purported directory name for error messages"
        print "   if no directory arguments, -l sys.path is assumed"
        print "-x regexp: skip files matching the regular expression regexp"
        print "   the regexp is search for in the full path of the file"
        sys.exit(2)
    maxlevels = 10
    ddir = None
    force = 0
    rx = None
    for o, a in opts:
        if o == '-l': maxlevels = 0
        if o == '-d': ddir = a
        if o == '-f': force = 1
        if o == '-x':
            import re
            rx = re.compile(a)
    if ddir:
        if len(args) != 1:
            print "-d destdir require exactly one directory argument"
            sys.exit(2)
    success = 1
    try:
        if args:
            for dir in args:
                if not compile_dir(dir, maxlevels, ddir, force, rx):
                    success = 0
        else:
            success = compile_path()
    except KeyboardInterrupt:
        print "\n[interrupt]"
        success = 0
    return success

if __name__ == '__main__':
    exit_status = not main()
    sys.exit(exit_status)
