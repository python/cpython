"""Recursively copy a directory but skip undesired files and
directories (CVS, backup files, pyc files, etc)"""

import sys
import os
import shutil

verbose = 1
debug = 0

def isclean(name):
    if name == 'CVS': return 0
    if name == '.cvsignore': return 0
    if name == '.DS_store': return 0
    if name.endswith('~'): return 0
    if name.endswith('.BAK'): return 0
    if name.endswith('.pyc'): return 0
    if name.endswith('.pyo'): return 0
    if name.endswith('.orig'): return 0
    return 1

def copycleandir(src, dst):
    for cursrc, dirs, files in os.walk(src):
        assert cursrc.startswith(src)
        curdst = dst + cursrc[len(src):]
        if verbose:
            print "mkdir", curdst
        if not debug:
            if not os.path.exists(curdst):
                os.makedirs(curdst)
        for fn in files:
            if isclean(fn):
                if verbose:
                    print "copy", os.path.join(cursrc, fn), os.path.join(curdst, fn)
                if not debug:
                    shutil.copy2(os.path.join(cursrc, fn), os.path.join(curdst, fn))
            else:
                if verbose:
                    print "skipfile", os.path.join(cursrc, fn)
        for i in range(len(dirs)-1, -1, -1):
            if not isclean(dirs[i]):
                if verbose:
                    print "skipdir", os.path.join(cursrc, dirs[i])
                del dirs[i]

def main():
    if len(sys.argv) != 3:
        sys.stderr.write("Usage: %s srcdir dstdir\n" % sys.argv[0])
        sys.exit(1)
    copycleandir(sys.argv[1], sys.argv[2])

if __name__ == '__main__':
    main()
