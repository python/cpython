"""Module/script to byte-compile all .py files to .pyc (or .pyo) files.

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
import errno
import imp
import py_compile
import struct

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
        elif (maxlevels > 0 and name != os.curdir and name != os.pardir and
              os.path.isdir(fullname) and not os.path.islink(fullname)):
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


def main():
    """Script main program."""
    import argparse

    parser = argparse.ArgumentParser(
        description='Utilities to support installing Python libraries.')
    parser.add_argument('-l', action='store_const', default=10, const=0,
                        dest='maxlevels', help="don't recurse down")
    parser.add_argument('-f', action='store_true', dest='force',
                        help='force rebuild even if timestamps are up to date')
    parser.add_argument('-q', action='store_true', dest='quiet',
                        help='reduce output')
    parser.add_argument('-b', action='store_true', dest='legacy',
                        help='produce legacy byte-compiled file paths')
    parser.add_argument('-d', metavar='DESTDIR',  dest='ddir', default=None,
                        help=('purported directory name for error messages; '
                              'if no directory arguments, -l sys.path '
                              'is assumed.'))
    parser.add_argument('-x', metavar='REGEXP', dest='rx', default=None,
                        help=('skip files matching the regular expression.\n\t'
                              'The regexp is searched for in the full path '
                              'of the file'))
    parser.add_argument('-i', metavar='FILE', dest='flist',
                        help='expand the list with the content of FILE.')
    parser.add_argument('compile_dest', metavar='FILE|DIR', nargs='?')
    args = parser.parse_args()

    if (args.ddir and args.compile_dest != 1 and
        not os.path.isdir(args.compile_dest)):
        raise argparse.ArgumentError(
            "-d destdir requires exactly one directory argument")
    if args.rx:
        import re
        args.rx = re.compile(args.rx)

    # if flist is provided then load it
    compile_dests = [args.compile_dest]
    if args.flist:
        with open(args.flist) as f:
            files = f.read().split()
            compile_dests.extend(files)

    try:
        if compile_dests:
            for dest in compile_dests:
                if os.path.isdir(dest):
                    if not compile_dir(dest, args.maxlevels, args.ddir,
                                       args.force, args.rx, args.quiet,
                                       args.legacy):
                        return 0
                else:
                    if not compile_file(dest, args.ddir, args.force, args.rx,
                                        args.quiet, args.legacy):
                        return 0
        else:
            return compile_path(legacy=args.legacy)
    except KeyboardInterrupt:
        print("\n[interrupted]")
        return 0
    return 1


if __name__ == '__main__':
    exit_status = int(not main())
    sys.exit(exit_status)
