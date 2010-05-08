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
import errno
import sys
import py_compile
import struct
import imp

__all__ = ["compile_dir","compile_file","compile_path"]

def compile_dir(dir, maxlevels=10, ddir=None,
                force=False, rx=None, quiet=False, legacy=False):
    """Byte-compile all modules in the given directory tree.

    Arguments (only dir is required):

    dir:       the directory to byte-compile
    maxlevels: maximum recursion level (default 10)
    ddir:      if given, purported directory name (this is the
               directory name that will show up in error messages)
    force:     if True, force compilation, even if timestamps are up-to-date
    quiet:     if True, be quiet during compilation
    legacy:    if True, produce legacy pyc paths instead of PEP 3147 paths

    """
    if not quiet:
        print('Listing', dir, '...')
    try:
        names = os.listdir(dir)
    except os.error:
        print("Can't list", dir)
        names = []
    names.sort()
    success = 1
    for name in names:
        if name == '__pycache__':
            continue
        fullname = os.path.join(dir, name)
        if ddir is not None:
            dfile = os.path.join(ddir, name)
        else:
            dfile = None
        if not os.path.isdir(fullname):
            if not compile_file(fullname, ddir, force, rx, quiet, legacy):
                success = 0
        elif maxlevels > 0 and \
             name != os.curdir and name != os.pardir and \
             os.path.isdir(fullname) and \
             not os.path.islink(fullname):
            if not compile_dir(fullname, maxlevels - 1, dfile, force, rx,
                               quiet, legacy):
                success = 0
    return success

def compile_file(fullname, ddir=None, force=0, rx=None, quiet=False,
                 legacy=False):
    """Byte-compile file.
    fullname:  the file to byte-compile
    ddir:      if given, purported directory name (this is the
               directory name that will show up in error messages)
    force:     if True, force compilation, even if timestamps are up-to-date
    quiet:     if True, be quiet during compilation
    legacy:    if True, produce legacy pyc paths instead of PEP 3147 paths

    """
    success = 1
    name = os.path.basename(fullname)
    if ddir is not None:
        dfile = os.path.join(ddir, name)
    else:
        dfile = None
    if rx is not None:
        mo = rx.search(fullname)
        if mo:
            return success
    if os.path.isfile(fullname):
        if legacy:
            cfile = fullname + ('c' if __debug__ else 'o')
        else:
            cfile = imp.cache_from_source(fullname)
            cache_dir = os.path.dirname(cfile)
        head, tail = name[:-3], name[-3:]
        if tail == '.py':
            if not force:
                try:
                    mtime = int(os.stat(fullname).st_mtime)
                    expect = struct.pack('<4sl', imp.get_magic(), mtime)
                    with open(cfile, 'rb') as chandle:
                        actual = chandle.read(8)
                    if expect == actual:
                        return success
                except IOError:
                    pass
            if not quiet:
                print('Compiling', fullname, '...')
            try:
                ok = py_compile.compile(fullname, cfile, dfile, True)
            except py_compile.PyCompileError as err:
                if quiet:
                    print('*** Error compiling', fullname, '...')
                else:
                    print('*** ', end='')
                # escape non-printable characters in msg
                msg = err.msg.encode(sys.stdout.encoding,
                                     errors='backslashreplace')
                msg = msg.decode(sys.stdout.encoding)
                print(msg)
                success = 0
            except (SyntaxError, UnicodeError, IOError) as e:
                if quiet:
                    print('*** Error compiling', fullname, '...')
                else:
                    print('*** ', end='')
                print(e.__class__.__name__ + ':', e)
                success = 0
            else:
                if ok == 0:
                    success = 0
    return success

def compile_path(skip_curdir=1, maxlevels=0, force=False, quiet=False,
                 legacy=False):
    """Byte-compile all module on sys.path.

    Arguments (all optional):

    skip_curdir: if true, skip current directory (default true)
    maxlevels:   max recursion level (default 0)
    force: as for compile_dir() (default False)
    quiet: as for compile_dir() (default False)
    legacy: as for compile_dir() (default False)

    """
    success = 1
    for dir in sys.path:
        if (not dir or dir == os.curdir) and skip_curdir:
            print('Skipping current directory')
        else:
            success = success and compile_dir(dir, maxlevels, None,
                                              force, quiet=quiet,
                                              legacy=legacy)
    return success

def expand_args(args, flist):
    """read names in flist and append to args"""
    expanded = args[:]
    if flist:
        try:
            if flist == '-':
                fd = sys.stdin
            else:
                fd = open(flist)
            while 1:
                line = fd.readline()
                if not line:
                    break
                expanded.append(line[:-1])
        except IOError:
            print("Error reading file list %s" % flist)
            raise
    return expanded

def main():
    """Script main program."""
    import getopt
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'lfqd:x:i:b')
    except getopt.error as msg:
        print(msg)
        print("usage: python compileall.py [-l] [-f] [-q] [-d destdir] "
              "[-x regexp] [-i list] [directory|file ...]")
        print("-l: don't recurse down")
        print("-f: force rebuild even if timestamps are up-to-date")
        print("-q: quiet operation")
        print("-d destdir: purported directory name for error messages")
        print("   if no directory arguments, -l sys.path is assumed")
        print("-x regexp: skip files matching the regular expression regexp")
        print("   the regexp is searched for in the full path of the file")
        print("-i list: expand list with its content "
              "(file and directory names)")
        print("-b: Produce legacy byte-compile file paths")
        sys.exit(2)
    maxlevels = 10
    ddir = None
    force = False
    quiet = False
    rx = None
    flist = None
    legacy = False
    for o, a in opts:
        if o == '-l': maxlevels = 0
        if o == '-d': ddir = a
        if o == '-f': force = True
        if o == '-q': quiet = True
        if o == '-x':
            import re
            rx = re.compile(a)
        if o == '-i': flist = a
        if o == '-b': legacy = True
    if ddir:
        if len(args) != 1 and not os.path.isdir(args[0]):
            print("-d destdir require exactly one directory argument")
            sys.exit(2)
    success = 1
    try:
        if args or flist:
            try:
                if flist:
                    args = expand_args(args, flist)
            except IOError:
                success = 0
            if success:
                for arg in args:
                    if os.path.isdir(arg):
                        if not compile_dir(arg, maxlevels, ddir,
                                           force, rx, quiet, legacy):
                            success = 0
                    else:
                        if not compile_file(arg, ddir, force, rx,
                                            quiet, legacy):
                            success = 0
        else:
            success = compile_path(legacy=legacy)
    except KeyboardInterrupt:
        print("\n[interrupt]")
        success = 0
    return success

if __name__ == '__main__':
    exit_status = int(not main())
    sys.exit(exit_status)
